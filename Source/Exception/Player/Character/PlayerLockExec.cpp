// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "Boss/Base/BRBossBase.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_Combat/AI/CombatEnemy.h"

namespace
{
	bool IsLockOnActorAlive(const AActor* Target)
	{
		if (!IsValid(Target) || Target->IsActorBeingDestroyed())
		{
			return false;
		}

		if (const ABRBossBase* Boss = Cast<ABRBossBase>(Target))
		{
			return !Boss->IsDead();
		}

		if (const ACombatEnemy* Enemy = Cast<ACombatEnemy>(Target))
		{
			return Enemy->CurrentHP > 0.0f;
		}

		return false;
	}

	void GatherLockOnCandidates(const UObject* WorldContextObject, TArray<AActor*>& OutTargets)
	{
		TArray<AActor*> Bosses;
		UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ABRBossBase::StaticClass(), Bosses);
		OutTargets.Append(Bosses);

		TArray<AActor*> FieldEnemies;
		UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ACombatEnemy::StaticClass(), FieldEnemies);
		OutTargets.Append(FieldEnemies);
	}
}

void AExceptionCharacter::ToggleLockOn()
{
	if (CombatState == EBRPlayerCombatState::Dead)
	{
		ClearLockOn();
		return;
	}

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
	LockOnOccludedTime = 0.0f;
	bLockOnSwitchInputReady = true;
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
	LockOnOccludedTime = 0.0f;
	bLockOnSwitchInputReady = true;
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
	if (!GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> Candidates;
	GatherLockOnCandidates(this, Candidates);

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	const FVector CameraLocation = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
	const FVector CameraForward = FollowCamera
		? FollowCamera->GetForwardVector()
		: (GetController() ? GetController()->GetControlRotation().Vector() : GetActorForwardVector());
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	if (PlayerController)
	{
		PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	}

	for (AActor* Candidate : Candidates)
	{
		if (!IsLockOnCandidateValid(Candidate, LockOnRange, true))
		{
			continue;
		}

		const FVector TargetFocus = GetLockOnFocusLocation(Candidate);
		const FVector CameraToTarget = (TargetFocus - CameraLocation).GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(CameraForward, CameraToTarget);
		if (ForwardDot <= 0.0f)
		{
			continue;
		}

		float ScreenCenterPenalty = 1.0f - ForwardDot;
		FVector2D ScreenPosition;
		if (PlayerController && ViewportWidth > 0 && ViewportHeight > 0
			&& PlayerController->ProjectWorldLocationToScreen(TargetFocus, ScreenPosition, true))
		{
			const FVector2D ViewCenter(ViewportWidth * 0.5f, ViewportHeight * 0.5f);
			const FVector2D NormalizedOffset(
				(ScreenPosition.X - ViewCenter.X) / FMath::Max(ViewCenter.X, 1.0f),
				(ScreenPosition.Y - ViewCenter.Y) / FMath::Max(ViewCenter.Y, 1.0f));
			ScreenCenterPenalty = FMath::Min(NormalizedOffset.Size(), 2.0f);
		}

		const float DistancePenalty = FVector::Dist(GetActorLocation(), Candidate->GetActorLocation())
			/ FMath::Max(LockOnRange, 1.0f);
		const float Score = (ScreenCenterPenalty * LockOnScreenCenterWeight)
			+ ((1.0f - ForwardDot) * LockOnCameraForwardWeight)
			+ (DistancePenalty * LockOnDistanceWeight);
		if (Score < BestScore)
		{
			BestTarget = Candidate;
			BestScore = Score;
		}
	}

	return BestTarget;
}

FVector AExceptionCharacter::GetLockOnFocusLocation(const AActor* Target) const
{
	if (!Target)
	{
		return GetActorLocation();
	}

	const FVector ActorLocation = Target->GetActorLocation();
	const FBox TargetBounds = Target->GetComponentsBoundingBox(true);
	const float BoundsCenterZ = TargetBounds.IsValid ? TargetBounds.GetCenter().Z : ActorLocation.Z + LockOnTargetHeightOffset;
	const float FocusZ = FMath::Clamp(
		BoundsCenterZ,
		ActorLocation.Z + FMath::Min(50.0f, LockOnTargetHeightOffset),
		ActorLocation.Z + LockOnTargetHeightOffset);
	return FVector(ActorLocation.X, ActorLocation.Y, FocusZ);
}

bool AExceptionCharacter::IsLockOnCandidateValid(AActor* Candidate, float MaxRange, bool bRequireLineOfSight) const
{
	if (Candidate == this || !IsLockOnActorAlive(Candidate) || Candidate->IsHidden())
	{
		return false;
	}

	if (FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation()) > FMath::Square(MaxRange))
	{
		return false;
	}

	return !bRequireLineOfSight || HasLockOnLineOfSight(Candidate);
}

