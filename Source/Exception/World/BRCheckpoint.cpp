#include "BRCheckpoint.h"

#include "BRSaveGameSubsystem.h"
#include "Player/Character/ExceptionCharacter.h"
#include "Player/Controller/ExceptionPlayerController.h"
#include "ExceptionGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ABRCheckpoint::ABRCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetWorldScale3D(FVector(0.8f, 0.8f, 0.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CylinderMesh.Object);
	}

	ActivationSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationSphere"));
	ActivationSphere->SetupAttachment(RootComponent);
	ActivationSphere->SetSphereRadius(160.0f);
	ActivationSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ActivationSphere->SetCollisionObjectType(ECC_WorldDynamic);
	ActivationSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	ActivationSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ABRCheckpoint::BeginPlay()
{
	Super::BeginPlay();

	if (ActivationSphere)
	{
		ActivationSphere->OnComponentBeginOverlap.AddDynamic(this, &ABRCheckpoint::OnActivationBeginOverlap);
	}
}

void ABRCheckpoint::OnActivationBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AExceptionCharacter* PlayerCharacter = Cast<AExceptionCharacter>(OtherActor);
	// RespawnAtCheckpoint teleports the still-dead pawn into this overlap before
	// restoring its stats. Treat that as arrival, not a new rest interaction;
	// reopening the pause menu here freezes the world timers used by every later
	// retry (boss intro AI and the next player respawn included).
	if (!PlayerCharacter || PlayerCharacter->GetCurrentHP() <= 0.0f)
	{
		return;
	}

	if (AExceptionGameMode* ExceptionGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AExceptionGameMode>() : nullptr)
	{
		FTransform SavedTransform = GetActorTransform();
		SavedTransform.SetLocation(GetActorLocation() + FVector(0.0f, 0.0f, 100.0f));
		SavedTransform.SetScale3D(FVector::OneVector);
		ExceptionGameMode->SetCheckpointTransform(SavedTransform);
	}

	if (bRestorePlayerOnActivation)
	{
		PlayerCharacter->RestoreHPAndStamina();
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
		GEngine->AddOnScreenDebugMessage(3001, 1.5f, FColor::Green, TEXT("Checkpoint Activated"));
	}

	if (ActivationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ActivationSound, GetActorLocation(), ActivationSoundVolume);
	}
	OnCheckpointActivated.Broadcast(PlayerCharacter);
	BP_CheckpointActivated(PlayerCharacter);

	if (AExceptionPlayerController* PlayerController = Cast<AExceptionPlayerController>(PlayerCharacter->GetController()))
	{
		PlayerController->ShowPauseMenuWidget();
	}
}
