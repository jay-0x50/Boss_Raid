#pragma once

#include "CoreMinimal.h"
#include "Boss/Base/BRBossBase.h"
#include "BRPatternBossBase.generated.h"

UENUM(BlueprintType)
enum class EBRBossPatternType : uint8
{
	Melee,
	Dash,
	AOE
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRBossPatternEvent, FName, PatternName);

USTRUCT(BlueprintType)
struct FBRBossPatternData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	FName PatternName = TEXT("Basic");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	FName AnimationActionName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	EBRBossPatternType PatternType = EBRBossPatternType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float MinRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float MaxRange = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0"))
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.01", Units="s"))
	float Windup = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.01", Units="s"))
	float Cooldown = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="1.0", Units="cm"))
	float Radius = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float ForwardOffset = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float DashDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	bool bDashAwayFromTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	bool bRequiresTeamMateNear = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float TeamMateNearDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	bool bEnableInPhase1 = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	bool bEnableInPhase2 = true;
};

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Pattern Boss Base"))
class EXCEPTION_API ABRPatternBossBase : public ABRBossBase
{
	GENERATED_BODY()

public:
	ABRPatternBossBase();

	UFUNCTION(BlueprintCallable, Category="Exception|Boss")
	void ResetPatternBoss();

	virtual void SetCombatAIEnabled(bool bEnabled) override;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossPatternEvent OnPatternStarted;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossPatternEvent OnPatternHit;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossPatternEvent OnPatternFinished;

protected:
	virtual void OnBossReset() override;
	virtual void OnBossDeadInternal() override;
	virtual void OnBossGroggyInternal() override;
	virtual void OnBossRecoveredFromGroggyInternal() override;
	virtual void OnBossPhaseChanged(EBRBossPhase NewPhase) override;
	virtual void UpdateBossAI(float DeltaSeconds) override;
	virtual void DrawBossDebug() const override;
	virtual FString GetBossDebugName() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI", meta=(ClampMin="0.0", Units="cm"))
	float DetectionRange = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI", meta=(ClampMin="0.0", Units="cm/s"))
	float MoveSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI", meta=(ClampMin="0.0"))
	float Phase2MoveSpeedMultiplier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI", meta=(ClampMin="0.0"))
	float Phase2CooldownMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI", meta=(ClampMin="0.0"))
	float RotationInterpSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team", meta=(ClampMin="0.0", Units="cm"))
	float MeleeStandbyDistance = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team", meta=(ClampMin="0.0", Units="cm"))
	float RangedStandbyDistance = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team", meta=(ClampMin="0.0", Units="cm"))
	float RangedComfortMinDistance = 480.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	TArray<FBRBossPatternData> AttackPatterns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bDrawAttackDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bDrawAttackTelegraph = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug", meta=(ClampMin="0.0", Units="cm"))
	float TelegraphHeightOffset = 12.0f;

	FTimerHandle AttackWindupTimerHandle;
	float LastAttackTime = -1000.0f;
	int32 ActivePatternIndex = INDEX_NONE;

	void FaceTarget(float DeltaSeconds);
	void MoveTowardTarget(float DeltaSeconds);
	void MoveToTeamStandbyDistance(float DeltaSeconds, float CurrentDistanceToTarget);
	int32 SelectPattern(float DistanceToTarget) const;
	bool CanStartPattern(const FBRBossPatternData& Pattern, float DistanceToTarget) const;
	float GetPatternCooldown(const FBRBossPatternData& Pattern) const;
	float GetCurrentMoveSpeed() const;
	void StartBossAttack(int32 PatternIndex);
	void PerformBossAttack();
	void DrawActivePatternTelegraph() const;
	virtual void ClearBaseTimers() override;
};
