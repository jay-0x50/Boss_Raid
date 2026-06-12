// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ExceptionCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UAnimMontage;
class UBRInventoryComponent;
class ABRBossBase;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class EBRPlayerCombatState : uint8
{
	Idle,
	LightAttack,
	HeavyAttack,
	Dodge,
	Parry,
	Execution,
	Hit,
	Dead
};

UENUM(BlueprintType)
enum class EBRPlayerUpgradeStat : uint8
{
	Vitality,
	Endurance,
	Power
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FBRPlayerStatChanged, float, CurrentValue, float, MaxValue, float, NormalizedValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRPlayerStateChanged, EBRPlayerCombatState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBRPlayerProgressionChanged);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AExceptionCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBRInventoryComponent> InventoryComponent;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Light Attack Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* LightAttackAction;

	/** Heavy Attack Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* HeavyAttackAction;

	/** Dodge Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* DodgeAction;

	/** Parry Input Action */
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* ParryAction;

	/** Interact Input Action, reserved for groggy execution later */
	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* LockOnAction;

	// 체력 / 스태미나
	/** Max player HP for the boss raid demo */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="1.0"))
	float MaxHP = 1000.0f;

	/** Max stamina used by attack, dodge, and parry */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="1.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="0.0"))
	float StaminaRegenPerSecond = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Stats", meta=(ClampMin="0.0"))
	float StaminaRegenDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="1"))
	int32 PlayerLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0"))
	int32 UpgradePoints = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0"))
	int32 VitalityLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0"))
	int32 EnduranceLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0"))
	int32 PowerLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0.0"))
	float HPPerVitalityLevel = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0.0"))
	float StaminaPerEnduranceLevel = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0.0"))
	float DamagePerPowerLevel = 3.0f;

	// 스태미나 소모량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Cost", meta=(ClampMin="0.0"))
	float LightAttackStaminaCost = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Cost", meta=(ClampMin="0.0"))
	float HeavyAttackStaminaCost = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Cost", meta=(ClampMin="0.0"))
	float DodgeStaminaCost = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Cost", meta=(ClampMin="0.0"))
	float ParryStaminaCost = 15.0f;

	// 패링 그로기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Parry", meta=(ClampMin="0.0"))
	float ParrySuccessGroggyDamage = 40.0f;

	// 액션 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Timing", meta=(ClampMin="0.01", Units="s"))
	float LightAttackDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Timing", meta=(ClampMin="0.01", Units="s"))
	float HeavyAttackDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Timing", meta=(ClampMin="0.01", Units="s"))
	float DodgeDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Timing", meta=(ClampMin="0.01", Units="s"))
	float DodgeInvincibleDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Timing", meta=(ClampMin="0.01", Units="s"))
	float ParryDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Timing", meta=(ClampMin="0.01", Units="s"))
	float ParryActiveDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Timing", meta=(ClampMin="0.01", Units="s"))
	float HitStunDuration = 0.35f;

	// 공격력 / 그로기 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="0.0"))
	float LightAttackDamage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="0.0"))
	float LightAttackGroggyDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="0.0"))
	float HeavyAttackDamage = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="0.0"))
	float HeavyAttackGroggyDamage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="0.0", Units="cm"))
	float AttackTraceDistance = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="1.0", Units="cm"))
	float AttackTraceRadius = 55.0f;

	// 회피 / 피격 넉백
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Dodge", meta=(ClampMin="0.0"))
	float DodgeImpulseStrength = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hit", meta=(ClampMin="0.0"))
	float HitKnockbackStrength = 350.0f;

	// 애니메이션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TObjectPtr<UAnimMontage> LightAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TObjectPtr<UAnimMontage> HeavyAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TObjectPtr<UAnimMontage> ParryMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TObjectPtr<UAnimMontage> ExecutionMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	// 현재 체력 / 현재 스태미나
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Stats")
	float CurrentHP = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Stats")
	float CurrentStamina = 0.0f;

	// 현재 전투 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	EBRPlayerCombatState CombatState = EBRPlayerCombatState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	bool bIsInvincible = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|State")
	bool bIsParryActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bShowCombatDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bDrawAttackTraceDebug = true;

	// 리스폰
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Respawn", meta=(ClampMin="0.0", Units="s"))
	float RespawnDelay = 1.5f;

	// 락온
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="cm"))
	float LockOnRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="cm"))
	float LockOnBreakRange = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0"))
	float LockOnRotationInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0"))
	float LockOnLookInputSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="deg"))
	float LockOnYawOffsetLimit = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="deg"))
	float LockOnPitchOffsetLimit = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0"))
	float LockOnOffsetReturnSpeed = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0"))
	float LockOnCharacterRotationInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="cm"))
	float LockOnTargetHeightOffset = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="cm"))
	float LockOnCameraArmLength = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(Units="cm"))
	FVector LockOnCameraTargetOffset = FVector(0.0f, 0.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(Units="cm"))
	FVector LockOnCameraSocketOffset = FVector(0.0f, 55.0f, 25.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="cm"))
	float FreeCameraArmLength = 400.0f;

	// 처형
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="0.0", Units="cm"))
	float ExecutionRange = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="0.0", Units="cm"))
	float ExecutionSnapDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="0.01", Units="s"))
	float ExecutionDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ExecutionDamageMaxHPRatio = 0.3f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	int32 LastAttackHitCount = 0;

	// 락온 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|LockOn")
	bool bIsLockedOn = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|LockOn")
	TObjectPtr<AActor> LockOnTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|LockOn")
	float LockOnYawOffset = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|LockOn")
	float LockOnPitchOffset = 0.0f;

	float LastStaminaSpendTime = -1000.0f;
	float LastAttackDebugTime = -1000.0f;

	// 타이머
	FTimerHandle StateTimerHandle;
	FTimerHandle InvincibleTimerHandle;
	FTimerHandle ParryTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTimerHandle ExecutionTimerHandle;

	// 처형 대상
	UPROPERTY(Transient)
	TObjectPtr<ABRBossBase> PendingExecutionTarget;

	// 런타임 입력
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeCombatMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeLightAttackAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeHeavyAttackAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeDodgeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeParryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeInteractAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeLockOnAction;

