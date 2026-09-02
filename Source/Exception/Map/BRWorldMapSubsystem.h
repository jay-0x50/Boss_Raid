#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BRWorldMapSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRMapRegionUnlocked, FName, RegionId, int32, UnlockedCount);

UCLASS(BlueprintType)
class EXCEPTION_API UBRWorldMapSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|Map")
	bool UnlockRegion(FName RegionId);

	UFUNCTION(BlueprintPure, Category="Exception|Map")
	bool IsRegionUnlocked(FName RegionId) const;

	UFUNCTION(BlueprintPure, Category="Exception|Map")
	int32 GetUnlockedRegionCount() const { return UnlockedRegionIds.Num(); }

	UFUNCTION(BlueprintPure, Category="Exception|Map")
	TArray<FName> GetUnlockedRegionIds() const;

	UFUNCTION(BlueprintCallable, Category="Exception|Map")
	void ApplySavedMapState(const TArray<FName>& SavedRegionIds);

	UFUNCTION(BlueprintCallable, Category="Exception|Map")
	void ResetMapState();

	UPROPERTY(BlueprintAssignable, Category="Exception|Map")
	FBRMapRegionUnlocked OnMapRegionUnlocked;

private:
	UPROPERTY()
	TSet<FName> UnlockedRegionIds;
};
