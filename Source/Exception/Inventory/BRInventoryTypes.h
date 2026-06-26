#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "BRInventoryTypes.generated.h"

UENUM(BlueprintType)
enum class EBRInventoryItemEffect : uint8
{
	None,
	HealHP,
	RestoreStamina,
	RestoreAll,
	GrantUpgradePoint,
	HiddenRootWeapon
};

USTRUCT(BlueprintType)
struct EXCEPTION_API FBRInventoryItemDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory", meta=(ClampMin="1"))
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	bool bUsable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	bool bConsumeOnUse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	EBRInventoryItemEffect Effect = EBRInventoryItemEffect::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	float EffectValue = 0.0f;

	bool IsValid() const
	{
		return !ItemId.IsNone();
	}
};

USTRUCT(BlueprintType)
struct EXCEPTION_API FBRInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory")
	FBRInventoryItemDefinition Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exception|Inventory", meta=(ClampMin="0"))
	int32 Quantity = 0;

	bool IsEmpty() const
	{
		return Quantity <= 0 || !Item.IsValid();
	}
};