bool AExceptionCharacter::HasLockOnLineOfSight(AActor* Candidate) const
{
	UWorld* World = GetWorld();
	if (!World || !Candidate)
	{
		return false;
	}

	const FVector TraceStart = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ExceptionLockOnTrace), false, this);
	QueryParams.AddIgnoredActor(Candidate);
	return !World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		GetLockOnFocusLocation(Candidate),
		ECC_Visibility,
		QueryParams);
}

AActor* AExceptionCharacter::FindLockOnTargetInDirection(float ScreenDirection) const
{
	if (!bIsLockedOn || !IsLockOnActorAlive(LockOnTarget) || FMath::IsNearlyZero(ScreenDirection))
	{
		return nullptr;
	}

	TArray<AActor*> Candidates;
	GatherLockOnCandidates(this, Candidates);

	const float DirectionSign = FMath::Sign(ScreenDirection);
	const FVector CameraLocation = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
	const FVector CameraForward = FollowCamera
		? FollowCamera->GetForwardVector()
		: (GetController() ? GetController()->GetControlRotation().Vector() : GetActorForwardVector());
	const FVector CameraRight = FollowCamera ? FollowCamera->GetRightVector() : GetActorRightVector();
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	FVector2D CurrentScreenPosition = FVector2D::ZeroVector;
	bool bHasCurrentScreenPosition = false;
	if (PlayerController)
	{
		PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
		bHasCurrentScreenPosition = ViewportWidth > 0 && ViewportHeight > 0
			&& PlayerController->ProjectWorldLocationToScreen(GetLockOnFocusLocation(LockOnTarget), CurrentScreenPosition, true);
	}

	const FVector CurrentDirection = (GetLockOnFocusLocation(LockOnTarget) - CameraLocation).GetSafeNormal();
	const float CurrentViewAngle = FMath::RadiansToDegrees(FMath::Atan2(
		FVector::DotProduct(CurrentDirection, CameraRight),
		FVector::DotProduct(CurrentDirection, CameraForward)));

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (AActor* Candidate : Candidates)
	{
		if (Candidate == LockOnTarget || !IsLockOnCandidateValid(Candidate, LockOnRange, true))
		{
			continue;
		}

		const FVector TargetFocus = GetLockOnFocusLocation(Candidate);
		const FVector CandidateDirection = (TargetFocus - CameraLocation).GetSafeNormal();
		if (FVector::DotProduct(CameraForward, CandidateDirection) <= 0.0f)
		{
			continue;
		}

		float DirectionalGap = 0.0f;
		float VerticalGap = 0.0f;
		FVector2D CandidateScreenPosition;
		if (bHasCurrentScreenPosition
			&& PlayerController->ProjectWorldLocationToScreen(TargetFocus, CandidateScreenPosition, true))
		{
			const float SignedScreenGap = (CandidateScreenPosition.X - CurrentScreenPosition.X) * DirectionSign;
			if (SignedScreenGap <= 5.0f)
			{
				continue;
			}

			DirectionalGap = SignedScreenGap / FMath::Max(static_cast<float>(ViewportWidth), 1.0f);
			VerticalGap = FMath::Abs(CandidateScreenPosition.Y - CurrentScreenPosition.Y)
				/ FMath::Max(static_cast<float>(ViewportHeight), 1.0f);
		}
		else
		{
			const float CandidateViewAngle = FMath::RadiansToDegrees(FMath::Atan2(
				FVector::DotProduct(CandidateDirection, CameraRight),
				FVector::DotProduct(CandidateDirection, CameraForward)));
			const float SignedAngleGap = FMath::FindDeltaAngleDegrees(CurrentViewAngle, CandidateViewAngle) * DirectionSign;
			if (SignedAngleGap <= 1.0f)
			{
				continue;
			}

			DirectionalGap = SignedAngleGap / 180.0f;
		}

		const float DistancePenalty = FVector::Dist(GetActorLocation(), Candidate->GetActorLocation())
			/ FMath::Max(LockOnRange, 1.0f);
		const float Score = (DirectionalGap * 0.75f) + (VerticalGap * 0.15f) + (DistancePenalty * 0.10f);
		if (Score < BestScore)
		{
			BestTarget = Candidate;
			BestScore = Score;
		}
	}

	return BestTarget;
}

