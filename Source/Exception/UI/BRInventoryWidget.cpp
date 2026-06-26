#include "BRInventoryWidget.h"

#include "BRInventoryComponent.h"
#include "BRInventorySlotWidget.h"
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

void UBRInventoryWidget::HandleCloseClicked()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(GetOwningPlayer()))
	{
		ExceptionPC->HideInventoryWidget();
	}
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

	UHorizontalBox* HeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderBox"));
	if (UVerticalBoxSlot* HeaderSlot = MainBox->AddChildToVerticalBox(HeaderBox))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("INVENTORY")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 1.0f, 1.0f, 1.0f)));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 34));
	if (UHorizontalBoxSlot* TitleSlot = HeaderBox->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButton->SetBackgroundColor(FLinearColor(0.22f, 0.2f, 0.16f, 0.9f));
	CloseButton->OnClicked.AddUniqueDynamic(this, &UBRInventoryWidget::HandleCloseClicked);
	if (UHorizontalBoxSlot* CloseSlot = HeaderBox->AddChildToHorizontalBox(CloseButton))
	{
		CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		CloseSlot->SetHorizontalAlignment(HAlign_Right);
		CloseSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
	CloseText->SetText(FText::FromString(TEXT("Close")));
	CloseText->SetJustification(ETextJustify::Center);
	CloseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.86f, 0.76f, 1.0f)));
	CloseText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
	CloseButton->AddChild(CloseText);

	UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryHintText"));
	HintText->SetText(FText::FromString(TEXT("Click item slots to use consumables. Hotbar slots: Q / E / R")));
	HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 1.0f, 1.0f, 1.0f)));
	HintText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12));
	if (UVerticalBoxSlot* HintSlot = MainBox->AddChildToVerticalBox(HintText))
	{
		HintSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
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

	FString TypeText = InventorySlot.Item.bUsable ? TEXT("TYPE: USABLE") : TEXT("TYPE: KEY ITEM");
	FString EffectText = TEXT("EFFECT:\nPassive inventory item.");

	switch (InventorySlot.Item.Effect)
	{
	case EBRInventoryItemEffect::HealHP:
		TypeText = TEXT("TYPE: CONSUMABLE");
		EffectText = FString::Printf(TEXT("EFFECT:\nRestores %.0f HP."), InventorySlot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::RestoreStamina:
		TypeText = TEXT("TYPE: CONSUMABLE");
		EffectText = FString::Printf(TEXT("EFFECT:\nRestores %.0f stamina."), InventorySlot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::RestoreAll:
		TypeText = TEXT("TYPE: CONSUMABLE");
		EffectText = FString::Printf(TEXT("EFFECT:\nRestores %.0f HP and stamina."), InventorySlot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::GrantUpgradePoint:
		TypeText = TEXT("TYPE: RUNE MEMORY");
		EffectText = FString::Printf(TEXT("EFFECT:\nGrants %.0f upgrade point."), InventorySlot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::HiddenRootWeapon:
		TypeText = TEXT("TYPE: HIDDEN WEAPON");
		EffectText = FString::Printf(TEXT("EFFECT:\nAuthority damage against CMD x%.1f."), InventorySlot.Item.EffectValue);
		break;
	default:
		break;
	}

	ItemTypeText->SetText(FText::FromString(TypeText));
	ItemEffectText->SetText(FText::FromString(EffectText));
	ItemDescriptionText->SetText(FText::FromString(FString::Printf(TEXT("DESCRIPTION:\n%s"), *InventorySlot.Item.Description.ToString())));
}
