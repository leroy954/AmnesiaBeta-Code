#include "AmnesiaFireball.h"
#include "AmnesiaEnemy.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaSkills.h"
#include "CombatDamageable.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AAmnesiaFireball::AAmnesiaFireball()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem> FireFinder(TEXT("/Game/GreenwoodFantasyVillage/Particles/PS_Fire_warm.PS_Fire_warm"));

	// overlap enemies, stop at the ground and walls
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetSphereRadius(24.0f);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Sphere->SetGenerateOverlapEvents(true);
	RootComponent = Sphere;

	auto SetupMesh = [&](UStaticMeshComponent* Component, float Scale)
	{
		Component->SetupAttachment(RootComponent);
		Component->SetStaticMesh(SphereFinder.Object);
		Component->SetRelativeScale3D(FVector(Scale));
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
	};

	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
	SetupMesh(Core, 0.26f);

	Flame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flame"));
	SetupMesh(Flame, 0.52f);

	Fire = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Fire"));
	Fire->SetupAttachment(RootComponent);
	Fire->SetTemplate(FireFinder.Object);
	Fire->SetRelativeScale3D(FVector(0.45f));
	Fire->SetRelativeLocation(FVector(0.0f, 0.0f, -22.0f));

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SparksFinder(TEXT("/Game/Variant_Combat/VFX/NS_Damage.NS_Damage"));
	BurstEffect = SparksFinder.Object;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);
	Light->IntensityUnits = ELightUnits::Candelas;
	Light->SetIntensity(45.0f);
	Light->SetAttenuationRadius(520.0f);
	Light->SetLightColor(FLinearColor(1.0f, 0.5f, 0.15f));
	Light->SetCastShadows(false);

	for (int32 Index = 1; Index <= 2; ++Index)
	{
		const FString Path = FString::Printf(TEXT("/Game/Audio/Companion/SW_Fireball_Hit_%02d.SW_Fireball_Hit_%02d"), Index, Index);
		ConstructorHelpers::FObjectFinder<USoundBase> Finder(*Path);

		if (Finder.Succeeded())
		{
			ImpactSounds.Sounds.Add(Finder.Object);
		}
	}

	ImpactSounds.Volume = 0.75f;
	ImpactSounds.PitchMin = 0.95f;
	ImpactSounds.PitchMax = 1.15f;
	ImpactSounds.Range = 3500.0f;
}

void AAmnesiaFireball::BeginPlay()
{
	Super::BeginPlay();

	SpawnTime = GetWorld()->GetTimeSeconds();
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AAmnesiaFireball::OnSphereOverlap);

	// the status overlay material glows at the rim and is see-through in the middle: a ball of fire and a halo around it, not flat discs
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Amnesia/Materials/M_StatusOverlay.M_StatusOverlay")))
	{
		CoreMaterial = UMaterialInstanceDynamic::Create(Base, this);
		CoreMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(9.0f, 4.5f, 1.2f));
		CoreMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.95f);
		CoreMaterial->SetScalarParameterValue(TEXT("RimBoost"), 1.5f);
		CoreMaterial->SetScalarParameterValue(TEXT("RimPower"), 2.0f);
		Core->SetMaterial(0, CoreMaterial);

		FlameMaterial = UMaterialInstanceDynamic::Create(Base, this);
		FlameMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(4.0f, 1.1f, 0.15f));
		FlameMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.4f);
		FlameMaterial->SetScalarParameterValue(TEXT("RimBoost"), 2.5f);
		FlameMaterial->SetScalarParameterValue(TEXT("RimPower"), 1.6f);
		Flame->SetMaterial(0, FlameMaterial);
	}
}

void AAmnesiaFireball::Launch(AActor* InTarget, AActor* InHero, float InDamage)
{
	Target = InTarget;
	Hero = InHero;
	Damage = InDamage;

	// an ice sword makes the dragon spit blue fire
	if (const AAmnesiaCharacter* HeroCharacter = Cast<AAmnesiaCharacter>(InHero))
	{
		if (HeroCharacter->GetSkills() && HeroCharacter->GetSkills()->GetElement() == EAmnesiaElement::Ice)
		{
			if (CoreMaterial)
			{
				CoreMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.5f, 4.5f, 9.0f));
			}

			if (FlameMaterial)
			{
				FlameMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.15f, 1.1f, 4.0f));
			}

			Light->SetLightColor(FLinearColor(0.25f, 0.55f, 1.0f));
			Fire->SetVisibility(false);
		}
	}

	const FVector Aim = InTarget ? InTarget->GetActorLocation() - GetActorLocation() : GetActorForwardVector();
	Velocity = Aim.GetSafeNormal() * Speed;
}

