#include "Boss/Python/BRPythonBoss.h"

#include "Engine/Engine.h"

ABRPythonBoss::ABRPythonBoss()
{
	InitialMaxHP = 260.0f;
	InitialMaxGroggy = 100.0f;
	GroggyDuration = 3.5f;
	Phase2StartHPRatio = 0.5f;
	DetectionRange = 1900.0f;
	RunSpeed = 210.0f;
	Phase2MoveSpeedMultiplier = 1.25f;
	Phase2CooldownMultiplier = 0.72f;
	MeleeStandbyDistance = 560.0f;
	RangedStandbyDistance = 950.0f;
	RangedComfortMinDistance = 520.0f;
}

void ABRPythonBoss::ApplyTeamSlot(int32 TeamSlotIndex)
{
	Super::ApplyTeamSlot(TeamSlotIndex);

	if (!bUseTeamSlotRole)
	{
		return;
	}

	PythonBossIdentity = TeamSlotIndex == 0
		? EBRPythonBossIdentity::Vethara
		: EBRPythonBossIdentity::Aurathos;
	ConfigurePythonPatterns();
}

void ABRPythonBoss::OnBossReset()
{
	Super::OnBossReset();
	ConfigurePythonPatterns();
}

void ABRPythonBoss::OnBossPhaseChanged(EBRBossPhase NewPhase)
{
	Super::OnBossPhaseChanged(NewPhase);
	ConfigurePythonPatterns();

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2020, 2.0f, FColor::Orange, TEXT("Python Twin Boss Phase Pattern Refresh"));
	}
}

