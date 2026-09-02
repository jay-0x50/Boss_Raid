#include "Boss/Pattern/BRPatternBossBase.h"

#include "Boss/AI/BRBossAIController.h"
#include "Combat/BRBossDamageType.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

int32 ABRPatternBossBase::PickAttack(float PlayerDist) const
{
	const int32 PatternCount = AttackPatterns.Num();
	for (int32 Offset = 1; Offset <= PatternCount; ++Offset)
	{
		const int32 Index = (LastPatternIndex + Offset) % PatternCount;
		if (CanUseAttack(AttackPatterns[Index], PlayerDist))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool ABRPatternBossBase::CanUseAttack(const FBRBossPatternData& Attack, float PlayerDist) const
{
	const UWorld* World = GetWorld();
	if (!World || bIsPhaseTransitioning || !IsValid(CurrentTarget))
	{
		return false;
	}

	const bool bPhaseEnabled = BossPhase == EBRBossPhase::Phase1 ? Attack.bEnableInPhase1 : Attack.bEnableInPhase2;
	if (!bPhaseEnabled || PlayerDist < Attack.MinRange || PlayerDist > Attack.MaxRange)
	{
		return false;
	}

	if (Attack.bRequiresTeamMateNear && !IsTeamMateWithin(Attack.TeamMateNearDistance))
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	if (!CanStartCoordinatedAttack() || Now < NextAttackTime)
	{
		return false;
	}

	const float* LastPatternTime = LastPatternTimes.Find(Attack.PatternName);
	return !LastPatternTime || Now - *LastPatternTime >= GetAttackCool(Attack);
}

float ABRPatternBossBase::GetAttackCool(const FBRBossPatternData& Attack) const
{
	const float PhaseMultiplier = BossPhase == EBRBossPhase::Phase2 ? Phase2CooldownMultiplier : 1.0f;
	const float EnrageMultiplier = bIsEnraged ? EnrageCooldownMultiplier : 1.0f;
	return Attack.Cooldown * PhaseMultiplier * EnrageMultiplier;
}

UNiagaraSystem* ABRPatternBossBase::ResolvePatternEffect(const FBRBossPatternData& Pattern, bool bTelegraph) const
{
	if (bTelegraph && Pattern.TelegraphEffect)
	{
		return Pattern.TelegraphEffect.Get();
	}

	if (!bTelegraph && Pattern.ImpactEffect)
	{
		return Pattern.ImpactEffect.Get();
	}

	switch (Pattern.PatternType)
	{
	case EBRBossPatternType::Melee:
		return bTelegraph && MeleeTelegraphEffect ? MeleeTelegraphEffect.Get() : MeleeEffect.Get();
	case EBRBossPatternType::Dash:
		return bTelegraph && DashTelegraphEffect ? DashTelegraphEffect.Get() : DashEffect.Get();
	case EBRBossPatternType::AOE:
		return bTelegraph && AOETelegraphEffect ? AOETelegraphEffect.Get() : AOEEffect.Get();
	default:
		return nullptr;
	}
}

FVector ABRPatternBossBase::GetAOECenter(const FBRBossPatternData& Pattern, float HeightOffset) const
{
	FVector Center = GetActorLocation();
	if (bHasActivePattern)
	{
		Center = Pattern.bCenterAOEOnTarget ? LockedTargetLocation : LockedAttackOrigin;
	}

	if (Pattern.bCenterAOEOnTarget)
	{
		if (const ACharacter* TargetCharacter = Cast<ACharacter>(CurrentTarget))
		{
			if (const UCapsuleComponent* TargetCapsule = TargetCharacter->GetCapsuleComponent())
			{
				Center.Z -= TargetCapsule->GetScaledCapsuleHalfHeight();
			}
		}
	}
	else
	{
		Center.Z -= GroundTraceActorHalfHeight;
	}

	Center.Z += HeightOffset;
	return Center;
}

FVector ABRPatternBossBase::GetLockedDashDirection(const FBRBossPatternData& Pattern) const
{
	const FVector BaseDirection = LockedAttackDirection.IsNearlyZero()
		? GetActorForwardVector().GetSafeNormal2D()
		: LockedAttackDirection.GetSafeNormal2D();
	return Pattern.bDashAwayFromTarget ? -BaseDirection : BaseDirection;
}

FTransform ABRPatternBossBase::GetPatternEffectTransform(
	const FBRBossPatternData& Pattern,
	float HeightOffset) const
{
	if (Pattern.PatternType == EBRBossPatternType::AOE)
	{
		return FTransform(GetActorRotation(), GetAOECenter(Pattern, HeightOffset));
	}

	const bool bIsRetreat = Pattern.PatternType == EBRBossPatternType::Dash && Pattern.bDashAwayFromTarget;
	const bool bUseCurrentDashLocation = Pattern.PatternType == EBRBossPatternType::Dash && bAttackHasImpacted && !bIsRetreat;
	FVector EffectOrigin = bHasActivePattern && !bUseCurrentDashLocation ? LockedAttackOrigin : GetActorLocation();
	EffectOrigin.Z += HeightOffset;
	const FVector EffectDirection = Pattern.PatternType == EBRBossPatternType::Dash && !bIsRetreat
		? GetLockedDashDirection(Pattern)
		: (bHasActivePattern ? LockedAttackDirection : GetActorForwardVector());
	const FVector EffectLocation = EffectOrigin + (EffectDirection.GetSafeNormal2D() * Pattern.ForwardOffset);
	return FTransform(EffectDirection.Rotation(), EffectLocation);
}

void ABRPatternBossBase::SpawnPatternEffect(
	UNiagaraSystem* Effect,
	const FBRBossPatternData& Pattern,
	FName SocketName,
	float HeightOffset,
	const FVector& Scale) const
{
	if (!Effect)
	{
		return;
	}

	const bool bUseSocket = !Pattern.bDashAwayFromTarget
		&& !SocketName.IsNone()
		&& SkeletalMeshComponent
		&& SkeletalMeshComponent->DoesSocketExist(SocketName);
	if (bUseSocket)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			Effect,
			SkeletalMeshComponent,
			SocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Scale,
			EAttachLocation::SnapToTarget,
			true,
			ENCPoolMethod::None,
			true,
			true);
		return;
	}

	const FTransform EffectTransform = GetPatternEffectTransform(Pattern, HeightOffset);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		Effect,
		EffectTransform.GetLocation(),
		EffectTransform.Rotator(),
		Scale,
		true,
		true);
}

