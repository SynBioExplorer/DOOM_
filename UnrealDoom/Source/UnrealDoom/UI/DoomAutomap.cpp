// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomAutomapWidget implementation - 2D automap drawing.
// Mirrors am_map.c: AM_Drawer, AM_drawWalls, AM_drawPlayers, etc.

#include "DoomAutomap.h"
#include "Renderer/DoomLevelLoader.h"
#include "Renderer/DoomMapData.h"

// =============================================================================
// Automap color constants (matching original DOOM palette colors)
// =============================================================================

const FLinearColor UDoomAutomapWidget::COLOR_WALL            = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);       // Red
const FLinearColor UDoomAutomapWidget::COLOR_FLOOR_CHANGE    = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);       // Yellow
const FLinearColor UDoomAutomapWidget::COLOR_CEILING_CHANGE  = FLinearColor(0.6f, 0.4f, 0.1f, 1.0f);       // Brown
const FLinearColor UDoomAutomapWidget::COLOR_TWO_SIDED       = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);       // Grey
const FLinearColor UDoomAutomapWidget::COLOR_PLAYER          = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);       // White
const FLinearColor UDoomAutomapWidget::COLOR_BACKGROUND      = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);      // Near-black

UDoomAutomapWidget::UDoomAutomapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDoomAutomapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}

// =============================================================================
// Setup
// =============================================================================

void UDoomAutomapWidget::BindToLevel(UDoomLevelLoader* InLevelLoader)
{
	LevelLoader = InLevelLoader;
}

// =============================================================================
// Visibility
// =============================================================================

void UDoomAutomapWidget::ToggleVisibility()
{
	SetAutomapVisible(!bAutomapVisible);
}

void UDoomAutomapWidget::SetAutomapVisible(bool bVisible)
{
	bAutomapVisible = bVisible;
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

// =============================================================================
// View control
// =============================================================================

void UDoomAutomapWidget::ZoomIn()
{
	Zoom = FMath::Clamp(Zoom + ZOOM_INCREMENT, MIN_ZOOM, MAX_ZOOM);
}

void UDoomAutomapWidget::ZoomOut()
{
	Zoom = FMath::Clamp(Zoom - ZOOM_INCREMENT, MIN_ZOOM, MAX_ZOOM);
}

void UDoomAutomapWidget::Pan(float DeltaX, float DeltaY)
{
	if (!bFollowPlayer)
	{
		PanOffset.X += DeltaX * PAN_SPEED / Zoom;
		PanOffset.Y += DeltaY * PAN_SPEED / Zoom;
	}
}

void UDoomAutomapWidget::ToggleFollowPlayer()
{
	bFollowPlayer = !bFollowPlayer;

	if (bFollowPlayer)
	{
		// Reset pan offset when re-enabling follow mode
		PanOffset = FVector2D::ZeroVector;
	}
}

// =============================================================================
// Player tracking
// =============================================================================

void UDoomAutomapWidget::UpdatePlayerPosition(float WorldX, float WorldY, float AngleDegrees)
{
	PlayerX = WorldX;
	PlayerY = WorldY;
	PlayerAngle = AngleDegrees;
}

// =============================================================================
// NativePaint - Core rendering
// =============================================================================

int32 UDoomAutomapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bAutomapVisible || !LevelLoader || !LevelLoader->IsLoaded())
	{
		return LayerId;
	}

	// Draw background overlay
	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		COLOR_BACKGROUND
	);
	LayerId++;

	// Draw map geometry lines
	DrawMapLines(AllottedGeometry, OutDrawElements, LayerId);
	LayerId++;

	// Draw player position arrow
	DrawPlayerArrow(AllottedGeometry, OutDrawElements, LayerId);
	LayerId++;

	return LayerId;
}

// =============================================================================
// DrawMapLines - Iterate all linedefs and draw based on type
// =============================================================================

void UDoomAutomapWidget::DrawMapLines(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	if (!LevelLoader)
	{
		return;
	}

	const TArray<FDoomLine>& Lines = LevelLoader->Lines;
	const TArray<FDoomVertex>& Vertexes = LevelLoader->Vertexes;

	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const FDoomLine& Line = Lines[i];

		// Validate vertex indices
		if (!Vertexes.IsValidIndex(Line.V1) || !Vertexes.IsValidIndex(Line.V2))
		{
			continue;
		}

		const FDoomVertex& V1 = Vertexes[Line.V1];
		const FDoomVertex& V2 = Vertexes[Line.V2];

		// Transform world coordinates to map screen coordinates
		const FVector2D ScreenV1 = WorldToMap(FVector2D(V1.X, V1.Y), AllottedGeometry);
		const FVector2D ScreenV2 = WorldToMap(FVector2D(V2.X, V2.Y), AllottedGeometry);

		// Determine line color based on type
		const FLinearColor LineColor = GetLineColor(i);

		// Draw the line using Slate
		TArray<FVector2D> LinePoints;
		LinePoints.Add(ScreenV1);
		LinePoints.Add(ScreenV2);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			ESlateDrawEffect::None,
			LineColor,
			true,  // bAntialias
			1.5f   // Thickness
		);
	}
}

