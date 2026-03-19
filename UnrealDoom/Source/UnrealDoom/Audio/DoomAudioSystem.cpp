// =============================================================================
// DoomAudioSystem.cpp
//
// Port of DOOM's s_sound.c to Unreal Engine 5.
// Implements channel-based sound effect playback with DOOM-style distance
// attenuation, stereo panning, pitch variation, and channel priority stealing.
// =============================================================================

#include "DoomAudioSystem.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

// DOOM volume range
static constexpr int32 DOOM_MAX_VOLUME = 15;
static constexpr float INV_DOOM_MAX_VOLUME = 1.0f / static_cast<float>(DOOM_MAX_VOLUME);

// =============================================================================
// FDoomSoundChannel
// =============================================================================

bool FDoomSoundChannel::IsPlaying() const
{
	return SfxId != EDoomSfx::None
		&& AudioComponent.IsValid()
		&& AudioComponent->IsPlaying();
}

void FDoomSoundChannel::Stop()
{
	if (AudioComponent.IsValid())
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
	}
	Clear();
}

void FDoomSoundChannel::Clear()
{
	SfxId = EDoomSfx::None;
	Origin = nullptr;
	AudioComponent = nullptr;
	Priority = 0;
}

// =============================================================================
// UDoomAudioSystem - Constructor
// =============================================================================

UDoomAudioSystem::UDoomAudioSystem()
{
	Channels.SetNum(MaxChannels);
}

// =============================================================================
// Init - mirrors S_Init()
// =============================================================================

void UDoomAudioSystem::Init(int32 InSfxVolume, int32 InMusicVolume)
{
	SetSfxVolume(InSfxVolume);
	SetMusicVolume(InMusicVolume);

	// Ensure the channel array matches MaxChannels
	Channels.SetNum(MaxChannels);
	for (FDoomSoundChannel& Channel : Channels)
	{
		Channel.Clear();
	}

	CurrentMusic = EDoomMusic::None;
	bMusicPaused = false;

	UE_LOG(LogTemp, Log, TEXT("DoomAudioSystem: Initialized with %d channels, SFX vol=%d, Music vol=%d"),
		MaxChannels, SfxVolume, MusicVolume);
}

// =============================================================================
// Start - per-level startup, mirrors S_Start()
// =============================================================================

void UDoomAudioSystem::Start(int32 GameEpisode, int32 GameMap, bool bCommercial)
{
	// Stop everything from the previous level
	StopAllSounds();
	StopMusic();

	// Calculate music track from episode/map
	EDoomMusic MusicId = EDoomMusic::None;

	if (bCommercial)
	{
		// DOOM 2: MAP01..MAP32 maps to Runnin (first DOOM2 track) + (map - 1)
		// EDoomMusic::Runnin is the first DOOM 2 music entry
		const int32 Doom2MusicBase = static_cast<int32>(EDoomMusic::Runnin);
		const int32 MapIndex = FMath::Clamp(GameMap - 1, 0, 31);
		MusicId = static_cast<EDoomMusic>(Doom2MusicBase + MapIndex);
	}
	else
	{
		// DOOM 1: E1M1..E3M9
		// Each episode has 9 maps. E1M1 is the first entry.
		const int32 EpisodeBase = static_cast<int32>(EDoomMusic::E1M1);
		const int32 Episode = FMath::Clamp(GameEpisode - 1, 0, 2);
		const int32 Map = FMath::Clamp(GameMap - 1, 0, 8);
		MusicId = static_cast<EDoomMusic>(EpisodeBase + Episode * 9 + Map);
	}

	// Start the level music (looping)
	if (MusicId != EDoomMusic::None)
	{
		ChangeMusic(MusicId, true);
	}

	UE_LOG(LogTemp, Log, TEXT("DoomAudioSystem: Level started (Episode=%d, Map=%d, Commercial=%d)"),
		GameEpisode, GameMap, bCommercial ? 1 : 0);
}

// =============================================================================
// StartSound - mirrors S_StartSound()
// =============================================================================

void UDoomAudioSystem::StartSound(AActor* Origin, EDoomSfx SfxId)
{
	StartSoundAtVolume(Origin, SfxId, DOOM_MAX_VOLUME);
}

// =============================================================================
// StartSoundAtVolume - mirrors S_StartSoundAtVolume()
// =============================================================================

