#include "Boss/Pattern/BRPatternBossBase.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void ABRPatternBossBase::UpdateBossAI(float DeltaSeconds)
{
	if (!bCombatAIEnabled || bIsDead || bIsGroggy || bIsAttacking || bIsBeingExecuted || bIsPhaseTransitioning)
	{
		return;
	}

	if (!CurrentTarget)
	{
		CurrentTarget = UGameplayStatics::GetPlayerCharacter(this, 0);
	}

	if (!CurrentTarget)
	{
		return;
	}

	// Pattern hit shapes are planar, so eligibility must use the same metric.
	const float DistanceToTarget = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	if (DistanceToTarget > DetectionRange)
	{
		if (CurrentAnimationStage != EBRBossAnimationStage::Idle)
		{
			NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
		}
		return;
	}

	LookAtPlayer(DeltaSeconds);

	const int32 PatternIndex = PickAttack(DistanceToTarget);
	if (PatternIndex != INDEX_NONE)
	{
		StartBossAttack(PatternIndex);
		return;
	}

	if (IsTeamMateAttacking())
	{
		KeepSpace(DeltaSeconds, DistanceToTarget);
		return;
	}

	if (TeamRole == EBRBossTeamRole::Ranged && DistanceToTarget < RangedComfortMinDistance)
	{
		KeepSpace(DeltaSeconds, DistanceToTarget);
		return;
	}

	if (TeamRole != EBRBossTeamRole::Ranged && DistanceToTarget > MeleeStandbyDistance)
	{
		RunToPlayer(DeltaSeconds);
		return;
	}

	if (TeamRole == EBRBossTeamRole::Ranged)
	{
		KeepSpace(DeltaSeconds, DistanceToTarget);
		return;
	}

	RunToPlayer(DeltaSeconds);
}

void ABRPatternBossBase::LookAtPlayer(float DeltaSeconds)
{
	if (!CurrentTarget)
	{
		return;
	}

	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	const FVector FlatDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRotation = FlatDirection.Rotation();
	const FRotator NewRotation = TurnSpeed > 0.0f
		? FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, TurnSpeed)
		: DesiredRotation;
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}

void ABRPatternBossBase::RunToPlayer(float DeltaSeconds)
{
	if (!CurrentTarget)
	{
		return;
	}

	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	const FVector MoveDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	if (ToTarget.Size2D() <= MeleeStandbyDistance)
	{
		if (CurrentAnimationStage != EBRBossAnimationStage::Idle)
		{
			NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
		}
		return;
	}

	AddActorWorldOffset(MoveDirection * GetRunSpeed() * DeltaSeconds, true);
	NotifyBossAnimationStage(EBRBossAnimationStage::Move);
}

void ABRPatternBossBase::KeepSpace(float DeltaSeconds, float PlayerDist)
{
	if (!CurrentTarget)
	{
		return;
	}

	const float DesiredDistance = TeamRole == EBRBossTeamRole::Ranged ? RangedStandbyDistance : MeleeStandbyDistance;
	const float DistanceTolerance = 80.0f;
	if (FMath::Abs(PlayerDist - DesiredDistance) <= DistanceTolerance)
	{
		if (CurrentAnimationStage != EBRBossAnimationStage::Idle)
		{
			NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
		}
		return;
	}

	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	const FVector DirectionToTarget = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
	if (DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	const FVector MoveDirection = PlayerDist > DesiredDistance ? DirectionToTarget : -DirectionToTarget;
	AddActorWorldOffset(MoveDirection * GetRunSpeed() * DeltaSeconds, true);
	NotifyBossAnimationStage(EBRBossAnimationStage::Move);
}

float ABRPatternBossBase::GetRunSpeed() const
{
	const float PhaseMultiplier = BossPhase == EBRBossPhase::Phase2 ? Phase2MoveSpeedMultiplier : 1.0f;
	const float EnrageMultiplier = bIsEnraged ? EnrageMoveSpeedMultiplier : 1.0f;
	return RunSpeed * PhaseMultiplier * EnrageMultiplier;
}
