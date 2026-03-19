#pragma once

#include "CoreMinimal.h"
#include "DoomTicCmd.generated.h"

/**
 * FDoomTicCmd - The data sampled per tick and transmitted to peers.
 *
 * Direct port of DOOM's ticcmd_t struct. Contains movement commands,
 * button states, and a consistency check for network games.
 * This is the fundamental unit of player input in the DOOM engine.
 *
 * Original struct (d_ticcmd.h):
 *   char    forwardmove;   // *2048 for move
 *   char    sidemove;      // *2048 for move
 *   short   angleturn;     // <<16 for angle delta
 *   short   consistancy;   // checks for net game
 *   byte    chatchar;
 *   byte    buttons;
 */
USTRUCT(BlueprintType)
struct UNREALDOOM_API FDoomTicCmd
{
	GENERATED_BODY()

	/** Forward/backward movement. Positive = forward. Multiplied by 2048 for actual movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|TicCmd")
	int32 ForwardMove;

	/** Left/right strafing. Positive = right. Multiplied by 2048 for actual movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|TicCmd")
	int32 SideMove;

	/** Turning angle delta. Left-shifted 16 for angle delta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|TicCmd")
	int32 AngleTurn;

	/** Consistency check value for network games. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|TicCmd")
	int32 Consistancy;

	/** Chat character to send. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|TicCmd")
	uint8 ChatChar;

	/** Button bitfield (BT_ATTACK, BT_USE, BT_CHANGE, BT_SPECIAL, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|TicCmd")
	uint8 Buttons;

	FDoomTicCmd()
		: ForwardMove(0)
		, SideMove(0)
		, AngleTurn(0)
		, Consistancy(0)
		, ChatChar(0)
		, Buttons(0)
	{
	}

	/** Clear all fields to zero. */
	void Clear()
	{
		ForwardMove = 0;
		SideMove = 0;
		AngleTurn = 0;
		Consistancy = 0;
		ChatChar = 0;
		Buttons = 0;
	}

	/**
	 * Pack this command into the compact wire format matching the original ticcmd_t.
	 * Returns 8 bytes: forwardmove(i8), sidemove(i8), angleturn(i16), consistancy(i16), chatchar(u8), buttons(u8).
	 */
	TArray<uint8> Pack() const
	{
		TArray<uint8> Data;
		Data.SetNum(8);
		Data[0] = static_cast<uint8>(static_cast<int8>(FMath::Clamp(ForwardMove, -128, 127)));
		Data[1] = static_cast<uint8>(static_cast<int8>(FMath::Clamp(SideMove, -128, 127)));
		const int16 Turn = static_cast<int16>(FMath::Clamp(AngleTurn, -32768, 32767));
		Data[2] = static_cast<uint8>(Turn & 0xFF);
		Data[3] = static_cast<uint8>((Turn >> 8) & 0xFF);
		const int16 Consist = static_cast<int16>(FMath::Clamp(Consistancy, -32768, 32767));
		Data[4] = static_cast<uint8>(Consist & 0xFF);
		Data[5] = static_cast<uint8>((Consist >> 8) & 0xFF);
		Data[6] = ChatChar;
		Data[7] = Buttons;
		return Data;
	}

	/**
	 * Unpack from the compact wire format.
	 */
	void Unpack(const TArray<uint8>& Data)
	{
		if (Data.Num() < 8)
		{
			Clear();
			return;
		}
		ForwardMove = static_cast<int8>(Data[0]);
		SideMove = static_cast<int8>(Data[1]);
		AngleTurn = static_cast<int16>(Data[2] | (Data[3] << 8));
		Consistancy = static_cast<int16>(Data[4] | (Data[5] << 8));
		ChatChar = Data[6];
		Buttons = Data[7];
	}
};

/**
 * Button/action code definitions matching the original DOOM BT_ constants.
 */
namespace DoomButtons
{
	/** Press "Fire". */
	static constexpr uint8 BT_ATTACK    = 1;
	/** Use button, to open doors, activate switches. */
	static constexpr uint8 BT_USE       = 2;
	/** Flag: weapon change pending. If true, the next 3 bits hold weapon number. */
	static constexpr uint8 BT_CHANGE    = 4;
	/** The 3-bit weapon mask. */
	static constexpr uint8 BT_WEAPONMASK = (8 + 16 + 32);
	/** Weapon shift amount. */
	static constexpr uint8 BT_WEAPONSHIFT = 3;

	/** Special button flag. */
	static constexpr uint8 BT_SPECIAL   = 128;
	static constexpr uint8 BT_SPECIALMASK = 3;

	/** Pause the game. */
	static constexpr uint8 BTS_PAUSE    = 1;
	/** Save the game at a specific slot. */
	static constexpr uint8 BTS_SAVEGAME = 2;

	static constexpr uint8 BTS_SAVEMASK  = (4 + 8 + 16);
	static constexpr uint8 BTS_SAVESHIFT = 2;
}
