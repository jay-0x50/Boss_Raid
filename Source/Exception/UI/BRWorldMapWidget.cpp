#include "BRWorldMapWidget.h"

#include "BRWorldMapSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

namespace
{
constexpr float FullMinX = 500.0f;
constexpr float FullMaxX = 15500.0f;
constexpr float FullMinY = -6500.0f;
constexpr float FullMaxY = 6500.0f;

float DistanceToSegmentSquared(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
{
	const FVector2D Segment = B - A;
	const float LengthSquared = Segment.SizeSquared();
	if (LengthSquared <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::DistSquared(Point, A);
	}
	const float Alpha = FMath::Clamp(FVector2D::DotProduct(Point - A, Segment) / LengthSquared, 0.0f, 1.0f);
	return FVector2D::DistSquared(Point, A + Segment * Alpha);
}
}

TSharedRef<SWidget> UBRWorldMapWidget::RebuildWidget()
{
	// Native-only UUserWidgets must finish their WidgetTree before Super builds
	// the Slate hierarchy; creating it in NativeConstruct is already too late.
	BuildMapWidget();
	return Super::RebuildWidget();
}

void UBRWorldMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>())
		{
			WorldMap->OnMapRegionUnlocked.AddUniqueDynamic(this, &UBRWorldMapWidget::HandleMapRegionUnlocked);
		}
	}

	RefreshDiscovery();
	RefreshLayout();
}

void UBRWorldMapWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBRWorldMapSubsystem* WorldMap = GameInstance->GetSubsystem<UBRWorldMapSubsystem>())
		{
			WorldMap->OnMapRegionUnlocked.RemoveDynamic(this, &UBRWorldMapWidget::HandleMapRegionUnlocked);
		}
	}
	Super::NativeDestruct();
}

void UBRWorldMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshPlayerMarker();
	if (!bFullMapMode)
	{
		RefreshLayout();
	}
}

void UBRWorldMapWidget::SetFullMapMode(bool bNewFullMapMode)
{
	if (bFullMapMode == bNewFullMapMode && MapFrame)
	{
		return;
	}
	bFullMapMode = bNewFullMapMode;
	RefreshDiscovery();
	RefreshLayout();
}

