// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "BRInventoryComponent.h"
#include "BRHiddenStorySubsystem.h"
#include "BRPlayerGraveMarker.h"
#include "ExceptionGameMode.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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
	CancelAttackChain();
	CurrentHP = MaxHP;
	CurrentStamina = MaxStamina;
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	GetWorldTimerManager().ClearTimer(ExecutionTimerHandle);
	GetWorldTimerManager().ClearTimer(ExecHitTimer);
	SetCombatState(EBRPlayerCombatState::Idle);
	bIsInvincible = false;
	bIsParryActive = false;
	PendingExecutionTarget = nullptr;
	ResetExecCam();
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
	CancelAttackChain();
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	GetWorldTimerManager().ClearTimer(ExecutionTimerHandle);
	GetWorldTimerManager().ClearTimer(ExecHitTimer);

	CurrentHP = FMath::Clamp(SavedHP, 1.0f, MaxHP);
	CurrentStamina = FMath::Clamp(SavedStamina, 0.0f, MaxStamina);
	SetCombatState(EBRPlayerCombatState::Idle);
	bIsInvincible = false;
	bIsParryActive = false;
	PendingExecutionTarget = nullptr;
	ResetExecCam();
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
	ApplyLevelStats();
	OnProgressionChanged.Broadcast();
	BroadcastHP();
	BroadcastStamina();
}

void AExceptionCharacter::SaveBaseStats()
{
	if (bBaseStatsSaved)
	{
		return;
	}

	BaseMaxHP = FMath::Max(1.0f, MaxHP - (VitalityLevel * HPPerVitalityLevel));
	BaseMaxStamina = FMath::Max(1.0f, MaxStamina - (EnduranceLevel * StaminaPerEnduranceLevel));
	BaseLightDamage = FMath::Max(0.0f, LightAttackDamage - (PowerLevel * DamagePerPowerLevel));
	BaseHeavyDamage = FMath::Max(0.0f, HeavyAttackDamage - (PowerLevel * DamagePerPowerLevel * 1.75f));
	bBaseStatsSaved = true;
}

void AExceptionCharacter::ApplyLevelStats()
{
	SaveBaseStats();
	MaxHP = BaseMaxHP + (VitalityLevel * HPPerVitalityLevel);
	MaxStamina = BaseMaxStamina + (EnduranceLevel * StaminaPerEnduranceLevel);
	LightAttackDamage = BaseLightDamage + (PowerLevel * DamagePerPowerLevel);
	HeavyAttackDamage = BaseHeavyDamage + (PowerLevel * DamagePerPowerLevel * 1.75f);
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
	if (UpgradePoints <= 0 || CurrentExperience < LevelUpCost || CombatState == EBRPlayerCombatState::Dead)
	{
		return false;
	}

	switch (UpgradeStat)
	{
	case EBRPlayerUpgradeStat::Vitality:
	case EBRPlayerUpgradeStat::Endurance:
	case EBRPlayerUpgradeStat::Power:
		break;
	default:
		return false;
	}

	CurrentExperience -= LevelUpCost;
	--UpgradePoints;
	++PlayerLevel;

	switch (UpgradeStat)
	{
	case EBRPlayerUpgradeStat::Vitality:
		++VitalityLevel;
		ApplyLevelStats();
		CurrentHP = MaxHP;
		BroadcastHP();
		break;
	case EBRPlayerUpgradeStat::Endurance:
		++EnduranceLevel;
		ApplyLevelStats();
		CurrentStamina = MaxStamina;
		BroadcastStamina();
		break;
	case EBRPlayerUpgradeStat::Power:
		++PowerLevel;
		ApplyLevelStats();
		break;
	default:
		return false;
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
	if (!InventoryComponent)
	{
		return;
	}
	if (HasInventoryItem(TEXT("Weapon_MimikatzAuthoritySeized")))
	{
		SetRootWeapon(true);
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

				SetRootWeapon(true);

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(1013, 4.0f, FColor::Purple, TEXT("Hidden Weapon Acquired: Mimikatz, Authority Seized"));
				}
			}
		}
	}
}

