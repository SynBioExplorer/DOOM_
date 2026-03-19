#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoomThinker.generated.h"

/**
 * FDoomThinker - Base thinker struct, port of DOOM's thinker_t.
 *
 * In the original DOOM source, thinker_t is a doubly-linked list node
 * with a function pointer union (actionf_t). The actual game objects
 * (mobj_t, etc.) embed a thinker_t as their first member.
 *
 * In this UE5 port, we use a TFunction callback instead of C function
 * pointers, and manage the linked list via TDoubleLinkedList in the
 * UDoomThinkerManager.
 */
USTRUCT()
struct UNREALDOOM_API FDoomThinker
{
	GENERATED_BODY()

	/** Unique ID for this thinker, used for lookup and removal. */
	int32 ThinkerId;

	/**
	 * The think function, called once per tic.
	 * Replaces the original actionf_t function pointer union.
	 * The parameter is a pointer to this thinker's owning data (analogous
	 * to the original pattern where acp1 receives the thinker pointer).
	 */
	TFunction<void(FDoomThinker&)> ThinkFunction;

	/**
	 * If true, this thinker is marked for removal.
	 * Mirrors the original lazy deallocation pattern where
	 * thinker->function.acv is set to -1.
	 */
	bool bPendingRemoval;

	/**
	 * Opaque user data pointer. In the original DOOM, the thinker_t is the
	 * first member of larger structs (mobj_t, etc.). This pointer allows
	 * associating arbitrary data with the thinker.
	 */
	void* UserData;

	FDoomThinker()
		: ThinkerId(0)
		, bPendingRemoval(false)
		, UserData(nullptr)
	{
	}

	FDoomThinker(int32 InId, TFunction<void(FDoomThinker&)> InFunction, void* InUserData = nullptr)
		: ThinkerId(InId)
		, ThinkFunction(MoveTemp(InFunction))
		, bPendingRemoval(false)
		, UserData(InUserData)
	{
	}

	/** Mark this thinker for lazy removal (will be cleaned up during RunThinkers). */
	void MarkForRemoval()
	{
		bPendingRemoval = true;
		ThinkFunction = nullptr;
	}

	bool IsValid() const
	{
		return !bPendingRemoval && ThinkFunction;
	}
};

/**
 * UDoomThinkerManager - Manages the doubly-linked list of active thinkers.
 *
 * Port of the thinker list management from p_tick.c. In the original DOOM,
 * thinkers are stored in a circular doubly-linked list with thinkercap as
 * the sentinel node. Here we use a TArray for simplicity while preserving
 * the same iteration and lazy-removal semantics.
 */
UCLASS(BlueprintType)
class UNREALDOOM_API UDoomThinkerManager : public UObject
{
	GENERATED_BODY()

public:
	UDoomThinkerManager();

	/**
	 * Initialize the thinker list. Equivalent to P_InitThinkers().
	 * Clears all existing thinkers and resets the ID counter.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Thinker")
	void InitThinkers();

	/**
	 * Add a new thinker to the end of the list. Equivalent to P_AddThinker().
	 * @param ThinkFunction The function to call each tic.
	 * @param UserData Optional opaque data pointer.
	 * @return The ID assigned to the new thinker.
	 */
	int32 AddThinker(TFunction<void(FDoomThinker&)> ThinkFunction, void* UserData = nullptr);

	/**
	 * Mark a thinker for removal by ID. Equivalent to P_RemoveThinker().
	 * The thinker is not immediately freed; it will be cleaned up lazily
	 * during the next RunThinkers() call.
	 * @param ThinkerId The ID of the thinker to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Thinker")
	void RemoveThinker(int32 ThinkerId);

	/**
	 * Run all active thinkers. Equivalent to P_RunThinkers().
	 * Iterates the thinker list, calling each think function. Thinkers
	 * marked for removal are cleaned up during iteration.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Thinker")
	void RunThinkers();

	/** Get the number of active thinkers (excluding those pending removal). */
	UFUNCTION(BlueprintPure, Category = "Doom|Thinker")
	int32 GetActiveThinkerCount() const;

	/** Get the total number of thinkers (including those pending removal). */
	UFUNCTION(BlueprintPure, Category = "Doom|Thinker")
	int32 GetTotalThinkerCount() const { return Thinkers.Num(); }

	/** Find a thinker by ID. Returns nullptr if not found. */
	FDoomThinker* FindThinker(int32 ThinkerId);

private:
	/** The list of all thinkers. Replaces the original circular doubly-linked list. */
	TArray<FDoomThinker> Thinkers;

	/** Auto-incrementing ID counter for thinker assignment. */
	int32 NextThinkerId;
};
