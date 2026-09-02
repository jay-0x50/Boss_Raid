#include "Boss/Base/BRBossBase.h"

#include "Boss/AI/BRBossAIController.h"
#include "Boss/Team/BRBossTeamCoordinator.h"
#include "BRStatComponent.h"
#include "Engine/Engine.h"
#include "Player/Character/ExceptionCharacter.h"

float ABRBossBase::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float GroggyDamage = Damage * GroggyDamageMultiplier;
	return ReceiveCombatHit_Implementation(Damage, GroggyDamage, DamageCauser) ? Damage : 0.0f;
}

bool ABRBossBase::ReceiveCombatHit_Implementation(float Damage, float GroggyDamage, AActor* DamageCauser)
{
	if (!StatComponent || bIsDead || Damage <= 0.0f || (bIsPhaseTransitioning && bInvulnerableDuringPhaseTransition))
	{
		return false;
	}

	// OnDead is broadcast inside ApplyDamageToStats, so save the attacker first.
	LastDamageCauser = DamageCauser;
	const bool bApplied = StatComponent->ApplyDamageToStats(Damage, GroggyDamage);
	if (!bApplied)
	{
		return false;
	}

	StartProceduralHitReaction(DamageCauser);
	PlayCameraFeedbackForActor(DamageCauser, BossReceivedHitCameraShakeScale, BossReceivedHitRumbleIntensity);
	RefreshPhaseByHP();
	if (TeamCoordinator)
	{
		TeamCoordinator->NotifyMemberHealthChanged(this);
	}

	UE_LOG(LogTemp, Log, TEXT("%s hit: Damage=%.1f, GroggyDamage=%.1f, HP=%.1f/%.1f, Groggy=%.1f/%.1f"),
		*GetBossDebugName(),
		Damage,
		GroggyDamage,
		StatComponent->GetCurrentHP(),
		StatComponent->GetMaxHP(),
		StatComponent->GetCurrentGroggy(),
		StatComponent->GetMaxGroggy());

	if (bShowDebug && GEngine)
	{
		const FString HitText = FString::Printf(TEXT("%s Hit! -%.0f HP / +%.0f Groggy"), *GetBossDebugName(), Damage, GroggyDamage);
		GEngine->AddOnScreenDebugMessage(2002, 1.0f, FColor::Yellow, HitText);
	}

	return true;
}

void ABRBossBase::ResetBoss()
{
	if (bResetTransformOnBossReset)
	{
		SetActorTransform(InitialBossTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	bIsDead = false;
	bIsGroggy = false;
	bIsAttacking = false;
	bIsBeingExecuted = false;
	bIsPhaseTransitioning = false;
	SetEnraged(false);
	LastDamageCauser = nullptr;
	VerticalFallSpeed = 0.0f;
	ProceduralHitReactionTime = 0.0f;
	BossPhase = EBRBossPhase::Phase1;
	ClearBaseTimers();
	if (TeamCoordinator)
	{
		TeamCoordinator->NotifyMemberReset(this);
	}

	if (StatComponent)
	{
		StatComponent->ConfigureMaxStats(InitialMaxHP, 0.0f, InitialMaxGroggy);
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetBossAnimationPlaying(bCombatAIEnabled && !bIsDead);

	OnBossReset();
}

void ABRBossBase::SetCombatAIEnabled(bool bEnabled)
{
	if (!bEnabled)
	{
		ClearBaseTimers();
		bIsPhaseTransitioning = false;
	}

	bCombatAIEnabled = bEnabled;
	bIsAttacking = false;
	SetBossAnimationPlaying(bCombatAIEnabled && !bIsDead);
	SetActorEnableCollision(!bDisableCollisionWhenInactive || bCombatAIEnabled);
	NotifyBossAnimationStage(EBRBossAnimationStage::Idle);

	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->SetBossAIEnabled(bCombatAIEnabled);
	}

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2005,
			1.5f,
			bCombatAIEnabled ? FColor::Red : FColor::Silver,
			bCombatAIEnabled ? TEXT("Boss AI Enabled") : TEXT("Boss AI Disabled"));
	}
}

void ABRBossBase::PrepareForArenaInactive()
{
	ClearBaseTimers();
	bCombatAIEnabled = false;
	bIsAttacking = false;
	bIsBeingExecuted = false;
	bIsPhaseTransitioning = false;
	VerticalFallSpeed = 0.0f;

	SetBossAnimationPlaying(false);
	NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
	SetActorHiddenInGame(!bShowBossWhenInactive);
	SetActorEnableCollision(!bDisableCollisionWhenInactive);

	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->SetBossAIEnabled(false);
	}
}

