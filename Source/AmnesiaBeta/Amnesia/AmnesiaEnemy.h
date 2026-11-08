#pragma once

#include "CoreMinimal.h"
#include "CombatEnemy.h"
#include "AmnesiaSound.h"
#include "AmnesiaEnemy.generated.h"

class UAnimSequenceBase;
class UStaticMesh;
class USoundBase;
class UMaterialInstanceDynamic;
class UAmnesiaCreatureAnimInstance;
class UNiagaraComponent;
class UNiagaraSystem;
class AAmnesiaFrost;

/**
 *  One single animation attack, with the moment its hit is checked
 */
USTRUCT(BlueprintType)
struct FAmnesiaEnemyAttack
{
	GENERATED_BODY()

	/** Attack animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	TObjectPtr<UAnimSequenceBase> Animation;

	/** Time into the animation when the hit is checked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float HitTime = 0.5f;

	/** Bone or socket the hit check starts from */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack")
	FName SourceBone;

	/** Animation play rate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack", meta = (ClampMin = 0.1, ClampMax = 5))
	float PlayRate = 1.0f;
};

/**
 *  A static mesh attached to a socket of the enemy, like a hammer or a shield
 */
USTRUCT(BlueprintType)
struct FAmnesiaEnemyProp
{
	GENERATED_BODY()

	/** Mesh to attach */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop")
	TObjectPtr<UStaticMesh> Mesh;

	/** Socket or bone of the enemy mesh to attach it to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop")
	FName Socket;

	/** Offset from the socket */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop")
	FTransform Offset;
};

/**
 *  2.5D melee enemy for Amnesia.
 *  Uses a lightweight side scrolling brain instead of StateTree:
 *  - Patrols back and forth around its starting point
 *  - Chases the player when they get close
 *  - Attacks when the player is within range
 *  Works with the combo montage of an Anim Blueprint, or with plain animation sequences
 *  through the creature anim instance.
 */
UCLASS(abstract)
class AAmnesiaEnemy : public ACombatEnemy
{
	GENERATED_BODY()

protected:

