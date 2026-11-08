#pragma once

#include "CoreMinimal.h"
#include "AmnesiaSound.generated.h"

class USoundBase;

/**
 *  A few takes of one sound effect.
 *  Each play picks a random take (never the same one twice in a row) with a little
 *  pitch and volume variation, so repeated swings and steps don't sound mechanical.
 */
USTRUCT(BlueprintType)
struct FAmnesiaSoundSet
{
	GENERATED_BODY()

	/** Takes to pick from */
	UPROPERTY(EditAnywhere, Category="Sound")
	TArray<TObjectPtr<USoundBase>> Sounds;

	/** Base volume */
	UPROPERTY(EditAnywhere, Category="Sound", meta = (ClampMin = 0))
	float Volume = 1.0f;

	/** Random pitch range */
	UPROPERTY(EditAnywhere, Category="Sound", meta = (ClampMin = 0.1))
	float PitchMin = 0.95f;

	UPROPERTY(EditAnywhere, Category="Sound", meta = (ClampMin = 0.1))
	float PitchMax = 1.05f;

	/** Sounds played farther than this from the camera are silent, fading out from half the range. 0 plays at full volume everywhere */
	UPROPERTY(EditAnywhere, Category="Sound", meta = (ClampMin = 0, Units = "cm"))
	float Range = 0.0f;

	/** True if there is anything to play */
	bool IsSet() const { return Sounds.Num() > 0; }

	/** Plays a random take at a world location */
	void PlayAt(const UObject* WorldContext, const FVector& Location, float VolumeScale = 1.0f) const;

	/** Plays a random take without a position, for the menu and the level up fanfare */
	void Play2D(const UObject* WorldContext, float VolumeScale = 1.0f) const;

	/** Returns a random take, not the last one if there is a choice */
	USoundBase* Pick() const;

	/** Test helper: when true every play is written to the log */
	static bool bLogPlays;

private:

	/** Take played last time */
	mutable int32 LastIndex = INDEX_NONE;
};
