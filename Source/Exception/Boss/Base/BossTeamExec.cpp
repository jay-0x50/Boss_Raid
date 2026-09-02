#include "Boss/Base/BRBossBase.h"

#include "Boss/Team/BRBossTeamCoordinator.h"
#include "BRStatComponent.h"
#include "Engine/Engine.h"

bool ABRBossBase::IsTeamMateAttacking() const
{
	return TeamCoordinator && TeamCoordinator->IsOtherBossAttacking(const_cast<ABRBossBase*>(this));
}

bool ABRBossBase::IsTeamMateWithin(float Distance) const
{
	return TeamCoordinator && TeamCoordinator->IsOtherBossWithin(const_cast<ABRBossBase*>(this), Distance);
}

void ABRBossBase::SetTeamCoordinator(ABRBossTeamCoordinator* NewTeamCoordinator)
{
	if (TeamCoordinator == NewTeamCoordinator)
	{
		if (TeamCoordinator)
		{
			TeamCoordinator->RegisterBoss(this);
		}
		return;
	}

	if (TeamCoordinator)
	{
		TeamCoordinator->UnregisterBoss(this);
	}

	TeamCoordinator = NewTeamCoordinator;
	if (TeamCoordinator)
	{
		TeamCoordinator->RegisterBoss(this);
	}
}

void ABRBossBase::ApplyTeamSlot(int32 TeamSlotIndex)
{
}

bool ABRBossBase::CanBeExecuted() const
{
	return bIsGroggy && !bIsDead && !bIsBeingExecuted && !bIsPhaseTransitioning;
}

bool ABRBossBase::BeginExecution(AActor* Executor)
{
	if (!CanBeExecuted())
	{
		return false;
	}

	bIsBeingExecuted = true;
	bIsAttacking = false;
	ClearBaseTimers();
	OnExecutionStarted.Broadcast(Executor);

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2009, 1.5f, FColor::Purple, TEXT("Execution Started"));
	}

	return true;
}

bool ABRBossBase::CompleteExecution(float Damage, AActor* Executor)
{
	if (!bIsBeingExecuted || bIsDead || !StatComponent)
	{
		return false;
	}

	bIsBeingExecuted = false;
	LastDamageCauser = Executor;
	const bool bApplied = StatComponent->ApplyDamageToStats(Damage, 0.0f);
	if (bApplied)
	{
		StartProceduralHitReaction(Executor);
		PlayCameraFeedbackForActor(Executor, 1.25f, 0.5f);
	}
	RefreshPhaseByHP();
	if (TeamCoordinator)
	{
		TeamCoordinator->NotifyMemberHealthChanged(this);
	}

	if (!bIsDead)
	{
		bIsGroggy = false;
		StatComponent->ResetGroggy();
		if (!bIsPhaseTransitioning)
		{
			NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
			OnBossRecoveredFromGroggy.Broadcast();
			OnBossRecoveredFromGroggyInternal();
		}
	}

	OnExecutionCompleted.Broadcast(Executor);

	if (bShowDebug && GEngine)
	{
		const FString ExecutionText = FString::Printf(TEXT("Execution Hit! -%.0f HP"), Damage);
		GEngine->AddOnScreenDebugMessage(2010, 1.5f, FColor::Purple, ExecutionText);
	}

	return bApplied;
}

bool ABRBossBase::CanStartCoordinatedAttack() const
{
	return !TeamCoordinator || TeamCoordinator->CanStartAttack(const_cast<ABRBossBase*>(this));
}

bool ABRBossBase::NotifyCoordinatedAttackStarted()
{
	return !TeamCoordinator || TeamCoordinator->NotifyAttackStarted(this);
}

void ABRBossBase::NotifyCoordinatedAttackFinished()
{
	if (TeamCoordinator)
	{
		TeamCoordinator->NotifyAttackFinished(this);
	}
}
