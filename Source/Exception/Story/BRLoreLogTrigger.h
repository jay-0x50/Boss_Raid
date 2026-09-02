#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRLoreLogTrigger.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Lore Log Trigger"))
class EXCEPTION_API ABRLoreLogTrigger : public AActor
{
	GENERATED_BODY()

public:
	ABRLoreLogTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story")
	FText LogTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story", meta=(MultiLine="true"))
	FText LogText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story", meta=(ClampMin="0.5", Units="s"))
	float ShowTime = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story")
	bool bTriggerOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story")
	bool bHideObjectAfterRead = true;

	UFUNCTION()
	void OnLogBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bWasRead = false;
};
