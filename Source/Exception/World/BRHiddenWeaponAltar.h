#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRHiddenWeaponAltar.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Hidden Weapon Altar"))
class EXCEPTION_API ABRHiddenWeaponAltar : public AActor
{
	GENERATED_BODY()

public:
	ABRHiddenWeaponAltar();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> AltarMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> InteractionBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Weapon")
	bool bGrantOnlyOnce = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Hidden Weapon")
	bool bHasGrantedReward = false;

	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