bool AExceptionCharacter::TryUseInventoryItem(int32 SlotIndex, const FBRInventorySlot& Slot)
{
	if (Slot.IsEmpty())
	{
		return false;
	}

	switch (Slot.Item.Effect)
	{
	case EBRInventoryItemEffect::HealHP:
		if (CombatState != EBRPlayerCombatState::Idle || CurrentHP >= MaxHP)
		{
			return false;
		}
		return BeginFlaskHeal(Slot.Item.EffectValue);
	case EBRInventoryItemEffect::RestoreStamina:
		if (CombatState == EBRPlayerCombatState::Dead || CurrentStamina >= MaxStamina)
		{
			return false;
		}
		RestoreStamina(Slot.Item.EffectValue);
		return true;
	case EBRInventoryItemEffect::RestoreAll:
		if (CombatState == EBRPlayerCombatState::Dead || (CurrentHP >= MaxHP && CurrentStamina >= MaxStamina))
		{
			return false;
		}
		HealHP(Slot.Item.EffectValue);
		RestoreStamina(Slot.Item.EffectValue);
		return true;
	case EBRInventoryItemEffect::GrantUpgradePoint:
		AddUpgradePoints(FMath::Max(1, FMath::RoundToInt(Slot.Item.EffectValue)));
		return true;
	case EBRInventoryItemEffect::HiddenRootWeapon:
		SetRootWeapon(true);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1012, 2.0f, FColor::Purple, TEXT("Mimikatz, Authority Seized is already bound."));
		}
		return true;
	default:
		return true;
	}
}

FBRInventoryItemDefinition AExceptionCharacter::MakePotionItem() const
{
	FBRInventoryItemDefinition Item;
	Item.ItemId = TEXT("Potion_RuntimeFlask");
	Item.DisplayName = FText::FromString(TEXT("Runtime Chalice"));
	Item.Description = FText::FromString(TEXT("A silver-blue chalice holding stable Runtime memory. Restores HP after a committed drink."));
	Item.Category = EBRInventoryItemCategory::Consumable;
	Item.MaxStack = 9;
	Item.bUsable = true;
	Item.bConsumeOnUse = true;
	Item.Effect = EBRInventoryItemEffect::HealHP;
	Item.EffectValue = 300.0f;
	return Item;
}

bool AExceptionCharacter::BeginFlaskHeal(float HealAmount)
{
	if (!CanStartCombatAction() || HealAmount <= 0.0f || CurrentHP >= MaxHP || !RuntimeFlask)
	{
		return false;
	}

	StopSprint();
	SetCombatState(EBRPlayerCombatState::Healing);
	PendingHealAmount = HealAmount;
	HealNow = 0.0f;
	bHealApplied = false;
	RuntimeFlask->SetHiddenInGame(false);
	// The chalice and the right-hand blade share the grip socket. Keep the
	// equipped weapon state, but hide both blades for the committed drink.
	if (RootBladeR)
	{
		RootBladeR->SetHiddenInGame(true);
	}
	if (RootBladeL)
	{
		RootBladeL->SetHiddenInGame(true);
	}
	RuntimeFlask->SetRelativeLocation(FlaskBaseLocation);
	RuntimeFlask->SetRelativeRotation(FlaskBaseRotation);
	if (FlaskAura)
	{
		FlaskAura->SetIntensity(240.0f);
	}

	const float HealPlayRate = HealAnim && FlaskUseTime > KINDA_SMALL_NUMBER
		? HealAnim->GetPlayLength() / FlaskUseTime
		: 1.0f;
	bHealUsesNotify = false;
	const bool bAnimationStarted = PlayAttackSequence(HealAnim.Get(), HealMontage.Get(), HealPlayRate);
	const UAnimSequenceBase* HealAsset = HealAnim
		? static_cast<UAnimSequenceBase*>(HealAnim.Get())
		: static_cast<UAnimSequenceBase*>(HealMontage.Get());
	const bool bHealEventFiredDuringPlay = bHealUsesNotify;
	bHealUsesNotify = bHealEventFiredDuringPlay
		|| (bAnimationStarted && AnimationUsesEvent(HealAsset, EBRPlayerAnimEvent::Heal));
	if (!bHealUsesNotify)
	{
		GetWorldTimerManager().SetTimer(FlaskHealTimerHandle, this, &AExceptionCharacter::ApplyFlaskHeal, FlaskHealDelay, false);
	}
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AExceptionCharacter::FinishFlaskHeal, FlaskUseTime, false);
	UE_LOG(LogTemplateCharacter, Log, TEXT("Runtime Chalice: drink started, heal in %.2fs / Notify=%s"),
		FlaskHealDelay,
		bHealUsesNotify ? TEXT("true") : TEXT("false"));
	return true;
}

