#include "Boss/Pattern/BRPatternBossBase.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "NiagaraSystem.h"

ABRPatternBossBase::ABRPatternBossBase()
{
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitMelee(TEXT("/Game/VFX/Boss/NS_Boss_Melee.NS_Boss_Melee"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitDash(TEXT("/Game/VFX/Boss/NS_Boss_Dash.NS_Boss_Dash"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitAOE(TEXT("/Game/VFX/Boss/NS_Boss_AOE.NS_Boss_AOE"));
	MeleeEffect = HitMelee.Object;
	DashEffect = HitDash.Object;
	AOEEffect = HitAOE.Object;
	// 별도 전조 에셋이 없는 보스도 공격 종류별로 반드시 경고를 보여 준다.
	MeleeTelegraphEffect = HitMelee.Object;
	DashTelegraphEffect = HitDash.Object;
	AOETelegraphEffect = HitAOE.Object;

	InitialMaxHP = 300.0f;
	InitialMaxGroggy = 100.0f;
	GroggyDuration = 3.0f;

	FBRBossPatternData CloseAttack;
	CloseAttack.PatternName = TEXT("CloseSwing");
	CloseAttack.PatternType = EBRBossPatternType::Melee;
	CloseAttack.MinRange = 0.0f;
	CloseAttack.MaxRange = 280.0f;
	CloseAttack.Damage = 20.0f;
	CloseAttack.bCanBeParried = true;
	CloseAttack.Windup = 0.65f;
	CloseAttack.Cooldown = 1.8f;
	CloseAttack.Radius = 90.0f;
	CloseAttack.ForwardOffset = 180.0f;
	CloseAttack.bEnableInPhase1 = true;
	CloseAttack.bEnableInPhase2 = true;
	AttackPatterns.Add(CloseAttack);

	FBRBossPatternData DashAttack;
	DashAttack.PatternName = TEXT("Phase2Dash");
	DashAttack.PatternType = EBRBossPatternType::Dash;
	DashAttack.MinRange = 520.0f;
	DashAttack.MaxRange = 900.0f;
	DashAttack.Damage = 24.0f;
	DashAttack.Windup = 0.55f;
	DashAttack.Cooldown = 3.0f;
	DashAttack.Radius = 110.0f;
	DashAttack.ForwardOffset = 160.0f;
	DashAttack.DashDistance = 420.0f;
	DashAttack.bEnableInPhase1 = false;
	DashAttack.bEnableInPhase2 = true;
	AttackPatterns.Add(DashAttack);

	FBRBossPatternData LongAttack;
	LongAttack.PatternName = TEXT("LongStab");
	LongAttack.PatternType = EBRBossPatternType::Melee;
	LongAttack.MinRange = 220.0f;
	LongAttack.MaxRange = 520.0f;
	LongAttack.Damage = 16.0f;
	LongAttack.bCanBeParried = true;
	LongAttack.Windup = 0.85f;
	LongAttack.Cooldown = 2.4f;
	LongAttack.Radius = 70.0f;
	LongAttack.ForwardOffset = 320.0f;
	LongAttack.bEnableInPhase1 = false;
	LongAttack.bEnableInPhase2 = true;
	AttackPatterns.Add(LongAttack);
}

void ABRPatternBossBase::ResetPatternBoss()
{
	ResetBoss();
}

void ABRPatternBossBase::SetCombatAIEnabled(bool bEnabled)
{
	Super::SetCombatAIEnabled(bEnabled);
}

void ABRPatternBossBase::OnBossReset()
{
	LastAttackTime = -1000.0f;
	NextAttackTime = -1000.0f;
	LastPatternTimes.Reset();
	ActivePatternIndex = INDEX_NONE;
	LastPatternIndex = INDEX_NONE;
	ActivePatternSnapshot = FBRBossPatternData();
	bHasActivePattern = false;
	bAttackHasImpacted = false;
	bAttackSlotClaimed = false;
	ClearBaseTimers();

	if (MeshComponent)
	{
		MeshComponent->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0f, 1.0f, 1.0f));
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0f, 1.0f, 1.0f));
	}
}

void ABRPatternBossBase::OnBossDeadInternal()
{
	CancelBossAttack();
}

void ABRPatternBossBase::OnBossGroggyInternal()
{
	CancelBossAttack();
}

void ABRPatternBossBase::OnBossRecoveredFromGroggyInternal()
{
	ActivePatternIndex = INDEX_NONE;
}

void ABRPatternBossBase::OnBossPhaseChanged(EBRBossPhase NewPhase)
{
	FBRBossPatternData PhaseEffectPattern;
	PhaseEffectPattern.PatternName = TEXT("PhaseTransition");
	PhaseEffectPattern.PatternType = EBRBossPatternType::AOE;
	const FVector EffectScale(PhaseTransitionEffectScale);
	SpawnPatternEffect(
		ResolvePatternEffect(PhaseEffectPattern, false),
		PhaseEffectPattern,
		NAME_None,
		TelegraphHeightOffset,
		EffectScale);
	PlayCameraFeedbackForActor(CurrentTarget, PhaseTransitionCameraShakeScale, 0.45f);

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2012, 2.0f, FColor::Orange, TEXT("Pattern boss table switched to Phase 2"));
	}
}

void ABRPatternBossBase::ClearBaseTimers()
{
	Super::ClearBaseTimers();
	CancelBossAttack();
}

FString ABRPatternBossBase::GetBossDebugName() const
{
	return TEXT("Pattern Boss Base");
}