void ABRPythonBoss::ConfigurePythonPatterns()
{
	AttackPatterns.Reset();

	if (PythonBossIdentity == EBRPythonBossIdentity::Aurathos)
	{
		TeamRole = EBRBossTeamRole::Melee;

		FBRBossPatternData TailSweepCombo;
		TailSweepCombo.PatternName = TEXT("Aurathos_TailSweepCombo");
		TailSweepCombo.ImpactSocketName = TEXT("FX_Tail");
		TailSweepCombo.PatternType = EBRBossPatternType::Melee;
		TailSweepCombo.MinRange = 0.0f;
		TailSweepCombo.MaxRange = 430.0f;
		TailSweepCombo.Damage = 22.0f * AurathosDamageMultiplier;
		TailSweepCombo.Windup = 0.5f;
		TailSweepCombo.Cooldown = 1.5f;
		TailSweepCombo.Radius = 170.0f;
		TailSweepCombo.ForwardOffset = 240.0f;
		TailSweepCombo.bEnableInPhase1 = true;
		TailSweepCombo.bEnableInPhase2 = true;
		AttackPatterns.Add(TailSweepCombo);

		FBRBossPatternData ShadowDash;
		ShadowDash.PatternName = TEXT("Aurathos_ShadowDash");
		ShadowDash.ImpactSocketName = TEXT("FX_FrontRight");
		ShadowDash.PatternType = EBRBossPatternType::Dash;
		ShadowDash.MinRange = 360.0f;
		ShadowDash.MaxRange = 850.0f;
		ShadowDash.Damage = 25.0f * AurathosDamageMultiplier;
		ShadowDash.Windup = 0.65f;
		ShadowDash.Cooldown = 2.6f;
		ShadowDash.Radius = 125.0f;
		ShadowDash.ForwardOffset = 150.0f;
		ShadowDash.DashDistance = 460.0f;
		ShadowDash.bEnableInPhase1 = true;
		ShadowDash.bEnableInPhase2 = true;
		AttackPatterns.Add(ShadowDash);

		FBRBossPatternData LavaEruption;
		LavaEruption.PatternName = TEXT("Aurathos_LavaEruption");
		LavaEruption.PatternType = EBRBossPatternType::AOE;
		LavaEruption.bCenterAOEOnTarget = true;
		LavaEruption.MinRange = 0.0f;
		LavaEruption.MaxRange = 420.0f;
		LavaEruption.Damage = 24.0f * AurathosDamageMultiplier;
		LavaEruption.Windup = 0.8f;
		LavaEruption.Cooldown = 3.2f;
		LavaEruption.Radius = 220.0f;
		LavaEruption.bEnableInPhase1 = false;
		LavaEruption.bEnableInPhase2 = true;
		AttackPatterns.Add(LavaEruption);
	}
	else
	{
		TeamRole = EBRBossTeamRole::Ranged;

		FBRBossPatternData FrostBeam;
		FrostBeam.PatternName = TEXT("Vethara_FrostBeam");
		FrostBeam.ImpactSocketName = TEXT("FX_Mouth");
		FrostBeam.PatternType = EBRBossPatternType::Melee;
		FrostBeam.MinRange = RangedComfortMinDistance;
		FrostBeam.MaxRange = 1450.0f;
		FrostBeam.Damage = 18.0f * VetharaDamageMultiplier;
		FrostBeam.Windup = 0.75f;
		FrostBeam.Cooldown = 2.1f;
		FrostBeam.Radius = 155.0f;
		FrostBeam.ForwardOffset = 850.0f;
		FrostBeam.bEnableInPhase1 = true;
		FrostBeam.bEnableInPhase2 = true;
		AttackPatterns.Add(FrostBeam);

		FBRBossPatternData RetreatStrike;
		RetreatStrike.PatternName = TEXT("Vethara_ThunderDashRetreat");
		RetreatStrike.ImpactSocketName = TEXT("FX_FrontRight");
		RetreatStrike.PatternType = EBRBossPatternType::Dash;
		RetreatStrike.MinRange = 0.0f;
		RetreatStrike.MaxRange = 430.0f;
		RetreatStrike.Damage = 15.0f * VetharaDamageMultiplier;
		RetreatStrike.Windup = 0.55f;
		RetreatStrike.Cooldown = 3.4f;
		RetreatStrike.Radius = 140.0f;
		RetreatStrike.ForwardOffset = 680.0f;
		RetreatStrike.DashDistance = 360.0f;
		RetreatStrike.bDashAwayFromTarget = true;
		RetreatStrike.bRequiresTeamMateNear = true;
		RetreatStrike.TeamMateNearDistance = 760.0f;
		RetreatStrike.bEnableInPhase1 = true;
		RetreatStrike.bEnableInPhase2 = true;
		AttackPatterns.Add(RetreatStrike);

		FBRBossPatternData BlizzardZone;
		BlizzardZone.PatternName = TEXT("Vethara_BlizzardZone");
		BlizzardZone.PatternType = EBRBossPatternType::AOE;
		BlizzardZone.bCenterAOEOnTarget = true;
		BlizzardZone.MinRange = 450.0f;
		BlizzardZone.MaxRange = 1000.0f;
		BlizzardZone.Damage = 20.0f * VetharaDamageMultiplier;
		BlizzardZone.Windup = 1.0f;
		BlizzardZone.Cooldown = 3.0f;
		BlizzardZone.Radius = 260.0f;
		BlizzardZone.bEnableInPhase1 = true;
		BlizzardZone.bEnableInPhase2 = true;
		AttackPatterns.Add(BlizzardZone);

		FBRBossPatternData PanicBite;
		PanicBite.PatternName = TEXT("Vethara_PanicBite");
		PanicBite.ImpactSocketName = TEXT("FX_Mouth");
		PanicBite.PatternType = EBRBossPatternType::Melee;
		PanicBite.MinRange = 0.0f;
		PanicBite.MaxRange = 260.0f;
		PanicBite.Damage = 12.0f * VetharaDamageMultiplier;
		PanicBite.Windup = 0.7f;
		PanicBite.Cooldown = 2.2f;
		PanicBite.Radius = 85.0f;
		PanicBite.ForwardOffset = 140.0f;
		PanicBite.bEnableInPhase1 = true;
		PanicBite.bEnableInPhase2 = true;
		AttackPatterns.Add(PanicBite);
	}
}

FString ABRPythonBoss::GetBossDebugName() const
{
	return PythonBossIdentity == EBRPythonBossIdentity::Vethara
		? TEXT("Vethara, Unhandled Exception")
		: TEXT("Aurathos, Fatal Process");
}
