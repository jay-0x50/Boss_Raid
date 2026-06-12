#include "BRInventorySlotWidget.h"

#include "Player/Controller/ExceptionPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

void UBRInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildSlotWidget();
	RefreshVisuals();
}

void UBRInventorySlotWidget::SetSlotData(int32 NewSlotIndex, const FBRInventorySlot& NewInventorySlot)
{
	SlotIndex = NewSlotIndex;
	CurrentInventorySlot = NewInventorySlot;
	RefreshVisuals();
}

void UBRInventorySlotWidget::HandleClicked()
{
	if (AExceptionPlayerController* ExceptionPC = Cast<AExceptionPlayerController>(GetOwningPlayer()))
	{
		ExceptionPC->UseInventorySlot(SlotIndex);
	}
}

void UBRInventorySlotWidget::BuildSlotWidget()
{
	if (!WidgetTree || SlotButton)
	{
		return;
	}

	USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotSizeBox"));
	RootSizeBox->SetWidthOverride(76.0f);
	RootSizeBox->SetHeightOverride(82.0f);
	WidgetTree->RootWidget = RootSizeBox;

	SlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SlotButton"));
	SlotButton->SetBackgroundColor(FLinearColor(0.0f, 0.85f, 0.9f, 0.22f));
	SlotButton->OnClicked.AddUniqueDynamic(this, &UBRInventorySlotWidget::HandleClicked);
	RootSizeBox->AddChild(SlotButton);

	FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FrameBorder"));
	FrameBorder->SetBrushColor(FLinearColor(0.0f, 0.95f, 1.0f, 0.55f));
	FrameBorder->SetPadding(FMargin(4.0f));
	SlotButton->AddChild(FrameBorder);

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	FrameBorder->SetContent(ContentBox);

	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
	if (UVerticalBoxSlot* IconSlot = ContentBox->AddChildToVerticalBox(IconImage))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 3.0f));
	}

	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
	NameText->SetJustification(ETextJustify::Center);
	NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 1.0f, 1.0f, 1.0f)));
	NameText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 9));
	if (UVerticalBoxSlot* NameSlot = ContentBox->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	QuantityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuantityText"));
	QuantityText->SetJustification(ETextJustify::Center);
	QuantityText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 1.0f, 1.0f, 1.0f)));
	QuantityText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 9));
	if (UVerticalBoxSlot* QuantitySlot = ContentBox->AddChildToVerticalBox(QuantityText))
	{
		QuantitySlot->SetHorizontalAlignment(HAlign_Fill);
	}
}

void UBRInventorySlotWidget::RefreshVisuals()
{
	if (!FrameBorder || !IconImage || !NameText || !QuantityText)
	{
		return;
	}

	if (CurrentInventorySlot.IsEmpty())
	{
		FrameBorder->SetBrushColor(FLinearColor(0.0f, 0.35f, 0.38f, 0.35f));
		IconImage->SetVisibility(ESlateVisibility::Hidden);
		NameText->SetText(FText::GetEmpty());
		QuantityText->SetText(FText::GetEmpty());
		return;
	}

	FrameBorder->SetBrushColor(FLinearColor(0.0f, 0.95f, 1.0f, 0.65f));
	IconImage->SetVisibility(ESlateVisibility::Visible);
	NameText->SetText(CurrentInventorySlot.Item.DisplayName.IsEmpty() ? FText::FromName(CurrentInventorySlot.Item.ItemId) : CurrentInventorySlot.Item.DisplayName);
	QuantityText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), CurrentInventorySlot.Quantity)));

	if (!CurrentInventorySlot.Item.Icon.IsNull())
	{
		if (UTexture2D* LoadedIcon = CurrentInventorySlot.Item.Icon.LoadSynchronous())
		{
			IconImage->SetBrushFromTexture(LoadedIcon, true);
			return;
		}
	}

	FSlateBrush EmptyBrush;
	EmptyBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.18f, 0.18f, 1.0f));
	EmptyBrush.ImageSize = FVector2D(42.0f, 42.0f);
	IconImage->SetBrush(EmptyBrush);
}
