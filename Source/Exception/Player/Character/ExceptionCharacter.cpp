// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Character/ExceptionCharacter.h"

#include "BRInventoryComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
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
		Hit,
		Heal
	};

	USoundWaveProcedural* MakePlayerSfx(UObject* Outer, EPlayerSfx Kind)
	{
		constexpr int32 Rate = 22050;
		const float Len = Kind == EPlayerSfx::Step ? 0.11f
			: Kind == EPlayerSfx::Swing ? 0.22f
			: Kind == EPlayerSfx::BigSwing ? 0.34f
			: Kind == EPlayerSfx::Heal ? 0.62f
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
			else if (Kind == EPlayerSfx::Heal)
			{
				const float Chime = FMath::Sin(2.0f * PI * 523.25f * T)
					+ 0.55f * FMath::Sin(2.0f * PI * 783.99f * T)
					+ 0.28f * FMath::Sin(2.0f * PI * 1046.5f * T);
				const float Rise = FMath::SmoothStep(0.0f, 0.22f, T);
				const float Fall = FMath::Exp(-3.8f * T);
				Sample = Chime * 0.22f * Rise * Fall + SoftNoise * 0.025f * Fall;
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
	GetCharacterMovement()->MaxWalkSpeed = JogSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->MaxAcceleration = 1500.0f;
	GetCharacterMovement()->GroundFriction = 7.2f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1500.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	NormalGroundFriction = GetCharacterMovement()->GroundFriction;
	NormalBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
	NormalCapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

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

	RuntimeFlask = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RuntimeFlask"));
	RuntimeFlask->SetupAttachment(GetMesh(), TEXT("HandGrip_R"));
	RuntimeFlask->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RuntimeFlask->SetGenerateOverlapEvents(false);
	RuntimeFlask->SetCastShadow(false);
	RuntimeFlask->SetHiddenInGame(true);
	RuntimeFlask->SetRelativeLocation(FVector(2.5f, 1.5f, -1.0f));
	RuntimeFlask->SetRelativeRotation(FRotator(8.0f, 88.0f, -12.0f));
	RuntimeFlask->SetRelativeScale3D(FVector(0.72f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> FlaskMesh(TEXT("/Game/Items/Consumables/RuntimeFlask/Filled/SM_RuntimeFlask_Filled.SM_RuntimeFlask_Filled"));
	if (FlaskMesh.Succeeded())
	{
		RuntimeFlask->SetStaticMesh(FlaskMesh.Object);
	}

	FlaskAura = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlaskAura"));
	FlaskAura->SetupAttachment(RuntimeFlask);
	FlaskAura->SetRelativeLocation(FVector(0.0f, 0.0f, 16.0f));
	FlaskAura->SetLightColor(FLinearColor(0.12f, 0.82f, 1.0f));
	FlaskAura->SetIntensity(0.0f);
	FlaskAura->SetAttenuationRadius(240.0f);
	FlaskAura->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BladeRMesh(TEXT("/Game/Items/Weapons/Mimikatz/Right/SM_MimikatzAuthoritySeized_R.SM_MimikatzAuthoritySeized_R"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BladeLMesh(TEXT("/Game/Items/Weapons/Mimikatz/Left/SM_MimikatzAuthoritySeized_L.SM_MimikatzAuthoritySeized_L"));
	RootBladeR->SetStaticMesh(BladeRMesh.Object);
	RootBladeL->SetStaticMesh(BladeLMesh.Object);

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RootLight(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RootHeavy(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ComboOne(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ComboTwo(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ComboThree(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HeavyAlt(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultDodge(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/AM_Player_Dodge.AM_Player_Dodge"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HealUse(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload.MM_Pistol_Reload"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ParrySuccess(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_04.MM_HitReact_Front_Lgt_04"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HitFront(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01.MM_HitReact_Front_Lgt_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HitBack(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Back_Med_01.MM_HitReact_Back_Med_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HitLeft(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_03.MM_HitReact_Front_Lgt_03"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HitRight(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_02.MM_HitReact_Front_Lgt_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HeavyKnockback(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Death(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
	RootLightAnim = RootLight.Object;
	RootHeavyAnim = RootHeavy.Object;
	if (ComboOne.Succeeded())
	{
		LightComboAnims.Add(ComboOne.Object);
	}
	if (ComboTwo.Succeeded())
	{
		LightComboAnims.Add(ComboTwo.Object);
	}
	if (ComboThree.Succeeded())
	{
		LightComboAnims.Add(ComboThree.Object);
	}
	HeavyAltAnim = HeavyAlt.Object;
	DodgeMontage = DefaultDodge.Object;
	HealMontage = nullptr;
	HealAnim = HealUse.Object;
	ParrySuccessAnim = ParrySuccess.Object;
	HitFrontAnim = HitFront.Object;
	HitBackAnim = HitBack.Object;
	HitLeftAnim = HitLeft.Object;
	HitRightAnim = HitRight.Object;
	HeavyKnockbackAnim = HeavyKnockback.Object;
	DeathAnim = Death.Object;
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

	const bool bCanRegenStamina = CombatState == EBRPlayerCombatState::Idle && !bSprinting;
	if (bCanRegenStamina && CurrentStamina < MaxStamina
		&& World->GetTimeSeconds() - LastStaminaSpendTime >= StaminaRegenDelay)
	{
		CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenPerSecond * DeltaSeconds));
		BroadcastStamina();
	}

	UpdateSprint(DeltaSeconds);
	UpdateDodgeRoll(DeltaSeconds);
	UpdateFlaskHeal(DeltaSeconds);
	UpdateLockOn(DeltaSeconds);
	UpdateStepSfx(DeltaSeconds);
	UpdateRootSwing(DeltaSeconds);
	UpdateAttackHitWindow(DeltaSeconds);
	UpdateExecCam(DeltaSeconds);
	UpdateHendelAppearance();
	DrawCombatDebug();
}

void AExceptionCharacter::ApplyHendelAppearance()
{
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (!BodyMesh)
	{
		return;
	}

	if (USkeletalMesh* HendelMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")))
	{
		BodyMesh->SetSkeletalMeshAsset(HendelMesh);
	}

	const TCHAR* MaterialPaths[] =
	{
		TEXT("/Game/Characters/Exception/Materials/M_Hendel_Armor01.M_Hendel_Armor01"),
		TEXT("/Game/Characters/Exception/Materials/M_Hendel_Armor02.M_Hendel_Armor02"),
	};

	HendelMaterials.Reset();
	for (int32 MaterialIndex = 0; MaterialIndex < UE_ARRAY_COUNT(MaterialPaths); ++MaterialIndex)
	{
		UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, MaterialPaths[MaterialIndex]);
		if (!BaseMaterial)
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		DynamicMaterial->SetVectorParameterValue(TEXT("ArmorTint"), FLinearColor(0.025f, 0.045f, 0.075f, 1.0f));
		DynamicMaterial->SetVectorParameterValue(TEXT("CodeColor"), FLinearColor(0.0f, 0.72f, 1.0f, 1.0f));
		DynamicMaterial->SetScalarParameterValue(TEXT("GlowStrength"), 1.8f);
		BodyMesh->SetMaterial(MaterialIndex, DynamicMaterial);
		HendelMaterials.Add(DynamicMaterial);
	}
}

void AExceptionCharacter::UpdateHendelAppearance()
{
	if (HendelMaterials.IsEmpty())
	{
		return;
	}

	const float HPRatio = MaxHP > KINDA_SMALL_NUMBER ? CurrentHP / MaxHP : 1.0f;
	float GlowStrength = 1.8f;
	if (CombatState == EBRPlayerCombatState::Execution)
	{
		GlowStrength = 5.5f;
	}
	else if (HPRatio <= 0.3f && GetWorld())
	{
		const float Flicker = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 17.0f);
		GlowStrength = FMath::Lerp(0.55f, 2.8f, Flicker);
	}

	for (UMaterialInstanceDynamic* DynamicMaterial : HendelMaterials)
	{
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(TEXT("GlowStrength"), GlowStrength);
		}
	}
}

void AExceptionCharacter::PlayRootAnim(bool bHeavy)
{
	UAnimSequence* Anim = bHeavy ? RootHeavyAnim.Get() : RootLightAnim.Get();
	PlayAttackSequence(Anim, bHeavy ? HeavyAttackMontage.Get() : LightAttackMontage.Get(), bHeavy ? 2.1f : 1.9f);
}

bool AExceptionCharacter::PlayAttackSequence(UAnimSequence* Anim, UAnimMontage* FallbackMontage, float Rate)
{
	UAnimInstance* AnimBP = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim || !AnimBP)
	{
		return PlayOptionalMontage(FallbackMontage);
	}

	return AnimBP->PlaySlotAnimationAsDynamicMontage(
		Anim,
		TEXT("DefaultSlot"),
		0.06f,
		0.10f,
		FMath::Max(0.1f, Rate),
		1,
		-1.0f,
		0.0f) != nullptr;
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
		HandleAnimationEvent(EBRPlayerAnimEvent::Footstep);
	}
}

void AExceptionCharacter::PlayStepSfx()
{
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerFootstepSurface), false, this);
	QueryParams.bReturnPhysicalMaterial = true;
	const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 55.0f);
	if (UWorld* World = GetWorld())
	{
		World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	}

	const EPhysicalSurface SurfaceType = UGameplayStatics::GetSurfaceType(GroundHit);
	const FName SurfaceName = GroundHit.PhysMaterial.IsValid()
		? GroundHit.PhysMaterial->GetFName()
		: FName(*FString::Printf(TEXT("SurfaceType_%d"), static_cast<int32>(SurfaceType)));
	BP_PlayerFootstep(SurfaceName, GroundHit.bBlockingHit ? GroundHit.ImpactPoint : GetActorLocation());

	USoundWaveProcedural* Sfx = MakePlayerSfx(this, EPlayerSfx::Step);
	const float SurfacePitch = 0.98f + 0.015f * static_cast<float>(static_cast<uint8>(SurfaceType) % 4);
	const float Pitch = (bLeftStep ? 0.94f : 1.06f) * SurfacePitch;
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

void AExceptionCharacter::PlayHealSfx()
{
	USoundWaveProcedural* Sfx = MakePlayerSfx(this, EPlayerSfx::Heal);
	UGameplayStatics::PlaySoundAtLocation(this, Sfx, GetActorLocation() + FVector(0.0f, 0.0f, 80.0f), 0.72f);
}

void AExceptionCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyHendelAppearance();
	BaseMeshRelativeLocation = GetMesh()->GetRelativeLocation();
	BaseMeshRelativeRotation = GetMesh()->GetRelativeRotation();
	if (RuntimeFlask)
	{
		FlaskBaseLocation = RuntimeFlask->GetRelativeLocation();
		FlaskBaseRotation = RuntimeFlask->GetRelativeRotation();
	}
	SaveBaseStats();

	if (InventoryComponent)
	{
		InventoryComponent->TryUseItem.BindUObject(this, &AExceptionCharacter::TryUseInventoryItem);
	}

	RestoreHPAndStamina();
	GrantDefaultLoadout();
	RegisterInitialCheckpoint();
}

void AExceptionCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearHitStop();
	Super::EndPlay(EndPlayReason);
}