public:

	/** Constructor */
	AExceptionCharacter();	

	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void LightAttackPressed();
	void HeavyAttackPressed();
	void DodgePressed();
	void ParryPressed();
	void InteractPressed();
	void LockOnPressed();
	void SetupRuntimeCombatInput(class UEnhancedInputComponent* EnhancedInputComponent);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category="Exception|Combat")
	virtual bool DoLightAttack();

	UFUNCTION(BlueprintCallable, Category="Exception|Combat")
	virtual bool DoHeavyAttack();

	UFUNCTION(BlueprintCallable, Category="Exception|Combat")
	virtual bool DoDodge();

	UFUNCTION(BlueprintCallable, Category="Exception|Combat")
	virtual bool DoParry();

	UFUNCTION(BlueprintCallable, Category="Exception|Combat")
	virtual void DoInteract();

	UFUNCTION(BlueprintCallable, Category="Exception|Execution")
	virtual bool TryExecution();

	UFUNCTION(BlueprintCallable, Category="Exception|LockOn")
	virtual void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category="Exception|LockOn")
	virtual void ClearLockOn();

	UFUNCTION(BlueprintPure, Category="Exception|LockOn")
	bool IsLockedOn() const { return bIsLockedOn; }

	UFUNCTION(BlueprintPure, Category="Exception|LockOn")
	AActor* GetLockOnTarget() const { return LockOnTarget; }

	UFUNCTION(BlueprintCallable, Category="Exception|Combat")
	virtual void PerformAttackTrace(float Damage, float GroggyDamage);

	UFUNCTION(BlueprintCallable, Category="Exception|Stats")
	bool SpendStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category="Exception|Stats")
	void RestoreHPAndStamina();

	UFUNCTION(BlueprintCallable, Category="Exception|Stats")
	void ApplySavedStats(float SavedHP, float SavedStamina);

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	void ApplySavedProgression(int32 SavedPlayerLevel, int32 SavedUpgradePoints, int32 SavedVitalityLevel, int32 SavedEnduranceLevel, int32 SavedPowerLevel);

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category="Exception|Stats")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetPlayerLevel() const { return PlayerLevel; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetUpgradePoints() const { return UpgradePoints; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetVitalityLevel() const { return VitalityLevel; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetEnduranceLevel() const { return EnduranceLevel; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetPowerLevel() const { return PowerLevel; }

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	void AddUpgradePoints(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	bool SpendUpgradePoint(EBRPlayerUpgradeStat UpgradeStat);

	UFUNCTION(BlueprintPure, Category="Exception|Inventory")
	UBRInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category="Exception|Combat")
	bool IsParryActive() const { return bIsParryActive; }

	UFUNCTION(BlueprintCallable, Category="Exception|Respawn")
	void RespawnAtCheckpoint();

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRPlayerStatChanged OnHPChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRPlayerStatChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRPlayerStateChanged OnCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Exception|Events")
	FBRPlayerProgressionChanged OnProgressionChanged;

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Events")
	void BP_CombatActionStarted(EBRPlayerCombatState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Events")
	void BP_CombatActionEnded(EBRPlayerCombatState PreviousState);

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Events")
	void BP_AttackHit(AActor* HitActor, float Damage);

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Events")
	void BP_DamageReceived(float Damage);

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Events")
	void BP_ParryWindowStarted();

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Events")
	void BP_ParryWindowEnded();

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Events")
	void BP_ExecutionStarted(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Events")
	void BP_ExecutionFinished(AActor* Target, float Damage);

protected:
	bool CanStartCombatAction() const;
	void SetCombatState(EBRPlayerCombatState NewState);
	void FinishCombatAction();
	void EndInvincibility();
	void EndParryWindow();
	void PlayOptionalMontage(UAnimMontage* Montage);
	void BroadcastHP();
	void BroadcastStamina();
	void DrawCombatDebug() const;
	FString GetCombatStateName() const;
	void RegisterInitialCheckpoint();
	AActor* FindLockOnTarget() const;
	void UpdateLockOn(float DeltaSeconds);
	ABRBossBase* FindExecutionTarget() const;
	void StartExecution(ABRBossBase* Target);
	void FinishExecution();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
