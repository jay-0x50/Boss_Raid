// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "Combat/BRBossDamageType.h"

#include "BRCombatInterface.h"
#include "BRPlayerGraveMarker.h"
#include "Boss/Base/BRBossBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

bool AExceptionCharacter::AnimationUsesWindow(const UAnimSequenceBase* Animation, EBRPlayerAnimWindow Window) const
{
	if (!Animation)
	{
		return false;
	}

	for (const FAnimNotifyEvent& NotifyEvent : Animation->Notifies)
	{
		const UBRPlayerAnimNotifyState* PlayerNotify = Cast<UBRPlayerAnimNotifyState>(NotifyEvent.NotifyStateClass);
		if (PlayerNotify && PlayerNotify->Window == Window)
		{
			return true;
		}
	}
	return false;
}

bool AExceptionCharacter::AnimationUsesEvent(const UAnimSequenceBase* Animation, EBRPlayerAnimEvent Event) const
{
	if (!Animation)
	{
		return false;
	}

	for (const FAnimNotifyEvent& NotifyEvent : Animation->Notifies)
	{
		const UBRPlayerAnimNotify* PlayerNotify = Cast<UBRPlayerAnimNotify>(NotifyEvent.Notify);
		if (PlayerNotify && PlayerNotify->Event == Event)
		{
			return true;
		}
	}
	return false;
}

bool AExceptionCharacter::DoLightAttack()
{
	if (CombatState == EBRPlayerCombatState::LightAttack)
	{
		if (CanBufferLightComboInput())
		{
			bLightComboQueued = true;
			return true;
		}

		return false;
	}

	if (!CanStartCombatAction())
	{
		return false;
	}

	if (const UWorld* World = GetWorld(); World && World->GetTimeSeconds() - LastLightComboTime > ComboResetDelay)
	{
		LightComboIndex = 0;
	}

	return StartLightComboStep();
}

bool AExceptionCharacter::CanBufferLightComboInput() const
{
	const int32 ComboCount = FMath::Max(1, LightComboAnims.Num());
	if (CombatState != EBRPlayerCombatState::LightAttack || LightComboIndex + 1 >= ComboCount
		|| CurrentLightAttackStepDuration <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	if (bComboWindowUsesNotify)
	{
		return bComboInputWindowActive;
	}

	const float RemainingTime = GetWorldTimerManager().GetTimerRemaining(StateTimerHandle);
	if (RemainingTime < 0.0f)
	{
		return false;
	}

	const float Progress = 1.0f - FMath::Clamp(RemainingTime / CurrentLightAttackStepDuration, 0.0f, 1.0f);
	return Progress >= 1.0f - LightComboBufferWindowFraction;
}

bool AExceptionCharacter::StartLightComboStep()
{
	GetWorldTimerManager().ClearTimer(AttackHitTimerHandle);
	EndAttackHitWindow();
	DamagedActorsThisAttack.Reset();
	bHitStopTriggeredThisAttack = false;
	bComboWindowUsesNotify = false;
	bComboInputWindowActive = false;
	bRootMotionLocked = false;

	if (!SpendStamina(LightAttackStaminaCost))
	{
		return false;
	}

	const int32 ComboCount = FMath::Max(1, LightComboAnims.Num());
	LightComboIndex = FMath::Clamp(LightComboIndex, 0, ComboCount - 1);
	bLightComboQueued = false;
	SetCombatState(EBRPlayerCombatState::LightAttack);

	UAnimSequence* AttackAnim = LightComboAnims.IsValidIndex(LightComboIndex) ? LightComboAnims[LightComboIndex].Get() : RootLightAnim.Get();
	const float PlayRates[] = {1.55f, 1.42f, 1.28f};
	const float DurationRates[] = {0.92f, 1.04f, 1.24f};
	const float DamageRates[] = {1.0f, 1.12f, 1.35f};
	const float GroggyRates[] = {1.0f, 1.15f, 1.45f};
	const int32 TuneIndex = FMath::Clamp(LightComboIndex, 0, 2);
	PendingAttackDamage = LightAttackDamage * DamageRates[TuneIndex];
	PendingAttackGroggyDamage = LightAttackGroggyDamage * GroggyRates[TuneIndex];
	const bool bAnimationStarted = PlayAttackSequence(AttackAnim, LightAttackMontage.Get(), PlayRates[TuneIndex]);
	StartRootSwing(false);

	const bool bAttackWindowBeganDuringPlay = bAttackWindowUsesNotify;
	const bool bComboWindowBeganDuringPlay = bComboWindowUsesNotify;
	bAttackWindowUsesNotify = bAttackWindowBeganDuringPlay || (bAnimationStarted && AnimationUsesWindow(
		AttackAnim ? static_cast<UAnimSequenceBase*>(AttackAnim) : static_cast<UAnimSequenceBase*>(LightAttackMontage.Get()),
		EBRPlayerAnimWindow::AttackTrace));
	bComboWindowUsesNotify = bComboWindowBeganDuringPlay || (bAnimationStarted && AnimationUsesWindow(
		AttackAnim ? static_cast<UAnimSequenceBase*>(AttackAnim) : static_cast<UAnimSequenceBase*>(LightAttackMontage.Get()),
		EBRPlayerAnimWindow::ComboInput));
	if (!bComboWindowBeganDuringPlay)
	{
		bComboInputWindowActive = false;
	}
	if (!bAttackWindowUsesNotify)
	{
		const float HitDelay = LightAttackHitDelay + static_cast<float>(TuneIndex) * 0.035f;
		GetWorldTimerManager().SetTimer(AttackHitTimerHandle, this, &AExceptionCharacter::ApplyPendingAttackHit, HitDelay, false);
		HandleAnimationEvent(EBRPlayerAnimEvent::LightWeaponSwing);
	}

	CurrentLightAttackStepDuration = LightAttackDuration * DurationRates[TuneIndex];
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishLightComboStep, CurrentLightAttackStepDuration, false);
	return true;
}

