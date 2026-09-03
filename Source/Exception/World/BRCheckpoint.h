#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRCheckpoint.generated.h"

class UBillboardComponent;
class USphereComponent;
class UStaticMeshComponent;
class USoundBase;
class AExceptionCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRCheckpointEvent, AExceptionCharacter*, PlayerCharacter);

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Exception Checkpoint"))
class EXCEPTION_API ABRCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	ABRCheckpoint();

	UPROPERTY(BlueprintAssignable, Category="Exception|Checkpoint")
	FBRCheckpointEvent OnCheckpointActivated;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> ActivationSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Checkpoint")
	bool bRestorePlayerOnActivation = true;

	/** Optional authored cue. The Blueprint/event hook still fires when no final sound is assigned. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Checkpoint|Audio")
	TObjectPtr<USoundBase> ActivationSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Checkpoint|Audio", meta=(ClampMin="0.0"))
	float ActivationSoundVolume = 1.0f;

	UFUNCTION(BlueprintImplementableEvent, Category="Exception|Checkpoint", meta=(DisplayName="Checkpoint Activated"))
	void BP_CheckpointActivated(AExceptionCharacter* PlayerCharacter);

	UFUNCTION()
	void OnActivationBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
