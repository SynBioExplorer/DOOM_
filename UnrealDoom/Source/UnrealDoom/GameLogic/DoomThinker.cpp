#include "DoomThinker.h"

UDoomThinkerManager::UDoomThinkerManager()
	: NextThinkerId(1)
{
}

void UDoomThinkerManager::InitThinkers()
{
	// Equivalent to P_InitThinkers():
	//   thinkercap.prev = thinkercap.next = &thinkercap;
	// We simply clear the list and reset IDs.
	Thinkers.Empty();
	NextThinkerId = 1;
}

int32 UDoomThinkerManager::AddThinker(TFunction<void(FDoomThinker&)> ThinkFunction, void* UserData)
{
	// Equivalent to P_AddThinker():
	//   Adds a new thinker at the end of the list.
	const int32 Id = NextThinkerId++;
	Thinkers.Emplace(Id, MoveTemp(ThinkFunction), UserData);
	return Id;
}

void UDoomThinkerManager::RemoveThinker(int32 ThinkerId)
{
	// Equivalent to P_RemoveThinker():
	//   thinker->function.acv = (actionf_v)(-1);
	// Lazy removal - just mark it, actual cleanup happens in RunThinkers.
	for (FDoomThinker& Thinker : Thinkers)
	{
		if (Thinker.ThinkerId == ThinkerId)
		{
			Thinker.MarkForRemoval();
			return;
		}
	}
}

void UDoomThinkerManager::RunThinkers()
{
	// Equivalent to P_RunThinkers():
	//   Iterate the list. Remove thinkers marked with -1, call think
	//   functions for the rest.
	//
	// We iterate by index because removal during iteration can invalidate
	// iterators. We process in forward order to match the original behavior.
	for (int32 i = 0; i < Thinkers.Num(); /* increment handled below */)
	{
		FDoomThinker& Current = Thinkers[i];

		if (Current.bPendingRemoval)
		{
			// Time to remove it - equivalent to:
			//   currentthinker->next->prev = currentthinker->prev;
			//   currentthinker->prev->next = currentthinker->next;
			//   Z_Free(currentthinker);
			Thinkers.RemoveAt(i);
			// Do not increment i; the next element shifted into this slot.
		}
		else
		{
			// Call the think function if it's valid.
			// Equivalent to: if (currentthinker->function.acp1)
			//                     currentthinker->function.acp1(currentthinker);
			if (Current.ThinkFunction)
			{
				Current.ThinkFunction(Current);
			}
			++i;
		}
	}
}

int32 UDoomThinkerManager::GetActiveThinkerCount() const
{
	int32 Count = 0;
	for (const FDoomThinker& Thinker : Thinkers)
	{
		if (!Thinker.bPendingRemoval)
		{
			Count++;
		}
	}
	return Count;
}

FDoomThinker* UDoomThinkerManager::FindThinker(int32 ThinkerId)
{
	for (FDoomThinker& Thinker : Thinkers)
	{
		if (Thinker.ThinkerId == ThinkerId && !Thinker.bPendingRemoval)
		{
			return &Thinker;
		}
	}
	return nullptr;
}
