#include "Boss/Base/BRBossBase.h"

#include "Boss/AI/BRBossAIController.h"
#include "Boss/Team/BRBossTeamCoordinator.h"
#include "BRStatComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/World.h"

ABRBossBase::ABRBossBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ABRBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(VisualRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionObjectType(ECC_Pawn);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetVisibility(false);
	MeshComponent->SetHiddenInGame(true);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(VisualRoot);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetCollisionObjectType(ECC_Pawn);
	SkeletalMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	SkeletalMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SkeletalMeshComponent->SetGenerateOverlapEvents(false);
	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetHiddenInGame(true);

	StatComponent = CreateDefaultSubobject<UBRStatComponent>(TEXT("StatComponent"));
}

void ABRBossBase::BeginPlay()
{
	Super::BeginPlay();
	InitialBossTransform = GetActorTransform();

	if (!GetController())
	{
		SpawnDefaultController();
	}

	ApplyMeshVisualTransform();

	if (StatComponent)
	{
		StatComponent->ConfigureMaxStats(InitialMaxHP, 0.0f, InitialMaxGroggy);
		StatComponent->OnHPChanged.AddDynamic(this, &ABRBossBase::HandleHPChanged);
		StatComponent->OnGroggyChanged.AddDynamic(this, &ABRBossBase::HandleGroggyChanged);
		StatComponent->OnDead.AddDynamic(this, &ABRBossBase::HandleDead);
		StatComponent->OnGroggy.AddDynamic(this, &ABRBossBase::HandleGroggy);
	}

	if (TeamCoordinator)
	{
		TeamCoordinator->RegisterBoss(this);
	}

	ResetBoss();
	if (!bCombatAIEnabled)
	{
		PrepareForArenaInactive();
	}
}

void ABRBossBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TeamCoordinator)
	{
		TeamCoordinator->UnregisterBoss(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ABRBossBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyMeshVisualTransform();
}

void ABRBossBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ApplyGroundGravity(DeltaSeconds);

	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->RefreshBossBlackboard();
		if (!BossAIController->IsBehaviorTreeActive())
		{
			UpdateBossAI(DeltaSeconds);
		}
	}
	else
	{
		UpdateBossAI(DeltaSeconds);
	}

	DrawBossDebug();
}

void ABRBossBase::ApplyMeshVisualTransform()
{
	if (VisualRoot)
	{
		VisualRoot->SetRelativeLocation(FVector::ZeroVector);
		VisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
		VisualRoot->SetRelativeScale3D(FVector::OneVector);
	}

	const bool bHasStaticMesh = MeshComponent && MeshComponent->GetStaticMesh();
	const bool bHasSkeletalMesh = SkeletalMeshComponent && SkeletalMeshComponent->GetSkeletalMeshAsset();
	const bool bUseSkeletalMesh = bHasSkeletalMesh && (VisualMeshType == EBRBossVisualMeshType::SkeletalMesh || !bHasStaticMesh);
	const bool bUseStaticMesh = bHasStaticMesh && !bUseSkeletalMesh;

	if (MeshComponent)
	{
		MeshComponent->SetRelativeLocation(MeshRelativeLocation);
		MeshComponent->SetRelativeRotation(MeshRelativeRotation);
		MeshComponent->SetRelativeScale3D(MeshRelativeScale);
		MeshComponent->SetVisibility(bUseStaticMesh);
		MeshComponent->SetHiddenInGame(!bUseStaticMesh);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetGenerateOverlapEvents(false);
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetRelativeLocation(MeshRelativeLocation);
		SkeletalMeshComponent->SetRelativeRotation(MeshRelativeRotation);
		SkeletalMeshComponent->SetRelativeScale3D(MeshRelativeScale);
		SkeletalMeshComponent->SetVisibility(bUseSkeletalMesh);
		SkeletalMeshComponent->SetHiddenInGame(!bUseSkeletalMesh);
		SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkeletalMeshComponent->SetGenerateOverlapEvents(false);
		SetBossAnimationPlaying(bUseSkeletalMesh && bCombatAIEnabled && !bIsDead);
	}
}

