// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatActivationVolume.generated.h"

class UBoxComponent;

/**
 *  A simple volume that activates a list of actors when the player pawn enters.
 */
UCLASS()
class ACombatActivationVolume : public AActor
{
	GENERATED_BODY()

	/** Collision box volume */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* Box;
	
protected:

	/** List of actors to activate when this volume is entered */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Activation Volume")
	TArray<AActor*> ActorsToActivate;

	/** Prevents repeated activation from the same one-way field trigger */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Activation Volume")
	bool bActivateOnlyOnce = false;

	/** Deactivates linked actors when the player leaves this volume */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Activation Volume")
	bool bDeactivateOnPlayerExit = false;

	/** True after this volume has activated its actor list */
	bool bHasActivated = false;

public:	
	
	/** Constructor */
	ACombatActivationVolume();

protected:

	/** Validates reusable/one-shot option combinations once gameplay starts */
	virtual void BeginPlay() override;

	/** Handles overlaps with the box volume */
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Handles optional cleanup for a reusable encounter volume */
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Activates or deactivates every valid linked actor */
	void SetLinkedActorsActive(bool bActive, AActor* ActivationInstigator);

public:

	/** Makes a one-shot volume usable again; optionally resets linked encounters now */
	UFUNCTION(BlueprintCallable, Category="Activation Volume")
	void ResetActivation(bool bDeactivateLinkedActors = true);

};
