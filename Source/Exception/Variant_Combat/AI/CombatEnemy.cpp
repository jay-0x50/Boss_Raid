// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatEnemy.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CombatAIController.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/DamageEvents.h"
#include "CombatLifeBar.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Player/Character/ExceptionCharacter.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/GameplayStatics.h"

ACombatEnemy::ACombatEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// bind the attack montage ended delegate
	OnAttackMontageEnded.BindUObject(this, &ACombatEnemy::AttackMontageEnded);

	// set the AI Controller class by default
	AIControllerClass = ACombatAIController::StaticClass();

	// use an AI Controller regardless of whether we're placed or spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ignore the controller's yaw rotation
	bUseControllerRotationYaw = false;

	// create the life bar
	LifeBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBar"));
	LifeBar->SetupAttachment(RootComponent);

	FieldVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FieldVisual"));
	FieldVisual->SetupAttachment(GetCapsuleComponent());
	FieldVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FieldVisual->SetGenerateOverlapEvents(false);
	FieldVisual->SetCanEverAffectNavigation(false);
	FieldVisual->SetVisibility(false, true);
	FieldVisual->SetHiddenInGame(true);
	FieldMonsterMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Enemies/FieldMonsters/FieldMonster_01/SM_FieldMonster_01.SM_FieldMonster_01")));

	// set the collision capsule size
	GetCapsuleComponent()->SetCapsuleSize(35.0f, 90.0f);

	// set the character movement properties
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	// reset HP to maximum
	CurrentHP = MaxHP;
}

void ACombatEnemy::DoAIComboAttack()
{
	// ignore if we're already playing an attack animation
	if (bIsAttacking)
	{
		return;
	}
	if (bIsStunned || CurrentHP <= 0.0f)
	{
		FinishMissingAttack();
		return;
	}
	if (bUseFieldVisual)
	{
		StartFallbackAttack();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !ComboAttackMontage || ComboSectionNames.IsEmpty())
	{
		StartFallbackAttack();
		return;
	}

	// choose how many times we're going to attack
	TargetComboCount = FMath::RandRange(1, ComboSectionNames.Num());

	// reset the attack counter
	CurrentComboAttack = 0;

	const float MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
	if (MontageLength > 0.0f)
	{
		++AttackGeneration;
		bIsAttacking = true;
		AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);
	}
	else
	{
		StartFallbackAttack();
	}
}

void ACombatEnemy::DoAIChargedAttack()
{
	// ignore if we're already playing an attack animation
	if (bIsAttacking)
	{
		return;
	}
	if (bIsStunned || CurrentHP <= 0.0f)
	{
		FinishMissingAttack();
		return;
	}
	if (bUseFieldVisual)
	{
		StartFallbackAttack();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !ChargedAttackMontage || ChargeLoopSection.IsNone() || ChargeAttackSection.IsNone())
	{
		StartFallbackAttack();
		return;
	}

	// choose how many loops are we going to charge for
	const int32 SafeMinLoops = FMath::Max(1, FMath::Min(MinChargeLoops, MaxChargeLoops));
	const int32 SafeMaxLoops = FMath::Max(SafeMinLoops, FMath::Max(MinChargeLoops, MaxChargeLoops));
	TargetChargeLoops = FMath::RandRange(SafeMinLoops, SafeMaxLoops);

	// reset the charge loop counter
	CurrentChargeLoop = 0;

	const float MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
	if (MontageLength > 0.0f)
	{
		++AttackGeneration;
		bIsAttacking = true;
		AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, ChargedAttackMontage);
	}
	else
	{
		StartFallbackAttack();
	}
}

void ACombatEnemy::AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	FinishAttack();
}

const FVector& ACombatEnemy::GetLastDangerLocation() const
{
	return LastDangerLocation;
}

float ACombatEnemy::GetLastDangerTime() const
{
	return LastDangerTime;
}

