// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "BRInventoryComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AExceptionCharacter::AExceptionCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = FreeCameraArmLength;
	CameraBoom->TargetOffset = FVector::ZeroVector;
	CameraBoom->SocketOffset = FVector::ZeroVector;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	InventoryComponent = CreateDefaultSubobject<UBRInventoryComponent>(TEXT("InventoryComponent"));
}

void AExceptionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CombatState == EBRPlayerCombatState::Dead)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (CurrentStamina < MaxStamina && World->GetTimeSeconds() - LastStaminaSpendTime >= StaminaRegenDelay)
	{
		CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenPerSecond * DeltaSeconds));
		BroadcastStamina();
	}

	UpdateLockOn(DeltaSeconds);
	DrawCombatDebug();
}

void AExceptionCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryComponent)
	{
		InventoryComponent->OnItemUsed.AddUniqueDynamic(this, &AExceptionCharacter::HandleInventoryItemUsed);
	}

	RestoreHPAndStamina();
	GrantDefaultLoadout();
	RegisterInitialCheckpoint();
}
