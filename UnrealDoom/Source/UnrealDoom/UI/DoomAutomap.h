#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 C++.
//
// UDoomAutomapWidget - 2D top-down automap display as a UMG widget.
// Draws map lines using NativePaint() with Slate draw primitives,
// matching the original DOOM automap color scheme and behavior.
//
// Equivalent to am_map.c.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DoomAutomap.generated.h"

// Forward declarations
class UDoomLevelLoader;
struct FDoomLine;
struct FDoomVertex;
struct FDoomSector;

/**
 * UDoomAutomapWidget
 *
 * Renders the DOOM automap as a 2D overlay. Uses NativePaint() to draw
 * line segments representing the level geometry, with colors matching
 * the original DOOM automap:
 *
 * - Red: One-sided lines (solid walls)
 * - Yellow: Two-sided lines with floor height change (steps)
 * - Brown: Two-sided lines with ceiling height change
 * - Grey: Other two-sided lines (same height on both sides)
 *
 * Features:
 * - Player position/direction arrow
 * - Zoom in/out
 * - Pan offset (manual or follow-player mode)
 * - World-to-map coordinate transformation
 */
UCLASS()
class UNREALDOOM_API UDoomAutomapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDoomAutomapWidget(const FObjectInitializer& ObjectInitializer);

	// =========================================================================
	// Setup
	// =========================================================================

	/**
	 * Bind the automap to a level loader to access line/vertex/sector data.
	 *
	 * @param InLevelLoader - The level loader with the currently loaded map data
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Automap")
	void BindToLevel(UDoomLevelLoader* InLevelLoader);

	// =========================================================================
	// Visibility
	// =========================================================================

	/** Toggle automap visibility on/off */
	UFUNCTION(BlueprintCallable, Category = "Doom|Automap")
	void ToggleVisibility();

	/** Set automap visible or hidden */
	UFUNCTION(BlueprintCallable, Category = "Doom|Automap")
	void SetAutomapVisible(bool bVisible);

	/** @return Whether the automap is currently displayed */
	UFUNCTION(BlueprintPure, Category = "Doom|Automap")
	bool IsAutomapVisible() const { return bAutomapVisible; }

	// =========================================================================
	// View control
	// =========================================================================

	/** Zoom in the automap view */
	UFUNCTION(BlueprintCallable, Category = "Doom|Automap")
	void ZoomIn();

	/** Zoom out the automap view */
	UFUNCTION(BlueprintCallable, Category = "Doom|Automap")
	void ZoomOut();

	/**
	 * Pan the automap view by a delta offset.
	 * @param DeltaX - Horizontal pan amount
	 * @param DeltaY - Vertical pan amount
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Automap")
	void Pan(float DeltaX, float DeltaY);

	/** Toggle follow-player mode */
	UFUNCTION(BlueprintCallable, Category = "Doom|Automap")
	void ToggleFollowPlayer();

	// =========================================================================
	// Player tracking
	// =========================================================================

	/**
	 * Update the player position for display on the automap.
	 * Called each tick by the game mode or player controller.
	 *
	 * @param WorldX - Player X position in Unreal world units
	 * @param WorldY - Player Y position in Unreal world units
	 * @param AngleDegrees - Player facing angle in degrees
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Automap")
	void UpdatePlayerPosition(float WorldX, float WorldY, float AngleDegrees);

	// =========================================================================
	// View properties
	// =========================================================================

	/** Current zoom level (1.0 = default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Automap")
	float Zoom = 1.0f;

	/** Pan offset in world units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Automap")
	FVector2D PanOffset = FVector2D::ZeroVector;

	/** Whether the automap follows the player position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Automap")
	bool bFollowPlayer = true;

protected:
	// =========================================================================
	// UUserWidget overrides
	// =========================================================================

	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// =========================================================================
	// Drawing methods
	// =========================================================================

	/**
	 * Draw all map lines from the level data.
	 * Colors lines based on their type (one-sided, floor change, ceiling change, etc.)
	 */
	void DrawMapLines(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId) const;

	/**
	 * Draw the player position arrow on the automap.
	 */
	void DrawPlayerArrow(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId) const;

	// =========================================================================
	// Coordinate transformation
	// =========================================================================

	/**
	 * Transform a world coordinate to automap screen coordinate.
	 *
	 * @param WorldPos - Position in Unreal world units
	 * @param AllottedGeometry - The widget geometry for screen mapping
	 * @return Screen-space position within the widget
	 */
	FVector2D WorldToMap(const FVector2D& WorldPos, const FGeometry& AllottedGeometry) const;

	// =========================================================================
	// Line color determination
	// =========================================================================

	/**
	 * Get the automap color for a line based on its properties.
	 * - Red (1,0,0): one-sided (solid wall)
	 * - Yellow (1,1,0): two-sided with floor height change
	 * - Brown (0.6,0.4,0.1): two-sided with ceiling height change
	 * - Grey (0.5,0.5,0.5): other two-sided
	 *
	 * @param LineIndex - Index into the level loader's Lines array
	 * @return The color for drawing this line
	 */
	FLinearColor GetLineColor(int32 LineIndex) const;

	// =========================================================================
	// State
	// =========================================================================

	/** Reference to the level loader for map geometry data */
	UPROPERTY()
	TObjectPtr<UDoomLevelLoader> LevelLoader;

	/** Whether the automap is currently being displayed */
	bool bAutomapVisible = false;

	/** Player world position X */
	float PlayerX = 0.0f;

	/** Player world position Y */
	float PlayerY = 0.0f;

	/** Player facing angle in degrees */
	float PlayerAngle = 0.0f;

	// =========================================================================
	// Zoom constraints
	// =========================================================================

	/** Minimum zoom level */
	static constexpr float MIN_ZOOM = 0.1f;

	/** Maximum zoom level */
	static constexpr float MAX_ZOOM = 10.0f;

	/** Zoom increment per step */
	static constexpr float ZOOM_INCREMENT = 0.2f;

	/** Pan speed multiplier */
	static constexpr float PAN_SPEED = 50.0f;

	// =========================================================================
	// Automap colors (matching original DOOM)
	// =========================================================================

	/** Color for one-sided lines (solid walls) */
	static const FLinearColor COLOR_WALL;

	/** Color for floor height change lines */
	static const FLinearColor COLOR_FLOOR_CHANGE;

	/** Color for ceiling height change lines */
	static const FLinearColor COLOR_CEILING_CHANGE;

	/** Color for two-sided lines with no height change */
	static const FLinearColor COLOR_TWO_SIDED;

	/** Color for the player arrow */
	static const FLinearColor COLOR_PLAYER;

	/** Background color */
	static const FLinearColor COLOR_BACKGROUND;

	// =========================================================================
	// ML_ flag constants for line type detection
	// =========================================================================

	static constexpr int32 ML_TWOSIDED = 4;
};