	/** Max distance the enemy walks away from its starting point while patrolling */
	UPROPERTY(EditAnywhere, Category="AI|Patrol", meta = (ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float PatrolDistance = 400.0f;

	/** Walk speed while patrolling */
	UPROPERTY(EditAnywhere, Category="AI|Patrol", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float PatrolSpeed = 150.0f;

	/** Time to wait at each end of the patrol path */
	UPROPERTY(EditAnywhere, Category="AI|Patrol", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float PatrolWaitTime = 1.5f;

	/** Time the enemy may be blocked by a wall or ledge before it turns around */
	UPROPERTY(EditAnywhere, Category="AI|Patrol", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float BlockedTurnTime = 0.3f;

	/** Walk speed while chasing the player */
	UPROPERTY(EditAnywhere, Category="AI|Chase", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm/s"))
	float ChaseSpeed = 300.0f;

	/** Horizontal distance at which the enemy notices the player */
	UPROPERTY(EditAnywhere, Category="AI|Chase", meta = (ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float AggroRange = 900.0f;

	/** Horizontal distance at which the enemy gives up the chase */
	UPROPERTY(EditAnywhere, Category="AI|Chase", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float LoseAggroRange = 1500.0f;

	/** Max height difference at which the enemy notices the player */
	UPROPERTY(EditAnywhere, Category="AI|Chase", meta = (ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float AggroHeight = 400.0f;

	/** Horizontal distance at which the enemy starts attacking */
	UPROPERTY(EditAnywhere, Category="AI|Attack", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float AttackRange = 150.0f;

	/** Time to wait between attacks */
	UPROPERTY(EditAnywhere, Category="AI|Attack", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float AttackCooldown = 1.5f;

	/** Time the enemy stays stunned after being hit */
	UPROPERTY(EditAnywhere, Category="AI|Attack", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float HitStunTime = 0.6f;

	/** Single animation attacks, one is picked at random. If empty, the combo attack montage is used instead */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Single Animation")
	TArray<FAmnesiaEnemyAttack> Attacks;

	/** Anim slot used to play single animation attacks through an Anim Blueprint */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Single Animation")
	FName AttackSlotName = FName("DefaultSlot");

	/** Idle animation for the creature anim instance */
	UPROPERTY(EditAnywhere, Category="Animation|Creature")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	/** Walk animation for the creature anim instance, used while patrolling */
	UPROPERTY(EditAnywhere, Category="Animation|Creature")
	TObjectPtr<UAnimSequenceBase> WalkAnimation;

	/** Run animation for the creature anim instance, used while chasing */
	UPROPERTY(EditAnywhere, Category="Animation|Creature")
	TObjectPtr<UAnimSequenceBase> RunAnimation;

	/** Ground speed the walk animation was made for */
	UPROPERTY(EditAnywhere, Category="Animation|Creature", meta = (ClampMin = 1, Units = "cm/s"))
	float WalkAnimationSpeed = 150.0f;

	/** Ground speed the run animation was made for */
	UPROPERTY(EditAnywhere, Category="Animation|Creature", meta = (ClampMin = 1, Units = "cm/s"))
	float RunAnimationSpeed = 350.0f;

	/** Hit reactions, one is picked at random. When set, they replace the partial ragdoll on hit */
	UPROPERTY(EditAnywhere, Category="Animation|Creature")
	TArray<TObjectPtr<UAnimSequenceBase>> HitAnimations;

	/** Death animation. When set, it replaces the death ragdoll */
	UPROPERTY(EditAnywhere, Category="Animation|Creature")
	TObjectPtr<UAnimSequenceBase> DeathAnimation;

	/** If true, the floating life bar widget is hidden because the HUD draws it instead */
	UPROPERTY(EditAnywhere, Category="Damage")
	bool bHideWorldLifeBar = true;

	/** Level of this enemy. Higher levels have more HP, hit harder and give more XP. Set it per enemy in the level */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 1, ClampMax = 20))
	int32 EnemyLevel = 1;

	/** Extra max HP per level above 1, as a fraction of the base HP */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 0))
	float HPBonusPerLevel = 0.25f;

	/** Extra melee damage per level above 1, as a fraction of the base damage */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 0))
	float DamageBonusPerLevel = 0.15f;

	/** Toughness of every enemy, on top of the level scaling. These multiply the values of the Blueprint when the game starts,
	 *  so they hold for every placed enemy. Max HP and damage first, then how fast the enemy attacks, recovers from a hit and chases */
	UPROPERTY(EditAnywhere, Category="Level|Difficulty", meta = (ClampMin = 0.1))
	float HPMultiplier = 1.8f;

	UPROPERTY(EditAnywhere, Category="Level|Difficulty", meta = (ClampMin = 0.1))
	float DamageMultiplier = 1.25f;

	/** Added to HPBonusPerLevel and DamageBonusPerLevel: the deeper enemies grow tougher faster than the hero's sword */
	UPROPERTY(EditAnywhere, Category="Level|Difficulty", meta = (ClampMin = 0))
	float ExtraHPBonusPerLevel = 0.2f;

	UPROPERTY(EditAnywhere, Category="Level|Difficulty", meta = (ClampMin = 0))
	float ExtraDamageBonusPerLevel = 0.08f;

	/** Seconds between attacks and the stun after a hit are multiplied by these: lower is more aggressive */
	UPROPERTY(EditAnywhere, Category="Level|Difficulty", meta = (ClampMin = 0.1))
	float AttackCooldownScale = 0.75f;

	UPROPERTY(EditAnywhere, Category="Level|Difficulty", meta = (ClampMin = 0.1))
	float HitStunScale = 0.55f;

	UPROPERTY(EditAnywhere, Category="Level|Difficulty", meta = (ClampMin = 0.1))
	float ChaseSpeedScale = 1.12f;

	/** Tougher enemies are worth more */
	UPROPERTY(EditAnywhere, Category="Level|Difficulty", meta = (ClampMin = 0.1))
	float XPMultiplier = 1.5f;

	/** Chance to drop healing orbs on death */
	UPROPERTY(EditAnywhere, Category="Loot", meta = (ClampMin = 0, ClampMax = 1))
	float HealDropChance = 0.4f;

	/** Healing orbs dropped */
	UPROPERTY(EditAnywhere, Category="Loot", meta = (ClampMin = 1))
	int32 HealDropCount = 1;

	/** What the sword sounds like hitting this enemy: wood, bone or metal */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet HitSounds;

	/** Pain grunts, played on some of the hits */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet HurtSounds;

	/** Chance a hit also plays a pain grunt */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound", meta = (ClampMin = 0, ClampMax = 1))
	float HurtSoundChance = 0.5f;

	/** Death cry */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet DeathSounds;

	/** The body hitting the ground, a moment after the death cry */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet DeathFallSounds;

	/** Seconds from the death cry to the fall */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound", meta = (ClampMin = 0, Units = "s"))
	float DeathFallDelay = 0.7f;

	/** Battle cry at the start of an attack */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet AttackSounds;

	/** Heavy steps while walking */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound")
	FAmnesiaSoundSet FootstepSounds;

	/** Time between steps at walking speed */
	UPROPERTY(EditAnywhere, Category="Feedback|Sound", meta = (ClampMin = 0.1, Units = "s"))
	float FootstepInterval = 0.6f;

	/** Counts down to the next step */
	float FootstepCountdown = 0.0f;

	FTimerHandle DeathFallTimer;

	/** White flash overlay shown for a moment on every hit */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlashMaterial;

	FTimerHandle FlashTimer;

	/** Extra XP reward per level above 1, as a fraction of the base reward */
	UPROPERTY(EditAnywhere, Category="Level", meta = (ClampMin = 0))
	float XPBonusPerLevel = 0.25f;

	/** Static meshes attached to sockets when the game starts, like a hammer and a shield */
	UPROPERTY(EditAnywhere, Category="Appearance")
	TArray<FAmnesiaEnemyProp> AttachedProps;

	/** Test helper: set an animation here and DebugPlayAnimation to true on a running enemy to play it once */
	UPROPERTY(EditAnywhere, Category="Animation|Debug")
	TObjectPtr<UAnimSequenceBase> DebugAnimation;

	/** Test helper: plays the debug animation */
	UPROPERTY(EditAnywhere, Category="Animation|Debug")
	bool bDebugPlayAnimation = false;

	/** The enemy is scaled by this when the game starts. Attack range grows with it */
	UPROPERTY(EditAnywhere, Category="Appearance", meta = (ClampMin = 0.1))
	float SizeScale = 1.3f;

	/** XP the hero gets for killing this enemy */
	UPROPERTY(EditAnywhere, Category="Damage", meta = (ClampMin = 0))
	int32 XPReward = 15;

	/** For enemies without attack animations: how long the body lunge lasts. 0 turns the lunge off */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Lunge", meta = (ClampMin = 0, Units = "s"))
	float LungeDuration = 0.0f;

	/** Time into the lunge when the hit is checked */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Lunge", meta = (ClampMin = 0, Units = "s"))
	float LungeHitTime = 0.4f;

	/** How far the body leans forward at the strike, in degrees. It leans back a little first */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Lunge", meta = (ClampMin = 0, ClampMax = 80, Units = "deg"))
	float LungePitch = 25.0f;

	/** How far the body steps forward at the strike */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Lunge", meta = (ClampMin = 0, Units = "cm"))
	float LungeStep = 50.0f;

	/** Bone or socket the lunge hit is swept from. The mesh origin is used if it doesn't exist */
	UPROPERTY(EditAnywhere, Category="Melee Attack|Lunge")
	FName LungeSourceBone = FName("hand_r");

	/** Game time the current lunge started. Negative when there's none */
	float LungeStartTime = -1000.0f;

	/** True while the mesh is away from its rest pose */
	bool bLungePoseDirty = false;

	/** Mesh transform before the lunge */
	FQuat MeshBaseRotation = FQuat::Identity;
	FVector MeshBaseLocation = FVector::ZeroVector;

	/** Montage currently playing a single animation attack through an Anim Blueprint */
	UPROPERTY()
	TObjectPtr<UAnimMontage> SingleAttackMontage;

	/** Attack currently being played */
	FAmnesiaEnemyAttack CurrentAttack;

	/** Single animation attack hit timer */
	FTimerHandle AttackHitTimer;

	/** Single animation attack end timer, used with the creature anim instance */
	FTimerHandle AttackEndTimer;

	/** Horizontal position the patrol path is centered on */
	float HomeX = 0.0f;

	/** Where the enemy stood when the game started */
	FVector HomeLocation = FVector::ZeroVector;

	/** Current patrol direction. 1 walks towards +X, -1 towards -X */
	float PatrolDirection = 1.0f;

	/** Game time until which the enemy waits instead of patrolling */
	float PatrolWaitEndTime = 0.0f;

	/** Time the enemy has been blocked while trying to patrol */
	float BlockedTime = 0.0f;

	/** Game time of the last attack */
	float LastAttackTime = -1000.0f;

	/** Game time until which the enemy is stunned */
	float StunEndTime = 0.0f;

	/** If true, the enemy is currently chasing the player */
	bool bIsChasing = false;

	/** The flames on a burning or frozen enemy: its height divided by this is the size of the biggest, how many of them there are, how big they
	 *  are next to that, how long each burns and how fast */
	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 1))
	float StatusFxHeightDivisor = 90.0f;

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0, ClampMax = 6))
	int32 StatusFlameCount = 2;

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0.05))
	float StatusFlameScale = 0.4f;

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0.2, Units = "s"))
	float StatusFlameLifetime = 1.4f;

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0.1))
	float StatusFlameSpeed = 1.2f;

	/** How wide and how tall the flames are against their usual shape */
	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0.2))
	float StatusFlameWidth = 0.7f;

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0.2))
	float StatusFlameHeight = 1.2f;

	/** The flash when the enemy is hit: a bright glow along the edge of the body with a light tint over it, and how long it lasts */
	UPROPERTY(EditAnywhere, Category="Hit Flash")
	FLinearColor HitFlashColor = FLinearColor(1.0f, 0.92f, 0.8f);

	UPROPERTY(EditAnywhere, Category="Hit Flash", meta = (ClampMin = 0, ClampMax = 1))
	float HitFlashOpacity = 0.3f;

	UPROPERTY(EditAnywhere, Category="Hit Flash")
	float HitFlashRimBoost = 5.0f;

	UPROPERTY(EditAnywhere, Category="Hit Flash", meta = (ClampMin = 0.02, Units = "s"))
	float HitFlashTime = 0.1f;

	/** The glow over the body while burning, chilled or frozen: the color of the glow (an HDR color, the rim is brighter than it) and how much of it shows
	 *  (0 to 1). The rim is where the body turns away from the camera: how bright it is, and how thin (a higher power is a thinner rim) */
	UPROPERTY(EditAnywhere, Category="Status")
	FLinearColor BurnTint = FLinearColor(0.9f, 0.28f, 0.04f);

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0, ClampMax = 1))
	float BurnTintOpacity = 0.35f;

	UPROPERTY(EditAnywhere, Category="Status")
	float BurnRimBoost = 4.0f;

	UPROPERTY(EditAnywhere, Category="Status")
	FLinearColor ChillTint = FLinearColor(0.3f, 0.6f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0, ClampMax = 1))
	float ChillTintOpacity = 0.3f;

	UPROPERTY(EditAnywhere, Category="Status")
	float ChillRimBoost = 3.0f;

	UPROPERTY(EditAnywhere, Category="Status")
	FLinearColor FreezeTint = FLinearColor(0.7f, 0.95f, 1.15f);

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0, ClampMax = 1))
	float FreezeTintOpacity = 0.7f;

	UPROPERTY(EditAnywhere, Category="Status")
	float FreezeRimBoost = 3.0f;

	/** Hits on a frozen enemy hurt this many times more, and nothing pushes it around */
	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 1))
	float FrozenDamageMult = 1.35f;

	/** The red glow of a bleeding wound */
	UPROPERTY(EditAnywhere, Category="Status")
	FLinearColor BleedTint = FLinearColor(0.85f, 0.04f, 0.04f);

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0, ClampMax = 1))
	float BleedTintOpacity = 0.3f;

	UPROPERTY(EditAnywhere, Category="Status")
	float BleedRimBoost = 2.5f;

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0.5))
	float RimPower = 2.2f;

	UPROPERTY(EditAnywhere, Category="Status", meta = (ClampMin = 0.5))
	float FreezeRimPower = 1.7f;

	/** Burning: the fire hurts every half second until BurnEndTime */
	float BurnEndTime = 0.0f;
	float BurnDps = 0.0f;
	float NextBurnTick = 0.0f;
	TWeakObjectPtr<AActor> BurnSource;

	/** Chilled: slower until ChillEndTime. Frozen: stands still until FreezeEndTime */
	float ChillEndTime = 0.0f;
	float ChillSlow = 0.0f;
	float FreezeEndTime = 0.0f;

	/** Bleeding: the wound hurts every half second until BleedEndTime */
	float BleedEndTime = 0.0f;
	float BleedDps = 0.0f;
	float NextBleedTick = 0.0f;
	TWeakObjectPtr<AActor> BleedSource;

	/** The ice around a frozen enemy, and what it does when it breaks */
	TWeakObjectPtr<AAmnesiaFrost> Frost;
	TWeakObjectPtr<AActor> FreezeSource;
	float FreezeShatterDamage = 0.0f;
	bool bIceUp = false;

	/** True while a status effect tints or stops the enemy, so it is cleaned up when the effects end */
	bool bStatusActive = false;

	/** The tint over the body while a status effect is on */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StatusMaterial;

	/** The flames of the burning and the frozen enemy */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> BurnFxSystem;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> FreezeFxSystem;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> StatusFxList;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> StatusFxSystem;

