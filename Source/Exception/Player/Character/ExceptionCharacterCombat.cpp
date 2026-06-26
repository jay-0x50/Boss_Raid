// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "BRCombatInterface.h"
#include "Boss/Base/BRBossBase.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

bool AExceptionCharacter::DoLightAttack()
{
	if (!CanStartCombatAction() || !SpendStamina(LightAttackStaminaCost))
	{
		return false;
	}

	SetCombatState(EBRPlayerCombatState::LightAttack);
	PlayOptionalMontage(LightAttackMontage);
	PerformAttackTrace(LightAttackDamage, LightAttackGroggyDamage);
	UE_LOG(LogTemplateCharacter, Log, TEXT("LightAttack: Damage=%.1f, HitCount=%d"), LightAttackDamage, LastAttackHitCount);

	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, LightAttackDuration, false);
	return true;
}

bool AExceptionCharacter::DoHeavyAttack()
{
	if (!CanStartCombatAction() || !SpendStamina(HeavyAttackStaminaCost))
	{
		return false;
	}

	SetCombatState(EBRPlayerCombatState::HeavyAttack);
	PlayOptionalMontage(HeavyAttackMontage);
	PerformAttackTrace(HeavyAttackDamage, HeavyAttackGroggyDamage);
	UE_LOG(LogTemplateCharacter, Log, TEXT("HeavyAttack: Damage=%.1f, HitCount=%d"), HeavyAttackDamage, LastAttackHitCount);

	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, HeavyAttackDuration, false);
	return true;
}

bool AExceptionCharacter::DoDodge()
{
	if (!CanStartCombatAction() || !SpendStamina(DodgeStaminaCost))
	{
		return false;
	}

	SetCombatState(EBRPlayerCombatState::Dodge);
	PlayOptionalMontage(DodgeMontage);
	UE_LOG(LogTemplateCharacter, Log, TEXT("Dodge: Invincible %.2fs"), DodgeInvincibleDuration);

	bIsInvincible = true;
	const FVector DodgeDirection = GetLastMovementInputVector().IsNearlyZero()
		? GetActorForwardVector()
		: GetLastMovementInputVector().GetSafeNormal();
	LaunchCharacter(DodgeDirection * DodgeImpulseStrength, true, false);

	GetWorldTimerManager().SetTimer(InvincibleTimerHandle, this, &AExceptionCharacter::EndInvincibility, DodgeInvincibleDuration, false);
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishCombatAction, DodgeDuration, false);
	return true;
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
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

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

	TSet<AActor*> DamagedActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);
		const float EffectiveDamage = GetEffectiveAttackDamage(Damage, HitActor);
		if (HitActor->GetClass()->ImplementsInterface(UBRCombatInterface::StaticClass()))
		{
			IBRCombatInterface::Execute_ReceiveCombatHit(HitActor, EffectiveDamage, GroggyDamage, this);
		}
		else
		{
			UGameplayStatics::ApplyDamage(HitActor, EffectiveDamage, GetController(), this, UDamageType::StaticClass());
		}
		BP_AttackHit(HitActor, EffectiveDamage);
		++LastAttackHitCount;

		if (bDrawAttackTraceDebug)
		{
			DrawDebugSphere(World, Hit.ImpactPoint, 18.0f, 12, FColor::Yellow, false, 1.0f);
		}
	}
}

float AExceptionCharacter::GetEffectiveAttackDamage(float BaseDamage, AActor* TargetActor) const
{
	float Damage = BaseDamage;
	if (HasInventoryItem(TEXT("Weapon_MimikatzAuthoritySeized")))
	{
		Damage *= HiddenRootWeaponDamageMultiplier;
		if (TargetActor && TargetActor->GetClass()->GetName().Contains(TEXT("CMD")))
		{
			Damage *= HiddenRootWeaponCMDDamageMultiplier;
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

	if (bIsParryActive)
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
		GetWorldTimerManager().ClearTimer(StateTimerHandle);
		GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
		GetWorldTimerManager().ClearTimer(ParryTimerHandle);
		GetWorldTimerManager().ClearTimer(ExecutionTimerHandle);
		ClearLockOn();
		SetCombatState(EBRPlayerCombatState::Dead);
		GetCharacterMovement()->DisableMovement();
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AExceptionCharacter::RespawnAtCheckpoint, RespawnDelay, false);
		return Damage;
	}

	GetWorldTimerManager().ClearTimer(StateTimerHandle);
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
