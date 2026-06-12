// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "Boss/Base/BRBossBase.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

void AExceptionCharacter::ToggleLockOn()
{
	if (bIsLockedOn)
	{
		ClearLockOn();
		return;
	}

	LockOnTarget = FindLockOnTarget();
	if (!LockOnTarget)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1005, 1.0f, FColor::Silver, TEXT("No Lock-on Target"));
		}
		return;
	}

	bIsLockedOn = true;
	LockOnYawOffset = 0.0f;
	LockOnPitchOffset = 0.0f;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	CameraBoom->TargetArmLength = LockOnCameraArmLength;
	CameraBoom->TargetOffset = LockOnCameraTargetOffset;
	CameraBoom->SocketOffset = LockOnCameraSocketOffset;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1006, 1.0f, FColor::Cyan, TEXT("Lock-on Enabled"));
	}
}

void AExceptionCharacter::ClearLockOn()
{
	if (!bIsLockedOn && !LockOnTarget)
	{
		return;
	}

	bIsLockedOn = false;
	LockOnTarget = nullptr;
	LockOnYawOffset = 0.0f;
	LockOnPitchOffset = 0.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	CameraBoom->TargetArmLength = FreeCameraArmLength;
	CameraBoom->TargetOffset = FVector::ZeroVector;
	CameraBoom->SocketOffset = FVector::ZeroVector;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1007, 1.0f, FColor::Silver, TEXT("Lock-on Disabled"));
	}
}

bool AExceptionCharacter::TryExecution()
{
	if (!CanStartCombatAction())
	{
		return false;
	}

	ABRBossBase* ExecutionTarget = FindExecutionTarget();
	if (!ExecutionTarget)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1010, 1.0f, FColor::Silver, TEXT("No Execution Target"));
		}
		return false;
	}

	StartExecution(ExecutionTarget);
	return true;
}

AActor* AExceptionCharacter::FindLockOnTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> Bosses;
	UGameplayStatics::GetAllActorsOfClass(this, ABRBossBase::StaticClass(), Bosses);

	AActor* BestTarget = nullptr;
	float BestDistanceSq = FMath::Square(LockOnRange);
	const FVector TraceStart = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();

	for (AActor* Candidate : Bosses)
	{
		ABRBossBase* Boss = Cast<ABRBossBase>(Candidate);
		if (!Boss || Boss->IsDead())
		{
			continue;
		}

		const FVector TargetFocus = Boss->GetActorLocation() + FVector(0.0f, 0.0f, LockOnTargetHeightOffset);
		const float DistanceSq = FVector::DistSquared(GetActorLocation(), Boss->GetActorLocation());
		if (DistanceSq > BestDistanceSq)
		{
			continue;
		}

		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ExceptionLockOnTrace), false, this);
		QueryParams.AddIgnoredActor(Boss);
		const bool bBlocked = World->LineTraceSingleByChannel(Hit, TraceStart, TargetFocus, ECC_Visibility, QueryParams);
		if (bBlocked)
		{
			continue;
		}

		BestTarget = Boss;
		BestDistanceSq = DistanceSq;
	}

	return BestTarget;
}

static float GetExecutionDistanceSq2D(const AExceptionCharacter* Player, const ABRBossBase* Boss)
{
	if (!Player || !Boss)
	{
		return TNumericLimits<float>::Max();
	}

	const FVector PlayerLocation = Player->GetActorLocation();
	const FBox BossBounds = Boss->GetComponentsBoundingBox(true);
	const FVector ClosestPoint = BossBounds.IsValid ? BossBounds.GetClosestPointTo(PlayerLocation) : Boss->GetActorLocation();
	return FVector::DistSquared2D(PlayerLocation, ClosestPoint);
}

ABRBossBase* AExceptionCharacter::FindExecutionTarget() const
{
	ABRBossBase* BestTarget = Cast<ABRBossBase>(LockOnTarget);
	if (BestTarget && BestTarget->CanBeExecuted() && GetExecutionDistanceSq2D(this, BestTarget) <= FMath::Square(ExecutionRange))
	{
		return BestTarget;
	}

	TArray<AActor*> Bosses;
	UGameplayStatics::GetAllActorsOfClass(this, ABRBossBase::StaticClass(), Bosses);

	float BestDistanceSq = FMath::Square(ExecutionRange);
	BestTarget = nullptr;
	for (AActor* Candidate : Bosses)
	{
		ABRBossBase* Boss = Cast<ABRBossBase>(Candidate);
		if (!Boss || !Boss->CanBeExecuted())
		{
			continue;
		}

		const float DistanceSq = GetExecutionDistanceSq2D(this, Boss);
		if (DistanceSq <= BestDistanceSq)
		{
			BestTarget = Boss;
			BestDistanceSq = DistanceSq;
		}
	}

	return BestTarget;
}