public:

	/** Constructor */
	AAmnesiaEnemy();

	/** Runs the side scrolling brain */
	virtual void Tick(float DeltaTime) override;

	/** Interrupts attacks, stuns the enemy and plays a hit reaction when it's hit */
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;

	/** Plays the death animation if there's one, otherwise ragdolls */
	virtual void HandleDeath() override;

	/** Returns the current HP */
	UFUNCTION(BlueprintPure, Category="Damage")
	float GetHealth() const { return CurrentHP; }

	/** Returns the max HP */
	UFUNCTION(BlueprintPure, Category="Damage")
	float GetMaxHealth() const { return MaxHP; }

	/** Puts a living enemy back where it started, healed and calm. Used when the hero respawns */
	void ResetToHome();

	/** Sets the enemy on fire: it loses DamagePerSecond for Duration seconds. Burning again keeps the stronger fire and the longer time */
	void ApplyBurn(float DamagePerSecond, float Duration, AActor* Source, UNiagaraSystem* Effect);

	/** Slows the enemy down by Slow for Duration seconds. 0.3 is 30% slower */
	void ApplyChill(float Slow, float Duration, UNiagaraSystem* Effect);

	/** Freezes the enemy in ice for Duration seconds: it stands still and takes more damage, and when the ice breaks it is hurt for ShatterDamage */
	void ApplyFreeze(float Duration, UNiagaraSystem* Effect, float ShatterDamage = 0.0f, AActor* Source = nullptr);

	/** Makes the enemy bleed: it loses DamagePerSecond for Duration seconds */
	void ApplyBleed(float DamagePerSecond, float Duration, AActor* Source);

	/** Stuns the enemy: it can't move or attack for Seconds */
	void ApplyStun(float Seconds);

	/** True while frozen */
	bool IsFrozen() const;

	/** 1 at full speed, less while chilled */
	float GetSpeedScale() const;

	/** Puts an enemy far from the hero to sleep: no thinking, movement, animation, cloth or bone physics until it wakes up */
	void SetDormant(bool bDormant);

	bool IsDormant() const { return bIsDormant; }

