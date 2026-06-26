// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "BRInventoryComponent.h"
#include "BRHiddenStorySubsystem.h"
#include "BRPlayerGraveMarker.h"
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

void AExceptionCharacter::HealHP(float Amount)
{
	if (Amount <= 0.0f || CombatState == EBRPlayerCombatState::Dead)
	{
		return;
	}

	CurrentHP = FMath::Min(MaxHP, CurrentHP + Amount);
	BroadcastHP();
}

void AExceptionCharacter::RestoreStamina(float Amount)
{
	if (Amount <= 0.0f || CombatState == EBRPlayerCombatState::Dead)
	{
		return;
	}

	CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + Amount);
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

void AExceptionCharacter::ApplySavedExperience(int32 SavedCurrentExperience, int32 SavedDroppedExperience)
{
	CurrentExperience = FMath::Max(0, SavedCurrentExperience);
	DroppedExperience = FMath::Max(0, SavedDroppedExperience);
	OnProgressionChanged.Broadcast();
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

void AExceptionCharacter::AddExperience(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CurrentExperience += Amount;
	OnProgressionChanged.Broadcast();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1014, 1.5f, FColor::Green, FString::Printf(TEXT("+%d XP"), Amount));
	}
}

int32 AExceptionCharacter::DropCurrentExperience()
{
	const int32 ExperienceToDrop = FMath::Max(0, CurrentExperience);
	CurrentExperience = 0;
	DroppedExperience = ExperienceToDrop;
	OnProgressionChanged.Broadcast();
	return ExperienceToDrop;
}

void AExceptionCharacter::RecoverDroppedExperience(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CurrentExperience += Amount;
	DroppedExperience = FMath::Max(0, DroppedExperience - Amount);
	OnProgressionChanged.Broadcast();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1015, 2.0f, FColor::Green, FString::Printf(TEXT("Recovered %d XP"), Amount));
	}
}

int32 AExceptionCharacter::GetLevelUpExperienceCost() const
{
	return BaseLevelUpExperienceCost + FMath::Max(0, PlayerLevel - 1) * LevelUpExperienceCostIncrease;
}

void AExceptionCharacter::AwardBossVictoryRewards(AActor* DefeatedBoss)
{
	AddUpgradePoints(BossKillUpgradePointReward);
	AddExperience(BossKillUpgradePointReward * GetLevelUpExperienceCost());

	if (InventoryComponent)
	{
		int32 RemainingQuantity = 0;
		InventoryComponent->AddItem(MakePotionItem(), 1, RemainingQuantity);
	}

	if (GEngine)
	{
		const FString BossName = DefeatedBoss ? DefeatedBoss->GetName() : TEXT("Boss");
		const FString RewardText = FString::Printf(TEXT("%s defeated: +%d Upgrade Point, +1 Potion"), *BossName, BossKillUpgradePointReward);
		GEngine->AddOnScreenDebugMessage(1011, 2.5f, FColor::Green, RewardText);
	}
}

bool AExceptionCharacter::SpendUpgradePoint(EBRPlayerUpgradeStat UpgradeStat)
{
	const int32 LevelUpCost = GetLevelUpExperienceCost();
	if (CurrentExperience < LevelUpCost || CombatState == EBRPlayerCombatState::Dead)
	{
		return false;
	}

	CurrentExperience -= LevelUpCost;
	UpgradePoints = FMath::Max(0, UpgradePoints - 1);
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

void AExceptionCharacter::GrantDefaultLoadout()
{
	if (!InventoryComponent)
	{
		return;
	}

	int32 RemainingQuantity = 0;
	InventoryComponent->AddItem(MakePotionItem(), 5, RemainingQuantity);
	InventoryComponent->AddItem(MakeStaminaItem(), 3, RemainingQuantity);
	RefreshHiddenStoryRewards();

	const int32 PotionSlot = InventoryComponent->FindFirstItemSlot(TEXT("Potion_RuntimeFlask"));
	if (PotionSlot != INDEX_NONE && PotionSlot != 20)
	{
		InventoryComponent->MoveSlot(PotionSlot, 20);
	}

	const int32 StaminaSlot = InventoryComponent->FindFirstItemSlot(TEXT("Potion_ThreadSpark"));
	if (StaminaSlot != INDEX_NONE && StaminaSlot != 21)
	{
		InventoryComponent->MoveSlot(StaminaSlot, 21);
	}

	const int32 HiddenWeaponSlot = InventoryComponent->FindFirstItemSlot(TEXT("Weapon_MimikatzAuthoritySeized"));
	if (HiddenWeaponSlot != INDEX_NONE && HiddenWeaponSlot != 22)
	{
		InventoryComponent->MoveSlot(HiddenWeaponSlot, 22);
	}
}

bool AExceptionCharacter::HasInventoryItem(FName ItemId) const
{
	return InventoryComponent && InventoryComponent->FindFirstItemSlot(ItemId) != INDEX_NONE;
}

void AExceptionCharacter::CompleteNelHiddenRequest(FName RequestId)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			HiddenStory->MarkNelHiddenRequestCompleted(RequestId);
			RefreshHiddenStoryRewards();
		}
	}
}

