#include "Boss/Pattern/BRPatternBossBase.h"

#include "BRStatComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

void ABRPatternBossBase::DrawBossDebug() const
{
	DrawActivePatternTelegraph();

	if (!bShowDebug || !GEngine)
	{
		return;
	}

	const float CurrentHP = StatComponent ? StatComponent->GetCurrentHP() : 0.0f;
	const float MaxHP = StatComponent ? StatComponent->GetMaxHP() : 0.0f;
	const float CurrentGroggy = StatComponent ? StatComponent->GetCurrentGroggy() : 0.0f;
	const float MaxGroggy = StatComponent ? StatComponent->GetMaxGroggy() : 0.0f;
	const FString PatternText = bHasActivePattern
		? ActivePatternSnapshot.PatternName.ToString()
		: TEXT("None");

	const FString DebugText = FString::Printf(
		TEXT("Pattern Boss Base\nPhase: %s\nRole: %s\nHP: %.0f / %.0f\nGroggy: %.0f / %.0f\nAI: %s\nTeamMate Attacking: %s\nAttacking: %s\nPattern: %s\nGroggy State: %s\nExecution: %s\nDead: %s"),
		BossPhase == EBRBossPhase::Phase2 ? TEXT("Phase2") : TEXT("Phase1"),
		TeamRole == EBRBossTeamRole::Ranged ? TEXT("Ranged") : TeamRole == EBRBossTeamRole::Melee ? TEXT("Melee") : TeamRole == EBRBossTeamRole::Support ? TEXT("Support") : TEXT("Solo"),
		CurrentHP,
		MaxHP,
		CurrentGroggy,
		MaxGroggy,
		bCombatAIEnabled ? TEXT("true") : TEXT("false"),
		IsTeamMateAttacking() ? TEXT("true") : TEXT("false"),
		bIsAttacking ? TEXT("true") : TEXT("false"),
		*PatternText,
		bIsGroggy ? TEXT("true") : TEXT("false"),
		bIsBeingExecuted ? TEXT("true") : TEXT("false"),
		bIsDead ? TEXT("true") : TEXT("false"));

	GEngine->AddOnScreenDebugMessage(2001, 0.0f, FColor::Orange, DebugText);
}

void ABRPatternBossBase::DrawActivePatternTelegraph() const
{
	if (!bDrawAttackTelegraph || !bIsAttacking || !bHasActivePattern || bAttackHasImpacted || !GetWorld())
	{
		return;
	}

	const FBRBossPatternData& Pattern = ActivePatternSnapshot;
	const FVector GroundOffset(0.0f, 0.0f, TelegraphHeightOffset - GroundTraceActorHalfHeight);
	const FVector AttackStart = LockedAttackOrigin + GroundOffset;
	const FVector AttackForward = Pattern.PatternType == EBRBossPatternType::Dash
		? GetLockedDashDirection(Pattern)
		: LockedAttackDirection;
	const FVector AttackEnd = AttackStart + (AttackForward * Pattern.ForwardOffset);
	const FColor TelegraphColor = BossPhase == EBRBossPhase::Phase2 ? FColor(255, 40, 0) : FColor(255, 135, 0);
	constexpr float TelegraphDuration = 0.06f;

	if (Pattern.PatternType == EBRBossPatternType::AOE)
	{
		const FVector AOECenter = GetAOECenter(Pattern, TelegraphHeightOffset);
		DrawDebugCylinder(
			GetWorld(),
			AOECenter,
			AOECenter + FVector(0.0f, 0.0f, 12.0f),
			Pattern.Radius,
			48,
			TelegraphColor,
			false,
			TelegraphDuration,
			0,
			3.0f);
		return;
	}

	DrawDebugLine(GetWorld(), AttackStart, AttackEnd, TelegraphColor, false, TelegraphDuration, 0, Pattern.Radius * 0.08f);
	DrawDebugSphere(GetWorld(), AttackEnd, Pattern.Radius, 24, TelegraphColor, false, TelegraphDuration, 0, 2.0f);

	if (Pattern.PatternType == EBRBossPatternType::Dash && Pattern.DashDistance > 0.0f)
	{
		const FVector DashEnd = AttackStart + (AttackForward * (Pattern.DashDistance + Pattern.ForwardOffset));
		DrawDebugLine(GetWorld(), AttackStart, DashEnd, FColor::Red, false, TelegraphDuration, 0, 5.0f);
		DrawDebugSphere(GetWorld(), DashEnd, Pattern.Radius * 0.75f, 20, FColor::Red, false, TelegraphDuration, 0, 2.0f);
	}

}
