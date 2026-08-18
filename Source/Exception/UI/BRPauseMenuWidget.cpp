#include "BRPauseMenuWidget.h"

#include "Player/Character/ExceptionCharacter.h"
#include "Player/Controller/ExceptionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UBRPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildMenuWidget();
	BindDesignerWidgets();
	RefreshMenu();
}

void UBRPauseMenuWidget::RefreshMenu()
{
	const AExceptionCharacter* PlayerCharacter = GetOwningPlayer() ? Cast<AExceptionCharacter>(GetOwningPlayer()->GetPawn()) : nullptr;
	if (!PlayerCharacter)
	{
		return;
	}

	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("LEVEL %d"), PlayerCharacter->GetPlayerLevel())));
	}

	if (PointsText)
	{
		PointsText->SetText(FText::FromString(FString::Printf(
			TEXT("Point %d   XP %d / %d   Dropped %d"),
			PlayerCharacter->GetUpgradePoints(),
			PlayerCharacter->GetCurrentExperience(),
			PlayerCharacter->GetLevelUpExperienceCost(),
			PlayerCharacter->GetDroppedExperience())));
	}

	if (VitalityText)
	{
		VitalityText->SetText(FText::FromString(FString::Printf(TEXT("Vitality  %d   HP %.0f"), PlayerCharacter->GetVitalityLevel(), PlayerCharacter->GetMaxHP())));
	}

	if (EnduranceText)
	{
		EnduranceText->SetText(FText::FromString(FString::Printf(TEXT("Endurance %d   ST %.0f"), PlayerCharacter->GetEnduranceLevel(), PlayerCharacter->GetMaxStamina())));
	}

	if (PowerText)
	{
		PowerText->SetText(FText::FromString(FString::Printf(TEXT("Power     %d"), PlayerCharacter->GetPowerLevel())));
	}
}

void UBRPauseMenuWidget::HandleResumeClicked()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(GetOwningPlayer()))
	{
		ExceptionPC->HidePauseMenuWidget();
	}
}

void UBRPauseMenuWidget::HandleLevelVitalityClicked()
{
	if (AExceptionCharacter* PlayerCharacter = GetOwningPlayer() ? Cast<AExceptionCharacter>(GetOwningPlayer()->GetPawn()) : nullptr)
	{
		if (!PlayerCharacter->SpendUpgradePoint(EBRPlayerUpgradeStat::Vitality) && StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Need 1 point and enough XP.")));
		}
		RefreshMenu();
	}
}

void UBRPauseMenuWidget::HandleLevelEnduranceClicked()
{
	if (AExceptionCharacter* PlayerCharacter = GetOwningPlayer() ? Cast<AExceptionCharacter>(GetOwningPlayer()->GetPawn()) : nullptr)
	{
		if (!PlayerCharacter->SpendUpgradePoint(EBRPlayerUpgradeStat::Endurance) && StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Need 1 point and enough XP.")));
		}
		RefreshMenu();
	}
}

void UBRPauseMenuWidget::HandleLevelPowerClicked()
{
	if (AExceptionCharacter* PlayerCharacter = GetOwningPlayer() ? Cast<AExceptionCharacter>(GetOwningPlayer()->GetPawn()) : nullptr)
	{
		if (!PlayerCharacter->SpendUpgradePoint(EBRPlayerUpgradeStat::Power) && StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Need 1 point and enough XP.")));
		}
		RefreshMenu();
	}
}

void UBRPauseMenuWidget::HandleSaveClicked()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(GetOwningPlayer()))
	{
		const bool bSaved = ExceptionPC->SaveGameFromPauseMenu();
		if (StatusText)
		{
			StatusText->SetText(bSaved ? FText::FromString(TEXT("Save complete.")) : FText::FromString(TEXT("Save failed.")));
		}
	}
}

void UBRPauseMenuWidget::HandleInventoryClicked()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(GetOwningPlayer()))
	{
		ExceptionPC->ShowInventoryWidget();
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Inventory opened.")));
		}
	}
}

void UBRPauseMenuWidget::HandleTitleClicked()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(GetOwningPlayer()))
	{
		ExceptionPC->ReturnToTitle();
	}
}

void UBRPauseMenuWidget::HandleQuitClicked()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(GetOwningPlayer()))
	{
		ExceptionPC->QuitGame();
	}
}