void AExceptionCharacter::FinishLightComboStep()
{
	LastLightComboTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastLightComboTime;
	const int32 ComboCount = FMath::Max(1, LightComboAnims.Num());
	const bool bCanContinue = bLightComboQueued && LightComboIndex + 1 < ComboCount;
	if (bCanContinue)
	{
		++LightComboIndex;
		if (StartLightComboStep())
		{
			return;
		}
	}

	LightComboIndex = 0;
	bLightComboQueued = false;
	CurrentLightAttackStepDuration = 0.0f;
	EndAttackHitWindow();
	SetCombatState(EBRPlayerCombatState::Idle);
}

bool AExceptionCharacter::DoHeavyAttack()
{
	if (!CanStartCombatAction() || !SpendStamina(HeavyAttackStaminaCost))
	{
		return false;
	}

	SetCombatState(EBRPlayerCombatState::HeavyAttack);
	GetWorldTimerManager().ClearTimer(AttackHitTimerHandle);
	EndAttackHitWindow();
	DamagedActorsThisAttack.Reset();
	bHitStopTriggeredThisAttack = false;
	bComboWindowUsesNotify = false;
	bComboInputWindowActive = false;
	bRootMotionLocked = false;
	const bool bAltHeavy = (HeavyVariationIndex++ % 2) == 1 && HeavyAltAnim != nullptr;
	UAnimSequence* AttackAnim = bAltHeavy ? HeavyAltAnim.Get() : RootHeavyAnim.Get();
	PendingAttackDamage = HeavyAttackDamage * (bAltHeavy ? 1.18f : 1.0f);
	PendingAttackGroggyDamage = HeavyAttackGroggyDamage * (bAltHeavy ? 1.30f : 1.0f);
	const bool bAnimationStarted = PlayAttackSequence(AttackAnim, HeavyAttackMontage.Get(), bAltHeavy ? 1.12f : 1.34f);
	StartRootSwing(true);
	const bool bAttackWindowBeganDuringPlay = bAttackWindowUsesNotify;
	bAttackWindowUsesNotify = bAttackWindowBeganDuringPlay || (bAnimationStarted && AnimationUsesWindow(
		AttackAnim ? static_cast<UAnimSequenceBase*>(AttackAnim) : static_cast<UAnimSequenceBase*>(HeavyAttackMontage.Get()),
		EBRPlayerAnimWindow::AttackTrace));
	if (!bAttackWindowUsesNotify)
	{
		GetWorldTimerManager().SetTimer(
			AttackHitTimerHandle,
			this,
			&AExceptionCharacter::ApplyPendingAttackHit,
			HeavyAttackHitDelay * (bAltHeavy ? 1.20f : 1.0f),
			false);
		HandleAnimationEvent(EBRPlayerAnimEvent::HeavyWeaponSwing);
	}

	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, HeavyAttackDuration * (bAltHeavy ? 1.22f : 1.0f), false);
	return true;
}

void AExceptionCharacter::ApplyPendingAttackHit()
{
	if (CombatState != EBRPlayerCombatState::LightAttack && CombatState != EBRPlayerCombatState::HeavyAttack)
	{
		return;
	}

	BeginAttackHitWindow();
	UE_LOG(LogTemplateCharacter, Log, TEXT("AttackWindow: Combo=%d Damage=%.1f InitialHits=%d"), LightComboIndex + 1, PendingAttackDamage, LastAttackHitCount);
}

