#pragma once

#include "CoreMinimal.h"
#include "BRCombatInterface.h"
#include "GameFramework/Pawn.h"
#include "BRBossBase.generated.h"

class ABRBossAIController;
class UBRStatComponent;
class UBehaviorTree;
class UCapsuleComponent;
class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBRBossStateEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FBRBossStatChanged, float, CurrentValue, float, MaxValue, float, NormalizedValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRBossExecutionEvent, AActor*, Executor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRBossPhaseChanged, EBRBossPhase, NewPhase);

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

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsGroggy() const { return bIsGroggy; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsCombatAIEnabled() const { return bCombatAIEnabled; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	bool IsAttacking() const { return bIsAttacking; }

	UFUNCTION(BlueprintPure, Category="Exception|Boss")
	EBRBossPhase GetBossPhase() const { return BossPhase; }

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> BossCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Collision", meta=(ClampMin="1.0", Units="cm"))
	float BossCollisionRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Collision", meta=(ClampMin="1.0", Units="cm"))
	float BossCollisionHalfHeight = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="1.0"))
	float InitialMaxHP = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="0.0"))
	float InitialMaxGroggy = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="0.0"))
	float GroggyDamageMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Phase", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Phase2StartHPRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Groggy", meta=(ClampMin="0.1", Units="s"))
	float GroggyDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI")
	bool bCombatAIEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI")
	bool bRunBehaviorTreeWhenAssigned = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|AI")
	TObjectPtr<UBehaviorTree> BossBehaviorTree;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bShowDebug = true;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;

	FTimerHandle GroggyTimerHandle;

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
	void ApplyBossCollisionSettings();
	void ApplyMeshVisualTransform();
	bool CanStartCoordinatedAttack() const;
	bool NotifyCoordinatedAttackStarted();
	void NotifyCoordinatedAttackFinished();
	virtual void ClearBaseTimers();
};