void UBRWorldMapWidget::BuildMapWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RegionRoutes.Add(TEXT("Field1"), TArray<FVector2D>{
		FVector2D(1200.0f, 0.0f), FVector2D(2050.0f, 250.0f), FVector2D(2520.0f, 1050.0f),
		FVector2D(2920.0f, 1880.0f), FVector2D(2600.0f, 2750.0f), FVector2D(3040.0f, 3650.0f),
		FVector2D(2420.0f, 4480.0f), FVector2D(1820.0f, 5200.0f), FVector2D(4300.0f, 5200.0f), FVector2D(6460.0f, 5200.0f)});
	RegionRoutes.Add(TEXT("Field2"), TArray<FVector2D>{
		FVector2D(6460.0f, 5200.0f), FVector2D(7140.0f, 4240.0f), FVector2D(7300.0f, 2750.0f),
		FVector2D(6620.0f, 1120.0f), FVector2D(5480.0f, -720.0f), FVector2D(4300.0f, -2500.0f),
		FVector2D(3000.0f, -4140.0f), FVector2D(1820.0f, -5200.0f), FVector2D(4300.0f, -5200.0f), FVector2D(6460.0f, -5200.0f)});
	RegionRoutes.Add(TEXT("Field3"), TArray<FVector2D>{
		FVector2D(6460.0f, -5200.0f), FVector2D(7800.0f, -4380.0f), FVector2D(9020.0f, -3120.0f),
		FVector2D(10100.0f, -1820.0f), FVector2D(11220.0f, -760.0f), FVector2D(12320.0f, 0.0f), FVector2D(14800.0f, 0.0f)});

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WorldMapRoot"));
	WidgetTree->RootWidget = RootCanvas;
	SetVisibility(ESlateVisibility::HitTestInvisible);

	FullScreenShade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WorldMapShade"));
	FullScreenShade->SetBrushColor(FLinearColor(0.005f, 0.008f, 0.012f, 0.86f));
	if (UCanvasPanelSlot* ShadeSlot = RootCanvas->AddChildToCanvas(FullScreenShade))
	{
		ShadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		ShadeSlot->SetOffsets(FMargin(0.0f));
	}

	MapFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WorldMapFrame"));
	MapFrame->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.045f, 0.94f));
	MapFrame->SetPadding(FMargin(2.0f));
	if (UCanvasPanelSlot* FrameSlot = RootCanvas->AddChildToCanvas(MapFrame))
	{
		FrameSlot->SetAutoSize(false);
		FrameSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		FrameSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		FrameSlot->SetPosition(FVector2D(-MiniMapScreenMargin.X, MiniMapScreenMargin.Y));
		FrameSlot->SetSize(MiniMapFrameSize);
	}

	MapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MapCanvas"));
	MapCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	MapFrame->SetContent(MapCanvas);

	MapTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapTitle"));
	MapTitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));
	MapTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.76f, 0.87f, 0.92f, 1.0f)));
	MapTitle->SetShadowOffset(FVector2D(1.0f, 1.0f));
	MapCanvas->AddChildToCanvas(MapTitle);

	ModeHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapModeHint"));
	ModeHintText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12));
	ModeHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.58f, 0.68f, 0.72f, 1.0f)));
	MapCanvas->AddChildToCanvas(ModeHintText);

	for (const TPair<FName, TArray<FVector2D>>& Route : RegionRoutes)
	{
		AddRoute(Route.Value, Route.Key);
	}

	AddRegionLabel(FVector2D(2600.0f, 2700.0f), TEXT("Field1"), TEXT("PYTHON RUINS"));
	AddRegionLabel(FVector2D(5450.0f, -350.0f), TEXT("Field2"), TEXT("PERL DESCENT"));
	AddRegionLabel(FVector2D(9800.0f, -2600.0f), TEXT("Field3"), TEXT("CMD RUNTIME"));

	AddMarker(FVector2D(1200.0f, 0.0f), TEXT("Field1"), TEXT("AWAKENING"), true);
	AddMarker(FVector2D(2360.0f, 720.0f), TEXT("Field1"), TEXT("FRAGMENT"));
	AddMarker(FVector2D(4300.0f, 5200.0f), TEXT("Field1"), TEXT("PYTHON"));
	AddMarker(FVector2D(6900.0f, 4600.0f), TEXT("Field2"), TEXT("FRAGMENT"));
	AddMarker(FVector2D(2510.0f, -4460.0f), TEXT("Field2"), TEXT("REST"), true);
	AddMarker(FVector2D(4300.0f, -5200.0f), TEXT("Field2"), TEXT("VRITRA"));
	AddMarker(FVector2D(7050.0f, -4840.0f), TEXT("Field3"), TEXT("FRAGMENT"));
	AddMarker(FVector2D(11480.0f, -280.0f), TEXT("Field3"), TEXT("REST"), true);
	AddMarker(FVector2D(14800.0f, 0.0f), TEXT("Field3"), TEXT("CMD"));

	PlayerMarker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerMapMarker"));
	PlayerMarker->SetText(FText::FromString(TEXT("▲")));
	PlayerMarker->SetJustification(ETextJustify::Center);
	PlayerMarker->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.93f, 0.62f, 1.0f)));
	PlayerMarker->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 19));
	if (UCanvasPanelSlot* PlayerSlot = MapCanvas->AddChildToCanvas(PlayerMarker))
	{
		PlayerSlot->SetSize(FVector2D(28.0f, 28.0f));
		PlayerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PlayerSlot->SetZOrder(20);
	}
}

void UBRWorldMapWidget::AddRoute(const TArray<FVector2D>& Points, FName RegionId)
{
	for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
	{
		UBorder* Line = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Line->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
		if (UCanvasPanelSlot* LineSlot = MapCanvas->AddChildToCanvas(Line))
		{
			LineSlot->SetZOrder(2);
		}
		FBRMapLineVisual& Visual = RouteLines.AddDefaulted_GetRef();
		Visual.Widget = Line;
		Visual.WorldA = Points[Index];
		Visual.WorldB = Points[Index + 1];
		Visual.RegionId = RegionId;
	}
}

