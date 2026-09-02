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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Story", meta=(ClampMin="1"))
	int32 FragmentAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Story")
	FName FragmentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Story")
	bool bDestroyOnPickup = true;

	UFUNCTION()
	void OnPickupBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHiddenFragmentCollected(FName PersistentId);

private:
	FName GetResolvedFragmentId() const;
	void RefreshCollectedState();
	void ApplyCollectedState();
	void ApplyAvailableState();
	bool bWasCollected = false;
};