void AExceptionCharacter::CollectHiddenFragment(int32 Amount)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			HiddenStory->CollectHiddenFragment(Amount);
			RefreshHiddenStoryRewards();
		}
	}
}

void AExceptionCharacter::RefreshHiddenStoryRewards()
{
	if (!InventoryComponent || HasInventoryItem(TEXT("Weapon_MimikatzAuthoritySeized")))
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			if (HiddenStory->IsMimikatzAuthoritySeizedUnlocked())
			{
				int32 RemainingQuantity = 0;
				InventoryComponent->AddItem(MakeHiddenRootWeaponItem(), 1, RemainingQuantity);

				const int32 HiddenWeaponSlot = InventoryComponent->FindFirstItemSlot(TEXT("Weapon_MimikatzAuthoritySeized"));
				if (HiddenWeaponSlot != INDEX_NONE && HiddenWeaponSlot != 22)
				{
					InventoryComponent->MoveSlot(HiddenWeaponSlot, 22);
				}

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(1013, 4.0f, FColor::Purple, TEXT("Hidden Weapon Acquired: Mimikatz, Authority Seized"));
				}
			}
		}
	}
}

void AExceptionCharacter::HandleInventoryItemUsed(int32 SlotIndex, const FBRInventorySlot& Slot)
{
	if (Slot.IsEmpty())
	{
		return;
	}

	switch (Slot.Item.Effect)
	{
	case EBRInventoryItemEffect::HealHP:
		HealHP(Slot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::RestoreStamina:
		RestoreStamina(Slot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::RestoreAll:
		HealHP(Slot.Item.EffectValue);
		RestoreStamina(Slot.Item.EffectValue);
		break;
	case EBRInventoryItemEffect::GrantUpgradePoint:
		AddUpgradePoints(FMath::Max(1, FMath::RoundToInt(Slot.Item.EffectValue)));
		break;
	case EBRInventoryItemEffect::HiddenRootWeapon:
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1012, 2.0f, FColor::Purple, TEXT("Mimikatz, Authority Seized is already bound."));
		}
		break;
	default:
		break;
	}
}

FBRInventoryItemDefinition AExceptionCharacter::MakePotionItem() const
{
	FBRInventoryItemDefinition Item;
	Item.ItemId = TEXT("Potion_RuntimeFlask");
	Item.DisplayName = FText::FromString(TEXT("Runtime Flask"));
	Item.Description = FText::FromString(TEXT("Restores HP. A small patch of stable runtime memory."));
	Item.Category = EBRInventoryItemCategory::Consumable;
	Item.MaxStack = 9;
	Item.bUsable = true;
	Item.bConsumeOnUse = true;
	Item.Effect = EBRInventoryItemEffect::HealHP;
	Item.EffectValue = 300.0f;
	return Item;
}

FBRInventoryItemDefinition AExceptionCharacter::MakeStaminaItem() const
{
	FBRInventoryItemDefinition Item;
	Item.ItemId = TEXT("Potion_ThreadSpark");
	Item.DisplayName = FText::FromString(TEXT("Thread Spark"));
	Item.Description = FText::FromString(TEXT("Restores stamina. Useful before a long dodge chain."));
	Item.Category = EBRInventoryItemCategory::Consumable;
	Item.MaxStack = 9;
	Item.bUsable = true;
	Item.bConsumeOnUse = true;
	Item.Effect = EBRInventoryItemEffect::RestoreStamina;
	Item.EffectValue = 55.0f;
	return Item;
}

FBRInventoryItemDefinition AExceptionCharacter::MakeHiddenRootWeaponItem() const
{
	FBRInventoryItemDefinition Item;
	Item.ItemId = TEXT("Weapon_MimikatzAuthoritySeized");
	Item.DisplayName = FText::FromString(TEXT("Mimikatz, Authority Seized"));
	Item.Description = FText::FromString(TEXT("Hidden root weapon. Deals heavy authority damage to CMD."));
	Item.Category = EBRInventoryItemCategory::Equipment;
	Item.MaxStack = 1;
	Item.bUsable = true;
	Item.bConsumeOnUse = false;
	Item.Effect = EBRInventoryItemEffect::HiddenRootWeapon;
	Item.EffectValue = HiddenRootWeaponCMDDamageMultiplier;
	return Item;
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

void AExceptionCharacter::SpawnPlayerGraveMarker()
{
	if (!GetWorld())
	{
		return;
	}

	UClass* GraveClass = PlayerGraveClass ? PlayerGraveClass.Get() : ABRPlayerGraveMarker::StaticClass();
	const int32 ExperienceToDrop = DropCurrentExperience();
	FTransform GraveTransform = GetActorTransform();
	GraveTransform.SetLocation(GetActorLocation() + FVector(0.0f, 0.0f, 70.0f));
	GraveTransform.SetRotation(FRotator(0.0f, GetActorRotation().Yaw, 0.0f).Quaternion());
	GraveTransform.SetScale3D(FVector::OneVector);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	if (ABRPlayerGraveMarker* GraveMarker = GetWorld()->SpawnActor<ABRPlayerGraveMarker>(GraveClass, GraveTransform, SpawnParams))
	{
		GraveMarker->SetStoredExperience(ExperienceToDrop);
	}
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
