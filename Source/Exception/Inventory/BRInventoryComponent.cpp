#include "BRInventoryComponent.h"

UBRInventoryComponent::UBRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBRInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeInventory();
}

void UBRInventoryComponent::InitializeInventory()
{
	Slots = InitialSlots;
	Slots.SetNum(FMath::Max(1, Capacity));
	BroadcastInventoryChanged();
}

void UBRInventoryComponent::SetCapacity(int32 NewCapacity)
{
	Capacity = FMath::Max(1, NewCapacity);
	Slots.SetNum(Capacity);
	BroadcastInventoryChanged();
}

void UBRInventoryComponent::SetSlots(const TArray<FBRInventorySlot>& NewSlots)
{
	Slots = NewSlots;
	Capacity = FMath::Max(1, Slots.Num());
	Slots.SetNum(Capacity);
	BroadcastInventoryChanged();

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		BroadcastSlotChanged(SlotIndex);
	}
}

bool UBRInventoryComponent::IsValidSlotIndex(int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex);
}

bool UBRInventoryComponent::IsSlotEmpty(int32 SlotIndex) const
{
	return !Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty();
}

FBRInventorySlot UBRInventoryComponent::GetSlot(int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex] : FBRInventorySlot();
}

int32 UBRInventoryComponent::FindFirstItemSlot(FName ItemId) const
{
	if (ItemId.IsNone())
	{
		return INDEX_NONE;
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (!Slots[SlotIndex].IsEmpty() && Slots[SlotIndex].Item.ItemId == ItemId)
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

bool UBRInventoryComponent::AddItem(const FBRInventoryItemDefinition& Item, int32 Quantity, int32& RemainingQuantity)
{
	RemainingQuantity = FMath::Max(0, Quantity);
	if (!Item.IsValid() || RemainingQuantity <= 0)
	{
		return false;
	}

	const int32 MaxStack = FMath::Max(1, Item.MaxStack);
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && RemainingQuantity > 0; ++SlotIndex)
	{
		FBRInventorySlot& Slot = Slots[SlotIndex];
		if (Slot.IsEmpty() || Slot.Item.ItemId != Item.ItemId || Slot.Quantity >= MaxStack)
		{
			continue;
		}

		const int32 AddedQuantity = FMath::Min(MaxStack - Slot.Quantity, RemainingQuantity);
		Slot.Quantity += AddedQuantity;
		RemainingQuantity -= AddedQuantity;
		BroadcastSlotChanged(SlotIndex);
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && RemainingQuantity > 0; ++SlotIndex)
	{
		FBRInventorySlot& Slot = Slots[SlotIndex];
		if (!Slot.IsEmpty())
		{
			continue;
		}

		const int32 AddedQuantity = FMath::Min(MaxStack, RemainingQuantity);
		Slot.Item = Item;
		Slot.Quantity = AddedQuantity;
		RemainingQuantity -= AddedQuantity;
		BroadcastSlotChanged(SlotIndex);
	}

	BroadcastInventoryChanged();
	return RemainingQuantity < Quantity;
}

bool UBRInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
	if (ItemId.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int32 RemainingQuantity = Quantity;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && RemainingQuantity > 0; ++SlotIndex)
	{
		FBRInventorySlot& Slot = Slots[SlotIndex];
		if (Slot.IsEmpty() || Slot.Item.ItemId != ItemId)
		{
			continue;
		}

		const int32 RemovedQuantity = FMath::Min(Slot.Quantity, RemainingQuantity);
		Slot.Quantity -= RemovedQuantity;
		RemainingQuantity -= RemovedQuantity;
		if (Slot.Quantity <= 0)
		{
			EmptySlot(SlotIndex);
		}
		BroadcastSlotChanged(SlotIndex);
	}

	const bool bRemovedAny = RemainingQuantity < Quantity;
	if (bRemovedAny)
	{
		BroadcastInventoryChanged();
	}
	return bRemovedAny;
}

bool UBRInventoryComponent::RemoveFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty() || Quantity <= 0)
	{
		return false;
	}

	FBRInventorySlot& Slot = Slots[SlotIndex];
	Slot.Quantity -= FMath::Min(Slot.Quantity, Quantity);
	if (Slot.Quantity <= 0)
	{
		EmptySlot(SlotIndex);
	}

	BroadcastSlotChanged(SlotIndex);
	BroadcastInventoryChanged();
	return true;
}

bool UBRInventoryComponent::MoveSlot(int32 FromSlotIndex, int32 ToSlotIndex)
{
	if (!Slots.IsValidIndex(FromSlotIndex) || !Slots.IsValidIndex(ToSlotIndex) || FromSlotIndex == ToSlotIndex)
	{
		return false;
	}

	FBRInventorySlot& FromSlot = Slots[FromSlotIndex];
	FBRInventorySlot& ToSlot = Slots[ToSlotIndex];
	if (FromSlot.IsEmpty())
	{
		return false;
	}

	if (!ToSlot.IsEmpty() && ToSlot.Item.ItemId == FromSlot.Item.ItemId)
	{
		const int32 MaxStack = FMath::Max(1, ToSlot.Item.MaxStack);
		const int32 AddedQuantity = FMath::Min(MaxStack - ToSlot.Quantity, FromSlot.Quantity);
		if (AddedQuantity > 0)
		{
			ToSlot.Quantity += AddedQuantity;
			FromSlot.Quantity -= AddedQuantity;
			if (FromSlot.Quantity <= 0)
			{
				EmptySlot(FromSlotIndex);
			}
		}
	}
	else
	{
		Swap(FromSlot, ToSlot);
	}

	BroadcastSlotChanged(FromSlotIndex);
	BroadcastSlotChanged(ToSlotIndex);
	BroadcastInventoryChanged();
	return true;
}

bool UBRInventoryComponent::UseSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty() || !Slots[SlotIndex].Item.bUsable)
	{
		return false;
	}

	const FBRInventorySlot UsedSlot = Slots[SlotIndex];
	OnItemUsed.Broadcast(SlotIndex, UsedSlot);

	if (UsedSlot.Item.bConsumeOnUse)
	{
		RemoveFromSlot(SlotIndex, 1);
	}

	return true;
}

void UBRInventoryComponent::ClearInventory()
{
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		EmptySlot(SlotIndex);
		BroadcastSlotChanged(SlotIndex);
	}
	BroadcastInventoryChanged();
}

void UBRInventoryComponent::EmptySlot(int32 SlotIndex)
{
	if (Slots.IsValidIndex(SlotIndex))
	{
		Slots[SlotIndex] = FBRInventorySlot();
	}
}

void UBRInventoryComponent::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast(Slots);
}

void UBRInventoryComponent::BroadcastSlotChanged(int32 SlotIndex)
{
	if (Slots.IsValidIndex(SlotIndex))
	{
		OnSlotChanged.Broadcast(SlotIndex, Slots[SlotIndex]);
	}
}
