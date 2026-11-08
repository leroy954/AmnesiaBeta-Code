#include "AmnesiaWeather.h"
#include "AmnesiaAtmosphere.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAmnesiaWeather::AAmnesiaWeather()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Game/Amnesia/Materials/M_Weather.M_Weather"));

	// the drops are only visuals, so they skip collision, shadows and navigation
	auto SetupDrops = [this](UInstancedStaticMeshComponent* Mesh, UStaticMesh* Shape)
	{
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetStaticMesh(Shape);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetReceivesDecals(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->bAffectDistanceFieldLighting = false;
		Mesh->bNeverDistanceCull = true;
		Mesh->SetVisibility(false);
	};

	RainMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainMesh"));
	SetupDrops(RainMesh, CubeFinder.Object);

	SnowMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SnowMesh"));
	SetupDrops(SnowMesh, SphereFinder.Object);

	LightningLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightningLight"));
	LightningLight->SetupAttachment(RootComponent);
	LightningLight->SetMobility(EComponentMobility::Movable);
	LightningLight->IntensityUnits = ELightUnits::Candelas;
	LightningLight->SetIntensity(0.0f);
	LightningLight->SetAttenuationRadius(12000.0f);
	LightningLight->SetLightColor(FLinearColor(0.7f, 0.8f, 1.0f));
	LightningLight->SetCastShadows(false);
	LightningLight->SetVisibility(false);

	DropMaterial = MaterialFinder.Object;
}

void AAmnesiaWeather::BeginPlay()
{
	Super::BeginPlay();

	if (!Atmosphere)
	{
		for (TActorIterator<AAmnesiaAtmosphere> It(GetWorld()); It; ++It)
		{
			Atmosphere = *It;
			break;
		}
	}

	if (DropMaterial)
	{
		RainMaterial = UMaterialInstanceDynamic::Create(DropMaterial, this);
		RainMaterial->SetVectorParameterValue(TEXT("Color"), RainColor);
		RainMaterial->SetScalarParameterValue(TEXT("Opacity"), RainOpacity);
		RainMesh->SetMaterial(0, RainMaterial);

		SnowMaterial = UMaterialInstanceDynamic::Create(DropMaterial, this);
		SnowMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
		SnowMaterial->SetScalarParameterValue(TEXT("Opacity"), SnowOpacity);
		SnowMesh->SetMaterial(0, SnowMaterial);
	}

	const FVector Focus = GetFocusLocation();

	// create every drop once and recycle them
	ScatterDrops(RainDrops, MaxRainDrops, Focus);
	ScatterDrops(SnowFlakes, MaxSnowFlakes, Focus);

	TArray<FTransform> RainTransforms;
	RainTransforms.Init(FTransform(FQuat::Identity, Focus, FVector::ZeroVector), RainDrops.Num());
	RainMesh->AddInstances(RainTransforms, false, true);

	TArray<FTransform> SnowTransforms;
	SnowTransforms.Init(FTransform(FQuat::Identity, Focus, FVector::ZeroVector), SnowFlakes.Num());
	SnowMesh->AddInstances(SnowTransforms, false, true);

	LightningCountdown = FMath::FRandRange(MinLightningInterval, MaxLightningInterval);

	// rain and wind loops play all the time, silent until the weather needs them
	if (RainLoopSound)
	{
		RainAudio = UGameplayStatics::SpawnSound2D(this, RainLoopSound, 0.001f, 1.0f, 0.0f, nullptr, false, false);
	}

	if (WindLoopSound)
	{
		WindAudio = UGameplayStatics::SpawnSound2D(this, WindLoopSound, 0.001f, 1.0f, 0.0f, nullptr, false, false);
	}

	// the storm starts hidden and forms when the hero reaches it
	if (StormActor)
	{
		StormActor->CallFunctionByNameWithArguments(TEXT("HideStormImmediately"), *GLog, nullptr, true);
	}
}

void AAmnesiaWeather::SetAmbience(USoundBase* Sound, USoundBase* Layer, float LayerVolume)
{
	CrossfadeTo(AmbienceAudio, ActiveAmbience, Sound, AmbienceVolume);
	CrossfadeTo(LayerAudio, ActiveLayer, Layer, AmbienceVolume * LayerVolume);
}

