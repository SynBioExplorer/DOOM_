#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "DoomSoundDefs.h"
#include "DoomAudioSystem.generated.h"

class AActor;

// =============================================================================
// FDoomSoundChannel - Represents one active sound channel
// Mirrors channel_t from s_sound.c
// =============================================================================

USTRUCT()
struct FDoomSoundChannel
{
	GENERATED_BODY()

	// The sound effect playing on this channel (None = channel available)
	UPROPERTY()
	EDoomSfx SfxId = EDoomSfx::None;

	// Origin actor of the sound (nullptr for UI/global sounds)
	UPROPERTY()
	TWeakObjectPtr<AActor> Origin;

	// UE5 audio component handling playback
	UPROPERTY()
	TWeakObjectPtr<UAudioComponent> AudioComponent;

	// Priority of the sound on this channel
	int32 Priority = 0;

	// Whether this channel is actively playing
	bool IsPlaying() const;

	// Stop and clear this channel
	void Stop();

	// Clear channel state
	void Clear();
};

// =============================================================================
// UDoomAudioSystem - Main audio system, port of s_sound.c
//
// Manages all sound effects and music playback using UE5 audio components.
// Implements DOOM's distance-based attenuation, stereo panning, pitch
// variation, and channel priority system.
// =============================================================================

UCLASS(BlueprintType, Blueprintable)
class UNREALDOOM_API UDoomAudioSystem : public UObject
{
	GENERATED_BODY()

public:
	UDoomAudioSystem();

	// =========================================================================
	// Initialization - mirrors S_Init()
	// =========================================================================

	/** Initialize the sound system with volume settings. Call once at game start. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void Init(int32 SfxVolume = 15, int32 MusicVolume = 15);

	/** Per-level startup. Kills all playing sounds, starts level music. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void Start(int32 GameEpisode, int32 GameMap, bool bCommercial);

	// =========================================================================
	// Sound Effects - mirrors S_StartSound(), S_StopSound()
	// =========================================================================

	/**
	 * Play a positional sound effect from an origin actor.
	 * If Origin is nullptr, plays as a UI/global sound (no attenuation).
	 * Mirrors S_StartSound() from s_sound.c.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void StartSound(AActor* Origin, EDoomSfx SfxId);

	/**
	 * Play a sound at a specific volume (0-15 range, matching original DOOM).
	 * Mirrors S_StartSoundAtVolume() from s_sound.c.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void StartSoundAtVolume(AActor* Origin, EDoomSfx SfxId, int32 Volume);

	/** Stop all sounds originating from the given actor. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void StopSound(AActor* Origin);

	/** Stop all currently playing sounds. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void StopAllSounds();

	// =========================================================================
	// Music - mirrors S_StartMusic(), S_ChangeMusic(), S_StopMusic()
	// =========================================================================

	/** Start a music track (non-looping). Mirrors S_StartMusic(). */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void StartMusic(EDoomMusic MusicId);

	/**
	 * Change the current music track with optional looping.
	 * Mirrors S_ChangeMusic() from s_sound.c.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void ChangeMusic(EDoomMusic MusicId, bool bLooping = true);

	/** Stop the currently playing music track. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void StopMusic();

	/** Pause music playback (for game pause). */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void PauseMusic();

	/** Resume music playback after pause. */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void ResumeMusic();

	// =========================================================================
	// Volume Control - mirrors S_SetSfxVolume(), S_SetMusicVolume()
	// =========================================================================

	/** Set SFX volume (0-15 range). Mirrors S_SetSfxVolume(). */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void SetSfxVolume(int32 Volume);

	/** Set music volume (0-15 range). Mirrors S_SetMusicVolume(). */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void SetMusicVolume(int32 Volume);

	UFUNCTION(BlueprintPure, Category = "Doom|Audio")
	int32 GetSfxVolume() const { return SfxVolume; }

