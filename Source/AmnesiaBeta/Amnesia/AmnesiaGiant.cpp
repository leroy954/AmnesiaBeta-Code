#include "AmnesiaGiant.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaCreatureAnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AAmnesiaGiant::AAmnesiaGiant()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(TEXT("/Game/StoneGolem/Mesh/SKM_StoneGolem.SKM_StoneGolem"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HammerFinder(TEXT("/Game/StoneGolem/Mesh/SM_StoneGolem_Hammer.SM_StoneGolem_Hammer"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShieldFinder(TEXT("/Game/StoneGolem/Mesh/SM_StoneGolem_Shield.SM_StoneGolem_Shield"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleFinder(TEXT("/Game/StoneGolem/Animations/AS_StoneGolem_idle1.AS_StoneGolem_idle1"));

	for (const TCHAR* Name : { TEXT("idle2"), TEXT("idle3"), TEXT("idle4") })
	{
		const FString Path = FString::Printf(TEXT("/Game/StoneGolem/Animations/AS_StoneGolem_%s.AS_StoneGolem_%s"), Name, Name);
		ConstructorHelpers::FObjectFinder<UAnimSequenceBase> Finder(*Path);

		if (Finder.Succeeded())
		{
			ShiftAnimations.Add(Finder.Object);
		}
	}

	// the golem's own grunts, slowed way down into a rumble
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		const FString Path = FString::Printf(TEXT("/Game/Audio/Enemy/SW_Golem_Hurt_%02d.SW_Golem_Hurt_%02d"), Index, Index);
		ConstructorHelpers::FObjectFinder<USoundBase> Finder(*Path);

		if (Finder.Succeeded())
		{
			RumbleSounds.Sounds.Add(Finder.Object);
		}
	}

	RumbleSounds.Volume = 0.9f;
	RumbleSounds.PitchMin = 0.4f;
	RumbleSounds.PitchMax = 0.5f;
	RumbleSounds.Range = 14000.0f;

	IdleAnimation = IdleFinder.Object;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetSkeletalMeshAsset(MeshFinder.Object);
	Mesh->SetAnimInstanceClass(UAmnesiaCreatureAnimInstance::StaticClass());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	auto SetupProp = [this](UStaticMeshComponent* Prop, UStaticMesh* PropMesh, FName Socket)
	{
		Prop->SetupAttachment(Mesh, Socket);
		Prop->SetStaticMesh(PropMesh);
		Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	};

	Hammer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hammer"));
	SetupProp(Hammer, HammerFinder.Object, FName("weapon_rSocket"));

	Shield = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
	SetupProp(Shield, ShieldFinder.Object, FName("weapon_lSocket"));
}

void AAmnesiaGiant::BeginPlay()
{
	Super::BeginPlay();

	Mesh->SetRelativeRotation(FRotator(0.0f, MeshYawOffset, 0.0f));
	StartYaw = GetActorRotation().Yaw;
	NextShiftTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(MinShiftInterval, MaxShiftInterval) * 0.5f;

	if (UAmnesiaCreatureAnimInstance* Creature = Cast<UAmnesiaCreatureAnimInstance>(Mesh->GetAnimInstance()))
	{
		Creature->IdleAnimation = IdleAnimation;
		Creature->ActionBlendTime = 0.8f;
	}

	// everything plays slowly, so it feels huge
	Mesh->GlobalAnimRateScale = PlayRate;
}

void AAmnesiaGiant::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Now = GetWorld()->GetTimeSeconds();
	const APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
	const float Distance = Hero ? FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation()) : TNumericLimits<float>::Max();

	// keep a slow eye on the hero, within a small arc
	if (Hero && Distance < WatchRange)
	{
		const float Wanted = (Hero->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
		const float Clamped = StartYaw + FMath::Clamp(FRotator::NormalizeAxis(Wanted - StartYaw), -MaxTurn, MaxTurn);
		const float Current = GetActorRotation().Yaw;
		const float Step = FMath::Clamp(FRotator::NormalizeAxis(Clamped - Current), -TurnSpeed * DeltaTime, TurnSpeed * DeltaTime);
		SetActorRotation(FRotator(0.0f, Current + Step, 0.0f));
	}

	if (Now < NextShiftTime)
	{
		return;
	}

	NextShiftTime = Now + FMath::FRandRange(MinShiftInterval, MaxShiftInterval);

	UAmnesiaCreatureAnimInstance* Creature = Cast<UAmnesiaCreatureAnimInstance>(Mesh->GetAnimInstance());

	if (!Creature || ShiftAnimations.Num() == 0)
	{
		return;
	}

	Creature->PlayAction(ShiftAnimations[FMath::RandRange(0, ShiftAnimations.Num() - 1)]);

	// silent until the hero has come out of the cave
	if (!Hero || Hero->GetActorLocation().X < HearFromHeroX)
	{
		return;
	}

	RumbleSounds.PlayAt(this, GetActorLocation());

	// the ground trembles a little when it moves and the hero is close
	if (AAmnesiaCharacter* HeroCharacter = Cast<AAmnesiaCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (Distance < WatchRange * 0.6f)
		{
			HeroCharacter->AddCameraShake(ShakeStrength);
		}
	}
}
