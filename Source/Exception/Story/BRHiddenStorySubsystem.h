#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BRHiddenStorySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRNelHiddenRequestChanged, FName, RequestId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRHiddenFragmentChanged, int32, CurrentCount, int32, RequiredCount);

UENUM(BlueprintType)
enum class EBRRuntimeEnding : uint8
{
	None,
	BasicCMDDefeated,
	HiddenAuthoritySeized
};

UCLASS(BlueprintType)
class EXCEPTION_API UBRHiddenStorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void RegisterDefaultNelHiddenRequests();

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void RegisterNelHiddenRequest(FName RequestId);

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void MarkNelHiddenRequestCompleted(FName RequestId);

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	bool IsNelHiddenRequestCompleted(FName RequestId) const;

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	bool AreAllNelHiddenRequestsCompleted() const;

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void CollectHiddenFragment(int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	int32 GetHiddenFragmentCount() const { return HiddenFragmentCount; }

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	int32 GetRequiredHiddenFragmentCount() const { return RequiredHiddenFragmentCount; }

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	bool HasEnoughHiddenFragments() const { return HiddenFragmentCount >= RequiredHiddenFragmentCount; }

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void SetMimikatzAuthoritySeizedUnlocked(bool bUnlocked);

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	bool IsMimikatzAuthoritySeizedUnlocked() const { return bMimikatzAuthoritySeizedUnlocked; }

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void SetHiddenEndingEligible(bool bEligible);

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	bool IsHiddenEndingEligible() const { return bHiddenEndingEligible; }

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	EBRRuntimeEnding ResolveCMDEnding(bool bDefeatedWithMimikatzAuthoritySeized);

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	EBRRuntimeEnding GetLastResolvedEnding() const { return LastResolvedEnding; }

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	TArray<FName> GetRegisteredNelRequestIds() const;

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	TArray<FName> GetCompletedNelRequestIds() const;

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void ApplySavedHiddenStoryState(const TArray<FName>& RegisteredRequests, const TArray<FName>& CompletedRequests, int32 SavedHiddenFragmentCount, bool bSavedMimikatzUnlocked, bool bSavedHiddenEndingEligible, EBRRuntimeEnding SavedLastEnding);

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void ResetHiddenStoryState();

	UPROPERTY(BlueprintAssignable, Category="Exception|Hidden Story")
	FBRNelHiddenRequestChanged OnNelHiddenRequestCompleted;

	UPROPERTY(BlueprintAssignable, Category="Exception|Hidden Story")
	FBRHiddenFragmentChanged OnHiddenFragmentChanged;

private:
	UPROPERTY()
	TSet<FName> RegisteredNelRequests;

	UPROPERTY()
	TSet<FName> CompletedNelRequests;

	UPROPERTY()
	int32 HiddenFragmentCount = 0;

	UPROPERTY()
	int32 RequiredHiddenFragmentCount = 3;

	UPROPERTY()
	bool bMimikatzAuthoritySeizedUnlocked = false;

	UPROPERTY()
	bool bHiddenEndingEligible = false;

	UPROPERTY()
	EBRRuntimeEnding LastResolvedEnding = EBRRuntimeEnding::None;

	void RefreshHiddenEligibility();
};