void ABRBossBase::StartBossIntro()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(!bDisableCollisionWhenInactive);
	SetBossAnimationPlaying(true);
	NotifyBossAnimationStage(EBRBossAnimationStage::Intro);
	BP_BossIntroStarted();
}

float ABRBossBase::GetMaxHP() const
{
	return StatComponent ? StatComponent->GetMaxHP() : 0.0f;
}

float ABRBossBase::GetCurrentHP() const
{
	return StatComponent ? StatComponent->GetCurrentHP() : 0.0f;
}

float ABRBossBase::GetHPPercent() const
{
	const float MaxHP = GetMaxHP();
	return MaxHP > 0.0f ? GetCurrentHP() / MaxHP : 0.0f;
}

float ABRBossBase::GetMaxGroggy() const
{
	return StatComponent ? StatComponent->GetMaxGroggy() : 0.0f;
}

float ABRBossBase::GetCurrentGroggy() const
{
	return StatComponent ? StatComponent->GetCurrentGroggy() : 0.0f;
}

float ABRBossBase::GetGroggyPercent() const
{
	const float MaxGroggy = GetMaxGroggy();
	return MaxGroggy > 0.0f ? GetCurrentGroggy() / MaxGroggy : 0.0f;
}

FText ABRBossBase::GetBossDisplayName() const
{
	return FText::FromString(GetBossDebugName());
}

ABRBossAIController* ABRBossBase::GetBossAIController() const
{
	return Cast<ABRBossAIController>(GetController());
}

bool ABRBossBase::ApplyGroggyDamage(float GroggyDamage, AActor* DamageCauser)
{
	if (!StatComponent || bIsDead || bIsGroggy || bIsPhaseTransitioning || GroggyDamage <= 0.0f)
	{
		return false;
	}

	const bool bApplied = StatComponent->ApplyDamageToStats(0.0f, GroggyDamage);
	if (!bApplied)
	{
		return false;
	}

	StartProceduralHitReaction(DamageCauser);
	PlayCameraFeedbackForActor(DamageCauser, 0.75f, 0.45f);

	if (bShowDebug && GEngine)
	{
		const FString GroggyText = FString::Printf(TEXT("%s Parried! +%.0f Groggy"), *GetBossDebugName(), GroggyDamage);
		GEngine->AddOnScreenDebugMessage(2013, 1.2f, FColor::Cyan, GroggyText);
	}

	return true;
}

void ABRBossBase::HandleDead()
{
	bIsDead = true;
	bCombatAIEnabled = false;
	bIsAttacking = false;
	bIsBeingExecuted = false;
	bIsPhaseTransitioning = false;
	ClearBaseTimers();
	NotifyCoordinatedAttackFinished();
	if (TeamCoordinator)
	{
		TeamCoordinator->NotifyMemberDefeated(this);
	}
	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->StopMovement();
	}
	SetActorEnableCollision(false);
	SetBossAnimationPlaying(false);
	NotifyBossAnimationStage(EBRBossAnimationStage::Death);
	OnBossDead.Broadcast();
	OnBossDeadInternal();

	if (AExceptionCharacter* RewardCharacter = Cast<AExceptionCharacter>(LastDamageCauser);
		RewardCharacter && (!TeamCoordinator || TeamCoordinator->ConsumeTeamDefeatReward(this)))
	{
		RewardCharacter->AwardBossVictoryRewards(this);
	}

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2003, 2.0f, FColor::Red, TEXT("Boss Dead"));
	}
}

void ABRBossBase::HandleGroggy()
{
	if (bIsPhaseTransitioning)
	{
		if (StatComponent)
		{
			StatComponent->ResetGroggy();
		}
		return;
	}

	bIsGroggy = true;
	bIsAttacking = false;
	ClearBaseTimers();
	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->StopMovement();
	}
	NotifyBossAnimationStage(EBRBossAnimationStage::Groggy);
	OnBossGroggy.Broadcast();
	OnBossGroggyInternal();
	GetWorldTimerManager().SetTimer(GroggyTimerHandle, this, &ABRBossBase::RecoverFromGroggy, GroggyDuration, false);

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2004, 2.0f, FColor::Orange, TEXT("Boss Groggy"));
	}
}