void AExceptionCharacter::BeginAttackHitWindow()
{
	if (CombatState != EBRPlayerCombatState::LightAttack && CombatState != EBRPlayerCombatState::HeavyAttack)
	{
		return;
	}

	bAttackHitWindowActive = true;
	AttackHitWindowRemaining = CombatState == EBRPlayerCombatState::LightAttack
		? LightAttackHitWindowDuration
		: HeavyAttackHitWindowDuration;
	PerformAttackTrace(PendingAttackDamage, PendingAttackGroggyDamage);
}

void AExceptionCharacter::UpdateAttackHitWindow(float DeltaSeconds)
{
	if (!bAttackHitWindowActive)
	{
		return;
	}

	if (CombatState != EBRPlayerCombatState::LightAttack && CombatState != EBRPlayerCombatState::HeavyAttack)
	{
		EndAttackHitWindow();
		return;
	}

	// Authored notify states own their exact end time. Legacy assets without the
	// state continue to use the configured timer as a safe fallback.
	if (!bAttackWindowUsesNotify)
	{
		AttackHitWindowRemaining -= DeltaSeconds;
		if (AttackHitWindowRemaining <= 0.0f)
		{
			EndAttackHitWindow();
			return;
		}
	}

	PerformAttackTrace(PendingAttackDamage, PendingAttackGroggyDamage);
}

void AExceptionCharacter::EndAttackHitWindow()
{
	bAttackHitWindowActive = false;
	AttackHitWindowRemaining = 0.0f;
}

void AExceptionCharacter::CancelAttackChain()
{
	GetWorldTimerManager().ClearTimer(AttackHitTimerHandle);
	EndAttackHitWindow();
	ClearHitStop();
	DamagedActorsThisAttack.Reset();
	bHitStopTriggeredThisAttack = false;
	bAttackWindowUsesNotify = false;
	bComboWindowUsesNotify = false;
	bComboInputWindowActive = false;
	bRootMotionLocked = false;
	bLightComboQueued = false;
	LightComboIndex = 0;
	CurrentLightAttackStepDuration = 0.0f;
}

bool AExceptionCharacter::DoDodge()
{
	if (bRolling || !CanStartCombatAction() || !SpendStamina(DodgeStaminaCost))
	{
		return false;
	}

	const FVector DodgeDirection = GetLastMovementInputVector().IsNearlyZero()
		? GetActorForwardVector()
		: GetLastMovementInputVector().GetSafeNormal();
	SetCombatState(EBRPlayerCombatState::Dodge);
	UAnimMontage* ActiveDodgeMontage = SelectDodgeMontage(DodgeDirection);
	const float DodgePlayRate = ActiveDodgeMontage && DodgeDuration > KINDA_SMALL_NUMBER
		? ActiveDodgeMontage->GetPlayLength() / DodgeDuration
		: 1.0f;
	// Dodging is safe from the frame the action is accepted. A zero-time
	// AnimNotifyState may not dispatch until the first animation update, so do
	// not leave an input-to-notify vulnerability. The authored notify still owns
	// the exact end; the legacy timer is retained only when no notify is present.
	bIsInvincible = true;
	bInvincibilityUsesNotify = false;
	const bool bAnimationStarted = PlayOptionalMontage(ActiveDodgeMontage, DodgePlayRate);
	const bool bWindowBeganDuringPlay = bInvincibilityUsesNotify;
	bInvincibilityUsesNotify = bWindowBeganDuringPlay
		|| (bAnimationStarted && AnimationUsesWindow(ActiveDodgeMontage, EBRPlayerAnimWindow::Invincibility));
	HandleAnimationEvent(EBRPlayerAnimEvent::Dodge);
	UE_LOG(LogTemplateCharacter, Log, TEXT("DodgeRoll: Invincible %.2fs / Roll %.2fs / Notify=%s"),
		DodgeInvincibleDuration,
		DodgeDuration,
		bInvincibilityUsesNotify ? TEXT("true") : TEXT("false"));

	StartDodgeRoll(DodgeDirection);

	if (!bInvincibilityUsesNotify)
	{
		GetWorldTimerManager().SetTimer(InvincibleTimerHandle, this, &AExceptionCharacter::EndInvincibility, DodgeInvincibleDuration, false);
	}
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, DodgeDuration, false);
	return true;
}

UAnimMontage* AExceptionCharacter::SelectDodgeMontage(const FVector& WorldDirection) const
{
	if (!bIsLockedOn)
	{
		return DodgeForwardMontage ? DodgeForwardMontage.Get() : DodgeMontage.Get();
	}

	const FVector SafeDirection = WorldDirection.GetSafeNormal2D();
	const float ForwardAmount = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), SafeDirection);
	const float RightAmount = FVector::DotProduct(GetActorRightVector().GetSafeNormal2D(), SafeDirection);
	UAnimMontage* DirectionalMontage = nullptr;
	if (FMath::Abs(ForwardAmount) >= FMath::Abs(RightAmount))
	{
		DirectionalMontage = ForwardAmount >= 0.0f ? DodgeForwardMontage.Get() : DodgeBackMontage.Get();
	}
	else
	{
		DirectionalMontage = RightAmount >= 0.0f ? DodgeRightMontage.Get() : DodgeLeftMontage.Get();
	}
	return DirectionalMontage ? DirectionalMontage : DodgeMontage.Get();
}

