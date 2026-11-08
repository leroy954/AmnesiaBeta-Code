#pragma once

#include "CoreMinimal.h"
#include "CombatCharacter.h"
#include "AmnesiaSound.h"
#include "AmnesiaWeapons.h"
#include "AmnesiaCharacter.generated.h"

class UAnimSequenceBase;
class UNiagaraComponent;
class USoundBase;
class UAmnesiaSkillComponent;
class UStaticMesh;
class UStaticMeshComponent;
class AAmnesiaChakram;

/**
 *  One sword swing in the ground combo. It plays a part of a longer animation.
 */
USTRUCT(BlueprintType)
struct FAmnesiaSlash
{
	GENERATED_BODY()

	/** Time in the animation where this swing starts, in animation seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash", meta = (ClampMin = 0))
	float StartTime = 0.0f;

	/** Time after the start when the hit is checked, in animation seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash", meta = (ClampMin = 0))
	float HitTime = 0.3f;

	/** How much of the animation the swing plays, in animation seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash", meta = (ClampMin = 0.05))
	float Duration = 0.6f;

	/** Playback speed of this swing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash", meta = (ClampMin = 0.1, ClampMax = 5))
	float PlayRate = 1.8f;

	/** Damage of this swing as a multiple of the hero's melee damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash", meta = (ClampMin = 0))
	float DamageScale = 1.0f;

	/** Reach of the hit check: forward and up from the hero */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash")
	FVector2D HitOffset = FVector2D(120.0f, 20.0f);

	/** Radius of the hit check */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash", meta = (ClampMin = 0))
	float HitRadius = 110.0f;

	/** A push forward at the start of the move, so a thrust steps into the enemy (cm/s, 0 = none) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash", meta = (ClampMin = 0))
	float Step = 0.0f;

	/** Only the upper body plays the swing, the legs keep standing even when the hero stands still */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash")
	bool bUpperBodyOnly = false;

	/** The animation this swing is a part of. Empty means the shared ground slash animation of the hero */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slash")
	TObjectPtr<UAnimSequenceBase> Animation;
};

/**
 *  2.5D player character for Amnesia.
 *  Builds on the melee combat character and adds:
 *  - Movement constrained to the side scrolling plane (XZ)
 *  - Fixed side view camera
 *  - Double jump with coyote time
 *  - Sprinting
 *  - A downward slash when attacking in the air, which bounces the hero off whatever it hits
 */
UCLASS(abstract)
class AAmnesiaCharacter : public ACombatCharacter
{
	GENERATED_BODY()

	/** Weapon held in the hand while fighting */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Weapon;

	/** The model of a weapon that is not the sword: it shows in the same hand when one of them is equipped */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* WeaponProp;

	/** Niagara aura on the weapon. Pick the effect in the Blueprint */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* WeaponAura;

	/** Skills, skill points, the element on the sword and the active skills */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UAmnesiaSkillComponent* Skills;

protected:

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SprintAction;

	/** Fly speed for the Fly command */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Fly", meta = (ClampMin = 0, Units = "cm/s"))
	float FlySpeed = 1500.0f;

	/** True while flying with the Fly command */
	bool bFlying = false;

	/** Bone or socket on the character mesh that holds the weapon */
	UPROPERTY(EditAnywhere, Category="Weapon")
	FName WeaponSocketName = FName("hand_r_socket");

	/** Bones to hide on the character mesh, like a sheathed copy of the weapon */
	UPROPERTY(EditAnywhere, Category="Weapon")
	TArray<FName> HiddenMeshBones;

	/** If true, the life bar floating over the character is hidden because the HUD shows it instead */
	UPROPERTY(EditAnywhere, Category="Damage")
	bool bHideWorldLifeBar = true;

	/** Distance from the character to the side view camera */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Camera", meta = (ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float SideCameraDistance = 900.0f;

