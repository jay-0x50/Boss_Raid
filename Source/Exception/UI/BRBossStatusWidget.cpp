#include "BRBossStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UBRBossStatusWidget::ClearBosses()
{
	VisibleBossCount = 0;
	BossHPPercents.Reset();
	BossIdentityColors.Reset();
	BP_ClearBosses();
}

void UBRBossStatusWidget::SetBossCount(int32 BossCount)
{
	VisibleBossCount = FMath::Clamp(BossCount, 0, 8);
	BossHPPercents.Init(-1.0f, VisibleBossCount);
	BossIdentityColors.SetNum(VisibleBossCount);
	for (int32 BossIndex = 0; BossIndex < VisibleBossCount; ++BossIndex)
	{
		BossIdentityColors[BossIndex] = GetBossIdentityColor(BossIndex);
	}

	for (int32 BossIndex = 0; BossIndex < 8; ++BossIndex)
	{
		const bool bVisible = BossIndex < BossCount;
		SetNamedVisibility(TEXT("BossSlot"), BossIndex, bVisible);

		if (!bVisible)
		{
			SetNamedText(TEXT("BossNameText"), BossIndex, FText::GetEmpty());
			SetNamedProgress(TEXT("HPBar"), BossIndex, 0.0f);
			SetNamedProgress(TEXT("GroggyBar"), BossIndex, 0.0f);
			SetNamedVisibility(TEXT("GroggyReadyText"), BossIndex, false);
			SetNamedVisibility(TEXT("FinishPrompt"), BossIndex, false);
			SetNamedVisibility(TEXT("ExecutePrompt"), BossIndex, false);
		}
	}

	BP_SetBossCount(BossCount);
}

void UBRBossStatusWidget::SetBossHP(int32 BossIndex, FText BossName, float CurrentHP, float MaxHP, float NormalizedHP)
{
	const float ClampedHP = FMath::Clamp(NormalizedHP, 0.0f, 1.0f);
	if (BossHPPercents.IsValidIndex(BossIndex))
	{
		BossHPPercents[BossIndex] = ClampedHP;
	}
	const FLinearColor IdentityColor = GetBossIdentityColor(BossIndex, &BossName);
	if (BossIdentityColors.IsValidIndex(BossIndex))
	{
		BossIdentityColors[BossIndex] = IdentityColor;
	}
	SetNamedText(TEXT("BossNameText"), BossIndex, BossName);
	if (UTextBlock* BossNameText = Cast<UTextBlock>(FindIndexedWidget(TEXT("BossNameText"), BossIndex)))
	{
		BossNameText->SetColorAndOpacity(FSlateColor(IdentityColor));
		BossNameText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		BossNameText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	}
	SetNamedProgress(TEXT("HPBar"), BossIndex, ClampedHP);
	if (UProgressBar* HPBar = Cast<UProgressBar>(FindIndexedWidget(TEXT("HPBar"), BossIndex)))
	{
		FProgressBarStyle Style = HPBar->GetWidgetStyle();
		Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.025f, 0.025f, 0.025f, 0.92f));
		const FLinearColor BarColor = VisibleBossCount == 2
			? IdentityColor
			: FLinearColor(0.86f, 0.0f, 0.08f, 1.0f);
		Style.FillImage.TintColor = FSlateColor(BarColor);
		HPBar->SetWidgetStyle(Style);
		HPBar->SetFillColorAndOpacity(BarColor);
	}
	BP_SetBossHP(BossIndex, BossName, CurrentHP, MaxHP, ClampedHP);
	RefreshTeamBalanceStyle();
}

void UBRBossStatusWidget::SetBossGroggy(int32 BossIndex, float CurrentGroggy, float MaxGroggy, float NormalizedGroggy)
{
	const float ClampedGroggy = FMath::Clamp(NormalizedGroggy, 0.0f, 1.0f);
	SetNamedProgress(TEXT("GroggyBar"), BossIndex, ClampedGroggy);
	if (UProgressBar* GroggyBar = Cast<UProgressBar>(FindIndexedWidget(TEXT("GroggyBar"), BossIndex)))
	{
		FProgressBarStyle Style = GroggyBar->GetWidgetStyle();
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

FLinearColor UBRBossStatusWidget::GetBossIdentityColor(int32 BossIndex, const FText* BossName) const
{
	if (VisibleBossCount == 2)
	{
		if (BossName)
		{
			const FString Name = BossName->ToString();
			if (Name.Contains(TEXT("Vethara"), ESearchCase::IgnoreCase))
			{
				return FLinearColor(0.16f, 0.74f, 1.0f, 1.0f);
			}
			if (Name.Contains(TEXT("Aurathos"), ESearchCase::IgnoreCase))
			{
				return FLinearColor(1.0f, 0.68f, 0.16f, 1.0f);
			}
		}
		return BossIndex == 0
			? FLinearColor(0.16f, 0.74f, 1.0f, 1.0f)
			: FLinearColor(1.0f, 0.68f, 0.16f, 1.0f);
	}
	return FLinearColor(0.92f, 0.92f, 0.92f, 1.0f);
}

void UBRBossStatusWidget::RefreshTeamBalanceStyle()
{
	if (VisibleBossCount < 2
		|| BossHPPercents.Num() < VisibleBossCount
		|| BossIdentityColors.Num() < VisibleBossCount
		|| BossHPPercents.ContainsByPredicate([](float HP) { return HP < 0.0f; }))
	{
		return;
	}

	int32 HighestIndex = 0;
	float HighestHP = BossHPPercents[0];
	float LowestHP = BossHPPercents[0];
	for (int32 BossIndex = 1; BossIndex < VisibleBossCount; ++BossIndex)
	{
		if (BossHPPercents[BossIndex] > HighestHP)
		{
			HighestHP = BossHPPercents[BossIndex];
			HighestIndex = BossIndex;
		}
		LowestHP = FMath::Min(LowestHP, BossHPPercents[BossIndex]);
	}

	const bool bTeamGapWarning = HighestHP - LowestHP >= 0.30f;
	for (int32 BossIndex = 0; BossIndex < VisibleBossCount; ++BossIndex)
	{
		if (UTextBlock* BossNameText = Cast<UTextBlock>(FindIndexedWidget(TEXT("BossNameText"), BossIndex)))
		{
			const FLinearColor NameColor = bTeamGapWarning && BossIndex == HighestIndex
				? FLinearColor(1.0f, 0.12f, 0.04f, 1.0f)
				: BossIdentityColors[BossIndex];
			BossNameText->SetColorAndOpacity(FSlateColor(NameColor));
		}
	}
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
