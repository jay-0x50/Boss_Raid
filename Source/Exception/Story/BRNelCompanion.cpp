#include "Story/BRNelCompanion.h"

#include "Animation/AnimInstance.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABRNelCompanion::ABRNelCompanion()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NelBody"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCastShadow(false);
	BodyMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	BodyMesh->SetRelativeScale3D(FVector(0.86f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> NelMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	if (NelMesh.Succeeded())
	{
		BodyMesh->SetSkeletalMeshAsset(NelMesh.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> NelAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (NelAnim.Succeeded())
	{
		BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		BodyMesh->SetAnimInstanceClass(NelAnim.Class);
	}

	RobeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NelRobe"));
	RobeMesh->SetupAttachment(SceneRoot);
	RobeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RobeMesh->SetGenerateOverlapEvents(false);
	RobeMesh->SetCastShadow(false);
	RobeMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 47.0f));
	RobeMesh->SetRelativeScale3D(FVector(0.52f, 0.52f, 1.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RobeShape(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (RobeShape.Succeeded())
	{
		RobeMesh->SetStaticMesh(RobeShape.Object);
	}

	AuraLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("NelAura"));
	AuraLight->SetupAttachment(SceneRoot);
	AuraLight->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	AuraLight->SetLightColor(FLinearColor(0.55f, 0.78f, 1.0f));
	AuraLight->SetIntensity(0.0f);
	AuraLight->SetAttenuationRadius(360.0f);
	AuraLight->SetCastShadows(false);
}

void ABRNelCompanion::BeginPlay()
{
	Super::BeginPlay();
	BuildDynamicMaterials();
	ApplyFade(0.0f);
	SetActorHiddenInGame(true);
}

void ABRNelCompanion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFadingIn)
	{
		FadeAlpha = FMath::Min(1.0f, FadeAlpha + DeltaSeconds / FMath::Max(FadeInTime, KINDA_SMALL_NUMBER));
		if (FadeAlpha >= 1.0f)
		{
			bFadingIn = false;
		}
	}
	else if (HoldRemaining > 0.0f)
	{
		HoldRemaining -= DeltaSeconds;
		if (HoldRemaining <= 0.0f)
		{
			bFadingOut = true;
		}
	}
	else if (bFadingOut)
	{
		FadeAlpha = FMath::Max(0.0f, FadeAlpha - DeltaSeconds / FMath::Max(FadeOutTime, KINDA_SMALL_NUMBER));
		if (FadeAlpha <= 0.0f)
		{
			bFadingOut = false;
			SetActorHiddenInGame(true);
			SetActorTickEnabled(false);
		}
	}

	ApplyFade(FadeAlpha);
}

void ABRNelCompanion::Appear(float HoldTime)
{
	if (GhostMaterials.IsEmpty())
	{
		BuildDynamicMaterials();
	}

	if (bFacePlayerOnAppear)
	{
		FacePlayer();
	}

	HoldRemaining = HoldTime > 0.0f ? HoldTime : DefaultHoldTime;
	bFadingOut = false;
	bFadingIn = true;
	FadeAlpha = 0.0f;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	ApplyFade(0.0f);
}

void ABRNelCompanion::Disappear()
{
	bFadingIn = false;
	HoldRemaining = 0.0f;
	bFadingOut = true;
	SetActorTickEnabled(true);
}

void ABRNelCompanion::BuildDynamicMaterials()
{
	GhostMaterials.Reset();
	const TCHAR* BodyMaterialPaths[] =
	{
		TEXT("/Game/Characters/Exception/Materials/M_Nel_Ghost01.M_Nel_Ghost01"),
		TEXT("/Game/Characters/Exception/Materials/M_Nel_Ghost02.M_Nel_Ghost02"),
	};

	for (int32 MaterialIndex = 0; MaterialIndex < UE_ARRAY_COUNT(BodyMaterialPaths); ++MaterialIndex)
	{
		if (UMaterialInterface* Source = LoadObject<UMaterialInterface>(nullptr, BodyMaterialPaths[MaterialIndex]))
		{
			UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Source, this);
			BodyMesh->SetMaterial(MaterialIndex, DynamicMaterial);
			GhostMaterials.Add(DynamicMaterial);
		}
	}

	if (UMaterialInterface* RobeSource = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/Exception/Materials/M_Nel_Robe.M_Nel_Robe")))
	{
		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(RobeSource, this);
		RobeMesh->SetMaterial(0, DynamicMaterial);
		GhostMaterials.Add(DynamicMaterial);
	}
}

void ABRNelCompanion::ApplyFade(float InAlpha)
{
	const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(InAlpha, 0.0f, 1.0f));
	for (UMaterialInstanceDynamic* DynamicMaterial : GhostMaterials)
	{
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), SmoothAlpha * 0.78f);
			DynamicMaterial->SetScalarParameterValue(TEXT("GlowStrength"), FMath::Lerp(0.2f, 3.2f, SmoothAlpha));
		}
	}

	if (AuraLight)
	{
		AuraLight->SetIntensity(1050.0f * SmoothAlpha);
	}
}

void ABRNelCompanion::FacePlayer()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	const FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
	if (!ToPlayer.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	}
}