void UBRWorldMapWidget::AddMarker(const FVector2D& WorldLocation, FName RegionId, const FString& Label, bool bFullMapOnly)
{
	UTextBlock* Marker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Marker->SetText(FText::FromString(FString::Printf(TEXT("◆ %s"), *Label)));
	Marker->SetJustification(ETextJustify::Center);
	Marker->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 10));
	Marker->SetColorAndOpacity(FSlateColor(GetRegionColor(RegionId)));
	if (UCanvasPanelSlot* MarkerSlot = MapCanvas->AddChildToCanvas(Marker))
	{
		MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MarkerSlot->SetSize(FVector2D(108.0f, 22.0f));
		MarkerSlot->SetZOrder(8);
	}

	FBRMapPointVisual& Visual = PointVisuals.AddDefaulted_GetRef();
	Visual.Widget = Marker;
	Visual.WorldLocation = WorldLocation;
	Visual.RegionId = RegionId;
	Visual.bFullMapOnly = bFullMapOnly;
}

void UBRWorldMapWidget::AddRegionLabel(const FVector2D& WorldLocation, FName RegionId, const FString& Label)
{
	UTextBlock* RegionLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RegionLabel->SetText(FText::FromString(Label));
	RegionLabel->SetJustification(ETextJustify::Center);
	RegionLabel->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
	if (UCanvasPanelSlot* LabelSlot = MapCanvas->AddChildToCanvas(RegionLabel))
	{
		LabelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		LabelSlot->SetSize(FVector2D(190.0f, 34.0f));
		LabelSlot->SetZOrder(5);
	}
	RegionLabels.Add(RegionId, RegionLabel);

	FBRMapPointVisual& Visual = PointVisuals.AddDefaulted_GetRef();
	Visual.Widget = RegionLabel;
	Visual.WorldLocation = WorldLocation;
	Visual.RegionId = RegionId;
	Visual.bFullMapOnly = true;
}