void AExceptionCharacter::BeginSprintIfHeld()
{
	if (!bDodgeHeld || bSprinting || CombatState != EBRPlayerCombatState::Idle
		|| GetCharacterMovement()->IsFalling() || CurrentStamina <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	bSprintStartedThisHold = true;
	bSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AExceptionCharacter::StopSprint()
{
	bSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = JogSpeed;
}

void AExceptionCharacter::UpdateSprint(float DeltaSeconds)
{
	if (!bSprinting)
	{
		return;
	}

	if (!bDodgeHeld || CombatState != EBRPlayerCombatState::Idle || GetCharacterMovement()->IsFalling())
	{
		StopSprint();
		return;
	}

	if (GetVelocity().SizeSquared2D() < FMath::Square(70.0f))
	{
		return;
	}

	CurrentStamina = FMath::Max(0.0f, CurrentStamina - SprintStaminaPerSecond * DeltaSeconds);
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastStaminaSpendTime;
	BroadcastStamina();
	if (CurrentStamina <= KINDA_SMALL_NUMBER)
	{
		StopSprint();
	}
}

void AExceptionCharacter::StartDodgeRoll(const FVector& Direction)
{
	StopSprint();
	bRolling = true;
	bRollStartedLockedOn = bIsLockedOn;
	RollNow = 0.0f;
	RollTravelAlpha = 0.0f;
	RollDirection = Direction.GetSafeNormal2D();
	if (RollDirection.IsNearlyZero())
	{
		RollDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	// Free movement turns into the roll. Lock-on keeps the target-facing yaw so
	// the authored forward/back/left/right montages remain directionally legible.
	if (!bIsLockedOn)
	{
		SetActorRotation(RollDirection.Rotation());
	}
	if (GetMesh())
	{
		BaseMeshRelativeLocation = GetMesh()->GetRelativeLocation();
		BaseMeshRelativeRotation = GetMesh()->GetRelativeRotation();
	}

	RollSavedCapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	GetCapsuleComponent()->SetCapsuleHalfHeight(FMath::Min(RollCapsuleHalfHeight, RollSavedCapsuleHalfHeight), true);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		RollSavedGroundFriction = Movement->GroundFriction;
		RollSavedBrakingDeceleration = Movement->BrakingDecelerationWalking;
		bRollSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
		bRollSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
		Movement->StopMovementImmediately();
		Movement->GroundFriction = 0.0f;
		Movement->BrakingDecelerationWalking = 0.0f;
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = false;
	}
}

void AExceptionCharacter::UpdateDodgeRoll(float DeltaSeconds)
{
	if (!bRolling)
	{
		return;
	}

	RollNow += DeltaSeconds;
	const float Alpha = FMath::Clamp(RollNow / FMath::Max(DodgeDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float TravelAlpha = 0.5f - 0.5f * FMath::Cos(Alpha * PI);
	const float TravelDelta = FMath::Max(0.0f, TravelAlpha - RollTravelAlpha) * DodgeRollDistance;
	RollTravelAlpha = TravelAlpha;

	if (TravelDelta > KINDA_SMALL_NUMBER)
	{
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			const FVector MoveDelta = RollDirection * TravelDelta;
			FHitResult Hit;
			Movement->SafeMoveUpdatedComponent(MoveDelta, GetActorQuat(), true, Hit);
			if (Hit.IsValidBlockingHit())
			{
				// CharacterMovement's SlideAlongSurface override is protected. Reproduce
				// the small remaining tangent move through the public swept-move API.
				const FVector RemainingDelta = MoveDelta * FMath::Max(0.0f, 1.0f - Hit.Time);
				const FVector SlideDelta = FVector::VectorPlaneProject(RemainingDelta, Hit.Normal);
				if (!SlideDelta.IsNearlyZero())
				{
					FHitResult SlideHit;
					Movement->SafeMoveUpdatedComponent(SlideDelta, GetActorQuat(), true, SlideHit);
				}
			}
			Movement->Velocity = FVector::ZeroVector;
		}
	}

	if (Alpha >= 1.0f)
	{
		EndDodgeRoll();
	}
}

void AExceptionCharacter::EndDodgeRoll()
{
	if (!bRolling)
	{
		return;
	}

	bRolling = false;
	RollNow = 0.0f;
	RollTravelAlpha = 0.0f;
	if (GetMesh())
	{
		GetMesh()->SetRelativeLocation(BaseMeshRelativeLocation);
		GetMesh()->SetRelativeRotation(BaseMeshRelativeRotation);
	}
	GetCapsuleComponent()->SetCapsuleHalfHeight(RollSavedCapsuleHalfHeight, true);
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GroundFriction = RollSavedGroundFriction;
		Movement->BrakingDecelerationWalking = RollSavedBrakingDeceleration;
		Movement->bOrientRotationToMovement = bRollStartedLockedOn == bIsLockedOn
			? bRollSavedOrientRotationToMovement
			: !bIsLockedOn;
		Movement->bUseControllerDesiredRotation = bRollSavedUseControllerDesiredRotation;
		Movement->StopMovementImmediately();
		Movement->MaxWalkSpeed = JogSpeed;
	}
}

bool AExceptionCharacter::DoParry()
{
	if (!CanStartCombatAction() || !SpendStamina(ParryStaminaCost))
	{
		return false;
	}

	SetCombatState(EBRPlayerCombatState::Parry);
	// Mirror dodge's frame-zero safety. NotifyBegin is idempotent and NotifyEnd
	// remains the precise authored close when the montage contains the window.
	bIsParryActive = true;
	bParryWindowUsesNotify = false;
	BP_ParryWindowStarted();
	const bool bAnimationStarted = PlayOptionalMontage(ParryMontage);
	const bool bWindowBeganDuringPlay = bParryWindowUsesNotify;
	bParryWindowUsesNotify = bWindowBeganDuringPlay
		|| (bAnimationStarted && AnimationUsesWindow(ParryMontage, EBRPlayerAnimWindow::Parry));
	HandleAnimationEvent(EBRPlayerAnimEvent::ParryAttempt);
	UE_LOG(LogTemplateCharacter, Log, TEXT("Parry: Active %.2fs / Notify=%s"),
		ParryActiveDuration,
		bParryWindowUsesNotify ? TEXT("true") : TEXT("false"));

	if (!bParryWindowUsesNotify)
	{
		GetWorldTimerManager().SetTimer(ParryTimerHandle, this, &AExceptionCharacter::EndParryWindow, ParryActiveDuration, false);
	}
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, ParryDuration, false);
	return true;
}

void AExceptionCharacter::DoInteract()
{
	TryExecution();
}

void AExceptionCharacter::PerformAttackTrace(float Damage, float GroggyDamage)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	LastAttackHitCount = 0;
	LastAttackDebugTime = World->GetTimeSeconds();

	const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * AttackTraceDistance);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ExceptionPlayerAttackTrace), false, this);
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> Hits;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(AttackTraceRadius);
	const bool bHit = World->SweepMultiByObjectType(Hits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, Shape, QueryParams);

	if (bDrawAttackTraceDebug)
	{
		DrawDebugLine(World, TraceStart, TraceEnd, bHit ? FColor::Green : FColor::Red, false, 1.0f, 0, 2.0f);
		DrawDebugSphere(World, TraceEnd, AttackTraceRadius, 16, bHit ? FColor::Green : FColor::Red, false, 1.0f);
	}

	if (!bHit)
	{
		return;
	}

	AActor* HitStopTarget = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		const TWeakObjectPtr<AActor> HitActorPtr(HitActor);
		if (!HitActor || DamagedActorsThisAttack.Contains(HitActorPtr))
		{
			continue;
		}

		const float EffectiveDamage = GetEffectiveAttackDamage(Damage, HitActor);
		bool bDamageApplied = false;
		if (HitActor->GetClass()->ImplementsInterface(UBRCombatInterface::StaticClass()))
		{
			bDamageApplied = IBRCombatInterface::Execute_ReceiveCombatHit(HitActor, EffectiveDamage, GroggyDamage, this);
		}
		else
		{
			bDamageApplied = UGameplayStatics::ApplyDamage(
				HitActor,
				EffectiveDamage,
				GetController(),
				this,
				UDamageType::StaticClass()) > 0.0f;
		}
		if (!bDamageApplied)
		{
			continue;
		}

		DamagedActorsThisAttack.Add(HitActorPtr);
		if (!HitStopTarget)
		{
			HitStopTarget = HitActor;
		}
		BP_AttackHit(HitActor, EffectiveDamage);
		++LastAttackHitCount;

		if (bDrawAttackTraceDebug)
		{
			DrawDebugSphere(World, Hit.ImpactPoint, 18.0f, 12, FColor::Yellow, false, 1.0f);
		}
	}

	if (LastAttackHitCount > 0)
	{
		HandleAnimationEvent(EBRPlayerAnimEvent::HitVFX);
		HandleAnimationEvent(EBRPlayerAnimEvent::HitSFX);
		if (!bHitStopTriggeredThisAttack)
		{
			bHitStopTriggeredThisAttack = true;
			StartHitStop(HitStopTarget);
		}
	}
}

