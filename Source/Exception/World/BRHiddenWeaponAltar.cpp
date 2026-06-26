#include "BRHiddenWeaponAltar.h"

#include "BRHiddenStorySubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

ABRHiddenWeaponAltar::ABRHiddenWeaponAltar()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AltarMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarMeshComponent"));
	AltarMeshComponent->SetupAttachment(SceneRoot);
	AltarMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AltarMeshComponent->SetRelativeScale3D(FVector(2.4f, 2.4f, 0.35f));

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(180.0f, 180.0f, 140.0f));
	InteractionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		AltarMeshComponent->SetStaticMesh(CubeMesh.Object);
	}
}

void ABRHiddenWeaponAltar::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionBox)
	{
		InteractionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ABRHiddenWeaponAltar::OnInteractionBeginOverlap);
	}
}

void ABRHiddenWeaponAltar::OnInteractionBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (bGrantOnlyOnce && bHasGrantedReward)
	{
		return;
	}

	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRHiddenStorySubsystem* HiddenStory = GameInstance->GetSubsystem<UBRHiddenStorySubsystem>())
		{
			HiddenStory->MarkNelHiddenRequestCompleted(TEXT("Nel_FindPythonTrace"));
			HiddenStory->MarkNelHiddenRequestCompleted(TEXT("Nel_DecodePerlSigil"));
			HiddenStory->MarkNelHiddenRequestCompleted(TEXT("Nel_RecoverRuntimeShard"));
			HiddenStory->CollectHiddenFragment(HiddenStory->GetRequiredHiddenFragmentCount());
			HiddenStory->SetMimikatzAuthoritySeizedUnlocked(true);
		}
	}

	PlayerCharacter->RefreshHiddenStoryRewards();
	bHasGrantedReward = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3201, 4.0f, FColor::Purple, TEXT("Hidden Weapon Altar: Mimikatz, Authority Seized"));
	}
}
