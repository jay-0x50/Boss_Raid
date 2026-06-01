#include "BRSaveGameSubsystem.h"

#include "BRSaveGame.h"
#include "BRInventoryComponent.h"
#include "ExceptionCharacter.h"
#include "ExceptionGameMode.h"
#include "Kismet/GameplayStatics.h"

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

	return UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
}

bool UBRSaveGameSubsystem::LoadGameFromSlotAndOpenLevel(const FString& SlotName, int32 UserIndex)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return false;
	}

	PendingSaveGame = Cast<UBRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!PendingSaveGame || PendingSaveGame->SavedLevelName.IsNone())
	{
		PendingSaveGame = nullptr;
		return false;
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

	const FTransform RestoreTransform = PendingSaveGame->bHasCheckpoint ? PendingSaveGame->CheckpointTransform : PendingSaveGame->PlayerTransform;
	PlayerCharacter->SetActorTransform(RestoreTransform, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerCharacter->ApplySavedStats(PendingSaveGame->PlayerHP, PendingSaveGame->PlayerStamina);
	if (UBRInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
	{
		InventoryComponent->SetSlots(PendingSaveGame->InventorySlots);
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
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool UBRSaveGameSubsystem::DeleteSave(const FString& SlotName, int32 UserIndex)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return true;
	}

	return UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
}