bool ACombatEnemy::ShouldTargetPlayer(const AActor* PlayerActor)
{
	if (!PlayerActor || CurrentHP <= 0.0f)
	{
		bHasAggro = false;
		return false;
	}

	const float ChaseDistance = FMath::Max(100.0f, MaxChaseDistanceFromHome);
	const float DistanceFromHomeSquared = FVector::DistSquared2D(GetActorLocation(), HomeLocation);
	const float PlayerDistanceFromHomeSquared = FVector::DistSquared2D(PlayerActor->GetActorLocation(), HomeLocation);
	if (DistanceFromHomeSquared > FMath::Square(ChaseDistance) || PlayerDistanceFromHomeSquared > FMath::Square(ChaseDistance))
	{
		bHasAggro = false;
		return false;
	}

	const float PlayerDistanceSquared = FVector::DistSquared2D(PlayerActor->GetActorLocation(), GetActorLocation());
	const float TargetRange = bHasAggro ? FMath::Max(AggroDistance, LoseInterestDistance) : FMath::Max(100.0f, AggroDistance);
	bHasAggro = PlayerDistanceSquared <= FMath::Square(TargetRange);
	return bHasAggro;
}

void ACombatEnemy::DoAttackTrace(FName DamageSourceBone)
{
	// sweep for objects in front of the character to be hit by the attack
	TArray<FHitResult> OutHits;

	// start at the authored socket when available, otherwise use the active field visual/capsule
	FVector TraceStart = GetActorLocation();
	if (bUseFieldVisual && FieldVisual)
	{
		TraceStart = FieldVisual->Bounds.Origin;
	}
	else if (GetMesh() && !DamageSourceBone.IsNone() && GetMesh()->DoesSocketExist(DamageSourceBone))
	{
		TraceStart = GetMesh()->GetSocketLocation(DamageSourceBone);
	}
	else if (GetCapsuleComponent())
	{
		TraceStart = GetCapsuleComponent()->GetComponentLocation();
	}
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * MeleeTraceDistance);

	// enemies only affect Pawn collision objects; they don't knock back boxes
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	// use a sphere shape for the sweep
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(MeleeTraceRadius);

	// ignore self
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	TSet<const AActor*> DamagedActors;

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		// iterate over each object hit
		for (const FHitResult& CurrentHit : OutHits)
		{
			AActor* HitActor = CurrentHit.GetActor();
			if (!HitActor || DamagedActors.Contains(HitActor))
			{
				continue;
			}

			/** does the actor have the player tag? */
			if (HitActor->ActorHasTag(FName("Player")))
			{
				// check if the actor is damageable
				ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor);

				if (Damageable)
				{
					DamagedActors.Add(HitActor);
					// knock upwards and away from the impact normal
					const FVector Impulse = (CurrentHit.ImpactNormal * -MeleeKnockbackImpulse) + (FVector::UpVector * MeleeLaunchImpulse);

					// pass the damage event to the actor
					Damageable->ApplyDamage(MeleeDamage, this, CurrentHit.ImpactPoint, Impulse);

				}
			}
		}
	}
}

void ACombatEnemy::CheckCombo()
{
	if (!bIsAttacking || !ComboAttackMontage || ComboSectionNames.IsEmpty())
	{
		return;
	}

	// increase the combo counter
	++CurrentComboAttack;

	// do we still have attacks to play in this string?
	if (CurrentComboAttack < TargetComboCount && ComboSectionNames.IsValidIndex(CurrentComboAttack))
	{
		// jump to the next attack section
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_JumpToSection(ComboSectionNames[CurrentComboAttack], ComboAttackMontage);
		}
	}
}

void ACombatEnemy::CheckChargedAttack()
{
	if (!bIsAttacking || !ChargedAttackMontage)
	{
		return;
	}

	// increase the charge loop counter
	++CurrentChargeLoop;

	// jump to either the loop or attack section of the montage depending on whether we hit the loop target
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(CurrentChargeLoop >= TargetChargeLoops ? ChargeAttackSection : ChargeLoopSection, ChargedAttackMontage);
	}
}