	/** Height of the side view camera's focus point above the character */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Camera", meta = (ClampMin = -1000, ClampMax = 1000, Units = "cm"))
	float SideCameraHeight = 120.0f;

	/** Pitch of the side view camera. Negative values look down at the character */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Camera", meta = (ClampMin = -45, ClampMax = 45, Units = "deg"))
	float SideCameraPitch = -8.0f;

	/** Max amount of time that can pass since we walked off a ledge when we allow a regular jump */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Jump", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float MaxCoyoteTime = 0.16f;

	/** Run speed while sprinting */
	UPROPERTY(EditAnywhere, Category="Side Scrolling|Sprint", meta = (ClampMin = 0, ClampMax = 3000, Units = "cm/s"))
	float SprintSpeed = 750.0f;

	/** Animation played for the air slash. If unset, attacking in the air does a regular combo attack */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash")
	UAnimSequenceBase* AirSlashAnimation;

	/** Anim slot the air slash animation is played on */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash")
	FName AirSlashSlotName = FName("DefaultSlot");

	/** Where the swing starts in the animation, in animation seconds. The default is the overhead cut of the first ground swing:
	 *  the blade goes up, comes back over the head and cuts down in front of the hero (it comes down at about 0.38 to 0.46) */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash", meta = (ClampMin = 0))
	float AirSlashStartTime = 0.2f;

	/** How much of the animation the swing plays, in animation seconds. The blade swings back right after the cut,
	 *  so the swing ends soon after it */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash", meta = (ClampMin = 0.05))
	float AirSlashLength = 0.34f;

	/** Air slash animation play rate */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash", meta = (ClampMin = 0.1, ClampMax = 5))
	float AirSlashPlayRate = 1.0f;

	/** The part of the swing where the blade hurts: from and to, in animation seconds after the start. The blade is checked
	 *  every frame in between, so it only hits what it really passes through */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash")
	FVector2D AirSlashHitWindow = FVector2D(0.2f, 0.34f);

	/** How far from the blade something still counts as hit */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash", meta = (ClampMin = 0, ClampMax = 200, Units = "cm"))
	float AirSlashBladeRadius = 25.0f;

	/** Damage dealt by the air slash */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash", meta = (ClampMin = 0, ClampMax = 100))
	float AirSlashDamage = 2.0f;

	/** Upward speed the hero bounces with after the air slash hits something */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash", meta = (ClampMin = 0, ClampMax = 3000, Units = "cm/s"))
	float AirSlashBounceSpeed = 650.0f;

	/** Knockback impulse the air slash applies to what it hits */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Air Slash", meta = (ClampMin = 0, ClampMax = 3000, Units = "cm/s"))
	float AirSlashKnockback = 300.0f;

	/** Animation the ground sword swings are cut from. If unset, the template combo is used */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash")
	UAnimSequenceBase* GroundSlashAnimation;

	/** Anim slot the ground swings are played on */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash")
	FName GroundSlashSlotName = FName("DefaultSlot");

	/** The swings of the ground combo, in order. Attacking again right after the last one starts over */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash")
	TArray<FAmnesiaSlash> GroundSlashes;

	/** Time before a swing ends when the next attack press is accepted */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash", meta = (ClampMin = 0, Units = "s"))
	float SlashChainWindow = 0.25f;

	/** Pause after the last swing that still counts as the same combo */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash", meta = (ClampMin = 0, Units = "s"))
	float SlashComboResetTime = 0.7f;

	/** Knockback of the swings */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash", meta = (ClampMin = 0))
	float SlashKnockback = 260.0f;

	/** How close the weapon has to come to an enemy's body for a swing to hurt it. The weapon is the line from the hand to the tip */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash", meta = (ClampMin = 0, Units = "cm"))
	float SlashBodyTolerance = 25.0f;

	/** How long the left button has to be held for the heavy attack. A shorter press is a tap that swings the combo */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash", meta = (ClampMin = 0, Units = "s"))
	float HeavyHoldTime = 0.3f;

