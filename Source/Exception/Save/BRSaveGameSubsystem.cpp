#include "BRSaveGameSubsystem.h"

#include "BRSaveGame.h"
#include "BRInventoryComponent.h"
#include "BRHiddenStorySubsystem.h"
#include "BRNarrativeQueueSubsystem.h"
#include "BRWorldMapSubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "ExceptionGameMode.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 OldestSupportedSaveVersion = 3;
	constexpr int32 CurrentSupportedSaveVersion = 4;
	const FName RuntimeFieldLevelName(TEXT("L_Runtime_Field"));

	bool IsSupportedRuntimeSave(const UBRSaveGame* SaveGame)
	{
		return SaveGame
			&& SaveGame->SaveVersion >= OldestSupportedSaveVersion
			&& SaveGame->SaveVersion <= CurrentSupportedSaveVersion
			&& SaveGame->SavedLevelName == RuntimeFieldLevelName;
	}
}

bool UBRSaveGameSubsystem::SaveCurrentGame(const FString& SlotName, int32 UserIndex)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UBRSaveGame* SaveGame = Cast<UBRSaveGame>(UGameplayStatics::CreateSaveGameObject(UBRSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	SaveGame->SavedLevelName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));

	if (AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)))
	{
		SaveGame->PlayerTransform = PlayerCharacter->GetActorTransform();
		SaveGame->PlayerTransform.SetScale3D(FVector::OneVector);
		SaveGame->PlayerHP = PlayerCharacter->GetCurrentHP();
		SaveGame->PlayerStamina = PlayerCharacter->GetCurrentStamina();
		SaveGame->PlayerLevel = PlayerCharacter->GetPlayerLevel();
		SaveGame->UpgradePoints = PlayerCharacter->GetUpgradePoints();
		SaveGame->VitalityLevel = PlayerCharacter->GetVitalityLevel();
		SaveGame->EnduranceLevel = PlayerCharacter->GetEnduranceLevel();
		SaveGame->PowerLevel = PlayerCharacter->GetPowerLevel();
		SaveGame->CurrentExperience = PlayerCharacter->GetCurrentExperience();
		SaveGame->DroppedExperience = PlayerCharacter->GetDroppedExperience();
		if (UBRInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
		{
			SaveGame->InventorySlots = InventoryComponent->GetSlots();
		}
	}

	if (AExceptionGameMode* ExceptionGameMode = World->GetAuthGameMode<AExceptionGameMode>())
	{
		SaveGame->bHasCheckpoint = ExceptionGameMode->HasCheckpoint();
		if (SaveGame->bHasCheckpoint)
		{
			SaveGame->CheckpointTransform = ExceptionGameMode->GetCheckpointTransform();
			SaveGame->CheckpointTransform.SetScale3D(FVector::OneVector);
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			SaveGame->RegisteredNelRequests = HiddenStory->GetRegisteredNelRequestIds();
			SaveGame->CompletedNelRequests = HiddenStory->GetCompletedNelRequestIds();
			SaveGame->HiddenFragmentCount = HiddenStory->GetHiddenFragmentCount();
			SaveGame->ConsumedNarrativeBeatIds = HiddenStory->GetConsumedNarrativeBeatIds();
			SaveGame->CollectedHiddenFragmentIds = HiddenStory->GetCollectedHiddenFragmentIds();
			SaveGame->bMimikatzAuthoritySeizedUnlocked = HiddenStory->IsMimikatzAuthoritySeizedUnlocked();
			SaveGame->bHiddenEndingEligible = HiddenStory->IsHiddenEndingEligible();
			SaveGame->LastResolvedEnding = HiddenStory->GetLastResolvedEnding();
			SaveGame->DefeatedBossIds = HiddenStory->GetDefeatedMainBossIds();
		}
		if (UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>())
		{
			SaveGame->UnlockedMapRegionIds = WorldMap->GetUnlockedRegionIds();
		}
	}

	return UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
}

