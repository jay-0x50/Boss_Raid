#include "BRMapFragmentPickup.h"

#include "BRNarrativeQueueSubsystem.h"
#include "BRSaveGameSubsystem.h"
#include "BRWorldMapSubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABRMapFragmentPickup::ABRMapFragmentPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	SetRootComponent(PickupSphere);
	PickupSphere->InitSphereRadius(125.0f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	FragmentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FragmentMesh"));
	FragmentMesh->SetupAttachment(RootComponent);
	FragmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FragmentMesh->SetMobility(EComponentMobility::Movable);
	FragmentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
	FragmentMesh->SetRelativeScale3D(FVector(1.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> FragmentMeshFinder(
		TEXT("/Game/Items/KeyItems/NellHiddenMemoryFragment/SM_NellHiddenMemoryFragment.SM_NellHiddenMemoryFragment"));
	if (FragmentMeshFinder.Succeeded())
	{
		FragmentMesh->SetStaticMesh(FragmentMeshFinder.Object);
	}

	FragmentLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FragmentLight"));
	FragmentLight->SetupAttachment(RootComponent);
	FragmentLight->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	FragmentLight->SetLightColor(FLinearColor(0.16f, 0.72f, 1.0f));
	FragmentLight->SetIntensity(1850.0f);
	FragmentLight->SetAttenuationRadius(520.0f);
	FragmentLight->SetCastShadows(false);

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 32.0f, 0.0f);
	RotatingMovement->UpdatedComponent = FragmentMesh;

	RegionDisplayName = NSLOCTEXT("ExceptionMap", "DefaultFragmentName", "Unmapped Region");
}

void ABRMapFragmentPickup::BeginPlay()
{
	Super::BeginPlay();

	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ABRMapFragmentPickup::OnPickupBeginOverlap);
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>())
		{
			WorldMap->OnMapRegionUnlocked.AddUniqueDynamic(this, &ABRMapFragmentPickup::HandleMapRegionUnlocked);
			if (WorldMap->IsRegionUnlocked(RegionId))
			{
				DisableCollectedFragment();
			}
		}
	}
}

void ABRMapFragmentPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>())
		{
			WorldMap->OnMapRegionUnlocked.RemoveDynamic(this, &ABRMapFragmentPickup::HandleMapRegionUnlocked);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ABRMapFragmentPickup::OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<AExceptionCharacter>(OtherActor))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UBRWorldMapSubsystem* WorldMap = GameInstance ? GameInstance->GetSubsystem<UBRWorldMapSubsystem>() : nullptr;
	if (!WorldMap || !WorldMap->UnlockRegion(RegionId))
	{
		return;
	}

	if (UBRNarrativeQueueSubsystem* Narrative = GameInstance->GetSubsystem<UBRNarrativeQueueSubsystem>())
	{
		const FText RegionName = RegionDisplayName.IsEmpty() ? FText::FromName(RegionId) : RegionDisplayName;
		Narrative->ShowSystemLog(FText::Format(NSLOCTEXT("ExceptionMap", "FragmentUnlocked", "Map fragment restored: {0}"), RegionName), 3.5f,
			NSLOCTEXT("ExceptionMap", "MapUpdated", "WORLD MAP UPDATED"));
		if (!UnlockLine.IsEmpty())
		{
			Narrative->ShowNelLine(UnlockLine, false, 4.5f);
		}
	}

	if (UBRSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UBRSaveGameSubsystem>())
	{
		SaveSubsystem->SaveCurrentGame();
	}

	DisableCollectedFragment();
}

void ABRMapFragmentPickup::DisableCollectedFragment()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	if (PickupSphere)
	{
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (RotatingMovement)
	{
		RotatingMovement->Deactivate();
	}
}

void ABRMapFragmentPickup::HandleMapRegionUnlocked(FName ChangedRegionId, int32 UnlockedCount)
{
	if (ChangedRegionId.IsNone() || ChangedRegionId == RegionId)
	{
		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (const UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>(); WorldMap && WorldMap->IsRegionUnlocked(RegionId))
			{
				DisableCollectedFragment();
			}
		}
	}
}