void ABRBossBase::HandleHPChanged(float CurrentValue, float MaxValue, float NormalizedValue)
{
	OnBossHPChanged.Broadcast(CurrentValue, MaxValue, NormalizedValue);
}

void ABRBossBase::HandleGroggyChanged(float CurrentValue, float MaxValue, float NormalizedValue)
{
	OnBossGroggyChanged.Broadcast(CurrentValue, MaxValue, NormalizedValue);
}

void ABRBossBase::RecoverFromGroggy()
{
	if (bIsDead || bIsPhaseTransitioning || !bIsGroggy)
	{
		return;
	}

	if (bIsBeingExecuted)
	{
		GetWorldTimerManager().SetTimer(GroggyTimerHandle, this, &ABRBossBase::RecoverFromGroggy, 0.1f, false);
		return;
	}

	bIsGroggy = false;
	NotifyBossAnimationStage(EBRBossAnimationStage::Idle);

	if (StatComponent)
	{
		StatComponent->ResetGroggy();
	}

	OnBossRecoveredFromGroggy.Broadcast();
	OnBossRecoveredFromGroggyInternal();

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2008, 1.5f, FColor::Silver, TEXT("Boss Recovered From Groggy"));
	}
}

void ABRBossBase::RefreshPhaseByHP()
{
	if (BossPhase == EBRBossPhase::Phase2 || bIsDead || Phase2StartHPRatio <= 0.0f)
	{
		return;
	}

	if (GetHPPercent() <= Phase2StartHPRatio)
	{
		ForcePhase2(false);
	}
}

void ABRBossBase::SetEnraged(bool bNewEnraged)
{
	if ((bNewEnraged && bIsDead) || bIsEnraged == bNewEnraged)
	{
		return;
	}

	bIsEnraged = bNewEnraged;
	OnEnrageChanged.Broadcast(bIsEnraged);
	BP_BossEnrageChanged(bIsEnraged);

	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2014,
			2.0f,
			bIsEnraged ? FColor::Red : FColor::Silver,
			bIsEnraged ? TEXT("Boss Enraged") : TEXT("Boss Enrage Reset"));
	}
}

void ABRBossBase::ForcePhase2(bool bReplayTransitionIfAlreadyPhase2)
{
	if (bIsDead)
	{
		return;
	}

	const bool bPhaseChanged = BossPhase != EBRBossPhase::Phase2;
	if (!bPhaseChanged && (!bReplayTransitionIfAlreadyPhase2 || bIsPhaseTransitioning))
	{
		return;
	}

	BossPhase = EBRBossPhase::Phase2;
	BeginPhaseTransition();
	if (bPhaseChanged)
	{
		OnPhaseChanged.Broadcast(BossPhase);
	}
	OnBossPhaseChanged(BossPhase);

	if (bPhaseChanged && bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2011, 2.0f, FColor::Orange, TEXT("Boss Phase 2"));
	}
}

void ABRBossBase::BeginPhaseTransition()
{
	if (bIsDead || !bCombatAIEnabled)
	{
		return;
	}

	// A phase break owns the boss state. It cancels any windup/recovery and
	// clears a groggy triggered by the same threshold-crossing hit.
	ClearBaseTimers();
	bIsBeingExecuted = false;
	bIsGroggy = false;
	bIsPhaseTransitioning = true;
	bIsAttacking = true;
	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->StopMovement();
	}

	if (StatComponent && StatComponent->IsGroggy())
	{
		StatComponent->ResetGroggy();
	}

	NotifyBossAnimationStage(EBRBossAnimationStage::PhaseTransition);
	if (!bIsPhaseTransitioning || bIsDead || !bCombatAIEnabled)
	{
		return;
	}

	if (PhaseTransitionDuration <= KINDA_SMALL_NUMBER)
	{
		FinishPhaseTransition();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PhaseTransitionTimerHandle,
		this,
		&ABRBossBase::FinishPhaseTransition,
		PhaseTransitionDuration,
		false);
}

void ABRBossBase::FinishPhaseTransition()
{
	GetWorldTimerManager().ClearTimer(PhaseTransitionTimerHandle);
	if (!bIsPhaseTransitioning)
	{
		return;
	}

	bIsPhaseTransitioning = false;
	bIsAttacking = false;
	if (!bIsDead && bCombatAIEnabled)
	{
		NotifyBossAnimationStage(EBRBossAnimationStage::Idle);
		OnPhaseTransitionFinished.Broadcast();
	}
}
