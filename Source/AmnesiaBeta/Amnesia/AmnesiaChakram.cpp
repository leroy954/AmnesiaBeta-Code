#include "AmnesiaChakram.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaEnemy.h"
#include "AmnesiaSkills.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AAmnesiaChakram::AAmnesiaChakram()
{
	PrimaryActorTick.bCanEverTick = true;

	// the root spins around the axis of the camera, so the disc shows its face while it turns
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Spin"));

	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	Disc->SetupAttachment(RootComponent);
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetCastShadow(false);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(40.0f);
	Glow->SetAttenuationRadius(320.0f);
	Glow->SetCastShadows(false);
	Glow->SetLightingChannels(true, true, false);
}

void AAmnesiaChakram::Throw(AAmnesiaCharacter* InHero, UStaticMesh* Mesh, const FVector& Start, float InFacing, const FAmnesiaChakramThrow& InThrow, const FLinearColor& Color)
{
	Hero = InHero;
	Settings = InThrow;
	Facing = InFacing >= 0.0f ? 1.0f : -1.0f;
	BouncesLeft = Settings.Bounces;
	Phase = EPhase::Out;
	SetActorLocation(Start);

	if (Mesh)
	{
		Disc->SetStaticMesh(Mesh);

		// the flat side of the model faces the camera: its thinnest axis is turned onto the world Y axis, and the middle of the model sits on the spin axis
		const FBox Box = Mesh->GetBoundingBox();
		const FVector Size = Box.GetSize();
		FRotator Turn = FRotator::ZeroRotator;

		if (Size.X <= Size.Y && Size.X <= Size.Z)
		{
			Turn = FRotator(0.0f, 90.0f, 0.0f);

		} else if (Size.Z <= Size.X && Size.Z <= Size.Y) {

			Turn = FRotator(0.0f, 0.0f, 90.0f);
		}

		Disc->SetRelativeRotation(Turn);
		DiscOffset = -Turn.RotateVector(Box.GetCenter());

		if (InHero)
		{
			FlightScale = InHero->GetWeaponPropScale() * FlightToHand;
		}

		SetDiscSize(FlightScale);
	}

	Glow->SetLightColor(Color);
}

void AAmnesiaChakram::Recall()
{
	if (Phase != EPhase::Back)
	{
		StartBack();
	}
}

void AAmnesiaChakram::SetDiscSize(float Scale)
{
	Disc->SetRelativeScale3D(FVector(Scale));
	Disc->SetRelativeLocation(DiscOffset * Scale);
}

void AAmnesiaChakram::StartBack()
{
	// out of a whirl: back to the flying size and light
	if (Phase == EPhase::Whirl)
	{
		SetDiscSize(FlightScale);
		Glow->SetIntensity(40.0f);
	}

	Phase = EPhase::Back;
	BounceTarget = nullptr;
}

void AAmnesiaChakram::StartWhirl()
{
	Phase = EPhase::Whirl;
	WhirlLeft = Settings.WhirlTime;
	NextWhirlHit = 0.0f;

	// it grows, shines, and throws a cold wave to both sides
	SetDiscSize(FlightScale * WhirlGrow);
	Glow->SetIntensity(130.0f);

	if (const AAmnesiaCharacter* Thrower = Hero.Get())
	{
		if (UAmnesiaSkillComponent* Skills = Thrower->GetSkills())
		{
			Skills->PlayWaveStyled(GetActorLocation() - FVector(0.0f, 0.0f, 60.0f), 1.0f, 20.0f, WhirlRadius, 3, 0.8f, EAmnesiaFxStyle::Cold);
			Skills->PlayWaveStyled(GetActorLocation() - FVector(0.0f, 0.0f, 60.0f), -1.0f, 20.0f, WhirlRadius, 3, 0.8f, EAmnesiaFxStyle::Cold);
		}
	}
}

FVector AAmnesiaChakram::HandLocation() const
{
	const AAmnesiaCharacter* Thrower = Hero.Get();

	if (!Thrower)
	{
		return GetActorLocation();
	}

	return Thrower->GetMesh()->DoesSocketExist(FName("hand_r")) ? Thrower->GetMesh()->GetSocketLocation(FName("hand_r")) : Thrower->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
}

AAmnesiaEnemy* AAmnesiaChakram::NextBounceTarget() const
{
	AAmnesiaEnemy* Best = nullptr;
	float BestDistance = BounceReach;

	for (TActorIterator<AAmnesiaEnemy> It(GetWorld()); It; ++It)
	{
		AAmnesiaEnemy* Enemy = *It;

		if (Enemy->GetHealth() <= 0.0f || Enemy->IsDormant() || HitOut.Contains(Enemy))
		{
			continue;
		}

		const float Distance = FVector::Dist(Enemy->GetActorLocation(), GetActorLocation());

		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Enemy;
		}
	}

	return Best;
}

