#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRBossTeamCoordinator.generated.h"

class ABRBossBase;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Boss Team Coordinator"))
class EXCEPTION_API ABRBossTeamCoordinator : public AActor
{
	GENERATED_BODY()

public:
	ABRBossTeamCoordinator();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Exception|Team")
	void RegisterBoss(ABRBossBase* Boss);

	UFUNCTION(BlueprintCallable, Category="Exception|Team")
	void BindConfiguredTeamMembers();

	UFUNCTION(BlueprintCallable, Category="Exception|Team")
	void UnregisterBoss(ABRBossBase* Boss);

	UFUNCTION(BlueprintPure, Category="Exception|Team")
	bool CanStartAttack(ABRBossBase* RequestingBoss) const;

	UFUNCTION(BlueprintCallable, Category="Exception|Team")
	bool NotifyAttackStarted(ABRBossBase* AttackingBoss);

	UFUNCTION(BlueprintCallable, Category="Exception|Team")
	void NotifyAttackFinished(ABRBossBase* AttackingBoss);

	UFUNCTION(BlueprintPure, Category="Exception|Team")
	ABRBossBase* GetActiveAttacker() const { return ActiveAttacker; }

	UFUNCTION(BlueprintCallable, Category="Exception|Team")
	void GetTeamMembers(TArray<ABRBossBase*>& OutTeamMembers) const;

	UFUNCTION(BlueprintPure, Category="Exception|Team")
	bool IsOtherBossAttacking(ABRBossBase* RequestingBoss) const;

	UFUNCTION(BlueprintPure, Category="Exception|Team")
	bool IsOtherBossWithin(ABRBossBase* RequestingBoss, float Distance) const;

	/** Called by a member after an applied hit so team escalation can be evaluated. */
	void NotifyMemberHealthChanged(ABRBossBase* ChangedBoss);

	/** Called after the member has entered its dead state. */
	void NotifyMemberDefeated(ABRBossBase* DefeatedBoss);

	/** Clears encounter latches when an arena resets its members. */
	void NotifyMemberReset(ABRBossBase* ResetBoss);

	/** Returns true once, and only for the final defeated member of a real team. */
	bool ConsumeTeamDefeatReward(ABRBossBase* DefeatedBoss);

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Team")
	TArray<TObjectPtr<ABRBossBase>> TeamMembers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team")
	bool bAllowSimultaneousAttacks = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team", meta=(ClampMin="0.0", Units="s"))
	float TeamAttackGap = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team|Enrage", meta=(ClampMin="0.0", ClampMax="1.0"))
	float HealthDifferenceEnrageThreshold = 0.30f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Team")
	TObjectPtr<ABRBossBase> ActiveAttacker;

	float LastAttackFinishedTime = -1000.0f;
	bool bHealthDifferenceEnrageTriggered = false;
	bool bSurvivorEscalationTriggered = false;
	bool bTeamRewardGranted = false;

	void EvaluateHealthDifferenceEnrage();
	void EvaluateSurvivorEscalation();
	int32 GetValidMemberCount() const;
};
