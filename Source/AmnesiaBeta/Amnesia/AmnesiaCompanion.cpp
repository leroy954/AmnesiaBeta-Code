#include "AmnesiaCompanion.h"
#include "AmnesiaBeta.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaCreatureAnimInstance.h"
#include "AmnesiaEnemy.h"
#include "AmnesiaFireball.h"
#include "EngineUtils.h"
#include "Animation/AnimSequenceBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AmnesiaSkills.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AAmnesiaCompanion::AAmnesiaCompanion()
{
	PrimaryActorTick.bCanEverTick = true;

	// move after the hero has moved, so the follow doesn't jitter
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(TEXT("/Game/LittleDragons/Meshes/SK_Little_Dragon_Tiger_Toon.SK_Little_Dragon_Tiger_Toon"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyFinder(TEXT("/Game/LittleDragons/Materials/Little_Dragon_Tiger/Toon/MI_TD_Toon_Orange.MI_TD_Toon_Orange"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HoverFinder(TEXT("/Game/LittleDragons/Animations/Common/LD_C_Fly_Idle.LD_C_Fly_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> FlyFinder(TEXT("/Game/LittleDragons/Animations/In_Place/LD_IP_Fly_Forward.LD_IP_Fly_Forward"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> GlideFinder(TEXT("/Game/LittleDragons/Animations/In_Place/LD_IP_Fly_Glide.LD_IP_Fly_Glide"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> FlinchFinder(TEXT("/Game/LittleDragons/Animations/In_Place/LD_IP_Fly_Dodge_Left.LD_IP_Fly_Dodge_Left"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> FireFinder(TEXT("/Game/LittleDragons/Animations/Common/LD_C_Fly_Attack_Fireball.LD_C_Fly_Attack_Fireball"));

	FireAnimation = FireFinder.Object;
	FireballClass = AAmnesiaFireball::StaticClass();

	for (int32 Index = 1; Index <= 3; ++Index)
	{
		const FString Path = FString::Printf(TEXT("/Game/Audio/Companion/SW_Fireball_Launch_%02d.SW_Fireball_Launch_%02d"), Index, Index);
		ConstructorHelpers::FObjectFinder<USoundBase> Finder(*Path);

		if (Finder.Succeeded())
		{
			FireSounds.Sounds.Add(Finder.Object);
		}
	}

	FireSounds.Volume = 0.6f;
	FireSounds.PitchMin = 1.05f;
	FireSounds.PitchMax = 1.3f;
	FireSounds.Range = 3500.0f;

	HoverAnimation = HoverFinder.Object;
	FlyAnimation = FlyFinder.Object;
	GlideAnimation = GlideFinder.Object;
	FlinchAnimation = FlinchFinder.Object;
	BodyMaterial = BodyFinder.Object;

	// a big dragon's calls pitched way up sound like a little one
	for (int32 Index = 1; Index <= 7; ++Index)
	{
		const FString Path = FString::Printf(TEXT("/Game/Audio/Companion/SW_Dragon_Chirp_%02d.SW_Dragon_Chirp_%02d"), Index, Index);
		ConstructorHelpers::FObjectFinder<USoundBase> Finder(*Path);

		if (Finder.Succeeded())
		{
			ChirpSounds.Sounds.Add(Finder.Object);
		}
	}

	for (int32 Index = 1; Index <= 2; ++Index)
	{
		const FString Path = FString::Printf(TEXT("/Game/Audio/Companion/SW_Dragon_Flap_%02d.SW_Dragon_Flap_%02d"), Index, Index);
		ConstructorHelpers::FObjectFinder<USoundBase> Finder(*Path);

		if (Finder.Succeeded())
		{
			FlapSounds.Sounds.Add(Finder.Object);
		}
	}

	ChirpSounds.Volume = 0.45f;
	ChirpSounds.PitchMin = 1.55f;
	ChirpSounds.PitchMax = 1.9f;
	ChirpSounds.Range = 3500.0f;

	FlapSounds.Volume = 0.35f;
	FlapSounds.PitchMin = 1.25f;
	FlapSounds.PitchMax = 1.5f;
	FlapSounds.Range = 3500.0f;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetSkeletalMeshAsset(MeshFinder.Object);
	Mesh->SetAnimInstanceClass(UAmnesiaCreatureAnimInstance::StaticClass());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// lit by the camera light like the hero
	Mesh->LightingChannels.bChannel1 = true;

	// the glow sits under the belly, so it lights the ground and the hero rather than burning out the dragon itself
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	Glow->IntensityUnits = ELightUnits::Candelas;
	Glow->SetIntensity(5.0f);
	Glow->SetAttenuationRadius(650.0f);
	Glow->SetLightColor(FLinearColor(1.0f, 0.72f, 0.42f));
	Glow->SetCastShadows(false);
}

void AAmnesiaCompanion::BeginPlay()
{
	Super::BeginPlay();

	// the body turns around its middle, not around its feet
	Mesh->SetRelativeScale3D(FVector(MeshScale));
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -BodyCenterHeight * MeshScale));
	Mesh->SetRelativeRotation(FRotator(0.0f, MeshYawOffset, 0.0f));

	if (BodyMaterial)
	{
		Mesh->SetMaterial(0, BodyMaterial);

		// a copy of the body material, so the color can follow the element
		BodyInstance = Mesh->CreateDynamicMaterialInstance(0, BodyMaterial);
	}

	BaseGlowIntensity = Glow->Intensity;
	BaseGlowRadius = Glow->AttenuationRadius;
	BaseGlowColor = Glow->GetLightColor();

	SetupAnimation();

	// the fireballs leave from the jaw, or the head if the model has no jaw bone
	for (const TCHAR* Key : { TEXT("jaw"), TEXT("mouth"), TEXT("head") })
	{
		for (int32 Index = 0; Index < Mesh->GetNumBones() && MouthBone.IsNone(); ++Index)
		{
			if (Mesh->GetBoneName(Index).ToString().Contains(Key))
			{
				MouthBone = Mesh->GetBoneName(Index);
			}
		}
	}

	UE_LOG(LogAmnesiaBeta, Log, TEXT("Companion: fireballs leave from bone '%s'"), *MouthBone.ToString());

	CurrentYaw = GetActorRotation().Yaw;
	NextChirpTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(MinChirpInterval, MaxChirpInterval) * 0.5f;
}

void AAmnesiaCompanion::SetupAnimation()
{
	UAmnesiaCreatureAnimInstance* Creature = Cast<UAmnesiaCreatureAnimInstance>(Mesh->GetAnimInstance());

	if (!Creature)
	{
		return;
	}

	// hover when still, flap along at walking speed, glide when the hero sprints
	Creature->IdleAnimation = HoverAnimation;
	Creature->WalkAnimation = FlyAnimation;
	Creature->RunAnimation = GlideAnimation;
	Creature->WalkThreshold = 120.0f;
	Creature->WalkAnimationSpeed = 450.0f;
	Creature->RunThreshold = 680.0f;
	Creature->RunAnimationSpeed = 800.0f;
	Creature->LocomotionBlendTime = 0.4f;
}

void AAmnesiaCompanion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Now = GetWorld()->GetTimeSeconds();
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);

	if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(Pawn))
	{
		WatchHero(Hero);
		UpdateAttack(Hero, Now);
	}

	// the color slides to the one of the element on the sword
	if (BodyInstance && !FMath::IsNearlyEqual(CurrentHue, TargetHue, 0.002f))
	{
		CurrentHue = FMath::FInterpTo(CurrentHue, TargetHue, DeltaTime, 2.2f);
		BodyInstance->SetScalarParameterValue(TEXT("Hue"), CurrentHue);
	}

	if (bDebugCelebrate)
	{
		bDebugCelebrate = false;
		Celebrate();
	}

	if (bDebugFlinch)
	{
		bDebugFlinch = false;
		Flinch();
	}

	FVector Location = GetActorLocation();
	FVector Target = Location;
	FVector TargetVelocity = FVector::ZeroVector;

	if (Pawn)
	{
		Facing = Pawn->GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;

		// over the shoulder, on the side away from where the hero looks, with a lazy figure-eight hover
		Target = Pawn->GetActorLocation() + FVector(FollowOffset.X * Facing, FollowOffset.Y, FollowOffset.Z);
		Target.Z += FMath::Sin(Now * BobSpeed) * BobHeight;
		Target.X += FMath::Sin(Now * BobSpeed * 0.53f + 1.3f) * BobHeight * 0.7f;

		TargetVelocity = Pawn->GetVelocity();

		IdleTime = Pawn->GetVelocity().Size() < 30.0f ? IdleTime + DeltaTime : 0.0f;

		// the first frame, or the hero is suddenly far away: appear near the spot and swoop in
		if (!bPlaced || FVector::Dist(Location, Target) > TeleportDistance)
		{
			Location = bPlaced ? Target + FVector(-Facing * 260.0f, 0.0f, 240.0f) : Target;
			Velocity = FVector::ZeroVector;
			bPlaced = true;
		}
	}

	// critically damped spring towards the spot, moving along with the hero so it doesn't trail behind
	// small steps, so a slow frame still covers all of its time without the spring blowing up
	const float Damping = 2.0f * FMath::Sqrt(FollowStiffness);
	const float FrameTime = FMath::Min(DeltaTime, 0.5f);
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(FrameTime * 60.0f), 1, 30);
	const float Step = FrameTime / Steps;

	for (int32 Index = 0; Index < Steps; ++Index)
	{
		const FVector Acceleration = (Target - Location) * FollowStiffness + (TargetVelocity - Velocity) * Damping;
		Velocity += Acceleration * Step;
		Location += Velocity * Step;
	}

	SetActorLocation(Location);
	Root->ComponentVelocity = Velocity;

	// look where it flies, a little towards the camera, and right at the player after the hero rests a while
	const bool bLooking = IdleTime > IdleLookDelay && FMath::Fmod(IdleTime - IdleLookDelay, 12.0f) < 4.0f;
	const float PreviousLook = LookAlpha;
	LookAlpha = FMath::FInterpTo(LookAlpha, bLooking ? 1.0f : 0.0f, DeltaTime, 3.0f);

	if (PreviousLook < 0.05f && LookAlpha >= 0.05f)
	{
		ChirpSounds.PlayAt(this, Location, 0.8f);
	}

	float FlightYaw = FMath::Abs(Velocity.X) > 90.0f ? (Velocity.X > 0.0f ? 0.0f : 180.0f) : (Facing > 0.0f ? 0.0f : 180.0f);
	float AimPitch = 0.0f;
	const bool bAiming = IsAiming(Now);

	// turn to the enemy while spitting at it
	if (bAiming)
	{
		const FVector ToTarget = AimTarget->GetActorLocation() - Location;
		FlightYaw = ToTarget.X >= 0.0f ? 0.0f : 180.0f;
		AimPitch = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Z, FMath::Abs(ToTarget.X))), -35.0f, 20.0f);
	}

	float CameraYaw = 90.0f;

	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PlayerController->PlayerCameraManager)
		{
			CameraYaw = (PlayerController->PlayerCameraManager->GetCameraLocation() - Location).Rotation().Yaw;
		}
	}

	const float TowardsCamera = bAiming ? 0.12f : FMath::Lerp(FaceCameraAmount, LookAtPlayerAmount, LookAlpha);
	const float DesiredYaw = FlightYaw + FRotator::NormalizeAxis(CameraYaw - FlightYaw) * TowardsCamera;

	const float YawStep = FRotator::NormalizeAxis(DesiredYaw - CurrentYaw) * FMath::Min(1.0f, DeltaTime * (bAiming ? 12.0f : 5.0f));
	CurrentYaw = FRotator::NormalizeAxis(CurrentYaw + YawStep);

	// nose up when climbing, down when diving or rushing ahead, and lean into turns. While spitting, aim the head
	const float FlightPitch = FMath::Clamp(Velocity.Z * 0.045f - FMath::Abs(Velocity.X) * 0.008f, -MaxPitch, MaxPitch) * (1.0f - LookAlpha);
	const float TargetPitch = bAiming ? AimPitch : FlightPitch;
	// bank from a smoothed turn rate, so one uneven frame doesn't tip it over
	TurnRate = FMath::FInterpTo(TurnRate, DeltaTime > 0.0f ? YawStep / DeltaTime : 0.0f, DeltaTime, 6.0f);
	const float TargetBank = FMath::Clamp(-TurnRate * 0.1f, -MaxBank, MaxBank);

	CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, 4.0f);
	CurrentBank = FMath::FInterpTo(CurrentBank, TargetBank, DeltaTime, 4.0f);

	// celebration barrel roll
	float Spin = 0.0f;
	const float RollAge = Now - BarrelRollStart;

	if (RollAge >= 0.0f && RollAge < 0.9f)
	{
		Spin = 360.0f * FMath::InterpEaseInOut(0.0f, 1.0f, RollAge / 0.9f, 2.0f);
	}

	SetActorRotation(FRotator(CurrentPitch, CurrentYaw, CurrentBank + Spin));

	// a few wing beats when it speeds up
	const bool bFast = Velocity.Size() > 450.0f;

	if (bFast && !bFastLastFrame)
	{
		FlapSounds.PlayAt(this, Location);
	}

	bFastLastFrame = bFast;

	// a little call now and then
	if (Now > NextChirpTime)
	{
		ChirpSounds.PlayAt(this, Location);
		NextChirpTime = Now + FMath::FRandRange(MinChirpInterval, MaxChirpInterval);
	}
}

