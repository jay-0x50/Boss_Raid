// Copyright Epic Games, Inc. All Rights Reserved.


#include "CombatActivationVolume.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "CombatActivatable.h"

ACombatActivationVolume::ACombatActivationVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the box volume
	RootComponent = Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	check(Box);

	// set the box's extent
	Box->SetBoxExtent(FVector(500.0f, 500.0f, 500.0f));

	// set the default collision profile to overlap all dynamic
	Box->SetCollisionProfileName(FName("OverlapAllDynamic"));

	// bind the begin overlap 
	Box->OnComponentBeginOverlap.AddDynamic(this, &ACombatActivationVolume::OnOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &ACombatActivationVolume::OnEndOverlap);
}

void ACombatActivationVolume::BeginPlay()
{
	Super::BeginPlay();

	if (bActivateOnlyOnce && bDeactivateOnPlayerExit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Activation volume %s had one-shot and exit-deactivation enabled together; it will run as a reusable proximity volume."), *GetNameSafe(this));
		bActivateOnlyOnce = false;
	}
}

void ACombatActivationVolume::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!PlayerPawn || !PlayerPawn->IsPlayerControlled() || OtherComp != PlayerPawn->GetRootComponent())
	{
		return;
	}

	if (bActivateOnlyOnce && bHasActivated)
	{
		return;
	}

	bHasActivated = true;
	SetLinkedActorsActive(true, PlayerPawn);
}

void ACombatActivationVolume::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!bDeactivateOnPlayerExit || !PlayerPawn || !PlayerPawn->IsPlayerControlled() || OtherComp != PlayerPawn->GetRootComponent())
	{
		return;
	}

	SetLinkedActorsActive(false, PlayerPawn);
	// Exit-deactivation represents a reusable proximity volume, even if the one-shot option was also checked.
	bHasActivated = false;
}

void ACombatActivationVolume::SetLinkedActorsActive(bool bActive, AActor* ActivationInstigator)
{
	for (AActor* CurrentActor : ActorsToActivate)
	{
		if (ICombatActivatable* Activatable = Cast<ICombatActivatable>(CurrentActor))
		{
			if (bActive)
			{
				Activatable->ActivateInteraction(ActivationInstigator);
			}
			else
			{
				Activatable->DeactivateInteraction(ActivationInstigator);
			}
		}
	}
}

void ACombatActivationVolume::ResetActivation(bool bDeactivateLinkedActors)
{
	bHasActivated = false;
	if (bDeactivateLinkedActors)
	{
		SetLinkedActorsActive(false, this);
	}
}
