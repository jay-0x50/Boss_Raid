#include "Player/Controller/ExceptionPlayerController.h"

#include "BRSaveGameSubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "ExceptionGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UUserWidget* AExceptionPlayerController::ShowTitleMenuWidget()
{
	if (!IsLocalPlayerController())
	{
		return nullptr;
	}

	if (!TitleMenuWidget && TitleMenuWidgetClass)
	{
		TitleMenuWidget = CreateWidget<UUserWidget>(this, TitleMenuWidgetClass);
	}

	if (TitleMenuWidget && !TitleMenuWidget->IsInViewport())
	{
		TitleMenuWidget->AddToPlayerScreen(100);
	}

	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	if (TitleMenuWidget)
	{
		InputMode.SetWidgetToFocus(TitleMenuWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	return TitleMenuWidget;
}

UUserWidget* AExceptionPlayerController::ShowPauseMenuWidget()
{
	if (!IsLocalPlayerController())
	{
		return nullptr;
	}

	if (!PauseMenuWidget && PauseMenuWidgetClass)
	{
		PauseMenuWidget = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
	}

	if (PauseMenuWidget && !PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->AddToPlayerScreen(50);
	}

	SetPause(true);
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (PauseMenuWidget)
	{
		InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	}
	SetInputMode(InputMode);

	return PauseMenuWidget;
}

void AExceptionPlayerController::HidePauseMenuWidget()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
	}
	HideInventoryWidget();

	SetPause(false);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AExceptionPlayerController::TogglePauseMenuWidget()
{
	if (IsPauseMenuOpen())
	{
		HidePauseMenuWidget();
	}
	else
	{
		ShowPauseMenuWidget();
	}
}

bool AExceptionPlayerController::IsPauseMenuOpen() const
{
	return PauseMenuWidget && PauseMenuWidget->IsInViewport();
}

bool AExceptionPlayerController::SaveGameFromPauseMenu()
{
	if (AExceptionCharacter* ExceptionCharacter = Cast<AExceptionCharacter>(GetPawn()))
	{
		if (AExceptionGameMode* ExceptionGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AExceptionGameMode>() : nullptr)
		{
			FTransform ManualSaveTransform = ExceptionCharacter->GetActorTransform();
			ManualSaveTransform.SetScale3D(FVector::OneVector);
			ExceptionGameMode->SetCheckpointTransform(ManualSaveTransform);
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UBRSaveGameSubsystem>())
		{
			return SaveSubsystem->SaveCurrentGame();
		}
	}

	return false;
}

void AExceptionPlayerController::ReturnToTitle()
{
	HidePauseMenuWidget();
	if (!TitleLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, TitleLevelName);
	}
}

void AExceptionPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
