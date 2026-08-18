#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BRInventoryTypes.h"
#include "BRInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRInventoryChanged, const TArray<FBRInventorySlot>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRInventorySlotChanged, int32, SlotIndex, const FBRInventorySlot&, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRInventoryItemUsed, int32, SlotIndex, const FBRInventorySlot&, Slot);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FBRInventoryUseHandler, int32, const FBRInventorySlot&);

UCLASS(ClassGroup=(Exception), meta=(BlueprintSpawnableComponent))
class EXCEPTION_API UBRInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBRInventoryComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void InitializeInventory();

	/** Rejects a shrink that would discard a non-empty slot. */
	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void SetCapacity(int32 NewCapacity);

	/** Restores slots while keeping the default hotbar indices 20-22 valid. */
	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void SetSlots(const TArray<FBRInventorySlot>& NewSlots);

	UFUNCTION(BlueprintPure, Category="Exception|Inventory")
	int32 GetCapacity() const { return Slots.Num(); }

	UFUNCTION(BlueprintPure, Category="Exception|Inventory")
	TArray<FBRInventorySlot> GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category="Exception|Inventory")
	bool IsValidSlotIndex(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Exception|Inventory")
	bool IsSlotEmpty(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Exception|Inventory")
	FBRInventorySlot GetSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Exception|Inventory")
	int32 FindFirstItemSlot(FName ItemId) const;

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	bool AddItem(const FBRInventoryItemDefinition& Item, int32 Quantity, int32& RemainingQuantity);

	/** Removes the full requested quantity or leaves the inventory unchanged. */
	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	bool RemoveItem(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	bool RemoveFromSlot(int32 SlotIndex, int32 Quantity);

	/** Returns true only when at least one slot actually changes. */
	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	bool MoveSlot(int32 FromSlotIndex, int32 ToSlotIndex);

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	bool UseSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void ClearInventory();

	UPROPERTY(BlueprintAssignable, Category="Exception|Inventory")
	FBRInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Inventory")
	FBRInventorySlotChanged OnSlotChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Inventory")
	FBRInventoryItemUsed OnItemUsed;

	FBRInventoryUseHandler TryUseItem;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Inventory", meta=(ClampMin="23"))
	int32 Capacity = 23;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Inventory")
	TArray<FBRInventorySlot> InitialSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Inventory")
	TArray<FBRInventorySlot> Slots;

private:
	void EmptySlot(int32 SlotIndex);
	void NotifyInventoryChanged();
	void NotifySlotChanged(int32 SlotIndex);
};
