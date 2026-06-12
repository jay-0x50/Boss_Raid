#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BRBossAIController.generated.h"

class ABRBossBase;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBlackboardData;

namespace BRBossBlackboardKeys
{
	static const FName SelfActor(TEXT("SelfActor"));
	static const FName TargetActor(TEXT("TargetActor"));
	static const FName DistanceToTarget(TEXT("DistanceToTarget"));
	static const FName HPPercent(TEXT("HPPercent"));
	static const FName GroggyPercent(TEXT("GroggyPercent"));
	static const FName IsAIEnabled(TEXT("IsAIEnabled"));
	static const FName IsDead(TEXT("IsDead"));
	static const FName IsGroggy(TEXT("IsGroggy"));
	static const FName IsAttacking(TEXT("IsAttacking"));
	static const FName IsPhase2(TEXT("IsPhase2"));
	static const FName TeamMateAttacking(TEXT("TeamMateAttacking"));
}

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Boss AI Controller"))
class EXCEPTION_API ABRBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABRBossAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintCallable, Category="Exception|Boss AI")
	void SetBossAIEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Exception|Boss AI")
	void RefreshBossBlackboard();

	UFUNCTION(BlueprintPure, Category="Exception|Boss AI")
	bool IsBehaviorTreeActive() const;

	UFUNCTION(BlueprintPure, Category="Exception|Boss AI")
	UBlackboardComponent* GetBossBlackboardComponent() const { return BossBlackboardComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBehaviorTreeComponent> BossBehaviorTreeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBlackboardComponent> BossBlackboardComponent;

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardData> RuntimeBlackboardAsset;

	bool bBehaviorTreeActive = false;

	ABRBossBase* GetControlledBoss() const;
	UBlackboardData* GetOrCreateRuntimeBlackboardAsset();
	bool EnsureBlackboard();
};
