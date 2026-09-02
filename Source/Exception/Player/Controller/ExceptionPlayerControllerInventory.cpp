#include "Player/Controller/ExceptionPlayerController.h"

#include "BRInventoryComponent.h"
#include "BRPlayerHUDWidget.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Blueprint/UserWidget.h"

namespace
{
void SetObjectParam(UFunction* Function, uint8* Params, UObject* Value)
{
	for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
	{
		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(*PropertyIt))
		{
			ObjectProperty->SetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<void>(Params), Value);
		}
	}
}

void SetSlotIndexParam(UFunction* Function, uint8* Params, int32 SlotIndex)
{
	for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
	{
		if (PropertyIt->GetFName() == TEXT("SlotIndex"))
		{
			if (FIntProperty* IntProperty = CastField<FIntProperty>(*PropertyIt))
			{
				IntProperty->SetPropertyValue(IntProperty->ContainerPtrToValuePtr<void>(Params), SlotIndex);
			}
		}
	}
}

void SetInventorySlotParam(UFunction* Function, uint8* Params, const FBRInventorySlot& Slot)
{
	for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(*PropertyIt))
		{
			if (StructProperty->Struct == FBRInventorySlot::StaticStruct())
			{
				StructProperty->CopyCompleteValue(StructProperty->ContainerPtrToValuePtr<void>(Params), &Slot);
			}
		}
	}
}

void SetInventorySlotsParam(UFunction* Function, uint8* Params, const TArray<FBRInventorySlot>& Slots)
{
	for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
	{
		if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(*PropertyIt))
		{
			if (FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner))
			{
				if (InnerStructProperty->Struct == FBRInventorySlot::StaticStruct())
				{
					ArrayProperty->CopyCompleteValue(ArrayProperty->ContainerPtrToValuePtr<void>(Params), &Slots);
				}
			}
		}
	}
}

void CallWidgetFunc(UUserWidget* Widget, FName FunctionName, TFunctionRef<void(UFunction*, uint8*)> FillParams)
{
	if (!Widget)
	{
		return;
	}

	UFunction* Function = Widget->FindFunction(FunctionName);
	if (!Function)
	{
		return;
	}

	uint8* Params = Function->ParmsSize > 0 ? static_cast<uint8*>(FMemory_Alloca(Function->ParmsSize)) : nullptr;
	if (Params)
	{
		FMemory::Memzero(Params, Function->ParmsSize);
		FillParams(Function, Params);
	}

	Widget->ProcessEvent(Function, Params);

	if (Params)
	{
		for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
		{
			PropertyIt->DestroyValue_InContainer(Params);
		}
	}
}
}

UUserWidget* AExceptionPlayerController::ShowInventoryWidget()
{
	if (!IsLocalPlayerController() || IsInTitleLevel())
	{
		return nullptr;
	}

	if (!InventoryWidget && InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
	}

	if (IsWorldMapOpen())
	{
		HideWorldMapWidget();
	}

	if (!InventoryWidget)
	{
		return nullptr;
	}

	const bool bPauseMenuOpen = IsPauseMenuOpen();
	if (bPauseMenuOpen && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
	}

	if (InventoryWidget && !InventoryWidget->IsInViewport())
	{
		InventoryWidget->AddToPlayerScreen(bPauseMenuOpen ? 70 : 40);
	}
	if (!bPauseMenuOpen)
	{
		SetPause(true);
	}

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (InventoryWidget)
	{
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	}
	SetInputMode(InputMode);

	BindInventoryWidgetToPawn();
	RefreshInventoryUI();

	return InventoryWidget;
}

