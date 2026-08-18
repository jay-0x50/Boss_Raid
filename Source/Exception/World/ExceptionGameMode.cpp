// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExceptionGameMode.h"

#include "BRBossArenaTrigger.h"
#include "BRSaveGameSubsystem.h"
#include "Variant_Combat/AI/CombatEnemySpawner.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Player/Controller/ExceptionPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "EngineUtils.h"

AExceptionGameMode::AExceptionGameMode()
{
	PlayerControllerClass = AExceptionPlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/Core/BP_ExceptionCharacter"));
	if (PlayerPawnClassFinder.Succeeded())
	{
		DefaultPawnClass = PlayerPawnClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerClassFinder(TEXT("/Game/Blueprints/Core/BP_ExceptionPlayerController"));
	if (PlayerControllerClassFinder.Succeeded())
	{
		PlayerControllerClass = PlayerControllerClassFinder.Class;
	}
}

void AExceptionGameMode::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UBRSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UBRSaveGameSubsystem>())
			{
				SaveSubsystem->ApplyPendingLoadedGame();
			}
		}
	}));
}

void AExceptionGameMode::SetCheckpointTransform(const FTransform& NewCheckpointTransform)
{
	CheckpointTransform = NewCheckpointTransform;
	bHasCheckpoint = true;

	UE_LOG(LogTemp, Log, TEXT("Checkpoint saved: %s"), *CheckpointTransform.GetLocation().ToString());
}

FTransform AExceptionGameMode::GetCheckpointTransform() const
{
	return CheckpointTransform;
}

void AExceptionGameMode::SetActiveBossArena(ABRBossArenaTrigger* NewActiveArena)
{
	ActiveBossArena = NewActiveArena;
}

void AExceptionGameMode::ResetActiveBossArenaForRetry()
{
	if (ActiveBossArena)
	{
		ActiveBossArena->ResetArenaForRetry();
	}

	// Field encounters follow the same retry boundary as the active boss arena.
	// Auto-spawners re-arm their distance check; trigger-driven spawners stay dormant.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACombatEnemySpawner> SpawnerIt(World); SpawnerIt; ++SpawnerIt)
		{
			SpawnerIt->ResetEncounter(false);
		}
	}
}
