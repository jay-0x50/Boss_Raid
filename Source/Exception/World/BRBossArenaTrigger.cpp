#include "BRBossArenaTrigger.h"

#include "Boss/Base/BRBossBase.h"
#include "Boss/Team/BRBossTeamCoordinator.h"
#include "BRBossStatusWidget.h"
#include "Player/Character/ExceptionCharacter.h"
#include "ExceptionGameMode.h"
#include "Player/Controller/ExceptionPlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABRBossArenaTrigger::ABRBossArenaTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 120.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(RootComponent);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetWorldScale3D(FVector(4.0f, 4.0f, 0.05f));
	PreviewMesh->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PreviewMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void ABRBossArenaTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABRBossArenaTrigger::OnTriggerBeginOverlap);
	}

	TArray<ABRBossBase*> ManagedBosses;
	BuildManagedBossList(ManagedBosses);
	for (ABRBossBase* Boss : ManagedBosses)
	{
		BindBossEvents(Boss);
		if (Boss)
		{
			Boss->PrepareForArenaInactive();
		}
	}

	if (RewardActorToShowOnDefeat)
	{
		RewardActorToShowOnDefeat->SetActorHiddenInGame(true);
		RewardActorToShowOnDefeat->SetActorEnableCollision(false);
	}

	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (!bStartOnPlayerOverlap || bArenaStarted || bArenaCleared || !TriggerBox)
		{
			return;
		}

		TriggerBox->UpdateOverlaps();

		TArray<AActor*> OverlappingActors;
		TriggerBox->GetOverlappingActors(OverlappingActors, AExceptionCharacter::StaticClass());
		if (!OverlappingActors.IsEmpty())
		{
			StartArena();
		}
	}));

	for (const float Delay : {0.25f, 0.75f, 1.5f})
	{
		FTimerHandle RetryOverlapTimerHandle;
		GetWorldTimerManager().SetTimer(
			RetryOverlapTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (!bStartOnPlayerOverlap || bArenaStarted || bArenaCleared || !TriggerBox)
				{
					return;
				}

				TriggerBox->UpdateOverlaps();

				TArray<AActor*> OverlappingActors;
				TriggerBox->GetOverlappingActors(OverlappingActors, AExceptionCharacter::StaticClass());
				if (!OverlappingActors.IsEmpty())
				{
					StartArena();
				}
			}),
			Delay,
			false);
	}
}

void ABRBossArenaTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bStartOnPlayerOverlap || bArenaStarted || bArenaCleared || !Cast<AExceptionCharacter>(OtherActor))
	{
		return;
	}

	ActivateArena();
}

void ABRBossArenaTrigger::ActivateArena()
{
	StartArena();
}

void ABRBossArenaTrigger::StartArena()
{
	if (bArenaStarted || bArenaCleared)
	{
		return;
	}

	bArenaStarted = true;

	if (AExceptionGameMode* ExceptionGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AExceptionGameMode>() : nullptr)
	{
		ExceptionGameMode->SetActiveBossArena(this);
	}

	SpawnConfiguredBossIfNeeded();

	TArray<ABRBossBase*> ManagedBosses;
	BuildManagedBossList(ManagedBosses);

	if (bDeactivateUnmanagedBossesOnStart)
	{
		DeactivateUnmanagedBosses(ManagedBosses);
	}

	if (ManagedBosses.IsEmpty() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(4005, 4.0f, FColor::Yellow, TEXT("Boss Arena has no boss. Set Boss Actors or Boss Class To Spawn."));
	}

	if (!bHideBossStatusUntilIntroFinished)
	{
		if (UBRBossStatusWidget* ActiveBossStatusWidget = ShowBossStatusWidget())
		{
			ActiveBossStatusWidget->ClearBosses();
			ActiveBossStatusWidget->SetBossCount(ManagedBosses.Num());
		}
	}

	for (ABRBossBase* Boss : ManagedBosses)
	{
		if (!Boss)
		{
			continue;
		}

		if (bResetBossOnEnter)
		{
			Boss->ResetBoss();
		}

		Boss->StartBossIntro();
	}

	if (bPlayBossIntroBeforeAI && BossIntroDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(BossIntroTimerHandle, this, &ABRBossArenaTrigger::ActivateManagedBossesAfterIntro, BossIntroDelay, false);
	}
	else
	{
		ActivateManagedBossesAfterIntro();
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(4001, 2.0f, FColor::Red, TEXT("Boss Arena Started"));
	}
}

