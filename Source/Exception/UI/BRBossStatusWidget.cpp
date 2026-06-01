#include "BRBossStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UBRBossStatusWidget::ClearBosses()
{
	BP_ClearBosses();
}

void UBRBossStatusWidget::SetBossCount(int32 BossCount)
{
	SetNamedVisibility(TEXT("BossSlot"), 0, BossCount >= 1);
	SetNamedVisibility(TEXT("BossSlot"), 1, BossCount >= 2);
	BP_SetBossCount(BossCount);
}

void UBRBossStatusWidget::SetBossHP(int32 BossIndex, FText BossName, float CurrentHP, float MaxHP, float NormalizedHP)
{
	const float ClampedHP = FMath::Clamp(NormalizedHP, 0.0f, 1.0f);
	SetNamedText(TEXT("BossNameText"), BossIndex, BossName);
	if (UTextBlock* BossNameText = Cast<UTextBlock>(FindIndexedWidget(TEXT("BossNameText"), BossIndex)))
	{
		BossNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.92f, 0.92f, 1.0f)));
		BossNameText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		BossNameText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	}
	SetNamedProgress(TEXT("HPBar"), BossIndex, ClampedHP);
	if (UProgressBar* HPBar = Cast<UProgressBar>(FindIndexedWidget(TEXT("HPBar"), BossIndex)))
	{
		FProgressBarStyle Style = HPBar->WidgetStyle;
		Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.025f, 0.025f, 0.025f, 0.92f));
		Style.FillImage.TintColor = FSlateColor(FLinearColor(0.86f, 0.0f, 0.08f, 1.0f));
		HPBar->SetWidgetStyle(Style);
		HPBar->SetFillColorAndOpacity(FLinearColor(0.86f, 0.0f, 0.08f, 1.0f));
	}
	BP_SetBossHP(BossIndex, BossName, CurrentHP, MaxHP, ClampedHP);
}

void UBRBossStatusWidget::SetBossGroggy(int32 BossIndex, float CurrentGroggy, float MaxGroggy, float NormalizedGroggy)
{
	const float ClampedGroggy = FMath::Clamp(NormalizedGroggy, 0.0f, 1.0f);
	SetNamedProgress(TEXT("GroggyBar"), BossIndex, ClampedGroggy);
	if (UProgressBar* GroggyBar = Cast<UProgressBar>(FindIndexedWidget(TEXT("GroggyBar"), BossIndex)))
	{
		FProgressBarStyle Style = GroggyBar->WidgetStyle;
		Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.015f, 0.018f, 0.02f, 0.9f));
		Style.FillImage.TintColor = FSlateColor(FLinearColor(0.68f, 0.9f, 1.0f, 1.0f));
		GroggyBar->SetWidgetStyle(Style);
		GroggyBar->SetFillColorAndOpacity(FLinearColor(0.68f, 0.9f, 1.0f, 1.0f));
	}
	BP_SetBossGroggy(BossIndex, CurrentGroggy, MaxGroggy, ClampedGroggy);
}

void UBRBossStatusWidget::SetBossGroggyState(int32 BossIndex, bool bIsGroggy)
{
	SetNamedVisibility(TEXT("GroggyReadyText"), BossIndex, bIsGroggy);
	BP_SetBossGroggyState(BossIndex, bIsGroggy);
}

void UBRBossStatusWidget::SetBossExecutionState(int32 BossIndex, bool bCanBeExecuted)
{
	SetNamedVisibility(TEXT("FinishPrompt"), BossIndex, bCanBeExecuted);
	SetNamedVisibility(TEXT("ExecutePrompt"), BossIndex, bCanBeExecuted);
	if (UTextBlock* FinishPrompt = Cast<UTextBlock>(FindIndexedWidget(TEXT("FinishPrompt"), BossIndex)))
	{
		FinishPrompt->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.16f, 0.25f, 1.0f)));
		FinishPrompt->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		FinishPrompt->SetShadowOffset(FVector2D(1.5f, 1.5f));
	}
	BP_SetBossExecutionState(BossIndex, bCanBeExecuted);
}

void UBRBossStatusWidget::SetNamedText(FName BaseName, int32 BossIndex, const FText& Text)
{
	if (UTextBlock* TextBlock = Cast<UTextBlock>(FindIndexedWidget(BaseName, BossIndex)))
	{
		TextBlock->SetText(Text);
	}
}

void UBRBossStatusWidget::SetNamedProgress(FName BaseName, int32 BossIndex, float Percent)
{
	if (UProgressBar* ProgressBar = Cast<UProgressBar>(FindIndexedWidget(BaseName, BossIndex)))
	{
		ProgressBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UBRBossStatusWidget::SetNamedVisibility(FName BaseName, int32 BossIndex, bool bVisible)
{
	if (UWidget* Widget = FindIndexedWidget(BaseName, BossIndex))
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

UWidget* UBRBossStatusWidget::FindIndexedWidget(FName BaseName, int32 BossIndex) const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	if (BossIndex == 0)
	{
		if (UWidget* Widget = WidgetTree->FindWidget(BaseName))
		{
			return Widget;
		}
	}

	const FString BaseNameString = BaseName.ToString();
	const FString IndexedName = FString::Printf(TEXT("%s_%d"), *BaseNameString, BossIndex);
	if (UWidget* Widget = WidgetTree->FindWidget(FName(*IndexedName)))
	{
		return Widget;
	}

	const FString OneBasedIndexedName = FString::Printf(TEXT("%s_%d"), *BaseNameString, BossIndex + 1);
	return WidgetTree->FindWidget(FName(*OneBasedIndexedName));
}