	/** A weapon that shoots a bolt (the wand) casts it on a tap too: faster than the held cast, and with this share of its damage */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash", meta = (ClampMin = 0.1))
	float BoltTapRateScale = 1.35f;

	UPROPERTY(EditAnywhere, Category="Melee Attack|Ground Slash", meta = (ClampMin = 0))
	float BoltTapDamageScale = 0.5f;

	/** While the right button guards, the hero takes this share of the damage and walks at this share of the speed (1 = the guard does not slow the walk) */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Guard", meta = (ClampMin = 0, ClampMax = 1))
	float GuardDamageMult = 0.5f;

	UPROPERTY(EditAnywhere, Category="Melee Attack|Guard", meta = (ClampMin = 0, ClampMax = 1))
	float GuardSpeedMult = 1.0f;

	/** The pose of the guard: the animation, the second of it that is held, and how fast it drifts on from there */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Guard")
	FString GuardAnimationPath = TEXT("/Game/FreeAnimationLibrary/Animations/Counter-Finisher/anim_Counter_Victim_01.anim_Counter_Victim_01");

	UPROPERTY(EditAnywhere, Category="Melee Attack|Guard", meta = (ClampMin = 0, Units = "s"))
	float GuardPoseTime = 0.26f;

	/** While guarding the hero turns to the nearest enemy within this reach, and stays turned to it */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Guard", meta = (ClampMin = 0, Units = "cm"))
	float GuardFacingRange = 1500.0f;

	/** Test helper: true here on the running hero starts the guard, false ends it, as the right button does */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	bool bDebugGuard = false;

	/** Test helper: set to true on the running hero to play the swing below once */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	bool bDebugSlash = false;

	/** Test helper: start time, animation seconds and play rate of the debug swing */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	FVector DebugSlashStartDurationRate = FVector(0.0f, 1.0f, 1.0f);

	/** Test helper: set to true on the running hero to do an air slash, even standing on the ground */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	bool bDebugAirSlash = false;

	/** Test helper: while true, the place of the blade is written to the log every frame, relative to the hero (forward is plus) */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	bool bDebugLogBlade = false;

	/** Test helper: set to the id of a weapon (chakram, axe, wand, sword) on the running hero to put it in the hand, if it is owned */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	FName DebugWeapon;

	/** Test helpers: true here on the running hero swings the weapon: the next swing of the ground combo, or the heavy attack, as if the button was pressed */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	bool bDebugGroundSlash = false;

	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	bool bDebugHeavySlash = false;

	/** Test helper: true here on the running hero presses the left button and keeps it down: after the hold time the heavy attack comes */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Debug")
	bool bDebugHoldAttack = false;

	/** Index of the current swing in the combo */
	int32 SlashIndex = 0;

	/** True while a ground swing plays */
	bool bGroundSlashing = false;

	/** Game time the current swing ends, or the last one ended */
	float SlashEndTime = -1000.0f;

	/** Montage playing the swing */
	UPROPERTY()
	TObjectPtr<UAnimMontage> SlashMontage;

	/** Swing timers */
	FTimerHandle SlashHitTimer;
	FTimerHandle SwingSoundTimer;
	FTimerHandle DormancyTimer;

	/** Enemies farther than this along the level sleep until the hero comes closer */
	UPROPERTY(EditAnywhere, Category="Performance", meta = (Units = "cm"))
	float EnemyWakeRange = 5500.0f;

	/** Wakes the enemies near the hero and puts the far ones to sleep */
	void UpdateEnemyDormancy();
	FTimerHandle SlashEndTimer;

	/** Damage scale and the swing being resolved */
	FAmnesiaSlash CurrentSlash;
	float CurrentSlashDamageScale = 1.0f;

	/** The weapon in the hand: an id of the weapon table, none is the sword */
	FName EquippedWeapon;

	/** What the Blueprint gave the sword, kept to put the sword back: the place of the aura on it, and its swings */
	FTransform SwordAuraRelative;

	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> SwordGroundAnimation;

