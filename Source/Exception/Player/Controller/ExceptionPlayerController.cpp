// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Controller/ExceptionPlayerController.h"

#include "BRBossStatusWidget.h"
#include "BRInventoryWidget.h"
#include "BRPauseMenuWidget.h"
#include "BRPlayerHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/InputSettings.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Input/SVirtualJoystick.h"

AExceptionPlayerController::AExceptionPlayerController()
{
	BossStatusWidgetClass = UBRBossStatusWidget::StaticClass();

	PlayerHUDWidgetClass = UBRPlayerHUDWidget::StaticClass();

	static ConstructorHelpers::FClassFinder<UUserWidget> TitleMenuFinder(TEXT("/Game/UI/Title/WBP_TitleMenu"));
	if (TitleMenuFinder.Succeeded())
	{
		TitleMenuWidgetClass = TitleMenuFinder.Class;
	}

	PauseMenuWidgetClass = UBRPauseMenuWidget::StaticClass();
	InventoryWidgetClass = UBRInventoryWidget::StaticClass();
}

void AExceptionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsInTitleLevel())
	{
		ShowTitleMenuWidget();
	}
	else
	{
		SetPause(false);
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		ShowPlayerHUDWidget();
		BindPlayerHUDToPawn();
		BindInventoryWidgetToPawn();
	}

	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
	}
}

void AExceptionPlayerController::SetPawn(APawn* InPawn)
{
	UnbindPlayerHUDFromPawn();
	Super::SetPawn(InPawn);
	if (IsInTitleLevel())
	{
		return;
	}
	BindPlayerHUDToPawn();
	BindInventoryWidgetToPawn();
	RefreshPlayerHUD();
}

void AExceptionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AExceptionPlayerController::TogglePauseMenuWidget);
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AExceptionPlayerController::ToggleInventoryWidget);
		InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AExceptionPlayerController::UseHotbarSlotQ);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AExceptionPlayerController::UseHotbarSlotE);
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AExceptionPlayerController::UseHotbarSlotR);
	}

	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AExceptionPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

bool AExceptionPlayerController::IsInTitleLevel() const
{
	const UWorld* World = GetWorld();
	if (!World || TitleLevelName.IsNone())
	{
		return false;
	}

	return FName(*UGameplayStatics::GetCurrentLevelName(World, true)) == TitleLevelName;
}
