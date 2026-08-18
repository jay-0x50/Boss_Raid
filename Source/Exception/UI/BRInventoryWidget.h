#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRInventoryTypes.h"
#include "BRInventoryWidget.generated.h"

class UBRInventoryComponent;
class UBRInventorySlotWidget;
class UButton;
class UTextBlock;
class UUniformGridPanel;

UENUM()
enum class EBRInventoryTab : uint8
{
	All,
	Equipment,
	Consumable,
	KeyItem,
	QuestItem
};

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
	void BindDesignerWidgets();
	void BuildInventorySlots(UUniformGridPanel* SlotGrid);
	void RefreshSlotList();
	void SetActiveTab(EBRInventoryTab NewTab);
	bool DoesSlotMatchTab(const FBRInventorySlot& InventorySlot) const;
	EBRInventoryItemCategory GetItemType(const FBRInventorySlot& InventorySlot) const;
	FText GetTabText(EBRInventoryTab Tab) const;
	FString GetCategoryDisplayName(EBRInventoryItemCategory Category) const;
	void UpdateDetailsPanel(const FBRInventorySlot& InventorySlot);
	void ClearDetailsPanel();
	void UpdateTabVisuals();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleAllTabClicked();

	UFUNCTION()
	void HandleEquipmentTabClicked();

	UFUNCTION()
	void HandleConsumableTabClicked();

	UFUNCTION()
	void HandleKeyItemTabClicked();

	UFUNCTION()
	void HandleQuestItemTabClicked();

	UPROPERTY()
	TObjectPtr<UBRInventoryComponent> InventoryComponent;

	UPROPERTY(EditDefaultsOnly, Category="Exception|Inventory")
	TSubclassOf<UBRInventorySlotWidget> SlotWidgetClass;

	UPROPERTY()
	TArray<TObjectPtr<UBRInventorySlotWidget>> SlotWidgets;

	UPROPERTY()
	TArray<FBRInventorySlot> CachedSlots;

	UPROPERTY()
	TArray<int32> FilteredSlotIndices;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> TabButtons;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> TabTexts;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemTypeText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemEffectText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ItemDescriptionText;

	UPROPERTY()
	TObjectPtr<UTextBlock> InventoryCountText;

	EBRInventoryTab ActiveTab = EBRInventoryTab::All;

	static constexpr int32 DisplaySlotCount = 24;
};