void AAmnesiaFireball::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Now = GetWorld()->GetTimeSeconds();

	// after the burst: a short flash, flames licking up for a moment, then the embers fade
	if (BurstTime >= 0.0f)
	{
		const float Since = Now - BurstTime;

		if (Since >= 0.9f)
		{
			Destroy();
			return;
		}

		const float Flash = FMath::Clamp(Since / 0.25f, 0.0f, 1.0f);
		Flame->SetRelativeScale3D(FVector(FMath::Lerp(0.6f, 1.5f, FMath::Sqrt(Flash))));

		if (FlameMaterial)
		{
			FlameMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.55f * FMath::Square(1.0f - Flash));
		}

		if (Since > 0.3f && Fire->IsActive())
		{
			Fire->Deactivate();
		}

		Light->SetIntensity(90.0f * FMath::Square(1.0f - Since / 0.9f));
		return;
	}

	// steer after the target while it lives, aiming at its middle
	if (AAmnesiaEnemy* Enemy = Cast<AAmnesiaEnemy>(Target.Get()))
	{
		if (Enemy->GetHealth() > 0.0f)
		{
			const FVector Wanted = (Enemy->GetActorLocation() - GetActorLocation()).GetSafeNormal() * Speed;
			Velocity = FMath::VInterpTo(Velocity, Wanted, DeltaTime, Homing);
			Velocity = Velocity.GetSafeNormal() * Speed;
		}
	}

	// a living flame: flicker the light and wobble the flame
	const float Flicker = 1.0f + 0.2f * FMath::Sin(Now * 31.0f) + 0.12f * FMath::Sin(Now * 53.0f + 1.0f);
	Light->SetIntensity(35.0f * Flicker);
	Flame->SetRelativeScale3D(FVector(0.52f * (1.0f + 0.08f * FMath::Sin(Now * 40.0f))));

	FHitResult Hit;
	SetActorLocation(GetActorLocation() + Velocity * DeltaTime, true, &Hit);

	if (Hit.bBlockingHit)
	{
		Burst(Hit.ImpactPoint);
		return;
	}

	if (Now - SpawnTime > LifeTime)
	{
		Burst(GetActorLocation());
	}
}

void AAmnesiaFireball::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (BurstTime < 0.0f && TryHit(OtherActor, GetActorLocation()))
	{
		Burst(GetActorLocation());
	}
}

bool AAmnesiaFireball::TryHit(AActor* Actor, const FVector& Location)
{
	AAmnesiaEnemy* Enemy = Cast<AAmnesiaEnemy>(Actor);

	// only living enemies; the hero and the dragon fly through it
	if (!Enemy || Enemy->GetHealth() <= 0.0f)
	{
		return false;
	}

	if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(Enemy))
	{
		const float Push = Velocity.X >= 0.0f ? Knockback : -Knockback;
		Damageable->ApplyDamage(Damage, Hero.Get(), Location, FVector(Push, 0.0f, Knockback * 0.5f));

		// the fire carries the element of the hero's sword
		if (AAmnesiaCharacter* HeroCharacter = Cast<AAmnesiaCharacter>(Hero.Get()))
		{
			HeroCharacter->GetSkills()->OnHeroHit(Enemy, Damage, true);
		}
	}

	return true;
}

void AAmnesiaFireball::Burst(const FVector& Location)
{
	BurstTime = GetWorld()->GetTimeSeconds();
	SetActorLocation(Location);

	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Core->SetVisibility(false);

	// the flames flare up where it hit
	Fire->SetRelativeScale3D(FVector(0.8f));

	if (BurstEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BurstEffect, Location);
	}

	ImpactSounds.PlayAt(this, Location);
}
