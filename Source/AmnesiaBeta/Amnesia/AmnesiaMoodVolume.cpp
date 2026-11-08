#include "AmnesiaMoodVolume.h"
#include "AmnesiaAtmosphere.h"
#include "AmnesiaWeather.h"
#include "AmnesiaCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AAmnesiaMoodVolume::AAmnesiaMoodVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the trigger box
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;

	Box->SetBoxExtent(FVector(400.0f, 400.0f, 400.0f));
	Box->SetCollisionProfileName(FName("Trigger"));
}

void AAmnesiaMoodVolume::BeginPlay()
{
	Super::BeginPlay();

	Box->OnComponentBeginOverlap.AddDynamic(this, &AAmnesiaMoodVolume::OnBoxBeginOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &AAmnesiaMoodVolume::OnBoxEndOverlap);
}

void AAmnesiaMoodVolume::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// only react to the player
	if (!OtherActor || !OtherActor->ActorHasTag(FName("Player")))
	{
		return;
	}

	if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(OtherActor))
	{
		if (bSetCameraPitch)
		{
			Hero->SetCameraPitch(CameraPitch, 1.2f);
		}

		// snow crunches, caves clack
		Hero->SetZoneFootsteps(FootstepSounds);
	}

	if (AAmnesiaWeather* Weather = Cast<AAmnesiaWeather>(UGameplayStatics::GetActorOfClass(this, AAmnesiaWeather::StaticClass())))
	{
		if (StormControl != 0)
		{
			Weather->SetStormActive(StormControl > 0);
		}

		// zones without an ambience fade the last one out
		Weather->SetAmbience(AmbientSound, AmbientLayerSound, AmbientLayerVolume);
	}

	if (MoodsOnEnter.Num() == 0)
	{
		return;
	}

	if (AAmnesiaAtmosphere* CurrentAtmosphere = GetAtmosphere())
	{
		CurrentAtmosphere->SetCycle(MoodsOnEnter, BlendTime);
	}
}

void AAmnesiaMoodVolume::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// only react to the player
	if (!OtherActor || !OtherActor->ActorHasTag(FName("Player")) || MoodsOnExit.Num() == 0)
	{
		return;
	}

	if (AAmnesiaAtmosphere* CurrentAtmosphere = GetAtmosphere())
	{
		CurrentAtmosphere->SetCycle(MoodsOnExit, BlendTime);
	}
}

AAmnesiaAtmosphere* AAmnesiaMoodVolume::GetAtmosphere()
{
	if (!Atmosphere)
	{
		Atmosphere = Cast<AAmnesiaAtmosphere>(UGameplayStatics::GetActorOfClass(this, AAmnesiaAtmosphere::StaticClass()));
	}

	return Atmosphere;
}
