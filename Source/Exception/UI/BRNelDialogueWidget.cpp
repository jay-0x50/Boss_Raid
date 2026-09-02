#include "BRNelDialogueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UBRNelDialogueWidget::SetDialogue(const FText& InTitle, const FText& InText, bool bInHiddenHint)
{
	SavedTitle = InTitle;
	FullText = InText.ToString();
	bHiddenHint = bInHiddenHint;
	TypeTime = 0.0f;
	ShownCount = 0;
	RefreshText();
}

void UBRNelDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	RefreshText();
}

void UBRNelDialogueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ShownCount >= FullText.Len())
	{
		return;
	}

	TypeTime += InDeltaTime;
	const int32 NewCount = FMath::Min(FullText.Len(), FMath::FloorToInt(TypeTime / TypeSpeed));
	if (NewCount != ShownCount)
	{
		ShownCount = NewCount;
		RefreshText();
	}
}

void UBRNelDialogueWidget::BuildWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NelRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NelPanel"));
	Panel->SetBrushColor(FLinearColor(0.018f, 0.028f, 0.035f, 0.92f));
	Panel->SetPadding(FMargin(24.0f, 18.0f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		PanelSlot->SetPosition(FVector2D(-55.0f, -64.0f));
		PanelSlot->SetSize(FVector2D(650.0f, 178.0f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NelBox"));
	Panel->SetContent(Box);

	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NelName"));
	NameText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
	NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.18f, 0.82f, 0.92f, 1.0f)));
	if (UVerticalBoxSlot* NameSlot = Box->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	LineText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NelLine"));
	LineText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18));
	LineText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.88f, 0.80f, 1.0f)));
	LineText->SetAutoWrapText(true);
	LineText->SetWrapTextAt(585.0f);
	Box->AddChildToVerticalBox(LineText);

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBRNelDialogueWidget::RefreshText()
{
	if (NameText)
	{
		NameText->SetText(SavedTitle);
		NameText->SetColorAndOpacity(FSlateColor(
			bHiddenHint ? FLinearColor(0.76f, 0.18f, 0.30f, 1.0f) : FLinearColor(0.18f, 0.82f, 0.92f, 1.0f)));
	}

	if (LineText)
	{
		LineText->SetText(FText::FromString(FullText.Left(ShownCount)));
	}
}
