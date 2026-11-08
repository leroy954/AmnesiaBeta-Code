#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaSound.h"
#include "AmnesiaWeather.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AAmnesiaAtmosphere;
class USoundBase;
class UAudioComponent;

/**
 *  Rain, snow and lightning that follow the camera.
 *  - Reads the rain and snow strength from the level's atmosphere, so the weather changes with the mood
 *  - Drops are instanced meshes that are recycled around the camera
 *  - Lightning flashes while it rains hard
 */
UCLASS()
class AAmnesiaWeather : public AActor
{
	GENERATED_BODY()

	/** Rain streaks */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UInstancedStaticMeshComponent* RainMesh;

	/** Snow flakes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UInstancedStaticMeshComponent* SnowMesh;

	/** Light that flashes during lightning */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* LightningLight;

protected:

	/** Atmosphere that decides the weather. If unset, the first atmosphere in the level is used */
	UPROPERTY(EditAnywhere, Category="Weather")
	TObjectPtr<AAmnesiaAtmosphere> Atmosphere;

	/** Material for the drops. Needs a Color vector and an Opacity scalar parameter */
	UPROPERTY(EditAnywhere, Category="Weather")
	TObjectPtr<UMaterialInterface> DropMaterial;

	/** Looping rain sound, faded with the rain strength */
	UPROPERTY(EditAnywhere, Category="Weather|Sound")
	TObjectPtr<USoundBase> RainLoopSound;

	/** Looping wind sound, faded with the snow strength */
	UPROPERTY(EditAnywhere, Category="Weather|Sound")
	TObjectPtr<USoundBase> WindLoopSound;

	/** Thunder played a moment after each lightning flash */
	UPROPERTY(EditAnywhere, Category="Weather|Sound")
	FAmnesiaSoundSet ThunderSounds;

	/** Volume of the zone ambience */
	UPROPERTY(EditAnywhere, Category="Weather|Sound", meta = (ClampMin = 0))
	float AmbienceVolume = 0.8f;

	/** Audio players: rain, wind, and two crossfading players for the zone ambience and for its extra layer */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> RainAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> WindAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AmbienceAudio[2];

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LayerAudio[2];

	/** Which player of each pair is fading in */
	int32 ActiveAmbience = 0;
	int32 ActiveLayer = 0;

	FTimerHandle ThunderTimer;

	/** Volumetric Super Storm actor in the level, if any. Formed and dissolved by the mood volumes and flashed with the lightning */
	UPROPERTY(EditAnywhere, Category="Weather|Storm")
	TObjectPtr<AActor> StormActor;

	/** True while the storm is shown */
	bool bStormActive = false;

	/** Number of rain streaks at full intensity */
	UPROPERTY(EditAnywhere, Category="Weather|Rain", meta = (ClampMin = 0))
	int32 MaxRainDrops = 900;

	/** Fall speed of the rain */
	UPROPERTY(EditAnywhere, Category="Weather|Rain", meta = (ClampMin = 0, Units = "cm/s"))
	float RainSpeed = 2800.0f;

	/** Sideways speed of the rain, positive blows towards +X */
	UPROPERTY(EditAnywhere, Category="Weather|Rain", meta = (Units = "cm/s"))
	float RainWind = -450.0f;

	/** Size of a rain streak: width, width, length */
	UPROPERTY(EditAnywhere, Category="Weather|Rain")
	FVector RainScale = FVector(0.035f, 0.035f, 1.4f);

	/** Rain color */
	UPROPERTY(EditAnywhere, Category="Weather|Rain")
	FLinearColor RainColor = FLinearColor(2.4f, 2.8f, 3.4f);

	/** Rain opacity */
	UPROPERTY(EditAnywhere, Category="Weather|Rain", meta = (ClampMin = 0, ClampMax = 1))
	float RainOpacity = 0.28f;

	/** Number of snow flakes at full intensity */
	UPROPERTY(EditAnywhere, Category="Weather|Snow", meta = (ClampMin = 0))
	int32 MaxSnowFlakes = 700;

	/** Fall speed of the snow */
	UPROPERTY(EditAnywhere, Category="Weather|Snow", meta = (ClampMin = 0, Units = "cm/s"))
	float SnowSpeed = 230.0f;

