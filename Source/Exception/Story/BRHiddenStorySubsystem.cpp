#include "BRHiddenStorySubsystem.h"

void UBRHiddenStorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisterDefaultNelHiddenRequests();
}

void UBRHiddenStorySubsystem::RegisterDefaultNelHiddenRequests()
{
	RegisterNelHiddenRequest(TEXT("Nel_FindPythonTrace"));
	RegisterNelHiddenRequest(TEXT("Nel_DecodePerlSigil"));
	RegisterNelHiddenRequest(TEXT("Nel_RecoverRuntimeShard"));
}

void UBRHiddenStorySubsystem::RegisterNelHiddenRequest(FName RequestId)
{
	if (!RequestId.IsNone())
	{
		RegisteredNelRequests.Add(RequestId);
	}
}

void UBRHiddenStorySubsystem::MarkNelHiddenRequestCompleted(FName RequestId)
{
	if (RequestId.IsNone())
	{
		return;
	}

	RegisteredNelRequests.Add(RequestId);
	const bool bAlreadyCompleted = CompletedNelRequests.Contains(RequestId);
	CompletedNelRequests.Add(RequestId);

	if (!bAlreadyCompleted)
	{
		OnNelHiddenRequestCompleted.Broadcast(RequestId);
	}

	RefreshHiddenEligibility();
}

bool UBRHiddenStorySubsystem::IsNelHiddenRequestCompleted(FName RequestId) const
{
	return CompletedNelRequests.Contains(RequestId);
}

bool UBRHiddenStorySubsystem::AreAllNelHiddenRequestsCompleted() const
{
	if (RegisteredNelRequests.IsEmpty())
	{
		return false;
	}

	for (const FName& RequestId : RegisteredNelRequests)
	{
		if (!CompletedNelRequests.Contains(RequestId))
		{
			return false;
		}
	}

	return true;
}

void UBRHiddenStorySubsystem::CollectHiddenFragment(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	HiddenFragmentCount = FMath::Clamp(HiddenFragmentCount + Amount, 0, RequiredHiddenFragmentCount);
	OnHiddenFragmentChanged.Broadcast(HiddenFragmentCount, RequiredHiddenFragmentCount);
	RefreshHiddenEligibility();
}

void UBRHiddenStorySubsystem::SetMimikatzAuthoritySeizedUnlocked(bool bUnlocked)
{
	bMimikatzAuthoritySeizedUnlocked = bUnlocked;
	RefreshHiddenEligibility();
}

void UBRHiddenStorySubsystem::SetHiddenEndingEligible(bool bEligible)
{
	bHiddenEndingEligible = bEligible;
}

EBRRuntimeEnding UBRHiddenStorySubsystem::ResolveCMDEnding(bool bDefeatedWithMimikatzAuthoritySeized)
{
	RefreshHiddenEligibility();
	LastResolvedEnding = bHiddenEndingEligible && bMimikatzAuthoritySeizedUnlocked && bDefeatedWithMimikatzAuthoritySeized
		? EBRRuntimeEnding::HiddenAuthoritySeized
		: EBRRuntimeEnding::BasicCMDDefeated;
	return LastResolvedEnding;
}

TArray<FName> UBRHiddenStorySubsystem::GetRegisteredNelRequestIds() const
{
	TArray<FName> RequestIds;
	RequestIds.Reserve(RegisteredNelRequests.Num());
	for (const FName& RequestId : RegisteredNelRequests)
	{
		RequestIds.Add(RequestId);
	}
	return RequestIds;
}

TArray<FName> UBRHiddenStorySubsystem::GetCompletedNelRequestIds() const
{
	TArray<FName> RequestIds;
	RequestIds.Reserve(CompletedNelRequests.Num());
	for (const FName& RequestId : CompletedNelRequests)
	{
		RequestIds.Add(RequestId);
	}
	return RequestIds;
}

void UBRHiddenStorySubsystem::ApplySavedHiddenStoryState(const TArray<FName>& RegisteredRequests, const TArray<FName>& CompletedRequests, int32 SavedHiddenFragmentCount, bool bSavedMimikatzUnlocked, bool bSavedHiddenEndingEligible, EBRRuntimeEnding SavedLastEnding)
{
	RegisteredNelRequests.Reset();
	CompletedNelRequests.Reset();

	for (const FName& RequestId : RegisteredRequests)
	{
		if (!RequestId.IsNone())
		{
			RegisteredNelRequests.Add(RequestId);
		}
	}

	RegisterDefaultNelHiddenRequests();

	for (const FName& RequestId : CompletedRequests)
	{
		if (!RequestId.IsNone())
		{
			RegisteredNelRequests.Add(RequestId);
			CompletedNelRequests.Add(RequestId);
		}
	}

	HiddenFragmentCount = FMath::Clamp(SavedHiddenFragmentCount, 0, RequiredHiddenFragmentCount);
	bMimikatzAuthoritySeizedUnlocked = bSavedMimikatzUnlocked;
	bHiddenEndingEligible = bSavedHiddenEndingEligible;
	LastResolvedEnding = SavedLastEnding;
	RefreshHiddenEligibility();
}

void UBRHiddenStorySubsystem::ResetHiddenStoryState()
{
	RegisteredNelRequests.Reset();
	CompletedNelRequests.Reset();
	RegisterDefaultNelHiddenRequests();
	HiddenFragmentCount = 0;
	bMimikatzAuthoritySeizedUnlocked = false;
	bHiddenEndingEligible = false;
	LastResolvedEnding = EBRRuntimeEnding::None;
	OnHiddenFragmentChanged.Broadcast(HiddenFragmentCount, RequiredHiddenFragmentCount);
}

void UBRHiddenStorySubsystem::RefreshHiddenEligibility()
{
	if (AreAllNelHiddenRequestsCompleted() && HasEnoughHiddenFragments())
	{
		bMimikatzAuthoritySeizedUnlocked = true;
		bHiddenEndingEligible = true;
	}
}