void UBRWorldMapWidget::RefreshDiscovery()
{
	const int32 UnlockedCount = GetGameInstance() && GetGameInstance()->GetSubsystem<UBRWorldMapSubsystem>()
		? GetGameInstance()->GetSubsystem<UBRWorldMapSubsystem>()->GetUnlockedRegionCount() : 0;

	if (MapTitle)
	{
		MapTitle->SetText(FText::FromString(bFullMapMode
			? FString::Printf(TEXT("SYSTEM MAP  //  FRAGMENTS %d / 3"), UnlockedCount)
			: FString::Printf(TEXT("MINIMAP  %d/3   [M]"), UnlockedCount)));
	}
	if (ModeHintText)
	{
		ModeHintText->SetText(FText::FromString(bFullMapMode ? TEXT("M / ESC  CLOSE     ◆ REST / BOSS / FRAGMENT") : TEXT("")));
	}

	for (FBRMapLineVisual& Visual : RouteLines)
	{
		if (Visual.Widget)
		{
			const bool bUnlocked = IsRegionUnlocked(Visual.RegionId);
			Visual.Widget->SetVisibility(bUnlocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			Visual.Widget->SetBrushColor(GetRegionColor(Visual.RegionId));
		}
	}

	for (FBRMapPointVisual& Visual : PointVisuals)
	{
		if (Visual.Widget)
		{
			const bool bShow = IsRegionUnlocked(Visual.RegionId) && (!Visual.bFullMapOnly || bFullMapMode);
			Visual.Widget->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	const TMap<FName, FString> RegionNames = {
		{TEXT("Field1"), TEXT("PYTHON RUINS")},
		{TEXT("Field2"), TEXT("PERL DESCENT")},
		{TEXT("Field3"), TEXT("CMD RUNTIME")},
	};
	for (const TPair<FName, TObjectPtr<UTextBlock>>& Entry : RegionLabels)
	{
		if (!Entry.Value)
		{
			continue;
		}
		const bool bUnlocked = IsRegionUnlocked(Entry.Key);
		Entry.Value->SetVisibility(bFullMapMode ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Entry.Value->SetText(FText::FromString(bUnlocked ? RegionNames.FindRef(Entry.Key) : TEXT("UNMAPPED // FIND FRAGMENT")));
		Entry.Value->SetColorAndOpacity(FSlateColor(bUnlocked ? GetRegionColor(Entry.Key) : FLinearColor(0.18f, 0.22f, 0.24f, 0.82f)));
	}
}

void UBRWorldMapWidget::RefreshLayout()
{
	if (!MapFrame || !MapCanvas)
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	}
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	const FVector2D ViewportSize(
		(ViewportSizeX > 0 ? static_cast<float>(ViewportSizeX) : 1920.0f) / ViewportScale,
		(ViewportSizeY > 0 ? static_cast<float>(ViewportSizeY) : 1080.0f) / ViewportScale);
	const FVector2D MiniFrameSize(
		FMath::Min(MiniMapFrameSize.X, FMath::Max(ViewportSize.X - MiniMapScreenMargin.X * 2.0f, 120.0f)),
		FMath::Min(MiniMapFrameSize.Y, FMath::Max(ViewportSize.Y - MiniMapScreenMargin.Y * 2.0f, 120.0f)));
	const FVector2D FullFrameSize(
		FMath::Min(FullMapFrameSize.X, FMath::Max(ViewportSize.X - 48.0f, 320.0f)),
		FMath::Min(FullMapFrameSize.Y, FMath::Max(ViewportSize.Y - 48.0f, 240.0f)));
	const FVector2D FrameSize = bFullMapMode ? FullFrameSize : MiniFrameSize;

	if (UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(MapFrame->Slot))
	{
		FrameSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		FrameSlot->SetAlignment(FVector2D::ZeroVector);
		FrameSlot->SetPosition(bFullMapMode
			? (ViewportSize - FullFrameSize) * 0.5f
			: FVector2D(FMath::Max(ViewportSize.X - MiniMapScreenMargin.X - MiniFrameSize.X, 0.0f), MiniMapScreenMargin.Y));
		FrameSlot->SetSize(FrameSize);
	}

	FullScreenShade->SetVisibility(bFullMapMode ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	MapFrame->SetBrushColor(bFullMapMode ? FLinearColor(0.022f, 0.030f, 0.038f, 0.985f) : FLinearColor(0.018f, 0.028f, 0.036f, 0.88f));
	MapOrigin = bFullMapMode ? FVector2D(36.0f, 70.0f) : FVector2D(18.0f, 44.0f);
	MapSize = bFullMapMode
		? FVector2D(FMath::Max(FullFrameSize.X - 72.0f, 100.0f), FMath::Max(FullFrameSize.Y - 130.0f, 100.0f))
		: FVector2D(FMath::Max(MiniFrameSize.X - 36.0f, 100.0f), FMath::Max(MiniFrameSize.Y - 76.0f, 100.0f));

	if (const APawn* Pawn = GetOwningPlayerPawn())
	{
		MiniMapWorldCenter = FVector2D(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y);
	}

	if (UCanvasPanelSlot* TitleSlot = Cast<UCanvasPanelSlot>(MapTitle->Slot))
	{
		TitleSlot->SetPosition(bFullMapMode ? FVector2D(36.0f, 22.0f) : FVector2D(18.0f, 12.0f));
		TitleSlot->SetSize(bFullMapMode ? FVector2D(520.0f, 32.0f) : FVector2D(300.0f, 26.0f));
	}
	MapTitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), bFullMapMode ? 18 : 13));
	if (UCanvasPanelSlot* HintSlot = Cast<UCanvasPanelSlot>(ModeHintText->Slot))
	{
		HintSlot->SetPosition(FVector2D(36.0f, FMath::Max(FullFrameSize.Y - 40.0f, 0.0f)));
		HintSlot->SetSize(FVector2D(FMath::Max(FullFrameSize.X - 72.0f, 100.0f), 24.0f));
	}
	ModeHintText->SetVisibility(bFullMapMode ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	for (FBRMapLineVisual& Visual : RouteLines)
	{
		if (!Visual.Widget)
		{
			continue;
		}
		const FVector2D A = WorldToMap(Visual.WorldA);
		const FVector2D B = WorldToMap(Visual.WorldB);
		const FVector2D Delta = B - A;
		if (UCanvasPanelSlot* LineSlot = Cast<UCanvasPanelSlot>(Visual.Widget->Slot))
		{
			LineSlot->SetPosition(A);
			LineSlot->SetSize(FVector2D(Delta.Size(), bFullMapMode ? 5.0f : 3.0f));
		}
		Visual.Widget->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)));
	}

	for (FBRMapPointVisual& Visual : PointVisuals)
	{
		if (Visual.Widget)
		{
			if (UCanvasPanelSlot* PointSlot = Cast<UCanvasPanelSlot>(Visual.Widget->Slot))
			{
				PointSlot->SetPosition(WorldToMap(Visual.WorldLocation));
			}
		}
	}
	RefreshPlayerMarker();
}