void ACombatEnemy::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	
	// pass the damage event to the actor
	FDamageEvent DamageEvent;
	const float ActualDamage = TakeDamage(Damage, DamageEvent, nullptr, DamageCauser);

	// only process knockback and effects if we received nonzero damage
	if (ActualDamage > 0.0f)
	{
		if (DamageCauser && DamageCauser->ActorHasTag(FName("Player")))
		{
			bHasAggro = true;
		}

		// apply the knockback impulse
		GetCharacterMovement()->AddImpulse(DamageImpulse, true);

		// is the character ragdolling?
		if (GetMesh()->IsSimulatingPhysics())
		{
			// apply an impulse to the ragdoll
			GetMesh()->AddImpulseAtLocation(DamageImpulse * GetMesh()->GetMass(), DamageLocation);
		}

		// stop the attack montages to interrupt the attack
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.1f, ComboAttackMontage);
			AnimInstance->Montage_Stop(0.1f, ChargedAttackMontage);
		}
		FinishAttack();

		if (CurrentHP > 0.0f && HitStunDuration > 0.0f)
		{
			bIsStunned = true;
			GetCharacterMovement()->StopMovementImmediately();
			GetCharacterMovement()->DisableMovement();
			GetWorldTimerManager().ClearTimer(StunTimer);
			GetWorldTimerManager().SetTimer(StunTimer, this, &ACombatEnemy::EndHitStun, HitStunDuration, false);
		}

		// pass control to BP to play effects, etc.
		ReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
	}
}

void ACombatEnemy::HandleDeath()
{
	GetWorldTimerManager().ClearTimer(StunTimer);
	bIsStunned = false;

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->Montage_Stop(0.1f, ComboAttackMontage);
		AnimInstance->Montage_Stop(0.1f, ChargedAttackMontage);
	}
	FinishAttack();
	if (bUseFieldVisual)
	{
		SetActorTickEnabled(false);
	}

	// hide the life bar
	if (LifeBar)
	{
		LifeBar->SetHiddenInGame(true);
	}

	// disable the collision capsule to avoid being hit again while dead
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// disable character movement
	GetCharacterMovement()->DisableMovement();

	// enable full ragdoll physics
	if (!bUseFieldVisual && GetMesh() && GetMesh()->GetPhysicsAsset())
	{
		GetMesh()->SetSimulatePhysics(true);
	}

	// call the died delegate to notify any subscribers
	OnEnemyDiedNative.Broadcast(this);
	OnEnemyDied.Broadcast();

	if (AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(LastDamageCauser))
	{
		PlayerCharacter->AddExperience(ExperienceReward);
	}

	// set up the death timer
	if (DeathRemovalTime <= 0.0f)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ACombatEnemy::RemoveFromLevel);
	}
	else
	{
		GetWorldTimerManager().SetTimer(DeathTimer, this, &ACombatEnemy::RemoveFromLevel, DeathRemovalTime, false);
	}
}

void ACombatEnemy::ApplyHealing(float Healing, AActor* Healer)
{
	if (CurrentHP <= 0.0f || Healing <= 0.0f)
	{
		return;
	}

	CurrentHP = FMath::Min(MaxHP, CurrentHP + Healing);
	if (LifeBarWidget)
	{
		LifeBarWidget->SetLifePercentage(CurrentHP / FMath::Max(MaxHP, UE_SMALL_NUMBER));
	}
}

void ACombatEnemy::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// ensure we're being attacked by the player
	if (DangerSource && DangerSource->ActorHasTag(FName("Player")))
	{
		bHasAggro = true;
		// save the danger location and game time
		LastDangerLocation = DangerLocation;
		LastDangerTime = GetWorld()->GetTimeSeconds();
	}
}

void ACombatEnemy::RemoveFromLevel()
{
	// destroy this actor
	Destroy();
}

void ACombatEnemy::StartFallbackAttack()
{
	if (bIsAttacking || CurrentHP <= 0.0f)
	{
		FinishMissingAttack();
		return;
	}

	bIsAttacking = true;
	++AttackGeneration;
	bFallbackAttackPlaying = true;
	bFallbackAttackHitDone = false;
	FallbackAttackElapsed = 0.0f;
	bTickEnabledBeforeFallbackAttack = IsActorTickEnabled();
	SetActorTickEnabled(true);
}

