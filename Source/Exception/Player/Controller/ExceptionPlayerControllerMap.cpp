#include "Player/Controller/ExceptionPlayerController.h"

#include "BRWorldMapWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"

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
		// The demo is single-player. A global viewport layer keeps the minimap
		// stable across PIE map travel and above the player-screen HUD layer.
		WorldMapWidget->AddToViewport(35);
		WorldMapWidget->SetAnchorsInViewport(FAnchors(0.0f, 0.0f));
		WorldMapWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
		WorldMapWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
		int32 ViewportSizeX = 0;
		int32 ViewportSizeY = 0;
		GetViewportSize(ViewportSizeX, ViewportSizeY);
		if (ViewportSizeX > 0 && ViewportSizeY > 0)
		{
			const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
			WorldMapWidget->SetDesiredSizeInViewport(FVector2D(ViewportSizeX, ViewportSizeY) / ViewportScale);
		}
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
