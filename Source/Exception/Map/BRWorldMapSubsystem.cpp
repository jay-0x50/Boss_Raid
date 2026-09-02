#include "BRWorldMapSubsystem.h"

bool UBRWorldMapSubsystem::UnlockRegion(FName RegionId)
{
	if (RegionId.IsNone() || UnlockedRegionIds.Contains(RegionId))
	{
		return false;
	}

	UnlockedRegionIds.Add(RegionId);
	OnMapRegionUnlocked.Broadcast(RegionId, UnlockedRegionIds.Num());
	return true;
}

bool UBRWorldMapSubsystem::IsRegionUnlocked(FName RegionId) const
{
	return !RegionId.IsNone() && UnlockedRegionIds.Contains(RegionId);
}

TArray<FName> UBRWorldMapSubsystem::GetUnlockedRegionIds() const
{
	TArray<FName> Result;
	Result.Reserve(UnlockedRegionIds.Num());
	for (const FName& RegionId : UnlockedRegionIds)
	{
		Result.Add(RegionId);
	}
	Result.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
	return Result;
}

void UBRWorldMapSubsystem::ApplySavedMapState(const TArray<FName>& SavedRegionIds)
{
	UnlockedRegionIds.Reset();
	for (const FName& RegionId : SavedRegionIds)
	{
		if (!RegionId.IsNone())
		{
			UnlockedRegionIds.Add(RegionId);
		}
	}

	OnMapRegionUnlocked.Broadcast(NAME_None, UnlockedRegionIds.Num());
}

void UBRWorldMapSubsystem::ResetMapState()
{
	UnlockedRegionIds.Reset();
	OnMapRegionUnlocked.Broadcast(NAME_None, 0);
}