void ABRBossArenaTrigger::ActivateManagedBossesAfterIntro()
{
	if (!bArenaStarted || bArenaCleared)
	{
		return;
	}

	TArray<ABRBossBase*> ManagedBosses;
	BuildManagedBossList(ManagedBosses);

	if (bHideBossStatusUntilIntroFinished)
	{
		if (UBRBossStatusWidget* ActiveBossStatusWidget = ShowBossStatusWidget())
		{
			ActiveBossStatusWidget->ClearBosses();
			ActiveBossStatusWidget->SetBossCount(ManagedBosses.Num());
		}
	}

	for (ABRBossBase* Boss : ManagedBosses)
	{
		if (Boss)
		{
			Boss->SetCombatAIEnabled(true);
		}
	}

	RefreshBossStatusWidget();
}

ABRBossBase* ABRBossArenaTrigger::SpawnConfiguredBossIfNeeded()
{
	if (!bSpawnBossOnArenaStart || !BossClassToSpawn || !GetWorld())
	{
		return nullptr;
	}

	for (ABRBossBase* SpawnedBoss : SpawnedBosses)
	{
		if (IsValid(SpawnedBoss) && !SpawnedBoss->IsDead())
		{
			return SpawnedBoss;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABRBossBase* SpawnedBoss = GetWorld()->SpawnActor<ABRBossBase>(
		BossClassToSpawn,
		GetConfiguredBossSpawnTransform(),
		SpawnParameters);

	if (!SpawnedBoss)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(4006, 4.0f, FColor::Red, TEXT("Boss spawn failed. Check Boss Class To Spawn."));
		}
		return nullptr;
	}

	SpawnedBosses.AddUnique(SpawnedBoss);
	BindBossEvents(SpawnedBoss);

	if (bResetBossOnEnter)
	{
		SpawnedBoss->ResetBoss();
	}

	SpawnedBoss->PrepareForArenaInactive();

	return SpawnedBoss;
}

FTransform ABRBossArenaTrigger::GetConfiguredBossSpawnTransform() const
{
	if (BossSpawnPoint)
	{
		return BossSpawnPoint->GetActorTransform();
	}

	FTransform SpawnTransform = GetActorTransform();
	SpawnTransform.AddToTranslation(GetActorRotation().RotateVector(BossSpawnOffset));
	return SpawnTransform;
}

void ABRBossArenaTrigger::BindBossEvents(ABRBossBase* Boss)
{
	if (!Boss)
	{
		return;
	}

	Boss->OnBossDead.AddUniqueDynamic(this, &ABRBossArenaTrigger::HandleBossDefeated);
	Boss->OnBossHPChanged.AddUniqueDynamic(this, &ABRBossArenaTrigger::HandleBossStatChanged);
	Boss->OnBossGroggyChanged.AddUniqueDynamic(this, &ABRBossArenaTrigger::HandleBossStatChanged);
	Boss->OnBossGroggy.AddUniqueDynamic(this, &ABRBossArenaTrigger::HandleBossStateChanged);
	Boss->OnBossRecoveredFromGroggy.AddUniqueDynamic(this, &ABRBossArenaTrigger::HandleBossStateChanged);
	Boss->OnExecutionStarted.AddUniqueDynamic(this, &ABRBossArenaTrigger::HandleBossExecutionStateChanged);
	Boss->OnExecutionCompleted.AddUniqueDynamic(this, &ABRBossArenaTrigger::HandleBossExecutionStateChanged);
}