void ABRPatternBossBase::StartBossAttack(int32 PatternIndex)
{
	UWorld* World = GetWorld();
	if (!World || !AttackPatterns.IsValidIndex(PatternIndex) || !IsValid(CurrentTarget))
	{
		return;
	}

	if (!NotifyCoordinatedAttackStarted())
	{
		return;
	}

	bAttackSlotClaimed = true;
	++AttackSequence;
	ActivePatternIndex = PatternIndex;
	LastPatternIndex = PatternIndex;
	ActivePatternSnapshot = AttackPatterns[PatternIndex];
	bHasActivePattern = true;
	bAttackHasImpacted = false;
	bIsAttacking = true;
	LastAttackTime = World->GetTimeSeconds();
	const int32 StartedAttackId = AttackSequence;
	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->StopMovement();
	}

	LockedAttackOrigin = GetActorLocation();
	LockedTargetLocation = CurrentTarget->GetActorLocation();
	LockedAttackDirection = FVector(LockedTargetLocation - LockedAttackOrigin).GetSafeNormal2D();
	if (LockedAttackDirection.IsNearlyZero())
	{
		LockedAttackDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	if (!LockedAttackDirection.IsNearlyZero())
	{
		SetActorRotation(LockedAttackDirection.Rotation());
	}

	const FBRBossPatternData Pattern = ActivePatternSnapshot;
	LastPatternTimes.Add(Pattern.PatternName, LastAttackTime);
	OnPatternStarted.Broadcast(Pattern.PatternName);
	if (StartedAttackId != AttackSequence || !bIsAttacking || !bHasActivePattern)
	{
		return;
	}

	NotifyBossAnimationStage(
		EBRBossAnimationStage::PatternWindup,
		Pattern.AnimationActionName.IsNone() ? Pattern.PatternName : Pattern.AnimationActionName);
	if (StartedAttackId != AttackSequence || !bIsAttacking || !bHasActivePattern)
	{
		return;
	}

	SpawnPatternEffect(
		ResolvePatternEffect(Pattern, true),
		Pattern,
		Pattern.TelegraphSocketName,
		TelegraphHeightOffset,
		Pattern.TelegraphEffectScale);

	const float WindupTime = FMath::Max(Pattern.Windup, KINDA_SMALL_NUMBER);
	if (bShowDebug && GEngine)
	{
		const FString Message = FString::Printf(TEXT("WARNING: %s"), *Pattern.PatternName.ToString());
		GEngine->AddOnScreenDebugMessage(2006, WindupTime, FColor::Orange, Message);
	}

	FTimerDelegate WindupDelegate;
	WindupDelegate.BindUObject(this, &ABRPatternBossBase::PerformBossAttack, StartedAttackId);
	World->GetTimerManager().SetTimer(AttackWindupTimerHandle, WindupDelegate, WindupTime, false);
}

