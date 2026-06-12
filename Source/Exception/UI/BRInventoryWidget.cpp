#include "BRInventoryWidget.h"

#include "BRInventoryComponent.h"
#include "BRInventorySlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UBRInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildInventoryWidget();
	RefreshInventory();
}

void UBRInventoryWidget::SetInventoryComponent(UBRInventoryComponent* NewInventoryComponent)
{
	InventoryComponent = NewInventoryComponent;
	RefreshInventory();
}

void UBRInventoryWidget::SetInventorySlots(const TArray<FBRInventorySlot>& Slots)
{
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		SetInventorySlot(SlotIndex, Slots[SlotIndex]);
	}
}

void UBRInventoryWidget::SetInventorySlot(int32 SlotIndex, const FBRInventorySlot& InventorySlot)
{
	if (!SlotWidgets.IsValidIndex(SlotIndex) || !SlotWidgets[SlotIndex])
	{
		return;
	}

	SlotWidgets[SlotIndex]->SetSlotData(SlotIndex, InventorySlot);
	if (!InventorySlot.IsEmpty())
	{
		UpdateDetailsPanel(InventorySlot);
	}
}

void UBRInventoryWidget::RefreshInventory()
{
	if (!InventoryComponent)
	{
		return;
	}

	SetInventorySlots(InventoryComponent->GetSlots());
}

void UBRInventoryWidget::BuildInventoryWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InventoryRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBorder"));
	MainBorder->SetBrushColor(FLinearColor(0.0f, 0.12f, 0.13f, 0.94f));
	MainBorder->SetPadding(FMargin(20.0f));
	if (UCanvasPanelSlot* MainSlot = RootCanvas->AddChildToCanvas(MainBorder))
	{
		MainSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		MainSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MainSlot->SetSize(FVector2D(940.0f, 540.0f));
		MainSlot->SetPosition(FVector2D::ZeroVector);
	}

	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainBox"));
	MainBorder->SetContent(MainBox);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("INVENTORY")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 1.0f, 1.0f, 1.0f)));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 34));
	if (UVerticalBoxSlot* TitleSlot = MainBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UHorizontalBox* BodyBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BodyBox"));
	if (UVerticalBoxSlot* BodySlot = MainBox->AddChildToVerticalBox(BodyBox))
	{
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UUniformGridPanel* SlotGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("SlotGrid"));
	SlotGrid->SetSlotPadding(FMargin(6.0f));
	if (UHorizontalBoxSlot* GridSlot = BodyBox->AddChildToHorizontalBox(SlotGrid))
	{
		GridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UBorder* DetailBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailBorder"));
	DetailBorder->SetBrushColor(FLinearColor(0.02f, 0.17f, 0.18f, 0.75f));
	DetailBorder->SetPadding(FMargin(14.0f));
	if (UHorizontalBoxSlot* DetailSlot = BodyBox->AddChildToHorizontalBox(DetailBorder))
	{
		DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		DetailSlot->SetPadding(FMargin(22.0f, 0.0f, 0.0f, 0.0f));
	}

	USizeBox* DetailSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DetailSizeBox"));
	DetailSizeBox->SetWidthOverride(310.0f);
	DetailBorder->SetContent(DetailSizeBox);

	UVerticalBox* DetailBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailBox"));
	DetailSizeBox->AddChild(DetailBox);

	UTextBlock* LogText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LogText"));
	LogText->SetText(FText::FromString(TEXT("DATA_LOG")));
	LogText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 1.0f, 1.0f, 1.0f)));
	LogText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));
	DetailBox->AddChildToVerticalBox(LogText);

	ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));
	ItemNameText->SetText(FText::FromString(TEXT("ITEM: EMPTY")));
	ItemNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ItemNameText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 24));
	if (UVerticalBoxSlot* NameSlot = DetailBox->AddChildToVerticalBox(ItemNameText))
	{
		NameSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 14.0f));
	}

	ItemTypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemTypeText"));
	ItemTypeText->SetText(FText::FromString(TEXT("TYPE: -")));
	ItemTypeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ItemTypeText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
	DetailBox->AddChildToVerticalBox(ItemTypeText);

	ItemEffectText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemEffectText"));
	ItemEffectText->SetText(FText::FromString(TEXT("EFFECT:\n-")));
	ItemEffectText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ItemEffectText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* EffectSlot = DetailBox->AddChildToVerticalBox(ItemEffectText))
	{
		EffectSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 12.0f));
	}

	ItemDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemDescriptionText"));
	ItemDescriptionText->SetText(FText::FromString(TEXT("DESCRIPTION:\nNo item selected.")));
	ItemDescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ItemDescriptionText->SetAutoWrapText(true);
	DetailBox->AddChildToVerticalBox(ItemDescriptionText);

	UTextBlock* HotbarLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HotbarLabel"));
	HotbarLabel->SetText(FText::FromString(TEXT("Hotbar")));
	HotbarLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 1.0f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* HotbarLabelSlot = DetailBox->AddChildToVerticalBox(HotbarLabel))
	{
		HotbarLabelSlot->SetPadding(FMargin(0.0f, 28.0f, 0.0f, 4.0f));
		HotbarLabelSlot->SetHorizontalAlignment(HAlign_Right);
	}

	UUniformGridPanel* HotbarGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("HotbarGrid"));
	HotbarGrid->SetSlotPadding(FMargin(6.0f));
	DetailBox->AddChildToVerticalBox(HotbarGrid);

	BuildInventorySlots(SlotGrid, HotbarGrid);
}

