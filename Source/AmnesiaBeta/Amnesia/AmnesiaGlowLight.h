#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaGlowLight.generated.h"

class UPointLightComponent;

/**
 *  A point light with a few simple controls, for torches, mushrooms and campfires.
 *  It can flicker, and its settings are plain properties so they can be set from a level script.
 */
UCLASS()
class AAmnesiaGlowLight : public AActor
{
	GENERATED_BODY()

	/** The light */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* Light;

protected:

	/** Light color */
	UPROPERTY(EditAnywhere, Category="Glow")
	FLinearColor LightColor = FLinearColor(1.0f, 0.62f, 0.3f);

	/** Brightness in candelas. A torch is around 100, a dim glow around 40 */
	UPROPERTY(EditAnywhere, Category="Glow", meta = (ClampMin = 0))
	float Intensity = 100.0f;

	/** How far the light reaches */
	UPROPERTY(EditAnywhere, Category="Glow", meta = (ClampMin = 10, Units = "cm"))
	float Radius = 900.0f;

	/** How much the brightness flickers, as a fraction of the intensity. 0 is steady */
	UPROPERTY(EditAnywhere, Category="Glow", meta = (ClampMin = 0, ClampMax = 1))
	float FlickerAmount = 0.0f;

	/** Speed of the flicker */
	UPROPERTY(EditAnywhere, Category="Glow", meta = (ClampMin = 0))
	float FlickerSpeed = 7.0f;

	/** If true, the light casts shadows. Expensive, use sparingly */
	UPROPERTY(EditAnywhere, Category="Glow")
	bool bCastShadows = false;

	/** Random offset so lights placed together don't flicker in sync */
	float FlickerOffset = 0.0f;

	/** A light that fades out and removes itself: when it started, and how long it takes. 0 means it stays */
	float FadeStart = 0.0f;
	float FadeDuration = 0.0f;

public:

	/** Constructor */
	AAmnesiaGlowLight();

	/** Applies the settings, in the editor and when spawned */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Flickers the light */
	virtual void Tick(float DeltaTime) override;

	/** Sets the light up from code, for a flash that is made while the game runs */
	void Configure(const FLinearColor& NewColor, float NewIntensity, float NewRadius, float NewFlicker);

	/** The light is brightest at once, fades out over this long and then removes itself */
	void FadeOut(float Duration);

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;
};
