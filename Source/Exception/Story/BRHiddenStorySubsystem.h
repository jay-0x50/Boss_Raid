#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BRHiddenStorySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRNelHiddenRequestChanged, FName, RequestId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRPersistentStoryIdChanged, FName, PersistentId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRHiddenFragmentChanged, int32, CurrentCount, int32, RequiredCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRMainBossProgressChanged, FName, BossId, int32, DefeatedCount);

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

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	bool TryCollectHiddenFragment(FName FragmentId, int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	bool IsHiddenFragmentCollected(FName FragmentId) const;

	UFUNCTION(BlueprintPure, Category="Exception|Hidden Story")
	TArray<FName> GetCollectedHiddenFragmentIds() const;

	UFUNCTION(BlueprintCallable, Category="Exception|Story")
	bool TryConsumeNarrativeBeat(FName BeatId);

	UFUNCTION(BlueprintPure, Category="Exception|Story")
	bool IsNarrativeBeatConsumed(FName BeatId) const;

	UFUNCTION(BlueprintPure, Category="Exception|Story")
	TArray<FName> GetConsumedNarrativeBeatIds() const;

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

	UFUNCTION(BlueprintCallable, Category="Exception|Main Story")
	void MarkMainBossDefeated(FName BossId);

	UFUNCTION(BlueprintPure, Category="Exception|Main Story")
	bool IsMainBossDefeated(FName BossId) const;

	UFUNCTION(BlueprintPure, Category="Exception|Main Story")
	bool AreRequiredBossesDefeated(const TArray<FName>& BossIds) const;

	UFUNCTION(BlueprintPure, Category="Exception|Main Story")
	bool IsThreeBossArcComplete() const;

	UFUNCTION(BlueprintPure, Category="Exception|Main Story")
	int32 GetDefeatedMainBossCount() const { return DefeatedMainBossIds.Num(); }

	UFUNCTION(BlueprintPure, Category="Exception|Main Story")
	TArray<FName> GetDefeatedMainBossIds() const;

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void ApplySavedHiddenStoryState(const TArray<FName>& RegisteredRequests, const TArray<FName>& CompletedRequests, int32 SavedHiddenFragmentCount, bool bSavedMimikatzUnlocked, bool bSavedHiddenEndingEligible, EBRRuntimeEnding SavedLastEnding, const TArray<FName>& SavedDefeatedBossIds);

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void ApplySavedOneShotState(const TArray<FName>& SavedConsumedBeatIds, const TArray<FName>& SavedCollectedFragmentIds);

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void ResetHiddenStoryState();

	UPROPERTY(BlueprintAssignable, Category="Exception|Hidden Story")
	FBRNelHiddenRequestChanged OnNelHiddenRequestCompleted;

	UPROPERTY(BlueprintAssignable, Category="Exception|Hidden Story")
	FBRHiddenFragmentChanged OnHiddenFragmentChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Story")
	FBRPersistentStoryIdChanged OnNarrativeBeatConsumed;

	UPROPERTY(BlueprintAssignable, Category="Exception|Hidden Story")
	FBRPersistentStoryIdChanged OnHiddenFragmentIdCollected;

	UPROPERTY(BlueprintAssignable, Category="Exception|Main Story")
	FBRMainBossProgressChanged OnMainBossProgressChanged;

private:
	UPROPERTY()
	TSet<FName> RegisteredNelRequests;

	UPROPERTY()
	TSet<FName> CompletedNelRequests;

	UPROPERTY()
	TSet<FName> DefeatedMainBossIds;

	UPROPERTY()
	TSet<FName> ConsumedNarrativeBeatIds;

	UPROPERTY()
	TSet<FName> CollectedHiddenFragmentIds;

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