	TArray<FAmnesiaSlash> SwordGroundSlashes;

	/** The heavy swing of the weapon in the hand, when it has one of its own */
	FAmnesiaSlash WeaponHeavySlash;
	bool bHasWeaponHeavy = false;

	/** What the heavy swing that is playing does when it hits: the weapon's special, once it is learned */
	EAmnesiaWeaponSpecial CurrentSpecial = EAmnesiaWeaponSpecial::None;

	/** The swing that is playing is the heavy cast of a bolt weapon: it shoots instead of hitting */
	bool bBoltSwing = false;

	/** The swing that is playing throws the chakram, and whether it is the held throw */
	bool bDiscSwing = false;
	bool bDiscHeavy = false;

	/** The chakrams in the air. While as many are out as the hero may throw at once, the hand is empty and the button does nothing */
	TArray<TWeakObjectPtr<AAmnesiaChakram>> ThrownChakrams;
	bool bAuraHiddenForThrow = false;

	/** How many chakrams are in the air now (the lost ones are dropped from the list) */
	int32 CountThrownChakrams();

	/** How many chakrams may be in the air at once: one, and one more for each rank of its skill */
	int32 GetMaxChakrams() const;

	/** How far the chakram flies on a tap and on the held throw, how fast, and how long the held throw whirls (with its skill) */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Chakram", meta = (ClampMin = 0, Units = "cm"))
	float ChakramTapRange = 750.0f;

	UPROPERTY(EditAnywhere, Category="Melee Attack|Chakram", meta = (ClampMin = 0, Units = "cm"))
	float ChakramHeavyRange = 1100.0f;

	UPROPERTY(EditAnywhere, Category="Melee Attack|Chakram", meta = (ClampMin = 0, Units = "cm/s"))
	float ChakramSpeed = 1800.0f;

	UPROPERTY(EditAnywhere, Category="Melee Attack|Chakram", meta = (ClampMin = 0, Units = "s"))
	float ChakramWhirlTime = 1.5f;

	/** True while the right button guards, the enemy the hero faces then, and how the hero turned before (it does not follow the walking while guarding) */
	bool bGuarding = false;

	/** True while the right button is held down, so the guard can start again after a jump, a hit or a swing */
	bool bGuardHeld = false;
	TWeakObjectPtr<AAmnesiaEnemy> GuardTarget;
	float NextGuardTargetTime = 0.0f;
	bool bTurnedWithMovement = true;

	/** The left button is down and has not become a swing yet: a release swings the combo, holding it long enough is the heavy attack */
	bool bAttackPending = false;
	bool bAttackButtonDown = false;
	float AttackPressTime = 0.0f;

	/** The loaded animation of the guard pose */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> GuardAnimation;

	/** Animations of the weapons that were loaded, kept alive */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedWeaponAssets;

	/** Last recorded time when this character walked off a ledge */
	float LastFallTime = 0.0f;

	/** Walk speed before sprinting */
	float BaseWalkSpeed = 0.0f;

	/** If true, the character walked off a ledge and may still do a coyote time jump */
	bool bCanCoyoteJump = false;

	/** If true, this character has already double jumped */
	bool bHasDoubleJumped = false;

	/** If true, the sprint input is held */
	bool bIsSprinting = false;

