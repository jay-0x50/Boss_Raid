#include "Boss/Base/BRBossBase.h"

#include "Boss/AI/BRBossAIController.h"
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
	if (!StatComponent || bIsDead || Damage <= 0.0f)
	{
		return false;
	}

	const bool bApplied = StatComponent->ApplyDamageToStats(Damage, GroggyDamage);
	if (!bApplied)
	{
		return false;
	}

	LastDamageCauser = DamageCauser;
	RefreshPhaseByHP();

	UE_LOG(LogTemp, Log, TEXT("%s hit: Damage=%.1f, GroggyDamage=%.1f, HP=%.1f/%.1f, Groggy=%.1f/%.1f"),
		*GetBossDebugName(),
		Damage,
		GroggyDamage,
		StatComponent->GetCurrentHP(),
		StatComponent->GetMaxHP(),
		StatComponent->GetCurrentGroggy(),
		StatComponent->GetMaxGroggy());

	if (GEngine)
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
	LastDamageCauser = nullptr;
	VerticalFallSpeed = 0.0f;
	BossPhase = EBRBossPhase::Phase1;
	ClearBaseTimers();

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
	bCombatAIEnabled = bEnabled;
	bIsAttacking = false;
	SetBossAnimationPlaying(bCombatAIEnabled && !bIsDead);
	SetActorEnableCollision(!bDisableCollisionWhenInactive || bCombatAIEnabled);
	NotifyBossAnimationStage(EBRBossAnimationStage::Idle);

	if (ABRBossAIController* BossAIController = GetBossAIController())
	{
		BossAIController->SetBossAIEnabled(bCombatAIEnabled);
	}

	if (GEngine)
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
	bCombatAIEnabled = false;
	bIsAttacking = false;
	bIsBeingExecuted = false;
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
	if (!StatComponent || bIsDead || bIsGroggy || GroggyDamage <= 0.0f)
	{
		return false;
	}

	const bool bApplied = StatComponent->ApplyDamageToStats(0.0f, GroggyDamage);
	if (!bApplied)
	{
		return false;
	}

	if (GEngine)
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
	ClearBaseTimers();
	NotifyCoordinatedAttackFinished();
	SetActorEnableCollision(false);
	SetBossAnimationPlaying(false);
	NotifyBossAnimationStage(EBRBossAnimationStage::Death);
	OnBossDead.Broadcast();
	OnBossDeadInternal();

	if (AExceptionCharacter* RewardCharacter = Cast<AExceptionCharacter>(LastDamageCauser))
	{
		RewardCharacter->AwardBossVictoryRewards(this);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2003, 2.0f, FColor::Red, TEXT("Boss Dead"));
	}
}

void ABRBossBase::HandleGroggy()
{
	bIsGroggy = true;
	bIsAttacking = false;
	ClearBaseTimers();
	NotifyBossAnimationStage(EBRBossAnimationStage::Groggy);
	OnBossGroggy.Broadcast();
	OnBossGroggyInternal();
	GetWorldTimerManager().SetTimer(GroggyTimerHandle, this, &ABRBossBase::RecoverFromGroggy, GroggyDuration, false);

	if (GEngine)
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
	if (bIsDead)
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

	if (GEngine)
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
		BossPhase = EBRBossPhase::Phase2;
		OnPhaseChanged.Broadcast(BossPhase);
		OnBossPhaseChanged(BossPhase);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2011, 2.0f, FColor::Orange, TEXT("Boss Phase 2"));
		}
	}
}