void ABRBossBase::SetBossAnimationPlaying(bool bShouldPlay)
{
	if (!SkeletalMeshComponent || SkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		return;
	}

	if (bShouldPlay)
	{
		SkeletalMeshComponent->Play(true);
		return;
	}

	SkeletalMeshComponent->Stop();
	SkeletalMeshComponent->SetPosition(0.0f, false);
}

void ABRBossBase::PlayBossStageAnimation(EBRBossAnimationStage Stage, FName ActionName)
{
	if (!SkeletalMeshComponent || SkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		return;
	}

	UAnimationAsset* AnimationToPlay = nullptr;
	if (!ActionName.IsNone())
	{
		if (const TObjectPtr<UAnimationAsset>* FoundAnimation = ActionAnimations.Find(ActionName))
		{
			AnimationToPlay = FoundAnimation->Get();
		}
	}

	if (!AnimationToPlay)
	{
		if (const TObjectPtr<UAnimationAsset>* FoundAnimation = StageAnimations.Find(Stage))
		{
			AnimationToPlay = FoundAnimation->Get();
		}
	}

	if (!AnimationToPlay)
	{
		return;
	}

	const bool bLoopAnimation = Stage == EBRBossAnimationStage::Idle || Stage == EBRBossAnimationStage::Move || Stage == EBRBossAnimationStage::Groggy;
	if (CurrentBossAnimationAsset == AnimationToPlay && bLoopAnimation)
	{
		return;
	}

	SkeletalMeshComponent->SetAnimation(AnimationToPlay);
	SkeletalMeshComponent->Play(bLoopAnimation);
	CurrentBossAnimationAsset = AnimationToPlay;
}

void ABRBossBase::NotifyBossAnimationStage(EBRBossAnimationStage Stage, FName ActionName)
{
	PlayBossStageAnimation(Stage, ActionName);
	OnAnimationStageChanged.Broadcast(Stage, ActionName);
	BP_BossAnimationStageChanged(Stage, ActionName);
}

void ABRBossBase::ApplyGroundGravity(float DeltaSeconds)
{
	if (!bUseGroundGravity || DeltaSeconds <= 0.0f || !GetWorld())
	{
		return;
	}

	const FVector ActorLocation = GetActorLocation();
	const FVector TraceStart = ActorLocation;
	const FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, GroundTraceActorHalfHeight + GroundTraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossGroundGravity), false, this);
	FHitResult Hit;
	const bool bHitGround = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	if (bHitGround)
	{
		const float DesiredActorZ = Hit.Location.Z + GroundTraceActorHalfHeight;
		const float HeightDelta = ActorLocation.Z - DesiredActorZ;

		if (HeightDelta > GroundSnapTolerance)
		{
			VerticalFallSpeed = FMath::Max(VerticalFallSpeed + GroundGravity * DeltaSeconds, 0.0f);
			const float FallDistance = FMath::Min(VerticalFallSpeed * DeltaSeconds, HeightDelta);
			AddActorWorldOffset(FVector(0.0f, 0.0f, -FallDistance), true);
			return;
		}

		if (FMath::Abs(HeightDelta) > KINDA_SMALL_NUMBER)
		{
			SetActorLocation(FVector(ActorLocation.X, ActorLocation.Y, DesiredActorZ), true);
		}

		VerticalFallSpeed = 0.0f;
		return;
	}

	VerticalFallSpeed = FMath::Max(VerticalFallSpeed + GroundGravity * DeltaSeconds, 0.0f);
	AddActorWorldOffset(FVector(0.0f, 0.0f, -VerticalFallSpeed * DeltaSeconds), true);
}

void ABRBossBase::ClearBaseTimers()
{
	GetWorldTimerManager().ClearTimer(GroggyTimerHandle);
}

void ABRBossBase::OnBossReset()
{
}

void ABRBossBase::OnBossDeadInternal()
{
}

void ABRBossBase::OnBossGroggyInternal()
{
}

void ABRBossBase::OnBossRecoveredFromGroggyInternal()
{
}

void ABRBossBase::OnBossPhaseChanged(EBRBossPhase NewPhase)
{
}

void ABRBossBase::UpdateBossAI(float DeltaSeconds)
{
}

void ABRBossBase::DrawBossDebug() const
{
}

FString ABRBossBase::GetBossDebugName() const
{
	return GetName();
}
