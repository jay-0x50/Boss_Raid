#include "Boss/Selvara/BRSelvaraBoss.h"

#include "Engine/Engine.h"

ABRSelvaraBoss::ABRSelvaraBoss()
{
	InitialMaxHP = 520.0f;
	InitialMaxGroggy = 150.0f;
	GroggyDuration = 3.6f;
	Phase2StartHPRatio = 0.5f;
	DetectionRange = 2200.0f;
	RunSpeed = 135.0f;
	Phase2MoveSpeedMultiplier = 1.18f;
	Phase2CooldownMultiplier = 0.72f;
	TurnSpeed = 0.0f;
	MeleeStandbyDistance = 600.0f;
	RangedStandbyDistance = 980.0f;
	RangedComfortMinDistance = 540.0f;

	ConfigureSelvaraPatterns();
}

void ABRSelvaraBoss::OnBossReset()
{
	Super::OnBossReset();
	ConfigureSelvaraPatterns();
}

void ABRSelvaraBoss::OnBossPhaseChanged(EBRBossPhase NewPhase)
{
	Super::OnBossPhaseChanged(NewPhase);
	ConfigureSelvaraPatterns();

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2030, 2.0f, FColor::Orange, TEXT("Selvara shifted query phase"));
	}
}

void ABRSelvaraBoss::ConfigureSelvaraPatterns()
{
	AttackPatterns.Reset();
	TeamRole = EBRBossTeamRole::Solo;

	FBRBossPatternData TailJoin;
	TailJoin.PatternName = TEXT("Tail_JOIN_Sweep");
	TailJoin.PatternType = EBRBossPatternType::Melee;
	TailJoin.MinRange = 0.0f;
	TailJoin.MaxRange = 430.0f;
	TailJoin.Damage = 28.0f;
	TailJoin.bCanBeParried = true;
	TailJoin.Windup = 0.8f;
	TailJoin.Cooldown = 2.0f;
	TailJoin.Radius = 190.0f;
	TailJoin.ForwardOffset = 260.0f;
	TailJoin.bEnableInPhase1 = true;
	TailJoin.bEnableInPhase2 = true;
	AttackPatterns.Add(TailJoin);

	FBRBossPatternData IndexBreach;
	IndexBreach.PatternName = TEXT("Index_Breach");
	IndexBreach.PatternType = EBRBossPatternType::Dash;
	IndexBreach.MinRange = 420.0f;
	IndexBreach.MaxRange = 1050.0f;
	IndexBreach.Damage = 34.0f;
	IndexBreach.Windup = 1.05f;
	IndexBreach.Cooldown = 3.4f;
	IndexBreach.Radius = 155.0f;
	IndexBreach.ForwardOffset = 170.0f;
	IndexBreach.DashDistance = 520.0f;
	IndexBreach.bEnableInPhase1 = true;
	IndexBreach.bEnableInPhase2 = true;
	AttackPatterns.Add(IndexBreach);

	FBRBossPatternData DataSpout;
	DataSpout.PatternName = TEXT("Data_Spout");
	DataSpout.PatternType = EBRBossPatternType::Melee;
	DataSpout.MinRange = 720.0f;
	DataSpout.MaxRange = 1500.0f;
	DataSpout.Damage = 24.0f;
	DataSpout.Windup = 1.0f;
	DataSpout.Cooldown = 2.6f;
	DataSpout.Radius = 135.0f;
	DataSpout.ForwardOffset = 1050.0f;
	DataSpout.bEnableInPhase1 = true;
	DataSpout.bEnableInPhase2 = true;
	AttackPatterns.Add(DataSpout);

	FBRBossPatternData DeadlockWhirlpool;
	DeadlockWhirlpool.PatternName = TEXT("Deadlock_Whirlpool");
	DeadlockWhirlpool.PatternType = EBRBossPatternType::AOE;
	DeadlockWhirlpool.MinRange = 0.0f;
	DeadlockWhirlpool.MaxRange = 650.0f;
	DeadlockWhirlpool.Damage = 30.0f;
	DeadlockWhirlpool.Windup = 1.25f;
	DeadlockWhirlpool.Cooldown = 4.0f;
	DeadlockWhirlpool.Radius = 360.0f;
	DeadlockWhirlpool.bEnableInPhase1 = true;
	DeadlockWhirlpool.bEnableInPhase2 = true;
	AttackPatterns.Add(DeadlockWhirlpool);

	FBRBossPatternData ReplicationTsunami;
	ReplicationTsunami.PatternName = TEXT("Replication_Tsunami");
	ReplicationTsunami.PatternType = EBRBossPatternType::Melee;
	ReplicationTsunami.MinRange = 500.0f;
	ReplicationTsunami.MaxRange = 1750.0f;
	ReplicationTsunami.Damage = 38.0f;
	ReplicationTsunami.Windup = 1.35f;
	ReplicationTsunami.Cooldown = 4.4f;
	ReplicationTsunami.Radius = 230.0f;
	ReplicationTsunami.ForwardOffset = 1250.0f;
	ReplicationTsunami.bEnableInPhase1 = false;
	ReplicationTsunami.bEnableInPhase2 = true;
	AttackPatterns.Add(ReplicationTsunami);

	FBRBossPatternData CrashRecovery;
	CrashRecovery.PatternName = TEXT("Crash_Recovery_BellyFlop");
	CrashRecovery.PatternType = EBRBossPatternType::AOE;
	CrashRecovery.bCenterAOEOnTarget = true;
	CrashRecovery.MinRange = 0.0f;
	CrashRecovery.MaxRange = 820.0f;
	CrashRecovery.Damage = 42.0f;
	CrashRecovery.Windup = 1.5f;
	CrashRecovery.Cooldown = 5.2f;
	CrashRecovery.Radius = 470.0f;
	CrashRecovery.bEnableInPhase1 = false;
	CrashRecovery.bEnableInPhase2 = true;
	AttackPatterns.Add(CrashRecovery);
}

FString ABRSelvaraBoss::GetBossDebugName() const
{
	return TEXT("Selvara, Abyssal Database");
}
