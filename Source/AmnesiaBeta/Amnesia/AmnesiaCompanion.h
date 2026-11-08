#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaSound.h"
#include "AmnesiaCompanion.generated.h"

class USkeletalMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UAnimSequenceBase;
class UMaterialInterface;
class AAmnesiaCharacter;
class AAmnesiaFireball;

/**
 *  A little dragon that flies along with the hero, hovering over the hero's shoulder.
 *  - Follows on a soft spring, so it lags behind and swoops when the hero turns or jumps
 *  - Bobs, banks and pitches with its flight, and turns its face a little towards the camera
 *  - Hovers, flies or glides depending on its speed
 *  - Celebrates level ups with a barrel roll, flinches when the hero gets hurt, and looks at the player when the hero rests
 *  - Gives off a soft warm glow
 */
UCLASS()
class AAmnesiaCompanion : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

	/** The dragon */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;

	/** Soft light around the dragon */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* Glow;

protected:

	/** Where the dragon hovers relative to the hero: X behind the hero (flips with the hero's facing), Y depth (negative is away from the camera), Z height */
	UPROPERTY(EditAnywhere, Category="Companion|Follow")
	FVector FollowOffset = FVector(-85.0f, -45.0f, 170.0f);

	/** Size of the dragon */
	UPROPERTY(EditAnywhere, Category="Companion", meta = (ClampMin = 0.1))
	float MeshScale = 1.0f;

	/** Height of the middle of the body above the model's feet, at scale 1. The dragon turns around this point */
	UPROPERTY(EditAnywhere, Category="Companion", meta = (Units = "cm"))
	float BodyCenterHeight = 35.0f;

	/** How tightly the dragon follows. Higher catches up faster */
	UPROPERTY(EditAnywhere, Category="Companion|Follow", meta = (ClampMin = 1))
	float FollowStiffness = 14.0f;

	/** Farther than this from its spot, the dragon jumps there, for respawns and teleports */
	UPROPERTY(EditAnywhere, Category="Companion|Follow", meta = (Units = "cm"))
	float TeleportDistance = 2200.0f;

	/** Gentle up and down hover */
	UPROPERTY(EditAnywhere, Category="Companion|Follow", meta = (Units = "cm"))
	float BobHeight = 12.0f;

	UPROPERTY(EditAnywhere, Category="Companion|Follow")
	float BobSpeed = 2.2f;

	/** How far the dragon turns its face to the camera while flying, and while looking at the player */
	UPROPERTY(EditAnywhere, Category="Companion|Look", meta = (ClampMin = 0, ClampMax = 1))
	float FaceCameraAmount = 0.3f;

	UPROPERTY(EditAnywhere, Category="Companion|Look", meta = (ClampMin = 0, ClampMax = 1))
	float LookAtPlayerAmount = 0.85f;

	/** After the hero stands still this long, the dragon turns to look at the player for a while */
	UPROPERTY(EditAnywhere, Category="Companion|Look", meta = (Units = "s"))
	float IdleLookDelay = 6.0f;

	/** Yaw of the mesh against the actor, so the model faces the actor's forward axis */
	UPROPERTY(EditAnywhere, Category="Companion|Look")
	float MeshYawOffset = -90.0f;

	/** Strongest nose up or down, and the strongest bank */
	UPROPERTY(EditAnywhere, Category="Companion|Look", meta = (Units = "deg"))
	float MaxPitch = 22.0f;

	UPROPERTY(EditAnywhere, Category="Companion|Look", meta = (Units = "deg"))
	float MaxBank = 30.0f;

	/** Animations: hovering, flying and gliding, picked by speed */
	UPROPERTY(EditAnywhere, Category="Companion|Animation")
	TObjectPtr<UAnimSequenceBase> HoverAnimation;

	UPROPERTY(EditAnywhere, Category="Companion|Animation")
	TObjectPtr<UAnimSequenceBase> FlyAnimation;

	UPROPERTY(EditAnywhere, Category="Companion|Animation")
	TObjectPtr<UAnimSequenceBase> GlideAnimation;

	/** Quick dodge played when the hero gets hurt */
	UPROPERTY(EditAnywhere, Category="Companion|Animation")
	TObjectPtr<UAnimSequenceBase> FlinchAnimation;

	/** Body material, to pick the dragon's colors */
	UPROPERTY(EditAnywhere, Category="Companion")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	/** Little calls, now and then and when celebrating */
	UPROPERTY(EditAnywhere, Category="Companion|Sound")
	FAmnesiaSoundSet ChirpSounds;

	/** Wing beats when it speeds up */
	UPROPERTY(EditAnywhere, Category="Companion|Sound")
	FAmnesiaSoundSet FlapSounds;

	/** Time between idle calls */
	UPROPERTY(EditAnywhere, Category="Companion|Sound", meta = (Units = "s"))
	float MinChirpInterval = 16.0f;

	UPROPERTY(EditAnywhere, Category="Companion|Sound", meta = (Units = "s"))
	float MaxChirpInterval = 34.0f;

	/** If true, the dragon spits fireballs at enemies near the hero */
	UPROPERTY(EditAnywhere, Category="Companion|Fireball")
	bool bCanAttack = true;

	UPROPERTY(EditAnywhere, Category="Companion|Fireball")
	TSubclassOf<AAmnesiaFireball> FireballClass;

	/** Enemies closer than this to the hero get shot at */
	UPROPERTY(EditAnywhere, Category="Companion|Fireball", meta = (Units = "cm"))
	float FireRange = 1100.0f;

	/** Seconds between fireballs, varied a little */
	UPROPERTY(EditAnywhere, Category="Companion|Fireball", meta = (ClampMin = 0.5, Units = "s"))
	float FireCooldown = 5.5f;

	/** Pause before the first shot of a fight, so the dragon doesn't fire the moment an enemy shows up */
	UPROPERTY(EditAnywhere, Category="Companion|Fireball", meta = (Units = "s"))
	float FirstShotDelay = 0.8f;

	/** Fireball damage as a share of the hero's sword damage, so the dragon grows with the hero */
	UPROPERTY(EditAnywhere, Category="Companion|Fireball", meta = (ClampMin = 0))
	float FireballDamageScale = 0.35f;

	/** The spit, and how far into it the fireball leaves the mouth */
	UPROPERTY(EditAnywhere, Category="Companion|Fireball")
	TObjectPtr<UAnimSequenceBase> FireAnimation;

	UPROPERTY(EditAnywhere, Category="Companion|Fireball", meta = (Units = "s"))
	float FireReleaseTime = 0.22f;

	/** Whoosh of the fireball leaving */
	UPROPERTY(EditAnywhere, Category="Companion|Fireball")
	FAmnesiaSoundSet FireSounds;

	/** Attack state */
	TWeakObjectPtr<AActor> AimTarget;
	float NextFireTime = 0.0f;
	float NextTargetSearch = 0.0f;
	float AttackStartTime = -100.0f;
	float EnemyInRangeSince = -1.0f;
	bool bFireReleased = true;

	/** Bone the fireballs leave from, found by name */
	FName MouthBone;

	/** Test helpers: set on the running dragon to see the reactions */
	UPROPERTY(EditAnywhere, Category="Companion|Debug")
	bool bDebugCelebrate = false;

	UPROPERTY(EditAnywhere, Category="Companion|Debug")
	bool bDebugFlinch = false;

	/** Flight state */
	FVector Velocity = FVector::ZeroVector;
	float Facing = 1.0f;
	float CurrentYaw = 0.0f;
	float CurrentPitch = 0.0f;
	float CurrentBank = 0.0f;
	float TurnRate = 0.0f;
	float LookAlpha = 0.0f;
	float IdleTime = 0.0f;
	float BarrelRollStart = -100.0f;
	float NextChirpTime = 0.0f;
	bool bFastLastFrame = false;
	bool bPlaced = false;

	/** What the hero was last frame, to notice level ups and hits */
	TWeakObjectPtr<AAmnesiaCharacter> WatchedHero;
	int32 LastHeroLevel = 0;
	float LastHeroHP = 0.0f;

