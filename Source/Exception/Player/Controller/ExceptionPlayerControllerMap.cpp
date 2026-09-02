#include "Player/Controller/ExceptionPlayerController.h"

#include "BRWorldMapWidget.h"
#include "Blueprint/UserWidget.h"

UBRWorldMapWidget* AExceptionPlayerController::ShowWorldMapWidget()
{
	if (!IsLocalPlayerController() || IsInTitleLevel())
	{
		return nullptr;
	}

	if (!WorldMapWidget && WorldMapWidgetClass)
	{
		WorldMapWidget = CreateWidget<UBRWorldMapWidget>(this, WorldMapWidgetClass);
	}
	if (WorldMapWidget && !WorldMapWidget->IsInViewport())
	{
		WorldMapWidget->AddToPlayerScreen(35);
	}
	return WorldMapWidget;
}

void AExceptionPlayerController::HideWorldMapWidget()
{
	if (WorldMapWidget)
	{
		WorldMapWidget->SetFullMapMode(false);
	}

	if (!IsPauseMenuOpen() && !IsInventoryOpen() && !IsInTitleLevel())
	{
		SetPause(false);
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void AExceptionPlayerController::ToggleWorldMapWidget()
{
	if (!IsLocalPlayerController() || IsInTitleLevel())
	{
		return;
	}

	if (IsWorldMapOpen())
	{
		HideWorldMapWidget();
		return;
	}

	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
	}
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}

	if (UBRWorldMapWidget* MapWidget = ShowWorldMapWidget())
	{
		MapWidget->SetFullMapMode(true);
		SetPause(true);
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}

bool AExceptionPlayerController::IsWorldMapOpen() const
{
	return WorldMapWidget && WorldMapWidget->IsInViewport() && WorldMapWidget->IsFullMapMode();
}
