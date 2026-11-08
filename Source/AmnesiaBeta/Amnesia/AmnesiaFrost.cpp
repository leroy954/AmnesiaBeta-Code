#include "AmnesiaFrost.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"

namespace
{
	/** The crystals: a tall spike, a big one and a cluster of thin ones of the cave pack, with a plain cone when the pack is not there. Height is the
	 *  height of the mesh at scale 1, Base how far its foot is below its pivot, and Flip turns it upside down (the icicles hang down in the pack) */
	struct FShape
	{
		const TCHAR* Path;
		float Height;
		float Base;
		bool bFlip;
	};

	const FShape Shapes[] = {
		{ TEXT("/Game/MysteryCave/Meshes/SM_gr_StalagmitesGroup_9.SM_gr_StalagmitesGroup_9"), 412.0f, 0.0f, false },
		{ TEXT("/Game/MysteryCave/Meshes/SM_gr_StalagmitesGroup_2.SM_gr_StalagmitesGroup_2"), 1026.0f, 85.0f, false },
		{ TEXT("/Game/MysteryCave/Meshes/SM_gr_StalactiteThinG_1.SM_gr_StalactiteThinG_1"), 347.0f, 10.0f, true }
	};

	/** Where the crystals stand around the body: sideways and in depth as a part of the body's radius, the height as a part of the body's height, the tilt
	 *  outwards in degrees, and which shape. The camera looks from the +Y side, so few of them stand in front */
	struct FSpot
	{
		float X;
		float Y;
		float Height;
		float Tilt;
		int32 Shape;
	};

	const FSpot Spots[] = {
		{ -1.00f, 0.15f, 1.00f, 16.0f, 0 },
		{ 1.00f, -0.10f, 0.95f, 16.0f, 0 },
		{ -0.60f, -0.50f, 0.80f, 10.0f, 1 },
		{ 0.60f, 0.35f, 0.70f, 14.0f, 2 },
		{ -1.55f, -0.20f, 0.55f, 30.0f, 0 },
		{ 1.55f, 0.25f, 0.50f, 30.0f, 2 },
		{ 0.05f, -0.60f, 0.65f, 8.0f, 0 },
		{ -0.30f, 0.45f, 0.42f, 12.0f, 2 },
		{ 0.35f, -0.55f, 0.40f, 12.0f, 0 }
	};
}

AAmnesiaFrost::AAmnesiaFrost()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	SetCanBeDamaged(false);
}

void AAmnesiaFrost::PlaySoundAt(const TSoftObjectPtr<USoundBase>& Sound, float Volume)
{
	if (USoundBase* Loaded = Sound.IsNull() ? nullptr : Sound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Loaded, GetActorLocation(), Volume, FMath::FRandRange(0.94f, 1.06f));
	}
}

void AAmnesiaFrost::Grow(AActor* Target, float Radius, float HalfHeight)
{
	if (Target)
	{
		AttachToActor(Target, FAttachmentTransformRules::KeepWorldTransform);
	}

	// the crystals look like glass lit from behind: the status overlay material, an icy color, bright at the rim
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Amnesia/Materials/M_StatusOverlay.M_StatusOverlay")))
	{
		Crystal = UMaterialInstanceDynamic::Create(Base, this);
		Crystal->SetVectorParameterValue(TEXT("Color"), CrystalColor);
		Crystal->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
		Crystal->SetScalarParameterValue(TEXT("RimBoost"), CrystalRimBoost);
		Crystal->SetScalarParameterValue(TEXT("RimPower"), CrystalRimPower);
	}

	UStaticMesh* Loaded[UE_ARRAY_COUNT(Shapes)] = {};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Shapes); ++Index)
	{
		Loaded[Index] = LoadObject<UStaticMesh>(nullptr, Shapes[Index].Path);
	}

	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));

	const float BodyHeight = HalfHeight * 2.0f;
	FRandomStream Random(FMath::Rand());

	for (const FSpot& Spot : Spots)
	{
		const FShape& Shape = Shapes[Spot.Shape];
		UStaticMesh* Mesh = Loaded[Spot.Shape];
		float MeshHeight = Shape.Height;
		float FootBelowPivot = Shape.Base;
		bool bFlip = Shape.bFlip;

		// no cave pack: a cone, 100 tall with its pivot in the middle
		if (!Mesh)
		{
			Mesh = Cone;
			MeshHeight = 100.0f;
			FootBelowPivot = 50.0f;
			bFlip = false;
		}

		if (!Mesh)
		{
			continue;
		}

		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
		Piece->SetStaticMesh(Mesh);
		Piece->SetMobility(EComponentMobility::Movable);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCastShadow(false);
		Piece->bReceivesDecals = false;
		Piece->SetupAttachment(GetRootComponent());

		if (Crystal)
		{
			for (int32 Slot = 0; Slot < Piece->GetNumMaterials(); ++Slot)
			{
				Piece->SetMaterial(Slot, Crystal);
			}
		}

		Piece->RegisterComponent();

		// the spike leans away from the body, and turns around its own axis so the same shape does not look the same twice
		const float Side = Spot.X >= 0.0f ? 1.0f : -1.0f;
		const FQuat Lean(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(Side * Spot.Tilt));
		const FQuat Twist(FVector::UpVector, FMath::DegreesToRadians(Random.FRandRange(0.0f, 360.0f)));
		const FQuat Flip = bFlip ? FQuat(FVector(1.0f, 0.0f, 0.0f), UE_PI) : FQuat::Identity;

		FPiece Data;
		Data.Rotation = Lean * Twist * Flip;
		Data.FullScale = FMath::Max(BodyHeight * Spot.Height * Random.FRandRange(0.9f, 1.1f) / MeshHeight, 0.05f);
		Data.Delay = Random.FRandRange(0.0f, 0.12f);
		Data.BaseLocation = FVector(Spot.X * Radius * 1.3f, Spot.Y * Radius, -HalfHeight + FootBelowPivot * Data.FullScale);

		Piece->SetRelativeLocationAndRotation(Data.BaseLocation, Data.Rotation);
		Piece->SetRelativeScale3D(FVector(0.01f));

		Meshes.Add(Piece);
		Pieces.Add(Data);
	}

	PlaySoundAt(FreezeSound, 0.9f);
}

