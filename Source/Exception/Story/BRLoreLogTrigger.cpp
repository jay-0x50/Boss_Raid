#include "BRLoreLogTrigger.h"

#include "BRHiddenStorySubsystem.h"
#include "BRNarrativeQueueSubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABRLoreLogTrigger::ABRLoreLogTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 140.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(RootComponent);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -85.0f));
	PreviewMesh->SetRelativeScale3D(FVector(0.65f, 0.18f, 1.05f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PreviewMesh->SetStaticMesh(CubeMesh.Object);
	}

	LogTitle = FText::FromString(TEXT("RUNTIME // SYSTEM LOG"));
	LogText = FText::FromString(TEXT("> NO DATA"));
}

void ABRLoreLogTrigger::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			Story->OnNarrativeBeatConsumed.AddDynamic(this, &ABRLoreLogTrigger::HandleNarrativeBeatConsumed);
		}
	}
	RefreshConsumedState();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABRLoreLogTrigger::OnLogBeginOverlap);
	}
}

void ABRLoreLogTrigger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			Story->OnNarrativeBeatConsumed.RemoveDynamic(this, &ABRLoreLogTrigger::HandleNarrativeBeatConsumed);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ABRLoreLogTrigger::OnLogBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ((bTriggerOnce && bWasRead) || !Cast<AExceptionCharacter>(OtherActor) || LogText.IsEmpty())
	{
		return;
	}

	bWasRead = true;
	if (bTriggerOnce)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
			{
				if (!Story->TryConsumeNarrativeBeat(GetResolvedBeatId()))
				{
					ApplyConsumedState();
					return;
				}
			}
		}
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBRNarrativeQueueSubsystem* StoryQueue = GI->GetSubsystem<UBRNarrativeQueueSubsystem>())
		{
			StoryQueue->ShowSystemLogDeferred(LogText, ShowTime, LogTitle);
		}
	}
	ApplyConsumedState();
}

FName ABRLoreLogTrigger::GetResolvedBeatId() const
{
	return BeatId.IsNone() ? GetFName() : BeatId;
}

void ABRLoreLogTrigger::RefreshConsumedState()
{
	if (!bTriggerOnce)
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (const UBRHiddenStorySubsystem* Story = GI->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			if (Story->IsNarrativeBeatConsumed(GetResolvedBeatId()))
			{
				ApplyConsumedState();
			}
			else
			{
				ApplyAvailableState();
			}
		}
	}
}

void ABRLoreLogTrigger::ApplyConsumedState()
{
	bWasRead = true;
	if (bTriggerOnce && TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (bHideObjectAfterRead && PreviewMesh)
	{
		PreviewMesh->SetHiddenInGame(true);
	}
}

void ABRLoreLogTrigger::ApplyAvailableState()
{
	bWasRead = false;
	if (bTriggerOnce && TriggerBox)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (bHideObjectAfterRead && PreviewMesh)
	{
		PreviewMesh->SetHiddenInGame(false);
	}
}

void ABRLoreLogTrigger::HandleNarrativeBeatConsumed(FName PersistentId)
{
	if (PersistentId.IsNone() || PersistentId == GetResolvedBeatId())
	{
		RefreshConsumedState();
	}
}
