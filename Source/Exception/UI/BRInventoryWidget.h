#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRInventoryTypes.h"
#include "BRInventoryWidget.generated.h"

class UBRInventoryComponent;
class UBRInventorySlotWidget;
class UTextBlock;
class UUniformGridPanel;

UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBRInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void SetInventoryComponent(UBRInventoryComponent* NewInventoryComponent);

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void SetInventorySlots(const TArray<FBRInventorySlot>& Slots);

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void SetInventorySlot(int32 SlotIndex, const FBRInventorySlot& InventorySlot);

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void RefreshInventory();

protected:
	virtual void NativeConstruct() override;

private:
	void BuildInventoryWidget();
	void BuildInventorySlots(UUniformGridPanel* SlotGrid, UUniformGridPanel* HotbarGrid);
	void UpdateDetailsPanel(const FBRInventorySlot& InventorySlot);

	UPROPERTY()
	TObjectPtr<UBRInventoryComponent> InventoryComponent;

	UPROPERTY()
	TArray<TObjectPtr<UBRInventorySlotWidget>> SlotWidgets;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemTypeText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemEffectText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemDescriptionText;

	static constexpr int32 GeneralSlotCount = 20;
	static constexpr int32 HotbarSlotStartIndex = 20;
	static constexpr int32 HotbarSlotCount = 3;
};