void ABRBossArenaTrigger::ResetArenaForRetry()
{
	if (bArenaCleared)
	{
		return;
	}

	bArenaStarted = false;
	GetWorldTimerManager().ClearTimer(BossIntroTimerHandle);
	HideBossStatusWidget();

	TArray<ABRBossBase*> ManagedBosses;
	BuildManagedBossList(ManagedBosses);
	for (ABRBossBase* Boss : ManagedBosses)
	{
		if (Boss)
		{
			Boss->SetCombatAIEnabled(false);
			Boss->ResetBoss();
			Boss->PrepareForArenaInactive();
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(4003, 2.0f, FColor::Silver, TEXT("Boss Arena Reset For Retry"));
	}
}

void ABRBossArenaTrigger::HandleBossDefeated()
{
	if (bArenaCleared)
	{
		return;
	}

	RefreshBossStatusWidget();

	if (!AreAllManagedBossesDead())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(4004, 2.0f, FColor::Orange, TEXT("Boss Down - Team Still Fighting"));
		}
		return;
	}

	bArenaCleared = true;
	GetWorldTimerManager().ClearTimer(BossIntroTimerHandle);
	HideBossStatusWidget();

	TArray<ABRBossBase*> ManagedBosses;
	BuildManagedBossList(ManagedBosses);
	for (ABRBossBase* Boss : ManagedBosses)
	{
		if (Boss)
		{
			Boss->SetCombatAIEnabled(false);
		}
	}

	if (GateActorToHideOnDefeat)
	{
		GateActorToHideOnDefeat->SetActorHiddenInGame(true);
		GateActorToHideOnDefeat->SetActorEnableCollision(false);
	}

	if (RewardActorToShowOnDefeat)
	{
		RewardActorToShowOnDefeat->SetActorHiddenInGame(false);
		RewardActorToShowOnDefeat->SetActorEnableCollision(true);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(4002, 3.0f, FColor::Green, TEXT("Boss Defeated - Path Opened"));
	}
}

void ABRBossArenaTrigger::HandleBossStatChanged(float CurrentValue, float MaxValue, float NormalizedValue)
{
	RefreshBossStatusWidget();
}

void ABRBossArenaTrigger::HandleBossStateChanged()
{
	RefreshBossStatusWidget();
}

void ABRBossArenaTrigger::HandleBossExecutionStateChanged(AActor* Executor)
{
	RefreshBossStatusWidget();
}

void ABRBossArenaTrigger::BuildManagedBossList(TArray<ABRBossBase*>& OutBosses) const
{
	OutBosses.Reset();

	const bool bHasConfiguredBosses = BossDummy || !BossActors.IsEmpty() || !SpawnedBosses.IsEmpty() || BossClassToSpawn;

	for (ABRBossBase* Boss : BossActors)
	{
		AddBossAndLinkedTeam(Boss, OutBosses, true);
	}

	AddBossAndLinkedTeam(BossDummy, OutBosses, true);

	for (ABRBossBase* SpawnedBoss : SpawnedBosses)
	{
		AddBossAndLinkedTeam(SpawnedBoss, OutBosses, true);
	}

	if (bAutoIncludeNearbyBosses && !bHasConfiguredBosses)
	{
		AddNearbyBosses(OutBosses);
	}
}

void ABRBossArenaTrigger::AddBossAndLinkedTeam(ABRBossBase* Boss, TArray<ABRBossBase*>& OutBosses, bool bIncludeLinkedTeam) const
{
	if (!Boss)
	{
		return;
	}

	OutBosses.AddUnique(Boss);

	if (bIncludeLinkedTeam && bAutoIncludeTeamMembers)
	{
		AddTeamMembers(Boss->GetTeamCoordinator(), OutBosses);
	}
}

void ABRBossArenaTrigger::AddTeamMembers(ABRBossTeamCoordinator* TeamCoordinator, TArray<ABRBossBase*>& OutBosses) const
{
	if (!TeamCoordinator)
	{
		return;
	}

	TArray<ABRBossBase*> TeamMembers;
	TeamCoordinator->GetTeamMembers(TeamMembers);
	for (ABRBossBase* TeamMember : TeamMembers)
	{
		if (TeamMember)
		{
			OutBosses.AddUnique(TeamMember);
		}
	}
}

