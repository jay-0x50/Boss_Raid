// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatEnemySpawner.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "CombatEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

namespace
{
	constexpr float GoldenAngleRadians = 2.39996323f;
}

ACombatEnemySpawner::ACombatEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the reference spawn capsule
	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
}

void ACombatEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	ClearEncounter();

	// should we spawn an enemy right away?
	if (bShouldSpawnEnemiesImmediately)
	{
		ArmAutoActivation(InitialSpawnDelay);
	}
}

void ACombatEnemySpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorldTimerManager().ClearTimer(SpawnTimer);
	GetWorldTimerManager().ClearTimer(DepletedTimer);
	ActiveEnemies.Reset();
	SpawnedEnemies.Reset();
}

void ACombatEnemySpawner::StartEncounter()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);

	if (bEncounterActive || bEncounterDepleted)
	{
		return;
	}

	bEncounterActive = true;

	if (!EnemyClass)
	{
		bEncounterActive = false;
		UE_LOG(LogTemp, Error, TEXT("Enemy spawner %s cannot start because EnemyClass is not set."), *GetNameSafe(this));
		return;
	}

	if (RemainingSpawnCount <= 0)
	{
		FinishEncounter();
		return;
	}

	SpawnEnemy();
}

void ACombatEnemySpawner::TryAutoActivate()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);

	if (bEncounterActive || bEncounterDepleted)
	{
		return;
	}

	if (!bWaitForPlayerWhenAutoSpawning)
	{
		StartEncounter();
		return;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const float ActivationDistanceSquared = FMath::Square(FMath::Max(100.0f, AutoActivationDistance));
	if (PlayerPawn && FVector::DistSquared2D(PlayerPawn->GetActorLocation(), GetActorLocation()) <= ActivationDistanceSquared)
	{
		StartEncounter();
		return;
	}

	ArmAutoActivation(AutoActivationCheckInterval);
}

void ACombatEnemySpawner::ArmAutoActivation(float Delay)
{
	GetWorldTimerManager().SetTimer(SpawnTimer, this, &ACombatEnemySpawner::TryAutoActivate, FMath::Max(0.01f, Delay), false);
}

void ACombatEnemySpawner::SpawnEnemy()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);

	if (!bEncounterActive || bEncounterDepleted || !EnemyClass)
	{
		return;
	}

	const int32 AliveLimit = FMath::Clamp(MaxAliveEnemies, 1, 20);
	bool bSpawnFailed = false;

	while (RemainingSpawnCount > 0 && ActiveEnemies.Num() < AliveLimit)
	{
		FTransform SpawnTransform = SpawnCapsule->GetComponentTransform();
		if (SpawnSpreadRadius > 0.0f && AliveLimit > 1)
		{
			const float Angle = static_cast<float>(SpawnedCount) * GoldenAngleRadians;
			const float RingAlpha = FMath::Sqrt(static_cast<float>((SpawnedCount % AliveLimit) + 1) / static_cast<float>(AliveLimit));
			const FVector LocalOffset(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
			SpawnTransform.AddToTranslation(GetActorTransform().TransformVectorNoScale(LocalOffset * SpawnSpreadRadius * RingAlpha));
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = this;

		ACombatEnemy* SpawnedEnemy = GetWorld()->SpawnActor<ACombatEnemy>(EnemyClass, SpawnTransform, SpawnParams);

		if (!SpawnedEnemy)
		{
			bSpawnFailed = true;
			break;
		}

		--RemainingSpawnCount;
		++SpawnedCount;
		ActiveEnemies.Add(TWeakObjectPtr<ACombatEnemy>(SpawnedEnemy));
		SpawnedEnemies.Add(SpawnedEnemy);
		SpawnedEnemy->OnEnemyDiedNative.AddUObject(this, &ACombatEnemySpawner::OnEnemyDied);
		SpawnedEnemy->OnDestroyed.AddDynamic(this, &ACombatEnemySpawner::OnSpawnedEnemyDestroyed);
	}

	if (bSpawnFailed)
	{
		ScheduleSpawn(FMath::Max(0.1f, RespawnDelay));
	}
	else if (RemainingSpawnCount <= 0 && ActiveEnemies.Num() == 0)
	{
		FinishEncounter();
	}
}

void ACombatEnemySpawner::OnEnemyDied(ACombatEnemy* Enemy)
{
	HandleEnemyRemoved(Enemy);
}

void ACombatEnemySpawner::OnSpawnedEnemyDestroyed(AActor* DestroyedActor)
{
	HandleEnemyRemoved(Cast<ACombatEnemy>(DestroyedActor));
}

void ACombatEnemySpawner::HandleEnemyRemoved(ACombatEnemy* Enemy)
{
	if (!Enemy || ActiveEnemies.Remove(TWeakObjectPtr<ACombatEnemy>(Enemy)) == 0 || !bEncounterActive)
	{
		return;
	}

	if (RemainingSpawnCount > 0)
	{
		ScheduleSpawn(RespawnDelay);
	}
	else if (ActiveEnemies.Num() == 0)
	{
		FinishEncounter();
	}
}

void ACombatEnemySpawner::ScheduleSpawn(float Delay)
{
	if (!bEncounterActive || RemainingSpawnCount <= 0)
	{
		return;
	}
	if (GetWorldTimerManager().TimerExists(SpawnTimer))
	{
		return;
	}

	if (Delay <= 0.0f)
	{
		SpawnTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ACombatEnemySpawner::SpawnEnemy);
	}
	else
	{
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &ACombatEnemySpawner::SpawnEnemy, Delay, false);
	}
}

