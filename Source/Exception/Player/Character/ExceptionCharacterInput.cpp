// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "BRInventoryComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Exception.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Player/Controller/ExceptionPlayerController.h"
#include "TimerManager.h"
#include "World/BRBossActivationPlate.h"

void AExceptionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AExceptionCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AExceptionCharacter::Look);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Completed, this, &AExceptionCharacter::LookInputCompleted);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AExceptionCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Completed, this, &AExceptionCharacter::LookInputCompleted);

		if (LightAttackAction)
		{
			EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &AExceptionCharacter::LightAttackPressed);
		}

		if (HeavyAttackAction)
		{
			EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AExceptionCharacter::HeavyAttackPressed);
		}

		if (DodgeAction)
		{
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AExceptionCharacter::DodgePressed);
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Completed, this, &AExceptionCharacter::DodgeReleased);
		}

		if (ParryAction)
		{
			EnhancedInputComponent->BindAction(ParryAction, ETriggerEvent::Started, this, &AExceptionCharacter::ParryPressed);
		}

		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AExceptionCharacter::InteractPressed);
		}

		if (LockOnAction)
		{
			EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &AExceptionCharacter::LockOnPressed);
		}

		if (UseFlaskAction)
		{
			EnhancedInputComponent->BindAction(UseFlaskAction, ETriggerEvent::Started, this, &AExceptionCharacter::UseFlaskPressed);
		}

		SetupRuntimeCombatInput(EnhancedInputComponent);
	}
	else
	{
		UE_LOG(LogException, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system."), *GetNameSafe(this));
	}
}

void AExceptionCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AExceptionCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	const float HorizontalMagnitude = FMath::Abs(LookAxisVector.X);
	if (bIsLockedOn)
	{
		if (HorizontalMagnitude <= LockOnSwitchInputResetThreshold)
		{
			bLockOnSwitchInputReady = true;
		}
		else if (bLockOnSwitchInputReady && HorizontalMagnitude >= LockOnSwitchInputThreshold)
		{
			SwitchLockOnTarget(FMath::Sign(LookAxisVector.X));
			bLockOnSwitchInputReady = false;
		}
	}

	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AExceptionCharacter::LookInputCompleted()
{
	bLockOnSwitchInputReady = true;
}

void AExceptionCharacter::LightAttackPressed()
{
	DoLightAttack();
}

void AExceptionCharacter::HeavyAttackPressed()
{
	DoHeavyAttack();
}

void AExceptionCharacter::DodgePressed()
{
	if (bDodgeHeld || CombatState != EBRPlayerCombatState::Idle)
	{
		return;
	}

	bDodgeHeld = true;
	bSprintStartedThisHold = false;
	GetWorldTimerManager().SetTimer(SprintHoldTimerHandle, this, &AExceptionCharacter::BeginSprintIfHeld, SprintHoldTime, false);
}

void AExceptionCharacter::DodgeReleased()
{
	if (!bDodgeHeld)
	{
		return;
	}

	bDodgeHeld = false;
	GetWorldTimerManager().ClearTimer(SprintHoldTimerHandle);
	const bool bWasSprint = bSprintStartedThisHold;
	bSprintStartedThisHold = false;
	StopSprint();
	if (!bWasSprint)
	{
		DoDodge();
	}
}

void AExceptionCharacter::ParryPressed()
{
	DoParry();
}

void AExceptionCharacter::InteractPressed()
{
	DoInteract();
}

void AExceptionCharacter::LockOnPressed()
{
	ToggleLockOn();
}

void AExceptionCharacter::UseFlaskPressed()
{
	if (!InventoryComponent || CombatState != EBRPlayerCombatState::Idle)
	{
		return;
	}

	int32 FlaskSlot = InventoryComponent->FindFirstItemSlot(TEXT("Potion_RuntimeFlask"));
	if (FlaskSlot != INDEX_NONE)
	{
		InventoryComponent->UseSlot(FlaskSlot);
	}
}

void AExceptionCharacter::BossPlate1Pressed()
{
	ActivateBossPlateByIndex(1);
}

void AExceptionCharacter::BossPlate2Pressed()
{
	ActivateBossPlateByIndex(2);
}

void AExceptionCharacter::BossPlate3Pressed()
{
	ActivateBossPlateByIndex(3);
}

void AExceptionCharacter::ActivateBossPlateByIndex(int32 PlateIndex)
{
	const AExceptionPlayerController* PlayerController = Cast<AExceptionPlayerController>(GetController());
	if (!PlayerController || !PlayerController->AreDemoDebugHotkeysEnabled())
	{
		return;
	}

	ABRBossActivationPlate::ActivatePlateByIndex(this, PlateIndex, this);
}