void ABRBossArenaTrigger::AddNearbyBosses(TArray<ABRBossBase*>& OutBosses) const
{
	if (!GetWorld() || AutoBossSearchRadius <= 0.0f)
	{
		return;
	}

	TArray<AActor*> FoundBossActors;
	UGameplayStatics::GetAllActorsOfClass(this, ABRBossBase::StaticClass(), FoundBossActors);

	const float SearchRadiusSq = FMath::Square(AutoBossSearchRadius);
	for (AActor* FoundActor : FoundBossActors)
	{
		ABRBossBase* FoundBoss = Cast<ABRBossBase>(FoundActor);
		if (FoundBoss && FVector::DistSquared(FoundBoss->GetActorLocation(), GetActorLocation()) <= SearchRadiusSq)
		{
			AddBossAndLinkedTeam(FoundBoss, OutBosses, true);
		}
	}
}

void ABRBossArenaTrigger::DeactivateUnmanagedBosses(const TArray<ABRBossBase*>& ManagedBosses) const
{
	if (!GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundBossActors;
	UGameplayStatics::GetAllActorsOfClass(this, ABRBossBase::StaticClass(), FoundBossActors);
	for (AActor* FoundActor : FoundBossActors)
	{
		ABRBossBase* FoundBoss = Cast<ABRBossBase>(FoundActor);
		if (FoundBoss && !ManagedBosses.Contains(FoundBoss))
		{
			FoundBoss->SetCombatAIEnabled(false);
		}
	}
}

bool ABRBossArenaTrigger::AreAllManagedBossesDead() const
{
	TArray<ABRBossBase*> ManagedBosses;
	BuildManagedBossList(ManagedBosses);

	if (ManagedBosses.IsEmpty())
	{
		return false;
	}

	for (const ABRBossBase* Boss : ManagedBosses)
	{
		if (Boss && !Boss->IsDead())
		{
			return false;
		}
	}

	return true;
}

UBRBossStatusWidget* ABRBossArenaTrigger::ShowBossStatusWidget()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return nullptr;
	}

	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(PlayerController))
	{
		if (UBRBossStatusWidget* PlayerControllerWidget = ExceptionPC->ShowBossStatusWidget())
		{
			return PlayerControllerWidget;
		}
	}

	if (!BossStatusWidget)
	{
		BossStatusWidget = CreateWidget<UBRBossStatusWidget>(PlayerController, UBRBossStatusWidget::StaticClass());
	}

	if (BossStatusWidget && !BossStatusWidget->IsInViewport())
	{
		BossStatusWidget->AddToPlayerScreen(10);
	}

	return BossStatusWidget;
}

void ABRBossArenaTrigger::RefreshBossStatusWidget()
{
	if (!bArenaStarted || bArenaCleared)
	{
		return;
	}

	UBRBossStatusWidget* ActiveBossStatusWidget = ShowBossStatusWidget();
	if (!ActiveBossStatusWidget)
	{
		return;
	}

	TArray<ABRBossBase*> ManagedBosses;
	BuildManagedBossList(ManagedBosses);
	ActiveBossStatusWidget->SetBossCount(ManagedBosses.Num());

	for (int32 BossIndex = 0; BossIndex < ManagedBosses.Num(); ++BossIndex)
	{
		const ABRBossBase* Boss = ManagedBosses[BossIndex];
		if (!Boss)
		{
			continue;
		}

		ActiveBossStatusWidget->SetBossHP(
			BossIndex,
			Boss->GetBossDisplayName(),
			Boss->GetCurrentHP(),
			Boss->GetMaxHP(),
			Boss->GetHPPercent());

		ActiveBossStatusWidget->SetBossGroggy(
			BossIndex,
			Boss->GetCurrentGroggy(),
			Boss->GetMaxGroggy(),
			Boss->GetGroggyPercent());

		ActiveBossStatusWidget->SetBossGroggyState(BossIndex, Boss->IsGroggy());
		ActiveBossStatusWidget->SetBossExecutionState(BossIndex, Boss->CanBeExecuted());
	}
}

void ABRBossArenaTrigger::HideBossStatusWidget()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		ExceptionPC->HideBossStatusWidget();
	}

	if (BossStatusWidget)
	{
		BossStatusWidget->RemoveFromParent();
		BossStatusWidget->ClearBosses();
	}
}
