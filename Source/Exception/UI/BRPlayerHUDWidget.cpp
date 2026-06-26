#include "BRPlayerHUDWidget.h"

#include "BRInventorySlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UBRPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildHUDWidget();
}

void UBRPlayerHUDWidget::SetHP(float CurrentValue, float MaxValue, float NormalizedValue)
{
	if (HPBar)
	{
		HPBar->SetPercent(FMath::Clamp(NormalizedValue, 0.0f, 1.0f));
		HPBar->SetFillColorAndOpacity(FLinearColor(0.72f, 0.04f, 0.08f, 1.0f));
	}

	if (HPText)
	{
		HPText->SetText(FText::FromString(FString::Printf(TEXT("HP %.0f / %.0f"), CurrentValue, MaxValue)));
	}
}

void UBRPlayerHUDWidget::SetStamina(float CurrentValue, float MaxValue, float NormalizedValue)
{
	if (StaminaBar)
	{
		StaminaBar->SetPercent(FMath::Clamp(NormalizedValue, 0.0f, 1.0f));
		StaminaBar->SetFillColorAndOpacity(FLinearColor(0.12f, 0.58f, 0.28f, 1.0f));
	}

	if (StaminaText)
	{
		StaminaText->SetText(FText::FromString(FString::Printf(TEXT("ST %.0f / %.0f"), CurrentValue, MaxValue)));
	}
}

void UBRPlayerHUDWidget::SetHotbarSlot(int32 HotbarIndex, int32 InventorySlotIndex, const FBRInventorySlot& InventorySlot)
{
	if (HotbarSlots.IsValidIndex(HotbarIndex) && HotbarSlots[HotbarIndex])
	{
		HotbarSlots[HotbarIndex]->SetSlotData(InventorySlotIndex, InventorySlot);
	}
}

void UBRPlayerHUDWidget::BuildHUDWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UVerticalBox* StatBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatBox"));
	if (UCanvasPanelSlot* StatCanvasSlot = RootCanvas->AddChildToCanvas(StatBox))
	{
		StatCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		StatCanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		StatCanvasSlot->SetPosition(FVector2D(42.0f, 34.0f));
		StatCanvasSlot->SetSize(FVector2D(430.0f, 98.0f));
	}

	auto MakeBarRow = [this, StatBox](const TCHAR* Name, const FLinearColor& Color, TObjectPtr<UProgressBar>& OutBar, TObjectPtr<UTextBlock>& OutText)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(Name));
		if (UVerticalBoxSlot* RowSlot = StatBox->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}

		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BarSize->SetWidthOverride(310.0f);
		BarSize->SetHeightOverride(14.0f);
		if (UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(BarSize))
		{
			BarSlot->SetPadding(FMargin(0.0f, 4.0f, 10.0f, 0.0f));
		}

		OutBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		OutBar->SetPercent(1.0f);
		OutBar->SetFillColorAndOpacity(Color);
		BarSize->AddChild(OutBar);

		OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.86f, 0.76f, 1.0f)));
		OutText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12));
		Row->AddChildToHorizontalBox(OutText);
	};

	MakeBarRow(TEXT("HPRow"), FLinearColor(0.72f, 0.04f, 0.08f, 1.0f), HPBar, HPText);
	MakeBarRow(TEXT("StaminaRow"), FLinearColor(0.12f, 0.58f, 0.28f, 1.0f), StaminaBar, StaminaText);

	UVerticalBox* ShortcutBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShortcutBox"));
	if (UCanvasPanelSlot* ShortcutCanvasSlot = RootCanvas->AddChildToCanvas(ShortcutBox))
	{
		ShortcutCanvasSlot->SetAnchors(FAnchors(0.0f, 1.0f));
		ShortcutCanvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		ShortcutCanvasSlot->SetPosition(FVector2D(44.0f, -42.0f));
		ShortcutCanvasSlot->SetSize(FVector2D(320.0f, 132.0f));
	}

	UTextBlock* ShortcutTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShortcutTitle"));
	ShortcutTitle->SetText(FText::FromString(TEXT("SHORTCUT / INVENTORY")));
	ShortcutTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.72f, 0.62f, 1.0f)));
	ShortcutTitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
	ShortcutBox->AddChildToVerticalBox(ShortcutTitle);

	UTextBlock* MenuHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MenuHintText"));
	MenuHintText->SetText(FText::FromString(TEXT("[I] Inventory   [ESC] Settings")));
	MenuHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.86f, 0.76f, 1.0f)));
	MenuHintText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12));
	if (UVerticalBoxSlot* HintSlot = ShortcutBox->AddChildToVerticalBox(MenuHintText))
	{
		HintSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	}

	UHorizontalBox* HotbarBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HotbarBox"));
	if (UVerticalBoxSlot* HotbarBoxSlot = ShortcutBox->AddChildToVerticalBox(HotbarBox))
	{
		HotbarBoxSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	}

	const TCHAR* KeyLabels[] = {TEXT("[Q]"), TEXT("[E]"), TEXT("[R]")};
	HotbarSlots.SetNum(3);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UVerticalBox* EntryBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* EntrySlot = HotbarBox->AddChildToHorizontalBox(EntryBox))
		{
			EntrySlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		}

		UBRInventorySlotWidget* SlotWidget = CreateWidget<UBRInventorySlotWidget>(GetOwningPlayer(), UBRInventorySlotWidget::StaticClass());
		HotbarSlots[Index] = SlotWidget;
		EntryBox->AddChildToVerticalBox(SlotWidget);

		UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		KeyText->SetText(FText::FromString(KeyLabels[Index]));
		KeyText->SetJustification(ETextJustify::Center);
		KeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.86f, 0.76f, 1.0f)));
		EntryBox->AddChildToVerticalBox(KeyText);
	}
}
