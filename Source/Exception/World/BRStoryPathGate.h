#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRStoryPathGate.generated.h"

class ACameraActor;
class USceneComponent;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Story Path Gate"))
class EXCEPTION_API ABRStoryPathGate : public AActor
{
	GENERATED_BODY()

public:
	ABRStoryPathGate();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Exception|Story Gate")
	void OpenGate(bool bPlayReveal = true);

	UFUNCTION(BlueprintCallable, Category="Exception|Story Gate")
	void RefreshGateState(bool bInstant = true);

	UFUNCTION(BlueprintPure, Category="Exception|Story Gate")
	bool IsGateOpen() const { return bGateOpen; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story Gate")
	FName RequiredBossId = NAME_None;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Story Gate")
	TArray<TObjectPtr<AActor>> GatePieces;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Story Gate")
	TObjectPtr<ACameraActor> RevealCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story Gate", meta=(MultiLine="true"))
	FText GateOpenLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story Gate", meta=(ClampMin="0.25", Units="s"))
	float OpenDuration = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story Gate", meta=(ClampMin="0.0", Units="cm"))
	float SinkDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story Gate", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CollisionReleaseAlpha = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Story Gate")
	bool bPlayRevealOnUnlock = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Exception|Story Gate")
	bool bGateOpen = false;

private:
	UFUNCTION()
	void HandleMainBossProgressChanged(FName BossId, int32 DefeatedCount);

	void CacheGatePieceTransforms();
	void SetGateOpenInstant();
	void SetGateClosedInstant();
	void SetGateCollision(bool bEnabled);
	void StartRevealCamera();
	void RestorePlayerCamera();

	TArray<FTransform> GatePieceStartTransforms;
	TWeakObjectPtr<AActor> PreviousViewTarget;
	FTimerHandle RevealTimerHandle;
	float OpenElapsed = 0.0f;
	bool bOpening = false;
	bool bCollisionReleased = false;
};
