#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRBossArenaTrigger.generated.h"

class ABRBossBase;
class UBRBossStatusWidget;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="Boss Arena Trigger"))
class EXCEPTION_API ABRBossArenaTrigger : public AActor
{
	GENERATED_BODY()

public:
	ABRBossArenaTrigger();

	UFUNCTION(BlueprintCallable, Category="Exception|Arena")
	void ResetArenaForRetry();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena")
	TObjectPtr<ABRBossBase> BossDummy;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena")
	TArray<TObjectPtr<ABRBossBase>> BossActors;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena")
	TObjectPtr<AActor> GateActorToHideOnDefeat;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena")
	TObjectPtr<AActor> RewardActorToShowOnDefeat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bResetBossOnEnter = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bArenaStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bArenaCleared = false;

	UPROPERTY(Transient)
	TObjectPtr<UBRBossStatusWidget> BossStatusWidget;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleBossDefeated();

	UFUNCTION()
	void HandleBossStatChanged(float CurrentValue, float MaxValue, float NormalizedValue);

	UFUNCTION()
	void HandleBossStateChanged();

	UFUNCTION()
	void HandleBossExecutionStateChanged(AActor* Executor);

	void StartArena();
	void BuildManagedBossList(TArray<ABRBossBase*>& OutBosses) const;
	bool AreAllManagedBossesDead() const;
	UBRBossStatusWidget* ShowBossStatusWidget();
	void RefreshBossStatusWidget();
	void HideBossStatusWidget();
};