void ABRPatternBossBase::PerformBossAttack(int32 AttackId)
{
	if (AttackId != AttackSequence || !bIsAttacking || !bHasActivePattern)
	{
		return;
	}

	if (!bCombatAIEnabled || bIsDead || bIsGroggy || bIsBeingExecuted || bIsPhaseTransitioning || !IsValid(CurrentTarget))
	{
		const bool bReturnToIdle = bCombatAIEnabled && !bIsDead && !bIsGroggy && !bIsBeingExecuted && !bIsPhaseTransitioning;
		CancelBossAttack();
		if (bReturnToIdle)
		{
			NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
		}
		return;
	}

	const FBRBossPatternData Pattern = ActivePatternSnapshot;
	ActivePatternIndex = INDEX_NONE;
	bAttackHasImpacted = true;
	const FName AnimationActionName = Pattern.AnimationActionName.IsNone() ? Pattern.PatternName : Pattern.AnimationActionName;
	NotifyBossAnimationStage(EBRBossAnimationStage::PatternImpact, AnimationActionName);
	if (AttackId != AttackSequence || !bIsAttacking || !bHasActivePattern)
	{
		return;
	}

	FVector DashStart = GetActorLocation();
	FVector DashEnd = DashStart;
	const FVector DashDirection = GetLockedDashDirection(Pattern);
	if (Pattern.PatternType == EBRBossPatternType::Dash && Pattern.DashDistance > KINDA_SMALL_NUMBER)
	{
		FHitResult DashHit;
		AddActorWorldOffset(DashDirection * Pattern.DashDistance, true, &DashHit);
		DashEnd = GetActorLocation();
	}

	SpawnPatternEffect(
		ResolvePatternEffect(Pattern, false),
		Pattern,
		Pattern.ImpactSocketName,
		50.0f,
		Pattern.ImpactEffectScale);

	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector FlatTarget(TargetLocation.X, TargetLocation.Y, 0.0f);
	FVector HitDirection = LockedAttackDirection;
	FVector DebugStart = LockedAttackOrigin;
	FVector DebugEnd = LockedAttackOrigin + (LockedAttackDirection * Pattern.ForwardOffset);
	FVector DebugCenter = DebugEnd;
	bool bTargetInsideHitShape = false;

	if (Pattern.PatternType == EBRBossPatternType::AOE)
	{
		DebugCenter = GetAOECenter(Pattern, 0.0f);
		bTargetInsideHitShape = FVector::Dist2D(DebugCenter, TargetLocation) <= Pattern.Radius;
		HitDirection = FVector(TargetLocation - DebugCenter).GetSafeNormal2D();
	}
	else if (Pattern.PatternType == EBRBossPatternType::Dash)
	{
		DebugStart = DashStart;
		// A retreat moves away after striking toward the locked target. Do not
		// mirror its damage capsule into the escape direction.
		DebugEnd = Pattern.bDashAwayFromTarget
			? DashStart + (LockedAttackDirection * Pattern.ForwardOffset)
			: DashEnd + (DashDirection * Pattern.ForwardOffset);
		const FVector FlatStart(DebugStart.X, DebugStart.Y, 0.0f);
		const FVector FlatEnd(DebugEnd.X, DebugEnd.Y, 0.0f);
		const FVector ClosestPoint = FMath::ClosestPointOnSegment(FlatTarget, FlatStart, FlatEnd);
		bTargetInsideHitShape = FVector::Dist(ClosestPoint, FlatTarget) <= Pattern.Radius;
		HitDirection = Pattern.bDashAwayFromTarget ? LockedAttackDirection : DashDirection;
	}
	else
	{
		const FVector FlatStart(DebugStart.X, DebugStart.Y, 0.0f);
		const FVector FlatEnd(DebugEnd.X, DebugEnd.Y, 0.0f);
		const FVector ClosestPoint = FMath::ClosestPointOnSegment(FlatTarget, FlatStart, FlatEnd);
		const float ForwardDistance = FVector::DotProduct(FlatTarget - FlatStart, LockedAttackDirection);
		bTargetInsideHitShape = ForwardDistance >= -(Pattern.Radius * 0.25f)
			&& FVector::Dist(ClosestPoint, FlatTarget) <= Pattern.Radius;
	}

	if (bDrawAttackDebug)
	{
		const FColor DebugColor = bTargetInsideHitShape ? FColor::Red : FColor::Silver;
		if (Pattern.PatternType == EBRBossPatternType::AOE)
		{
			DrawDebugSphere(GetWorld(), DebugCenter, Pattern.Radius, 24, DebugColor, false, 1.0f, 0, 2.0f);
		}
		else
		{
			DrawDebugLine(GetWorld(), DebugStart, DebugEnd, DebugColor, false, 1.0f, 0, 3.0f);
			DrawDebugSphere(GetWorld(), DebugEnd, Pattern.Radius, 20, DebugColor, false, 1.0f);
		}
	}

	float AppliedDamage = 0.0f;
	if (bTargetInsideHitShape)
	{
		const float EffectiveDamage = Pattern.Damage * (bIsEnraged ? EnrageDamageMultiplier : 1.0f);
		const bool bParryableDamage = Pattern.PatternType == EBRBossPatternType::Melee && Pattern.bCanBeParried;
		const TSubclassOf<UDamageType> DamageTypeClass = bParryableDamage
			? UBRParryableBossDamageType::StaticClass()
			: UBRBossDamageType::StaticClass();
		AppliedDamage = UGameplayStatics::ApplyDamage(
			CurrentTarget,
			EffectiveDamage,
			GetController(),
			this,
			DamageTypeClass);
	}

	if (AppliedDamage > 0.0f)
	{
		ApplyPatternHitFeedback(CurrentTarget, Pattern, HitDirection);
		OnPatternHit.Broadcast(Pattern.PatternName);

		if (bShowDebug && GEngine)
		{
			const FString AttackText = FString::Printf(TEXT("%s Hit! -%.0f HP"), *Pattern.PatternName.ToString(), AppliedDamage);
			GEngine->AddOnScreenDebugMessage(2007, 1.0f, FColor::Red, AttackText);
		}
	}

	BeginAttackRecovery(Pattern, AttackId);
}