void AExceptionPlayerController::HideInventoryWidget()
{
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}

	if (IsPauseMenuOpen())
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		if (PauseMenuWidget)
		{
			InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
		}
		SetInputMode(InputMode);
	}
	else if (!IsInTitleLevel())
	{
		SetPause(false);
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void AExceptionPlayerController::ToggleInventoryWidget()
{
	if (IsInventoryOpen())
	{
		HideInventoryWidget();
	}
	else
	{
		ShowInventoryWidget();
	}
}

bool AExceptionPlayerController::IsInventoryOpen() const
{
	return InventoryWidget && InventoryWidget->IsInViewport();
}

UBRInventoryComponent* AExceptionPlayerController::GetPlayerInventoryComponent() const
{
	const AExceptionCharacter* ExceptionCharacter = Cast<AExceptionCharacter>(GetPawn());
	return ExceptionCharacter ? ExceptionCharacter->GetInventoryComponent() : nullptr;
}

bool AExceptionPlayerController::UseInventorySlot(int32 SlotIndex)
{
	if (UBRInventoryComponent* InventoryComponent = GetPlayerInventoryComponent())
	{
		return InventoryComponent->UseSlot(SlotIndex);
	}

	return false;
}

void AExceptionPlayerController::HandleInventoryChanged(const TArray<FBRInventorySlot>& Slots)
{
	if (UBRPlayerHUDWidget* PlayerHUD = Cast<UBRPlayerHUDWidget>(PlayerHUDWidget))
	{
		for (int32 HotbarIndex = 0; HotbarIndex < 3; ++HotbarIndex)
		{
			const int32 InventorySlotIndex = 20 + HotbarIndex;
			PlayerHUD->SetHotbarSlot(HotbarIndex, InventorySlotIndex, Slots.IsValidIndex(InventorySlotIndex) ? Slots[InventorySlotIndex] : FBRInventorySlot());
		}
	}

	RefreshInventoryUI();
}

void AExceptionPlayerController::HandleInventorySlotChanged(int32 SlotIndex, const FBRInventorySlot& Slot)
{
	if (UBRPlayerHUDWidget* PlayerHUD = Cast<UBRPlayerHUDWidget>(PlayerHUDWidget))
	{
		if (SlotIndex >= 20 && SlotIndex <= 22)
		{
			PlayerHUD->SetHotbarSlot(SlotIndex - 20, SlotIndex, Slot);
		}
	}

	RefreshInventorySlot(SlotIndex, Slot);
}

void AExceptionPlayerController::BindInventoryWidgetToPawn()
{
	UBRInventoryComponent* InventoryComponent = GetPlayerInventoryComponent();
	if (!InventoryComponent || BoundInventoryComponent == InventoryComponent)
	{
		RefreshInventoryUI();
		return;
	}

	UnbindInventoryWidgetFromPawn();
	BoundInventoryComponent = InventoryComponent;
	BoundInventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &AExceptionPlayerController::HandleInventoryChanged);
	BoundInventoryComponent->OnSlotChanged.AddUniqueDynamic(this, &AExceptionPlayerController::HandleInventorySlotChanged);
	RefreshInventoryUI();
}

void AExceptionPlayerController::UnbindInventoryWidgetFromPawn()
{
	if (!BoundInventoryComponent)
	{
		return;
	}

	BoundInventoryComponent->OnInventoryChanged.RemoveDynamic(this, &AExceptionPlayerController::HandleInventoryChanged);
	BoundInventoryComponent->OnSlotChanged.RemoveDynamic(this, &AExceptionPlayerController::HandleInventorySlotChanged);
	BoundInventoryComponent = nullptr;
}

void AExceptionPlayerController::RefreshInventoryUI()
{
	UBRInventoryComponent* InventoryComponent = BoundInventoryComponent ? BoundInventoryComponent.Get() : GetPlayerInventoryComponent();
	if (!InventoryComponent)
	{
		return;
	}

	if (UBRPlayerHUDWidget* PlayerHUD = Cast<UBRPlayerHUDWidget>(PlayerHUDWidget))
	{
		const TArray<FBRInventorySlot> Slots = InventoryComponent->GetSlots();
		for (int32 HotbarIndex = 0; HotbarIndex < 3; ++HotbarIndex)
		{
			const int32 InventorySlotIndex = 20 + HotbarIndex;
			PlayerHUD->SetHotbarSlot(HotbarIndex, InventorySlotIndex, Slots.IsValidIndex(InventorySlotIndex) ? Slots[InventorySlotIndex] : FBRInventorySlot());
		}
	}

	if (!InventoryWidget)
	{
		return;
	}

	if (FObjectPropertyBase* InventoryProperty = CastField<FObjectPropertyBase>(InventoryWidget->GetClass()->FindPropertyByName(TEXT("InventoryComponent"))))
	{
		InventoryProperty->SetObjectPropertyValue(InventoryProperty->ContainerPtrToValuePtr<void>(InventoryWidget), InventoryComponent);
	}

	CallWidgetFunc(InventoryWidget, TEXT("SetInventoryComponent"), [InventoryComponent](UFunction* Function, uint8* Params)
	{
		SetObjectParam(Function, Params, InventoryComponent);
	});

	const TArray<FBRInventorySlot> Slots = InventoryComponent->GetSlots();
	CallWidgetFunc(InventoryWidget, TEXT("SetInventorySlots"), [&Slots](UFunction* Function, uint8* Params)
	{
		SetInventorySlotsParam(Function, Params, Slots);
	});

	CallWidgetFunc(InventoryWidget, TEXT("RefreshInventory"), [](UFunction* Function, uint8* Params)
	{
	});
}

void AExceptionPlayerController::RefreshInventorySlot(int32 SlotIndex, const FBRInventorySlot& Slot)
{
	if (!InventoryWidget)
	{
		return;
	}

	CallWidgetFunc(InventoryWidget, TEXT("SetInventorySlot"), [SlotIndex, &Slot](UFunction* Function, uint8* Params)
	{
		SetSlotIndexParam(Function, Params, SlotIndex);
		SetInventorySlotParam(Function, Params, Slot);
	});

	CallWidgetFunc(InventoryWidget, TEXT("RefreshInventory"), [](UFunction* Function, uint8* Params)
	{
	});
}

void AExceptionPlayerController::UseHotbarSlotQ()
{
	UseInventorySlot(20);
}

void AExceptionPlayerController::UseHotbarSlotE()
{
	UseInventorySlot(21);
}

void AExceptionPlayerController::UseHotbarSlotR()
{
	UseInventorySlot(22);
}
