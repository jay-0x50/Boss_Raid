#include "Boss/Perl/BRVritraBoss.h"

#include "Engine/Engine.h"

ABRVritraBoss::ABRVritraBoss()
{
	InitialMaxHP = 560.0f;
	InitialMaxGroggy = 140.0f;
	GroggyDuration = 3.4f;
	Phase2StartHPRatio = 0.5f;
	DetectionRange = 2300.0f;
	MoveSpeed = 165.0f;
	Phase2MoveSpeedMultiplier = 1.22f;
	Phase2CooldownMultiplier = 0.7f;
	RotationInterpSpeed = 0.0f;
	MeleeStandbyDistance = 260.0f;
	RangedStandbyDistance = 1020.0f;
	RangedComfortMinDistance = 620.0f;
	VisualMeshType = EBRBossVisualMeshType::SkeletalMesh;
	MeshRelativeLocation = FVector(0.0f, 0.0f, -90.0f);
	MeshRelativeRotation = FRotator::ZeroRotator;
	MeshRelativeScale = FVector(100.0f, 100.0f, 100.0f);
	GroundTraceActorHalfHeight = 130.0f;

	ConfigureVritraPatterns();
}

void ABRVritraBoss::OnBossReset()
{
	Super::OnBossReset();
	ConfigureVritraPatterns();
}

void ABRVritraBoss::OnBossPhaseChanged(EBRBossPhase NewPhase)
{
	Super::OnBossPhaseChanged(NewPhase);
	ConfigureVritraPatterns();

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2040, 2.0f, FColor::Orange, TEXT("Vritra shifted regex phase"));
	}
}

void ABRVritraBoss::ConfigureVritraPatterns()
{
	AttackPatterns.Reset();
	TeamRole = EBRBossTeamRole::Solo;

	FBRBossPatternData HumpCrash;
	HumpCrash.PatternName = TEXT("Sigil_HumpCrash");
	HumpCrash.PatternType = EBRBossPatternType::AOE;
	HumpCrash.MinRange = 0.0f;
	HumpCrash.MaxRange = 560.0f;
	HumpCrash.Damage = 30.0f;
	HumpCrash.Windup = 0.9f;
	HumpCrash.Cooldown = 2.4f;
	HumpCrash.Radius = 300.0f;
	HumpCrash.bEnableInPhase1 = true;
	HumpCrash.bEnableInPhase2 = true;
	AttackPatterns.Add(HumpCrash);

	FBRBossPatternData RegexSpit;
	RegexSpit.PatternName = TEXT("Regex_SpitLine");
	RegexSpit.ImpactSocketName = TEXT("FX_Mouth");
	RegexSpit.PatternType = EBRBossPatternType::Melee;
	RegexSpit.MinRange = 560.0f;
	RegexSpit.MaxRange = 1550.0f;
	RegexSpit.Damage = 24.0f;
	RegexSpit.Windup = 1.0f;
	RegexSpit.Cooldown = 2.8f;
	RegexSpit.Radius = 140.0f;
	RegexSpit.ForwardOffset = 1150.0f;
	RegexSpit.bEnableInPhase1 = true;
	RegexSpit.bEnableInPhase2 = true;
	AttackPatterns.Add(RegexSpit);

	FBRBossPatternData CaravanRush;
	CaravanRush.PatternName = TEXT("Caravan_Rush");
	CaravanRush.ImpactSocketName = TEXT("FX_FrontRight");
	CaravanRush.PatternType = EBRBossPatternType::Dash;
	CaravanRush.MinRange = 420.0f;
	CaravanRush.MaxRange = 1150.0f;
	CaravanRush.Damage = 36.0f;
	CaravanRush.Windup = 1.1f;
	CaravanRush.Cooldown = 3.8f;
	CaravanRush.Radius = 165.0f;
	CaravanRush.ForwardOffset = 190.0f;
	CaravanRush.DashDistance = 620.0f;
	CaravanRush.bEnableInPhase1 = true;
	CaravanRush.bEnableInPhase2 = true;
	AttackPatterns.Add(CaravanRush);

	FBRBossPatternData HashSandstorm;
	HashSandstorm.PatternName = TEXT("Hash_Sandstorm");
	HashSandstorm.PatternType = EBRBossPatternType::AOE;
	HashSandstorm.bCenterAOEOnTarget = true;
	HashSandstorm.MinRange = 260.0f;
	HashSandstorm.MaxRange = 900.0f;
	HashSandstorm.Damage = 28.0f;
	HashSandstorm.Windup = 1.25f;
	HashSandstorm.Cooldown = 4.2f;
	HashSandstorm.Radius = 420.0f;
	HashSandstorm.bEnableInPhase1 = true;
	HashSandstorm.bEnableInPhase2 = true;
	AttackPatterns.Add(HashSandstorm);

	FBRBossPatternData BacktrackingStomp;
	BacktrackingStomp.PatternName = TEXT("Backtracking_Stomp");
	BacktrackingStomp.PatternType = EBRBossPatternType::AOE;
	BacktrackingStomp.MinRange = 0.0f;
	BacktrackingStomp.MaxRange = 760.0f;
	BacktrackingStomp.Damage = 40.0f;
	BacktrackingStomp.Windup = 1.35f;
	BacktrackingStomp.Cooldown = 4.6f;
	BacktrackingStomp.Radius = 500.0f;
	BacktrackingStomp.bEnableInPhase1 = false;
	BacktrackingStomp.bEnableInPhase2 = true;
	AttackPatterns.Add(BacktrackingStomp);

	FBRBossPatternData OneLinerPierce;
	OneLinerPierce.PatternName = TEXT("OneLiner_Pierce");
	OneLinerPierce.ImpactSocketName = TEXT("FX_FrontLeft");
	OneLinerPierce.PatternType = EBRBossPatternType::Dash;
	OneLinerPierce.MinRange = 700.0f;
	OneLinerPierce.MaxRange = 1700.0f;
	OneLinerPierce.Damage = 44.0f;
	OneLinerPierce.Windup = 1.2f;
	OneLinerPierce.Cooldown = 5.0f;
	OneLinerPierce.Radius = 135.0f;
	OneLinerPierce.ForwardOffset = 200.0f;
	OneLinerPierce.DashDistance = 820.0f;
	OneLinerPierce.bEnableInPhase1 = false;
	OneLinerPierce.bEnableInPhase2 = true;
	AttackPatterns.Add(OneLinerPierce);
}

FString ABRVritraBoss::GetBossDebugName() const
{
	return TEXT("Vritra, Perl Nomad");
}
