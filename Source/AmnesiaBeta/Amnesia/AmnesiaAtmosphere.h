#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaAtmosphere.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class APostProcessVolume;
class UMaterialParameterCollection;

/**
 *  A complete lighting mood: sun, sky light, fog, color grading and season
 */
USTRUCT(BlueprintType)
struct FAmnesiaMood
{
	GENERATED_BODY()

	/** Name used to refer to this mood from cycles, mood volumes and Blueprints */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mood")
	FName Name;

	/** World rotation of the sun. Low pitch values give long shadows and light shafts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun")
	FRotator SunRotation = FRotator(-22.0f, 112.0f, 0.0f);

	/** Sun brightness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun", meta = (ClampMin = 0, Units = "lux"))
	float SunIntensity = 3.0f;

	/** Sun color temperature. Low values are warm, high values are cool */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun", meta = (ClampMin = 1700, ClampMax = 12000, Units = "K"))
	float SunTemperature = 6500.0f;

	/** Tint of the light shafts around the sun */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun")
	FLinearColor LightShaftTint = FLinearColor::White;

	/** How strongly the sun lights up the volumetric fog */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun", meta = (ClampMin = 0, ClampMax = 10))
	float SunFogScattering = 2.0f;

	/** Ambient sky light brightness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky", meta = (ClampMin = 0, ClampMax = 10))
	float SkyLightIntensity = 0.6f;

	/** Ambient sky light color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky")
	FLinearColor SkyLightColor = FLinearColor::White;

	/** Fog density. Higher values fade the background out faster */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog", meta = (ClampMin = 0, ClampMax = 0.5))
	float FogDensity = 0.035f;

	/** Color of the fog away from the sun */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog")
	FLinearColor FogColor = FLinearColor(0.1f, 0.2f, 0.3f);

	/** Color of the fog glow around the sun */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog")
	FLinearColor FogSunColor = FLinearColor(0.5f, 0.5f, 0.5f);

	/** Color gain applied to the shadows */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Color Grading")
	FVector4 ShadowsGain = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

	/** Color gain applied to the highlights */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Color Grading")
	FVector4 HighlightsGain = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

	/** Overall color saturation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Color Grading")
	FVector4 Saturation = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

	/** Exposure compensation. Negative values darken the scene */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Color Grading", meta = (ClampMin = -5, ClampMax = 5))
	float ExposureBias = 0.0f;

	/** Snow on the Spruce Forest assets and the ground. 0 is summer, 1 is full winter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Season", meta = (ClampMin = 0, ClampMax = 1))
	float Winter = 0.0f;

	/** Autumn colors on the Spruce Forest assets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Season", meta = (ClampMin = 0, ClampMax = 1))
	float Autumn = 0.0f;

	/** Rain strength read by the weather actor. 0 is dry, 1 is a downpour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta = (ClampMin = 0, ClampMax = 1))
	float RainIntensity = 0.0f;

	/** Snowfall strength read by the weather actor. 0 is none, 1 is a blizzard */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta = (ClampMin = 0, ClampMax = 1))
	float SnowIntensity = 0.0f;
};

/**
 *  Drives the level lighting through a list of moods.
 *  - Blends smoothly between moods
 *  - Can loop through a cycle of moods automatically, like golden hour and blue hour
 *  - Moods can switch seasons through the Spruce Forest material parameter collection
 */
UCLASS()
class AAmnesiaAtmosphere : public AActor
{
	GENERATED_BODY()

protected:

	/** Sun light driven by the moods */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Scene")
	TObjectPtr<ADirectionalLight> Sun;

	/** Ambient sky light driven by the moods */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Scene")
	TObjectPtr<ASkyLight> SkyLight;

	/** Height fog driven by the moods */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Scene")
	TObjectPtr<AExponentialHeightFog> Fog;

	/** Unbound post process volume used for color grading */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Scene")
	TObjectPtr<APostProcessVolume> PostProcess;

	/** Material parameter collection that holds the season parameters */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Scene")
	TObjectPtr<UMaterialParameterCollection> SeasonParameters;

	/** Name of the winter parameter in the season collection */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Scene")
	FName WinterParameterName = FName("Winter");

	/** Name of the autumn parameter in the season collection */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Scene")
	FName AutumnParameterName = FName("Autumn");

