// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "Combat/BRBossDamageType.h"

#include "BRCombatInterface.h"
#include "BRPlayerGraveMarker.h"
#include "Boss/Base/BRBossBase.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

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
	PlayAttackSequence(AttackAnim, LightAttackMontage.Get(), PlayRates[TuneIndex]);
	StartRootSwing(false);
	PlaySwingSfx(false);

	PendingAttackDamage = LightAttackDamage * DamageRates[TuneIndex];
	PendingAttackGroggyDamage = LightAttackGroggyDamage * GroggyRates[TuneIndex];
	const float HitDelay = LightAttackHitDelay + static_cast<float>(TuneIndex) * 0.035f;
	GetWorldTimerManager().SetTimer(AttackHitTimerHandle, this, &AExceptionCharacter::ApplyPendingAttackHit, HitDelay, false);

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
	const bool bAltHeavy = (HeavyVariationIndex++ % 2) == 1 && HeavyAltAnim != nullptr;
	UAnimSequence* AttackAnim = bAltHeavy ? HeavyAltAnim.Get() : RootHeavyAnim.Get();
	PlayAttackSequence(AttackAnim, HeavyAttackMontage.Get(), bAltHeavy ? 1.12f : 1.34f);
	StartRootSwing(true);
	PlaySwingSfx(true);
	PendingAttackDamage = HeavyAttackDamage * (bAltHeavy ? 1.18f : 1.0f);
	PendingAttackGroggyDamage = HeavyAttackGroggyDamage * (bAltHeavy ? 1.30f : 1.0f);
	GetWorldTimerManager().SetTimer(
		AttackHitTimerHandle,
		this,
		&AExceptionCharacter::ApplyPendingAttackHit,
		HeavyAttackHitDelay * (bAltHeavy ? 1.20f : 1.0f),
		false);

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

	AttackHitWindowRemaining -= DeltaSeconds;
	if (AttackHitWindowRemaining <= 0.0f)
	{
		EndAttackHitWindow();
		return;
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

	SetCombatState(EBRPlayerCombatState::Dodge);
	PlayOptionalMontage(DodgeMontage);
	UE_LOG(LogTemplateCharacter, Log, TEXT("DodgeRoll: Invincible %.2fs / Roll %.2fs"), DodgeInvincibleDuration, DodgeDuration);

	bIsInvincible = true;
	const FVector DodgeDirection = GetLastMovementInputVector().IsNearlyZero()
		? GetActorForwardVector()
		: GetLastMovementInputVector().GetSafeNormal();
	StartDodgeRoll(DodgeDirection);

	GetWorldTimerManager().SetTimer(InvincibleTimerHandle, this, &AExceptionCharacter::EndInvincibility, DodgeInvincibleDuration, false);
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, DodgeDuration, false);
	return true;
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

	SetActorRotation(RollDirection.Rotation());
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

	const float TurnAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 1.65f);
	if (GetMesh())
	{
		GetMesh()->SetRelativeRotation(FRotator(
			BaseMeshRelativeRotation.Pitch - 360.0f * TurnAlpha,
			BaseMeshRelativeRotation.Yaw,
			BaseMeshRelativeRotation.Roll));
		GetMesh()->SetRelativeLocation(BaseMeshRelativeLocation + FVector(0.0f, 0.0f, FMath::Sin(Alpha * PI) * RollVisualLift));
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
	PlayOptionalMontage(ParryMontage);
	UE_LOG(LogTemplateCharacter, Log, TEXT("Parry: Active %.2fs"), ParryActiveDuration);

	bIsParryActive = true;
	BP_ParryWindowStarted();

	GetWorldTimerManager().SetTimer(ParryTimerHandle, this, &AExceptionCharacter::EndParryWindow, ParryActiveDuration, false);
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
		PlayHitSfx();
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
	PlayOptionalMontage(HitMontage);

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
	}
	if ((PreviousState == EBRPlayerCombatState::LightAttack || PreviousState == EBRPlayerCombatState::HeavyAttack)
		&& NewState != PreviousState)
	{
		EndAttackHitWindow();
		DamagedActorsThisAttack.Reset();
		if (PreviousState == EBRPlayerCombatState::LightAttack)
		{
			CurrentLightAttackStepDuration = 0.0f;
		}
	}
	if (PreviousState == EBRPlayerCombatState::Healing && NewState != EBRPlayerCombatState::Healing)
	{
		CancelFlaskHeal();
	}
	if (NewState != EBRPlayerCombatState::Idle)
	{
		StopSprint();
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
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
}

void AExceptionCharacter::EndParryWindow()
{
	if (!bIsParryActive)
	{
		return;
	}

	bIsParryActive = false;
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	BP_ParryWindowEnded();
}

void AExceptionCharacter::PlayOptionalMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->Montage_Play(Montage);
	}
}