void AAmnesiaChakram::HitAround(float Radius, float Damage, TSet<TWeakObjectPtr<AActor>>* Once)
{
	AAmnesiaCharacter* Thrower = Hero.Get();

	if (!Thrower)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Thrower);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius), QueryParams);

	TSet<AActor*> ThisTime;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AAmnesiaEnemy* Enemy = Cast<AAmnesiaEnemy>(Overlap.GetActor());

		if (!Enemy || ThisTime.Contains(Enemy) || Enemy->GetHealth() <= 0.0f || Enemy->IsDormant() || (Once && Once->Contains(Enemy)))
		{
			continue;
		}

		ThisTime.Add(Enemy);

		if (Once)
		{
			Once->Add(Enemy);
		}

		Thrower->OnChakramHit(Enemy, Damage, GetActorLocation(), Phase == EPhase::Back ? -Facing : Facing);

		// a bounce: from the first enemy on to the next one
		if (Phase == EPhase::Out && BouncesLeft > 0)
		{
			if (AAmnesiaEnemy* Next = NextBounceTarget())
			{
				BounceTarget = Next;
				BouncesLeft--;

			} else if (BounceTarget.IsValid()) {

				StartBack();
			}

		} else if (Phase == EPhase::Out && BounceTarget.IsValid()) {

			// the last jump landed
			StartBack();
		}
	}
}

void AAmnesiaChakram::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	AAmnesiaCharacter* Thrower = Hero.Get();

	// the hero is gone, or the disc got lost somewhere: it is back in the hand
	if (!Thrower || Age > 8.0f)
	{
		if (Thrower)
		{
			Thrower->OnChakramCaught(this);
		}

		Destroy();
		return;
	}

	// the spin, faster while it whirls
	const float Spin = SpinRate * 360.0f * DeltaSeconds * (Phase == EPhase::Whirl ? WhirlSpin : 1.0f);
	AddActorWorldRotation(FRotator(-Facing * Spin, 0.0f, 0.0f));

	const FVector Here = GetActorLocation();

	switch (Phase)
	{
	case EPhase::Out:
	{
		FVector Next;

		if (AAmnesiaEnemy* Target = BounceTarget.Get())
		{
			if (Target->GetHealth() <= 0.0f)
			{
				StartBack();
				break;
			}

			// on to the next enemy of the bounce
			const FVector Aim = Target->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
			Next = Here + (Aim - Here).GetClampedToMaxSize(Settings.Speed * DeltaSeconds);

		} else {

			Next = Here + FVector(Facing * Settings.Speed * DeltaSeconds, 0.0f, 0.0f);
			Travelled += Settings.Speed * DeltaSeconds;
		}

		// a wall stops the straight flight. A bounce goes on to its enemy whatever is in the way, or it would stick in a rock
		FHitResult Wall;
		FCollisionQueryParams WallParams;
		WallParams.AddIgnoredActor(this);
		WallParams.AddIgnoredActor(Thrower);

		if (!BounceTarget.IsValid() && GetWorld()->LineTraceSingleByChannel(Wall, Here, Next, ECC_WorldStatic, WallParams))
		{
			SetActorLocation(Wall.Location - (Next - Here).GetSafeNormal() * 20.0f);
			Travelled = Settings.Range;

		} else {

			SetActorLocation(Next);
		}

		HitAround(HitRadius, Settings.Damage, &HitOut);

		if (Phase == EPhase::Out && !BounceTarget.IsValid() && Travelled >= Settings.Range)
		{
			if (Settings.WhirlTime > 0.0f)
			{
				StartWhirl();

			} else {

				StartBack();
			}
		}

		break;
	}

	case EPhase::Whirl:
	{
		WhirlLeft -= DeltaSeconds;
		NextWhirlHit -= DeltaSeconds;

		if (NextWhirlHit <= 0.0f)
		{
			NextWhirlHit = WhirlHitInterval;
			HitAround(WhirlRadius, Settings.WhirlDamage, nullptr);

			// a flash goes round the disc with every hit of the whirl
			WhirlAngle += 110.0f;
			const float Radians = FMath::DegreesToRadians(WhirlAngle);

			if (UAmnesiaSkillComponent* Skills = Thrower->GetSkills())
			{
				Skills->PlayFlashStyled(Here + FVector(FMath::Cos(Radians), 0.0f, FMath::Sin(Radians)) * WhirlRadius * 0.55f, EAmnesiaFxStyle::Cold);
			}
		}

		if (WhirlLeft <= 0.0f)
		{
			StartBack();
		}

		break;
	}

	case EPhase::Back:
	{
		// home to the hand, a little faster than it went out
		const FVector Hand = HandLocation();
		const FVector ToHand = Hand - Here;

		if (ToHand.Size() <= CatchDistance)
		{
			Thrower->OnChakramCaught(this);
			Destroy();
			return;
		}

		SetActorLocation(Here + ToHand.GetClampedToMaxSize(Settings.Speed * 1.15f * DeltaSeconds));
		HitAround(HitRadius, Settings.Damage, &HitBack);
		break;
	}
	}
}
