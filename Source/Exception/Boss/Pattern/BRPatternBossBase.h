#pragma once

#include "CoreMinimal.h"
#include "Boss/Base/BRBossBase.h"
#include "BRPatternBossBase.generated.h"

class UNiagaraSystem;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Effects")
	TObjectPtr<UNiagaraSystem> TelegraphEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Effects")
	TObjectPtr<UNiagaraSystem> ImpactEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Effects")
	FName TelegraphSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Effects")
	FName ImpactSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Effects")
	FVector TelegraphEffectScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Effects")
	FVector ImpactEffectScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	EBRBossPatternType PatternType = EBRBossPatternType::Melee;

	/** Keeps ranged zones fixed at the target's windup position so they can be dodged. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	bool bCenterAOEOnTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float MinRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float MaxRange = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0"))
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.01", Units="s"))
	float Windup = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation", meta=(ClampMin="0.0", Units="s"))
	float ImpactHoldTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation", meta=(ClampMin="0.0", Units="s"))
	float RecoveryTime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.01", Units="s"))
	float Cooldown = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="1.0", Units="cm"))
	float Radius = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float ForwardOffset = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="cm"))
	float DashDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Feedback", meta=(ClampMin="0.0"))
	float KnockbackStrength = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Feedback", meta=(ClampMin="0.0"))
	float KnockbackLift = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Feedback", meta=(ClampMin="0.0"))
	float CameraShakeScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RumbleIntensity = 0.32f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Effects|Fallback")
	TObjectPtr<UNiagaraSystem> MeleeTelegraphEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Effects|Fallback")
	TObjectPtr<UNiagaraSystem> DashTelegraphEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Effects|Fallback")
	TObjectPtr<UNiagaraSystem> AOETelegraphEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Effects|Fallback")
	TObjectPtr<UNiagaraSystem> MeleeEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Effects|Fallback")
	TObjectPtr<UNiagaraSystem> DashEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Effects|Fallback")
	TObjectPtr<UNiagaraSystem> AOEEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern")
	TArray<FBRBossPatternData> AttackPatterns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Pattern", meta=(ClampMin="0.0", Units="s"))
	float MinAttackGap = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Phase", meta=(ClampMin="0.0"))
	float PhaseTransitionEffectScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Phase", meta=(ClampMin="0.0"))
	float PhaseTransitionCameraShakeScale = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bDrawAttackDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bDrawAttackTelegraph = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug", meta=(ClampMin="0.0", Units="cm"))
	float TelegraphHeightOffset = 12.0f;

	FTimerHandle AttackWindupTimerHandle;
	FTimerHandle AttackRecoveryTimerHandle;
	float LastAttackTime = -1000.0f;
	float NextAttackTime = -1000.0f;
	TMap<FName, float> LastPatternTimes;
	int32 ActivePatternIndex = INDEX_NONE;
	int32 LastPatternIndex = INDEX_NONE;
	int32 AttackSequence = 0;
	FBRBossPatternData ActivePatternSnapshot;
	FVector LockedAttackOrigin = FVector::ZeroVector;
	FVector LockedTargetLocation = FVector::ZeroVector;
	FVector LockedAttackDirection = FVector::ForwardVector;
	bool bHasActivePattern = false;
	bool bAttackHasImpacted = false;
	bool bAttackSlotClaimed = false;

	void FaceTarget(float DeltaSeconds);
	void MoveTowardTarget(float DeltaSeconds);
	void MoveToTeamStandbyDistance(float DeltaSeconds, float CurrentDistanceToTarget);
	int32 SelectPattern(float DistanceToTarget) const;
	bool CanStartPattern(const FBRBossPatternData& Pattern, float DistanceToTarget) const;
	float GetPatternCooldown(const FBRBossPatternData& Pattern) const;
	float GetCurrentMoveSpeed() const;
	void StartBossAttack(int32 PatternIndex);
	void PerformBossAttack(int32 AttackId);
	void BeginAttackRecovery(const FBRBossPatternData& Pattern, int32 AttackId);
	void StartAttackRecovery(int32 AttackId);
	void FinishBossAttack(int32 AttackId);
	void CancelBossAttack();
	void ReleaseAttackSlot();
	void ApplyPatternHitFeedback(AActor* HitActor, const FBRBossPatternData& Pattern, const FVector& HitDirection);
	FVector GetAOECenter(const FBRBossPatternData& Pattern, float HeightOffset) const;
	FVector GetLockedDashDirection(const FBRBossPatternData& Pattern) const;
	UNiagaraSystem* ResolvePatternEffect(const FBRBossPatternData& Pattern, bool bTelegraph) const;
	FTransform GetPatternEffectTransform(const FBRBossPatternData& Pattern, float HeightOffset) const;
	void SpawnPatternEffect(UNiagaraSystem* Effect, const FBRBossPatternData& Pattern, FName SocketName, float HeightOffset, const FVector& Scale) const;
	void DrawActivePatternTelegraph() const;
	virtual void ClearBaseTimers() override;
};
