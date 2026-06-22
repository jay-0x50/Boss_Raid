#include "BRBossActivationPlate.h"

#include "BRBossArenaTrigger.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ABRBossActivationPlate::ABRBossActivationPlate()
{
	PrimaryActorTick.bCanEverTick = false;

	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	SetRootComponent(PlateMesh);
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlateMesh->SetWorldScale3D(FVector(2.2f, 2.2f, 0.12f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		PlateMesh->SetStaticMesh(CylinderMesh.Object);
	}

	ActivationBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ActivationBox"));
	ActivationBox->SetupAttachment(RootComponent);
	ActivationBox->SetBoxExtent(FVector(140.0f, 140.0f, 80.0f));
	ActivationBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ActivationBox->SetCollisionObjectType(ECC_WorldDynamic);
	ActivationBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ActivationBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ABRBossActivationPlate::BeginPlay()
{
	Super::BeginPlay();

	if (ActivationBox)
	{
		ActivationBox->OnComponentBeginOverlap.AddDynamic(this, &ABRBossActivationPlate::OnActivationBeginOverlap);
	}
}

void ABRBossActivationPlate::OnActivationBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bActivateOnPlayerOverlap && Cast<AExceptionCharacter>(OtherActor))
	{
		ActivatePlate(OtherActor);
	}
}

void ABRBossActivationPlate::ActivatePlate(AActor* Activator)
{
	if (bActivateOnlyOnce && bHasActivated)
	{
		return;
	}

	if (!TargetArena)
	{
		if (GEngine)
		{
			const FString Message = FString::Printf(TEXT("Boss Plate %d has no Target Arena."), PlateIndex);
			GEngine->AddOnScreenDebugMessage(4101 + PlateIndex, 3.0f, FColor::Yellow, Message);
		}
		return;
	}

	bHasActivated = true;
	TargetArena->ActivateArena();

	if (GEngine)
	{
		const FString Message = FString::Printf(TEXT("Boss Plate %d Activated"), PlateIndex);
		GEngine->AddOnScreenDebugMessage(4101 + PlateIndex, 1.5f, FColor::Cyan, Message);
	}
}

bool ABRBossActivationPlate::ActivatePlateByIndex(const UObject* WorldContextObject, int32 PlateIndex, AActor* Activator)
{
	if (!WorldContextObject)
	{
		return false;
	}

	TArray<AActor*> FoundPlateActors;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ABRBossActivationPlate::StaticClass(), FoundPlateActors);
	for (AActor* FoundActor : FoundPlateActors)
	{
		ABRBossActivationPlate* Plate = Cast<ABRBossActivationPlate>(FoundActor);
		if (Plate && Plate->PlateIndex == PlateIndex)
		{
			Plate->ActivatePlate(Activator);
			return true;
		}
	}

	if (GEngine)
	{
		const FString Message = FString::Printf(TEXT("Boss Plate %d not found."), PlateIndex);
		GEngine->AddOnScreenDebugMessage(4110 + PlateIndex, 2.0f, FColor::Yellow, Message);
	}
	return false;
}