void AAmnesiaFrost::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShattering)
	{
		ShatterAge += DeltaTime;

		for (int32 Index = 0; Index < Meshes.Num(); ++Index)
		{
			UStaticMeshComponent* Piece = Meshes[Index];

			if (!Piece)
			{
				continue;
			}

			FPiece& Data = Pieces[Index];
			Data.Velocity.Z -= 980.0f * DeltaTime;

			Piece->AddWorldOffset(Data.Velocity * DeltaTime);
			Piece->AddLocalRotation(FRotator(Data.Spin.X, Data.Spin.Y, Data.Spin.Z) * DeltaTime);

			// the pieces shrink away in the second half of their flight
			const float Left = FMath::Clamp((ShatterTime - ShatterAge) / (ShatterTime * 0.5f), 0.0f, 1.0f);
			Piece->SetRelativeScale3D(Data.FlyScale * Left);
		}

		if (ShatterAge >= ShatterTime)
		{
			Destroy();
		}

		return;
	}

	Age += DeltaTime;

	for (int32 Index = 0; Index < Meshes.Num(); ++Index)
	{
		UStaticMeshComponent* Piece = Meshes[Index];

		if (!Piece)
		{
			continue;
		}

		const FPiece& Data = Pieces[Index];

		// up fast and slowing down, wider as it goes
		const float T = FMath::Clamp((Age - Data.Delay) / GrowTime, 0.0f, 1.0f);
		const float Eased = 1.0f - FMath::Pow(1.0f - T, 3.0f);
		const float Up = FMath::Max(Eased, 0.01f) * Data.FullScale;
		const float Wide = FMath::Max(0.55f + 0.45f * Eased, 0.01f) * Data.FullScale;

		Piece->SetRelativeScale3D(FVector(Wide, Wide, Up));
	}

	// the glass glints
	if (Crystal)
	{
		Crystal->SetScalarParameterValue(TEXT("RimBoost"), CrystalRimBoost * (1.0f + 0.15f * FMath::Sin(Age * 5.0f)));
	}
}

void AAmnesiaFrost::Shatter()
{
	if (bShattering)
	{
		return;
	}

	bShattering = true;
	ShatterAge = 0.0f;

	const FVector Center = GetActorLocation();
	FRandomStream Random(FMath::Rand());

	for (int32 Index = 0; Index < Meshes.Num(); ++Index)
	{
		UStaticMeshComponent* Piece = Meshes[Index];

		if (!Piece)
		{
			continue;
		}

		FPiece& Data = Pieces[Index];

		// they fly out from the middle, and up
		FVector Out = Piece->GetComponentLocation() - Center;
		Out.Y *= 0.4f;
		Out.Z = 0.0f;
		Out = Out.GetSafeNormal();

		if (Out.IsNearlyZero())
		{
			Out = FVector(Random.FRandRange(-1.0f, 1.0f), 0.0f, 0.0f);
		}

		Data.Velocity = Out * Random.FRandRange(260.0f, 560.0f) + FVector(0.0f, Random.FRandRange(-120.0f, 120.0f), Random.FRandRange(260.0f, 560.0f));
		Data.Spin = FVector(Random.FRandRange(-480.0f, 480.0f), Random.FRandRange(-480.0f, 480.0f), Random.FRandRange(-480.0f, 480.0f));
		Data.FlyScale = Piece->GetRelativeScale3D() * 0.6f;

		// the actor keeps following the enemy, but the pieces are on their own
		Piece->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	PlaySoundAt(ShatterSound, 1.0f);
}

void AAmnesiaFrost::Melt()
{
	Destroy();
}