void UDoomAudioSystem::StartSoundAtVolume(AActor* Origin, EDoomSfx SfxId, int32 Volume)
{
	if (SfxId == EDoomSfx::None || SfxId >= EDoomSfx::NumSfx)
	{
		return;
	}

	// Clamp volume to DOOM range
	Volume = FMath::Clamp(Volume, 0, DOOM_MAX_VOLUME);

	// Calculate priority
	const int32 Priority = GetSfxPriority(SfxId);

	// Find a channel (may steal from lower-priority sounds)
	const int32 ChannelIndex = GetChannel(Origin, SfxId, Priority);
	if (ChannelIndex < 0)
	{
		// No channel available
		return;
	}

	// Resolve the sound asset
	USoundBase* Sound = ResolveSfxAsset(SfxId);
	if (!Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("DoomAudioSystem: No sound asset for SFX %d"), static_cast<int32>(SfxId));
		return;
	}

	// Create audio component
	UAudioComponent* AudioComp = CreateAudioComponent(Origin, Sound);
	if (!AudioComp)
	{
		return;
	}

	// Set volume: combine the requested volume with the global SfxVolume
	const float NormalizedRequestVolume = static_cast<float>(Volume) * INV_DOOM_MAX_VOLUME;
	const float NormalizedGlobalVolume = static_cast<float>(SfxVolume) * INV_DOOM_MAX_VOLUME;
	AudioComp->SetVolumeMultiplier(NormalizedRequestVolume * NormalizedGlobalVolume);

	// Set DOOM-style random pitch variation
	AudioComp->SetPitchMultiplier(GetRandomPitch(SfxId));

	// Play the sound
	AudioComp->Play();

	// Fill the channel
	FDoomSoundChannel& Channel = Channels[ChannelIndex];
	Channel.SfxId = SfxId;
	Channel.Origin = Origin;
	Channel.AudioComponent = AudioComp;
	Channel.Priority = Priority;
}

// =============================================================================
// StopSound - stop all sounds from a given origin, mirrors S_StopSound()
// =============================================================================

void UDoomAudioSystem::StopSound(AActor* Origin)
{
	if (!Origin)
	{
		return;
	}

	for (int32 i = 0; i < Channels.Num(); ++i)
	{
		if (Channels[i].Origin.Get() == Origin)
		{
			StopChannel(i);
		}
	}
}

// =============================================================================
// StopAllSounds - mirrors part of S_Start() / S_StopSound() for all
// =============================================================================

void UDoomAudioSystem::StopAllSounds()
{
	for (int32 i = 0; i < Channels.Num(); ++i)
	{
		if (Channels[i].SfxId != EDoomSfx::None)
		{
			StopChannel(i);
		}
	}
}

// =============================================================================
// Music management
// =============================================================================

void UDoomAudioSystem::StartMusic(EDoomMusic MusicId)
{
	ChangeMusic(MusicId, false);
}

void UDoomAudioSystem::ChangeMusic(EDoomMusic MusicId, bool bLooping)
{
	if (MusicId == EDoomMusic::None || MusicId >= EDoomMusic::NumMusic)
	{
		return;
	}

	// If the same music is already playing, do nothing
	if (MusicId == CurrentMusic && MusicComponent.IsValid() && MusicComponent->IsPlaying())
	{
		return;
	}

	// Stop current music
	StopMusic();

	// Resolve the music asset
	USoundBase* MusicSound = ResolveMusicAsset(MusicId);
	if (!MusicSound)
	{
		UE_LOG(LogTemp, Warning, TEXT("DoomAudioSystem: No music asset for Music %d"), static_cast<int32>(MusicId));
		return;
	}

	// Create music audio component (not attached to any actor)
	UAudioComponent* MusicComp = CreateAudioComponent(nullptr, MusicSound);
	if (!MusicComp)
	{
		return;
	}

	// Configure music playback
	const float NormalizedVolume = static_cast<float>(MusicVolume) * INV_DOOM_MAX_VOLUME;
	MusicComp->SetVolumeMultiplier(NormalizedVolume);
	MusicComp->bIsMusic = true;

	// Set looping - UE5 AudioComponent does not have a simple loop toggle;
	// the source sound asset should be set to loop. We store the intent.
	// For runtime control, we rely on the sound asset's looping setting or
	// we can use bAutoDestroy = false and re-trigger.
	MusicComp->bAutoDestroy = false;

	MusicComp->Play();

	MusicComponent = MusicComp;
	CurrentMusic = MusicId;
	bMusicPaused = false;

	UE_LOG(LogTemp, Log, TEXT("DoomAudioSystem: Playing music %s (looping=%d)"),
		*DoomSoundUtils::GetMusicLumpName(MusicId), bLooping ? 1 : 0);
}