// =============================================================================
// DrawPlayerArrow - Arrow showing player position and facing direction
// =============================================================================

void UDoomAutomapWidget::DrawPlayerArrow(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	const FVector2D PlayerScreen = WorldToMap(FVector2D(PlayerX, PlayerY), AllottedGeometry);

	// Arrow size in screen pixels
	const float ArrowSize = 12.0f;

	// Convert angle to radians (DOOM angles: 0 = east, increases counter-clockwise)
	const float AngleRad = FMath::DegreesToRadians(PlayerAngle);
	const float CosA = FMath::Cos(AngleRad);
	const float SinA = FMath::Sin(AngleRad);

	// Arrow tip (forward)
	const FVector2D Tip = PlayerScreen + FVector2D(CosA, -SinA) * ArrowSize;

	// Arrow base points (left and right of player)
	const float BaseAngle = PI * 0.75f; // 135 degrees offset for arrow wings
	const FVector2D Left = PlayerScreen + FVector2D(
		FMath::Cos(AngleRad + BaseAngle),
		-FMath::Sin(AngleRad + BaseAngle)) * ArrowSize * 0.6f;
	const FVector2D Right = PlayerScreen + FVector2D(
		FMath::Cos(AngleRad - BaseAngle),
		-FMath::Sin(AngleRad - BaseAngle)) * ArrowSize * 0.6f;

	// Draw arrow as three line segments: tip-left, tip-right, left-right
	TArray<FVector2D> ArrowLines;

	// Tip to left wing
	ArrowLines.Empty();
	ArrowLines.Add(Tip);
	ArrowLines.Add(Left);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		ArrowLines, ESlateDrawEffect::None, COLOR_PLAYER, true, 2.0f);

	// Tip to right wing
	ArrowLines.Empty();
	ArrowLines.Add(Tip);
	ArrowLines.Add(Right);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		ArrowLines, ESlateDrawEffect::None, COLOR_PLAYER, true, 2.0f);

	// Left wing to right wing (base)
	ArrowLines.Empty();
	ArrowLines.Add(Left);
	ArrowLines.Add(Right);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		ArrowLines, ESlateDrawEffect::None, COLOR_PLAYER, true, 2.0f);
}

// =============================================================================
// WorldToMap - Coordinate transformation
// =============================================================================

FVector2D UDoomAutomapWidget::WorldToMap(const FVector2D& WorldPos, const FGeometry& AllottedGeometry) const
{
	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	const FVector2D WidgetCenter = WidgetSize * 0.5f;

	// Center point: either player position (follow mode) or manual pan
	FVector2D CenterWorld;
	if (bFollowPlayer)
	{
		CenterWorld = FVector2D(PlayerX, PlayerY);
	}
	else
	{
		CenterWorld = FVector2D(PlayerX, PlayerY) + PanOffset;
	}

	// Transform: world -> relative to center -> scale by zoom -> screen center
	FVector2D Relative = WorldPos - CenterWorld;

	// Apply zoom (higher zoom = more magnified = larger on screen)
	Relative *= Zoom;

	// Map to screen coordinates
	// DOOM X -> screen X (right), DOOM Y -> screen Y (up, but screen Y is down)
	FVector2D ScreenPos;
	ScreenPos.X = WidgetCenter.X + Relative.X;
	ScreenPos.Y = WidgetCenter.Y - Relative.Y;  // Flip Y for screen coordinates

	return ScreenPos;
}

// =============================================================================
// GetLineColor - Determine automap color for a line
// =============================================================================

FLinearColor UDoomAutomapWidget::GetLineColor(int32 LineIndex) const
{
	if (!LevelLoader || !LevelLoader->Lines.IsValidIndex(LineIndex))
	{
		return COLOR_TWO_SIDED;
	}

	const FDoomLine& Line = LevelLoader->Lines[LineIndex];
	const TArray<FDoomSector>& Sectors = LevelLoader->Sectors;

	// One-sided line (solid wall) -> Red
	if (!Line.IsTwoSided())
	{
		return COLOR_WALL;
	}

	// Two-sided line: check front and back sectors for height differences
	const int32 FrontIdx = Line.FrontSector;
	const int32 BackIdx = Line.BackSector;

	if (!Sectors.IsValidIndex(FrontIdx) || !Sectors.IsValidIndex(BackIdx))
	{
		return COLOR_TWO_SIDED;
	}

	const FDoomSector& Front = Sectors[FrontIdx];
	const FDoomSector& Back = Sectors[BackIdx];

	// Floor height change -> Yellow (steps, ledges)
	if (FMath::Abs(Front.FloorHeight - Back.FloorHeight) > KINDA_SMALL_NUMBER)
	{
		return COLOR_FLOOR_CHANGE;
	}

	// Ceiling height change -> Brown (door tracks, lowering ceilings)
	if (FMath::Abs(Front.CeilingHeight - Back.CeilingHeight) > KINDA_SMALL_NUMBER)
	{
		return COLOR_CEILING_CHANGE;
	}

	// Same height on both sides -> Grey
	return COLOR_TWO_SIDED;
}
