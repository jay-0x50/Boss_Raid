#include "Boss/Final/BRCMDBoss.h"

#include "BRHiddenStorySubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Engine/Engine.h"

ABRCMDBoss::ABRCMDBoss()
{
	InitialMaxHP = 900.0f;
	InitialMaxGroggy = 210.0f;
	GroggyDuration = 3.0f;
	Phase2StartHPRatio = 0.45f;
	DetectionRange = 2800.0f;
	RunSpeed = 260.0f;
	Phase2MoveSpeedMultiplier = 1.18f;
	Phase2CooldownMultiplier = 0.62f;
	TurnSpeed = 8.0f;
	MeleeStandbyDistance = 420.0f;
	RangedStandbyDistance = 1200.0f;
	RangedComfortMinDistance = 700.0f;
	VisualMeshType = EBRBossVisualMeshType::SkeletalMesh;
	MeshRelativeLocation = FVector(0.0f, 0.0f, -90.0f);
	MeshRelativeRotation = FRotator::ZeroRotator;
	MeshRelativeScale = FVector(1.0f, 1.0f, 1.0f);
	GroundTraceActorHalfHeight = 170.0f;

	ConfigureCMDPatterns();
}

void ABRCMDBoss::OnBossReset()
{
	Super::OnBossReset();
	ConfigureCMDPatterns();
}

void ABRCMDBoss::OnBossDeadInternal()
{
	Super::OnBossDeadInternal();

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>();
	if (!HiddenStory)
	{
		return;
	}

	const AExceptionCharacter* RewardCharacter = Cast<AExceptionCharacter>(LastDamageCauser);
	const bool bDefeatedWithMimikatz = RewardCharacter && RewardCharacter->HasInventoryItem(TEXT("Weapon_MimikatzAuthoritySeized"));
	const EBRRuntimeEnding Ending = HiddenStory->ResolveCMDEnding(bDefeatedWithMimikatz);

	if (GEngine)
	{
		const TCHAR* EndingText = Ending == EBRRuntimeEnding::HiddenAuthoritySeized
			? TEXT("Hidden Ending: Hendel seized CMD root authority.")
			: TEXT("Basic Ending: CMD defeated.");
		GEngine->AddOnScreenDebugMessage(2051, 6.0f, FColor::Purple, EndingText);
	}
}

void ABRCMDBoss::OnBossPhaseChanged(EBRBossPhase NewPhase)
{
	Super::OnBossPhaseChanged(NewPhase);
	ConfigureCMDPatterns();

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2050, 2.0f, FColor::Red, TEXT("CMD escalated root authority"));
	}
}

