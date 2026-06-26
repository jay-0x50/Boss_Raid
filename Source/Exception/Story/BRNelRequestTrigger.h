#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRNelRequestTrigger.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Nel Request Trigger"))
class EXCEPTION_API ABRNelRequestTrigger : public AActor
{
	GENERATED_BODY()

public:
	ABRNelRequestTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Story")
	FName RequestId = TEXT("Nel_FindPythonTrace");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Story")
	FText RequestCompletedMessage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Hidden Story")
	bool bDestroyOnComplete = false;

	UFUNCTION()
	void OnRequestBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