void UBRPauseMenuWidget::BuildMenuWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PausePanel"));
	MainBorder->SetBrushColor(FLinearColor(0.02f, 0.018f, 0.014f, 0.92f));
	MainBorder->SetPadding(FMargin(26.0f));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(MainBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		PanelSlot->SetPosition(FVector2D(54.0f, 0.0f));
		PanelSlot->SetSize(FVector2D(520.0f, 560.0f));
	}

	UHorizontalBox* MainBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PauseMainBox"));
	MainBorder->SetContent(MainBox);

	UVerticalBox* CommandBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CommandBox"));
	if (UHorizontalBoxSlot* CommandSlot = MainBox->AddChildToHorizontalBox(CommandBox))
	{
		CommandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CommandSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuTitle"));
	TitleText->SetText(FText::FromString(TEXT("SYSTEM")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.82f, 0.66f, 1.0f)));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 28));
	if (UVerticalBoxSlot* TitleSlot = CommandBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuStatusText"));
	StatusText->SetText(FText::FromString(TEXT("Rest at checkpoint.")));
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 0.95f, 0.86f, 1.0f)));
	StatusText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 13));
	if (UVerticalBoxSlot* StatusSlot = CommandBox->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UButton* ResumeButton = AddMenuButton(CommandBox, FText::FromString(TEXT("Resume")));
	ResumeButton->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleResumeClicked);

	UButton* SaveButton = AddMenuButton(CommandBox, FText::FromString(TEXT("Save")));
	SaveButton->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleSaveClicked);

	UButton* InventoryButton = AddMenuButton(CommandBox, FText::FromString(TEXT("Inventory")));
	InventoryButton->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleInventoryClicked);

	UButton* TitleButton = AddMenuButton(CommandBox, FText::FromString(TEXT("Return to Title")));
	TitleButton->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleTitleClicked);

	UButton* QuitButton = AddMenuButton(CommandBox, FText::FromString(TEXT("Quit Game")));
	QuitButton->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleQuitClicked);

	UVerticalBox* LevelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LevelBox"));
	if (UHorizontalBoxSlot* LevelSlot = MainBox->AddChildToHorizontalBox(LevelBox))
	{
		LevelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelText"));
	LevelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	LevelText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 24));
	LevelBox->AddChildToVerticalBox(LevelText);

	PointsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PointsText"));
	PointsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.82f, 0.66f, 1.0f)));
	if (UVerticalBoxSlot* PointsSlot = LevelBox->AddChildToVerticalBox(PointsText))
	{
		PointsSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 18.0f));
	}

	VitalityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VitalityText"));
	EnduranceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnduranceText"));
	PowerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PowerText"));

	for (UTextBlock* StatText : {VitalityText.Get(), EnduranceText.Get(), PowerText.Get()})
	{
		StatText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		StatText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
		LevelBox->AddChildToVerticalBox(StatText);
	}

	UTextBlock* SettingsTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CheckpointTitleText"));
	SettingsTitleText->SetText(FText::FromString(TEXT("CHECKPOINT")));
	SettingsTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 0.95f, 0.86f, 1.0f)));
	SettingsTitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18));
	if (UVerticalBoxSlot* SettingsTitleSlot = LevelBox->AddChildToVerticalBox(SettingsTitleText))
	{
		SettingsTitleSlot->SetPadding(FMargin(0.0f, 22.0f, 0.0f, 8.0f));
	}

	const TCHAR* SettingsLines[] =
	{
		TEXT("Spend XP to level one stat."),
		TEXT("Unspent XP drops on death."),
		TEXT("Recover your grave to reclaim it.")
	};

	for (const TCHAR* SettingsLine : SettingsLines)
	{
		UTextBlock* SettingsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SettingsText->SetText(FText::FromString(SettingsLine));
		SettingsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.86f, 0.76f, 1.0f)));
		SettingsText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 13));
		LevelBox->AddChildToVerticalBox(SettingsText);
	}

	UButton* VitalityButton = AddMenuButton(LevelBox, FText::FromString(TEXT("Level Vitality")));
	VitalityButton->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleLevelVitalityClicked);

	UButton* EnduranceButton = AddMenuButton(LevelBox, FText::FromString(TEXT("Level Endurance")));
	EnduranceButton->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleLevelEnduranceClicked);

	UButton* PowerButton = AddMenuButton(LevelBox, FText::FromString(TEXT("Level Power")));
	PowerButton->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleLevelPowerClicked);
}

void UBRPauseMenuWidget::BindDesignerWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	auto FindButton = [this](const TCHAR* Name)
	{
		return Cast<UButton>(WidgetTree->FindWidget(FName(Name)));
	};
	auto FindText = [this](const TCHAR* Name)
	{
		return Cast<UTextBlock>(WidgetTree->FindWidget(FName(Name)));
	};

	if (UButton* Button = FindButton(TEXT("Button_Resume")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleResumeClicked);
	}
	if (UButton* Button = FindButton(TEXT("Button_SaveGame")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleSaveClicked);
	}
	if (UButton* Button = FindButton(TEXT("Button_Inventory")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleInventoryClicked);
	}
	if (UButton* Button = FindButton(TEXT("Button_ReturnTitle")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleTitleClicked);
	}
	if (UButton* Button = FindButton(TEXT("Button_QuitGame")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleQuitClicked);
	}
	if (UButton* Button = FindButton(TEXT("VitButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleLevelVitalityClicked);
	}
	if (UButton* Button = FindButton(TEXT("EndButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleLevelEnduranceClicked);
	}
	if (UButton* Button = FindButton(TEXT("PowerButton")))
	{
		Button->OnClicked.AddUniqueDynamic(this, &UBRPauseMenuWidget::HandleLevelPowerClicked);
	}

	if (UTextBlock* Text = FindText(TEXT("LevelValue")))
	{
		LevelText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("PointValue")))
	{
		PointsText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("VitValue")))
	{
		VitalityText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("EndValue")))
	{
		EnduranceText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("PowerValue")))
	{
		PowerText = Text;
	}
	if (UTextBlock* Text = FindText(TEXT("MenuStatus")))
	{
		StatusText = Text;
	}
}

UButton* UBRPauseMenuWidget::AddMenuButton(UVerticalBox* ParentBox, const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(FLinearColor(0.22f, 0.2f, 0.16f, 0.85f));

	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ButtonText->SetText(Label);
	ButtonText->SetJustification(ETextJustify::Center);
	ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.86f, 0.76f, 1.0f)));
	ButtonText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
	Button->AddChild(ButtonText);

	if (UVerticalBoxSlot* ButtonSlot = ParentBox->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	return Button;
}
