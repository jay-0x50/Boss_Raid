#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRInventoryTypes.h"
#include "BRInventorySlotWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBRInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void SetSlotData(int32 NewSlotIndex, const FBRInventorySlot& NewInventorySlot);

	UFUNCTION(BlueprintPure, Category="Exception|Inventory")
	int32 GetSlotIndex() const { return SlotIndex; }

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UFUNCTION()
	void HandleClicked();

	void BuildSlotWidget();
	void BindDesignerWidgets();
	void RefreshVisuals();

	UPROPERTY()
	TObjectPtr<UButton> SlotButton;

	UPROPERTY()
	TObjectPtr<UBorder> FrameBorder;

	UPROPERTY()
	TObjectPtr<UImage> IconImage;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> QuantityText;

	int32 SlotIndex = INDEX_NONE;
	FBRInventorySlot CurrentInventorySlot;
};