	/** Highest level the hero can reach */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 1))
	int32 MaxLevel = 10;

	/** XP needed to go from level 1 to level 2 */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 1))
	int32 FirstLevelXP = 100;

	/** Each level needs this many times the XP of the previous one */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 1.0))
	float LevelXPMultiplier = 1.5f;

	/** Max HP gained per level */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 0))
	float HPPerLevel = 2.0f;

	/** Melee and air slash damage gained per level, as a fraction of the base damage */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 0))
	float DamageBonusPerLevel = 0.25f;

	/** Level 1 stats */
	float BaseMaxHP = 5.0f;
	float BaseMeleeDamage = 1.0f;
	float BaseAirSlashDamage = 2.0f;

	/** Current level */
	int32 Level = 1;

	/** XP gained towards the next level */
	int32 CurrentXP = 0;

	/** Sword swings */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet SwingSounds;

	/** Thud layered under the enemy's own impact sound when a swing lands */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet HitSounds;

	/** The hero gets hurt */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet HurtSounds;

	/** The hero dies */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet DeathSounds;

	/** Footsteps, played in step with the run. Mood volumes can swap them for the ground of their zone */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet FootstepSounds;

	/** Jump and landing */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet JumpSounds;

	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet LandSounds;

	/** Level up fanfare */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet LevelUpSounds;

	/** Healing, from orbs and rest places */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet HealSounds;

	/** Footsteps of the zone the hero is in, used instead of FootstepSounds when set */
	FAmnesiaSoundSet ZoneFootstepSounds;

	/** Anim Blueprint switched in when play starts. A child of UAmnesiaHeroAnimInstance, it lets sword swings play over running legs.
	 *  The mesh itself keeps its plain Anim Blueprint, so loading the hero never depends on this one */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSoftClassPtr<UAnimInstance> HeroAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Amnesia/Animation/ABP_Arden.ABP_Arden_C")));

	/** The little dragon that flies with the hero, spawned when the level starts. Clear it to play alone */
	UPROPERTY(EditAnywhere, Category="Companion")
	TSubclassOf<class AAmnesiaCompanion> CompanionClass;

	/** Time between footsteps at walking speed */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound", meta = (ClampMin = 0.05, Units = "s"))
	float FootstepInterval = 0.42f;

	/** How long the world freezes when a swing lands. 0 turns it off */
	UPROPERTY(EditAnywhere, Category="Feedback|Hit", meta = (ClampMin = 0, Units = "s"))
	float HitStopTime = 0.07f;

	/** Camera shake strength when the hero lands a hit and when the hero gets hit */
	UPROPERTY(EditAnywhere, Category="Feedback|Hit", meta = (ClampMin = 0))
	float HitShakeStrength = 7.0f;

	UPROPERTY(EditAnywhere, Category="Feedback|Hit", meta = (ClampMin = 0))
	float HurtShakeStrength = 14.0f;

	/** Seconds the hero can't be hurt again after a hit */
	UPROPERTY(EditAnywhere, Category="Damage", meta = (ClampMin = 0, Units = "s"))
	float HurtInvulnerabilityTime = 0.6f;

	/** Current shake strength, decays over time */
	float ShakeStrength = 0.0f;

	/** Time left until the next footstep */
	float FootstepCountdown = 0.0f;

	/** Game time until which the hero ignores damage after being hit */
	float HurtImmuneUntil = 0.0f;

	/** Actors frozen by the current hit stop */
	TArray<TWeakObjectPtr<AActor>> HitStopActors;

	/** Ends the hit stop */
	FTimerHandle HitStopTimer;

	/** If true, an air slash is in progress */
	bool bIsAirSlashing = false;

	/** True while the blade of the air slash can hurt */
	bool bAirSlashHitActive = false;

	/** True once the air slash has bounced the hero off something */
	bool bAirSlashBounced = false;

	/** Targets the current air slash has hurt, each one only once */
	TArray<TWeakObjectPtr<AActor>> AirSlashHitActors;

	/** Where the blade was at the last check, so a fast swing can't jump over a target */
	FVector LastBladeBase = FVector::ZeroVector;
	FVector LastBladeTip = FVector::ZeroVector;
	bool bHasLastBlade = false;

	/** The ends of the blade in the weapon's own space, found from the sword mesh when play starts */
	FVector BladeBaseLocal = FVector::ZeroVector;
	FVector BladeTipLocal = FVector(0.0f, 100.0f, 0.0f);

	/** Montage of the air slash, when the hero's own anim instance isn't there to lay it over the legs */
	UPROPERTY()
	TObjectPtr<UAnimMontage> AirSlashMontage;

	/** Air slash timers: the blade starts to hurt, stops hurting, and another slash may start */
	FTimerHandle AirSlashHitTimer;
	FTimerHandle AirSlashHitEndTimer;
	FTimerHandle AirSlashEndTimer;

