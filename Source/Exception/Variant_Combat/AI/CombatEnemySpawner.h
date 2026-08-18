// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatActivatable.h"
#include "CombatEnemySpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class ACombatEnemy;

/**
 *  A basic Actor in charge of spawning Enemy Characters and monitoring their deaths.
 *  A configurable number of enemies can stay alive together, which lets one spawner
 *  represent a small field encounter instead of a single-file spawn queue.
 *  The spawner can be remotely activated through the ICombatActivatable interface
 *  When the last spawned enemy dies, the spawner can also activate other ICombatActivatables
 */
UCLASS(abstract)
class ACombatEnemySpawner : public AActor, public ICombatActivatable
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;

protected:

	/** Type of enemy to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner")
	TSubclassOf<ACombatEnemy> EnemyClass;

	/** If true, the first enemy will be spawned as soon as the game starts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner")
	bool bShouldSpawnEnemiesImmediately = true;

	/** Time to wait before spawning the first enemy on game start */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float InitialSpawnDelay = 5.0f;

	/** Auto-spawn encounters wait until the local player is close, keeping distant field groups dormant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner|Distance Activation")
	bool bWaitForPlayerWhenAutoSpawning = false;

	/** Player distance required to start an auto-spawn encounter */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner|Distance Activation", meta = (EditCondition = "bWaitForPlayerWhenAutoSpawning", ClampMin = 100, ClampMax = 20000, Units = "cm"))
	float AutoActivationDistance = 2400.0f;

	/** How often a dormant auto-spawn encounter checks player distance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner|Distance Activation", meta = (EditCondition = "bWaitForPlayerWhenAutoSpawning", ClampMin = 0.1, ClampMax = 5.0, Units = "s"))
	float AutoActivationCheckInterval = 0.5f;

	/** Number of enemies to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 100))
	int32 SpawnCount = 1;

	/** Maximum number of enemies from this spawner that may be alive at once */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 1, ClampMax = 20))
	int32 MaxAliveEnemies = 1;

	/** Radius used to spread simultaneously alive enemies around the spawn marker */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 2000, Units = "cm"))
	float SpawnSpreadRadius = 180.0f;

	/** Time to wait before spawning the next enemy after the current one dies */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float RespawnDelay = 5.0f;

	/** Time to wait after this spawner is depleted before activating the actor list */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Activation", meta = (ClampMin = 0, ClampMax = 10))
	float ActivationDelay = 1.0f;

	/** List of actors to activate after the last enemy dies */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Activation")
	TArray<AActor*> ActorsToActivateWhenDepleted;

	/** Number of enemies that have not been spawned during the current run */
	int32 RemainingSpawnCount = 0;

	/** Number of spawn attempts that succeeded during the current run */
	int32 SpawnedCount = 0;

	/** True while this encounter is allowed to spawn and monitor enemies */
	bool bEncounterActive = false;

	/** True after the current run has spawned and defeated all configured enemies */
	bool bEncounterDepleted = false;

	/** Enemies that are alive and count against MaxAliveEnemies */
	TSet<TWeakObjectPtr<ACombatEnemy>> ActiveEnemies;

	/** Every still-existing enemy body created by the current run */
	TArray<TWeakObjectPtr<ACombatEnemy>> SpawnedEnemies;

	/** Timer to spawn enemies after a delay */
	FTimerHandle SpawnTimer;

	/** Timer to notify linked actors after this encounter is depleted */
	FTimerHandle DepletedTimer;

public:	
	
	/** Constructor */
	ACombatEnemySpawner();

public:

	/** Initialization */
	virtual void BeginPlay() override;

	/** Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** Starts a fresh run with the configured enemy count */
	void StartEncounter();

	/** Starts now or keeps polling until the player is near this auto-spawn encounter */
	void TryAutoActivate();

	/** Arms automatic activation after the requested delay */
	void ArmAutoActivation(float Delay);

	/** Fills all available alive slots and subscribes to spawned enemies */
	void SpawnEnemy();

	/** Called when the spawned enemy has died */
	void OnEnemyDied(ACombatEnemy* Enemy);

	/** Handles enemies removed by external gameplay before their death event */
	UFUNCTION()
	void OnSpawnedEnemyDestroyed(AActor* DestroyedActor);

	/** Updates spawning or completion after an active enemy leaves the encounter */
	void HandleEnemyRemoved(ACombatEnemy* Enemy);

	/** Schedules the next attempt to fill available alive slots */
	void ScheduleSpawn(float Delay);

	/** Marks the encounter complete and safely handles a zero activation delay */
	void FinishEncounter();

	/** Called after the last spawned enemy has died */
	void SpawnerDepleted();

	/** Destroys current enemies and restores the configured runtime count */
	void ClearEncounter();

public:

	// ~begin ICombatActivatable interface

	/** Toggles the Spawner */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void ToggleInteraction(AActor* ActivationInstigator) override;

	/** Activates the Spawner */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void ActivateInteraction(AActor* ActivationInstigator) override;

	/** Deactivates the Spawner */
	UFUNCTION(BlueprintCallable, Category="Activatable")
	virtual void DeactivateInteraction(AActor* ActivationInstigator) override;

	// ~end IActivatable interface

	/** Clears this encounter so it can be activated again (for checkpoints or player respawn) */
	UFUNCTION(BlueprintCallable, Category="Enemy Spawner")
	void ResetEncounter(bool bRestartImmediately = false);

	/** Returns the number of enemies currently alive in this encounter */
	UFUNCTION(BlueprintPure, Category="Enemy Spawner")
	int32 GetAliveEnemyCount() const { return ActiveEnemies.Num(); }

	/** Returns true once every enemy in the current run has been defeated */
	UFUNCTION(BlueprintPure, Category="Enemy Spawner")
	bool IsEncounterDepleted() const { return bEncounterDepleted; }
};
