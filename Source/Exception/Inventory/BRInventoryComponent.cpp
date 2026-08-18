#include "BRInventoryComponent.h"

namespace
{
	constexpr int32 DefaultHotbarCapacity = 23;
}

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
	Capacity = FMath::Max(DefaultHotbarCapacity, Capacity);
	Slots.SetNum(Capacity);
	NotifyInventoryChanged();
}

void UBRInventoryComponent::SetCapacity(int32 NewCapacity)
{
	const int32 RequestedCapacity = FMath::Max(DefaultHotbarCapacity, NewCapacity);
	if (RequestedCapacity < Slots.Num())
	{
		for (int32 SlotIndex = RequestedCapacity; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			if (!Slots[SlotIndex].IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("Inventory resize rejected: slot %d is not empty."), SlotIndex);
				return;
			}
		}
	}

	Capacity = RequestedCapacity;
	Slots.SetNum(Capacity);
	NotifyInventoryChanged();
}

void UBRInventoryComponent::SetSlots(const TArray<FBRInventorySlot>& NewSlots)
{
	Slots = NewSlots;
	Capacity = FMath::Max(DefaultHotbarCapacity, Slots.Num());
	Slots.SetNum(Capacity);
	NotifyInventoryChanged();

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		NotifySlotChanged(SlotIndex);
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
		NotifySlotChanged(SlotIndex);
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
		NotifySlotChanged(SlotIndex);
	}

	NotifyInventoryChanged();
	return RemainingQuantity < Quantity;
}

bool UBRInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
	if (ItemId.IsNone() || Quantity <= 0)
	{
		return false;
	}

	int64 AvailableQuantity = 0;
	for (const FBRInventorySlot& Slot : Slots)
	{
		if (!Slot.IsEmpty() && Slot.Item.ItemId == ItemId)
		{
			AvailableQuantity += Slot.Quantity;
			if (AvailableQuantity >= Quantity)
			{
				break;
			}
		}
	}

	if (AvailableQuantity < Quantity)
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
		NotifySlotChanged(SlotIndex);
	}

	NotifyInventoryChanged();
	return RemainingQuantity == 0;
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

	NotifySlotChanged(SlotIndex);
	NotifyInventoryChanged();
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
		const int32 ExistingStackMax = FMath::Max(1, ToSlot.Item.MaxStack);
		const int32 AvailableSpace = FMath::Max(0, ExistingStackMax - ToSlot.Quantity);
		const int32 AddedQuantity = FMath::Min(AvailableSpace, FromSlot.Quantity);
		if (AddedQuantity <= 0)
		{
			return false;
		}

		ToSlot.Quantity += AddedQuantity;
		FromSlot.Quantity -= AddedQuantity;
		if (FromSlot.Quantity <= 0)
		{
			EmptySlot(FromSlotIndex);
		}
	}
	else
	{
		Swap(FromSlot, ToSlot);
	}

	NotifySlotChanged(FromSlotIndex);
	NotifySlotChanged(ToSlotIndex);
	NotifyInventoryChanged();
	return true;
}

bool UBRInventoryComponent::UseSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty() || !Slots[SlotIndex].Item.bUsable)
	{
		return false;
	}

	const FBRInventorySlot UsedSlot = Slots[SlotIndex];
	if (TryUseItem.IsBound() && !TryUseItem.Execute(SlotIndex, UsedSlot))
	{
		return false;
	}

	OnItemUsed.Broadcast(SlotIndex, UsedSlot);

	if (UsedSlot.Item.bConsumeOnUse && !RemoveFromSlot(SlotIndex, 1))
	{
		return false;
	}

	return true;
}

void UBRInventoryComponent::ClearInventory()
{
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		EmptySlot(SlotIndex);
		NotifySlotChanged(SlotIndex);
	}
	NotifyInventoryChanged();
}

void UBRInventoryComponent::EmptySlot(int32 SlotIndex)
{
	if (Slots.IsValidIndex(SlotIndex))
	{
		Slots[SlotIndex] = FBRInventorySlot();
	}
}

void UBRInventoryComponent::NotifyInventoryChanged()
{
	OnInventoryChanged.Broadcast(Slots);
}

void UBRInventoryComponent::NotifySlotChanged(int32 SlotIndex)
{
	if (Slots.IsValidIndex(SlotIndex))
	{
		OnSlotChanged.Broadcast(SlotIndex, Slots[SlotIndex]);
	}
}
