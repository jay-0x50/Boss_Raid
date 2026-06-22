#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRBossActivationPlate.generated.h"

class ABRBossArenaTrigger;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Boss Activation Plate"))
class EXCEPTION_API ABRBossActivationPlate : public AActor
{
	GENERATED_BODY()

public:
	ABRBossActivationPlate();

	UFUNCTION(BlueprintCallable, Category="Exception|Boss Plate")
	void ActivatePlate(AActor* Activator);

	UFUNCTION(BlueprintCallable, Category="Exception|Boss Plate", meta=(WorldContext="WorldContextObject"))
	static bool ActivatePlateByIndex(const UObject* WorldContextObject, int32 PlateIndex, AActor* Activator);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> ActivationBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Boss Plate", meta=(ClampMin="1"))
	int32 PlateIndex = 1;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Boss Plate")
	TObjectPtr<ABRBossArenaTrigger> TargetArena;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Boss Plate")
	bool bActivateOnPlayerOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Boss Plate")
	bool bActivateOnlyOnce = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Boss Plate")
	bool bHasActivated = false;

	UFUNCTION()
	void OnActivationBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