void UBRInventoryWidget::BuildInventorySlots(UUniformGridPanel* SlotGrid, UUniformGridPanel* HotbarGrid)
{
	if (!SlotGrid || !HotbarGrid || SlotWidgets.Num() > 0)
	{
		return;
	}

	SlotWidgets.SetNum(GeneralSlotCount + HotbarSlotCount);
	for (int32 SlotIndex = 0; SlotIndex < GeneralSlotCount; ++SlotIndex)
	{
		UBRInventorySlotWidget* SlotWidget = CreateWidget<UBRInventorySlotWidget>(GetOwningPlayer(), UBRInventorySlotWidget::StaticClass());
		SlotWidgets[SlotIndex] = SlotWidget;
		if (UUniformGridSlot* GridSlot = SlotGrid->AddChildToUniformGrid(SlotWidget, SlotIndex / 5, SlotIndex % 5))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	for (int32 HotbarIndex = 0; HotbarIndex < HotbarSlotCount; ++HotbarIndex)
	{
		const int32 SlotIndex = HotbarSlotStartIndex + HotbarIndex;
		UBRInventorySlotWidget* SlotWidget = CreateWidget<UBRInventorySlotWidget>(GetOwningPlayer(), UBRInventorySlotWidget::StaticClass());
		SlotWidgets[SlotIndex] = SlotWidget;
		if (UUniformGridSlot* GridSlot = HotbarGrid->AddChildToUniformGrid(SlotWidget, 0, HotbarIndex))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UBRInventoryWidget::UpdateDetailsPanel(const FBRInventorySlot& InventorySlot)
{
	if (!ItemNameText || !ItemTypeText || !ItemEffectText || !ItemDescriptionText || InventorySlot.IsEmpty())
	{
		return;
	}

	const FText ItemName = InventorySlot.Item.DisplayName.IsEmpty() ? FText::FromName(InventorySlot.Item.ItemId) : InventorySlot.Item.DisplayName;
	ItemNameText->SetText(FText::FromString(FString::Printf(TEXT("ITEM: %s"), *ItemName.ToString())));
	ItemTypeText->SetText(InventorySlot.Item.bUsable ? FText::FromString(TEXT("TYPE: CONSUMABLE")) : FText::FromString(TEXT("TYPE: KEY ITEM")));
	ItemEffectText->SetText(InventorySlot.Item.bUsable
		? FText::FromString(TEXT("EFFECT:\nExecutes item use logic."))
		: FText::FromString(TEXT("EFFECT:\nPassive inventory item.")));
	ItemDescriptionText->SetText(FText::FromString(FString::Printf(TEXT("DESCRIPTION:\n%s"), *InventorySlot.Item.Description.ToString())));
}
