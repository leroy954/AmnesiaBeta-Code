#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaSound.h"
#include "AmnesiaMoodVolume.generated.h"

class UBoxComponent;
class AAmnesiaAtmosphere;
class USoundBase;

/**
 *  Switches the level atmosphere to a different set of moods while the player is inside.
 *  A single mood is held, several moods are cycled through.
 */
UCLASS()
class AAmnesiaMoodVolume : public AActor
{
	GENERATED_BODY()

	/** Trigger box */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* Box;

protected:

	/** Atmosphere to change. If unset, the first atmosphere in the level is used */
	UPROPERTY(EditAnywhere, Category="Mood")
	TObjectPtr<AAmnesiaAtmosphere> Atmosphere;

	/** Moods to use while the player is inside */
	UPROPERTY(EditAnywhere, Category="Mood")
	TArray<FName> MoodsOnEnter;

	/** Moods to go back to when the player leaves. Leave empty to keep the new moods */
	UPROPERTY(EditAnywhere, Category="Mood")
	TArray<FName> MoodsOnExit;

	/** Storm control. 1 forms the Volumetric Super Storm while the player is inside, -1 dissolves it, 0 leaves it alone */
	UPROPERTY(EditAnywhere, Category="Mood", meta = (ClampMin = -1, ClampMax = 1))
	int32 StormControl = 0;

	/** Looping ambience or music for this zone. Crossfades when the player walks in */
	UPROPERTY(EditAnywhere, Category="Sound")
	TObjectPtr<USoundBase> AmbientSound;

	/** Optional second loop on top, like a river next to the forest */
	UPROPERTY(EditAnywhere, Category="Sound")
	TObjectPtr<USoundBase> AmbientLayerSound;

	/** Volume of the second loop, relative to the ambience */
	UPROPERTY(EditAnywhere, Category="Sound", meta = (ClampMin = 0))
	float AmbientLayerVolume = 0.6f;

	/** The hero's footsteps on the ground of this zone. Empty keeps the hero's default steps */
	UPROPERTY(EditAnywhere, Category="Sound")
	FAmnesiaSoundSet FootstepSounds;

	/** If true, the hero's camera tilts to CameraPitch while the player is inside */
	UPROPERTY(EditAnywhere, Category="Camera")
	bool bSetCameraPitch = false;

	/** Camera pitch to use inside. Positive values look up and show more sky */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = -45, ClampMax = 45, Units = "deg"))
	float CameraPitch = -8.0f;

	/** Time it takes to blend into the new moods */
	UPROPERTY(EditAnywhere, Category="Mood", meta = (ClampMin = 0, ClampMax = 60, Units = "s"))
	float BlendTime = 4.0f;

public:

	/** Constructor */
	AAmnesiaMoodVolume();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Switches to the enter moods when the player walks in */
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Switches to the exit moods when the player walks out */
	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Returns the atmosphere to change, looking it up in the level if needed */
	AAmnesiaAtmosphere* GetAtmosphere();
};
