// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "BRInventoryComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

namespace
{
	enum class EPlayerSfx : uint8
	{
		Step,
		Swing,
		BigSwing,
		Hit
	};

	USoundWaveProcedural* MakePlayerSfx(UObject* Outer, EPlayerSfx Kind)
	{
		constexpr int32 Rate = 22050;
		const float Len = Kind == EPlayerSfx::Step ? 0.11f
			: Kind == EPlayerSfx::Swing ? 0.22f
			: Kind == EPlayerSfx::BigSwing ? 0.34f
			: 0.13f;
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Len * Rate));
		TArray<int16> Pcm;
		Pcm.SetNumUninitialized(Count);
		FRandomStream Rand(static_cast<int32>(FPlatformTime::Cycles()));
		float SoftNoise = 0.0f;

		for (int32 i = 0; i < Count; ++i)
		{
			const float T = static_cast<float>(i) / Rate;
			const float A = T / Len;
			const float Noise = Rand.FRandRange(-1.0f, 1.0f);
			SoftNoise = FMath::Lerp(SoftNoise, Noise, 0.12f);
			float Sample = 0.0f;

			if (Kind == EPlayerSfx::Step)
			{
				const float Thud = FMath::Sin(2.0f * PI * (92.0f * T - 130.0f * T * T));
				Sample = (Thud * 0.82f + SoftNoise * 0.25f) * FMath::Exp(-28.0f * T);
			}
			else if (Kind == EPlayerSfx::Hit)
			{
				const float Metal = FMath::Sin(2.0f * PI * 740.0f * T);
				const float Knock = FMath::Sin(2.0f * PI * 105.0f * T);
				Sample = (Metal * 0.32f + Knock * 0.7f + Noise * 0.28f) * FMath::Exp(-24.0f * T);
			}
			else
			{
				const bool bBig = Kind == EPlayerSfx::BigSwing;
				const float StartHz = bBig ? 720.0f : 1050.0f;
				const float EndHz = bBig ? 115.0f : 220.0f;
				const float Phase = 2.0f * PI * (StartHz * T + 0.5f * ((EndHz - StartHz) / Len) * T * T);
				const float Env = FMath::Sin(PI * FMath::Clamp(A, 0.0f, 1.0f));
				Sample = (FMath::Sin(Phase) * 0.38f + SoftNoise * 0.72f) * Env;
			}

			Pcm[i] = static_cast<int16>(FMath::Clamp(Sample, -1.0f, 1.0f) * 32767.0f);
		}

		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
		Wave->SetSampleRate(Rate);
		Wave->NumChannels = 1;
		Wave->Duration = Len;
		Wave->SoundGroup = SOUNDGROUP_Effects;
		Wave->bLooping = false;
		Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
		return Wave;
	}
}

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

	RootBladeR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RootBladeR"));
	RootBladeR->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
	RootBladeR->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootBladeR->SetGenerateOverlapEvents(false);
	RootBladeR->SetHiddenInGame(true);
	BladeBaseR = FRotator::ZeroRotator;
	RootBladeR->SetRelativeLocation(BladeGrip);
	RootBladeR->SetRelativeRotation(BladeBaseR);
	RootBladeR->SetRelativeScale3D(FVector(BladeSize));

	RootBladeL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RootBladeL"));
	RootBladeL->SetupAttachment(GetMesh(), TEXT("HandGrip_L"));
	RootBladeL->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootBladeL->SetGenerateOverlapEvents(false);
	RootBladeL->SetHiddenInGame(true);
	BladeBaseL = FRotator::ZeroRotator;
	RootBladeL->SetRelativeLocation(BladeGrip);
	RootBladeL->SetRelativeRotation(BladeBaseL);
	RootBladeL->SetRelativeScale3D(FVector(BladeSize));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BladeRMesh(TEXT("/Game/Items/Weapons/Mimikatz/Right/SM_MimikatzAuthoritySeized_R.SM_MimikatzAuthoritySeized_R"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BladeLMesh(TEXT("/Game/Items/Weapons/Mimikatz/Left/SM_MimikatzAuthoritySeized_L.SM_MimikatzAuthoritySeized_L"));
	RootBladeR->SetStaticMesh(BladeRMesh.Object);
	RootBladeL->SetStaticMesh(BladeLMesh.Object);

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RootLight(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RootHeavy(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03"));
	RootLightAnim = RootLight.Object;
	RootHeavyAnim = RootHeavy.Object;
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
	UpdateStepSfx(DeltaSeconds);
	UpdateRootSwing(DeltaSeconds);
	UpdateExecCam(DeltaSeconds);
	DrawCombatDebug();
}

void AExceptionCharacter::PlayRootAnim(bool bHeavy)
{
	UAnimSequence* Anim = bHeavy ? RootHeavyAnim.Get() : RootLightAnim.Get();
	UAnimInstance* AnimBP = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim || !AnimBP)
	{
		PlayOptionalMontage(bHeavy ? HeavyAttackMontage : LightAttackMontage);
		return;
	}

	const float Rate = bHeavy ? 2.1f : 1.9f;
	AnimBP->PlaySlotAnimationAsDynamicMontage(Anim, TEXT("DefaultSlot"), 0.08f, 0.12f, Rate, 1, -1.0f, 0.0f);
}

void AExceptionCharacter::SetRootWeapon(bool bOn)
{
	bRootOn = bOn;
	if (RootBladeR)
	{
		RootBladeR->SetHiddenInGame(!bOn);
	}
	if (RootBladeL)
	{
		RootBladeL->SetHiddenInGame(!bOn);
	}
}

void AExceptionCharacter::StartRootSwing(bool bHeavy)
{
	if (!bRootOn)
	{
		return;
	}

	bSwinging = true;
	bBigSwing = bHeavy;
	SwingNow = 0.0f;
}

void AExceptionCharacter::UpdateRootSwing(float DeltaSeconds)
{
	if (!bSwinging || !RootBladeR || !RootBladeL)
	{
		return;
	}

	SwingNow += DeltaSeconds;
	const float Len = FMath::Max(SwingTime * (bBigSwing ? 1.45f : 1.0f), KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(SwingNow / Len, 0.0f, 1.0f);
	const float Arc = FMath::Sin(Alpha * PI) * (bBigSwing ? 135.0f : 90.0f);
	RootBladeR->SetRelativeRotation(BladeBaseR + FRotator(-Arc, Arc * 0.2f, 0.0f));
	RootBladeL->SetRelativeRotation(BladeBaseL + FRotator(Arc, -Arc * 0.2f, 0.0f));

	if (Alpha >= 1.0f)
	{
		bSwinging = false;
		RootBladeR->SetRelativeRotation(BladeBaseR);
		RootBladeL->SetRelativeRotation(BladeBaseL);
	}
}

void AExceptionCharacter::UpdateStepSfx(float DeltaSeconds)
{
	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const float Speed = GetVelocity().Size2D();
	if (!MoveComp || !MoveComp->IsMovingOnGround() || Speed < 80.0f
		|| CombatState == EBRPlayerCombatState::Dead || CombatState == EBRPlayerCombatState::Execution)
	{
		StepNow = 0.0f;
		return;
	}

	StepNow += Speed * DeltaSeconds;
	if (StepNow >= StepGap)
	{
		StepNow = FMath::Fmod(StepNow, FMath::Max(StepGap, 1.0f));
		PlayStepSfx();
	}
}

void AExceptionCharacter::PlayStepSfx()
{
	USoundWaveProcedural* Sfx = MakePlayerSfx(this, EPlayerSfx::Step);
	const float Pitch = bLeftStep ? 0.94f : 1.06f;
	bLeftStep = !bLeftStep;
	UGameplayStatics::PlaySoundAtLocation(this, Sfx, GetActorLocation(), StepVol, Pitch);
}

void AExceptionCharacter::PlaySwingSfx(bool bHeavy)
{
	USoundWaveProcedural* Sfx = MakePlayerSfx(this, bHeavy ? EPlayerSfx::BigSwing : EPlayerSfx::Swing);
	const float Vol = AttackVol * (bRootOn ? 1.0f : 0.72f);
	UGameplayStatics::PlaySoundAtLocation(this, Sfx, GetActorLocation() + GetActorForwardVector() * 70.0f, Vol);
}

void AExceptionCharacter::PlayHitSfx()
{
	USoundWaveProcedural* Sfx = MakePlayerSfx(this, EPlayerSfx::Hit);
	UGameplayStatics::PlaySoundAtLocation(this, Sfx, GetActorLocation() + GetActorForwardVector() * AttackTraceDistance, AttackVol * 0.9f);
}

void AExceptionCharacter::BeginPlay()
{
	Super::BeginPlay();
	SaveBaseStats();

	if (InventoryComponent)
	{
		InventoryComponent->TryUseItem.BindUObject(this, &AExceptionCharacter::TryUseInventoryItem);
	}

	RestoreHPAndStamina();
	GrantDefaultLoadout();
	RegisterInitialCheckpoint();
}