	/** Sideways speed of the snow */
	UPROPERTY(EditAnywhere, Category="Weather|Snow", meta = (Units = "cm/s"))
	float SnowWind = -120.0f;

	/** Size of a snow flake */
	UPROPERTY(EditAnywhere, Category="Weather|Snow", meta = (ClampMin = 0))
	float SnowSize = 0.09f;

	/** Snow opacity */
	UPROPERTY(EditAnywhere, Category="Weather|Snow", meta = (ClampMin = 0, ClampMax = 1))
	float SnowOpacity = 0.85f;

	/** Half width of the box of drops around the camera */
	UPROPERTY(EditAnywhere, Category="Weather|Area", meta = (ClampMin = 100))
	float HalfWidth = 2600.0f;

	/** Half height of the box of drops around the camera */
	UPROPERTY(EditAnywhere, Category="Weather|Area", meta = (ClampMin = 100))
	float HalfHeight = 1300.0f;

	/** How far behind the hero (away from the camera) the drops go, along Y */
	UPROPERTY(EditAnywhere, Category="Weather|Area")
	float BackDepth = 3500.0f;

	/** How far in front of the hero (towards the camera) the drops go, along Y */
	UPROPERTY(EditAnywhere, Category="Weather|Area")
	float FrontDepth = 500.0f;

	/** Lightning only happens when the rain is at least this strong */
	UPROPERTY(EditAnywhere, Category="Weather|Lightning", meta = (ClampMin = 0, ClampMax = 1))
	float LightningRainThreshold = 0.8f;

	/** Shortest time between lightning strikes */
	UPROPERTY(EditAnywhere, Category="Weather|Lightning", meta = (ClampMin = 0.5, Units = "s"))
	float MinLightningInterval = 5.0f;

	/** Longest time between lightning strikes */
	UPROPERTY(EditAnywhere, Category="Weather|Lightning", meta = (ClampMin = 0.5, Units = "s"))
	float MaxLightningInterval = 13.0f;

	/** Brightness of the lightning flash */
	UPROPERTY(EditAnywhere, Category="Weather|Lightning", meta = (ClampMin = 0))
	float LightningIntensity = 25000.0f;

	/** Position of a drop around the camera, with its own random fall speed factor */
	struct FDrop
	{
		FVector Position = FVector::ZeroVector;
		float SpeedFactor = 1.0f;
		float Phase = 0.0f;
	};

	TArray<FDrop> RainDrops;
	TArray<FDrop> SnowFlakes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RainMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SnowMaterial;

	/** Time left before the next lightning strike */
	float LightningCountdown = 8.0f;

	/** Time since the current lightning strike started. Negative when there's none */
	float LightningTime = -1.0f;

	/** Strength the rain currently shows, smoothed so the weather fades in and out */
	float ShownRain = 0.0f;

	/** Strength the snow currently shows, smoothed so the weather fades in and out */
	float ShownSnow = 0.0f;

public:

	/** Constructor */
	AAmnesiaWeather();

	/** Moves the drops and the lightning */
	virtual void Tick(float DeltaTime) override;

	/** Forms or dissolves the storm in the sky. Does nothing if the state doesn't change or there's no storm */
	void SetStormActive(bool bActive);

	/** Crossfades to a new looping ambience, like forest birds or cave drips, plus an optional extra layer like a river. Null fades to silence */
	void SetAmbience(USoundBase* Sound, USoundBase* Layer = nullptr, float LayerVolume = 0.6f);

protected:

	/** Fades one crossfading pair of players over to a new sound */
	void CrossfadeTo(TObjectPtr<UAudioComponent> (&Players)[2], int32& Active, USoundBase* Sound, float Volume);

	/** Plays the thunder */
	void PlayThunder();

public:

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Returns the location the weather is centered on */
	FVector GetFocusLocation() const;

	/** Scatters drops through the box around a location */
	void ScatterDrops(TArray<FDrop>& Drops, int32 Count, const FVector& Focus);

	/** Moves drops, wraps them around the focus and updates the instances */
	void UpdateDrops(UInstancedStaticMeshComponent* Mesh, TArray<FDrop>& Drops, float Strength, const FVector& Focus, float DeltaTime, float Speed, float Wind, const FVector& Scale, bool bSway);

	/** Runs the lightning timer and flashes */
	void UpdateLightning(float DeltaTime, const FVector& Focus);
};