void AExceptionCharacter::StartHitStop(AActor* HitActor)
{
	if (!GetWorld() || HitStopDuration <= 0.0f)
	{
		return;
	}

	if (!bHitStopActive)
	{
		SavedPlayerCustomTimeDilation = CustomTimeDilation;
		bHitStopActive = true;
	}
	CustomTimeDilation = HitStopTimeDilation;

	if (IsValid(HitActor) && HitActor != this)
	{
		const TWeakObjectPtr<AActor> HitActorPtr(HitActor);
		if (!HitStopActorDilations.Contains(HitActorPtr))
		{
			HitStopActorDilations.Add(HitActorPtr, HitActor->CustomTimeDilation);
		}
		HitActor->CustomTimeDilation = HitStopTimeDilation;
	}

	GetWorldTimerManager().ClearTimer(HitStopTimerHandle);
	GetWorldTimerManager().SetTimer(HitStopTimerHandle, this, &AExceptionCharacter::ClearHitStop, HitStopDuration, false);
}

void AExceptionCharacter::ClearHitStop()
{
	GetWorldTimerManager().ClearTimer(HitStopTimerHandle);
	if (bHitStopActive)
	{
		CustomTimeDilation = SavedPlayerCustomTimeDilation;
	}

	for (const TPair<TWeakObjectPtr<AActor>, float>& Pair : HitStopActorDilations)
	{
		if (AActor* HitActor = Pair.Key.Get())
		{
			HitActor->CustomTimeDilation = Pair.Value;
		}
	}

	bHitStopActive = false;
	SavedPlayerCustomTimeDilation = 1.0f;
	HitStopActorDilations.Reset();
}

