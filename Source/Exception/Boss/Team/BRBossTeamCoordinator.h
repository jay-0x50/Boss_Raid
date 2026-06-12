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

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Team")
	TArray<TObjectPtr<ABRBossBase>> TeamMembers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team")
	bool bAllowSimultaneousAttacks = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team", meta=(ClampMin="0.0", Units="s"))
	float TeamAttackGap = 0.45f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Team")
	TObjectPtr<ABRBossBase> ActiveAttacker;

	float LastAttackFinishedTime = -1000.0f;
};
