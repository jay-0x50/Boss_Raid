#include "BRNelRequestTrigger.h"

#include "BRNarrativeQueueSubsystem.h"
#include "Story/BRNelCompanion.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
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
	DisplayLine = FText::FromString(TEXT("이 흔적, 그냥 지나치기엔 조금 이상하지 않아?"));
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
	if (!PlayerCharacter || (bTriggerOnce && bWasTriggered))
	{
		return;
	}

	bWasTriggered = true;
	if (bCompletesRequest && !RequestId.IsNone())
	{
		PlayerCharacter->CompleteNelHiddenRequest(RequestId);
	}

	if (!DisplayLine.IsEmpty())
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UBRNarrativeQueueSubsystem* StoryQueue = GI->GetSubsystem<UBRNarrativeQueueSubsystem>())
			{
				StoryQueue->ShowNelLine(DisplayLine, bIsHiddenRequestHint, 4.5f);
			}
		}
	}

	if (bShowNelCompanion && NelCompanion)
	{
		NelCompanion->Appear(NelVisibleTime);
	}

	if (GEngine && bCompletesRequest)
	{
		GEngine->AddOnScreenDebugMessage(5102, 2.5f, FColor::Cyan, RequestCompletedMessage.ToString());
	}

	if (bTriggerOnce && TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (bDestroyOnComplete)
	{
		Destroy();
	}
}
