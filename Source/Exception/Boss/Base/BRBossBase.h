#pragma once

#include "CoreMinimal.h"
#include "BRCombatInterface.h"
#include "GameFramework/Pawn.h"
#include "BRBossBase.generated.h"

class ABRBossAIController;
class UBRStatComponent;
class UBehaviorTree;
class USceneComponent;
class UCapsuleComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimationAsset;
class UCameraShakeBase;
class USoundBase;
class ABRBossTeamCoordinator;

UENUM(BlueprintType)
enum class EBRBossVisualMeshType : uint8
{
	StaticMesh,
	SkeletalMesh
};

UENUM(BlueprintType)
enum class EBRBossPhase : uint8
{
	Phase1,
	Phase2
};

UENUM(BlueprintType)
enum class EBRBossTeamRole : uint8
{
	Solo,
	Melee,
	Ranged,
	Support
};

UENUM(BlueprintType)
enum class EBRBossAnimationStage : uint8
{
	Idle,
	Intro,
	Move,
	PatternWindup UMETA(DisplayName="Windup"),
	PatternImpact UMETA(DisplayName="Impact"),
	PatternRecovery UMETA(DisplayName="Recovery"),
	Groggy,
	Death,
	PhaseTransition,
	// Appended to preserve the serialized values of existing Blueprint maps.
	Hit,
	ExecutionReaction
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBRBossStateEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FBRBossStatChanged, float, CurrentValue, float, MaxValue, float, NormalizedValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRBossExecutionEvent, AActor*, Executor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRBossPhaseChanged, EBRBossPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRBossEnrageChanged, bool, bEnraged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRBossAnimationStageEvent, EBRBossAnimationStage, Stage, FName, ActionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRBossCueEvent, FName, CueName, FVector, WorldLocation);

UCLASS(Abstract, Blueprintable, BlueprintType)
class EXCEPTION_API ABRBossBase : public APawn, public IBRCombatInterface
{
	GENERATED_BODY()

public:
	ABRBossBase();

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual bool ReceiveCombatHit_Implementation(float Damage, float GroggyDamage, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category="Exception|Boss")
	virtual void ResetBoss();

	UFUNCTION(BlueprintCallable, Category="Exception|AI")
	virtual void SetCombatAIEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Exception|Boss")
	void PrepareForArenaInactive();

