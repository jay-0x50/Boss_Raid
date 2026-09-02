#include "BRHiddenFragmentPickup.h"

#include "BRSaveGameSubsystem.h"
#include "BRHiddenStorySubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABRHiddenFragmentPickup::ABRHiddenFragmentPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	SetRootComponent(PickupSphere);
	PickupSphere->InitSphereRadius(90.0f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(RootComponent);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetRelativeScale3D(FVector(0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		PreviewMesh->SetStaticMesh(SphereMesh.Object);
	}
}

void ABRHiddenFragmentPickup::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			Story->OnHiddenFragmentIdCollected.AddDynamic(this, &ABRHiddenFragmentPickup::HandleHiddenFragmentCollected);
		}
	}
	RefreshCollectedState();

	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ABRHiddenFragmentPickup::OnPickupBeginOverlap);
	}
}

void ABRHiddenFragmentPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			Story->OnHiddenFragmentIdCollected.RemoveDynamic(this, &ABRHiddenFragmentPickup::HandleHiddenFragmentCollected);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ABRHiddenFragmentPickup::OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(OtherActor);
	if (!PlayerCharacter || bWasCollected)
	{
		return;
	}

	bWasCollected = true;
	bool bUsedPersistentStoryState = false;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			bUsedPersistentStoryState = true;
			if (!Story->TryCollectHiddenFragment(GetResolvedFragmentId(), FragmentAmount))
			{
				ApplyCollectedState();
				if (bDestroyOnPickup)
				{
					Destroy();
				}
				return;
			}
			PlayerCharacter->RefreshHiddenStoryRewards();
		}
	}
	if (!bUsedPersistentStoryState)
	{
		PlayerCharacter->CollectHiddenFragment(FragmentAmount);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UBRSaveGameSubsystem>())
		{
			SaveSubsystem->SaveCurrentGame();
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(5101, 2.0f, FColor::Purple, TEXT("Hidden Fragment Collected"));
	}
	ApplyCollectedState();

	if (bDestroyOnPickup)
	{
		Destroy();
	}
}

FName ABRHiddenFragmentPickup::GetResolvedFragmentId() const
{
	return FragmentId.IsNone() ? GetFName() : FragmentId;
}

void ABRHiddenFragmentPickup::RefreshCollectedState()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (const UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			if (Story->IsHiddenFragmentCollected(GetResolvedFragmentId()))
			{
				ApplyCollectedState();
			}
			else
			{
				ApplyAvailableState();
			}
		}
	}
}

void ABRHiddenFragmentPickup::ApplyCollectedState()
{
	bWasCollected = true;
	if (PickupSphere)
	{
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (PreviewMesh)
	{
		PreviewMesh->SetHiddenInGame(true);
	}
}

void ABRHiddenFragmentPickup::ApplyAvailableState()
{
	bWasCollected = false;
	if (PickupSphere)
	{
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (PreviewMesh)
	{
		PreviewMesh->SetHiddenInGame(false);
	}
}

void ABRHiddenFragmentPickup::HandleHiddenFragmentCollected(FName PersistentId)
{
	if (PersistentId.IsNone() || PersistentId == GetResolvedFragmentId())
	{
		RefreshCollectedState();
	}
}
