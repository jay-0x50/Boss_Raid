#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRBossArenaTrigger.generated.h"

class ABRBossBase;
class ABRBossTeamCoordinator;
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
	void ActivateArena();

	UFUNCTION(BlueprintCallable, Category="Exception|Arena")
	void ResetArenaForRetry();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena", meta=(DisplayName="Primary Boss"))
	TObjectPtr<ABRBossBase> BossDummy;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena")
	TArray<TObjectPtr<ABRBossBase>> BossActors;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena|Spawn")
	TSubclassOf<ABRBossBase> BossClassToSpawn;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena|Spawn")
	TObjectPtr<AActor> BossSpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Spawn")
	FVector BossSpawnOffset = FVector(600.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Spawn")
	bool bSpawnBossOnArenaStart = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena")
	TObjectPtr<AActor> GateActorToHideOnDefeat;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Exception|Arena")
	TObjectPtr<AActor> RewardActorToShowOnDefeat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bResetBossOnEnter = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bStartOnPlayerOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bAutoIncludeTeamMembers = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bAutoIncludeNearbyBosses = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bDeactivateUnmanagedBossesOnStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Intro")
	bool bPlayBossIntroBeforeAI = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Intro", meta=(ClampMin="0.0", Units="s"))
	float BossIntroDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Intro")
	bool bHideBossStatusUntilIntroFinished = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Story")
	FText BossStoryTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Story", meta=(MultiLine="true"))
	FText BossIntroLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Story", meta=(MultiLine="true"))
	FText BossDefeatLog;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Story")
	FName BossStoryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Story")
	TArray<FName> RequiredBossStoryIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Story")
	bool bBlockArenaUntilStoryReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena|Story", meta=(MultiLine="true"))
	FText StoryLockedLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exception|Arena", meta=(ClampMin="0.0", Units="cm"))
	float AutoBossSearchRadius = 5000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bArenaStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exception|Arena")
	bool bArenaCleared = false;

	UPROPERTY(Transient)
	TObjectPtr<UBRBossStatusWidget> BossStatusWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ABRBossBase>> SpawnedBosses;

	FTimerHandle BossIntroTimerHandle;

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
	void ActivateManagedBossesAfterIntro();
	ABRBossBase* SpawnConfiguredBossIfNeeded();
	FTransform GetConfiguredBossSpawnTransform() const;
	void BindBossEvents(ABRBossBase* Boss);
	void BuildManagedBossList(TArray<ABRBossBase*>& OutBosses) const;
	void AddBossAndLinkedTeam(ABRBossBase* Boss, TArray<ABRBossBase*>& OutBosses, bool bIncludeLinkedTeam) const;
	void AddTeamMembers(ABRBossTeamCoordinator* TeamCoordinator, TArray<ABRBossBase*>& OutBosses) const;
	void AddNearbyBosses(TArray<ABRBossBase*>& OutBosses) const;
	void DeactivateUnmanagedBosses(const TArray<ABRBossBase*>& ManagedBosses) const;
	bool AreAllManagedBossesDead() const;
	UBRBossStatusWidget* ShowBossStatusWidget();
	void RefreshBossStatusWidget();
	void HideBossStatusWidget();
	void ShowBossStoryIntro();
	bool IsStoryReadyToStart() const;
};