void UDoomAudioSystem::StopMusic()
{
	if (MusicComponent.IsValid())
	{
		MusicComponent->Stop();
		MusicComponent->DestroyComponent();
		MusicComponent = nullptr;
	}
	CurrentMusic = EDoomMusic::None;
	bMusicPaused = false;
}

void UDoomAudioSystem::PauseMusic()
{
	if (MusicComponent.IsValid() && MusicComponent->IsPlaying())
	{
		MusicComponent->SetPaused(true);
		bMusicPaused = true;
	}
}

void UDoomAudioSystem::ResumeMusic()
{
	if (MusicComponent.IsValid() && bMusicPaused)
	{
		MusicComponent->SetPaused(false);
		bMusicPaused = false;
	}
}

// =============================================================================
// Volume Control
// =============================================================================

void UDoomAudioSystem::SetSfxVolume(int32 Volume)
{
	SfxVolume = FMath::Clamp(Volume, 0, DOOM_MAX_VOLUME);

	// Update all currently playing sound channels
	const float NormalizedVolume = static_cast<float>(SfxVolume) * INV_DOOM_MAX_VOLUME;
	for (FDoomSoundChannel& Channel : Channels)
	{
		if (Channel.IsPlaying())
		{
			Channel.AudioComponent->SetVolumeMultiplier(NormalizedVolume);
		}
	}
}

void UDoomAudioSystem::SetMusicVolume(int32 Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0, DOOM_MAX_VOLUME);

	// Update music component if playing
	if (MusicComponent.IsValid())
	{
		const float NormalizedVolume = static_cast<float>(MusicVolume) * INV_DOOM_MAX_VOLUME;
		MusicComponent->SetVolumeMultiplier(NormalizedVolume);
	}
}

// =============================================================================
// UpdateSounds - mirrors S_UpdateSounds()
// =============================================================================

void UDoomAudioSystem::UpdateSounds(AActor* Listener)
{
	for (int32 i = 0; i < Channels.Num(); ++i)
	{
		FDoomSoundChannel& Channel = Channels[i];

		if (Channel.SfxId == EDoomSfx::None)
		{
			continue;
		}

		// Check if the audio component has finished playing naturally
		if (!Channel.IsPlaying())
		{
			StopChannel(i);
			continue;
		}

		// If the origin is invalid (destroyed actor), stop the channel
		AActor* Origin = Channel.Origin.Get();
		if (Channel.Origin.IsStale())
		{
			StopChannel(i);
			continue;
		}

		// For non-positional sounds (Origin == nullptr), no attenuation needed
		if (!Origin)
		{
			continue;
		}

		// Adjust sound parameters based on listener position
		float AdjustedVolume = 0.0f;
		float AdjustedPan = 0.0f;
		float AdjustedPitch = 1.0f;

		if (AdjustSoundParams(Listener, Origin, AdjustedVolume, AdjustedPan, AdjustedPitch))
		{
			// Sound is still in range: update volume and panning
			if (Channel.AudioComponent.IsValid())
			{
				// Volume = adjusted volume * global SFX volume
				const float NormalizedGlobalVolume = static_cast<float>(SfxVolume) * INV_DOOM_MAX_VOLUME;
				Channel.AudioComponent->SetVolumeMultiplier(AdjustedVolume * NormalizedGlobalVolume);

				// UE5 does not have a direct "pan" on AudioComponent for 3D sounds.
				// The positional audio handles this automatically when the component
				// is attached to the source actor. For 2D sounds we could manipulate
				// a sound concurrency/submix, but we keep it simple here.
			}
		}
		else
		{
			// Sound is out of range: stop it
			StopChannel(i);
		}
	}
}

// =============================================================================
// AdjustSoundParams - mirrors S_AdjustSoundParams()
//
// DOOM distance attenuation model:
//   - Beyond ClippingDistance: inaudible (return false)
//   - Within CloseDistance: full volume
//   - Between: linear falloff
// Pan is based on the angle between listener forward and direction to source.
// =============================================================================