void ACombatEnemy::UpdateFieldMonster(float DeltaSeconds)
{
	if (!FieldVisual)
	{
		return;
	}

	FieldMotionTime += DeltaSeconds;
	FTransform VisualTransform = FieldVisualBaseTransform;
	float HopHeight = 0.0f;
	float LungeDistance = 0.0f;
	float ShapeAlpha = 0.0f;

	if (bFallbackAttackPlaying)
	{
		const float AttackAlpha = FMath::Clamp(FallbackAttackElapsed / FMath::Max(0.15f, FallbackAttackDuration), 0.0f, 1.0f);
		const float AttackArc = FMath::Sin(AttackAlpha * UE_PI);
		HopHeight = IdleHopHeight * 2.2f * AttackArc;
		LungeDistance = FallbackAttackLunge * AttackArc;
		ShapeAlpha = AttackArc;
	}
	else if (CurrentHP > 0.0f)
	{
		const float IdleWave = FMath::Sin(FieldMotionTime * FMath::Max(0.1f, IdleHopFrequency) * 2.0f * UE_PI);
		HopHeight = FMath::Max(0.0f, IdleWave) * IdleHopHeight;
		ShapeAlpha = IdleWave;
	}

	VisualTransform.AddToTranslation(FVector(LungeDistance, 0.0f, HopHeight));
	const FVector BaseScale = FieldVisualBaseTransform.GetScale3D();
	VisualTransform.SetScale3D(BaseScale * FVector(1.0f - (ShapeAlpha * 0.06f), 1.0f - (ShapeAlpha * 0.06f), 1.0f + (ShapeAlpha * 0.12f)));
	FieldVisual->SetRelativeTransform(VisualTransform);

	if (bFallbackAttackPlaying || bIsStunned || CurrentHP <= 0.0f || DirectMoveFallbackSpeed <= 0.0f)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bShouldChase = ShouldTargetPlayer(PlayerPawn);
	const FVector MoveTarget = bShouldChase && PlayerPawn ? PlayerPawn->GetActorLocation() : HomeLocation;
	const FVector ToTarget = MoveTarget - GetActorLocation();
	const float StopDistance = bShouldChase ? FMath::Max(100.0f, MeleeTraceDistance + (MeleeTraceRadius * 0.75f)) : 50.0f;

	AAIController* AIController = Cast<AAIController>(GetController());
	const bool bAIPathIsMoving = AIController && AIController->GetMoveStatus() == EPathFollowingStatus::Moving;
	if (!bShouldChase && AIController)
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	if (!bAIPathIsMoving && !ToTarget.IsNearlyZero())
	{
		const FRotator DesiredRotation(0.0f, ToTarget.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, 8.0f));
	}

	if (!bAIPathIsMoving && ToTarget.SizeSquared2D() > FMath::Square(StopDistance))
	{
		GetCharacterMovement()->MaxWalkSpeed = DirectMoveFallbackSpeed;
		AddMovementInput(ToTarget.GetSafeNormal2D(), 1.0f);
	}
}

void ACombatEnemy::RefreshVisualMode()
{
	if (!FieldVisual || !GetMesh())
	{
		return;
	}

	if (bUseFieldVisual && !FieldVisual->GetStaticMesh() && !FieldMonsterMesh.IsNull())
	{
		FieldVisual->SetStaticMesh(FieldMonsterMesh.LoadSynchronous());
	}

	FieldVisual->SetVisibility(bUseFieldVisual, true);
	FieldVisual->SetHiddenInGame(!bUseFieldVisual);
	GetMesh()->SetVisibility(!bUseFieldVisual, true);
	GetMesh()->SetHiddenInGame(bUseFieldVisual);
	GetMesh()->SetComponentTickEnabled(!bUseFieldVisual);
}

void ACombatEnemy::FinishAttack()
{
	if (!bIsAttacking)
	{
		return;
	}

	bIsAttacking = false;
	bFallbackAttackPlaying = false;
	bFallbackAttackHitDone = false;
	FallbackAttackElapsed = 0.0f;
	if (FieldVisual)
	{
		FieldVisual->SetRelativeTransform(FieldVisualBaseTransform);
	}
	if (!bUseFieldVisual)
	{
		SetActorTickEnabled(bTickEnabledBeforeFallbackAttack);
	}
	OnAttackCompleted.ExecuteIfBound();
}

