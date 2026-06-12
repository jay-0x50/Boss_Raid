#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRInventoryTypes.h"
#include "BRPlayerHUDWidget.generated.h"

class UBRInventorySlotWidget;
class UProgressBar;
class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBRPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|HUD")
	void SetHP(float CurrentValue, float MaxValue, float NormalizedValue);

	UFUNCTION(BlueprintCallable, Category="Exception|HUD")
	void SetStamina(float CurrentValue, float MaxValue, float NormalizedValue);

	UFUNCTION(BlueprintCallable, Category="Exception|HUD")
	void SetHotbarSlot(int32 HotbarIndex, int32 InventorySlotIndex, const FBRInventorySlot& InventorySlot);

protected:
	virtual void NativeConstruct() override;

private:
	void BuildHUDWidget();

	UPROPERTY()
	TObjectPtr<UProgressBar> HPBar;

	UPROPERTY()
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> HPText;

	UPROPERTY()
	TObjectPtr<UTextBlock> StaminaText;

	UPROPERTY()
	TArray<TObjectPtr<UBRInventorySlotWidget>> HotbarSlots;
};