void UBRWorldMapWidget::RefreshPlayerMarker()
{
	const APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn || !PlayerMarker)
	{
		return;
	}

	const FVector2D PlayerWorld(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y);
	if (!bFullMapMode)
	{
		MiniMapWorldCenter = PlayerWorld;
	}
	if (UCanvasPanelSlot* PlayerSlot = Cast<UCanvasPanelSlot>(PlayerMarker->Slot))
	{
		PlayerSlot->SetPosition(WorldToMap(PlayerWorld));
	}
	PlayerMarker->SetRenderTransformAngle(90.0f - Pawn->GetActorRotation().Yaw);
}

FVector2D UBRWorldMapWidget::WorldToMap(const FVector2D& WorldLocation) const
{
	const float MiniWidth = FMath::Max(MiniMapWorldSpan.X, 100.0f);
	const float MiniHeight = FMath::Max(MiniMapWorldSpan.Y, 100.0f);
	const float MinX = bFullMapMode ? FullMinX : MiniMapWorldCenter.X - MiniWidth * 0.5f;
	const float MaxX = bFullMapMode ? FullMaxX : MiniMapWorldCenter.X + MiniWidth * 0.5f;
	const float MinY = bFullMapMode ? FullMinY : MiniMapWorldCenter.Y - MiniHeight * 0.5f;
	const float MaxY = bFullMapMode ? FullMaxY : MiniMapWorldCenter.Y + MiniHeight * 0.5f;
	const float X = MapOrigin.X + (WorldLocation.X - MinX) / (MaxX - MinX) * MapSize.X;
	const float Y = MapOrigin.Y + MapSize.Y - (WorldLocation.Y - MinY) / (MaxY - MinY) * MapSize.Y;
	return FVector2D(X, Y);
}

FName UBRWorldMapWidget::FindClosestRegion(const FVector2D& WorldLocation) const
{
	FName Closest = NAME_None;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	for (const TPair<FName, TArray<FVector2D>>& Route : RegionRoutes)
	{
		for (int32 Index = 0; Index + 1 < Route.Value.Num(); ++Index)
		{
			const float DistanceSquared = DistanceToSegmentSquared(WorldLocation, Route.Value[Index], Route.Value[Index + 1]);
			if (DistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = DistanceSquared;
				Closest = Route.Key;
			}
		}
	}
	return Closest;
}

bool UBRWorldMapWidget::IsRegionUnlocked(FName RegionId) const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UBRWorldMapSubsystem* WorldMap = GameInstance ? GameInstance->GetSubsystem<UBRWorldMapSubsystem>() : nullptr;
	return WorldMap && WorldMap->IsRegionUnlocked(RegionId);
}

FLinearColor UBRWorldMapWidget::GetRegionColor(FName RegionId) const
{
	if (RegionId == TEXT("Field1"))
	{
		return FLinearColor(0.18f, 0.56f, 0.95f, 0.92f);
	}
	if (RegionId == TEXT("Field2"))
	{
		return FLinearColor(0.96f, 0.49f, 0.15f, 0.92f);
	}
	return FLinearColor(0.94f, 0.12f, 0.24f, 0.92f);
}

void UBRWorldMapWidget::HandleMapRegionUnlocked(FName RegionId, int32 UnlockedCount)
{
	RefreshDiscovery();
	RefreshLayout();
}