void AAmnesiaCompanion::WatchHero(AAmnesiaCharacter* Hero)
{
	const int32 HeroLevel = Hero->GetHeroLevel();
	const float HeroHP = Hero->GetHealth();

	// a new hero after a respawn: just start watching
	if (WatchedHero.Get() != Hero)
	{
		WatchedHero = Hero;
		LastHeroLevel = HeroLevel;
		LastHeroHP = HeroHP;
		return;
	}

	if (HeroLevel > LastHeroLevel)
	{
		Celebrate();
	}

	if (HeroHP < LastHeroHP - 0.01f && HeroHP > 0.0f)
	{
		Flinch();
	}

	LastHeroLevel = HeroLevel;
	LastHeroHP = HeroHP;
}

void AAmnesiaCompanion::UpdateAttack(AAmnesiaCharacter* Hero, float Now)
{
	// the fireball leaves partway into the spit
	if (!bFireReleased && Now - AttackStartTime >= FireReleaseTime)
	{
		ReleaseFireball(Hero);
	}

	if (!bCanAttack || !FireballClass || Hero->GetHealth() <= 0.0f)
	{
		EnemyInRangeSince = -1.0f;
		return;
	}

	if (Now < NextFireTime || Now < NextTargetSearch)
	{
		return;
	}

	NextTargetSearch = Now + 0.25f;

	AActor* Enemy = FindTarget(Hero);

	if (!Enemy)
	{
		EnemyInRangeSince = -1.0f;
		return;
	}

	// notice the enemy first, then spit
	if (EnemyInRangeSince < 0.0f)
	{
		EnemyInRangeSince = Now;
	}

	if (Now - EnemyInRangeSince < FirstShotDelay)
	{
		return;
	}

	AimTarget = Enemy;
	AttackStartTime = Now;
	bFireReleased = false;
	NextFireTime = Now + FireCooldown * SkillCooldownMult * FMath::FRandRange(0.85f, 1.15f);

	if (UAmnesiaCreatureAnimInstance* Creature = Cast<UAmnesiaCreatureAnimInstance>(Mesh->GetAnimInstance()))
	{
		Creature->PlayAction(FireAnimation, 1.0f);
	}
}