bool UDoomAudioSystem::AdjustSoundParams(
	AActor* Listener,
	AActor* Source,
	float& OutVolume,
	float& OutPan,
	float& OutPitch) const
{
	if (!Listener || !Source)
	{
		// No positional data: play at full volume, center pan
		OutVolume = 1.0f;
		OutPan = 0.0f;
		OutPitch = 1.0f;
		return true;
	}

	const FVector ListenerLocation = Listener->GetActorLocation();
	const FVector SourceLocation = Source->GetActorLocation();

	// Calculate distance (2D, matching original DOOM which is top-down)
	const float Dx = SourceLocation.X - ListenerLocation.X;
	const float Dy = SourceLocation.Y - ListenerLocation.Y;
	const float Distance = FMath::Sqrt(Dx * Dx + Dy * Dy);

	// Beyond clipping distance: inaudible
	if (Distance > ClippingDistance)
	{
		return false;
	}

	// Volume: full within CloseDistance, linear falloff to ClippingDistance
	if (Distance <= CloseDistance)
	{
		OutVolume = 1.0f;
	}
	else
	{
		// Linear attenuation between CloseDistance and ClippingDistance
		const float Range = ClippingDistance - CloseDistance;
		if (Range > SMALL_NUMBER)
		{
			OutVolume = 1.0f - ((Distance - CloseDistance) / Range);
			OutVolume = FMath::Clamp(OutVolume, 0.0f, 1.0f);
		}
		else
		{
			OutVolume = 1.0f;
		}
	}

	// Pan: based on angle difference from listener's forward direction
	// DOOM calculates: angle from listener to source, subtract listener angle,
	// then map to stereo separation.
	const FVector ListenerForward = Listener->GetActorForwardVector();
	const FVector ToSource = (SourceLocation - ListenerLocation).GetSafeNormal2D();

	if (!ToSource.IsNearlyZero())
	{
		// Cross product Y component gives sine of angle (left/right)
		const float Cross = FVector::CrossProduct(ListenerForward, ToSource).Z;
		// Pan ranges from -1 (full left) to +1 (full right)
		OutPan = FMath::Clamp(Cross * StereoSwing, -1.0f, 1.0f);
	}
	else
	{
		OutPan = 0.0f;
	}

	OutPitch = 1.0f;

	return true;
}

// =============================================================================
// GetChannel - mirrors S_getChannel()
//
// Find a free channel, or steal one from a lower-priority sound.
// Same-origin-same-sfx: replace existing instance.
// =============================================================================

int32 UDoomAudioSystem::GetChannel(AActor* Origin, EDoomSfx SfxId, int32 Priority)
{
	int32 FreeChannel = -1;
	int32 LowestPriorityChannel = -1;
	int32 LowestPriority = INT32_MAX;

	for (int32 i = 0; i < Channels.Num(); ++i)
	{
		FDoomSoundChannel& Channel = Channels[i];

		// Same origin and same sound: kill and reuse this channel
		if (Channel.Origin.Get() == Origin && Channel.SfxId == SfxId && Origin != nullptr)
		{
			StopChannel(i);
			return i;
		}

		// Track free channels
		if (Channel.SfxId == EDoomSfx::None || !Channel.IsPlaying())
		{
			if (FreeChannel < 0)
			{
				FreeChannel = i;
			}
			continue;
		}

		// Track lowest priority channel for potential stealing
		if (Channel.Priority < LowestPriority)
		{
			LowestPriority = Channel.Priority;
			LowestPriorityChannel = i;
		}
	}

	// Use a free channel if available
	if (FreeChannel >= 0)
	{
		StopChannel(FreeChannel);
		return FreeChannel;
	}

	// Steal the lowest priority channel if our sound has higher priority
	if (LowestPriorityChannel >= 0 && Priority >= LowestPriority)
	{
		StopChannel(LowestPriorityChannel);
		return LowestPriorityChannel;
	}

	// No channel available
	return -1;
}

// =============================================================================
// StopChannel - mirrors S_StopChannel()
// =============================================================================

void UDoomAudioSystem::StopChannel(int32 ChannelIndex)
{
	if (!Channels.IsValidIndex(ChannelIndex))
	{
		return;
	}

	Channels[ChannelIndex].Stop();
}

// =============================================================================
// GetSfxPriority
//
// In original DOOM, priority is stored in sfxinfo_t. Here we use the enum
// ordinal as a simple proxy - lower enum values (weapons, environment) get
// higher priority. This can be overridden by populating FDoomSfxInfo tables.
// =============================================================================

