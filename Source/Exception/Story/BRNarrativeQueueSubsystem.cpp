#include "BRNarrativeQueueSubsystem.h"

#include "UI/BRLoreLogWidget.h"
#include "UI/BRNelDialogueWidget.h"
#include "UI/BREndingWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void UBRNarrativeQueueSubsystem::Deinitialize()
{
	ClearMessages();
	Super::Deinitialize();
}

void UBRNarrativeQueueSubsystem::AddMessage(const FBRNarrativeMessage& Message)
{
	if (Message.Text.IsEmpty())
	{
		return;
	}

	MessageQueue.Add(Message);
	TryShowNext();
}

void UBRNarrativeQueueSubsystem::ShowSystemLog(const FText& Text, float ShowTime, const FText& Title)
{
	FBRNarrativeMessage Message;
	Message.Type = EBRNarrativeType::SystemLog;
	Message.Title = Title.IsEmpty() ? FText::FromString(TEXT("RUNTIME // SYSTEM LOG")) : Title;
	Message.Text = Text;
	Message.ShowTime = ShowTime;
	AddMessage(Message);
}

void UBRNarrativeQueueSubsystem::ShowNelLine(const FText& Text, bool bHiddenHint, float ShowTime)
{
	FBRNarrativeMessage Message;
	Message.Type = EBRNarrativeType::Nel;
	Message.Title = bHiddenHint
		? FText::FromString(TEXT("NEL // HIDDEN TRACE"))
		: FText::FromString(TEXT("NEL // null"));
	Message.Text = Text;
	Message.ShowTime = ShowTime;
	Message.bHiddenHint = bHiddenHint;
	AddMessage(Message);
}

void UBRNarrativeQueueSubsystem::ShowBossLine(const FText& BossTitle, const FText& Text, float ShowTime)
{
	FBRNarrativeMessage Message;
	Message.Type = EBRNarrativeType::Boss;
	Message.Title = BossTitle;
	Message.Text = Text;
	Message.ShowTime = ShowTime;
	AddMessage(Message);
}

void UBRNarrativeQueueSubsystem::ShowEnding(const FText& Title, const FText& Text, bool bHiddenEnding, float ShowTime)
{
	FBRNarrativeMessage Message;
	Message.Type = EBRNarrativeType::Ending;
	Message.Title = Title;
	Message.Text = Text;
	Message.ShowTime = ShowTime;
	Message.bHiddenHint = bHiddenEnding;
	AddMessage(Message);
}

void UBRNarrativeQueueSubsystem::ClearMessages()
{
	MessageQueue.Reset();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWorld* World = GI->GetWorld())
		{
			World->GetTimerManager().ClearTimer(MessageTimer);
		}
	}

	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}
}

void UBRNarrativeQueueSubsystem::TryShowNext()
{
	if (ActiveWidget || MessageQueue.IsEmpty())
	{
		return;
	}

	APlayerController* PC = GetLocalPC();
	UGameInstance* GI = GetGameInstance();
	UWorld* World = GI ? GI->GetWorld() : nullptr;
	if (!PC || !World)
	{
		return;
	}

	const FBRNarrativeMessage Message = MessageQueue[0];
	MessageQueue.RemoveAt(0);

	if (Message.Type == EBRNarrativeType::Nel)
	{
		UBRNelDialogueWidget* NelWidget = CreateWidget<UBRNelDialogueWidget>(PC, UBRNelDialogueWidget::StaticClass());
		if (NelWidget)
		{
			NelWidget->SetDialogue(Message.Title, Message.Text, Message.bHiddenHint);
			ActiveWidget = NelWidget;
		}
	}
	else if (Message.Type == EBRNarrativeType::Ending)
	{
		UBREndingWidget* EndingWidget = CreateWidget<UBREndingWidget>(PC, UBREndingWidget::StaticClass());
		if (EndingWidget)
		{
			EndingWidget->SetEnding(Message.Title, Message.Text, Message.bHiddenHint, Message.ShowTime);
			ActiveWidget = EndingWidget;
		}
	}
	else
	{
		UBRLoreLogWidget* LogWidget = CreateWidget<UBRLoreLogWidget>(PC, UBRLoreLogWidget::StaticClass());
		if (LogWidget)
		{
			LogWidget->SetLog(Message.Title, Message.Text, Message.Type == EBRNarrativeType::Boss);
			ActiveWidget = LogWidget;
		}
	}

	if (!ActiveWidget)
	{
		TryShowNext();
		return;
	}

	ActiveWidget->AddToPlayerScreen(Message.Type == EBRNarrativeType::Ending ? 90 : 30);
	World->GetTimerManager().SetTimer(
		MessageTimer,
		this,
		&UBRNarrativeQueueSubsystem::FinishMessage,
		FMath::Max(0.5f, Message.ShowTime),
		false);
}

void UBRNarrativeQueueSubsystem::FinishMessage()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}

	TryShowNext();
}

APlayerController* UBRNarrativeQueueSubsystem::GetLocalPC() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetFirstLocalPlayerController() : nullptr;
}