void AExceptionCharacter::SetupRuntimeCombatInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!EnhancedInputComponent)
	{
		return;
	}

	bool bNeedsRuntimeMapping = false;
	if (!RuntimeCombatMappingContext)
	{
		RuntimeCombatMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_RuntimeExceptionCombat"));
	}

	if (!LightAttackAction)
	{
		RuntimeLightAttackAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeLightAttack"));
		RuntimeLightAttackAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeLightAttackAction, EKeys::LeftMouseButton);
		RuntimeCombatMappingContext->MapKey(RuntimeLightAttackAction, EKeys::Gamepad_RightShoulder);
		EnhancedInputComponent->BindAction(RuntimeLightAttackAction, ETriggerEvent::Started, this, &AExceptionCharacter::LightAttackPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!HeavyAttackAction)
	{
		RuntimeHeavyAttackAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeHeavyAttack"));
		RuntimeHeavyAttackAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeHeavyAttackAction, EKeys::RightMouseButton);
		RuntimeCombatMappingContext->MapKey(RuntimeHeavyAttackAction, EKeys::Gamepad_RightTrigger);
		EnhancedInputComponent->BindAction(RuntimeHeavyAttackAction, ETriggerEvent::Started, this, &AExceptionCharacter::HeavyAttackPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!DodgeAction)
	{
		RuntimeDodgeAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeDodge"));
		RuntimeDodgeAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeDodgeAction, EKeys::LeftShift);
		RuntimeCombatMappingContext->MapKey(RuntimeDodgeAction, EKeys::Gamepad_FaceButton_Right);
		EnhancedInputComponent->BindAction(RuntimeDodgeAction, ETriggerEvent::Started, this, &AExceptionCharacter::DodgePressed);
		EnhancedInputComponent->BindAction(RuntimeDodgeAction, ETriggerEvent::Completed, this, &AExceptionCharacter::DodgeReleased);
		bNeedsRuntimeMapping = true;
	}

	if (!ParryAction)
	{
		RuntimeParryAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeParry"));
		RuntimeParryAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeParryAction, EKeys::F);
		RuntimeCombatMappingContext->MapKey(RuntimeParryAction, EKeys::Gamepad_LeftShoulder);
		EnhancedInputComponent->BindAction(RuntimeParryAction, ETriggerEvent::Started, this, &AExceptionCharacter::ParryPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!InteractAction)
	{
		RuntimeInteractAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeInteract"));
		RuntimeInteractAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeInteractAction, EKeys::E);
		// FaceButton_Bottom is already Jump in IMC_Default, so Y/triangle stays conflict-free.
		RuntimeCombatMappingContext->MapKey(RuntimeInteractAction, EKeys::Gamepad_FaceButton_Top);
		EnhancedInputComponent->BindAction(RuntimeInteractAction, ETriggerEvent::Started, this, &AExceptionCharacter::InteractPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!LockOnAction)
	{
		RuntimeLockOnAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeLockOn"));
		RuntimeLockOnAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeLockOnAction, EKeys::Tab);
		RuntimeCombatMappingContext->MapKey(RuntimeLockOnAction, EKeys::Gamepad_RightThumbstick);
		EnhancedInputComponent->BindAction(RuntimeLockOnAction, ETriggerEvent::Started, this, &AExceptionCharacter::LockOnPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!UseFlaskAction)
	{
		RuntimeUseFlaskAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeUseFlask"));
		RuntimeUseFlaskAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeUseFlaskAction, EKeys::Q);
		RuntimeCombatMappingContext->MapKey(RuntimeUseFlaskAction, EKeys::Gamepad_FaceButton_Left);
		EnhancedInputComponent->BindAction(RuntimeUseFlaskAction, ETriggerEvent::Started, this, &AExceptionCharacter::UseFlaskPressed);
		bNeedsRuntimeMapping = true;
	}

	const AExceptionPlayerController* ExceptionController = Cast<AExceptionPlayerController>(GetController());
	if (ExceptionController && ExceptionController->AreDemoDebugHotkeysEnabled())
	{
		RuntimeBossPlate1Action = NewObject<UInputAction>(this, TEXT("IA_RuntimeBossPlate1"));
		RuntimeBossPlate1Action->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeBossPlate1Action, EKeys::One);
		EnhancedInputComponent->BindAction(RuntimeBossPlate1Action, ETriggerEvent::Started, this, &AExceptionCharacter::BossPlate1Pressed);

		RuntimeBossPlate2Action = NewObject<UInputAction>(this, TEXT("IA_RuntimeBossPlate2"));
		RuntimeBossPlate2Action->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeBossPlate2Action, EKeys::Two);
		EnhancedInputComponent->BindAction(RuntimeBossPlate2Action, ETriggerEvent::Started, this, &AExceptionCharacter::BossPlate2Pressed);

		RuntimeBossPlate3Action = NewObject<UInputAction>(this, TEXT("IA_RuntimeBossPlate3"));
		RuntimeBossPlate3Action->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeBossPlate3Action, EKeys::Three);
		EnhancedInputComponent->BindAction(RuntimeBossPlate3Action, ETriggerEvent::Started, this, &AExceptionCharacter::BossPlate3Pressed);
		bNeedsRuntimeMapping = true;
	}

	if (bNeedsRuntimeMapping)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
				{
					Subsystem->AddMappingContext(RuntimeCombatMappingContext, 1);
				}
			}
		}
	}
}