void AAmnesiaWeather::CrossfadeTo(TObjectPtr<UAudioComponent> (&Players)[2], int32& Active, USoundBase* Sound, float Volume)
{
	UAudioComponent* Current = Players[Active];

	if (Current && Current->Sound == Sound && Current->IsPlaying())
	{
		return;
	}

	if (FAmnesiaSoundSet::bLogPlays)
	{
		UE_LOG(LogTemp, Log, TEXT("Sound: ambience fades to %s"), Sound ? *Sound->GetName() : TEXT("silence"));
	}

	// fade the old player out and the new one in
	if (Current)
	{
		Current->FadeOut(2.5f, 0.0f);
	}

	Active = 1 - Active;

	if (Players[Active])
	{
		Players[Active]->Stop();
		Players[Active] = nullptr;
	}

	if (Sound)
	{
		Players[Active] = UGameplayStatics::SpawnSound2D(this, Sound, 1.0f, 1.0f, 0.0f, nullptr, false, false);

		if (Players[Active])
		{
			Players[Active]->FadeIn(2.5f, Volume);
		}
	}
}

void AAmnesiaWeather::PlayThunder()
{
	ThunderSounds.Play2D(this, FMath::FRandRange(0.7f, 1.0f));
}

void AAmnesiaWeather::SetStormActive(bool bActive)
{
	if (!StormActor || bActive == bStormActive)
	{
		return;
	}

	bStormActive = bActive;

	// the storm plugin is called by name, so the game module doesn't depend on it
	// show the mature storm right away, so it's there when the hero looks up
	StormActor->CallFunctionByNameWithArguments(bActive ? TEXT("ShowMatureStorm") : TEXT("HideStormImmediately"), *GLog, nullptr, true);
}

FVector AAmnesiaWeather::GetFocusLocation() const
{
	// follow the camera horizontally and vertically, but stay on the side scrolling plane
	if (const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (PlayerController->PlayerCameraManager)
		{
			const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();

			return FVector(CameraLocation.X, 0.0f, CameraLocation.Z);
		}
	}

	return GetActorLocation();
}

void AAmnesiaWeather::ScatterDrops(TArray<FDrop>& Drops, int32 Count, const FVector& Focus)
{
	Drops.SetNum(Count);

	for (FDrop& Drop : Drops)
	{
		Drop.Position = Focus + FVector(FMath::FRandRange(-HalfWidth, HalfWidth), FMath::FRandRange(-BackDepth, FrontDepth), FMath::FRandRange(-HalfHeight, HalfHeight));
		Drop.SpeedFactor = FMath::FRandRange(0.8f, 1.25f);
		Drop.Phase = FMath::FRandRange(0.0f, UE_TWO_PI);
	}
}

void AAmnesiaWeather::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector Focus = GetFocusLocation();

	const float TargetRain = Atmosphere ? Atmosphere->GetRainIntensity() : 0.0f;
	const float TargetSnow = Atmosphere ? Atmosphere->GetSnowIntensity() : 0.0f;

	// fade the weather in and out
	ShownRain = FMath::FInterpTo(ShownRain, TargetRain, DeltaTime, 0.8f);
	ShownSnow = FMath::FInterpTo(ShownSnow, TargetSnow, DeltaTime, 0.8f);

	UpdateDrops(RainMesh, RainDrops, ShownRain, Focus, DeltaTime, RainSpeed, RainWind, RainScale, false);
	UpdateDrops(SnowMesh, SnowFlakes, ShownSnow, Focus, DeltaTime, SnowSpeed, SnowWind, FVector(SnowSize), true);

	UpdateLightning(DeltaTime, Focus);

	if (RainAudio)
	{
		RainAudio->SetVolumeMultiplier(FMath::Max(ShownRain, 0.001f));
	}

	if (WindAudio)
	{
		WindAudio->SetVolumeMultiplier(FMath::Max(ShownSnow * 0.7f + ShownRain * 0.3f, 0.001f));
	}
}

