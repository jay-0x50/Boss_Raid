#include "BRHiddenFragmentPickup.h"

#include "Player/Character/ExceptionCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
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

	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ABRHiddenFragmentPickup::OnPickupBeginOverlap);
	}
}

void ABRHiddenFragmentPickup::OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->CollectHiddenFragment(FragmentAmount);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(5101, 2.0f, FColor::Purple, TEXT("Hidden Fragment Collected"));
	}

	if (bDestroyOnPickup)
	{
		Destroy();
	}
}
