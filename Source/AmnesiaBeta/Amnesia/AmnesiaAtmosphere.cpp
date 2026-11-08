#include "AmnesiaAtmosphere.h"
#include "AmnesiaBeta.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InputComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace
{
	/** Component-wise interpolation for color grading vectors */
	FVector4 LerpVector4(const FVector4& A, const FVector4& B, float Alpha)
	{
		return FVector4(FMath::Lerp(A.X, B.X, Alpha), FMath::Lerp(A.Y, B.Y, Alpha), FMath::Lerp(A.Z, B.Z, Alpha), FMath::Lerp(A.W, B.W, Alpha));
	}
}

AAmnesiaAtmosphere::AAmnesiaAtmosphere()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// golden hour and blue hour loop by default
	AddDefaultMoods();

	Cycle.Add(FName("Summer Golden"));
	Cycle.Add(FName("Summer Blue"));
}

void AAmnesiaAtmosphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsBlending)
	{
		BlendElapsed += DeltaTime;

		const float Alpha = BlendDuration > 0.0f ? FMath::Clamp(BlendElapsed / BlendDuration, 0.0f, 1.0f) : 1.0f;

		// ease in and out of the blend
		CurrentMood = LerpMood(BlendSourceMood, BlendTargetMood, FMath::SmoothStep(0.0f, 1.0f, Alpha));
		ApplyMood(CurrentMood);

		// hold the new mood once the blend is done
		if (Alpha >= 1.0f)
		{
			bIsBlending = false;
			HoldTimeLeft = HoldTime;
		}

		return;
	}

	// move on through the cycle once the hold time is over
	if (bAutoCycle && Cycle.Num() > 1)
	{
		HoldTimeLeft -= DeltaTime;

		if (HoldTimeLeft <= 0.0f)
		{
			BlendToMood(Cycle[(CycleIndex + 1) % Cycle.Num()], CycleBlendTime);
		}
	}
}

void AAmnesiaAtmosphere::BlendToMood(FName MoodName, float Duration)
{
	const FAmnesiaMood* Mood = FindMood(MoodName);

	if (!Mood)
	{
		UE_LOG(LogAmnesiaBeta, Warning, TEXT("Atmosphere has no mood named %s"), *MoodName.ToString());
		return;
	}

	// continue the cycle from this mood if it's part of it
	const int32 Index = Cycle.IndexOfByKey(MoodName);

	if (Index != INDEX_NONE)
	{
		CycleIndex = Index;
	}

	// switch right away if there's nothing to blend
	if (Duration <= 0.0f)
	{
		bIsBlending = false;
		CurrentMood = *Mood;
		ApplyMood(CurrentMood);
		HoldTimeLeft = HoldTime;

		return;
	}

	BlendSourceMood = CurrentMood;
	BlendTargetMood = *Mood;
	BlendDuration = Duration;
	BlendElapsed = 0.0f;
	bIsBlending = true;
}

void AAmnesiaAtmosphere::SetCycle(const TArray<FName>& NewCycle, float Duration)
{
	Cycle = NewCycle;

	if (Cycle.Num() > 0)
	{
		BlendToMood(Cycle[0], Duration);
	}
}

void AAmnesiaAtmosphere::NextMood()
{
	if (Moods.Num() == 0)
	{
		return;
	}

	// find the mood we're showing or blending to and move on to the one after it
	const FName TargetName = GetTargetMoodName();
	const int32 Index = Moods.IndexOfByPredicate([TargetName](const FAmnesiaMood& Mood) { return Mood.Name == TargetName; });

	BlendToMood(Moods[(Index + 1) % Moods.Num()].Name, 3.0f);
}

void AAmnesiaAtmosphere::SetAutoCycle(bool bEnabled)
{
	bAutoCycle = bEnabled;
	HoldTimeLeft = HoldTime;
}

FName AAmnesiaAtmosphere::GetTargetMoodName() const
{
	return bIsBlending ? BlendTargetMood.Name : CurrentMood.Name;
}

