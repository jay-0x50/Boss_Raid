#include "BRLoreLogWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UBRLoreLogWidget::SetLog(const FText& InTitle, const FText& InText, bool bBossMessage)
{
	SavedTitle = InTitle;
	FullText = InText.ToString();
	bBossStyle = bBossMessage;
	TypeTime = 0.0f;
	ShownCount = 0;
	RefreshText();
}

void UBRLoreLogWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	RefreshText();
}

void UBRLoreLogWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

void UBRLoreLogWidget::BuildWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LogRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LogPanel"));
	Panel->SetBrushColor(FLinearColor(0.012f, 0.018f, 0.025f, 0.92f));
	Panel->SetPadding(FMargin(22.0f, 16.0f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(-52.0f, 52.0f));
		PanelSlot->SetSize(FVector2D(590.0f, 164.0f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LogBox"));
	Panel->SetContent(Box);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LogTitle"));
	TitleText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.76f, 0.05f, 0.10f, 1.0f)));
	if (UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LogBody"));
	BodyText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 17));
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.86f, 0.76f, 1.0f)));
	BodyText->SetAutoWrapText(true);
	BodyText->SetWrapTextAt(530.0f);
	Box->AddChildToVerticalBox(BodyText);

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBRLoreLogWidget::RefreshText()
{
	if (TitleText)
	{
		TitleText->SetText(SavedTitle);
		TitleText->SetColorAndOpacity(FSlateColor(
			bBossStyle ? FLinearColor(0.88f, 0.48f, 0.12f, 1.0f) : FLinearColor(0.76f, 0.05f, 0.10f, 1.0f)));
	}

	if (BodyText)
	{
		BodyText->SetText(FText::FromString(FullText.Left(ShownCount)));
	}
}
