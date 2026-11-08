#include "AmnesiaCheckpoint.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaHUD.h"
#include "AmnesiaProgress.h"
#include "CombatPlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AAmnesiaCheckpoint::AAmnesiaCheckpoint()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BowlFinder(TEXT("/Game/GreenwoodFantasyVillage/Meshes/SM_LightColumn_FireBowl.SM_LightColumn_FireBowl"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SparksFinder(TEXT("/Game/MysteryCave/Particles/NiagaraSystem_Particles.NiagaraSystem_Particles"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(BowlFinder.Object);
	Mesh->SetRelativeLocation(FVector(0.0f, -260.0f, 0.0f));
	Mesh->SetRelativeScale3D(FVector(1.6f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(RootComponent);
	FireLight->SetRelativeLocation(FVector(0.0f, -220.0f, 520.0f));
	FireLight->IntensityUnits = ELightUnits::Candelas;
	FireLight->SetIntensity(15.0f);
	FireLight->SetAttenuationRadius(1400.0f);
	FireLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.22f));
	FireLight->SetCastShadows(false);

	Sparks = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Sparks"));
	Sparks->SetupAttachment(RootComponent);
	Sparks->SetRelativeLocation(FVector(0.0f, -260.0f, 500.0f));
	Sparks->SetAsset(SparksFinder.Object);
	Sparks->SetAutoActivate(false);

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(RootComponent);
	SpawnPoint->SetRelativeLocation(FVector(120.0f, 0.0f, 120.0f));

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(RootComponent);
	Trigger->SetBoxExtent(FVector(140.0f, 500.0f, 400.0f));
	Trigger->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
	Trigger->SetCollisionProfileName(FName("Trigger"));
}

void AAmnesiaCheckpoint::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AAmnesiaCheckpoint::OnTriggerOverlap);

	// checkpoints the saved game already passed start lit
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		if (ProgressSystem->GetProgress()->CheckpointIndex >= CheckpointIndex && ProgressSystem->bContinueFromCheckpoint)
		{
			SetLitSilently();
		}
	}
}

FTransform AAmnesiaCheckpoint::GetSpawnTransform() const
{
	FTransform Transform = SpawnPoint->GetComponentTransform();
	Transform.SetRotation(FQuat::Identity);
	Transform.SetScale3D(FVector::OneVector);

	return Transform;
}

void AAmnesiaCheckpoint::SetLitSilently()
{
	bLit = true;
	Sparks->Activate(true);
}

void AAmnesiaCheckpoint::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(OtherActor);

	if (!Hero || Hero->GetHealth() <= 0.0f)
	{
		return;
	}

	// resting always heals, even at a fire that is already lit
	Hero->HealFully();

	// respawn here from now on
	if (ACombatPlayerController* PC = Cast<ACombatPlayerController>(Hero->GetController()))
	{
		PC->SetRespawnTransform(GetSpawnTransform());
	}

	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);

	if (ProgressSystem)
	{
		ProgressSystem->StoreHero(Hero->GetHeroLevel(), Hero->GetCurrentXP());

		// never move the save point backwards
		if (CheckpointIndex >= ProgressSystem->GetProgress()->CheckpointIndex)
		{
			ProgressSystem->StoreCheckpoint(CheckpointIndex, GetSpawnTransform());

		} else {

			ProgressSystem->SaveGame();
		}
	}

	if (bLit)
	{
		return;
	}

	bLit = true;
	LitTime = GetWorld()->GetTimeSeconds();
	Sparks->Activate(true);

	LightSounds.PlayAt(this, GetActorLocation());

	if (APlayerController* PC = Cast<APlayerController>(Hero->GetController()))
	{
		if (AAmnesiaHUD* HUD = Cast<AAmnesiaHUD>(PC->GetHUD()))
		{
			HUD->ShowBanner(DisplayName, FText::FromString(TEXT("Életerő feltöltve  •  Játék mentve")));
		}
	}
}

void AAmnesiaCheckpoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Time = GetWorld()->GetTimeSeconds() * 6.0f + GetActorLocation().X * 0.01f;
	const float Flicker = 1.0f + 0.12f * FMath::Sin(Time) + 0.08f * FMath::Sin(Time * 2.7f + 1.3f);

	// flare up right after lighting, then settle
	const float Flare = 1.0f + 1.5f * FMath::Exp(-(GetWorld()->GetTimeSeconds() - LitTime) * 2.0f);

	FireLight->SetIntensity((bLit ? LitIntensity * Flare : DimIntensity) * Flicker);
}
