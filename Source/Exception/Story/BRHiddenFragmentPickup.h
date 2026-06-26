#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRHiddenFragmentPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Hidden Fragment Pickup"))
class EXCEPTION_API ABRHiddenFragmentPickup : public AActor
{
	GENERATED_BODY()

public:
	ABRHiddenFragmentPickup();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Story", meta=(ClampMin="1"))
	int32 FragmentAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Story")
	bool bDestroyOnPickup = true;

	UFUNCTION()
	void OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