void AExceptionCharacter::ApplyFlaskHeal()
{
	if (CombatState != EBRPlayerCombatState::Healing || bHealApplied)
	{
		return;
	}

	bHealApplied = true;
	HealHP(PendingHealAmount);
	if (!bHealUsesNotify)
	{
		BP_PlayerAnimationEvent(TEXT("Heal"));
	}
	PlayHealSfx();
	if (FlaskAura)
	{
		FlaskAura->SetIntensity(2600.0f);
	}
	UE_LOG(LogTemplateCharacter, Log, TEXT("Runtime Chalice: +%.0f HP"), PendingHealAmount);
}

void AExceptionCharacter::FinishFlaskHeal()
{
	if (CombatState != EBRPlayerCombatState::Healing)
	{
		return;
	}

	PendingHealAmount = 0.0f;
	SetCombatState(EBRPlayerCombatState::Idle);
}

void AExceptionCharacter::UpdateFlaskHeal(float DeltaSeconds)
{
	if (CombatState != EBRPlayerCombatState::Healing || !RuntimeFlask)
	{
		return;
	}

	HealNow += DeltaSeconds;
	const float Alpha = FMath::Clamp(HealNow / FMath::Max(FlaskUseTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float LiftIn = FMath::SmoothStep(0.0f, 0.36f, Alpha);
	const float LiftOut = 1.0f - FMath::SmoothStep(0.72f, 1.0f, Alpha);
	const float Lift = FMath::Min(LiftIn, LiftOut);
	RuntimeFlask->SetRelativeLocation(FlaskBaseLocation + FVector(4.0f, -3.0f, 18.0f) * Lift);
	RuntimeFlask->SetRelativeRotation(FlaskBaseRotation + FRotator(-72.0f * Lift, 8.0f * Lift, 18.0f * Lift));
	if (FlaskAura)
	{
		const float Pulse = 0.78f + 0.22f * FMath::Sin(HealNow * 18.0f);
		const float BaseIntensity = bHealApplied ? 2100.0f : 320.0f + 920.0f * Lift;
		FlaskAura->SetIntensity(BaseIntensity * Pulse);
	}
}

void AExceptionCharacter::CancelFlaskHeal()
{
	GetWorldTimerManager().ClearTimer(FlaskHealTimerHandle);
	HealNow = 0.0f;
	PendingHealAmount = 0.0f;
	bHealApplied = false;
	bHealUsesNotify = false;
	if (RuntimeFlask)
	{
		RuntimeFlask->SetHiddenInGame(true);
		RuntimeFlask->SetRelativeLocation(FlaskBaseLocation);
		RuntimeFlask->SetRelativeRotation(FlaskBaseRotation);
	}
	if (RootBladeR)
	{
		RootBladeR->SetHiddenInGame(!bRootOn);
	}
	if (RootBladeL)
	{
		RootBladeL->SetHiddenInGame(!bRootOn);
	}
	if (FlaskAura)
	{
		FlaskAura->SetIntensity(0.0f);
	}
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
	Item.EffectValue = RootCmdDmg;
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