public:

	AAmnesiaCompanion();

	virtual void Tick(float DeltaTime) override;

	/** Barrel roll and a happy call */
	void Celebrate();

	/** A quick startled dodge */
	void Flinch();

	/** Takes the dragon's fire, light, color and shield from the hero's skills */
	void ApplySkills(AAmnesiaCharacter* Hero);

protected:

	virtual void BeginPlay() override;

	/** Gives the anim instance its animations */
	void SetupAnimation();

	/** Notices level ups and hits of the hero */
	void WatchHero(AAmnesiaCharacter* Hero);

	/** Picks targets and runs the spit */
	void UpdateAttack(AAmnesiaCharacter* Hero, float Now);

	/** The nearest living enemy in range of the hero, if any */
	AActor* FindTarget(const AAmnesiaCharacter* Hero) const;

	/** Sends a fireball from the mouth at the aim target */
	void ReleaseFireball(AAmnesiaCharacter* Hero);

	/** True while the dragon turns to and spits at its target */
	bool IsAiming(float Now) const { return Now - AttackStartTime < 0.75f && AimTarget.IsValid(); }

	/** The body material, so the color can follow the element on the hero's sword */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyInstance;

	/** What the skills do to the fire: stronger and quicker */
	float SkillDamageMult = 1.0f;
	float SkillCooldownMult = 1.0f;

	/** Hue of the body material, a full color wheel from 0 to 1. 0 is the orange it comes in, 0.6 is blue */
	float CurrentHue = 0.0f;
	float TargetHue = 0.0f;

	/** The glow as it was built, before the lantern skill brightens it */
	float BaseGlowIntensity = 5.0f;
	float BaseGlowRadius = 650.0f;
	FLinearColor BaseGlowColor = FLinearColor(1.0f, 0.72f, 0.42f);
};
