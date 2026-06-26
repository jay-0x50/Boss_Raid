#include "BRNelRequestTrigger.h"

#include "Player/Character/ExceptionCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABRNelRequestTrigger::ABRNelRequestTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(120.0f, 120.0f, 120.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(RootComponent);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.08f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PreviewMesh->SetStaticMesh(CubeMesh.Object);
	}

	RequestCompletedMessage = FText::FromString(TEXT("Nel request completed."));
}

void ABRNelRequestTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABRNelRequestTrigger::OnRequestBeginOverlap);
	}
}

void ABRNelRequestTrigger::OnRequestBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(OtherActor);
	if (!PlayerCharacter || RequestId.IsNone())
	{
		return;
	}

	PlayerCharacter->CompleteNelHiddenRequest(RequestId);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(5102, 2.5f, FColor::Cyan, RequestCompletedMessage.ToString());
	}

	if (bDestroyOnComplete)
	{
		Destroy();
	}
}