AActor* AAmnesiaCompanion::FindTarget(const AAmnesiaCharacter* Hero) const
{
	const FVector HeroLocation = Hero->GetActorLocation();
	AActor* Best = nullptr;
	float BestDistance = FireRange;

	for (TActorIterator<AAmnesiaEnemy> It(GetWorld()); It; ++It)
	{
		if (It->GetHealth() <= 0.0f)
		{
			continue;
		}

		const FVector Offset = It->GetActorLocation() - HeroLocation;

		// on the hero's stretch of the path, not on a ledge far above or below
		if (FMath::Abs(Offset.Z) > 500.0f || FMath::Abs(Offset.X) >= BestDistance)
		{
			continue;
		}

		Best = *It;
		BestDistance = FMath::Abs(Offset.X);
	}

	return Best;
}

void AAmnesiaCompanion::ReleaseFireball(AAmnesiaCharacter* Hero)
{
	bFireReleased = true;

	AAmnesiaEnemy* Enemy = Cast<AAmnesiaEnemy>(AimTarget.Get());

	if (!Enemy || Enemy->GetHealth() <= 0.0f || !FireballClass)
	{
		return;
	}

	const FVector Mouth = MouthBone.IsNone() ? GetActorLocation() + GetActorForwardVector() * 40.0f : Mesh->GetBoneLocation(MouthBone);
	const FVector ToTarget = Enemy->GetActorLocation() - Mouth;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AAmnesiaFireball* Fireball = GetWorld()->SpawnActor<AAmnesiaFireball>(FireballClass, Mouth, ToTarget.Rotation(), Params))
	{
		Fireball->Launch(Enemy, Hero, Hero->GetMeleeDamage() * FireballDamageScale * SkillDamageMult);
	}

	FireSounds.PlayAt(this, Mouth);

	// a little kick back from the spit
	Velocity -= ToTarget.GetSafeNormal() * 140.0f;
}