int32 UDoomAudioSystem::GetSfxPriority(EDoomSfx SfxId) const
{
	// Higher value = higher priority. Invert the enum ordinal so that
	// sounds declared earlier in the enum (weapons, etc.) have higher priority.
	return static_cast<int32>(EDoomSfx::NumSfx) - static_cast<int32>(SfxId);
}

// =============================================================================
// GetRandomPitch - DOOM-style random pitch variation
//
// Original DOOM applied +/- ~6% pitch variation to most sounds.
// Some sounds (e.g., saw, chaingun) had no variation.
// =============================================================================

float UDoomAudioSystem::GetRandomPitch(EDoomSfx SfxId) const
{
	// Certain sounds should not have pitch variation
	switch (SfxId)
	{
	case EDoomSfx::Sawup:
	case EDoomSfx::Sawidl:
	case EDoomSfx::Sawful:
	case EDoomSfx::Sawhit:
	case EDoomSfx::Itemup:
	case EDoomSfx::Getpow:
		return 1.0f;
	default:
		break;
	}

	// DOOM pitch variation: 128 +/- random(0..7) out of 128 base
	// That gives approximately +/- 5.5% variation
	// We produce a value in [0.945, 1.055]
	const float Variation = (FMath::FRand() - 0.5f) * 0.11f;
	return 1.0f + Variation;
}

// =============================================================================
// ResolveSfxAsset - look up in SfxAssets map, load synchronously if needed
// =============================================================================

USoundBase* UDoomAudioSystem::ResolveSfxAsset(EDoomSfx SfxId) const
{
	const TSoftObjectPtr<USoundBase>* SoftPtr = SfxAssets.Find(SfxId);
	if (!SoftPtr)
	{
		return nullptr;
	}

	// If already loaded, return directly
	if (SoftPtr->IsValid())
	{
		return SoftPtr->Get();
	}

	// Synchronous load
	USoundBase* Loaded = SoftPtr->LoadSynchronous();
	return Loaded;
}

// =============================================================================
// ResolveMusicAsset - look up in MusicAssets map, load synchronously if needed
// =============================================================================

USoundBase* UDoomAudioSystem::ResolveMusicAsset(EDoomMusic MusicId) const
{
	const TSoftObjectPtr<USoundBase>* SoftPtr = MusicAssets.Find(MusicId);
	if (!SoftPtr)
	{
		return nullptr;
	}

	if (SoftPtr->IsValid())
	{
		return SoftPtr->Get();
	}

	USoundBase* Loaded = SoftPtr->LoadSynchronous();
	return Loaded;
}

// =============================================================================
// CreateAudioComponent
//
// Creates a UAudioComponent. If AttachTo is valid, attaches to that actor.
// Otherwise creates a world-level component for UI/global sounds.
// =============================================================================

UAudioComponent* UDoomAudioSystem::CreateAudioComponent(AActor* AttachTo, USoundBase* Sound) const
{
	if (!Sound)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		// Try to get world from the attach actor
		if (AttachTo)
		{
			World = AttachTo->GetWorld();
		}
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("DoomAudioSystem: No valid world for audio component creation"));
			return nullptr;
		}
	}

	UAudioComponent* AudioComp = nullptr;

	if (AttachTo)
	{
		// Create and attach to the actor's root component for positional audio
		AudioComp = NewObject<UAudioComponent>(AttachTo);
		if (AudioComp)
		{
			AudioComp->SetSound(Sound);
			AudioComp->bAutoActivate = false;
			AudioComp->bAutoDestroy = true;
			AudioComp->AttachToComponent(
				AttachTo->GetRootComponent(),
				FAttachmentTransformRules::KeepRelativeTransform);
			AudioComp->RegisterComponent();
		}
	}
	else
	{
		// Non-positional / UI sound: use UGameplayStatics for a fire-and-forget 2D sound
		// But we need a managed component, so create one on the world settings actor
		AActor* WorldSettings = World->GetWorldSettings();
		if (WorldSettings)
		{
			AudioComp = NewObject<UAudioComponent>(WorldSettings);
			if (AudioComp)
			{
				AudioComp->SetSound(Sound);
				AudioComp->bAutoActivate = false;
				AudioComp->bAutoDestroy = true;
				AudioComp->bIsUISound = true;
				AudioComp->RegisterComponent();
			}
		}
	}

	return AudioComp;
}
