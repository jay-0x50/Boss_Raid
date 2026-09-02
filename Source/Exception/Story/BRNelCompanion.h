#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRNelCompanion.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/** Nel materializes at authored story beats, faces the player, then dissolves. */
UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Nel Companion"))
class EXCEPTION_API ABRNelCompanion : public AActor
{
	GENERATED_BODY()

public:
	ABRNelCompanion();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Nel")
	void Appear(float HoldTime = 6.0f);

	UFUNCTION(BlueprintCallable, Category="Nel")
	void Disappear();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Nel")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Nel")
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Nel")
	TObjectPtr<UStaticMeshComponent> RobeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Nel")
	TObjectPtr<UPointLightComponent> AuraLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nel|Fade", meta=(ClampMin="0.05", Units="s"))
	float FadeInTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nel|Fade", meta=(ClampMin="0.05", Units="s"))
	float FadeOutTime = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nel|Fade", meta=(ClampMin="0.1", Units="s"))
	float DefaultHoldTime = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nel|Look")
	bool bFacePlayerOnAppear = true;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GhostMaterials;

private:
	void BuildDynamicMaterials();
	void ApplyFade(float InAlpha);
	void FacePlayer();

	float FadeAlpha = 0.0f;
	float HoldRemaining = 0.0f;
	bool bFadingIn = false;
	bool bFadingOut = false;
};
