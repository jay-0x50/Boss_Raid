#include "Boss/Team/BRBossTeamCoordinator.h"

#include "Boss/Base/BRBossBase.h"

ABRBossTeamCoordinator::ABRBossTeamCoordinator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABRBossTeamCoordinator::BeginPlay()
{
	Super::BeginPlay();

	BindConfiguredTeamMembers();
}

void ABRBossTeamCoordinator::BindConfiguredTeamMembers()
{
	const TArray<TObjectPtr<ABRBossBase>> ConfiguredMembers = TeamMembers;
	for (int32 Index = 0; Index < ConfiguredMembers.Num(); ++Index)
	{
		ABRBossBase* Boss = ConfiguredMembers[Index];
		if (Boss)
		{
			Boss->ApplyTeamSlot(Index);
			Boss->SetTeamCoordinator(this);
		}
	}
}

void ABRBossTeamCoordinator::RegisterBoss(ABRBossBase* Boss)
{
	if (Boss)
	{
		TeamMembers.AddUnique(Boss);
	}
}

void ABRBossTeamCoordinator::UnregisterBoss(ABRBossBase* Boss)
{
	TeamMembers.Remove(Boss);
	if (ActiveAttacker == Boss)
	{
		ActiveAttacker = nullptr;
		LastAttackFinishedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastAttackFinishedTime;
	}
}

bool ABRBossTeamCoordinator::CanStartAttack(ABRBossBase* RequestingBoss) const
{
	if (!RequestingBoss)
	{
		return false;
	}

	if (bAllowSimultaneousAttacks)
	{
		return true;
	}

	if (ActiveAttacker && ActiveAttacker != RequestingBoss)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	return !World || World->GetTimeSeconds() - LastAttackFinishedTime >= TeamAttackGap;
}

bool ABRBossTeamCoordinator::NotifyAttackStarted(ABRBossBase* AttackingBoss)
{
	if (!CanStartAttack(AttackingBoss))
	{
		return false;
	}

	if (!bAllowSimultaneousAttacks)
	{
		ActiveAttacker = AttackingBoss;
	}

	RegisterBoss(AttackingBoss);
	return true;
}

void ABRBossTeamCoordinator::NotifyAttackFinished(ABRBossBase* AttackingBoss)
{
	if (ActiveAttacker == AttackingBoss)
	{
		ActiveAttacker = nullptr;
		LastAttackFinishedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastAttackFinishedTime;
	}
}

void ABRBossTeamCoordinator::GetTeamMembers(TArray<ABRBossBase*>& OutTeamMembers) const
{
	OutTeamMembers.Reset();
	for (ABRBossBase* Boss : TeamMembers)
	{
		if (Boss)
		{
			OutTeamMembers.AddUnique(Boss);
		}
	}
}

bool ABRBossTeamCoordinator::IsOtherBossAttacking(ABRBossBase* RequestingBoss) const
{
	return ActiveAttacker && ActiveAttacker != RequestingBoss;
}

bool ABRBossTeamCoordinator::IsOtherBossWithin(ABRBossBase* RequestingBoss, float Distance) const
{
	if (!RequestingBoss || Distance <= 0.0f)
	{
		return false;
	}

	const float DistanceSq = FMath::Square(Distance);
	for (const ABRBossBase* Boss : TeamMembers)
	{
		if (Boss && Boss != RequestingBoss && !Boss->IsDead())
		{
			if (FVector::DistSquared(Boss->GetActorLocation(), RequestingBoss->GetActorLocation()) <= DistanceSq)
			{
				return true;
			}
		}
	}

	return false;
}

void ABRBossTeamCoordinator::NotifyMemberHealthChanged(ABRBossBase* ChangedBoss)
{
	RegisterBoss(ChangedBoss);
	EvaluateHealthDifferenceEnrage();
	EvaluateSurvivorEscalation();
}

void ABRBossTeamCoordinator::NotifyMemberDefeated(ABRBossBase* DefeatedBoss)
{
	RegisterBoss(DefeatedBoss);
	EvaluateSurvivorEscalation();
}

void ABRBossTeamCoordinator::NotifyMemberReset(ABRBossBase* ResetBoss)
{
	RegisterBoss(ResetBoss);
	bHealthDifferenceEnrageTriggered = false;
	bSurvivorEscalationTriggered = false;
	bTeamRewardGranted = false;
	ActiveAttacker = nullptr;
	LastAttackFinishedTime = -1000.0f;

	for (ABRBossBase* Boss : TeamMembers)
	{
		if (Boss)
		{
			Boss->SetEnraged(false);
		}
	}
}

bool ABRBossTeamCoordinator::ConsumeTeamDefeatReward(ABRBossBase* DefeatedBoss)
{
	if (!DefeatedBoss)
	{
		return false;
	}

	RegisterBoss(DefeatedBoss);
	if (GetValidMemberCount() <= 1)
	{
		return true;
	}

	for (const ABRBossBase* Boss : TeamMembers)
	{
		if (Boss && Boss != DefeatedBoss && !Boss->IsDead())
		{
			return false;
		}
	}

	if (bTeamRewardGranted)
	{
		return false;
	}

	bTeamRewardGranted = true;
	return true;
}

void ABRBossTeamCoordinator::EvaluateHealthDifferenceEnrage()
{
	if (bHealthDifferenceEnrageTriggered || HealthDifferenceEnrageThreshold <= 0.0f)
	{
		return;
	}

	ABRBossBase* HighestHealthBoss = nullptr;
	float HighestHealthPercent = -1.0f;
	float LowestHealthPercent = 2.0f;
	int32 AliveMemberCount = 0;
	for (ABRBossBase* Boss : TeamMembers)
	{
		if (!Boss || Boss->IsDead())
		{
			continue;
		}

		++AliveMemberCount;
		const float HealthPercent = Boss->GetHPPercent();
		if (HealthPercent > HighestHealthPercent)
		{
			HighestHealthPercent = HealthPercent;
			HighestHealthBoss = Boss;
		}
		LowestHealthPercent = FMath::Min(LowestHealthPercent, HealthPercent);
	}

	if (AliveMemberCount >= 2 && HighestHealthBoss
		&& HighestHealthPercent - LowestHealthPercent >= HealthDifferenceEnrageThreshold)
	{
		bHealthDifferenceEnrageTriggered = true;
		HighestHealthBoss->SetEnraged(true);
	}
}

void ABRBossTeamCoordinator::EvaluateSurvivorEscalation()
{
	if (bSurvivorEscalationTriggered || GetValidMemberCount() < 2)
	{
		return;
	}

	ABRBossBase* Survivor = nullptr;
	int32 AliveMemberCount = 0;
	for (ABRBossBase* Boss : TeamMembers)
	{
		if (Boss && !Boss->IsDead())
		{
			Survivor = Boss;
			++AliveMemberCount;
		}
	}

	if (AliveMemberCount == 1 && Survivor)
	{
		bSurvivorEscalationTriggered = true;
		Survivor->SetEnraged(true);
		Survivor->ForcePhase2(true);
	}
}

int32 ABRBossTeamCoordinator::GetValidMemberCount() const
{
	int32 ValidMemberCount = 0;
	for (const ABRBossBase* Boss : TeamMembers)
	{
		if (Boss)
		{
			++ValidMemberCount;
		}
	}
	return ValidMemberCount;
}
