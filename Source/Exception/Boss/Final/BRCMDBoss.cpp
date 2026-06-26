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
	MoveSpeed = 145.0f;
	Phase2MoveSpeedMultiplier = 1.18f;
	Phase2CooldownMultiplier = 0.62f;
	RotationInterpSpeed = 0.0f;
	MeleeStandbyDistance = 360.0f;
	RangedStandbyDistance = 1200.0f;
	RangedComfortMinDistance = 700.0f;
	VisualMeshType = EBRBossVisualMeshType::SkeletalMesh;
	MeshRelativeLocation = FVector(0.0f, 0.0f, -100.0f);
	MeshRelativeRotation = FRotator::ZeroRotator;
	MeshRelativeScale = FVector(100.0f, 100.0f, 100.0f);
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
			? TEXT("Hidden Ending: Handel seized CMD root authority.")
			: TEXT("Basic Ending: CMD defeated.");
		GEngine->AddOnScreenDebugMessage(2051, 6.0f, FColor::Purple, EndingText);
	}
}

void ABRCMDBoss::OnBossPhaseChanged(EBRBossPhase NewPhase)
{
	Super::OnBossPhaseChanged(NewPhase);
	ConfigureCMDPatterns();

	if (GEngine)
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
	DirSweep.PatternType = EBRBossPatternType::Melee;
	DirSweep.MinRange = 0.0f;
	DirSweep.MaxRange = 620.0f;
	DirSweep.Damage = 34.0f;
	DirSweep.Windup = 0.75f;
	DirSweep.Cooldown = 2.1f;
	DirSweep.Radius = 210.0f;
	DirSweep.ForwardOffset = 360.0f;
	DirSweep.bEnableInPhase1 = true;
	DirSweep.bEnableInPhase2 = true;
	AttackPatterns.Add(DirSweep);

	FBRBossPatternData PingFlood;
	PingFlood.PatternName = TEXT("PING_Flood");
	PingFlood.AnimationActionName = TEXT("PING_Flood");
	PingFlood.PatternType = EBRBossPatternType::AOE;
	PingFlood.MinRange = 250.0f;
	PingFlood.MaxRange = 1100.0f;
	PingFlood.Damage = 30.0f;
	PingFlood.Windup = 1.05f;
	PingFlood.Cooldown = 3.1f;
	PingFlood.Radius = 460.0f;
	PingFlood.bEnableInPhase1 = true;
	PingFlood.bEnableInPhase2 = true;
	AttackPatterns.Add(PingFlood);

	FBRBossPatternData TaskkillCharge;
	TaskkillCharge.PatternName = TEXT("TASKKILL_Charge");
	TaskkillCharge.AnimationActionName = TEXT("TASKKILL_Charge");
	TaskkillCharge.PatternType = EBRBossPatternType::Dash;
	TaskkillCharge.MinRange = 520.0f;
	TaskkillCharge.MaxRange = 1450.0f;
	TaskkillCharge.Damage = 42.0f;
	TaskkillCharge.Windup = 1.1f;
	TaskkillCharge.Cooldown = 4.0f;
	TaskkillCharge.Radius = 170.0f;
	TaskkillCharge.ForwardOffset = 240.0f;
	TaskkillCharge.DashDistance = 760.0f;
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
	RootPrompt.Cooldown = 4.6f;
	RootPrompt.Radius = 560.0f;
	RootPrompt.bEnableInPhase1 = true;
	RootPrompt.bEnableInPhase2 = true;
	AttackPatterns.Add(RootPrompt);

	FBRBossPatternData FormatZone;
	FormatZone.PatternName = TEXT("FORMAT_RuntimeZone");
	FormatZone.AnimationActionName = TEXT("FORMAT_RuntimeZone");
	FormatZone.PatternType = EBRBossPatternType::AOE;
	FormatZone.MinRange = 420.0f;
	FormatZone.MaxRange = 1700.0f;
	FormatZone.Damage = 52.0f;
	FormatZone.Windup = 1.55f;
	FormatZone.Cooldown = 5.4f;
	FormatZone.Radius = 720.0f;
	FormatZone.bEnableInPhase1 = false;
	FormatZone.bEnableInPhase2 = true;
	AttackPatterns.Add(FormatZone);

	FBRBossPatternData AuthoritySeize;
	AuthoritySeize.PatternName = TEXT("AUTHORITY_Seize");
	AuthoritySeize.AnimationActionName = TEXT("AUTHORITY_Seize");
	AuthoritySeize.PatternType = EBRBossPatternType::Dash;
	AuthoritySeize.MinRange = 820.0f;
	AuthoritySeize.MaxRange = 2200.0f;
	AuthoritySeize.Damage = 60.0f;
	AuthoritySeize.Windup = 1.35f;
	AuthoritySeize.Cooldown = 6.0f;
	AuthoritySeize.Radius = 190.0f;
	AuthoritySeize.ForwardOffset = 260.0f;
	AuthoritySeize.DashDistance = 1050.0f;
	AuthoritySeize.bEnableInPhase1 = false;
	AuthoritySeize.bEnableInPhase2 = true;
	AttackPatterns.Add(AuthoritySeize);
}

FString ABRCMDBoss::GetBossDebugName() const
{
	return TEXT("CMD, The First Command");
}