float AExceptionCharacter::GetEffectiveAttackDamage(float BaseDamage, AActor* TargetActor) const
{
	float Damage = BaseDamage;
	if (bRootOn)
	{
		Damage *= RootDmg;
		if (TargetActor && TargetActor->GetClass()->GetName().Contains(TEXT("CMD")))
		{
			Damage *= RootCmdDmg;
		}
	}

	return Damage;
}

float AExceptionCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (CombatState == EBRPlayerCombatState::Dead || bIsInvincible || Damage <= 0.0f)
	{
		return 0.0f;
	}

	const UBRBossDamageType* BossDamageType = DamageEvent.DamageTypeClass
		? Cast<UBRBossDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject())
		: nullptr;
	const bool bCanParryIncomingDamage = BossDamageType && BossDamageType->CanBeParried();
	if (bIsParryActive && bCanParryIncomingDamage)
	{
		EndParryWindow();
		PlayParrySuccessReaction();

		if (ABRBossBase* BossCauser = Cast<ABRBossBase>(DamageCauser))
		{
			BossCauser->ApplyGroggyDamage(ParrySuccessGroggyDamage, this);
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1004, 1.2f, FColor::Cyan, TEXT("Parry Success"));
		}

		return 0.0f;
	}

	CurrentHP = FMath::Max(0.0f, CurrentHP - Damage);
	BroadcastHP();
	BP_DamageReceived(Damage);

	if (CurrentHP <= 0.0f)
	{
		CancelAttackChain();
		GetWorldTimerManager().ClearTimer(StateTimerHandle);
		GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
		GetWorldTimerManager().ClearTimer(ParryTimerHandle);
		GetWorldTimerManager().ClearTimer(ExecutionTimerHandle);
		GetWorldTimerManager().ClearTimer(ExecHitTimer);
		ResetExecCam();
		ClearLockOn();
		SetCombatState(EBRPlayerCombatState::Dead);
		PlayDeathReaction(DamageCauser);
		HandleAnimationEvent(EBRPlayerAnimEvent::PlayerDeath);
		GetCharacterMovement()->DisableMovement();
		SpawnPlayerGraveMarker();
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AExceptionCharacter::RespawnAtCheckpoint, RespawnDelay, false);
		return Damage;
	}

	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	CancelAttackChain();
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	EndParryWindow();
	bIsInvincible = false;
	SetCombatState(EBRPlayerCombatState::Hit);
	const bool bHeavyHit = Damage >= HeavyHitDamageThreshold;
	PlayHitReaction(DamageCauser, bHeavyHit);
	HandleAnimationEvent(EBRPlayerAnimEvent::PlayerHit);

	if (DamageCauser)
	{
		const FVector AwayFromCauser = FVector(GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();
		const FVector KnockbackDirection = AwayFromCauser.IsNearlyZero() ? -GetActorForwardVector() : AwayFromCauser;
		LaunchCharacter(KnockbackDirection * HitKnockbackStrength, true, false);
	}

	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, HitStunDuration, false);
	return Damage;
}

bool AExceptionCharacter::CanStartCombatAction() const
{
	return CombatState == EBRPlayerCombatState::Idle && !GetCharacterMovement()->IsFalling();
}