void ABRCMDBoss::ConfigureCMDPatterns()
{
	AttackPatterns.Reset();
	TeamRole = EBRBossTeamRole::Solo;

	FBRBossPatternData DirSweep;
	DirSweep.PatternName = TEXT("DIR_Sweep");
	DirSweep.AnimationActionName = TEXT("DIR_Sweep");
	DirSweep.ImpactSocketName = TEXT("FX_RightHand");
	DirSweep.PatternType = EBRBossPatternType::Melee;
	DirSweep.MinRange = 0.0f;
	DirSweep.MaxRange = 620.0f;
	DirSweep.Damage = 34.0f;
	DirSweep.Windup = 0.75f;
	DirSweep.ImpactHoldTime = 0.16f;
	DirSweep.RecoveryTime = 0.38f;
	DirSweep.Cooldown = 2.1f;
	DirSweep.Radius = 210.0f;
	DirSweep.ForwardOffset = 360.0f;
	DirSweep.KnockbackStrength = 300.0f;
	DirSweep.CameraShakeScale = 0.85f;
	DirSweep.bEnableInPhase1 = true;
	DirSweep.bEnableInPhase2 = true;
	AttackPatterns.Add(DirSweep);

	FBRBossPatternData PingFlood;
	PingFlood.PatternName = TEXT("PING_Flood");
	PingFlood.AnimationActionName = TEXT("PING_Flood");
	PingFlood.PatternType = EBRBossPatternType::AOE;
	PingFlood.bCenterAOEOnTarget = true;
	PingFlood.MinRange = 250.0f;
	PingFlood.MaxRange = 1100.0f;
	PingFlood.Damage = 30.0f;
	PingFlood.Windup = 1.05f;
	PingFlood.ImpactHoldTime = 0.24f;
	PingFlood.RecoveryTime = 0.55f;
	PingFlood.Cooldown = 3.1f;
	PingFlood.Radius = 460.0f;
	PingFlood.KnockbackStrength = 390.0f;
	PingFlood.CameraShakeScale = 1.0f;
	PingFlood.bEnableInPhase1 = true;
	PingFlood.bEnableInPhase2 = true;
	AttackPatterns.Add(PingFlood);

	FBRBossPatternData TaskkillCharge;
	TaskkillCharge.PatternName = TEXT("TASKKILL_Charge");
	TaskkillCharge.AnimationActionName = TEXT("TASKKILL_Charge");
	TaskkillCharge.ImpactSocketName = TEXT("FX_LeftHand");
	TaskkillCharge.PatternType = EBRBossPatternType::Dash;
	TaskkillCharge.MinRange = 520.0f;
	TaskkillCharge.MaxRange = 1450.0f;
	TaskkillCharge.Damage = 42.0f;
	TaskkillCharge.Windup = 1.1f;
	TaskkillCharge.ImpactHoldTime = 0.22f;
	TaskkillCharge.RecoveryTime = 0.7f;
	TaskkillCharge.Cooldown = 4.0f;
	TaskkillCharge.Radius = 170.0f;
	TaskkillCharge.ForwardOffset = 240.0f;
	TaskkillCharge.DashDistance = 760.0f;
	TaskkillCharge.KnockbackStrength = 480.0f;
	TaskkillCharge.KnockbackLift = 105.0f;
	TaskkillCharge.CameraShakeScale = 1.2f;
	TaskkillCharge.bEnableInPhase1 = true;
	TaskkillCharge.bEnableInPhase2 = true;
	AttackPatterns.Add(TaskkillCharge);

	FBRBossPatternData RootPrompt;
	RootPrompt.PatternName = TEXT("ROOT_PromptCrash");
	RootPrompt.AnimationActionName = TEXT("ROOT_PromptCrash");
	RootPrompt.PatternType = EBRBossPatternType::AOE;
	RootPrompt.MinRange = 0.0f;
	RootPrompt.MaxRange = 760.0f;
	RootPrompt.Damage = 46.0f;
	RootPrompt.Windup = 1.3f;
	RootPrompt.ImpactHoldTime = 0.28f;
	RootPrompt.RecoveryTime = 0.72f;
	RootPrompt.Cooldown = 4.6f;
	RootPrompt.Radius = 560.0f;
	RootPrompt.KnockbackStrength = 460.0f;
	RootPrompt.KnockbackLift = 120.0f;
	RootPrompt.CameraShakeScale = 1.2f;
	RootPrompt.bEnableInPhase1 = true;
	RootPrompt.bEnableInPhase2 = true;
	AttackPatterns.Add(RootPrompt);

	FBRBossPatternData FormatZone;
	FormatZone.PatternName = TEXT("FORMAT_RuntimeZone");
	FormatZone.AnimationActionName = TEXT("FORMAT_RuntimeZone");
	FormatZone.PatternType = EBRBossPatternType::AOE;
	FormatZone.bCenterAOEOnTarget = true;
	FormatZone.MinRange = 420.0f;
	FormatZone.MaxRange = 1700.0f;
	FormatZone.Damage = 52.0f;
	FormatZone.Windup = 1.55f;
	FormatZone.ImpactHoldTime = 0.3f;
	FormatZone.RecoveryTime = 0.9f;
	FormatZone.Cooldown = 5.4f;
	FormatZone.Radius = 720.0f;
	FormatZone.KnockbackStrength = 520.0f;
	FormatZone.KnockbackLift = 130.0f;
	FormatZone.CameraShakeScale = 1.35f;
	FormatZone.bEnableInPhase1 = false;
	FormatZone.bEnableInPhase2 = true;
	AttackPatterns.Add(FormatZone);

	FBRBossPatternData AuthoritySeize;
	AuthoritySeize.PatternName = TEXT("AUTHORITY_Seize");
	AuthoritySeize.AnimationActionName = TEXT("AUTHORITY_Seize");
	AuthoritySeize.ImpactSocketName = TEXT("FX_RightHand");
	AuthoritySeize.PatternType = EBRBossPatternType::Dash;
	AuthoritySeize.MinRange = 820.0f;
	AuthoritySeize.MaxRange = 2200.0f;
	AuthoritySeize.Damage = 60.0f;
	AuthoritySeize.Windup = 1.35f;
	AuthoritySeize.ImpactHoldTime = 0.32f;
	AuthoritySeize.RecoveryTime = 1.0f;
	AuthoritySeize.Cooldown = 6.0f;
	AuthoritySeize.Radius = 190.0f;
	AuthoritySeize.ForwardOffset = 260.0f;
	AuthoritySeize.DashDistance = 1050.0f;
	AuthoritySeize.KnockbackStrength = 600.0f;
	AuthoritySeize.KnockbackLift = 150.0f;
	AuthoritySeize.CameraShakeScale = 1.5f;
	AuthoritySeize.RumbleIntensity = 0.5f;
	AuthoritySeize.bEnableInPhase1 = false;
	AuthoritySeize.bEnableInPhase2 = true;
	AttackPatterns.Add(AuthoritySeize);
}

FString ABRCMDBoss::GetBossDebugName() const
{
	return TEXT("CMD, The First Command");
}