void AAmnesiaAtmosphere::PreviewStartMood()
{
	const FAmnesiaMood* Mood = FindMood(StartMood);

	if (!Mood)
	{
		UE_LOG(LogAmnesiaBeta, Warning, TEXT("Atmosphere has no start mood named %s"), *StartMood.ToString());
		return;
	}

	// mark the scene actors as modified so the preview can be saved and undone
	const TArray<AActor*> SceneActors = { Sun.Get(), SkyLight.Get(), Fog.Get(), PostProcess.Get() };

	for (AActor* SceneActor : SceneActors)
	{
		if (SceneActor)
		{
			SceneActor->Modify();

			if (USceneComponent* Root = SceneActor->GetRootComponent())
			{
				Root->Modify();
			}
		}
	}

	CurrentMood = *Mood;
	ApplyMood(CurrentMood);
}

void AAmnesiaAtmosphere::BeginPlay()
{
	Super::BeginPlay();

	// bring the saved moods up to date with the code defaults
	if (bRefreshDefaultMoods)
	{
		const TArray<FAmnesiaMood> SavedMoods = Moods;

		Moods.Reset();
		AddDefaultMoods();

		// keep any custom mood the level added by hand
		for (const FAmnesiaMood& Saved : SavedMoods)
		{
			if (!FindMood(Saved.Name))
			{
				Moods.Add(Saved);
			}
		}
	}

	// start from the start mood, or from the first cycle mood if there's none
	const FName FirstMood = !StartMood.IsNone() ? StartMood : (Cycle.Num() > 0 ? Cycle[0] : NAME_None);

	BlendToMood(FirstMood, 0.0f);

	if (bEnableDebugKeys)
	{
		SetupDebugKeys();
	}
}

const FAmnesiaMood* AAmnesiaAtmosphere::FindMood(FName MoodName) const
{
	return Moods.FindByPredicate([MoodName](const FAmnesiaMood& Mood) { return Mood.Name == MoodName; });
}

void AAmnesiaAtmosphere::ApplyMood(const FAmnesiaMood& Mood)
{
	if (Sun)
	{
		Sun->SetActorRotation(Mood.SunRotation);

		if (ULightComponent* Light = Sun->GetLightComponent())
		{
			Light->SetIntensity(Mood.SunIntensity);
			Light->SetUseTemperature(true);
			Light->SetTemperature(Mood.SunTemperature);
			Light->SetBloomTint(Mood.LightShaftTint.ToFColor(true));
			Light->SetVolumetricScatteringIntensity(Mood.SunFogScattering);
		}
	}

	if (SkyLight)
	{
		if (USkyLightComponent* Sky = SkyLight->GetLightComponent())
		{
			Sky->SetIntensity(Mood.SkyLightIntensity);
			Sky->SetLightColor(Mood.SkyLightColor);
		}
	}

	if (Fog)
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			FogComponent->SetFogDensity(Mood.FogDensity);
			FogComponent->SetFogInscatteringColor(Mood.FogColor);
			FogComponent->SetDirectionalInscatteringColor(Mood.FogSunColor);
		}
	}

	if (PostProcess)
	{
		FPostProcessSettings& Settings = PostProcess->Settings;

		Settings.bOverride_ColorGainShadows = true;
		Settings.ColorGainShadows = Mood.ShadowsGain;

		Settings.bOverride_ColorGainHighlights = true;
		Settings.ColorGainHighlights = Mood.HighlightsGain;

		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = Mood.Saturation;

		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = Mood.ExposureBias;
	}

	if (SeasonParameters)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, SeasonParameters, WinterParameterName, Mood.Winter);
		UKismetMaterialLibrary::SetScalarParameterValue(this, SeasonParameters, AutumnParameterName, Mood.Autumn);
	}
}

