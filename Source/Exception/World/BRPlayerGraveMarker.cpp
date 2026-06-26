#include "BRPlayerGraveMarker.h"

#include "Player/Character/ExceptionCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

ABRPlayerGraveMarker::ABRPlayerGraveMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GraveMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GraveMeshComponent"));
	GraveMeshComponent->SetupAttachment(SceneRoot);
	GraveMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GraveMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GraveMeshComponent->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.4f));

	RecoverySphere = CreateDefaultSubobject<USphereComponent>(TEXT("RecoverySphere"));
	RecoverySphere->SetupAttachment(SceneRoot);
	RecoverySphere->SetSphereRadius(130.0f);
	RecoverySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RecoverySphere->SetCollisionObjectType(ECC_WorldDynamic);
	RecoverySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	RecoverySphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		GraveMeshComponent->SetStaticMesh(CubeMesh.Object);
	}
}

void ABRPlayerGraveMarker::BeginPlay()
{
	Super::BeginPlay();

	if (RecoverySphere)
	{
		RecoverySphere->OnComponentBeginOverlap.AddDynamic(this, &ABRPlayerGraveMarker::OnRecoveryBeginOverlap);
	}
}

void ABRPlayerGraveMarker::SetStoredExperience(int32 Amount)
{
	StoredExperience = FMath::Max(0, Amount);
}

void ABRPlayerGraveMarker::OnRecoveryBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(OtherActor);
	if (!PlayerCharacter || StoredExperience <= 0)
	{
		return;
	}

	PlayerCharacter->RecoverDroppedExperience(StoredExperience);
	StoredExperience = 0;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3010, 2.0f, FColor::Green, TEXT("Runtime grave recovered"));
	}

	Destroy();
}
