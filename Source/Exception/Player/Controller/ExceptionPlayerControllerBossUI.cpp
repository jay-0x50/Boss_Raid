#include "Player/Controller/ExceptionPlayerController.h"

#include "BRBossStatusWidget.h"

UBRBossStatusWidget* AExceptionPlayerController::ShowBossStatusWidget()
{
	if (!IsLocalPlayerController())
	{
		return nullptr;
	}

	if (!BossStatusWidgetClass)
	{
		BossStatusWidgetClass = UBRBossStatusWidget::StaticClass();
	}

	if (!BossStatusWidget && BossStatusWidgetClass)
	{
		BossStatusWidget = CreateWidget<UBRBossStatusWidget>(this, BossStatusWidgetClass);
	}

	if (BossStatusWidget && !BossStatusWidget->IsInViewport())
	{
		BossStatusWidget->AddToPlayerScreen(10);
	}

	return BossStatusWidget;
}

void AExceptionPlayerController::HideBossStatusWidget()
{
	if (BossStatusWidget)
	{
		BossStatusWidget->RemoveFromParent();
		BossStatusWidget->ClearBosses();
	}
}