void AAmnesiaCompanion::Celebrate()
{
	BarrelRollStart = GetWorld()->GetTimeSeconds();
	Velocity += FVector(0.0f, 0.0f, 260.0f);

	ChirpSounds.PlayAt(this, GetActorLocation(), 1.2f);
	FlapSounds.PlayAt(this, GetActorLocation());
}

void AAmnesiaCompanion::Flinch()
{
	if (UAmnesiaCreatureAnimInstance* Creature = Cast<UAmnesiaCreatureAnimInstance>(Mesh->GetAnimInstance()))
	{
		Creature->PlayAction(FlinchAnimation, 1.5f);
	}

	// hop back from the danger
	Velocity += FVector(-Facing * 260.0f, 0.0f, 160.0f);
}

void AAmnesiaCompanion::ApplySkills(AAmnesiaCharacter* Hero)
{
	UAmnesiaSkillComponent* Skills = Hero ? Hero->GetSkills() : nullptr;

	if (!Skills)
	{
		return;
	}

	const FAmnesiaSkillStats& Stats = Skills->GetStats();

	SkillDamageMult = Stats.DragonDamageMult;
	SkillCooldownMult = Stats.DragonCooldownMult;

	// the dragon takes the color of the sword: orange for fire and steel, blue for ice
	const EAmnesiaElement Element = Skills->GetElement();
	TargetHue = Element == EAmnesiaElement::Ice ? 0.6f : 0.0f;

	Glow->SetLightColor(Element == EAmnesiaElement::Ice ? FLinearColor(0.45f, 0.72f, 1.0f) : BaseGlowColor);
	Glow->SetIntensity(BaseGlowIntensity * Stats.DragonLightMult);
	Glow->SetAttenuationRadius(BaseGlowRadius * Stats.DragonLightMult);
}
