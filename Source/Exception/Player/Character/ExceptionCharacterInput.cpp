// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Exception.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "World/BRBossActivationPlate.h"

void AExceptionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AExceptionCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AExceptionCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AExceptionCharacter::Look);

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
	DoLook(LookAxisVector.X, LookAxisVector.Y);
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
	DoDodge();
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

void AExceptionCharacter::BossPlate1Pressed()
{
	ActivateBossPlateByIndex(1);
}

void AExceptionCharacter::BossPlate2Pressed()
{
	ActivateBossPlateByIndex(2);
}

void AExceptionCharacter::ActivateBossPlateByIndex(int32 PlateIndex)
{
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
		EnhancedInputComponent->BindAction(RuntimeLightAttackAction, ETriggerEvent::Started, this, &AExceptionCharacter::LightAttackPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!HeavyAttackAction)
	{
		RuntimeHeavyAttackAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeHeavyAttack"));
		RuntimeHeavyAttackAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeHeavyAttackAction, EKeys::RightMouseButton);
		EnhancedInputComponent->BindAction(RuntimeHeavyAttackAction, ETriggerEvent::Started, this, &AExceptionCharacter::HeavyAttackPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!DodgeAction)
	{
		RuntimeDodgeAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeDodge"));
		RuntimeDodgeAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeDodgeAction, EKeys::LeftShift);
		EnhancedInputComponent->BindAction(RuntimeDodgeAction, ETriggerEvent::Started, this, &AExceptionCharacter::DodgePressed);
		bNeedsRuntimeMapping = true;
	}

	if (!ParryAction)
	{
		RuntimeParryAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeParry"));
		RuntimeParryAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeParryAction, EKeys::F);
		EnhancedInputComponent->BindAction(RuntimeParryAction, ETriggerEvent::Started, this, &AExceptionCharacter::ParryPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!InteractAction)
	{
		RuntimeInteractAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeInteract"));
		RuntimeInteractAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeInteractAction, EKeys::E);
		EnhancedInputComponent->BindAction(RuntimeInteractAction, ETriggerEvent::Started, this, &AExceptionCharacter::InteractPressed);
		bNeedsRuntimeMapping = true;
	}

	if (!LockOnAction)
	{
		RuntimeLockOnAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeLockOn"));
		RuntimeLockOnAction->ValueType = EInputActionValueType::Boolean;
		RuntimeCombatMappingContext->MapKey(RuntimeLockOnAction, EKeys::Tab);
		EnhancedInputComponent->BindAction(RuntimeLockOnAction, ETriggerEvent::Started, this, &AExceptionCharacter::LockOnPressed);
		bNeedsRuntimeMapping = true;
	}

	RuntimeBossPlate1Action = NewObject<UInputAction>(this, TEXT("IA_RuntimeBossPlate1"));
	RuntimeBossPlate1Action->ValueType = EInputActionValueType::Boolean;
	RuntimeCombatMappingContext->MapKey(RuntimeBossPlate1Action, EKeys::One);
	EnhancedInputComponent->BindAction(RuntimeBossPlate1Action, ETriggerEvent::Started, this, &AExceptionCharacter::BossPlate1Pressed);
	bNeedsRuntimeMapping = true;

	RuntimeBossPlate2Action = NewObject<UInputAction>(this, TEXT("IA_RuntimeBossPlate2"));
	RuntimeBossPlate2Action->ValueType = EInputActionValueType::Boolean;
	RuntimeCombatMappingContext->MapKey(RuntimeBossPlate2Action, EKeys::Two);
	EnhancedInputComponent->BindAction(RuntimeBossPlate2Action, ETriggerEvent::Started, this, &AExceptionCharacter::BossPlate2Pressed);
	bNeedsRuntimeMapping = true;

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
