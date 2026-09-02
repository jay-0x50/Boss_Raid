// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BRInventoryTypes.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ExceptionCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UAnimMontage;
class UAnimSequence;
class UMaterialInstanceDynamic;
class UBRInventoryComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class ABRBossBase;
class ABRPlayerGraveMarker;
struct FInputActionValue;
struct FBRInventoryItemDefinition;
struct FBRInventorySlot;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class EBRPlayerCombatState : uint8
{
	Idle,
	LightAttack,
	HeavyAttack,
	Dodge,
	Parry,
	Healing,
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> RootBladeR;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> RootBladeL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> RuntimeFlask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> FlaskAura;
	
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

	UPROPERTY(EditAnywhere, Category="Input|Combat")
	UInputAction* UseFlaskAction;

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
	int32 CurrentExperience = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Progression")
	int32 DroppedExperience = 0;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="1"))
	int32 BaseLevelUpExperienceCost = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0"))
	int32 LevelUpExperienceCostIncrease = 35;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Progression", meta=(ClampMin="0.0"))
	int32 BossKillUpgradePointReward = 1;

	// 스태미나 소모량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Cost", meta=(ClampMin="0.0"))
	float LightAttackStaminaCost = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Cost", meta=(ClampMin="0.0"))
	float HeavyAttackStaminaCost = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Cost", meta=(ClampMin="0.0"))
	float DodgeStaminaCost = 25.0f;

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
	float DodgeDuration = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Timing", meta=(ClampMin="0.01", Units="s"))
	float DodgeInvincibleDuration = 0.32f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="1.0"))
	float RootDmg = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="1.0"))
	float RootCmdDmg = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="0.0", Units="cm"))
	float AttackTraceDistance = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack", meta=(ClampMin="1.0", Units="cm"))
	float AttackTraceRadius = 55.0f;

	// 회피 / 피격 넉백
	/** Legacy impulse tuning retained for serialized Blueprint compatibility. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Dodge", meta=(ClampMin="0.0"))
	float DodgeImpulseStrength = 920.0f;

	/** Deterministic unobstructed travel distance of one dodge roll. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Dodge", meta=(ClampMin="320.0", ClampMax="420.0", Units="cm"))
	float DodgeRollDistance = 380.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement", meta=(ClampMin="100.0", Units="cm/s"))
	float JogSpeed = 430.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement", meta=(ClampMin="100.0", Units="cm/s"))
	float SprintSpeed = 680.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement", meta=(ClampMin="0.0"))
	float SprintStaminaPerSecond = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Movement", meta=(ClampMin="0.05", Units="s"))
	float SprintHoldTime = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Dodge", meta=(ClampMin="0.0", Units="cm"))
	float RollVisualLift = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Dodge", meta=(ClampMin="40.0", Units="cm"))
	float RollCapsuleHalfHeight = 62.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Healing", meta=(ClampMin="0.2", Units="s"))
	float FlaskUseTime = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Healing", meta=(ClampMin="0.05", Units="s"))
	float FlaskHealDelay = 0.72f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation")
	TObjectPtr<UAnimMontage> HealMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|RootBlade")
	TObjectPtr<UAnimSequence> RootLightAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|RootBlade")
	TObjectPtr<UAnimSequence> RootHeavyAnim;

	/** Three quick attacks used in order when the player buffers light attack input. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Combo")
	TArray<TObjectPtr<UAnimSequence>> LightComboAnims;

	/** Alternate committed heavy swing, used every other heavy attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Combo")
	TObjectPtr<UAnimSequence> HeavyAltAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Combo", meta=(ClampMin="0.1", Units="s"))
	float ComboResetDelay = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Combo", meta=(ClampMin="0.01", Units="s"))
	float LightAttackHitDelay = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Combo", meta=(ClampMin="0.01", Units="s"))
	float HeavyAttackHitDelay = 0.30f;

	/** Only the final portion of a light attack accepts the next combo input. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Animation|Combo", meta=(ClampMin="0.05", ClampMax="0.5"))
	float LightComboBufferWindowFraction = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack|Hit Window", meta=(ClampMin="0.01", ClampMax="0.25", Units="s"))
	float LightAttackHitWindowDuration = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack|Hit Window", meta=(ClampMin="0.01", ClampMax="0.25", Units="s"))
	float HeavyAttackHitWindowDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack|Hit Stop", meta=(ClampMin="0.035", ClampMax="0.055", Units="s"))
	float HitStopDuration = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Attack|Hit Stop", meta=(ClampMin="0.01", ClampMax="1.0"))
	float HitStopTimeDilation = 0.05f;

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
	bool bShowCombatDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Debug")
	bool bDrawAttackTraceDebug = false;

	// 리스폰
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Respawn", meta=(ClampMin="0.0", Units="s"))
	float RespawnDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Respawn")
	TSubclassOf<ABRPlayerGraveMarker> PlayerGraveClass;

	// 락온
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="cm"))
	float LockOnRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="cm"))
	float LockOnBreakRange = 2000.0f;

	/** A short obstruction is tolerated so pillars and camera collision do not cause lock-on flicker. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", Units="s"))
	float LockOnOcclusionBreakDelay = 1.25f;

	/** Initial target selection favors actors near the center of the player's view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0"))
	float LockOnScreenCenterWeight = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0"))
	float LockOnCameraForwardWeight = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0"))
	float LockOnDistanceWeight = 0.15f;

	/** Horizontal look input above this value switches to the next target on that side. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.1", ClampMax="1.0"))
	float LockOnSwitchInputThreshold = 0.70f;

	/** The stick/mouse must return below this value before another switch can occur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|LockOn", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LockOnSwitchInputResetThreshold = 0.20f;

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
	float ExecRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="0.0", Units="cm"))
	float ExecGap = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="0.01", Units="s"))
	float ExecTime = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ExecDmgRate = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="0.05", Units="s"))
	float ExecHitTime = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(ClampMin="50.0", Units="cm"))
	float ExecCamLen = 230.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Execution", meta=(Units="cm"))
	FVector ExecCamSide = FVector(0.0f, 95.0f, 35.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Weapon", meta=(ClampMin="0.05", Units="s"))
	float SwingTime = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Weapon", meta=(Units="cm"))
	FVector BladeGrip = FVector(48.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Weapon", meta=(ClampMin="0.05"))
	float BladeSize = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Sound", meta=(ClampMin="50.0", Units="cm"))
	float StepGap = 135.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Sound", meta=(ClampMin="0.0", ClampMax="2.0"))
	float StepVol = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Sound", meta=(ClampMin="0.0", ClampMax="2.0"))
	float AttackVol = 0.75f;

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

	float LockOnOccludedTime = 0.0f;
	bool bLockOnSwitchInputReady = true;

	float LastStaminaSpendTime = -1000.0f;
	float LastAttackDebugTime = -1000.0f;

	// 타이머
	FTimerHandle StateTimerHandle;
	FTimerHandle InvincibleTimerHandle;
	FTimerHandle ParryTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTimerHandle ExecutionTimerHandle;
	FTimerHandle ExecHitTimer;
	FTimerHandle AttackHitTimerHandle;
	FTimerHandle HitStopTimerHandle;
	FTimerHandle SprintHoldTimerHandle;
	FTimerHandle FlaskHealTimerHandle;

	// 처형 대상
	UPROPERTY(Transient)
	TObjectPtr<ABRBossBase> PendingExecutionTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Weapon")
	bool bRootOn = false;
	bool bSwinging = false;
	bool bBigSwing = false;
	float SwingNow = 0.0f;
	float ExecDmg = 0.0f;
	float StepNow = 0.0f;
	bool bLeftStep = false;
	bool bLightComboQueued = false;
	int32 LightComboIndex = 0;
	int32 HeavyVariationIndex = 0;
	float LastLightComboTime = -1000.0f;
	float CurrentLightAttackStepDuration = 0.0f;
	float PendingAttackDamage = 0.0f;
	float PendingAttackGroggyDamage = 0.0f;
	float AttackHitWindowRemaining = 0.0f;
	bool bAttackHitWindowActive = false;
	bool bHitStopTriggeredThisAttack = false;
	TSet<TWeakObjectPtr<AActor>> DamagedActorsThisAttack;
	bool bHitStopActive = false;
	float SavedPlayerCustomTimeDilation = 1.0f;
	TMap<TWeakObjectPtr<AActor>, float> HitStopActorDilations;
	float RollNow = 0.0f;
	float RollTravelAlpha = 0.0f;
	float HealNow = 0.0f;
	float PendingHealAmount = 0.0f;
	float NormalGroundFriction = 8.0f;
	float NormalBrakingDeceleration = 2000.0f;
	float NormalCapsuleHalfHeight = 96.0f;
	bool bDodgeHeld = false;
	bool bSprintStartedThisHold = false;
	bool bSprinting = false;
	bool bRolling = false;
	bool bRollStartedLockedOn = false;
	bool bRollSavedOrientRotationToMovement = true;
	bool bRollSavedUseControllerDesiredRotation = false;
	float RollSavedGroundFriction = 8.0f;
	float RollSavedBrakingDeceleration = 2000.0f;
	float RollSavedCapsuleHalfHeight = 96.0f;
	bool bHealApplied = false;
	FVector RollDirection = FVector::ForwardVector;
	FVector BaseMeshRelativeLocation = FVector::ZeroVector;
	FRotator BaseMeshRelativeRotation = FRotator::ZeroRotator;
	FVector FlaskBaseLocation = FVector::ZeroVector;
	FRotator FlaskBaseRotation = FRotator::ZeroRotator;
	FRotator BladeBaseR = FRotator::ZeroRotator;
	FRotator BladeBaseL = FRotator::ZeroRotator;

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

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeBossPlate1Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeBossPlate2Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeBossPlate3Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuntimeUseFlaskAction;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> HendelMaterials;

public:

	/** Constructor */
	AExceptionCharacter();	

	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	void LookInputCompleted();

	void LightAttackPressed();
	void HeavyAttackPressed();
	void DodgePressed();
	void DodgeReleased();
	void ParryPressed();
	void InteractPressed();
	void LockOnPressed();
	void UseFlaskPressed();
	void BossPlate1Pressed();
	void BossPlate2Pressed();
	void BossPlate3Pressed();
	void ActivateBossPlateByIndex(int32 PlateIndex);
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
	void HealHP(float Amount);

	UFUNCTION(BlueprintCallable, Category="Exception|Stats")
	void RestoreStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category="Exception|Stats")
	void ApplySavedStats(float SavedHP, float SavedStamina);

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	void ApplySavedProgression(int32 SavedPlayerLevel, int32 SavedUpgradePoints, int32 SavedVitalityLevel, int32 SavedEnduranceLevel, int32 SavedPowerLevel);

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	void ApplySavedExperience(int32 SavedCurrentExperience, int32 SavedDroppedExperience);

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
	int32 GetCurrentExperience() const { return CurrentExperience; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetDroppedExperience() const { return DroppedExperience; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetLevelUpExperienceCost() const;

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetVitalityLevel() const { return VitalityLevel; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetEnduranceLevel() const { return EnduranceLevel; }

	UFUNCTION(BlueprintPure, Category="Exception|Progression")
	int32 GetPowerLevel() const { return PowerLevel; }

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	void AddUpgradePoints(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	void AddExperience(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	int32 DropCurrentExperience();

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	void RecoverDroppedExperience(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	bool SpendUpgradePoint(EBRPlayerUpgradeStat UpgradeStat);

	UFUNCTION(BlueprintCallable, Category="Exception|Progression")
	void AwardBossVictoryRewards(AActor* DefeatedBoss);

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	void GrantDefaultLoadout();

	UFUNCTION(BlueprintCallable, Category="Exception|Inventory")
	bool HasInventoryItem(FName ItemId) const;

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void CompleteNelHiddenRequest(FName RequestId);

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void CollectHiddenFragment(int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category="Exception|Hidden Story")
	void RefreshHiddenStoryRewards();

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
	void SaveBaseStats();
	void ApplyLevelStats();
	void DrawCombatDebug() const;
	FString GetCombatStateName() const;
	void RegisterInitialCheckpoint();
	void SpawnPlayerGraveMarker();
	AActor* FindLockOnTarget() const;
	AActor* FindLockOnTargetInDirection(float ScreenDirection) const;
	FVector GetLockOnFocusLocation(const AActor* Target) const;
	bool IsLockOnCandidateValid(AActor* Candidate, float MaxRange, bool bRequireLineOfSight) const;
	bool HasLockOnLineOfSight(AActor* Candidate) const;
	void SwitchLockOnTarget(float ScreenDirection);
	void UpdateLockOn(float DeltaSeconds);
	ABRBossBase* FindExecutionTarget() const;
	void StartExecution(ABRBossBase* Target);
	void DoExecHit();
	void FinishExecution();
	void UpdateExecCam(float DeltaSeconds);
	void ResetExecCam();
	void SetRootWeapon(bool bOn);
	void PlayRootAnim(bool bHeavy);
	void PlayAttackSequence(UAnimSequence* Anim, UAnimMontage* FallbackMontage, float Rate);
	bool CanBufferLightComboInput() const;
	bool StartLightComboStep();
	void FinishLightComboStep();
	void ApplyPendingAttackHit();
	void BeginAttackHitWindow();
	void UpdateAttackHitWindow(float DeltaSeconds);
	void EndAttackHitWindow();
	void StartHitStop(AActor* HitActor);
	void ClearHitStop();
	void CancelAttackChain();
	void StartRootSwing(bool bHeavy);
	void UpdateRootSwing(float DeltaSeconds);
	void UpdateStepSfx(float DeltaSeconds);
	void PlayStepSfx();
	void PlaySwingSfx(bool bHeavy);
	void PlayHitSfx();
	void PlayHealSfx();
	void ApplyHendelAppearance();
	void UpdateHendelAppearance();
	void BeginSprintIfHeld();
	void StopSprint();
	void UpdateSprint(float DeltaSeconds);
	void StartDodgeRoll(const FVector& Direction);
	void UpdateDodgeRoll(float DeltaSeconds);
	void EndDodgeRoll();
	bool BeginFlaskHeal(float HealAmount);
	void ApplyFlaskHeal();
	void FinishFlaskHeal();
	void UpdateFlaskHeal(float DeltaSeconds);
	void CancelFlaskHeal();
	float GetEffectiveAttackDamage(float BaseDamage, AActor* TargetActor) const;

	float BaseMaxHP = 0.0f;
	float BaseMaxStamina = 0.0f;
	float BaseLightDamage = 0.0f;
	float BaseHeavyDamage = 0.0f;
	bool bBaseStatsSaved = false;

	bool TryUseInventoryItem(int32 SlotIndex, const FBRInventorySlot& Slot);

	FBRInventoryItemDefinition MakePotionItem() const;
	FBRInventoryItemDefinition MakeStaminaItem() const;
	FBRInventoryItemDefinition MakeHiddenRootWeaponItem() const;

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
