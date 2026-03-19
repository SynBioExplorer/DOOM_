#pragma once

// Copyright (C) 1993-1996 by id Software, Inc.
// Ported to Unreal Engine 5 - ADoomEntity: base class for all DOOM map objects (mobj_t)

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealDoom/Core/DoomTypes.h"
#include "UnrealDoom/Entities/DoomEntityInfo.h"
#include "UnrealDoom/AI/DoomAI.h"
#include "DoomEntity.generated.h"

// Forward declarations
class ADoomEntity;

// =============================================================================
// Think function signature - mirrors thinker_t action pointer
// =============================================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMobjStateChanged, int32, NewStateIndex);

// =============================================================================
// ADoomEntity - Base class for all DOOM map objects (mobj_t port)
// =============================================================================
UCLASS(BlueprintType, Blueprintable)
class UNREALDOOM_API ADoomEntity : public AActor
{
	GENERATED_BODY()

public:
	ADoomEntity();

	// =========================================================================
	// Components
	// =========================================================================

	/** Collision sphere - replaces DOOM's radius/height blockmap checks */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	/** Placeholder visual mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doom|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// =========================================================================
	// Core mobj_t properties
	// =========================================================================

	/** Entity type (mobjtype_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	EDoomMobjType Type;

	/** Position in world space (replaces x, y, z fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	FVector Position;

	/** Facing angle in degrees (replaces angle_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	float Angle;

	/** mobj flags bitmask (MF_SOLID, MF_SHOOTABLE, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	int32 MobjFlags;

	/** Current health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	int32 Health;

	/** Momentum vector (replaces momx, momy, momz fixed_t) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	FVector Momentum;

	/** Collision radius in Unreal units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	float Radius;

	/** Collision height in Unreal units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	float Height;

	/** Current floor Z below entity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	float FloorZ;

	/** Current ceiling Z above entity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	float CeilingZ;

	// =========================================================================
	// Sprite / state machine (replaces state_t pointer)
	// =========================================================================

	/** Current sprite number */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|State")
	int32 Sprite;

	/** Current sprite frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|State")
	int32 Frame;

	/** Tics remaining in current state (-1 = infinite) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|State")
	int32 Tics;

	/** Index into the global states table */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|State")
	int32 CurrentStateIndex;

	/** Think function - called each tick while in current state */
	TFunction<void(ADoomEntity*)> ActionFunction;

	/** Delegate broadcast when state changes */
	UPROPERTY(BlueprintAssignable, Category = "Doom|State")
	FOnMobjStateChanged OnStateChanged;

	// =========================================================================
	// AI / movement (p_enemy.c fields)
	// =========================================================================

	/** Current movement direction (0-7, or NoDir) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|AI")
	EDoomDirection MoveDir;

	/** When 0, select a new direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|AI")
	int32 MoveCount;

	/** Tics before first attack after seeing player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|AI")
	int32 ReactionTime;

	/** Damage threshold before switching targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|AI")
	int32 Threshold;

	/** Current target entity (replaces mobj_t* target) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|AI")
	TObjectPtr<ADoomEntity> Target;

	/** Last entity that attacked this one (replaces mobj_t* tracer for homing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|AI")
	TObjectPtr<ADoomEntity> Tracer;

	/** Last entity that was looked up (replaces lastenemy / lastlook) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|AI")
	int32 LastLook;

	// =========================================================================
	// Sector / subsector tracking
	// =========================================================================

	/** Current sector index in the BSP map */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SectorIndex;

	/** Current subsector index in the BSP map */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Map")
	int32 SubsectorIndex;

	// =========================================================================
	// Linked list pointers (for sector thing lists)
	// =========================================================================

	/** Next entity in same sector */
	UPROPERTY(BlueprintReadWrite, Category = "Doom|Map")
	TObjectPtr<ADoomEntity> SectorNext;

	/** Previous entity in same sector */
	UPROPERTY(BlueprintReadWrite, Category = "Doom|Map")
	TObjectPtr<ADoomEntity> SectorPrev;

	/** Next entity in same blockmap cell */
	UPROPERTY(BlueprintReadWrite, Category = "Doom|Map")
	TObjectPtr<ADoomEntity> BlockNext;

	/** Previous entity in same blockmap cell */
	UPROPERTY(BlueprintReadWrite, Category = "Doom|Map")
	TObjectPtr<ADoomEntity> BlockPrev;

	// =========================================================================
	// Additional mobj_t fields
	// =========================================================================

	/** Mass of entity (for thrust calculations) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	int32 Mass;

	/** Damage value for missiles */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	int32 Damage;

	/** Speed from mobjinfo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	int32 Speed;

	/** Player number that spawned this, or -1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Entity")
	int32 PlayerNumber;

	// =========================================================================
	// Factory methods
	// =========================================================================

	/**
	 * Spawn a new map object. Port of P_SpawnMobj.
	 * @param World       The world to spawn in
	 * @param Location    Spawn location in Unreal coordinates
	 * @param MobjType    The type of entity to spawn
	 * @return            The spawned entity, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Entity", meta = (WorldContext = "WorldContextObject"))
	static ADoomEntity* SpawnMobj(UObject* WorldContextObject, FVector Location, EDoomMobjType MobjType);

	/**
	 * Remove this map object from the world. Port of P_RemoveMobj.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Entity")
	void RemoveMobj();

	// =========================================================================
	// State machine
	// =========================================================================

	/**
	 * Transition to a new state. Port of P_SetMobjState.
	 * @param StateIndex  Index into the global state table
	 * @return            True if entity is still alive after state change
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Entity")
	bool SetState(int32 StateIndex);

	// =========================================================================
	// AActor overrides
	// =========================================================================

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// =========================================================================
	// P_MobjThinker internals
	// =========================================================================

	/**
	 * Main thinker function. Port of P_MobjThinker.
	 * Called each tick: advances state machine, applies momentum,
	 * gravity, friction, and floor/ceiling clipping.
	 */
	void MobjThinker(float DeltaTime);

	/** Apply horizontal momentum with friction. Port of P_XYMovement. */
	void XYMovement();

	/** Apply vertical momentum and gravity. Port of P_ZMovement. */
	void ZMovement();

	/** Clamp position to floor and ceiling. */
	void ClipToFloorCeiling();

	/** Apply friction to horizontal momentum. */
	void ApplyFriction();

	/** Apply gravity (if not MF_NOGRAVITY). */
	void ApplyGravity();

	/** Accumulator for sub-tick timing (DOOM runs at 35 Hz) */
	float TicAccumulator;

	/** Duration of one DOOM tic in seconds */
	static constexpr float DOOM_TIC_DURATION = 1.0f / 35.0f;

	/** Gravity constant (DOOM fixed-point GRAVITY = FRACUNIT) */
	static constexpr float DOOM_GRAVITY = 1.0f;

	/** Ground friction factor (DOOM: 0xe800 / 0x10000 ~ 0.90625) */
	static constexpr float DOOM_FRICTION = 0.90625f;
};