public:

	/** Constructor */
	AAmnesiaCharacter();

protected:

	/** Initialization */
	virtual void BeginPlay() override;

	/** Cleanup */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Handles input bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:

	/** Resets the jump and air slash state when landing */
	virtual void Landed(const FHitResult& Hit) override;

	/** Keeps track of ledges we walk off for coyote time jumps */
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/** Moves along the side scrolling axis only */
	virtual void DoMove(float Right, float Forward) override;

	/** The side view camera is fixed, so look input is ignored */
	virtual void DoLook(float Yaw, float Pitch) override;

	/** Air slashes while falling, otherwise sword swings */
	virtual void DoComboAttackStart() override;

	/** The right button guards while it is held: the hero takes a share of the damage */
	virtual void DoChargedAttackStart() override;

	virtual void DoChargedAttackEnd() override;

	/** Runs the debug swing */
	virtual void Tick(float DeltaTime) override;

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump released inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Handles sprint pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** Handles sprint released inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintEnd();

	/** Console command: "Fly" toggles flying. While flying, A/D move sideways and W/S go up and down. "Fly 1" turns it on, "Fly 0" off */
	UFUNCTION(Exec)
	void Fly(int32 Enabled = -1);

	/** Returns the current HP */
	UFUNCTION(BlueprintPure, Category="Damage")
	float GetHealth() const { return CurrentHP; }

	/** Returns the max HP */
	UFUNCTION(BlueprintPure, Category="Damage")
	float GetMaxHealth() const { return MaxHP; }

	/** Returns the current level */
	UFUNCTION(BlueprintPure, Category="Level")
	int32 GetHeroLevel() const { return Level; }

	/** Returns the sword damage of one swing at the current level */
	UFUNCTION(BlueprintPure, Category="Damage")
	float GetMeleeDamage() const { return MeleeDamage; }

	/** Returns the XP gained towards the next level */
	UFUNCTION(BlueprintPure, Category="Level")
	int32 GetCurrentXP() const { return CurrentXP; }

	/** Returns the XP needed to finish the current level. 0 at the max level */
	UFUNCTION(BlueprintPure, Category="Level")
	int32 GetXPToNextLevel() const;

	/** Returns true at the max level */
	UFUNCTION(BlueprintPure, Category="Level")
	bool IsMaxLevel() const { return Level >= MaxLevel; }

	/** Returns the highest level the hero can reach */
	UFUNCTION(BlueprintPure, Category="Level")
	int32 GetMaxLevel() const { return MaxLevel; }

	/** Adds XP and levels up as many times as the XP allows */
	UFUNCTION(BlueprintCallable, Category="Level")
	void GainXP(int32 Amount);

	/** Adds gold to the purse, which is kept in the saved progress */
	void GainGold(int32 Amount);

	/** Console command: "Lvl 10" sets the level, resets the XP and heals */
	UFUNCTION(Exec)
	void Lvl(int32 NewLevel);

	/** Console command: "Gold 500" puts gold in the purse, a negative number takes it out */
	UFUNCTION(Exec)
	void Gold(int32 Amount);

	/** Console command: "InfinitePoints 1" makes the skill points endless for testing, 0 puts them back */
	UFUNCTION(Exec)
	void InfinitePoints(int32 On);

	/** Heals up to the max HP and shows the number. A quiet heal makes no sound and shows no number */
	void HealBy(float Amount, bool bQuiet = false);

	/** Heals to full */
	void HealFully();

	/** The skill component */
	UAmnesiaSkillComponent* GetSkills() const { return Skills; }

	/** The aura on the weapon in the hand */
	UNiagaraComponent* GetWeaponAura() const { return WeaponAura; }

	/** The weapon in the hand, or null for the sword. The sword is the weapon of the Blueprint, the others come from the weapon table */
	const FAmnesiaWeaponDef* GetEquippedWeaponDef() const;

	FName GetEquippedWeaponId() const
	{
		return EquippedWeapon.IsNone() ? FAmnesiaWeapons::SwordId() : EquippedWeapon;
	}

	/** The model of the weapon in the hand if it is not the sword. The aura draws its glow over this shape */
	UStaticMesh* GetWeaponPropMesh() const;

	/** How big the weapon model in the hand is drawn (its world scale), so a thrown chakram can fly at the size it had in the hand */
	float GetWeaponPropScale() const;

	/** Puts a weapon in the hand. False if it is not owned. The sword is always owned */
	bool EquipWeapon(FName Id);

	/** The next weapon of the ones that are owned */
	void CycleWeapon();

	/** Takes the weapon from the saved progress again, after a new game started or the hero respawned */
	void ReloadWeaponFromProgress();

	/** The skills changed: max HP, damage, speed and the dragon follow */
	void OnSkillsChanged();

	/** The dragon that flies with the hero, made if it is not there yet. Null if there is no dragon class */
	class AAmnesiaCompanion* SpawnCompanion();

	/** The dragon was just bought: it appears next to the hero and takes the skills */
	void SummonDragon();

	/** Sounds of the skills */
	void PlayLevelUpSound() { LevelUpSounds.Play2D(this); }
	void PlayCastSound() { SwingSounds.PlayAt(this, GetActorLocation()); }

	/** Opens the pause menu */
	void OnPausePressed();

	/** Keys of the skills: the two active skills, the element on the sword and the skill screen */
	void OnFlameBurstPressed();
	void OnFrostNovaPressed();
	void OnCycleElementPressed();
	void OnSkillScreenPressed();

	/** Opens the shop */
	void OnShopPressed();

	/** Pets the cat that is in reach, if there is one */
	void OnPetPressed();

	/** Changes the weapon in the hand */
	void OnCycleWeaponPressed();

	/** Opens or closes the admin panel */
	void OnAdminPressed();

	/** True while flying with the Fly command */
	bool IsFlying() const { return bFlying; }

	/** Hides the level up banner, for a fresh start */
	void ResetLevelUpBanner() { LastLevelUpTime = -100.0f; }

	/** Restores HP through the combat damageable interface */
	virtual void ApplyHealing(float Healing, AActor* Healer) override;

	/** Plays the death sound, saves the progress and shows the death screen */
	virtual void HandleDeath() override;

	/** Shakes the camera, scaled by the screen shake setting */
	void AddCameraShake(float Strength);

	/** Freezes the hero and a hit target for a moment, so hits land with weight */
	void StartHitStop(AActor* Target);

	/** Ends the hit stop */
	void EndHitStop();

	/** Called when a hero attack lands: hit stop, shake, sound */
	void OnAttackLanded(AActor* Target, const FVector& ImpactPoint, float Damage);

	/** A hit of the thrown chakram, in the hero's name: damage, the number, and what the skills add */
	void OnChakramHit(AAmnesiaEnemy* Enemy, float Damage, const FVector& Point, float Direction);

	/** The chakram is back in the hand */
	void OnChakramCaught(AAmnesiaChakram* Disc);

	/** Console command: "Lvl 10"-like helper that restarts at a checkpoint index. "Checkpoint 3" */
	UFUNCTION(Exec)
	void Checkpoint(int32 Index);

	/** Console command: "God" toggles invulnerability. "God 1" turns it on, "God 0" turns it off */
	UFUNCTION(Exec)
	void God(int32 Enabled = -1);

	/** Ignores damage while invulnerable */
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;

	/** Returns true while invulnerable */
	UFUNCTION(BlueprintPure, Category="Damage")
	bool IsInvulnerable() const { return bInvulnerable; }

	/** Tilts the side camera to a new pitch, blending over time. Positive values look up, which shows more sky */
	UFUNCTION(BlueprintCallable, Category="Side Scrolling|Camera")
	void SetCameraPitch(float NewPitch, float BlendSpeed = 1.5f);

	/** Swaps the footsteps for the ground of a zone. An empty set goes back to the default steps */
	void SetZoneFootsteps(const FAmnesiaSoundSet& Footsteps) { ZoneFootstepSounds = Footsteps; }

	/** Time in seconds since the last level up, used by the HUD */
	float GetTimeSinceLevelUp() const;