void AExceptionCharacter::SwitchLockOnTarget(float ScreenDirection)
{
	if (AActor* NewTarget = FindLockOnTargetInDirection(ScreenDirection))
	{
		LockOnTarget = NewTarget;
		LockOnYawOffset = 0.0f;
		LockOnPitchOffset = 0.0f;
		LockOnOccludedTime = 0.0f;

		if (bShowCombatDebug && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1008, 0.8f, FColor::Cyan,
				FString::Printf(TEXT("Lock-on: %s"), *GetNameSafe(NewTarget)));
		}
	}
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
	if (BestTarget && BestTarget->CanBeExecuted() && GetExecutionDistanceSq2D(this, BestTarget) <= FMath::Square(ExecRange))
	{
		return BestTarget;
	}

	TArray<AActor*> Bosses;
	UGameplayStatics::GetAllActorsOfClass(this, ABRBossBase::StaticClass(), Bosses);

	float BestDistanceSq = FMath::Square(ExecRange);
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
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = ExecCamLen;
	CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 80.0f);
	CameraBoom->SocketOffset = ExecCamSide;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	const FVector ToPlayer = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	const FVector SnapDirection = ToPlayer.IsNearlyZero() ? -Target->GetActorForwardVector() : ToPlayer;
	const FVector SnapLocation = Target->GetActorLocation() + (SnapDirection * ExecGap);
	SetActorLocation(SnapLocation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	SetActorRotation(FRotationMatrix::MakeFromX(FVector(ToTarget.X, ToTarget.Y, 0.0f)).Rotator());
	Target->SetActorRotation(FRotationMatrix::MakeFromX(FVector(-ToTarget.X, -ToTarget.Y, 0.0f)).Rotator());

	ExecDmg = Target->GetMaxHP() * ExecDmgRate;
	bExecutionDamageUsesNotify = false;
	const bool bAnimationStarted = PlayOptionalMontage(ExecutionMontage);
	const bool bDamageEventFiredDuringPlay = bExecutionDamageUsesNotify;
	bExecutionDamageUsesNotify = bDamageEventFiredDuringPlay
		|| (bAnimationStarted && AnimationUsesEvent(ExecutionMontage.Get(), EBRPlayerAnimEvent::ExecutionDamage));
	StartRootSwing(true);
	BP_ExecutionStarted(Target);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1011, 1.2f, FColor::Purple, TEXT("Player Execution"));
	}

	if (!bExecutionDamageUsesNotify)
	{
		GetWorldTimerManager().SetTimer(ExecHitTimer, this, &AExceptionCharacter::DoExecHit, FMath::Min(ExecHitTime, ExecTime * 0.85f), false);
	}
	GetWorldTimerManager().SetTimer(ExecutionTimerHandle, this, &AExceptionCharacter::FinishExecution, ExecTime, false);
}

void AExceptionCharacter::DoExecHit()
{
	ABRBossBase* Target = PendingExecutionTarget.Get();
	if (Target && !Target->IsDead())
	{
		Target->CompleteExecution(ExecDmg, this);
	}
	StartRootSwing(true);
}

void AExceptionCharacter::FinishExecution()
{
	GetWorldTimerManager().ClearTimer(ExecHitTimer);
	ABRBossBase* Target = PendingExecutionTarget.Get();
	if (Target && !Target->IsDead())
	{
		// 이미 타격 타이머가 실행됐다면 CompleteExecution이 false로 끝나므로 중복 피해는 없다.
		DoExecHit();
	}

	PendingExecutionTarget = nullptr;
	bExecutionDamageUsesNotify = false;
	bIsInvincible = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetCombatState(EBRPlayerCombatState::Idle);
	ResetExecCam();
	BP_ExecutionFinished(Target, ExecDmg);
	ExecDmg = 0.0f;
}

void AExceptionCharacter::UpdateExecCam(float DeltaSeconds)
{
	ABRBossBase* Target = PendingExecutionTarget.Get();
	if (CombatState != EBRPlayerCombatState::Execution || !Target || !FollowCamera)
	{
		return;
	}

	const FVector Focus = Target->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);
	if (AController* CurrentController = GetController())
	{
		const FRotator WantRot = (Focus - FollowCamera->GetComponentLocation()).Rotation();
		CurrentController->SetControlRotation(FMath::RInterpTo(CurrentController->GetControlRotation(), WantRot, DeltaSeconds, 7.0f));
	}
}

void AExceptionCharacter::ResetExecCam()
{
	if (!CameraBoom)
	{
		return;
	}

	CameraBoom->bDoCollisionTest = true;
	CameraBoom->TargetArmLength = FreeCameraArmLength;
	CameraBoom->TargetOffset = FVector::ZeroVector;
	CameraBoom->SocketOffset = FVector::ZeroVector;
}

void AExceptionCharacter::UpdateLockOn(float DeltaSeconds)
{
	if (!bIsLockedOn)
	{
		return;
	}

	AActor* Target = LockOnTarget;
	if (!IsLockOnActorAlive(Target) || CombatState == EBRPlayerCombatState::Dead)
	{
		ClearLockOn();
		return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (DistanceToTarget > LockOnBreakRange)
	{
		ClearLockOn();
		return;
	}

	if (HasLockOnLineOfSight(Target))
	{
		LockOnOccludedTime = 0.0f;
	}
	else
	{
		LockOnOccludedTime += DeltaSeconds;
		if (LockOnOccludedTime >= LockOnOcclusionBreakDelay)
		{
			ClearLockOn();
			return;
		}
	}

	const FVector TargetFocus = GetLockOnFocusLocation(Target);
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

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const FRotator DesiredActorRotation = FRotationMatrix::MakeFromX(FVector(ToTarget.X, ToTarget.Y, 0.0f)).Rotator();
	const FRotator NewActorRotation = FMath::RInterpTo(GetActorRotation(), DesiredActorRotation, DeltaSeconds, LockOnCharacterRotationInterpSpeed);
	SetActorRotation(NewActorRotation);
}