FAmnesiaMood AAmnesiaAtmosphere::LerpMood(const FAmnesiaMood& A, const FAmnesiaMood& B, float Alpha)
{
	FAmnesiaMood Result;

	Result.Name = Alpha < 1.0f ? A.Name : B.Name;

	Result.SunRotation = FQuat::Slerp(A.SunRotation.Quaternion(), B.SunRotation.Quaternion(), Alpha).Rotator();
	Result.SunIntensity = FMath::Lerp(A.SunIntensity, B.SunIntensity, Alpha);
	Result.SunTemperature = FMath::Lerp(A.SunTemperature, B.SunTemperature, Alpha);
	Result.LightShaftTint = FMath::Lerp(A.LightShaftTint, B.LightShaftTint, Alpha);
	Result.SunFogScattering = FMath::Lerp(A.SunFogScattering, B.SunFogScattering, Alpha);

	Result.SkyLightIntensity = FMath::Lerp(A.SkyLightIntensity, B.SkyLightIntensity, Alpha);
	Result.SkyLightColor = FMath::Lerp(A.SkyLightColor, B.SkyLightColor, Alpha);

	Result.FogDensity = FMath::Lerp(A.FogDensity, B.FogDensity, Alpha);
	Result.FogColor = FMath::Lerp(A.FogColor, B.FogColor, Alpha);
	Result.FogSunColor = FMath::Lerp(A.FogSunColor, B.FogSunColor, Alpha);

	Result.ShadowsGain = LerpVector4(A.ShadowsGain, B.ShadowsGain, Alpha);
	Result.HighlightsGain = LerpVector4(A.HighlightsGain, B.HighlightsGain, Alpha);
	Result.Saturation = LerpVector4(A.Saturation, B.Saturation, Alpha);
	Result.ExposureBias = FMath::Lerp(A.ExposureBias, B.ExposureBias, Alpha);

	Result.Winter = FMath::Lerp(A.Winter, B.Winter, Alpha);
	Result.Autumn = FMath::Lerp(A.Autumn, B.Autumn, Alpha);

	Result.RainIntensity = FMath::Lerp(A.RainIntensity, B.RainIntensity, Alpha);
	Result.SnowIntensity = FMath::Lerp(A.SnowIntensity, B.SnowIntensity, Alpha);

	return Result;
}

void AAmnesiaAtmosphere::SetupDebugKeys()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
	{
		return;
	}

	EnableInput(PlayerController);

	InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AAmnesiaAtmosphere::NextMood);
	InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AAmnesiaAtmosphere::ToggleAutoCycle);
}

void AAmnesiaAtmosphere::ToggleAutoCycle()
{
	SetAutoCycle(!bAutoCycle);
}