	UFUNCTION(BlueprintCallable, Category="Exception|Boss")
	void StartBossIntro();

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsGroggy() const { return bIsGroggy; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsCombatAIEnabled() const { return bCombatAIEnabled; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsAttacking() const { return bIsAttacking; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsPhaseTransitioning() const { return bIsPhaseTransitioning; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	EBRBossPhase GetBossPhase() const { return BossPhase; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsEnraged() const { return bIsEnraged; }

	UFUNCTION(BlueprintCallable, Category="Exception|Boss")
	void SetEnraged(bool bNewEnraged);

	/** Enters Phase 2 through the normal transition path. */
	UFUNCTION(BlueprintCallable, Category="Exception|Boss")
	void ForcePhase2(bool bReplayTransitionIfAlreadyPhase2 = false);

	UFUNCTION(BlueprintPure, Category="Exception|Team")
	EBRBossTeamRole GetTeamRole() const { return TeamRole; }

	UFUNCTION(BlueprintPure, Category="Exception|Team")
	bool IsTeamMateAttacking() const;

	UFUNCTION(BlueprintPure, Category="Exception|Team")
	bool IsTeamMateWithin(float Distance) const;

	UFUNCTION(BlueprintCallable, Category="Exception|Team")
	void SetTeamCoordinator(ABRBossTeamCoordinator* NewTeamCoordinator);

	UFUNCTION(BlueprintPure, Category="Exception|Team")
	ABRBossTeamCoordinator* GetTeamCoordinator() const { return TeamCoordinator; }

	UFUNCTION(BlueprintCallable, Category="Exception|Team")
	virtual void ApplyTeamSlot(int32 TeamSlotIndex);

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetMaxHP() const;

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetCurrentHP() const;

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetHPPercent() const;

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetMaxGroggy() const;

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetCurrentGroggy() const;

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetGroggyPercent() const;

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	FText GetBossDisplayName() const;

	UFUNCTION(BlueprintPure, Category="Exception|AI")
	UBehaviorTree* GetBossBehaviorTree() const { return BossBehaviorTree; }

	UFUNCTION(BlueprintPure, Category="Exception|AI")
	bool ShouldRunBehaviorTree() const { return bRunBehaviorTreeWhenAssigned; }

	UFUNCTION(BlueprintPure, Category="Exception|AI")
	ABRBossAIController* GetBossAIController() const;

	UFUNCTION(BlueprintPure, Category="Exception|AI")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintCallable, Category="Exception|Stats")
	bool ApplyGroggyDamage(float GroggyDamage, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category="Exception|Execution")
	bool CanBeExecuted() const;

	UFUNCTION(BlueprintCallable, Category="Exception|Execution")
	bool BeginExecution(AActor* Executor);

	UFUNCTION(BlueprintCallable, Category="Exception|Execution")
	bool CompleteExecution(float Damage, AActor* Executor);

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossStateEvent OnBossDead;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossStateEvent OnBossGroggy;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossStateEvent OnBossRecoveredFromGroggy;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossStatChanged OnBossHPChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossStatChanged OnBossGroggyChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossExecutionEvent OnExecutionStarted;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossExecutionEvent OnExecutionCompleted;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossEnrageChanged OnEnrageChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossStateEvent OnPhaseTransitionFinished;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossAnimationStageEvent OnAnimationStageChanged;

	/** Designer-facing SFX hook. It fires even when no temporary sound is assigned. */
	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRBossCueEvent OnBossCueRequested;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void OnBossReset();
	virtual void OnBossDeadInternal();
	virtual void OnBossGroggyInternal();
	virtual void OnBossRecoveredFromGroggyInternal();
	virtual void OnBossPhaseChanged(EBRBossPhase NewPhase);
	virtual void UpdateBossAI(float DeltaSeconds);
	virtual void DrawBossDebug() const;
	virtual FString GetBossDebugName() const;

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Boss")
	void BP_BossIntroStarted();

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Boss")
	void BP_BossEnrageChanged(bool bNewEnraged);

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Animation")
	void BP_BossAnimationStageChanged(EBRBossAnimationStage Stage, FName ActionName);

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Audio")
	void BP_BossCueRequested(FName CueName, FVector WorldLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> HitCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBRStatComponent> StatComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Visual")
	EBRBossVisualMeshType VisualMeshType = EBRBossVisualMeshType::StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Visual")
	FVector MeshRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Visual")
	FRotator MeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Visual", meta=(ClampMin="0.01"))
	FVector MeshRelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement")
	bool bUseGroundGravity = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement", meta=(ClampMin="0.0", Units="cm"))
	float GroundTraceActorHalfHeight = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement", meta=(ClampMin="0.0", Units="cm/s^2"))
	float GroundGravity = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement", meta=(ClampMin="0.0", Units="cm"))
	float GroundTraceDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement", meta=(ClampMin="0.0", Units="cm"))
	float GroundSnapTolerance = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="1.0"))
	float InitialMaxHP = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="0.0"))
	float InitialMaxGroggy = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="0.0"))
	float GroggyDamageMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Phase", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Phase2StartHPRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Phase", meta=(ClampMin="0.0", Units="s"))
	float PhaseTransitionDuration = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Phase")
	bool bInvulnerableDuringPhaseTransition = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Groggy", meta=(ClampMin="0.1", Units="s"))
	float GroggyDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI")
	bool bCombatAIEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bDisableCollisionWhenInactive = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bShowBossWhenInactive = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bResetTransformOnBossReset = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI")
	bool bRunBehaviorTreeWhenAssigned = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI")
	TObjectPtr<UBehaviorTree> BossBehaviorTree;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TMap<EBRBossAnimationStage, TObjectPtr<UAnimationAsset>> StageAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TMap<FName, TObjectPtr<UAnimationAsset>> ActionAnimations;

	/** Optional authored sounds keyed by the cue names emitted by the combat code. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Audio")
	TMap<FName, TObjectPtr<USoundBase>> BossSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Audio", meta=(ClampMin="0.0"))
	float BossSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback")
	bool bUseProceduralIdleFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback", meta=(ClampMin="0.0", Units="cm"))
	float ProceduralIdleBobAmplitude = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback", meta=(ClampMin="0.0", Units="Hz"))
	float ProceduralIdleFrequency = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback", meta=(ClampMin="0.0", Units="deg"))
	float ProceduralIdleLeanAngle = 1.0f;

	/** Whole-body fallback only; authored quadruped animation always takes priority. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback")
	bool bUseProceduralStageFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback", meta=(ClampMin="0.0", Units="cm"))
	float ProceduralAttackTravelDistance = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback", meta=(ClampMin="0.0", Units="deg"))
	float ProceduralAttackLeanAngle = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback", meta=(ClampMin="0.0", Units="cm"))
	float ProceduralGroggyDropDistance = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Fallback", meta=(ClampMin="0.0", Units="deg"))
	float ProceduralDeathRollAngle = 72.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Feedback")
	TSubclassOf<UCameraShakeBase> CombatHitCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Feedback", meta=(ClampMin="0.0"))
	float BossReceivedHitCameraShakeScale = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BossReceivedHitRumbleIntensity = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Feedback", meta=(ClampMin="0.0", Units="cm"))
	float ProceduralHitReactionDistance = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Feedback", meta=(ClampMin="0.01", Units="s"))
	float ProceduralHitReactionDuration = 0.14f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Team")
	TObjectPtr<ABRBossTeamCoordinator> TeamCoordinator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Team")
	EBRBossTeamRole TeamRole = EBRBossTeamRole::Solo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	EBRBossPhase BossPhase = EBRBossPhase::Phase1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	bool bIsGroggy = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	bool bIsBeingExecuted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	bool bIsPhaseTransitioning = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	bool bIsEnraged = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bShowDebug = false;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastDamageCauser;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> CurrentBossAnimationAsset;

	float VerticalFallSpeed = 0.0f;
	float ProceduralIdleTime = 0.0f;
	float ProceduralStageTime = 0.0f;
	float ProceduralIdleBlendAlpha = 0.0f;
	FTransform VisualRootBaseRelativeTransform = FTransform::Identity;
	bool bVisualRootBaseTransformCaptured = false;
	float ProceduralHitReactionTime = 0.0f;
	FVector ProceduralHitReactionDirection = FVector::BackwardVector;
	EBRBossAnimationStage CurrentAnimationStage = EBRBossAnimationStage::Idle;
	FName CurrentAnimationActionName = NAME_None;

	FTransform InitialBossTransform = FTransform::Identity;

	FTimerHandle GroggyTimerHandle;
	FTimerHandle PhaseTransitionTimerHandle;

	UFUNCTION()
	virtual void HandleDead();

	UFUNCTION()
	virtual void HandleGroggy();

	UFUNCTION()
	void HandleHPChanged(float CurrentValue, float MaxValue, float NormalizedValue);

	UFUNCTION()
	void HandleGroggyChanged(float CurrentValue, float MaxValue, float NormalizedValue);

	void RecoverFromGroggy();
	void RefreshPhaseByHP();
	void BeginPhaseTransition();
	void FinishPhaseTransition();
	void ApplyMeshVisualTransform();
	void UpdateProceduralIdleMotion(float DeltaSeconds);
	void StartProceduralHitReaction(AActor* DamageCauser);
	void UpdateProceduralHitReaction(float DeltaSeconds);
	void PlayCameraFeedbackForActor(AActor* FeedbackActor, float ShakeScale, float RumbleIntensity) const;
	void RequestBossCue(FName CueName);
	void SetBossAnimationPlaying(bool bShouldPlay);
	void PlayBossStageAnimation(EBRBossAnimationStage Stage, FName ActionName);
	void ApplyGroundGravity(float DeltaSeconds);
	void NotifyBossAnimationStage(EBRBossAnimationStage Stage, FName ActionName = NAME_None);
	bool CanStartCoordinatedAttack() const;
	bool NotifyCoordinatedAttackStarted();
	void NotifyCoordinatedAttackFinished();
	virtual void ClearBaseTimers();
};
