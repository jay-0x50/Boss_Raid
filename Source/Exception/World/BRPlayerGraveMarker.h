#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRPlayerGraveMarker.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class AExceptionCharacter;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Player Grave Marker"))
class EXCEPTION_API ABRPlayerGraveMarker : public AActor
{
	GENERATED_BODY()

public:
	ABRPlayerGraveMarker();

	UFUNCTION(BlueprintCallable, Category="Exception|Grave")
	void SetStoredExperience(int32 Amount);

	UFUNCTION(BlueprintPure, Category="Exception|Grave")
	int32 GetStoredExperience() const { return StoredExperience; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> GraveMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> RecoverySphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Grave")
	int32 StoredExperience = 0;

	UFUNCTION()
	void OnRecoveryBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