protected:

	bool bIsDormant = false;

	/** Collision of the capsule and the skeleton from before the enemy fell asleep */
	TEnumAsByte<ECollisionEnabled::Type> SavedCapsuleCollision = ECollisionEnabled::QueryAndPhysics;
	TEnumAsByte<ECollisionEnabled::Type> SavedMeshCollision = ECollisionEnabled::QueryOnly;

public:

	/** Returns the enemy's level */
	UFUNCTION(BlueprintPure, Category="Level")
	int32 GetEnemyLevel() const { return EnemyLevel; }

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** EndPlay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Returns the creature anim instance, if the mesh uses one */
	UAmnesiaCreatureAnimInstance* GetCreatureAnimInstance() const;

	/** Returns the player character if it can currently be targeted */
	ACharacter* GetTargetPlayer() const;

	/** Walks back and forth around the starting point */
	void UpdatePatrol(float DeltaTime);

	/** Walks towards the player and attacks when in range */
	void UpdateChase(const ACharacter* Player);

	/** Turns the enemy to face along the side scrolling axis */
	void FaceDirection(float Direction);

	/** Starts an attack using either a single animation or the combo montage */
	void StartAttack();

	/** Called from a timer to check the single animation attack hit */
	void SingleAttackHit();

	/** Called from a timer when a creature anim instance attack ends */
	void SingleAttackEnded();

	/** Stops any single animation attack in progress */
	void CancelSingleAttack();

	/** Moves the mesh through the lunge, or back to rest */
	void UpdateLungeVisual();

	/** Flashes the mesh white */
	void FlashHit();

	/** Removes the hit flash */
	void EndFlash();

	/** Spawns healing orbs */
	void DropLoot();

	/** Loot and XP for whoever killed the enemy */
	void RewardKill(AActor* Killer);

	/** Damage from a burn: no hit reaction, no flash, no stun */
	void ApplyStatusDamage(float Amount, AActor* Source);

	/** Burn ticks, the tint and the flames. Runs every tick while a status effect is on */
	void UpdateStatus();

	/** Ends every status effect */
	void ClearStatus();

	/** Grows the ice around the enemy, and breaks it (the pieces fly) or takes it away at once. Breaking it by itself hurts, see ApplyFreeze */
	void SpawnFrost();
	void BreakIce(bool bShatter);

	/** Puts several small flames on the body, at different heights, and takes them off again */
	void SpawnStatusFlames(UNiagaraSystem* System);
	void DestroyStatusFlames();
};