protected:

	/** Sets max HP and damage for the current level and heals fully */
	void ApplyLevelStats();

	/** Sets max HP and damage from the level and the skills, without healing */
	void RecomputeStats();

	/** Camera pitch the boom is moving towards, and the one it shows now */
	float TargetCameraPitch = -8.0f;
	float CurrentCameraPitch = -8.0f;
	float CameraPitchBlendSpeed = 1.5f;

	/** If true, the hero takes no damage */
	bool bInvulnerable = false;

	/** World time of the last level up */
	float LastLevelUpTime = -100.0f;

	/** Points the camera boom at the character from the side */
	void SetupSideCamera();

	/** Plays the next swing of the ground combo */
	void GroundSlash(bool bHeavy);

	/** Plays part of an animation as a swing */
	void PlaySlash(const FAmnesiaSlash& Slash, float RateScale, float DamageScale);

	/** Called from a timer to check the swing hit */
	void SlashHit();

	/** Called from a timer when the swing ends */
	void SlashEnded();

	/** Starts the air slash */
	void AirSlash();

	/** Called from a timer when the blade of the air slash starts to hurt */
	void AirSlashHitBegin();

	/** Called from a timer when the blade of the air slash stops hurting */
	void AirSlashHitStop();

	/** Checks the blade against what it passes through. Runs every frame while it hurts */
	void AirSlashTrace();

	/** Called from a timer when the air slash may be used again */
	void AirSlashEnded();

	/** Finds the ends of the blade in the weapon's space from the sword mesh */
	void FindBlade();

	/** Shows the weapon of EquippedWeapon: its model, aura, swings and numbers. A change that the player made is announced */
	void ApplyWeapon(bool bAnnounce);

	/** The numbers of the weapon in the hand with the skills of its branch: damage, speed, reach and how fat the swing is */
	float GetWeaponDamageMult() const;
	float GetWeaponRateMult() const;
	float GetWeaponReachMult() const;
	float GetWeaponRadiusMult() const;

	/** What a heavy swing does with the weapon in the hand, if its last skill is learned */
	EAmnesiaWeaponSpecial GetUnlockedSpecial() const;

	/** What a hit of the weapon in the hand adds: the axe's bleeding, the wand's splash */
	void OnWeaponHit(AAmnesiaEnemy* Enemy, float Damage);

	/** The heavy attack of a weapon: the quake, the blast (the chakram whirls by itself) */
	void PerformSpecial(EAmnesiaWeaponSpecial Special, float Facing);

	/** Throws the wand's magic bolt forward, with the weapon's damage */
	void ShootBolt(float Facing);

	/** Throws the chakram forward from the hand */
	void ThrowChakram(float Facing);

	/** True if the weapon, the line from Start to End, comes within SlashBodyTolerance of the body that the sweep found */
	bool SlashTouchesBody(const FHitResult& Hit, const FVector& Start, const FVector& End) const;

	/** The heavy attack: the weapon's heavy swing on the ground, the air slash in the air */
	void HeavyAttack();

	/** The left button was let go: a tap that is still waiting swings the combo */
	void OnAttackReleased();

	/** Raises the guard with its pose, and lowers it again */
	void StartGuard();
	void EndGuard();

	/** Turns the hero to the nearest enemy while guarding. The enemy is looked for a few times a second, or at once if forced */
	void UpdateGuardFacing(bool bForce);

public:
	bool IsGuarding() const { return bGuarding; }
};
