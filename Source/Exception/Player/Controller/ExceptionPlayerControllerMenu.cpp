#include "Player/Controller/ExceptionPlayerController.h"

#include "BRSaveGameSubsystem.h"
#include "BRNarrativeQueueSubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "ExceptionGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

UUserWidget* AExceptionPlayerController::ShowTitleMenuWidget()
{
	if (!IsLocalPlayerController())
	{
		return nullptr;
	}

	if (!TitleMenuWidget && TitleMenuWidgetClass)
	{
		TArray<UUserWidget*> ExistingMenus;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ExistingMenus, TitleMenuWidgetClass, false);
		for (UUserWidget* ExistingMenu : ExistingMenus)
		{
			if (ExistingMenu && ExistingMenu->IsInViewport())
			{
				TitleMenuWidget = ExistingMenu;
				break;
			}
		}
		if (!TitleMenuWidget)
		{
			TitleMenuWidget = CreateWidget<UUserWidget>(this, TitleMenuWidgetClass);
		}
	}

	if (TitleMenuWidget && !TitleMenuWidget->IsInViewport())
	{
		// The legacy title Level Blueprint adds its widget to the global viewport.
		// Keep the controller-owned fallback in the same layer at a higher Z order,
		// so it cannot be covered by a late legacy instance during map startup.
		TitleMenuWidget->AddToViewport(100);
	}

	BindTitleMenuActions();
	RefreshTitleMenuState();

	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	UWidget* TitleFocusWidget = TitleMenuWidget;
	if (TitleMenuWidget)
	{
		TitleFocusWidget = TitleMenuWidget->GetWidgetFromName(TEXT("NewGameButton"));
		if (!TitleFocusWidget)
		{
			TitleFocusWidget = TitleMenuWidget->GetWidgetFromName(TEXT("StartButton"));
		}
		if (!TitleFocusWidget)
		{
			TitleFocusWidget = TitleMenuWidget;
		}
		InputMode.SetWidgetToFocus(TitleFocusWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	if (TitleFocusWidget && TitleFocusWidget != TitleMenuWidget)
	{
		TitleFocusWidget->SetUserFocus(this);
	}

	return TitleMenuWidget;
}

void AExceptionPlayerController::BindTitleMenuActions()
{
	if (!TitleMenuWidget)
	{
		return;
	}

	UButton* NewGameButton = Cast<UButton>(TitleMenuWidget->GetWidgetFromName(TEXT("NewGameButton")));
	if (!NewGameButton)
	{
		NewGameButton = Cast<UButton>(TitleMenuWidget->GetWidgetFromName(TEXT("StartButton")));
	}
	if (NewGameButton)
	{
		NewGameButton->OnClicked.Clear();
		NewGameButton->OnClicked.AddUniqueDynamic(this, &AExceptionPlayerController::HandleTitleNewGameClicked);
	}
	if (UButton* ContinueButton = Cast<UButton>(TitleMenuWidget->GetWidgetFromName(TEXT("ContinueButton"))))
	{
		ContinueButton->OnClicked.Clear();
		ContinueButton->OnClicked.AddUniqueDynamic(this, &AExceptionPlayerController::HandleTitleContinueClicked);
	}
	if (UButton* OptionsButton = Cast<UButton>(TitleMenuWidget->GetWidgetFromName(TEXT("OptionsButton"))))
	{
		OptionsButton->OnClicked.Clear();
		OptionsButton->OnClicked.AddUniqueDynamic(this, &AExceptionPlayerController::HandleTitleOptionsClicked);
	}
	if (UButton* QuitButton = Cast<UButton>(TitleMenuWidget->GetWidgetFromName(TEXT("QuitButton"))))
	{
		QuitButton->OnClicked.Clear();
		QuitButton->OnClicked.AddUniqueDynamic(this, &AExceptionPlayerController::HandleTitleQuitClicked);
	}
}

void AExceptionPlayerController::RefreshTitleMenuState()
{
	if (!TitleMenuWidget)
	{
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const UBRSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UBRSaveGameSubsystem>() : nullptr;
	if (UButton* ContinueButton = Cast<UButton>(TitleMenuWidget->GetWidgetFromName(TEXT("ContinueButton"))))
	{
		ContinueButton->SetIsEnabled(SaveSubsystem && SaveSubsystem->DoesSaveExist());
	}
}

void AExceptionPlayerController::SetTitleMenuStatus(const FText& Message)
{
	if (!TitleMenuWidget)
	{
		return;
	}

	UWidget* StatusWidget = TitleMenuWidget->GetWidgetFromName(TEXT("Text_LogMessage"));
	if (UEditableTextBox* EditableStatus = Cast<UEditableTextBox>(StatusWidget))
	{
		EditableStatus->SetText(Message);
	}
	else if (UTextBlock* TextStatus = Cast<UTextBlock>(StatusWidget))
	{
		TextStatus->SetText(Message);
	}
}

void AExceptionPlayerController::HandleTitleNewGameClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Title New Game selected; opening %s."), *GameplayLevelName.ToString());

	UGameInstance* GameInstance = GetGameInstance();
	UBRSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UBRSaveGameSubsystem>() : nullptr;
	if (!SaveSubsystem || !SaveSubsystem->DeleteSave())
	{
		SetTitleMenuStatus(FText::FromString(TEXT("NEW GAME // Failed to clear existing save data.")));
		RefreshTitleMenuState();
		return;
	}

	if (GameplayLevelName.IsNone())
	{
		SetTitleMenuStatus(FText::FromString(TEXT("NEW GAME // Gameplay level is not configured.")));
		return;
	}

	UGameplayStatics::OpenLevel(this, GameplayLevelName);
}

void AExceptionPlayerController::HandleTitleContinueClicked()
{
	UGameInstance* GameInstance = GetGameInstance();
	UBRSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UBRSaveGameSubsystem>() : nullptr;
	if (!SaveSubsystem || !SaveSubsystem->DoesSaveExist() || !SaveSubsystem->LoadGameFromSlotAndOpenLevel())
	{
		SetTitleMenuStatus(FText::FromString(TEXT("CONTINUE // No valid save data was found.")));
		RefreshTitleMenuState();
	}
}

void AExceptionPlayerController::HandleTitleOptionsClicked()
{
	SetTitleMenuStatus(FText::FromString(TEXT("OPTIONS // Configuration is available from the in-game SYSTEM menu.")));
}

void AExceptionPlayerController::HandleTitleQuitClicked()
{
	QuitGame();
}

UUserWidget* AExceptionPlayerController::ShowPauseMenuWidget()
{
	if (!IsLocalPlayerController())
	{
		return nullptr;
	}

	if (IsWorldMapOpen())
	{
		HideWorldMapWidget();
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
	if (IsWorldMapOpen())
	{
		HideWorldMapWidget();
		return;
	}

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
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRNarrativeQueueSubsystem* NarrativeQueue = GameInstance->GetSubsystem<UBRNarrativeQueueSubsystem>())
		{
			NarrativeQueue->ClearMessages();
		}
	}
	if (!TitleLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, TitleLevelName);
	}
}

void AExceptionPlayerController::QuitGame()
{
	if (const UWorld* World = GetWorld())
	{
		if (World->WorldType == EWorldType::PIE)
		{
#if WITH_EDITOR
			if (GEditor)
			{
				GEditor->RequestEndPlayMap();
				return;
			}
#endif

			return;
		}
	}

	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