	UFUNCTION(BlueprintPure, Category = "Doom|Audio")
	int32 GetMusicVolume() const { return MusicVolume; }

	// =========================================================================
	// Update - mirrors S_UpdateSounds()
	// =========================================================================

	/**
	 * Update 3D sound positions and attenuation relative to the listener.
	 * Should be called every tick. Mirrors S_UpdateSounds() from s_sound.c.
	 */
	UFUNCTION(BlueprintCallable, Category = "Doom|Audio")
	void UpdateSounds(AActor* Listener);

	// =========================================================================
	// Configuration
	// =========================================================================

	/** SFX data table: map EDoomSfx to USoundBase assets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Audio|Config")
	TMap<EDoomSfx, TSoftObjectPtr<USoundBase>> SfxAssets;

	/** Music data table: map EDoomMusic to USoundBase assets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Audio|Config")
	TMap<EDoomMusic, TSoftObjectPtr<USoundBase>> MusicAssets;

	/** Maximum number of simultaneous sound channels (default 8, original had variable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Audio|Config", meta = (ClampMin = "1", ClampMax = "32"))
	int32 MaxChannels = 8;

	/**
	 * Clipping distance in Unreal units. Sounds beyond this are inaudible.
	 * Original DOOM: 1200 map units. Default scaled for UE5 (1 DOOM unit ~ 1 UU).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Audio|Config")
	float ClippingDistance = 1200.0f;

	/**
	 * Close distance in Unreal units. Sounds within this range play at max volume.
	 * Original DOOM: 160 map units.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Audio|Config")
	float CloseDistance = 160.0f;

	/** Stereo separation swing factor (controls L/R panning strength) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doom|Audio|Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StereoSwing = 0.75f;

protected:
	// =========================================================================
	// Internal state
	// =========================================================================

	/** Active sound channels */
	UPROPERTY()
	TArray<FDoomSoundChannel> Channels;

	/** Current SFX volume (0-15) */
	UPROPERTY()
	int32 SfxVolume = 15;

	/** Current music volume (0-15) */
	UPROPERTY()
	int32 MusicVolume = 15;

	/** Music audio component */
	UPROPERTY()
	TWeakObjectPtr<UAudioComponent> MusicComponent;

	/** Currently playing music ID */
	UPROPERTY()
	EDoomMusic CurrentMusic = EDoomMusic::None;

	/** Whether music is paused */
	UPROPERTY()
	bool bMusicPaused = false;

	// =========================================================================
	// Internal helpers - mirrors static functions from s_sound.c
	// =========================================================================

	/**
	 * Adjust sound parameters based on distance and angle from listener.
	 * Mirrors S_AdjustSoundParams() from s_sound.c.
	 * @return true if sound is audible, false if it should be clipped.
	 */
	bool AdjustSoundParams(
		AActor* Listener,
		AActor* Source,
		float& OutVolume,
		float& OutPan,
		float& OutPitch) const;

	/**
	 * Find a free channel, or steal one from a lower-priority sound.
	 * Mirrors S_getChannel() from s_sound.c.
	 * @return Channel index, or -1 if no channel available.
	 */
	int32 GetChannel(AActor* Origin, EDoomSfx SfxId, int32 Priority);

	/** Stop a specific channel. Mirrors S_StopChannel(). */
	void StopChannel(int32 ChannelIndex);

	/** Get the priority value for a sound effect */
	int32 GetSfxPriority(EDoomSfx SfxId) const;

	/** Apply DOOM-style random pitch variation to a sound */
	float GetRandomPitch(EDoomSfx SfxId) const;

	/** Resolve a soft object pointer and get the loaded sound */
	USoundBase* ResolveSfxAsset(EDoomSfx SfxId) const;
	USoundBase* ResolveMusicAsset(EDoomMusic MusicId) const;

	/** Create an audio component on an actor (or the world) for playback */
	UAudioComponent* CreateAudioComponent(AActor* AttachTo, USoundBase* Sound) const;
};
