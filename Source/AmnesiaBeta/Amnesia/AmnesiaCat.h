#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaSound.h"
#include "AmnesiaCat.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class AAmnesiaCharacter;

/** What a cat is doing */
UENUM()
enum class EAmnesiaCatState : uint8
{
	/** Lying asleep until it is petted for the first time */
	Asleep,

	/** Just petted: a happy hop */
	Happy,

	/** Awake and sitting about, it turns to the hero when the hero is close */
	Awake
};

/**
 *  A cat to find. It sleeps somewhere in a zone; a gentle pet (the F key) wakes it up, and the first time ever that gives a little gold and XP.
 *  Cats that were found stay awake. Every cat has an id that is kept in the saved progress.
 *  Finding all of them gives a skill point.
 *
 *  The model, the coats and the poses come from the cat pack. They are soft references that load when the game starts,
 *  so a project without the pack has no cats and nothing breaks.
 */
UCLASS()
class AAmnesiaCat : public AActor
{
	GENERATED_BODY()

	/** Where the cat stands */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

	/** The cat's body */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;

protected:

	/** The cat's id in the saved progress. Cats with the same id are the same cat */
	UPROPERTY(EditAnywhere, Category="Cat")
	FName CatId;

	/** Which of the pack's 11 coats the cat has */
	UPROPERTY(EditAnywhere, Category="Cat", meta = (ClampMin = 1, ClampMax = 11))
	int32 Coat = 1;

	/** Size of the cat. The pack's cat is about a meter long */
	UPROPERTY(EditAnywhere, Category="Cat", meta = (ClampMin = 0.1))
	float CatScale = 0.85f;

	/** Yaw of the model against the actor, so the cat faces the actor's forward axis */
	UPROPERTY(EditAnywhere, Category="Cat")
	float MeshYawOffset = -90.0f;

	/** XP for finding the cat, once. The cats give no gold */
	UPROPERTY(EditAnywhere, Category="Cat|Reward", meta = (ClampMin = 0))
	int32 XPReward = 20;

	/** How close the hero has to be along the walking line to pet the cat */
	UPROPERTY(EditAnywhere, Category="Cat", meta = (ClampMin = 0, Units = "cm"))
	float PetRange = 190.0f;

	/** The sleeping cat shows its Zzz from this far */
	UPROPERTY(EditAnywhere, Category="Cat", meta = (ClampMin = 0, Units = "cm"))
	float NoticeRange = 1100.0f;

	/** The model, the folder of the coats (MI_Cat_01 ... MI_Cat_11) and the poses: the lying pose is the last frame of the short fall */
	UPROPERTY(EditAnywhere, Category="Cat|Pack")
	TSoftObjectPtr<USkeletalMesh> CatMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Animal-Cat-FreeBundle/Meshes/SKM_Cat.SKM_Cat")));

	UPROPERTY(EditAnywhere, Category="Cat|Pack")
	FString CoatFolder = TEXT("/Game/Animal-Cat-FreeBundle/Materials/");

	UPROPERTY(EditAnywhere, Category="Cat|Pack")
	TSoftObjectPtr<UAnimSequence> SleepPose = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Animal-Cat-FreeBundle/Animations/Damage/AS_Cat_Dead.AS_Cat_Dead")));

	UPROPERTY(EditAnywhere, Category="Cat|Pack")
	TSoftObjectPtr<UAnimSequence> IdleAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Animal-Cat-FreeBundle/Animations/Movement/AS_Cat_Idle.AS_Cat_Idle")));

	UPROPERTY(EditAnywhere, Category="Cat|Pack")
	TSoftObjectPtr<UAnimSequence> HappyAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Animal-Cat-FreeBundle/Animations/Jump/AS_Cat_Jump.AS_Cat_Jump")));

	/** A soft chime when the cat is petted */
	UPROPERTY(EditAnywhere, Category="Cat|Sound")
	FAmnesiaSoundSet PetSounds;

	/** The loaded poses, kept alive */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedSleep;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedIdle;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedHappy;

	EAmnesiaCatState State = EAmnesiaCatState::Asleep;
	float StateStart = 0.0f;
	float Phase = 0.0f;
	FVector BaseScale = FVector::OneVector;

	/** The yaw the cat was put down with, which it goes back to when it falls asleep */
	float PlacedYaw = 0.0f;

	/** Every cat of the game, for the HUD and the pet key. Weak, since the cats come and go with the level */
	static TArray<TWeakObjectPtr<AAmnesiaCat>> AllCats;

public:

	AAmnesiaCat();

	virtual void Tick(float DeltaTime) override;

	/** All the cats of the game */
	static const TArray<TWeakObjectPtr<AAmnesiaCat>>& GetAll() { return AllCats; }

	/** The cat the hero can pet right now: the nearest one within reach, or null */
	static AAmnesiaCat* FindPettable(const AAmnesiaCharacter* Hero);

	/** True while the cat sleeps */
	bool IsAsleep() const { return State == EAmnesiaCatState::Asleep; }

	/** True during the happy hop after a pet */
	bool IsHopping() const { return State == EAmnesiaCatState::Happy; }

	/** True if the hero is close enough to pet the cat */
	bool CanBePetted(const AAmnesiaCharacter* Hero) const;

	/** How far the sleeping cat shows its Zzz */
	float GetNoticeRange() const { return NoticeRange; }

	/** The cat's id */
	FName GetCatId() const { return CatId; }

	/** A point just above the cat, where the Zzz, the prompt and the popups go */
	FVector GetMarkerLocation() const;

	/** The hero pets the cat: it hops, and the first time ever it gives its gold and XP */
	void Pet(AAmnesiaCharacter* Hero);

	/** Takes the found state from the saved progress: a cat that was found is awake, the others sleep. Also after a new game starts */
	void RefreshFromProgress();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Switches the pose to the one of a state */
	void SetState(EAmnesiaCatState NewState);

	/** Turns the cat to the side of the hero when the hero is close */
	void FaceHero(const AAmnesiaCharacter* Hero, float DeltaTime);
};
