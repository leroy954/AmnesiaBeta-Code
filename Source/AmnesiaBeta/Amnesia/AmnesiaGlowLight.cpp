#include "AmnesiaGlowLight.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AAmnesiaGlowLight::AAmnesiaGlowLight()
{
	PrimaryActorTick.bCanEverTick = true;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	RootComponent = Light;

	Light->SetMobility(EComponentMobility::Movable);
	Light->IntensityUnits = ELightUnits::Candelas;
}

void AAmnesiaGlowLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Light->SetLightColor(LightColor);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(Radius);
	Light->SetCastShadows(bCastShadows);
}

void AAmnesiaGlowLight::BeginPlay()
{
	Super::BeginPlay();

	// start each light at a different point of its flicker
	FlickerOffset = FMath::FRandRange(0.0f, 100.0f);

	// steady lights don't need to tick
	SetActorTickEnabled(FlickerAmount > 0.0f);
}

void AAmnesiaGlowLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// a light far from the hero is not seen: flicker it a couple of times a second only. A level has many of them
	if (const APawn* Watcher = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const bool bFar = FMath::Abs(Watcher->GetActorLocation().X - GetActorLocation().X) > 7000.0f;
		SetActorTickInterval(bFar ? 0.5f : 0.0f);

		if (bFar)
		{
			return;
		}
	}

	const float Time = GetWorld()->GetTimeSeconds() * FlickerSpeed + FlickerOffset;

	// a few sine waves at unrelated speeds read as a random flicker
	const float Noise = 0.5f * FMath::Sin(Time) + 0.3f * FMath::Sin(Time * 2.3f + 1.7f) + 0.2f * FMath::Sin(Time * 5.1f + 4.2f);

	float Fade = 1.0f;

	if (FadeDuration > 0.0f)
	{
		const float Elapsed = GetWorld()->GetTimeSeconds() - FadeStart;

		if (Elapsed >= FadeDuration)
		{
			Destroy();
			return;
		}

		// bright at once, then out, the way a flash of fire dies down
		Fade = FMath::Square(1.0f - Elapsed / FadeDuration);
	}

	Light->SetIntensity(Intensity * (1.0f + Noise * FlickerAmount) * Fade);
}

void AAmnesiaGlowLight::Configure(const FLinearColor& NewColor, float NewIntensity, float NewRadius, float NewFlicker)
{
	LightColor = NewColor;
	Intensity = NewIntensity;
	Radius = NewRadius;
	FlickerAmount = NewFlicker;

	Light->SetLightColor(LightColor);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(Radius);
}

void AAmnesiaGlowLight::FadeOut(float Duration)
{
	FadeStart = GetWorld()->GetTimeSeconds();
	FadeDuration = FMath::Max(Duration, 0.05f);

	// a steady light does not tick, a fading one has to
	SetActorTickEnabled(true);
}
