#include "Boss/Pattern/BRPatternBossBase.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

int32 ABRPatternBossBase::SelectPattern(float DistanceToTarget) const
{
	for (int32 Index = 0; Index < AttackPatterns.Num(); ++Index)
	{
		if (CanStartPattern(AttackPatterns[Index], DistanceToTarget))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool ABRPatternBossBase::CanStartPattern(const FBRBossPatternData& Pattern, float DistanceToTarget) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const bool bPhaseEnabled = BossPhase == EBRBossPhase::Phase1 ? Pattern.bEnableInPhase1 : Pattern.bEnableInPhase2;
	if (!bPhaseEnabled || DistanceToTarget < Pattern.MinRange || DistanceToTarget > Pattern.MaxRange)
	{
		return false;
	}

	if (Pattern.bRequiresTeamMateNear && !IsTeamMateWithin(Pattern.TeamMateNearDistance))
	{
		return false;
	}

	return CanStartCoordinatedAttack() && World->GetTimeSeconds() - LastAttackTime >= GetPatternCooldown(Pattern);
}

float ABRPatternBossBase::GetPatternCooldown(const FBRBossPatternData& Pattern) const
{
	const float PhaseMultiplier = BossPhase == EBRBossPhase::Phase2 ? Phase2CooldownMultiplier : 1.0f;
	return Pattern.Cooldown * PhaseMultiplier;
}

void ABRPatternBossBase::StartBossAttack(int32 PatternIndex)
{
	if (!AttackPatterns.IsValidIndex(PatternIndex))
	{
		return;
	}

	if (!NotifyCoordinatedAttackStarted())
	{
		return;
	}

	ActivePatternIndex = PatternIndex;
	bIsAttacking = true;
	LastAttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	const FBRBossPatternData& Pattern = AttackPatterns[ActivePatternIndex];
	OnPatternStarted.Broadcast(Pattern.PatternName);
	NotifyBossAnimationStage(EBRBossAnimationStage::PatternWindup, Pattern.AnimationActionName.IsNone() ? Pattern.PatternName : Pattern.AnimationActionName);
	if (GEngine)
	{
		const FString Message = FString::Printf(TEXT("WARNING: %s"), *Pattern.PatternName.ToString());
		GEngine->AddOnScreenDebugMessage(2006, Pattern.Windup, FColor::Orange, Message);
	}

	GetWorldTimerManager().SetTimer(AttackWindupTimerHandle, this, &ABRPatternBossBase::PerformBossAttack, Pattern.Windup, false);
}

void ABRPatternBossBase::PerformBossAttack()
{
	bIsAttacking = false;

	if (!bCombatAIEnabled || bIsDead || bIsGroggy || bIsBeingExecuted || !CurrentTarget || !AttackPatterns.IsValidIndex(ActivePatternIndex))
	{
		ActivePatternIndex = INDEX_NONE;
		NotifyCoordinatedAttackFinished();
		return;
	}

	const FBRBossPatternData Pattern = AttackPatterns[ActivePatternIndex];
	ActivePatternIndex = INDEX_NONE;
	const FName AnimationActionName = Pattern.AnimationActionName.IsNone() ? Pattern.PatternName : Pattern.AnimationActionName;
	NotifyBossAnimationStage(EBRBossAnimationStage::PatternImpact, AnimationActionName);

	if (Pattern.PatternType == EBRBossPatternType::Dash)
	{
		const FVector RawDashDirection = CurrentTarget
			? FVector(CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()
			: GetActorForwardVector();
		const FVector DashDirection = Pattern.bDashAwayFromTarget ? -RawDashDirection : RawDashDirection;
		const FVector FinalDashDirection = DashDirection.IsNearlyZero() ? GetActorForwardVector() : DashDirection;
		AddActorWorldOffset(FinalDashDirection * Pattern.DashDistance, true);
		FaceTarget(0.0f);
	}

	const FVector AttackStart = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector AttackForward = GetActorForwardVector();
	const FVector AttackEnd = AttackStart + (AttackForward * Pattern.ForwardOffset);
	const FVector AttackCenter = Pattern.PatternType == EBRBossPatternType::AOE ? AttackStart : AttackEnd;

	bool bHitTarget = false;
	if (Pattern.PatternType == EBRBossPatternType::AOE)
	{
		bHitTarget = FVector::Dist(AttackCenter, CurrentTarget->GetActorLocation()) <= Pattern.Radius;
	}
	else
	{
		const FVector TargetLocation = CurrentTarget->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
		const FVector ClosestPoint = FMath::ClosestPointOnSegment(TargetLocation, AttackStart, AttackEnd);
		const bool bHitForwardLine = FVector::Dist(ClosestPoint, TargetLocation) <= Pattern.Radius;
		const bool bHitCloseBody = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation()) <= Pattern.Radius;
		bHitTarget = bHitForwardLine || bHitCloseBody;
	}

	if (bDrawAttackDebug)
	{
		if (Pattern.PatternType == EBRBossPatternType::AOE)
		{
			DrawDebugSphere(GetWorld(), AttackCenter, Pattern.Radius, 16, bHitTarget ? FColor::Red : FColor::Silver, false, 1.0f, 0, 2.0f);
		}
		else
		{
			DrawDebugLine(GetWorld(), AttackStart, AttackEnd, bHitTarget ? FColor::Red : FColor::Silver, false, 1.0f, 0, 2.0f);
			DrawDebugSphere(GetWorld(), AttackEnd, Pattern.Radius, 16, bHitTarget ? FColor::Red : FColor::Silver, false, 1.0f);
		}
	}

	if (!bHitTarget)
	{
		NotifyBossAnimationStage(EBRBossAnimationStage::PatternRecovery, AnimationActionName);
		OnPatternFinished.Broadcast(Pattern.PatternName);
		NotifyCoordinatedAttackFinished();
		return;
	}

	UGameplayStatics::ApplyDamage(CurrentTarget, Pattern.Damage, nullptr, this, UDamageType::StaticClass());
	OnPatternHit.Broadcast(Pattern.PatternName);
	NotifyBossAnimationStage(EBRBossAnimationStage::PatternRecovery, AnimationActionName);
	OnPatternFinished.Broadcast(Pattern.PatternName);
	NotifyCoordinatedAttackFinished();

	if (GEngine)
	{
		const FString AttackText = FString::Printf(TEXT("%s Hit! -%.0f HP"), *Pattern.PatternName.ToString(), Pattern.Damage);
		GEngine->AddOnScreenDebugMessage(2007, 1.0f, FColor::Red, AttackText);
	}
}
