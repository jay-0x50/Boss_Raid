// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExceptionPlayerController.h"
#include "BRBossStatusWidget.h"
#include "BRInventoryComponent.h"
#include "BRSaveGameSubsystem.h"
#include "ExceptionCharacter.h"
#include "ExceptionGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/InputSettings.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Exception.h"
#include "Widgets/Input/SVirtualJoystick.h"

AExceptionPlayerController::AExceptionPlayerController()
{
	BossStatusWidgetClass = UBRBossStatusWidget::StaticClass();

	static ConstructorHelpers::FClassFinder<UUserWidget> PlayerHUDFinder(TEXT("/Game/UI/HUD/WBP_PlayerHUD"));
	if (PlayerHUDFinder.Succeeded())
	{
		PlayerHUDWidgetClass = PlayerHUDFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> TitleMenuFinder(TEXT("/Game/UI/Title/WBP_TitleMenu"));
	if (TitleMenuFinder.Succeeded())
	{
		TitleMenuWidgetClass = TitleMenuFinder.Class;
	}
}

UBRBossStatusWidget *AExceptionPlayerController::ShowBossStatusWidget()
{
	if (!IsLocalPlayerController())
	{
		return nullptr;
	}

	if (!BossStatusWidgetClass)
	{
		BossStatusWidgetClass = UBRBossStatusWidget::StaticClass();
	}

	if (!BossStatusWidget && BossStatusWidgetClass)
	{
		BossStatusWidget = CreateWidget<UBRBossStatusWidget>(this, BossStatusWidgetClass);
	}

	if (BossStatusWidget && !BossStatusWidget->IsInViewport())
	{
		BossStatusWidget->AddToPlayerScreen(10);
	}

	return BossStatusWidget;
}

void AExceptionPlayerController::HideBossStatusWidget()
{
	if (BossStatusWidget)
	{
		BossStatusWidget->RemoveFromParent();
		BossStatusWidget->ClearBosses();
	}
}

UUserWidget *AExceptionPlayerController::ShowPlayerHUDWidget()
{
	if (!IsLocalPlayerController())
	{
		return nullptr;
	}

	if (!PlayerHUDWidget && PlayerHUDWidgetClass)
	{
		PlayerHUDWidget = CreateWidget<UUserWidget>(this, PlayerHUDWidgetClass);
	}

	if (PlayerHUDWidget && !PlayerHUDWidget->IsInViewport())
	{
		PlayerHUDWidget->AddToPlayerScreen(1);
	}

	RefreshPlayerHUD();
	return PlayerHUDWidget;
}

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

void AExceptionPlayerController::RefreshPlayerHUD()
{
	AExceptionCharacter *ExceptionCharacter = Cast<AExceptionCharacter>(GetPawn());
	if (!ExceptionCharacter)
	{
		return;
	}

	UpdateHPGauge(ExceptionCharacter->GetCurrentHP(), ExceptionCharacter->GetMaxHP(), ExceptionCharacter->GetMaxHP() > 0.0f ? ExceptionCharacter->GetCurrentHP() / ExceptionCharacter->GetMaxHP() : 0.0f);
	UpdateStaminaGauge(ExceptionCharacter->GetCurrentStamina(), ExceptionCharacter->GetMaxStamina(), ExceptionCharacter->GetMaxStamina() > 0.0f ? ExceptionCharacter->GetCurrentStamina() / ExceptionCharacter->GetMaxStamina() : 0.0f);
	UpdateGroggyGauge(0.0f);
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
	}

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{

			UE_LOG(LogException, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void AExceptionPlayerController::SetPawn(APawn *InPawn)
{
	UnbindPlayerHUDFromPawn();
	Super::SetPawn(InPawn);
	if (IsInTitleLevel())
	{
		return;
	}
	BindPlayerHUDToPawn();
	RefreshPlayerHUD();
}

void AExceptionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AExceptionPlayerController::TogglePauseMenuWidget);
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext *CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext *CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AExceptionPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AExceptionPlayerController::HandlePlayerHPChanged(float CurrentValue, float MaxValue, float NormalizedValue)
{
	UpdateHPGauge(CurrentValue, MaxValue, NormalizedValue);
}

void AExceptionPlayerController::HandlePlayerStaminaChanged(float CurrentValue, float MaxValue, float NormalizedValue)
{
	UpdateStaminaGauge(CurrentValue, MaxValue, NormalizedValue);
}

void AExceptionPlayerController::BindPlayerHUDToPawn()
{
	if (!IsLocalPlayerController() || IsInTitleLevel())
	{
		return;
	}

	AExceptionCharacter *ExceptionCharacter = Cast<AExceptionCharacter>(GetPawn());
	if (!ExceptionCharacter || BoundHUDCharacter == ExceptionCharacter)
	{
		return;
	}

	BoundHUDCharacter = ExceptionCharacter;
	BoundHUDCharacter->OnHPChanged.AddUniqueDynamic(this, &AExceptionPlayerController::HandlePlayerHPChanged);
	BoundHUDCharacter->OnStaminaChanged.AddUniqueDynamic(this, &AExceptionPlayerController::HandlePlayerStaminaChanged);
}

void AExceptionPlayerController::UnbindPlayerHUDFromPawn()
{
	if (!BoundHUDCharacter)
	{
		return;
	}

	BoundHUDCharacter->OnHPChanged.RemoveDynamic(this, &AExceptionPlayerController::HandlePlayerHPChanged);
	BoundHUDCharacter->OnStaminaChanged.RemoveDynamic(this, &AExceptionPlayerController::HandlePlayerStaminaChanged);
	BoundHUDCharacter = nullptr;
}

void AExceptionPlayerController::UpdateRuntimeGauge(FName GaugeWidgetName, const FText &GaugeText, float NormalizedValue, const FLinearColor &GaugeColor)
{
	if (!PlayerHUDWidget || !PlayerHUDWidget->WidgetTree)
	{
		return;
	}

	UWidget *GaugeWidget = PlayerHUDWidget->WidgetTree->FindWidget(GaugeWidgetName);
	if (!GaugeWidget)
	{
		TArray<UWidget*> PlayerHUDWidgets;
		PlayerHUDWidget->WidgetTree->GetAllWidgets(PlayerHUDWidgets);

		const FString GaugeNameText = GaugeWidgetName.ToString();
		for (UWidget* ChildWidget : PlayerHUDWidgets)
		{
			if (ChildWidget && ChildWidget->GetName().Contains(GaugeNameText))
			{
				GaugeWidget = ChildWidget;
				break;
			}
		}
	}

	if (!GaugeWidget)
	{
		return;
	}

	UFunction *SetGaugeFunction = GaugeWidget->FindFunction(TEXT("SetGauge"));
	if (!SetGaugeFunction)
	{
		return;
	}

	uint8 *Params = static_cast<uint8 *>(FMemory_Alloca(SetGaugeFunction->ParmsSize));
	FMemory::Memzero(Params, SetGaugeFunction->ParmsSize);

	for (TFieldIterator<FProperty> PropertyIt(SetGaugeFunction); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
	{
		FProperty *Property = *PropertyIt;
		void *ValuePtr = Property->ContainerPtrToValuePtr<void>(Params);

		if (Property->GetFName() == TEXT("NewText"))
		{
			if (FTextProperty *TextProperty = CastField<FTextProperty>(Property))
			{
				TextProperty->SetPropertyValue(ValuePtr, GaugeText);
			}
		}
		else if (Property->GetFName() == TEXT("NewPercent"))
		{
			if (FFloatProperty *FloatProperty = CastField<FFloatProperty>(Property))
			{
				FloatProperty->SetPropertyValue(ValuePtr, FMath::Clamp(NormalizedValue, 0.0f, 1.0f));
			}
		}
		else if (Property->GetFName() == TEXT("NewColor"))
		{
			if (FStructProperty *StructProperty = CastField<FStructProperty>(Property))
			{
				if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
				{
					StructProperty->CopyCompleteValue(ValuePtr, &GaugeColor);
				}
			}
		}
	}

	GaugeWidget->ProcessEvent(SetGaugeFunction, Params);

	for (TFieldIterator<FProperty> PropertyIt(SetGaugeFunction); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
	{
		PropertyIt->DestroyValue_InContainer(Params);
	}

	auto FindPropertyByName = [](UObject *Object, std::initializer_list<const TCHAR *> CandidateNames) -> FProperty *
	{
		if (!Object)
		{
			return nullptr;
		}

		for (const TCHAR *CandidateName : CandidateNames)
		{
			if (FProperty *Property = Object->GetClass()->FindPropertyByName(FName(CandidateName)))
			{
				return Property;
			}
		}

		return nullptr;
	};

	if (FProperty *GaugeColorProperty = FindPropertyByName(GaugeWidget, {TEXT("GaugeColor"), TEXT("Gauge Color")}))
	{
		if (FStructProperty *StructProperty = CastField<FStructProperty>(GaugeColorProperty))
		{
			if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
			{
				StructProperty->CopyCompleteValue(StructProperty->ContainerPtrToValuePtr<void>(GaugeWidget), &GaugeColor);
			}
		}
	}

	if (UFunction *RefreshGaugeFunction = GaugeWidget->FindFunction(TEXT("RefreshGauge")))
	{
		GaugeWidget->ProcessEvent(RefreshGaugeFunction, nullptr);
	}

	if (UUserWidget *RuntimeGaugeWidget = Cast<UUserWidget>(GaugeWidget))
	{
		if (RuntimeGaugeWidget->WidgetTree)
		{
			TArray<UWidget*> RuntimeGaugeWidgets;
			RuntimeGaugeWidget->WidgetTree->GetAllWidgets(RuntimeGaugeWidgets);

			for (UWidget* ChildWidget : RuntimeGaugeWidgets)
			{
				if (UProgressBar* GaugeBar = Cast<UProgressBar>(ChildWidget))
				{
					FProgressBarStyle Style = GaugeBar->WidgetStyle;
					Style.FillImage.TintColor = FSlateColor(GaugeColor);
					GaugeBar->SetWidgetStyle(Style);

					GaugeBar->SetPercent(FMath::Clamp(NormalizedValue, 0.0f, 1.0f));
					GaugeBar->SetFillColorAndOpacity(GaugeColor);
				}
			}

			if (UTextBlock *StatusText = Cast<UTextBlock>(RuntimeGaugeWidget->WidgetTree->FindWidget(TEXT("StatusText"))))
			{
				StatusText->SetColorAndOpacity(FSlateColor(GaugeColor));
			}
		}
	}
}

void AExceptionPlayerController::UpdateHPGauge(float CurrentValue, float MaxValue, float NormalizedValue)
{
	const float ClampedValue = FMath::Clamp(NormalizedValue, 0.0f, 1.0f);
	const int32 Percent = FMath::RoundToInt(ClampedValue * 100.0f);
	FLinearColor HPColor(0.0f, 0.83f, 1.0f, 1.0f);
	FString Status = TEXT("STABLE");

	if (ClampedValue <= 0.3f)
	{
		HPColor = FLinearColor(1.0f, 0.18f, 0.33f, 1.0f);
		Status = TEXT("CRITICAL_DANGER");
	}
	else if (ClampedValue <= 0.5f)
	{
		HPColor = FLinearColor(1.0f, 0.55f, 0.0f, 1.0f);
		Status = TEXT("CRITICAL_WARNING");
	}

	const FString HPValue = Percent >= 100 ? TEXT("FULL") : FString::Printf(TEXT("%d%%"), Percent);
	UpdateRuntimeGauge(TEXT("HPGauge"), FText::FromString(FString::Printf(TEXT("[HP: %s // STATUS: %s]"), *HPValue, *Status)), ClampedValue, HPColor);
}

void AExceptionPlayerController::UpdateStaminaGauge(float CurrentValue, float MaxValue, float NormalizedValue)
{
	const float ClampedValue = FMath::Clamp(NormalizedValue, 0.0f, 1.0f);
	const int32 Percent = FMath::RoundToInt(ClampedValue * 100.0f);
	FLinearColor StaminaColor(0.0f, 0.95f, 0.55f, 1.0f);
	FString Status = TEXT("READY");

	if (ClampedValue <= 0.05f)
	{
		StaminaColor = FLinearColor(0.55f, 0.55f, 0.6f, 1.0f);
		Status = TEXT("EMPTY");
	}
	else if (ClampedValue <= 0.3f)
	{
		StaminaColor = FLinearColor(1.0f, 0.42f, 0.12f, 1.0f);
		Status = TEXT("LOW");
	}
	else if (ClampedValue < 1.0f)
	{
		StaminaColor = FLinearColor(1.0f, 0.86f, 0.18f, 1.0f);
		Status = TEXT("DEPLETED_REFRESH");
	}

	UpdateRuntimeGauge(TEXT("StaminaGauge"), FText::FromString(FString::Printf(TEXT("[ST: %d%% // STATUS: %s]"), Percent, *Status)), ClampedValue, StaminaColor);
}

void AExceptionPlayerController::UpdateGroggyGauge(float NormalizedValue)
{
	const float ClampedValue = FMath::Clamp(NormalizedValue, 0.0f, 1.0f);
	const int32 GaugeValue = FMath::RoundToInt(ClampedValue * 100.0f);
	const FString State = ClampedValue >= 1.0f ? TEXT("MAX") : ClampedValue >= 0.5f ? TEXT("HALF")
																					: TEXT("EMPTY");
	UpdateRuntimeGauge(TEXT("GroggyGauge"), FText::FromString(FString::Printf(TEXT("[GROGGY: %s // GAUGE: %03d]"), *State, GaugeValue)), ClampedValue, FLinearColor(1.0f, 0.9f, 0.0f, 1.0f));
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
