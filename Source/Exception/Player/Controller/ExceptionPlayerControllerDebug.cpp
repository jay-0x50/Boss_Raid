#include "Player/Controller/ExceptionPlayerController.h"

#include "BRHiddenStorySubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Engine/Engine.h"

namespace
{
void ShowDebugMessage(const FString& Message, const FColor& Color = FColor::Cyan)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(6200, 4.0f, Color, Message);
	}
}
}

void AExceptionPlayerController::DebugCompleteNelHiddenRoute()
{
	if (!bEnableDemoDebugHotkeys)
	{
		return;
	}

	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(GetPawn());
	if (!PlayerCharacter)
	{
		ShowDebugMessage(TEXT("Debug hidden route failed: no player character."), FColor::Red);
		return;
	}

	PlayerCharacter->CompleteNelHiddenRequest(TEXT("Nel_FindPythonTrace"));
	PlayerCharacter->CompleteNelHiddenRequest(TEXT("Nel_DecodePerlSigil"));
	PlayerCharacter->CompleteNelHiddenRequest(TEXT("Nel_RecoverRuntimeShard"));
	PlayerCharacter->CollectHiddenFragment(3);
	PlayerCharacter->RefreshHiddenStoryRewards();

	ShowDebugMessage(TEXT("Debug: Nel hidden route completed. Mimikatz should be unlocked."), FColor::Purple);
}

void AExceptionPlayerController::DebugCollectHiddenFragment()
{
	if (!bEnableDemoDebugHotkeys)
	{
		return;
	}

	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(GetPawn());
	if (!PlayerCharacter)
	{
		ShowDebugMessage(TEXT("Debug fragment failed: no player character."), FColor::Red);
		return;
	}

	PlayerCharacter->CollectHiddenFragment(1);
	ShowDebugMessage(TEXT("Debug: +1 hidden fragment."), FColor::Purple);
}

void AExceptionPlayerController::DebugGrantMimikatzAuthoritySeized()
{
	if (!bEnableDemoDebugHotkeys)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			HiddenStory->SetMimikatzAuthoritySeizedUnlocked(true);
			HiddenStory->SetHiddenEndingEligible(true);
		}
	}

	if (AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(GetPawn()))
	{
		PlayerCharacter->RefreshHiddenStoryRewards();
	}

	ShowDebugMessage(TEXT("Debug: Mimikatz, Authority Seized granted."), FColor::Purple);
}

void AExceptionPlayerController::DebugPrintHiddenStoryState()
{
	if (!bEnableDemoDebugHotkeys)
	{
		return;
	}

	UBRHiddenStorySubsystem* HiddenStory = GetGameInstance() ? GetGameInstance()->GetSubsystem<UBRHiddenStorySubsystem>() : nullptr;
	if (!HiddenStory)
	{
		ShowDebugMessage(TEXT("Debug hidden state failed: subsystem missing."), FColor::Red);
		return;
	}

	const FString EndingName = StaticEnum<EBRRuntimeEnding>()
		? StaticEnum<EBRRuntimeEnding>()->GetNameStringByValue(static_cast<int64>(HiddenStory->GetLastResolvedEnding()))
		: TEXT("Unknown");

	const FString Message = FString::Printf(
		TEXT("Hidden Story\nNel: %s\nFragments: %d/%d\nMimikatz: %s\nHidden Eligible: %s\nEnding: %s"),
		HiddenStory->AreAllNelHiddenRequestsCompleted() ? TEXT("Complete") : TEXT("Incomplete"),
		HiddenStory->GetHiddenFragmentCount(),
		HiddenStory->GetRequiredHiddenFragmentCount(),
		HiddenStory->IsMimikatzAuthoritySeizedUnlocked() ? TEXT("Unlocked") : TEXT("Locked"),
		HiddenStory->IsHiddenEndingEligible() ? TEXT("Yes") : TEXT("No"),
		*EndingName);

	ShowDebugMessage(Message, FColor::Cyan);
}
