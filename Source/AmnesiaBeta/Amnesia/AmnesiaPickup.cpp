#include "AmnesiaPickup.h"
#include "AmnesiaCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAmnesiaPickup::AAmnesiaPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetSphereRadius(60.0f);
	Sphere->SetCollisionProfileName(FName("OverlapAllDynamic"));
	RootComponent = Sphere;

	auto SetupMesh = [&](UStaticMeshComponent* Component, float Scale)
	{
		Component->SetupAttachment(RootComponent);
		Component->SetStaticMesh(SphereFinder.Object);
		Component->SetRelativeScale3D(FVector(Scale));
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
	};

	Orb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Orb"));
	SetupMesh(Orb, 0.22f);

	Halo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Halo"));
	SetupMesh(Halo, 0.42f);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->IntensityUnits = ELightUnits::Candelas;
	Glow->SetIntensity(60.0f);
	Glow->SetAttenuationRadius(450.0f);
	Glow->SetCastShadows(false);
}

void AAmnesiaPickup::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();
	SpawnTime = GetWorld()->GetTimeSeconds();
	Glow->SetLightColor(OrbColor);

	// the weather drop material is an unlit translucent color, perfect for a glowing orb
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Amnesia/Materials/M_Weather.M_Weather")))
	{
		OrbMaterial = UMaterialInstanceDynamic::Create(Base, this);
		OrbMaterial->SetVectorParameterValue(TEXT("Color"), OrbColor * 4.0f);
		OrbMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.95f);
		Orb->SetMaterial(0, OrbMaterial);

		HaloMaterial = UMaterialInstanceDynamic::Create(Base, this);
		HaloMaterial->SetVectorParameterValue(TEXT("Color"), OrbColor * 1.5f);
		HaloMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.25f);
		Halo->SetMaterial(0, HaloMaterial);
	}
}

void AAmnesiaPickup::Pop(const FVector& Velocity)
{
	PopVelocity = Velocity;
}

void AAmnesiaPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bCollected)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	// far from the hero nobody sees the orb: check a few times a second and don't move it (every move updates overlaps).
	// A level has many orbs
	if (PopVelocity.IsNearlyZero())
	{
		if (const APawn* Watcher = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const bool bFar = FMath::Abs(Watcher->GetActorLocation().X - GetActorLocation().X) > 4500.0f;
			SetActorTickInterval(bFar ? 0.3f : 0.0f);

			if (bFar)
			{
				return;
			}
		}
	}

	// fly out of the enemy and settle a little above the ground
	if (!PopVelocity.IsNearlyZero())
	{
		HomeLocation += PopVelocity * DeltaTime;
		PopVelocity.Z -= 1400.0f * DeltaTime;
		PopVelocity.X *= FMath::Max(0.0f, 1.0f - 2.0f * DeltaTime);

		// stop on the ground below the starting point
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (PopVelocity.Z < 0.0f && GetWorld()->LineTraceSingleByChannel(Hit, HomeLocation + FVector(0, 0, 50), HomeLocation - FVector(0, 0, 60), ECC_Visibility, Params))
		{
			HomeLocation.Z = Hit.ImpactPoint.Z + 60.0f;
			PopVelocity = FVector::ZeroVector;
		}
	}

	// bob and pulse
	const float Bob = FMath::Sin(Now * 3.0f + HomeLocation.X * 0.01f) * 12.0f;
	FVector Target = HomeLocation + FVector(0.0f, 0.0f, Bob);

	const float Pulse = 0.85f + 0.15f * FMath::Sin(Now * 5.0f);
	Halo->SetRelativeScale3D(FVector(0.42f * Pulse));

	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));

	if (Hero && Hero->GetHealth() > 0.0f && Now - SpawnTime > 0.6f)
	{
		const FVector HeroCenter = Hero->GetActorLocation();
		const float Distance = FVector::Dist(HeroCenter, GetActorLocation());

		// drift to the hero once close
		if (Distance < MagnetRange)
		{
			const float Speed = FMath::Lerp(1400.0f, 300.0f, Distance / MagnetRange);
			Target = GetActorLocation() + (HeroCenter - GetActorLocation()).GetSafeNormal() * FMath::Min(Speed * DeltaTime, Distance);
			HomeLocation = Target;
		}

		if (Distance < 70.0f)
		{
			Collect(Hero);
			return;
		}
	}

	SetActorLocation(Target);
}

void AAmnesiaPickup::Collect(AAmnesiaCharacter* Hero)
{
	bCollected = true;

	Hero->HealBy(Hero->GetMaxHealth() * HealFraction);

	PickupSounds.PlayAt(this, GetActorLocation());

	Destroy();
}