void AExceptionCharacter::SetCombatState(EBRPlayerCombatState NewState)
{
	if (CombatState == NewState)
	{
		return;
	}

	const EBRPlayerCombatState PreviousState = CombatState;
	const bool bFinishedStaminaAction = PreviousState == EBRPlayerCombatState::LightAttack
		|| PreviousState == EBRPlayerCombatState::HeavyAttack
		|| PreviousState == EBRPlayerCombatState::Dodge
		|| PreviousState == EBRPlayerCombatState::Parry;
	if (bFinishedStaminaAction)
	{
		LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastStaminaSpendTime;
	}

	if (PreviousState == EBRPlayerCombatState::Dodge && NewState != EBRPlayerCombatState::Dodge)
	{
		EndDodgeRoll();
		bInvincibilityUsesNotify = false;
	}
	if ((PreviousState == EBRPlayerCombatState::LightAttack || PreviousState == EBRPlayerCombatState::HeavyAttack)
		&& NewState != PreviousState)
	{
		EndAttackHitWindow();
		bAttackWindowUsesNotify = false;
		bComboWindowUsesNotify = false;
		bComboInputWindowActive = false;
		bRootMotionLocked = false;
		DamagedActorsThisAttack.Reset();
		if (PreviousState == EBRPlayerCombatState::LightAttack)
		{
			CurrentLightAttackStepDuration = 0.0f;
		}
	}
	if (PreviousState == EBRPlayerCombatState::Healing && NewState != EBRPlayerCombatState::Healing)
	{
		CancelFlaskHeal();
		bHealUsesNotify = false;
	}
	if (PreviousState == EBRPlayerCombatState::Parry && NewState != EBRPlayerCombatState::Parry)
	{
		bParryWindowUsesNotify = false;
	}
	if (NewState != EBRPlayerCombatState::Idle)
	{
		StopSprint();
	}
	if (NewState == EBRPlayerCombatState::Idle || NewState == EBRPlayerCombatState::Hit
		|| NewState == EBRPlayerCombatState::Dead || NewState == EBRPlayerCombatState::Execution)
	{
		bComboWindowUsesNotify = false;
		bComboInputWindowActive = false;
		bRootMotionLocked = false;
	}
	CombatState = NewState;
	OnCombatStateChanged.Broadcast(CombatState);

	if (NewState == EBRPlayerCombatState::Idle)
	{
		BP_CombatActionEnded(PreviousState);
	}
	else
	{
		BP_CombatActionStarted(NewState);
	}
}

void AExceptionCharacter::FinishCombatAction()
{
	if (CombatState == EBRPlayerCombatState::Dead || CombatState == EBRPlayerCombatState::Execution)
	{
		return;
	}

	if (bIsInvincible)
	{
		EndInvincibility();
	}

	if (bIsParryActive)
	{
		EndParryWindow();
	}

	if (CombatState == EBRPlayerCombatState::LightAttack)
	{
		FinishLightComboStep();
		return;
	}

	bLightComboQueued = false;
	SetCombatState(EBRPlayerCombatState::Idle);
}

void AExceptionCharacter::EndInvincibility()
{
	bIsInvincible = false;
	bInvincibilityUsesNotify = false;
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
}

void AExceptionCharacter::EndParryWindow()
{
	if (!bIsParryActive)
	{
		return;
	}

	bIsParryActive = false;
	bParryWindowUsesNotify = false;
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	BP_ParryWindowEnded();
}

bool AExceptionCharacter::PlayOptionalMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage)
	{
		return false;
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		return AnimInstance->Montage_Play(Montage, FMath::Max(0.1f, PlayRate)) > 0.0f;
	}
	return false;
}

void AExceptionCharacter::BeginAnimationWindow(EBRPlayerAnimWindow Window)
{
	switch (Window)
	{
	case EBRPlayerAnimWindow::AttackTrace:
		if (CombatState == EBRPlayerCombatState::LightAttack || CombatState == EBRPlayerCombatState::HeavyAttack)
		{
			GetWorldTimerManager().ClearTimer(AttackHitTimerHandle);
			bAttackWindowUsesNotify = true;
			BeginAttackHitWindow();
		}
		break;
	case EBRPlayerAnimWindow::Invincibility:
		if (CombatState == EBRPlayerCombatState::Dodge)
		{
			GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
			bInvincibilityUsesNotify = true;
			bIsInvincible = true;
		}
		break;
	case EBRPlayerAnimWindow::Parry:
		if (CombatState == EBRPlayerCombatState::Parry)
		{
			GetWorldTimerManager().ClearTimer(ParryTimerHandle);
			bParryWindowUsesNotify = true;
			if (!bIsParryActive)
			{
				bIsParryActive = true;
				BP_ParryWindowStarted();
			}
		}
		break;
	case EBRPlayerAnimWindow::ComboInput:
		if (CombatState == EBRPlayerCombatState::LightAttack)
		{
			bComboWindowUsesNotify = true;
			bComboInputWindowActive = true;
		}
		break;
	case EBRPlayerAnimWindow::RootMotionLock:
		if (CombatState != EBRPlayerCombatState::Idle && CombatState != EBRPlayerCombatState::Dead)
		{
			bRootMotionLocked = true;
			GetCharacterMovement()->StopMovementImmediately();
		}
		break;
	default:
		break;
	}
}

