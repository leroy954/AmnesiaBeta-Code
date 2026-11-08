#include "AmnesiaCat.h"
#include "AmnesiaBeta.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaHUD.h"
#include "AmnesiaProgress.h"
#include "AmnesiaSkills.h"
#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The hero may pet a cat this far above or below it */
	const float CatPetHeight = 220.0f;
}

TArray<TWeakObjectPtr<AAmnesiaCat>> AAmnesiaCat::AllCats;

AAmnesiaCat::AAmnesiaCat()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// a cat out of sight does not need its pose worked out, and a far one needs it less often
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh->bEnableUpdateRateOptimizations = true;

	// the chime of the healing orbs, a bit higher: a soft thank you
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		const FString Path = FString::Printf(TEXT("/Game/Audio/Hero/SW_Orb_Pickup_%02d.SW_Orb_Pickup_%02d"), Index, Index);
		ConstructorHelpers::FObjectFinder<USoundBase> Finder(*Path);

		if (Finder.Succeeded())
		{
			PetSounds.Sounds.Add(Finder.Object);
		}
	}

	PetSounds.Volume = 0.55f;
	PetSounds.PitchMin = 1.2f;
	PetSounds.PitchMax = 1.35f;
	PetSounds.Range = 3000.0f;
}

void AAmnesiaCat::BeginPlay()
{
	Super::BeginPlay();

	AllCats.AddUnique(this);

	if (CatId.IsNone())
	{
		CatId = GetFName();
	}

	PlacedYaw = GetActorRotation().Yaw;

	// the model, the coat and the poses come from the cat pack: a project without it has no cats
	USkeletalMesh* Model = CatMesh.LoadSynchronous();

	if (!Model)
	{
		UE_LOG(LogAmnesiaBeta, Warning, TEXT("Cat %s: the cat pack is not in the project, the cat stays away"), *GetName());
		SetActorHiddenInGame(true);
		SetActorTickEnabled(false);
		return;
	}

	LoadedSleep = SleepPose.LoadSynchronous();
	LoadedIdle = IdleAnim.LoadSynchronous();
	LoadedHappy = HappyAnim.LoadSynchronous();

	Mesh->SetSkeletalMeshAsset(Model);

	const FString CoatPath = FString::Printf(TEXT("%sMI_Cat_%02d.MI_Cat_%02d"), *CoatFolder, Coat, Coat);

	if (UMaterialInterface* CoatMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(CoatPath)).LoadSynchronous())
	{
		Mesh->SetMaterial(0, CoatMaterial);
	}

	Mesh->SetRelativeRotation(FRotator(0.0f, MeshYawOffset, 0.0f));
	BaseScale = FVector(CatScale);
	Mesh->SetRelativeScale3D(BaseScale);

	// the feet on the ground that is right under where the cat was put
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	const FVector Where = GetActorLocation();

	if (GetWorld()->LineTraceSingleByChannel(Hit, Where + FVector(0.0f, 0.0f, 90.0f), Where - FVector(0.0f, 0.0f, 140.0f), ECC_Visibility, Params))
	{
		SetActorLocation(FVector(Where.X, Where.Y, Hit.ImpactPoint.Z));
	}

	Phase = FMath::FRandRange(0.0f, 6.28f);

	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		ProgressSystem->RegisterCat(CatId);
	}

	RefreshFromProgress();
}

void AAmnesiaCat::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AllCats.Remove(this);

	Super::EndPlay(EndPlayReason);
}

void AAmnesiaCat::RefreshFromProgress()
{
	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);
	const bool bFound = ProgressSystem && ProgressSystem->HasPettedCat(CatId);

	SetState(bFound ? EAmnesiaCatState::Awake : EAmnesiaCatState::Asleep);
}

void AAmnesiaCat::SetState(EAmnesiaCatState NewState)
{
	State = NewState;
	StateStart = GetWorld()->GetTimeSeconds();

	// the breathing of a sleeper changes the scale: every state starts from the normal one
	Mesh->SetRelativeScale3D(BaseScale);

	switch (NewState)
	{
	case EAmnesiaCatState::Asleep:
		{
			// back to where it was put down
			FRotator Rotation = GetActorRotation();
			Rotation.Yaw = PlacedYaw;
			SetActorRotation(Rotation);

			// the lying pose is the last frame of the short fall: show it and hold it
			if (LoadedSleep)
			{
				Mesh->PlayAnimation(LoadedSleep, false);
				Mesh->SetPosition(LoadedSleep->GetPlayLength(), false);
				Mesh->Stop();
			}

			break;
		}

	case EAmnesiaCatState::Happy:
		if (LoadedHappy)
		{
			Mesh->PlayAnimation(LoadedHappy, false);
		}

		break;

	case EAmnesiaCatState::Awake:
		if (LoadedIdle)
		{
			Mesh->PlayAnimation(LoadedIdle, true);
		}

		break;
	}
}