	/** If true, moods saved in the level are replaced or completed with the code defaults when the game starts, so new default moods and weather show up */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Moods")
	bool bRefreshDefaultMoods = true;

	/** All the moods this level can use */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Moods", meta = (TitleProperty = "Name"))
	TArray<FAmnesiaMood> Moods;

	/** Mood applied when the level starts */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Moods")
	FName StartMood = FName("Summer Golden");

	/** If true, the atmosphere loops through the cycle moods */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Cycle")
	bool bAutoCycle = true;

	/** Moods to loop through, in order */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Cycle")
	TArray<FName> Cycle;

	/** Time each cycle mood is held before blending to the next one */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Cycle", meta = (ClampMin = 0, ClampMax = 600, Units = "s"))
	float HoldTime = 25.0f;

	/** Time it takes to blend between cycle moods */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Cycle", meta = (ClampMin = 0, ClampMax = 600, Units = "s"))
	float CycleBlendTime = 10.0f;

	/** If true, F5 blends to the next mood and F6 toggles the cycle. Handy for testing */
	UPROPERTY(EditAnywhere, Category="Atmosphere|Debug")
	bool bEnableDebugKeys = true;

	/** Mood currently applied to the scene */
	FAmnesiaMood CurrentMood;

	/** Mood we're blending from */
	FAmnesiaMood BlendSourceMood;

	/** Mood we're blending to */
	FAmnesiaMood BlendTargetMood;

	/** Total time of the current blend */
	float BlendDuration = 0.0f;

	/** Time elapsed in the current blend */
	float BlendElapsed = 0.0f;

	/** If true, we're currently blending between two moods */
	bool bIsBlending = false;

	/** Index into the cycle of the mood we're holding or blending to */
	int32 CycleIndex = 0;

	/** Time left before the cycle moves on to the next mood */
	float HoldTimeLeft = 0.0f;

public:

	/** Constructor */
	AAmnesiaAtmosphere();

	/** Updates blends and the mood cycle */
	virtual void Tick(float DeltaTime) override;

	/** Blends from the current mood to the named one */
	UFUNCTION(BlueprintCallable, Category="Atmosphere")
	void BlendToMood(FName MoodName, float Duration = 3.0f);

	/** Replaces the mood cycle and blends to its first mood */
	UFUNCTION(BlueprintCallable, Category="Atmosphere")
	void SetCycle(const TArray<FName>& NewCycle, float Duration = 3.0f);

	/** Blends to the next mood in the mood list */
	UFUNCTION(BlueprintCallable, Category="Atmosphere")
	void NextMood();

	/** Turns the automatic mood cycle on or off */
	UFUNCTION(BlueprintCallable, Category="Atmosphere")
	void SetAutoCycle(bool bEnabled);

	/** Returns the name of the mood we're showing or blending to */
	UFUNCTION(BlueprintPure, Category="Atmosphere")
	FName GetTargetMoodName() const;

	/** Rain strength of the mood we're showing or blending to, 0 to 1 */
	UFUNCTION(BlueprintPure, Category="Atmosphere")
	float GetRainIntensity() const { return CurrentMood.RainIntensity; }

	/** Snow strength of the mood we're showing or blending to, 0 to 1 */
	UFUNCTION(BlueprintPure, Category="Atmosphere")
	float GetSnowIntensity() const { return CurrentMood.SnowIntensity; }

	/** Applies the start mood to the scene so it can be previewed in the editor */
	UFUNCTION(CallInEditor, Category="Atmosphere")
	void PreviewStartMood();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Returns the mood with the given name, or nullptr if there's none */
	const FAmnesiaMood* FindMood(FName MoodName) const;

	/** Pushes a mood to the lights, fog, post process and season parameters */
	void ApplyMood(const FAmnesiaMood& Mood);

	/** Interpolates every value of two moods */
	static FAmnesiaMood LerpMood(const FAmnesiaMood& A, const FAmnesiaMood& B, float Alpha);

	/** Binds the F5 and F6 test keys */
	void SetupDebugKeys();

	/** Flips the automatic mood cycle. Bound to the F6 test key */
	void ToggleAutoCycle();

	/** Fills the mood list with the default summer and winter moods */
	void AddDefaultMoods();
};
