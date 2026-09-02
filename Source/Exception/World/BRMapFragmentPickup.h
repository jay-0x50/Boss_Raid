#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRMapFragmentPickup.generated.h"

class UPointLightComponent;
class URotatingMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="World Map Fragment"))
class EXCEPTION_API ABRMapFragmentPickup : public AActor
{
	GENERATED_BODY()

public:
	ABRMapFragmentPickup();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> FragmentMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> FragmentLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Map")
	FName RegionId = TEXT("Field1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Map")
	FText RegionDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Map", meta=(MultiLine="true"))
	FText UnlockLine;

	UFUNCTION()
	void OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleMapRegionUnlocked(FName ChangedRegionId, int32 UnlockedCount);

private:
	void DisableCollectedFragment();
};