void ABRPatternBossBase::ApplyPatternHitFeedback(
	AActor* HitActor,
	const FBRBossPatternData& Pattern,
	const FVector& HitDirection)
{
	float FeedbackMultiplier = 1.0f;
	switch (Pattern.PatternType)
	{
	case EBRBossPatternType::Melee:
		FeedbackMultiplier = 0.8f;
		break;
	case EBRBossPatternType::Dash:
		FeedbackMultiplier = 1.35f;
		break;
	case EBRBossPatternType::AOE:
		FeedbackMultiplier = 1.1f;
		break;
	default:
		break;
	}

	FVector SafeHitDirection = HitDirection.GetSafeNormal2D();
	if (SafeHitDirection.IsNearlyZero())
	{
		SafeHitDirection = LockedAttackDirection.GetSafeNormal2D();
	}

	if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
	{
		const float Knockback = Pattern.KnockbackStrength * FeedbackMultiplier;
		const float Lift = Pattern.KnockbackLift * FeedbackMultiplier;
		HitCharacter->LaunchCharacter((SafeHitDirection * Knockback) + (FVector::UpVector * Lift), true, false);
	}

	PlayCameraFeedbackForActor(
		HitActor,
		Pattern.CameraShakeScale * FeedbackMultiplier,
		Pattern.RumbleIntensity * FeedbackMultiplier);
}