void ACombatEnemy::FinishMissingAttack()
{
	if (!GetWorld())
	{
		return;
	}

	const uint32 PendingGeneration = ++AttackGeneration;
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, PendingGeneration]()
	{
		if (AttackGeneration == PendingGeneration && !bIsAttacking)
		{
			OnAttackCompleted.ExecuteIfBound();
		}
	}));
}

void ACombatEnemy::EndHitStun()
{
	if (CurrentHP <= 0.0f)
	{
		return;
	}

	bIsStunned = false;
	GetWorldTimerManager().ClearTimer(StunTimer);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

float ACombatEnemy::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// only process damage if the character is still alive
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(CurrentHP, FMath::Max(0.0f, Damage));
	if (AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	LastDamageCauser = DamageCauser;

	// reduce the current HP
	CurrentHP -= AppliedDamage;

	// have we run out of HP?
	if (CurrentHP <= 0.0f)
	{
		// die
		HandleDeath();
	}
	else
	{
		// update the life bar
		if (LifeBarWidget)
		{
			LifeBarWidget->SetLifePercentage(CurrentHP / FMath::Max(MaxHP, UE_SMALL_NUMBER));
		}

		// enable partial ragdoll physics, but keep the pelvis vertical
		if (!bUseFieldVisual)
		{
			GetMesh()->SetPhysicsBlendWeight(0.5f);
			GetMesh()->SetBodySimulatePhysics(PelvisBoneName, false);
		}
	}

	// return the received damage amount
	return AppliedDamage;
}

void ACombatEnemy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// is the character still alive?
	if (!bUseFieldVisual && CurrentHP > 0.0f)
	{
		// disable ragdoll physics
		GetMesh()->SetPhysicsBlendWeight(0.0f);
	}

	// call the landed Delegate for StateTree
	OnEnemyLanded.ExecuteIfBound();
}

void ACombatEnemy::BeginPlay()
{
	// reset HP to maximum
	MaxHP = FMath::Max(MaxHP, UE_SMALL_NUMBER);
	CurrentHP = MaxHP;

	// we top the HP before BeginPlay so StateTree picks it up at the right value
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	RefreshVisualMode();
	if (FieldVisual)
	{
		FieldVisualBaseTransform = FieldVisual->GetRelativeTransform();
	}
	if (bUseFieldVisual)
	{
		SetActorTickEnabled(true);
	}

	// get the life bar widget from the widget comp
	LifeBarWidget = LifeBar ? Cast<UCombatLifeBar>(LifeBar->GetUserWidgetObject()) : nullptr;

	// fill the life bar
	if (LifeBarWidget)
	{
		LifeBarWidget->SetLifePercentage(1.0f);
	}
	else if (LifeBar)
	{
		LifeBar->SetHiddenInGame(true);
		UE_LOG(LogTemp, Warning, TEXT("Combat enemy %s has no CombatLifeBar widget; the enemy will continue without a life bar."), *GetNameSafe(this));
	}
}

void ACombatEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFallbackAttackPlaying)
	{
		FallbackAttackElapsed += DeltaSeconds;
		const float AttackDuration = FMath::Max(0.15f, FallbackAttackDuration);
		const float AttackAlpha = FMath::Clamp(FallbackAttackElapsed / AttackDuration, 0.0f, 1.0f);
		if (!bFallbackAttackHitDone && AttackAlpha >= FMath::Clamp(FallbackAttackHitTime, 0.05f, 0.95f))
		{
			bFallbackAttackHitDone = true;
			DoAttackTrace(NAME_None);
		}

		if (FallbackAttackElapsed >= AttackDuration)
		{
			FinishAttack();
		}
	}

	if (bUseFieldVisual)
	{
		UpdateFieldMonster(DeltaSeconds);
	}
}

void ACombatEnemy::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisualMode();
	if (FieldVisual)
	{
		FieldVisualBaseTransform = FieldVisual->GetRelativeTransform();
	}
}

void ACombatEnemy::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the death timer
	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);
	GetWorld()->GetTimerManager().ClearTimer(StunTimer);
	OnEnemyDiedNative.Clear();
	++AttackGeneration;
	OnAttackCompleted.Unbind();
	OnEnemyLanded.Unbind();
}
