#include "AmnesiaSound.h"
#include "AmnesiaBeta.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

bool FAmnesiaSoundSet::bLogPlays = false;

USoundBase* FAmnesiaSoundSet::Pick() const
{
	if (Sounds.Num() == 0)
	{
		return nullptr;
	}

	int32 Index = FMath::RandRange(0, Sounds.Num() - 1);

	// step past the last take so a repeat never plays twice in a row
	if (Sounds.Num() > 1 && Index == LastIndex)
	{
		Index = (Index + 1 + FMath::RandRange(0, Sounds.Num() - 2)) % Sounds.Num();
	}

	LastIndex = Index;

	return Sounds[Index];
}

void FAmnesiaSoundSet::PlayAt(const UObject* WorldContext, const FVector& Location, float VolumeScale) const
{
	USoundBase* Sound = Pick();

	if (!Sound || !WorldContext)
	{
		return;
	}

	float Loudness = Volume * VolumeScale;

	// fade with the distance to the camera, since the sounds carry no attenuation settings.
	// Only the side scrolling plane counts: the camera sits far out in front, but what is next to the hero is close
	if (Range > 0.0f)
	{
		const UWorld* World = WorldContext->GetWorld();
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;

		if (PlayerController && PlayerController->PlayerCameraManager)
		{
			const FVector Camera = PlayerController->PlayerCameraManager->GetCameraLocation();
			const float Distance = FVector2D(Camera.X - Location.X, Camera.Z - Location.Z).Size();
			Loudness *= 1.0f - FMath::Clamp((Distance - Range * 0.5f) / (Range * 0.5f), 0.0f, 1.0f);
		}
	}

	if (bLogPlays && Loudness > 0.01f)
	{
		UE_LOG(LogAmnesiaBeta, Log, TEXT("Sound: %s at X %.0f, volume %.2f"), *Sound->GetName(), Location.X, Loudness);
	}

	if (Loudness > 0.01f)
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, Loudness, FMath::FRandRange(PitchMin, PitchMax));
	}
}

void FAmnesiaSoundSet::Play2D(const UObject* WorldContext, float VolumeScale) const
{
	if (USoundBase* Sound = Pick())
	{
		if (bLogPlays)
		{
			UE_LOG(LogAmnesiaBeta, Log, TEXT("Sound: %s (2D), volume %.2f"), *Sound->GetName(), Volume * VolumeScale);
		}

		UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume * VolumeScale, FMath::FRandRange(PitchMin, PitchMax));
	}
}
