#include "UI/BREndingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UBREndingWidget::SetEnding(const FText& InTitle, const FText& InText, bool bInHiddenEnding, float InShowTime)
{
	SavedTitle = InTitle;
	SavedText = InText;
	bHiddenEnding = bInHiddenEnding;
	ShowTime = FMath::Max(3.0f, InShowTime);
	LifeTime = 0.0f;
	RefreshEnding();
}

void UBREndingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidget();
	RefreshEnding();
}

void UBREndingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	LifeTime += InDeltaTime;
	const float FadeIn = FMath::SmoothStep(0.0f, 1.35f, LifeTime);
	const float FadeOut = 1.0f - FMath::SmoothStep(ShowTime - 1.25f, ShowTime, LifeTime);
	SetRenderOpacity(FMath::Min(FadeIn, FadeOut));
}

void UBREndingWidget::BuildWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EndingRoot"));
	WidgetTree->RootWidget = Root;

	Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EndingBackdrop"));
	Backdrop->SetPadding(FMargin(80.0f));
	if (UCanvasPanelSlot* BackSlot = Root->AddChildToCanvas(Backdrop))
	{
		BackSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackSlot->SetOffsets(FMargin(0.0f));
	}

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EndingContent"));
	Backdrop->SetContent(Content);

	ChapterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EndingChapter"));
	ChapterText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 17));
	ChapterText->SetJustification(ETextJustify::Center);
	ChapterText->SetText(FText::FromString(TEXT("EXCEPTION // THREE SEALED PROCESSES")));
	if (UVerticalBoxSlot* ChapterSlot = Content->AddChildToVerticalBox(ChapterText))
	{
		ChapterSlot->SetHorizontalAlignment(HAlign_Fill);
		ChapterSlot->SetPadding(FMargin(0.0f, 70.0f, 0.0f, 26.0f));
	}

	EndingTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EndingTitle"));
	EndingTitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 52));
	EndingTitle->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(EndingTitle))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 38.0f));
	}

	EndingBody = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EndingBody"));
	EndingBody->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 23));
	EndingBody->SetJustification(ETextJustify::Center);
	EndingBody->SetAutoWrapText(true);
	EndingBody->SetWrapTextAt(980.0f);
	if (UVerticalBoxSlot* BodySlot = Content->AddChildToVerticalBox(EndingBody))
	{
		BodySlot->SetHorizontalAlignment(HAlign_Fill);
		BodySlot->SetPadding(FMargin(100.0f, 0.0f, 100.0f, 44.0f));
	}

	FooterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EndingFooter"));
	FooterText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));
	FooterText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* FooterSlot = Content->AddChildToVerticalBox(FooterText))
	{
		FooterSlot->SetHorizontalAlignment(HAlign_Fill);
		FooterSlot->SetVerticalAlignment(VAlign_Bottom);
		FooterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		FooterSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 52.0f));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBREndingWidget::RefreshEnding()
{
	const FLinearColor Accent = bHiddenEnding
		? FLinearColor(0.12f, 0.78f, 1.0f, 1.0f)
		: FLinearColor(0.82f, 0.20f, 0.28f, 1.0f);
	if (Backdrop)
	{
		Backdrop->SetBrushColor(bHiddenEnding
			? FLinearColor(0.006f, 0.018f, 0.032f, 0.97f)
			: FLinearColor(0.012f, 0.012f, 0.018f, 0.97f));
	}
	if (ChapterText)
	{
		ChapterText->SetColorAndOpacity(FSlateColor(Accent * 0.72f));
	}
	if (EndingTitle)
	{
		EndingTitle->SetText(SavedTitle);
		EndingTitle->SetColorAndOpacity(FSlateColor(Accent));
	}
	if (EndingBody)
	{
		EndingBody->SetText(SavedText);
		EndingBody->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.90f, 0.94f, 1.0f)));
	}
	if (FooterText)
	{
		FooterText->SetText(FText::FromString(bHiddenEnding
			? TEXT("PYTHON [SEALED]     PERL [SEALED]     CMD [AUTHORITY SEIZED]     NEXT: FALLBACK_HANDLER.exe")
			: TEXT("PYTHON [SEALED]     PERL [SEALED]     CMD [TERMINATED]     THE RUNTIME CONTINUES")));
		FooterText->SetColorAndOpacity(FSlateColor(Accent * 0.82f));
	}
}