void ABRPatternBossBase::BeginAttackRecovery(const FBRBossPatternData& Pattern, int32 AttackId)
{
	if (AttackId != AttackSequence || !bIsAttacking || !bHasActivePattern)
	{
		return;
	}

	if (Pattern.ImpactHoldTime <= KINDA_SMALL_NUMBER)
	{
		StartAttackRecovery(AttackId);
		return;
	}

	FTimerDelegate RecoveryStartDelegate;
	RecoveryStartDelegate.BindUObject(this, &ABRPatternBossBase::StartAttackRecovery, AttackId);
	GetWorldTimerManager().SetTimer(AttackRecoveryTimerHandle, RecoveryStartDelegate, Pattern.ImpactHoldTime, false);
}

void ABRPatternBossBase::StartAttackRecovery(int32 AttackId)
{
	if (AttackId != AttackSequence || !bIsAttacking || !bHasActivePattern)
	{
		return;
	}

	const FBRBossPatternData Pattern = ActivePatternSnapshot;
	NotifyBossAnimationStage(EBRBossAnimationStage::PatternRecovery);
	if (AttackId != AttackSequence || !bIsAttacking || !bHasActivePattern)
	{
		return;
	}

	float RecoveryMultiplier = 1.0f;
	if (Pattern.PatternType == EBRBossPatternType::Dash)
	{
		RecoveryMultiplier = 1.35f;
	}
	else if (Pattern.PatternType == EBRBossPatternType::AOE)
	{
		RecoveryMultiplier = 1.15f;
	}

	const float RecoveryDuration = Pattern.RecoveryTime * RecoveryMultiplier;
	if (RecoveryDuration <= KINDA_SMALL_NUMBER)
	{
		FinishBossAttack(AttackId);
		return;
	}

	FTimerDelegate RecoveryFinishDelegate;
	RecoveryFinishDelegate.BindUObject(this, &ABRPatternBossBase::FinishBossAttack, AttackId);
	GetWorldTimerManager().SetTimer(AttackRecoveryTimerHandle, RecoveryFinishDelegate, RecoveryDuration, false);
}

void ABRPatternBossBase::FinishBossAttack(int32 AttackId)
{
	if (AttackId != AttackSequence || !bIsAttacking || !bHasActivePattern)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AttackWindupTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackRecoveryTimerHandle);
	const FName PatternName = ActivePatternSnapshot.PatternName;
	bIsAttacking = false;
	bHasActivePattern = false;
	bAttackHasImpacted = false;
	ActivePatternIndex = INDEX_NONE;
	ActivePatternSnapshot = FBRBossPatternData();
	NextAttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() + MinAttackGap : LastAttackTime + MinAttackGap;
	ReleaseAttackSlot();
	NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
	OnPatternFinished.Broadcast(PatternName);
}

void ABRPatternBossBase::CancelBossAttack()
{
	++AttackSequence;
	GetWorldTimerManager().ClearTimer(AttackWindupTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackRecoveryTimerHandle);
	bIsAttacking = false;
	bHasActivePattern = false;
	bAttackHasImpacted = false;
	ActivePatternIndex = INDEX_NONE;
	ActivePatternSnapshot = FBRBossPatternData();
	ReleaseAttackSlot();
}

void ABRPatternBossBase::ReleaseAttackSlot()
{
	if (!bAttackSlotClaimed)
	{
		return;
	}

	bAttackSlotClaimed = false;
	NotifyCoordinatedAttackFinished();
}