bool UBRSaveGameSubsystem::LoadGameFromSlotAndOpenLevel(const FString& SlotName, int32 UserIndex)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return false;
	}

	UBRSaveGame* LoadedSaveGame = Cast<UBRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!IsSupportedRuntimeSave(LoadedSaveGame))
	{
		PendingSaveGame = nullptr;
		return false;
	}
	PendingSaveGame = LoadedSaveGame;

	// Version 3 stored only the count. Promote that count to the stable field
	// fragment IDs introduced in version 4 so legacy saves cannot recollect them.
	if (PendingSaveGame->SaveVersion < 4 && PendingSaveGame->CollectedHiddenFragmentIds.IsEmpty())
	{
		static const FName LegacyFragmentIds[] = {
			TEXT("HiddenFragment_Field1"),
			TEXT("HiddenFragment_Field2"),
			TEXT("HiddenFragment_Field3"),
		};
		const int32 MigratedCount = FMath::Clamp(PendingSaveGame->HiddenFragmentCount, 0, UE_ARRAY_COUNT(LegacyFragmentIds));
		for (int32 Index = 0; Index < MigratedCount; ++Index)
		{
			PendingSaveGame->CollectedHiddenFragmentIds.Add(LegacyFragmentIds[Index]);
		}
	}

	// Seed one-shot state before opening the saved map so actors observe the saved
	// IDs during BeginPlay, rather than stale GameInstance state from a prior run.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			HiddenStory->ApplySavedHiddenStoryState(
				PendingSaveGame->RegisteredNelRequests,
				PendingSaveGame->CompletedNelRequests,
				PendingSaveGame->HiddenFragmentCount,
				PendingSaveGame->bMimikatzAuthoritySeizedUnlocked,
				PendingSaveGame->bHiddenEndingEligible,
				PendingSaveGame->LastResolvedEnding,
				PendingSaveGame->DefeatedBossIds);
			HiddenStory->ApplySavedOneShotState(
				PendingSaveGame->ConsumedNarrativeBeatIds,
				PendingSaveGame->CollectedHiddenFragmentIds);
		}
		if (UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>())
		{
			WorldMap->ApplySavedMapState(PendingSaveGame->UnlockedMapRegionIds);
		}
		if (UBRNarrativeQueueSubsystem* NarrativeQueue = GameInstance->GetSubsystem<UBRNarrativeQueueSubsystem>())
		{
			NarrativeQueue->ClearMessages();
		}
	}

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, PendingSaveGame->SavedLevelName);
		return true;
	}

	return false;
}

bool UBRSaveGameSubsystem::ApplyPendingLoadedGame()
{
	if (!PendingSaveGame)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AExceptionGameMode* ExceptionGameMode = World->GetAuthGameMode<AExceptionGameMode>();
	if (ExceptionGameMode && PendingSaveGame->bHasCheckpoint)
	{
		ExceptionGameMode->SetCheckpointTransform(PendingSaveGame->CheckpointTransform);
	}

	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (!PlayerCharacter)
	{
		return false;
	}

	UBRHiddenStorySubsystem* RestoredHiddenStory = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		RestoredHiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>();
		if (RestoredHiddenStory)
		{
			RestoredHiddenStory->ApplySavedHiddenStoryState(
				PendingSaveGame->RegisteredNelRequests,
				PendingSaveGame->CompletedNelRequests,
				PendingSaveGame->HiddenFragmentCount,
				PendingSaveGame->bMimikatzAuthoritySeizedUnlocked,
				PendingSaveGame->bHiddenEndingEligible,
				PendingSaveGame->LastResolvedEnding,
				PendingSaveGame->DefeatedBossIds);
			RestoredHiddenStory->ApplySavedOneShotState(
				PendingSaveGame->ConsumedNarrativeBeatIds,
				PendingSaveGame->CollectedHiddenFragmentIds);
		}
		if (UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>())
		{
			WorldMap->ApplySavedMapState(PendingSaveGame->UnlockedMapRegionIds);
		}
	}

	const FTransform RestoreTransform = PendingSaveGame->bHasCheckpoint ? PendingSaveGame->CheckpointTransform : PendingSaveGame->PlayerTransform;
	PlayerCharacter->SetActorTransform(RestoreTransform, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerCharacter->ApplySavedProgression(PendingSaveGame->PlayerLevel, PendingSaveGame->UpgradePoints, PendingSaveGame->VitalityLevel, PendingSaveGame->EnduranceLevel, PendingSaveGame->PowerLevel);
	PlayerCharacter->ApplySavedExperience(PendingSaveGame->CurrentExperience, PendingSaveGame->DroppedExperience);
	PlayerCharacter->ApplySavedStats(PendingSaveGame->PlayerHP, PendingSaveGame->PlayerStamina);
	if (UBRInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
	{
		InventoryComponent->SetSlots(PendingSaveGame->InventorySlots);
	}
	if (RestoredHiddenStory)
	{
		PlayerCharacter->RefreshHiddenStoryRewards();
	}

	if (AController* Controller = PlayerCharacter->GetController())
	{
		Controller->SetControlRotation(RestoreTransform.GetRotation().Rotator());
	}

	PendingSaveGame = nullptr;
	return true;
}

bool UBRSaveGameSubsystem::DoesSaveExist(const FString& SlotName, int32 UserIndex) const
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return false;
	}

	const UBRSaveGame* SaveGame = Cast<UBRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	return IsSupportedRuntimeSave(SaveGame);
}

bool UBRSaveGameSubsystem::DeleteSave(const FString& SlotName, int32 UserIndex)
{
	PendingSaveGame = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			HiddenStory->ResetHiddenStoryState();
		}
		if (UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>())
		{
			WorldMap->ResetMapState();
		}
		if (UBRNarrativeQueueSubsystem* NarrativeQueue = GameInstance->GetSubsystem<UBRNarrativeQueueSubsystem>())
		{
			NarrativeQueue->ClearMessages();
		}
	}

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return true;
	}

	return UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
}
