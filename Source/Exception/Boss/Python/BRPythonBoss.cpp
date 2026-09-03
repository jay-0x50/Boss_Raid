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
	EnrageMoveSpeedMultiplier = 1.18f;
	EnrageDamageMultiplier = 1.20f;
	EnrageCooldownMultiplier = 0.82f;
	MeleeStandbyDistance = 560.0f;
	RangedStandbyDistance = 950.0f;
	RangedComfortMinDistance = 520.0f;
	ConfigurePythonPatterns();
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
		ProceduralAttackTravelDistance = 42.0f;
		ProceduralAttackLeanAngle = 14.0f;
		ProceduralGroggyDropDistance = 34.0f;
		ProceduralDeathRollAngle = 78.0f;

		FBRBossPatternData TailSweepCombo;
		TailSweepCombo.PatternName = TEXT("Aurathos_TailSweepCombo");
		TailSweepCombo.AnimationActionName = TEXT("TailSweepCombo");
		TailSweepCombo.WindupCueName = TEXT("Aurathos_Windup");
		TailSweepCombo.ImpactCueName = TEXT("Aurathos_Impact");
		TailSweepCombo.ImpactSocketName = TEXT("FX_Tail");
		TailSweepCombo.PatternType = EBRBossPatternType::AOE;
		TailSweepCombo.bCenterAOEOnTarget = false;
		TailSweepCombo.MinRange = 0.0f;
		TailSweepCombo.MaxRange = 340.0f;
		TailSweepCombo.Damage = 20.0f * AurathosDamageMultiplier;
		TailSweepCombo.bCanBeParried = true;
		TailSweepCombo.Windup = 0.3f;
		TailSweepCombo.ImpactHoldTime = 0.15f;
		TailSweepCombo.RecoveryTime = 0.5f;
		TailSweepCombo.Cooldown = 3.0f;
		TailSweepCombo.Radius = 250.0f;
		TailSweepCombo.ForwardOffset = 0.0f;
		TailSweepCombo.FollowUpDelay = 0.15f;
		TailSweepCombo.FollowUpDamage = 20.0f * AurathosDamageMultiplier;
		TailSweepCombo.FollowUpRadius = 285.0f;
		TailSweepCombo.FollowUpAnimationActionName = TEXT("TailSweepFollowUp");
		TailSweepCombo.FollowUpCueName = TEXT("Aurathos_Impact");
		TailSweepCombo.bOverrideEffectColor = true;
		TailSweepCombo.EffectColor = FLinearColor(1.0f, 0.35f, 0.02f, 1.0f);
		TailSweepCombo.bEnableInPhase1 = true;
		TailSweepCombo.bEnableInPhase2 = true;
		AttackPatterns.Add(TailSweepCombo);

		FBRBossPatternData ShadowDash;
		ShadowDash.PatternName = TEXT("Aurathos_ShadowDash");
		ShadowDash.WindupAnimationActionName = TEXT("ShadowDashStart");
		ShadowDash.ImpactAnimationActionName = TEXT("ShadowDashImpact");
		ShadowDash.RecoveryAnimationActionName = TEXT("ShadowDashImpact");
		ShadowDash.WindupCueName = TEXT("Aurathos_Windup");
		ShadowDash.ImpactCueName = TEXT("Aurathos_Impact");
		ShadowDash.ImpactSocketName = TEXT("FX_FrontRight");
		ShadowDash.PatternType = EBRBossPatternType::Dash;
		ShadowDash.MinRange = 360.0f;
		ShadowDash.MaxRange = 735.0f;
		ShadowDash.Damage = 25.0f * AurathosDamageMultiplier;
		ShadowDash.bCanBeParried = false;
		ShadowDash.Windup = 0.65f;
		ShadowDash.Cooldown = 2.6f;
		ShadowDash.Radius = 125.0f;
		ShadowDash.ForwardOffset = 150.0f;
		ShadowDash.DashDistance = 460.0f;
		ShadowDash.bOverrideEffectColor = true;
		ShadowDash.EffectColor = FLinearColor(1.0f, 0.28f, 0.015f, 1.0f);
		ShadowDash.bEnableInPhase1 = true;
		ShadowDash.bEnableInPhase2 = true;
		AttackPatterns.Add(ShadowDash);

		FBRBossPatternData LavaEruption;
		LavaEruption.PatternName = TEXT("Aurathos_LavaEruption");
		LavaEruption.AnimationActionName = TEXT("LavaEruption");
		LavaEruption.WindupCueName = TEXT("Aurathos_Windup");
		LavaEruption.ImpactCueName = TEXT("Aurathos_Impact");
		LavaEruption.PatternType = EBRBossPatternType::AOE;
		LavaEruption.bCenterAOEOnTarget = true;
		LavaEruption.MinRange = 0.0f;
		LavaEruption.MaxRange = 420.0f;
		LavaEruption.Damage = 24.0f * AurathosDamageMultiplier;
		LavaEruption.bCanBeParried = false;
		LavaEruption.Windup = 0.8f;
		LavaEruption.Cooldown = 3.2f;
		LavaEruption.Radius = 220.0f;
		LavaEruption.bOverrideEffectColor = true;
		LavaEruption.EffectColor = FLinearColor(1.0f, 0.045f, 0.0f, 1.0f);
		LavaEruption.bEnableInPhase1 = false;
		LavaEruption.bEnableInPhase2 = true;
		AttackPatterns.Add(LavaEruption);

		FBRBossPatternData BiteOrClawCombo;
		BiteOrClawCombo.PatternName = TEXT("Aurathos_BiteOrClawCombo");
		BiteOrClawCombo.AnimationActionName = TEXT("BiteOrClawCombo");
		BiteOrClawCombo.WindupCueName = TEXT("Aurathos_Windup");
		BiteOrClawCombo.ImpactCueName = TEXT("Aurathos_Impact");
		BiteOrClawCombo.ImpactSocketName = TEXT("FX_Mouth");
		BiteOrClawCombo.PatternType = EBRBossPatternType::Melee;
		BiteOrClawCombo.MinRange = 0.0f;
		BiteOrClawCombo.MaxRange = 285.0f;
		BiteOrClawCombo.Damage = 18.0f * AurathosDamageMultiplier;
		BiteOrClawCombo.bCanBeParried = true;
		BiteOrClawCombo.Windup = 0.4f;
		BiteOrClawCombo.ImpactHoldTime = 0.14f;
		BiteOrClawCombo.RecoveryTime = 0.42f;
		BiteOrClawCombo.Cooldown = 2.4f;
		BiteOrClawCombo.Radius = 105.0f;
		BiteOrClawCombo.ForwardOffset = 210.0f;
		BiteOrClawCombo.bOverrideEffectColor = true;
		BiteOrClawCombo.EffectColor = FLinearColor(1.0f, 0.4f, 0.025f, 1.0f);
		BiteOrClawCombo.bEnableInPhase1 = true;
		BiteOrClawCombo.bEnableInPhase2 = true;
		AttackPatterns.Add(BiteOrClawCombo);
	}
	else
	{
		TeamRole = EBRBossTeamRole::Ranged;
		ProceduralAttackTravelDistance = 34.0f;
		ProceduralAttackLeanAngle = 11.0f;
		ProceduralGroggyDropDistance = 28.0f;
		ProceduralDeathRollAngle = 70.0f;

		FBRBossPatternData FrostBeam;
		FrostBeam.PatternName = TEXT("Vethara_FrostBeam");
		FrostBeam.AnimationActionName = TEXT("FrostBeam");
		FrostBeam.WindupCueName = TEXT("Vethara_Windup");
		FrostBeam.ImpactCueName = TEXT("Vethara_Impact");
		FrostBeam.ImpactSocketName = TEXT("FX_Mouth");
		FrostBeam.PatternType = EBRBossPatternType::Melee;
		FrostBeam.MinRange = RangedComfortMinDistance;
		FrostBeam.MaxRange = 1400.0f;
		FrostBeam.Damage = 18.0f * VetharaDamageMultiplier;
		FrostBeam.bCanBeParried = false;
		FrostBeam.Windup = 0.75f;
		FrostBeam.Cooldown = 2.1f;
		FrostBeam.Radius = 160.0f;
		FrostBeam.ForwardOffset = 1240.0f;
		FrostBeam.bOverrideEffectColor = true;
		FrostBeam.EffectColor = FLinearColor(0.01f, 0.5f, 1.0f, 1.0f);
		FrostBeam.bEnableInPhase1 = true;
		FrostBeam.bEnableInPhase2 = true;
		AttackPatterns.Add(FrostBeam);

		FBRBossPatternData LightningCast;
		LightningCast.PatternName = TEXT("Vethara_LightningCast");
		LightningCast.AnimationActionName = TEXT("LightningCast");
		LightningCast.WindupCueName = TEXT("Vethara_Windup");
		LightningCast.ImpactCueName = TEXT("Vethara_Impact");
		LightningCast.PatternType = EBRBossPatternType::AOE;
		LightningCast.bCenterAOEOnTarget = true;
		LightningCast.MinRange = 580.0f;
		LightningCast.MaxRange = 1500.0f;
		LightningCast.Damage = 17.0f * VetharaDamageMultiplier;
		LightningCast.bCanBeParried = false;
		LightningCast.Windup = 0.68f;
		LightningCast.ImpactHoldTime = 0.18f;
		LightningCast.RecoveryTime = 0.46f;
		LightningCast.Cooldown = 2.7f;
		LightningCast.Radius = 185.0f;
		LightningCast.bOverrideEffectColor = true;
		LightningCast.EffectColor = FLinearColor(0.12f, 0.82f, 1.0f, 1.0f);
		LightningCast.bEnableInPhase1 = true;
		LightningCast.bEnableInPhase2 = true;
		AttackPatterns.Add(LightningCast);

		FBRBossPatternData RetreatStrike;
		RetreatStrike.PatternName = TEXT("Vethara_RetreatStrike");
		RetreatStrike.AnimationActionName = TEXT("RetreatStrike");
		RetreatStrike.WindupCueName = TEXT("Vethara_Windup");
		RetreatStrike.ImpactCueName = TEXT("Vethara_Impact");
		RetreatStrike.ImpactSocketName = TEXT("FX_FrontRight");
		RetreatStrike.PatternType = EBRBossPatternType::Dash;
		RetreatStrike.MinRange = 0.0f;
		RetreatStrike.MaxRange = 430.0f;
		RetreatStrike.Damage = 15.0f * VetharaDamageMultiplier;
		RetreatStrike.bCanBeParried = false;
		RetreatStrike.Windup = 0.55f;
		RetreatStrike.Cooldown = 3.4f;
		RetreatStrike.Radius = 140.0f;
		RetreatStrike.ForwardOffset = 290.0f;
		RetreatStrike.DashDistance = 360.0f;
		RetreatStrike.bDashAwayFromTarget = true;
		RetreatStrike.bRequiresTeamMateNear = true;
		RetreatStrike.TeamMateNearDistance = 760.0f;
		RetreatStrike.bOverrideEffectColor = true;
		RetreatStrike.EffectColor = FLinearColor(0.0f, 0.62f, 1.0f, 1.0f);
		RetreatStrike.bEnableInPhase1 = true;
		RetreatStrike.bEnableInPhase2 = true;
		AttackPatterns.Add(RetreatStrike);

		FBRBossPatternData BlizzardZone;
		BlizzardZone.PatternName = TEXT("Vethara_BlizzardZone");
		BlizzardZone.AnimationActionName = TEXT("BlizzardZone");
		BlizzardZone.WindupCueName = TEXT("Vethara_Windup");
		BlizzardZone.ImpactCueName = TEXT("Vethara_Impact");
		BlizzardZone.PatternType = EBRBossPatternType::AOE;
		BlizzardZone.bCenterAOEOnTarget = true;
		BlizzardZone.MinRange = 450.0f;
		BlizzardZone.MaxRange = 1000.0f;
		BlizzardZone.Damage = 20.0f * VetharaDamageMultiplier;
		BlizzardZone.Windup = 1.0f;
		BlizzardZone.Cooldown = 3.0f;
		BlizzardZone.Radius = 260.0f;
		BlizzardZone.bOverrideEffectColor = true;
		BlizzardZone.EffectColor = FLinearColor(0.02f, 0.36f, 1.0f, 1.0f);
		BlizzardZone.bEnableInPhase1 = true;
		BlizzardZone.bEnableInPhase2 = true;
		AttackPatterns.Add(BlizzardZone);

		FBRBossPatternData PanicBite;
		PanicBite.PatternName = TEXT("Vethara_PanicBite");
		PanicBite.AnimationActionName = TEXT("PanicBite");
		PanicBite.WindupCueName = TEXT("Vethara_Windup");
		PanicBite.ImpactCueName = TEXT("Vethara_Impact");
		PanicBite.ImpactSocketName = TEXT("FX_Mouth");
		PanicBite.PatternType = EBRBossPatternType::Melee;
		PanicBite.MinRange = 0.0f;
		PanicBite.MaxRange = 225.0f;
		PanicBite.Damage = 12.0f * VetharaDamageMultiplier;
		PanicBite.bCanBeParried = true;
		PanicBite.Windup = 0.7f;
		PanicBite.Cooldown = 2.2f;
		PanicBite.Radius = 85.0f;
		PanicBite.ForwardOffset = 140.0f;
		PanicBite.bOverrideEffectColor = true;
		PanicBite.EffectColor = FLinearColor(0.0f, 0.46f, 1.0f, 1.0f);
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
