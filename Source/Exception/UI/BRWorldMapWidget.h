#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRWorldMapWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;
class UWidget;

USTRUCT()
struct FBRMapLineVisual
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UBorder> Widget;

	FVector2D WorldA = FVector2D::ZeroVector;
	FVector2D WorldB = FVector2D::ZeroVector;
	FName RegionId = NAME_None;
};

USTRUCT()
struct FBRMapPointVisual
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UWidget> Widget;

	FVector2D WorldLocation = FVector2D::ZeroVector;
	FName RegionId = NAME_None;
	bool bFullMapOnly = false;
};

UCLASS(Blueprintable, BlueprintType)
class EXCEPTION_API UBRWorldMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Exception|Map")
	void SetFullMapMode(bool bNewFullMapMode);

	UFUNCTION(BlueprintPure, Category="Exception|Map")
	bool IsFullMapMode() const { return bFullMapMode; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildMapWidget();
	void AddRoute(const TArray<FVector2D>& Points, FName RegionId);
	void AddMarker(const FVector2D& WorldLocation, FName RegionId, const FString& Label, bool bFullMapOnly = false);
	void AddRegionLabel(const FVector2D& WorldLocation, FName RegionId, const FString& Label);
	void RefreshDiscovery();
	void RefreshLayout();
	void RefreshPlayerMarker();
	FVector2D WorldToMap(const FVector2D& WorldLocation) const;
	FName FindClosestRegion(const FVector2D& WorldLocation) const;
	bool IsRegionUnlocked(FName RegionId) const;
	FLinearColor GetRegionColor(FName RegionId) const;

	UFUNCTION()
	void HandleMapRegionUnlocked(FName RegionId, int32 UnlockedCount);

	/** Native defaults stay playable, while a Blueprint subclass can tune the final HUD layout. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Map|Layout", meta=(AllowPrivateAccess="true"))
	FVector2D MiniMapFrameSize = FVector2D(340.0f, 260.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Map|Layout", meta=(AllowPrivateAccess="true"))
	FVector2D MiniMapScreenMargin = FVector2D(28.0f, 28.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Map|Layout", meta=(AllowPrivateAccess="true"))
	FVector2D FullMapFrameSize = FVector2D(1180.0f, 740.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Exception|Map|Layout", meta=(AllowPrivateAccess="true"))
	FVector2D MiniMapWorldSpan = FVector2D(5000.0f, 3900.0f);

	UPROPERTY()
	TObjectPtr<UBorder> FullScreenShade;

	UPROPERTY()
	TObjectPtr<UBorder> MapFrame;

	UPROPERTY()
	TObjectPtr<UCanvasPanel> MapCanvas;

	UPROPERTY()
	TObjectPtr<UTextBlock> MapTitle;

	UPROPERTY()
	TObjectPtr<UTextBlock> ModeHintText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PlayerMarker;

	UPROPERTY()
	TArray<FBRMapLineVisual> RouteLines;

	UPROPERTY()
	TArray<FBRMapPointVisual> PointVisuals;

	UPROPERTY()
	TMap<FName, TObjectPtr<UTextBlock>> RegionLabels;

	TMap<FName, TArray<FVector2D>> RegionRoutes;
	FVector2D MapOrigin = FVector2D::ZeroVector;
	FVector2D MapSize = FVector2D::ZeroVector;
	FVector2D MiniMapWorldCenter = FVector2D::ZeroVector;
	bool bFullMapMode = false;
};
