// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "ExceptionGameMode.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

bool AExceptionCharacter::SpendStamina(float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}

	if (CurrentStamina < Amount)
	{
		return false;
	}

	CurrentStamina -= Amount;
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	BroadcastStamina();
	return true;
}

void AExceptionCharacter::RestoreHPAndStamina()
{
	CurrentHP = MaxHP;
	CurrentStamina = MaxStamina;
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	GetWorldTimerManager().ClearTimer(ExecutionTimerHandle);
	SetCombatState(EBRPlayerCombatState::Idle);
	bIsInvincible = false;
	bIsParryActive = false;
	PendingExecutionTarget = nullptr;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->StopMovementImmediately();
	ClearLockOn();
	BroadcastHP();
	BroadcastStamina();
}

void AExceptionCharacter::ApplySavedStats(float SavedHP, float SavedStamina)
{
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	GetWorldTimerManager().ClearTimer(ExecutionTimerHandle);

	CurrentHP = FMath::Clamp(SavedHP, 1.0f, MaxHP);
	CurrentStamina = FMath::Clamp(SavedStamina, 0.0f, MaxStamina);
	SetCombatState(EBRPlayerCombatState::Idle);
	bIsInvincible = false;
	bIsParryActive = false;
	PendingExecutionTarget = nullptr;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->StopMovementImmediately();
	ClearLockOn();
	BroadcastHP();
	BroadcastStamina();
}

void AExceptionCharacter::ApplySavedProgression(int32 SavedPlayerLevel, int32 SavedUpgradePoints, int32 SavedVitalityLevel, int32 SavedEnduranceLevel, int32 SavedPowerLevel)
{
	PlayerLevel = FMath::Max(1, SavedPlayerLevel);
	UpgradePoints = FMath::Max(0, SavedUpgradePoints);
	VitalityLevel = FMath::Max(0, SavedVitalityLevel);
	EnduranceLevel = FMath::Max(0, SavedEnduranceLevel);
	PowerLevel = FMath::Max(0, SavedPowerLevel);
	OnProgressionChanged.Broadcast();
	BroadcastHP();
	BroadcastStamina();
}

void AExceptionCharacter::AddUpgradePoints(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	UpgradePoints += Amount;
	OnProgressionChanged.Broadcast();
}

bool AExceptionCharacter::SpendUpgradePoint(EBRPlayerUpgradeStat UpgradeStat)
{
	if (UpgradePoints <= 0 || CombatState == EBRPlayerCombatState::Dead)
	{
		return false;
	}

	--UpgradePoints;
	++PlayerLevel;

	switch (UpgradeStat)
	{
	case EBRPlayerUpgradeStat::Vitality:
		++VitalityLevel;
		MaxHP += HPPerVitalityLevel;
		CurrentHP = MaxHP;
		BroadcastHP();
		break;
	case EBRPlayerUpgradeStat::Endurance:
		++EnduranceLevel;
		MaxStamina += StaminaPerEnduranceLevel;
		CurrentStamina = MaxStamina;
		BroadcastStamina();
		break;
	case EBRPlayerUpgradeStat::Power:
		++PowerLevel;
		LightAttackDamage += DamagePerPowerLevel;
		HeavyAttackDamage += DamagePerPowerLevel * 1.75f;
		break;
	default:
		break;
	}

	OnProgressionChanged.Broadcast();
	return true;
}

void AExceptionCharacter::RespawnAtCheckpoint()
{
	AExceptionGameMode* ExceptionGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AExceptionGameMode>() : nullptr;
	FTransform RespawnTransform = ExceptionGameMode && ExceptionGameMode->HasCheckpoint()
		? ExceptionGameMode->GetCheckpointTransform()
		: GetActorTransform();
	RespawnTransform.SetScale3D(GetActorScale3D());

	if (ExceptionGameMode)
	{
		ExceptionGameMode->ResetActiveBossArenaForRetry();
	}

	SetActorTransform(RespawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	RestoreHPAndStamina();

	if (AController* CurrentController = GetController())
	{
		CurrentController->SetControlRotation(RespawnTransform.GetRotation().Rotator());
	}

	UE_LOG(LogTemplateCharacter, Log, TEXT("Player respawned at checkpoint: %s"), *RespawnTransform.GetLocation().ToString());
}

void AExceptionCharacter::BroadcastHP()
{
	OnHPChanged.Broadcast(CurrentHP, MaxHP, MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f);
}

void AExceptionCharacter::BroadcastStamina()
{
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, MaxStamina > 0.0f ? CurrentStamina / MaxStamina : 0.0f);
}

void AExceptionCharacter::DrawCombatDebug() const
{
	if (!bShowCombatDebug || !GEngine)
	{
		return;
	}

	const float StaminaPercent = MaxStamina > 0.0f ? CurrentStamina / MaxStamina : 0.0f;
	const float HPPercent = MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f;
	const FString DebugText = FString::Printf(
		TEXT("Player Debug\nState: %s\nHP: %.0f / %.0f (%.0f%%)\nStamina: %.0f / %.0f (%.0f%%)\nLock-on: %s\nInvincible: %s\nParry Active: %s\nLast Attack Hits: %d"),
		*GetCombatStateName(),
		CurrentHP,
		MaxHP,
		HPPercent * 100.0f,
		CurrentStamina,
		MaxStamina,
		StaminaPercent * 100.0f,
		bIsLockedOn ? TEXT("true") : TEXT("false"),
		bIsInvincible ? TEXT("true") : TEXT("false"),
		bIsParryActive ? TEXT("true") : TEXT("false"),
		LastAttackHitCount);

	GEngine->AddOnScreenDebugMessage(1001, 0.0f, FColor::Cyan, DebugText);
}

FString AExceptionCharacter::GetCombatStateName() const
{
	const UEnum* Enum = StaticEnum<EBRPlayerCombatState>();
	return Enum ? Enum->GetNameStringByValue(static_cast<int64>(CombatState)) : TEXT("Unknown");
}

void AExceptionCharacter::RegisterInitialCheckpoint()
{
	if (AExceptionGameMode* ExceptionGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AExceptionGameMode>() : nullptr)
	{
		if (!ExceptionGameMode->HasCheckpoint())
		{
			ExceptionGameMode->SetCheckpointTransform(GetActorTransform());
		}
	}
}
