#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BRInventoryTypes.h"
#include "BRHiddenStorySubsystem.h"
#include "BRSaveGame.generated.h"

UCLASS(BlueprintType)
class EXCEPTION_API UBRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	int32 SaveVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	FName SavedLevelName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	FTransform PlayerTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	FTransform CheckpointTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	bool bHasCheckpoint = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	float PlayerHP = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	float PlayerStamina = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	int32 PlayerLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	int32 UpgradePoints = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	int32 VitalityLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	int32 EnduranceLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	int32 PowerLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	TArray<FName> DefeatedBossIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	TArray<FBRInventorySlot> InventorySlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	TArray<FName> RegisteredNelRequests;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	TArray<FName> CompletedNelRequests;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	int32 HiddenFragmentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	bool bMimikatzAuthoritySeizedUnlocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	bool bHiddenEndingEligible = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Save")
	EBRRuntimeEnding LastResolvedEnding = EBRRuntimeEnding::None;
};