void AExceptionCharacter::StartExecution(ABRBossBase* Target)
{
	if (!Target || !Target->BeginExecution(this))
	{
		return;
	}

	PendingExecutionTarget = Target;
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	bIsInvincible = true;
	bIsParryActive = false;
	SetCombatState(EBRPlayerCombatState::Execution);
	ClearLockOn();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	const FVector ToPlayer = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	const FVector SnapDirection = ToPlayer.IsNearlyZero() ? -Target->GetActorForwardVector() : ToPlayer;
	const FVector SnapLocation = Target->GetActorLocation() + (SnapDirection * ExecutionSnapDistance);
	SetActorLocation(SnapLocation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotationMatrix::MakeFromX(FVector(ToTarget.X, ToTarget.Y, 0.0f)).Rotator());
	Target->SetActorRotation(FRotationMatrix::MakeFromX(FVector(-ToTarget.X, -ToTarget.Y, 0.0f)).Rotator());

	PlayOptionalMontage(ExecutionMontage);
	BP_ExecutionStarted(Target);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1011, 1.2f, FColor::Purple, TEXT("Player Execution"));
	}

	GetWorldTimerManager().SetTimer(ExecutionTimerHandle, this, &AExceptionCharacter::FinishExecution, ExecutionDuration, false);
}

void AExceptionCharacter::FinishExecution()
{
	ABRBossBase* Target = PendingExecutionTarget.Get();
	float AppliedDamage = 0.0f;
	if (Target && !Target->IsDead())
	{
		AppliedDamage = Target->GetMaxHP() * ExecutionDamageMaxHPRatio;
		Target->CompleteExecution(AppliedDamage, this);
	}

	PendingExecutionTarget = nullptr;
	bIsInvincible = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetCombatState(EBRPlayerCombatState::Idle);
	BP_ExecutionFinished(Target, AppliedDamage);
}

void AExceptionCharacter::UpdateLockOn(float DeltaSeconds)
{
	if (!bIsLockedOn)
	{
		return;
	}

	ABRBossBase* Boss = Cast<ABRBossBase>(LockOnTarget);
	if (!Boss || Boss->IsDead() || CombatState == EBRPlayerCombatState::Dead)
	{
		ClearLockOn();
		return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), Boss->GetActorLocation());
	if (DistanceToTarget > LockOnBreakRange)
	{
		ClearLockOn();
		return;
	}

	const FVector TargetFocus = Boss->GetActorLocation() + FVector(0.0f, 0.0f, LockOnTargetHeightOffset);
	const FVector CameraLocation = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
	FRotator DesiredControlRotation = (TargetFocus - CameraLocation).Rotation();
	DesiredControlRotation.Yaw += LockOnYawOffset;
	DesiredControlRotation.Pitch = FMath::Clamp(DesiredControlRotation.Pitch + LockOnPitchOffset, -85.0f, 85.0f);

	if (AController* CurrentController = GetController())
	{
		const FRotator NewControlRotation = FMath::RInterpTo(CurrentController->GetControlRotation(), DesiredControlRotation, DeltaSeconds, LockOnRotationInterpSpeed);
		CurrentController->SetControlRotation(NewControlRotation);
	}

	LockOnYawOffset = FMath::FInterpTo(LockOnYawOffset, 0.0f, DeltaSeconds, LockOnOffsetReturnSpeed);
	LockOnPitchOffset = FMath::FInterpTo(LockOnPitchOffset, 0.0f, DeltaSeconds, LockOnOffsetReturnSpeed);

	const FVector ToTarget = Boss->GetActorLocation() - GetActorLocation();
	const FRotator DesiredActorRotation = FRotationMatrix::MakeFromX(FVector(ToTarget.X, ToTarget.Y, 0.0f)).Rotator();
	const FRotator NewActorRotation = FMath::RInterpTo(GetActorRotation(), DesiredActorRotation, DeltaSeconds, LockOnCharacterRotationInterpSpeed);
	SetActorRotation(NewActorRotation);
}