void AAmnesiaCat::FaceHero(const AAmnesiaCharacter* Hero, float DeltaTime)
{
	const float DeltaX = Hero->GetActorLocation().X - GetActorLocation().X;

	// close enough to be noticed, not so close that it flips from side to side
	if (FMath::Abs(DeltaX) > 700.0f || FMath::Abs(DeltaX) < 25.0f)
	{
		return;
	}

	FRotator Rotation = GetActorRotation();
	Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, DeltaX > 0.0f ? 0.0f : 180.0f, 300.0f * DeltaTime);
	SetActorRotation(Rotation);
}

void AAmnesiaCat::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!Hero)
	{
		return;
	}

	// far from the hero nobody sees the cat: look a few times a second and keep it still
	const bool bFar = FMath::Abs(Hero->GetActorLocation().X - GetActorLocation().X) > 4500.0f;
	Mesh->bPauseAnims = bFar;
	SetActorTickInterval(bFar ? 0.4f : 0.0f);

	if (bFar)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	switch (State)
	{
	case EAmnesiaCatState::Asleep:
		{
			// the slow breathing of a sleeper
			const float Breath = 0.014f * FMath::Sin(Now * 1.8f + Phase);
			Mesh->SetRelativeScale3D(BaseScale * FVector(1.0f + Breath * 0.4f, 1.0f + Breath * 0.4f, 1.0f + Breath));
			break;
		}

	case EAmnesiaCatState::Happy:
		FaceHero(Hero, DeltaTime);

		if (Now - StateStart >= (LoadedHappy ? LoadedHappy->GetPlayLength() : 0.8f))
		{
			SetState(EAmnesiaCatState::Awake);
		}

		break;

	case EAmnesiaCatState::Awake:
		FaceHero(Hero, DeltaTime);
		break;
	}
}

FVector AAmnesiaCat::GetMarkerLocation() const
{
	return GetActorLocation() + FVector(0.0f, 0.0f, 55.0f * CatScale + 20.0f);
}

bool AAmnesiaCat::CanBePetted(const AAmnesiaCharacter* Hero) const
{
	if (!Hero || Hero->GetHealth() <= 0.0f || IsHidden())
	{
		return false;
	}

	const FVector Delta = Hero->GetActorLocation() - GetActorLocation();

	return FMath::Abs(Delta.X) <= PetRange && FMath::Abs(Delta.Z) <= CatPetHeight;
}

AAmnesiaCat* AAmnesiaCat::FindPettable(const AAmnesiaCharacter* Hero)
{
	AllCats.RemoveAll([](const TWeakObjectPtr<AAmnesiaCat>& Cat) { return !Cat.IsValid(); });

	AAmnesiaCat* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<AAmnesiaCat>& Weak : AllCats)
	{
		AAmnesiaCat* Cat = Weak.Get();

		if (Cat && Cat->CanBePetted(Hero))
		{
			const float Distance = FMath::Abs(Hero->GetActorLocation().X - Cat->GetActorLocation().X);

			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = Cat;
			}
		}
	}

	return Best;
}

void AAmnesiaCat::Pet(AAmnesiaCharacter* Hero)
{
	// one hop at a time
	if (!Hero || State == EAmnesiaCatState::Happy)
	{
		return;
	}

	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);
	const bool bNewFind = ProgressSystem && ProgressSystem->PetCat(CatId);

	AAmnesiaHUD* HUD = nullptr;

	if (const APlayerController* PC = Cast<APlayerController>(Hero->GetController()))
	{
		HUD = Cast<AAmnesiaHUD>(PC->GetHUD());
	}

	const FVector Above = GetMarkerLocation();

	// the cat turns to the hero and takes a happy hop, and it is awake from now on
	SetState(EAmnesiaCatState::Happy);
	PetSounds.PlayAt(this, GetActorLocation());

	// a cat that was found before is only glad to be petted again
	if (!bNewFind)
	{
		if (HUD)
		{
			HUD->AddTextPopup(Above, TEXT("prrr..."), FLinearColor(1.0f, 0.78f, 0.88f), 1.3f, 1.5f);
		}

		return;
	}

	// the first time ever: the reward, the counter and the save
	Hero->GainXP(XPReward);

	const int32 Found = ProgressSystem->GetCatsFound();
	const int32 Total = ProgressSystem->GetCatTotal();
	const bool bAll = Total > 0 && Found >= Total;

	if (bAll)
	{
		ProgressSystem->GetProgress()->CatSkillPoints += 1;
		Hero->GetSkills()->OnLevelChanged();
	}

	if (HUD)
	{
		if (XPReward > 0)
		{
			HUD->AddXPPopup(Above, XPReward);
		}

		if (bAll)
		{
			HUD->ShowBanner(FText::FromString(TEXT("MINDEN CICA MEGVAN!")), FText::FromString(TEXT("+1 képességpont")));

		} else {

			HUD->ShowBanner(FText::FromString(TEXT("CICA MEGTALÁLVA!")), FText::FromString(FString::Printf(TEXT("Cicák: %d / %d   •   +%d XP"), Found, Total, XPReward)));
		}
	}

	ProgressSystem->SaveGame();
}
