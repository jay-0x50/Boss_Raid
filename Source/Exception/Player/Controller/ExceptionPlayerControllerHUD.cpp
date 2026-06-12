#include "Player/Controller/ExceptionPlayerController.h"

#include "BRInventoryComponent.h"
#include "BRPlayerHUDWidget.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

UUserWidget* AExceptionPlayerController::ShowPlayerHUDWidget()
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

void AExceptionPlayerController::RefreshPlayerHUD()
{
	AExceptionCharacter* ExceptionCharacter = Cast<AExceptionCharacter>(GetPawn());
	if (!ExceptionCharacter)
	{
		return;
	}

	UpdateHPGauge(ExceptionCharacter->GetCurrentHP(), ExceptionCharacter->GetMaxHP(), ExceptionCharacter->GetMaxHP() > 0.0f ? ExceptionCharacter->GetCurrentHP() / ExceptionCharacter->GetMaxHP() : 0.0f);
	UpdateStaminaGauge(ExceptionCharacter->GetCurrentStamina(), ExceptionCharacter->GetMaxStamina(), ExceptionCharacter->GetMaxStamina() > 0.0f ? ExceptionCharacter->GetCurrentStamina() / ExceptionCharacter->GetMaxStamina() : 0.0f);
	UpdateGroggyGauge(0.0f);
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

	AExceptionCharacter* ExceptionCharacter = Cast<AExceptionCharacter>(GetPawn());
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

void AExceptionPlayerController::UpdateRuntimeGauge(FName GaugeWidgetName, const FText& GaugeText, float NormalizedValue, const FLinearColor& GaugeColor)
{
	if (!PlayerHUDWidget || !PlayerHUDWidget->WidgetTree)
	{
		return;
	}

	UWidget* GaugeWidget = PlayerHUDWidget->WidgetTree->FindWidget(GaugeWidgetName);
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

	UFunction* SetGaugeFunction = GaugeWidget->FindFunction(TEXT("SetGauge"));
	if (!SetGaugeFunction)
	{
		return;
	}

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(SetGaugeFunction->ParmsSize));
	FMemory::Memzero(Params, SetGaugeFunction->ParmsSize);

	for (TFieldIterator<FProperty> PropertyIt(SetGaugeFunction); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;
		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Params);

		if (Property->GetFName() == TEXT("NewText"))
		{
			if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
			{
				TextProperty->SetPropertyValue(ValuePtr, GaugeText);
			}
		}
		else if (Property->GetFName() == TEXT("NewPercent"))
		{
			if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
			{
				FloatProperty->SetPropertyValue(ValuePtr, FMath::Clamp(NormalizedValue, 0.0f, 1.0f));
			}
		}
		else if (Property->GetFName() == TEXT("NewColor"))
		{
			if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
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

	auto FindPropertyByName = [](UObject* Object, std::initializer_list<const TCHAR*> CandidateNames) -> FProperty*
	{
		if (!Object)
		{
			return nullptr;
		}

		for (const TCHAR* CandidateName : CandidateNames)
		{
			if (FProperty* Property = Object->GetClass()->FindPropertyByName(FName(CandidateName)))
			{
				return Property;
			}
		}

		return nullptr;
	};

	if (FProperty* GaugeColorProperty = FindPropertyByName(GaugeWidget, {TEXT("GaugeColor"), TEXT("Gauge Color")}))
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(GaugeColorProperty))
		{
			if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
			{
				StructProperty->CopyCompleteValue(StructProperty->ContainerPtrToValuePtr<void>(GaugeWidget), &GaugeColor);
			}
		}
	}

	if (UFunction* RefreshGaugeFunction = GaugeWidget->FindFunction(TEXT("RefreshGauge")))
	{
		GaugeWidget->ProcessEvent(RefreshGaugeFunction, nullptr);
	}

	if (UUserWidget* RuntimeGaugeWidget = Cast<UUserWidget>(GaugeWidget))
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

			if (UTextBlock* StatusText = Cast<UTextBlock>(RuntimeGaugeWidget->WidgetTree->FindWidget(TEXT("StatusText"))))
			{
				StatusText->SetColorAndOpacity(FSlateColor(GaugeColor));
			}
		}
	}
}

void AExceptionPlayerController::UpdateHPGauge(float CurrentValue, float MaxValue, float NormalizedValue)
{
	if (UBRPlayerHUDWidget* PlayerHUD = Cast<UBRPlayerHUDWidget>(PlayerHUDWidget))
	{
		PlayerHUD->SetHP(CurrentValue, MaxValue, NormalizedValue);
		return;
	}

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
	if (UBRPlayerHUDWidget* PlayerHUD = Cast<UBRPlayerHUDWidget>(PlayerHUDWidget))
	{
		PlayerHUD->SetStamina(CurrentValue, MaxValue, NormalizedValue);
		return;
	}

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
	const FString State = ClampedValue >= 1.0f ? TEXT("MAX") : ClampedValue >= 0.5f ? TEXT("HALF") : TEXT("EMPTY");
	UpdateRuntimeGauge(TEXT("GroggyGauge"), FText::FromString(FString::Printf(TEXT("[GROGGY: %s // GAUGE: %03d]"), *State, GaugeValue)), ClampedValue, FLinearColor(1.0f, 0.9f, 0.0f, 1.0f));
}
