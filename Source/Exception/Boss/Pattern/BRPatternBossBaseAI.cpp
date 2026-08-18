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

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
	if (DistanceToTarget > DetectionRange)
	{
		if (CurrentAnimationStage != EBRBossAnimationStage::Idle)
		{
			NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
		}
		return;
	}

	FaceTarget(DeltaSeconds);

	const int32 PatternIndex = SelectPattern(DistanceToTarget);
	if (PatternIndex != INDEX_NONE)
	{
		StartBossAttack(PatternIndex);
		return;
	}

	if (IsTeamMateAttacking())
	{
		MoveToTeamStandbyDistance(DeltaSeconds, DistanceToTarget);
		return;
	}

	if (TeamRole == EBRBossTeamRole::Ranged && DistanceToTarget < RangedComfortMinDistance)
	{
		MoveToTeamStandbyDistance(DeltaSeconds, DistanceToTarget);
		return;
	}

	if (TeamRole != EBRBossTeamRole::Ranged && DistanceToTarget > MeleeStandbyDistance)
	{
		MoveTowardTarget(DeltaSeconds);
		return;
	}

	if (TeamRole == EBRBossTeamRole::Ranged)
	{
		MoveToTeamStandbyDistance(DeltaSeconds, DistanceToTarget);
		return;
	}

	MoveTowardTarget(DeltaSeconds);
}

void ABRPatternBossBase::FaceTarget(float DeltaSeconds)
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
	const FRotator NewRotation = RotationInterpSpeed > 0.0f
		? FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, RotationInterpSpeed)
		: DesiredRotation;
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}

void ABRPatternBossBase::MoveTowardTarget(float DeltaSeconds)
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

	AddActorWorldOffset(MoveDirection * GetCurrentMoveSpeed() * DeltaSeconds, true);
	NotifyBossAnimationStage(EBRBossAnimationStage::Move);
}

void ABRPatternBossBase::MoveToTeamStandbyDistance(float DeltaSeconds, float CurrentDistanceToTarget)
{
	if (!CurrentTarget)
	{
		return;
	}

	const float DesiredDistance = TeamRole == EBRBossTeamRole::Ranged ? RangedStandbyDistance : MeleeStandbyDistance;
	const float DistanceTolerance = 80.0f;
	if (FMath::Abs(CurrentDistanceToTarget - DesiredDistance) <= DistanceTolerance)
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

	const FVector MoveDirection = CurrentDistanceToTarget > DesiredDistance ? DirectionToTarget : -DirectionToTarget;
	AddActorWorldOffset(MoveDirection * GetCurrentMoveSpeed() * DeltaSeconds, true);
	NotifyBossAnimationStage(EBRBossAnimationStage::Move);
}

float ABRPatternBossBase::GetCurrentMoveSpeed() const
{
	const float PhaseMultiplier = BossPhase == EBRBossPhase::Phase2 ? Phase2MoveSpeedMultiplier : 1.0f;
	return MoveSpeed * PhaseMultiplier;
}