void AAmnesiaWeather::UpdateDrops(UInstancedStaticMeshComponent* Mesh, TArray<FDrop>& Drops, float Strength, const FVector& Focus, float DeltaTime, float Speed, float Wind, const FVector& Scale, bool bSway)
{
	// nothing to draw and nothing to update while it's dry
	if (Strength < 0.01f || Drops.Num() == 0)
	{
		Mesh->SetVisibility(false);
		return;
	}

	Mesh->SetVisibility(true);

	const float Time = GetWorld()->GetTimeSeconds();
	const int32 ActiveDrops = FMath::CeilToInt(Drops.Num() * FMath::Clamp(Strength, 0.0f, 1.0f));

	// streaks lean into the wind, flakes stay upright
	const FVector Velocity(Wind, 0.0f, -Speed);
	const FQuat StreakRotation = bSway ? FQuat::Identity : FRotationMatrix::MakeFromZ(-Velocity).ToQuat();

	TArray<FTransform> Transforms;
	Transforms.SetNumUninitialized(Drops.Num());

	for (int32 Index = 0; Index < Drops.Num(); ++Index)
	{
		FDrop& Drop = Drops[Index];

		Drop.Position.Z -= Speed * Drop.SpeedFactor * DeltaTime;
		Drop.Position.X += Wind * DeltaTime;

		if (bSway)
		{
			Drop.Position.X += FMath::Sin(Time * 1.4f + Drop.Phase) * 45.0f * DeltaTime;
		}

		// wrap around the box so the drops follow the camera
		if (Drop.Position.Z < Focus.Z - HalfHeight)
		{
			Drop.Position.Z += HalfHeight * 2.0f;
		}
		else if (Drop.Position.Z > Focus.Z + HalfHeight)
		{
			Drop.Position.Z -= HalfHeight * 2.0f;
		}

		if (Drop.Position.X < Focus.X - HalfWidth)
		{
			Drop.Position.X += HalfWidth * 2.0f;
		}
		else if (Drop.Position.X > Focus.X + HalfWidth)
		{
			Drop.Position.X -= HalfWidth * 2.0f;
		}

		// drops further back are drawn bigger, so they don't vanish in the distance
		const float DepthBoost = 1.0f + FMath::Clamp(-Drop.Position.Y / BackDepth, 0.0f, 1.0f) * 1.2f;

		Transforms[Index] = FTransform(StreakRotation, Drop.Position, Index < ActiveDrops ? Scale * DepthBoost : FVector::ZeroVector);
	}

	Mesh->BatchUpdateInstancesTransforms(0, Transforms, true, true, true);
}

void AAmnesiaWeather::UpdateLightning(float DeltaTime, const FVector& Focus)
{
	// only strike during heavy rain
	if (ShownRain >= LightningRainThreshold && LightningTime < 0.0f)
	{
		LightningCountdown -= DeltaTime;

		if (LightningCountdown <= 0.0f)
		{
			LightningTime = 0.0f;
			LightningCountdown = FMath::FRandRange(MinLightningInterval, MaxLightningInterval);

			// light up the storm clouds too
			if (StormActor && bStormActive)
			{
				StormActor->CallFunctionByNameWithArguments(TEXT("PlayLightningSequence"), *GLog, nullptr, true);
			}

			// thunder follows the flash
			GetWorld()->GetTimerManager().SetTimer(ThunderTimer, this, &AAmnesiaWeather::PlayThunder, FMath::FRandRange(0.5f, 1.8f), false);

			// strike somewhere behind the hero
			LightningLight->SetWorldLocation(Focus + FVector(FMath::FRandRange(-1500.0f, 1500.0f), -1800.0f, 1800.0f));
		}
	}

	if (LightningTime < 0.0f)
	{
		return;
	}

	LightningTime += DeltaTime;

	// a sharp first flash followed by a weaker second one
	const auto Pulse = [this](float Center, float Width)
	{
		return FMath::Exp(-FMath::Square((LightningTime - Center) / Width));
	};

	const float Flash = FMath::Max(Pulse(0.06f, 0.05f), 0.55f * Pulse(0.26f, 0.08f));

	LightningLight->SetVisibility(Flash > 0.01f);
	LightningLight->SetIntensity(LightningIntensity * Flash);

	if (LightningTime > 0.8f)
	{
		LightningTime = -1.0f;
		LightningLight->SetIntensity(0.0f);
		LightningLight->SetVisibility(false);
	}
}
