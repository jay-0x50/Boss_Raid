// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "GameFramework/Controller.h"

void AExceptionCharacter::DoMove(float Right, float Forward)
{
	if (CombatState == EBRPlayerCombatState::Dead || CombatState == EBRPlayerCombatState::Hit
		|| CombatState == EBRPlayerCombatState::Dodge || CombatState == EBRPlayerCombatState::Healing
		|| CombatState == EBRPlayerCombatState::Execution)
	{
		return;
	}

	if (GetController())
	{
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AExceptionCharacter::DoLook(float Yaw, float Pitch)
{
	if (bIsLockedOn)
	{
		LockOnYawOffset = FMath::Clamp(LockOnYawOffset + (Yaw * LockOnLookInputSensitivity), -LockOnYawOffsetLimit, LockOnYawOffsetLimit);
		LockOnPitchOffset = FMath::Clamp(LockOnPitchOffset + (Pitch * LockOnLookInputSensitivity), -LockOnPitchOffsetLimit, LockOnPitchOffsetLimit);
		return;
	}

	if (GetController())
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AExceptionCharacter::DoJumpStart()
{
	if (CombatState != EBRPlayerCombatState::Idle)
	{
		return;
	}

	Jump();
}

void AExceptionCharacter::DoJumpEnd()
{
	StopJumping();
}