void ACombatEnemySpawner::FinishEncounter()
{
	bEncounterActive = false;
	bEncounterDepleted = true;
	GetWorldTimerManager().ClearTimer(SpawnTimer);
	GetWorldTimerManager().ClearTimer(DepletedTimer);

	if (ActivationDelay <= 0.0f)
	{
		SpawnerDepleted();
	}
	else
	{
		GetWorldTimerManager().SetTimer(DepletedTimer, this, &ACombatEnemySpawner::SpawnerDepleted, ActivationDelay, false);
	}
}

void ACombatEnemySpawner::SpawnerDepleted()
{
	if (!bEncounterDepleted)
	{
		return;
	}

	// process the actors to activate list
	for (AActor* CurrentActor : ActorsToActivateWhenDepleted)
	{
		// check if the actor is activatable
		if (ICombatActivatable* CombatActivatable = Cast<ICombatActivatable>(CurrentActor))
		{
			// activate the actor
			CombatActivatable->ActivateInteraction(this);
		}
	}
}

void ACombatEnemySpawner::ToggleInteraction(AActor* ActivationInstigator)
{
	if (bEncounterActive)
	{
		DeactivateInteraction(ActivationInstigator);
	}
	else
	{
		ActivateInteraction(ActivationInstigator);
	}
}

void ACombatEnemySpawner::ActivateInteraction(AActor* ActivationInstigator)
{
	if (bEncounterActive || bEncounterDepleted)
	{
		return;
	}

	StartEncounter();
}

void ACombatEnemySpawner::DeactivateInteraction(AActor* ActivationInstigator)
{
	ClearEncounter();
}

void ACombatEnemySpawner::ResetEncounter(bool bRestartImmediately)
{
	ClearEncounter();

	if (bRestartImmediately)
	{
		StartEncounter();
	}
	else if (bShouldSpawnEnemiesImmediately)
	{
		ArmAutoActivation(AutoActivationCheckInterval);
	}
}

void ACombatEnemySpawner::ClearEncounter()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
	GetWorldTimerManager().ClearTimer(DepletedTimer);
	bEncounterActive = false;
	bEncounterDepleted = false;

	TArray<TWeakObjectPtr<ACombatEnemy>> EnemiesToDestroy = MoveTemp(SpawnedEnemies);
	SpawnedEnemies.Reset();
	ActiveEnemies.Reset();

	for (const TWeakObjectPtr<ACombatEnemy>& EnemyPtr : EnemiesToDestroy)
	{
		if (ACombatEnemy* Enemy = EnemyPtr.Get())
		{
			Enemy->OnEnemyDiedNative.RemoveAll(this);
			Enemy->OnDestroyed.RemoveDynamic(this, &ACombatEnemySpawner::OnSpawnedEnemyDestroyed);
			Enemy->Destroy();
		}
	}

	RemainingSpawnCount = FMath::Max(0, SpawnCount);
	SpawnedCount = 0;
}