void AExceptionCharacter::EndAnimationWindow(EBRPlayerAnimWindow Window)
{
	switch (Window)
	{
	case EBRPlayerAnimWindow::AttackTrace:
		EndAttackHitWindow();
		bAttackWindowUsesNotify = false;
		break;
	case EBRPlayerAnimWindow::Invincibility:
		if (CombatState == EBRPlayerCombatState::Dodge)
		{
			EndInvincibility();
		}
		break;
	case EBRPlayerAnimWindow::Parry:
		if (CombatState == EBRPlayerCombatState::Parry)
		{
			EndParryWindow();
		}
		break;
	case EBRPlayerAnimWindow::ComboInput:
		bComboInputWindowActive = false;
		break;
	case EBRPlayerAnimWindow::RootMotionLock:
		bRootMotionLocked = false;
		break;
	default:
		break;
	}
}

void AExceptionCharacter::HandleAnimationEvent(EBRPlayerAnimEvent Event)
{
	const UEnum* EventEnum = StaticEnum<EBRPlayerAnimEvent>();
	const FName EventName = EventEnum
		? FName(*EventEnum->GetNameStringByValue(static_cast<int64>(Event)))
		: NAME_None;
	BP_PlayerAnimationEvent(EventName);

	switch (Event)
	{
	case EBRPlayerAnimEvent::Footstep:
		PlayStepSfx();
		break;
	case EBRPlayerAnimEvent::LightWeaponSwing:
		PlaySwingSfx(false);
		break;
	case EBRPlayerAnimEvent::HeavyWeaponSwing:
		PlaySwingSfx(true);
		break;
	case EBRPlayerAnimEvent::Heal:
		GetWorldTimerManager().ClearTimer(FlaskHealTimerHandle);
		bHealUsesNotify = true;
		ApplyFlaskHeal();
		break;
	case EBRPlayerAnimEvent::HitVFX:
		break;
	case EBRPlayerAnimEvent::HitSFX:
		PlayHitSfx();
		break;
	case EBRPlayerAnimEvent::Dodge:
	case EBRPlayerAnimEvent::ParryAttempt:
	case EBRPlayerAnimEvent::ParrySuccess:
	case EBRPlayerAnimEvent::PlayerHit:
	case EBRPlayerAnimEvent::PlayerDeath:
		break;
	case EBRPlayerAnimEvent::ExecutionDamage:
		if (CombatState == EBRPlayerCombatState::Execution)
		{
			GetWorldTimerManager().ClearTimer(ExecHitTimer);
			bExecutionDamageUsesNotify = true;
			DoExecHit();
		}
		break;
	case EBRPlayerAnimEvent::FinishAction:
		FinishCombatAction();
		break;
	default:
		break;
	}
}

void AExceptionCharacter::PlayHitReaction(AActor* DamageCauser, bool bHeavyHit)
{
	UAnimSequence* Reaction = bHeavyHit ? HeavyKnockbackAnim.Get() : HitFrontAnim.Get();
	if (!bHeavyHit && DamageCauser)
	{
		const FVector ToCauser = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const float ForwardAmount = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToCauser);
		const float RightAmount = FVector::DotProduct(GetActorRightVector().GetSafeNormal2D(), ToCauser);
		if (FMath::Abs(RightAmount) > FMath::Abs(ForwardAmount))
		{
			Reaction = RightAmount >= 0.0f ? HitRightAnim.Get() : HitLeftAnim.Get();
		}
		else if (ForwardAmount < 0.0f)
		{
			Reaction = HitBackAnim.Get();
		}
	}

	PlayAttackSequence(Reaction, HitMontage.Get(), bHeavyHit ? 1.05f : 1.25f);
}

void AExceptionCharacter::PlayDeathReaction(AActor* DamageCauser)
{
	if (DamageCauser)
	{
		const FVector Away = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D();
		if (!Away.IsNearlyZero())
		{
			SetActorRotation((-Away).Rotation());
		}
	}
	PlayAttackSequence(DeathAnim.Get(), nullptr, 1.0f);
}

void AExceptionCharacter::PlayParrySuccessReaction()
{
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	PlayAttackSequence(ParrySuccessAnim.Get(), nullptr, 1.45f);
	HandleAnimationEvent(EBRPlayerAnimEvent::ParrySuccess);
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, 0.42f, false);
}