void AAmnesiaAtmosphere::AddDefaultMoods()
{
	// warm golden hour, the sun low behind the trees
	FAmnesiaMood& SummerGolden = Moods.AddDefaulted_GetRef();
	SummerGolden.Name = FName("Summer Golden");
	SummerGolden.SunRotation = FRotator(-11.0f, 100.0f, 0.0f);
	SummerGolden.SunIntensity = 3.0f;
	SummerGolden.SunTemperature = 3900.0f;
	SummerGolden.LightShaftTint = FLinearColor(1.0f, 0.8f, 0.55f);
	SummerGolden.SunFogScattering = 3.0f;
	SummerGolden.SkyLightIntensity = 0.7f;
	SummerGolden.SkyLightColor = FLinearColor(0.75f, 0.84f, 1.0f);
	SummerGolden.FogDensity = 0.035f;
	SummerGolden.FogColor = FLinearColor(0.12f, 0.26f, 0.38f);
	SummerGolden.FogSunColor = FLinearColor(1.0f, 0.55f, 0.25f);
	SummerGolden.ShadowsGain = FVector4(0.78f, 0.9f, 1.15f, 1.0f);
	SummerGolden.HighlightsGain = FVector4(1.12f, 1.0f, 0.85f, 1.0f);
	SummerGolden.Saturation = FVector4(1.05f, 1.05f, 1.1f, 1.0f);

	// cool blue hour with teal fog
	FAmnesiaMood& SummerBlue = Moods.AddDefaulted_GetRef();
	SummerBlue.Name = FName("Summer Blue");
	SummerBlue.SunRotation = FRotator(-22.0f, 112.0f, 0.0f);
	SummerBlue.SunIntensity = 3.0f;
	SummerBlue.SunTemperature = 7800.0f;
	SummerBlue.LightShaftTint = FLinearColor(0.6f, 0.85f, 1.0f);
	SummerBlue.SunFogScattering = 2.0f;
	SummerBlue.SkyLightIntensity = 0.55f;
	SummerBlue.SkyLightColor = FLinearColor(0.63f, 0.78f, 1.0f);
	SummerBlue.FogDensity = 0.04f;
	SummerBlue.FogColor = FLinearColor(0.05f, 0.16f, 0.25f);
	SummerBlue.FogSunColor = FLinearColor(0.35f, 0.6f, 0.75f);
	SummerBlue.ShadowsGain = FVector4(0.7f, 0.88f, 1.15f, 1.0f);
	SummerBlue.HighlightsGain = FVector4(0.95f, 1.0f, 1.08f, 1.0f);
	SummerBlue.Saturation = FVector4(1.1f, 1.1f, 1.1f, 1.0f);
	SummerBlue.ExposureBias = -0.3f;

	// snowy forest at sunset
	FAmnesiaMood& WinterGolden = Moods.AddDefaulted_GetRef();
	WinterGolden.Name = FName("Winter Golden");
	WinterGolden.SunRotation = FRotator(-9.0f, 100.0f, 0.0f);
	WinterGolden.SunIntensity = 3.0f;
	WinterGolden.SunTemperature = 4500.0f;
	WinterGolden.LightShaftTint = FLinearColor(1.0f, 0.85f, 0.7f);
	WinterGolden.SunFogScattering = 2.5f;
	WinterGolden.SkyLightIntensity = 0.9f;
	WinterGolden.SkyLightColor = FLinearColor(0.8f, 0.88f, 1.0f);
	WinterGolden.FogDensity = 0.03f;
	WinterGolden.FogColor = FLinearColor(0.3f, 0.4f, 0.55f);
	WinterGolden.FogSunColor = FLinearColor(1.0f, 0.7f, 0.5f);
	WinterGolden.ShadowsGain = FVector4(0.8f, 0.9f, 1.15f, 1.0f);
	WinterGolden.HighlightsGain = FVector4(1.08f, 1.02f, 0.95f, 1.0f);
	WinterGolden.Saturation = FVector4(0.9f, 0.9f, 0.9f, 1.0f);
	WinterGolden.ExposureBias = 0.2f;
	WinterGolden.Winter = 1.0f;
	WinterGolden.SnowIntensity = 0.6f;

	// cold, misty winter dusk
	FAmnesiaMood& WinterBlue = Moods.AddDefaulted_GetRef();
	WinterBlue.Name = FName("Winter Blue");
	WinterBlue.SunRotation = FRotator(-25.0f, 112.0f, 0.0f);
	WinterBlue.SunIntensity = 2.5f;
	WinterBlue.SunTemperature = 9000.0f;
	WinterBlue.LightShaftTint = FLinearColor(0.7f, 0.85f, 1.0f);
	WinterBlue.SunFogScattering = 1.5f;
	WinterBlue.SkyLightIntensity = 0.8f;
	WinterBlue.SkyLightColor = FLinearColor(0.7f, 0.82f, 1.0f);
	WinterBlue.FogDensity = 0.045f;
	WinterBlue.FogColor = FLinearColor(0.18f, 0.28f, 0.42f);
	WinterBlue.FogSunColor = FLinearColor(0.5f, 0.65f, 0.85f);
	WinterBlue.ShadowsGain = FVector4(0.75f, 0.88f, 1.2f, 1.0f);
	WinterBlue.HighlightsGain = FVector4(0.95f, 1.0f, 1.1f, 1.0f);
	WinterBlue.Saturation = FVector4(0.85f, 0.85f, 0.85f, 1.0f);
	WinterBlue.Winter = 1.0f;
	WinterBlue.SnowIntensity = 1.0f;

	// the village: late summer turning to autumn, warm and dusty
	// copy by value, because adding moods can move the array in memory
	const FAmnesiaMood SummerGoldenCopy = Moods[0];
	FAmnesiaMood& AutumnGolden = Moods.AddDefaulted_GetRef();
	AutumnGolden = SummerGoldenCopy;
	AutumnGolden.Name = FName("Autumn Golden");
	AutumnGolden.SunTemperature = 3400.0f;
	AutumnGolden.LightShaftTint = FLinearColor(1.0f, 0.7f, 0.4f);
	AutumnGolden.FogColor = FLinearColor(0.2f, 0.2f, 0.28f);
	AutumnGolden.HighlightsGain = FVector4(1.15f, 0.98f, 0.8f, 1.0f);
	AutumnGolden.Autumn = 1.0f;

	// ruins in a green mist, sun shafts breaking through
	FAmnesiaMood& RuinsMist = Moods.AddDefaulted_GetRef();
	RuinsMist.Name = FName("Ruins Mist");
	RuinsMist.SunRotation = FRotator(-30.0f, 105.0f, 0.0f);
	RuinsMist.SunIntensity = 2.8f;
	RuinsMist.SunTemperature = 5200.0f;
	RuinsMist.LightShaftTint = FLinearColor(0.85f, 1.0f, 0.8f);
	RuinsMist.SunFogScattering = 4.0f;
	RuinsMist.SkyLightIntensity = 0.75f;
	RuinsMist.SkyLightColor = FLinearColor(0.7f, 0.9f, 0.85f);
	RuinsMist.FogDensity = 0.06f;
	RuinsMist.FogColor = FLinearColor(0.12f, 0.24f, 0.2f);
	RuinsMist.FogSunColor = FLinearColor(0.7f, 0.85f, 0.5f);
	RuinsMist.ShadowsGain = FVector4(0.75f, 0.95f, 0.95f, 1.0f);
	RuinsMist.HighlightsGain = FVector4(1.0f, 1.05f, 0.9f, 1.0f);
	RuinsMist.Saturation = FVector4(1.0f, 1.05f, 1.0f, 1.0f);

	// the same ruins in light rain
	const FAmnesiaMood RuinsMistCopy = Moods.Last();
	FAmnesiaMood& RuinsRain = Moods.AddDefaulted_GetRef();
	RuinsRain = RuinsMistCopy;
	RuinsRain.Name = FName("Ruins Rain");
	RuinsRain.SunIntensity = 1.2f;
	RuinsRain.SkyLightIntensity = 0.9f;
	RuinsRain.FogDensity = 0.08f;
	RuinsRain.FogColor = FLinearColor(0.1f, 0.18f, 0.2f);
	RuinsRain.Saturation = FVector4(0.85f, 0.9f, 0.9f, 1.0f);
	RuinsRain.RainIntensity = 0.55f;

	// underground: almost no sky, the glowing mushrooms and torches carry the scene
	FAmnesiaMood& Cave = Moods.AddDefaulted_GetRef();
	Cave.Name = FName("Cave");
	Cave.SunRotation = FRotator(-50.0f, 100.0f, 0.0f);
	Cave.SunIntensity = 0.0f;
	Cave.SunTemperature = 7000.0f;
	Cave.SunFogScattering = 0.0f;
	Cave.SkyLightIntensity = 0.35f;
	Cave.SkyLightColor = FLinearColor(0.45f, 0.55f, 1.0f);
	Cave.FogDensity = 0.05f;
	Cave.FogColor = FLinearColor(0.04f, 0.05f, 0.14f);
	Cave.FogSunColor = FLinearColor(0.1f, 0.12f, 0.3f);
	Cave.ShadowsGain = FVector4(0.7f, 0.8f, 1.3f, 1.0f);
	Cave.HighlightsGain = FVector4(0.9f, 1.0f, 1.2f, 1.0f);
	Cave.Saturation = FVector4(1.1f, 1.1f, 1.2f, 1.0f);
	Cave.ExposureBias = 0.4f;

	// the storm: heavy rain, dark clouds and lightning
	FAmnesiaMood& Storm = Moods.AddDefaulted_GetRef();
	Storm.Name = FName("Storm");
	Storm.SunRotation = FRotator(-35.0f, 105.0f, 0.0f);
	Storm.SunIntensity = 0.8f;
	Storm.SunTemperature = 8500.0f;
	Storm.LightShaftTint = FLinearColor(0.6f, 0.7f, 0.85f);
	Storm.SunFogScattering = 0.5f;
	Storm.SkyLightIntensity = 0.7f;
	Storm.SkyLightColor = FLinearColor(0.55f, 0.65f, 0.85f);
	Storm.FogDensity = 0.07f;
	Storm.FogColor = FLinearColor(0.07f, 0.1f, 0.15f);
	Storm.FogSunColor = FLinearColor(0.25f, 0.3f, 0.4f);
	Storm.ShadowsGain = FVector4(0.75f, 0.85f, 1.1f, 1.0f);
	Storm.HighlightsGain = FVector4(0.85f, 0.92f, 1.05f, 1.0f);
	Storm.Saturation = FVector4(0.8f, 0.85f, 0.9f, 1.0f);
	Storm.ExposureBias = 0.1f;
	Storm.RainIntensity = 1.0f;
}
