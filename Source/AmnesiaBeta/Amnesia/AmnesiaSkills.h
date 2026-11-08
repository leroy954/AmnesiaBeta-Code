#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AmnesiaSkills.generated.h"

class AAmnesiaCharacter;
class AAmnesiaEnemy;
class UNiagaraSystem;
class UNiagaraComponent;
class UStaticMesh;

/** The columns of the skill tree: the sword's three, the dragon's, and one for each weapon that is bought in the shop */
UENUM(BlueprintType)
enum class EAmnesiaSkillBranch : uint8
{
	Steel,
	Fire,
	Ice,
	Dragon,
	Chakram,
	Axe,
	Wand
};

/** What the sword is charged with. It picks the sword aura and the effect of every hit */
UENUM(BlueprintType)
enum class EAmnesiaElement : uint8
{
	None,
	Fire,
	Ice
};

/** The skills that are used with a key */
UENUM(BlueprintType)
enum class EAmnesiaActive : uint8
{
	FlameBurst,
	FrostNova
};

/** The color of a wave of flames: the Vefects fire effects come in several colors */
enum class EAmnesiaFxStyle : uint8
{
	Fire,
	Cold,
	Mystic,
	Spark,
	Dark
};

/** How a skill works */
enum class EAmnesiaSkillKind : uint8
{
	/** Always on, grows with its ranks */
	Passive,

	/** Charges the sword with an element, one element at a time */
	Element,

	/** Used with a key and then needs time to recharge */
	Active
};

/** One skill of the tree. The list is written in AmnesiaSkills.cpp */
struct FAmnesiaSkillDef
{
	FName Id;
	FString Name;
	FString Detail;

	/** The effect of rank r is Base + PerRank * r. It is shown as ValuePrefix, the number (times DisplayMultiplier, with
	 *  ValueDecimals digits) and ValueSuffix, like "+", 20 and "% sebzés". With no suffix and no prefix nothing is shown */
	float Base = 0.0f;
	float PerRank = 0.0f;
	float DisplayMultiplier = 1.0f;
	int32 ValueDecimals = 0;
	FString ValuePrefix;
	FString ValueSuffix;

	EAmnesiaSkillBranch Branch = EAmnesiaSkillBranch::Steel;
	EAmnesiaSkillKind Kind = EAmnesiaSkillKind::Passive;

	/** Row in the branch column, 0 is the top */
	int32 Tier = 0;
	int32 MaxRank = 1;

	/** Skill points for every rank */
	int32 Cost = 1;

	/** Skill that needs a rank before this one can be learned */
	FName Requires;

	/** Icon texture in the content browser */
	FString Icon;
};

/** What all the ranks add up to. The hero, the enemies' status effects and the dragon read it */
struct FAmnesiaSkillStats
{
	float DamageMult = 1.0f;
	float BonusMaxHP = 0.0f;
	float SpeedMult = 1.0f;
	float LifeOnHit = 0.0f;
	float DamageTakenMult = 1.0f;

	/** Fire: a hit sets the enemy on fire. BurnScale is the part of the hit's damage that burns away over BurnTime */
	bool bFireBlade = false;
	float BurnScale = 0.4f;
	float BurnTime = 3.0f;
	bool bFlameBurst = false;
	float BurstDamageScale = 1.6f;
	float BurstCooldown = 8.0f;

	/** Ice: a hit slows the enemy down by ChillSlow (a fraction) for ChillTime */
	bool bIceBlade = false;
	float ChillSlow = 0.3f;
	float ChillTime = 2.5f;
	bool bFrostNova = false;
	float NovaCooldown = 14.0f;

	/** How long the frost nova freezes, the chance that a hit of the ice sword freezes for a moment, and for how long */
	float NovaFreezeTime = 3.2f;
	float FreezeChance = 0.0f;
	float IceFreezeTime = 1.4f;

	/** The chakram: damage, how many more enemies a throw bounces on to, how many more discs may fly at once, and the whirl of the
	 *  held throw (the damage of each of its hits, as a multiple of the hero's melee damage) */
	float ChakramDamageMult = 1.0f;
	int32 ChakramBounces = 0;
	int32 ChakramExtraDiscs = 0;
	bool bChakramWhirl = false;
	float ChakramWhirlScale = 0.5f;

	/** The axe: damage, how fat the swing is, the part of a hit that bleeds out, and the quake of the heavy attack */
	float AxeDamageMult = 1.0f;
	float AxeRadiusMult = 1.0f;
	float AxeBleedScale = 0.0f;
	bool bAxeQuake = false;
	float AxeQuakeScale = 1.8f;

	/** The wand: damage, speed, the part of a hit that splashes on the enemies nearby, and the blast of the heavy attack */
	float WandDamageMult = 1.0f;
	float WandRateMult = 1.0f;
	float WandSplash = 0.0f;
	bool bWandBlast = false;
	float WandBlastScale = 2.4f;

	/** The dragon */
	float DragonDamageMult = 1.0f;
	float DragonCooldownMult = 1.0f;
	float DragonLightMult = 1.0f;

	/** Seconds the dragon's shield needs to come back after it took a hit. 0 means the dragon has no shield */
	float GuardCooldown = 0.0f;
};

/** The list of skills and the look of the branches */
struct FAmnesiaSkillTree
{
	static const TArray<FAmnesiaSkillDef>& All();
	static const FAmnesiaSkillDef* Find(FName Id);
	static FLinearColor BranchColor(EAmnesiaSkillBranch Branch);
	static FString BranchName(EAmnesiaSkillBranch Branch);
	static FString ElementName(EAmnesiaElement Element);

	/** The number of the skill's effect at a rank, in the units of the skill (0.1 for +10%) */
	static float Value(const FAmnesiaSkillDef& Def, int32 Rank);

	/** The effect of a rank as text for the screen, like "+20% sebzés". Empty if the skill shows no number */
	static FString ValueText(const FAmnesiaSkillDef& Def, int32 Rank);
};

/** Called when a rank, the skill points or the element changed */
DECLARE_MULTICAST_DELEGATE(FAmnesiaSkillsChanged);

/**
 *  The hero's skills: ranks, skill points, the element on the sword, the two active skills and the dragon's shield.
 *  The ranks live in the saved progress, so a respawned hero has them too.
 */
UCLASS(ClassGroup=(Amnesia), meta = (BlueprintSpawnableComponent))
class UAmnesiaSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UAmnesiaSkillComponent();

	/** Skill points the hero earns for every level above the first */
	UPROPERTY(EditAnywhere, Category="Skills", meta = (ClampMin = 0))
	int32 PointsPerLevel = 2;

	/** Extra skill points on top of the ones from the levels, for testing or as a reward. Not saved */
	UPROPERTY(EditAnywhere, Category="Skills", meta = (ClampMin = 0))
	int32 BonusPoints = 0;

	/** Aura of the sword with no element, and with each element */
	UPROPERTY(EditAnywhere, Category="Skills|Aura")
	TSoftObjectPtr<UNiagaraSystem> SteelAura = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/MeleeWeaponAura_VFX/VFX/V1/NS_StylizedWeapon_V1_12.NS_StylizedWeapon_V1_12")));

	UPROPERTY(EditAnywhere, Category="Skills|Aura")
	TSoftObjectPtr<UNiagaraSystem> FireAura = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/MeleeWeaponAura_VFX/VFX/V1/NS_StylizedWeapon_V1_01.NS_StylizedWeapon_V1_01")));

	UPROPERTY(EditAnywhere, Category="Skills|Aura")
	TSoftObjectPtr<UNiagaraSystem> IceAura = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/MeleeWeaponAura_VFX/VFX/V1/NS_StylizedWeapon_V1_08.NS_StylizedWeapon_V1_08")));

	/** The weapon shape the aura draws its glow over. Every aura system comes with a weapon of its own (the ice one a maul, the steel one a dagger), and that
	 *  shows up as a hammer head or a glow that stops half way along the blade: the sword's shape makes the glow follow the hero's blade */
	UPROPERTY(EditAnywhere, Category="Skills|Aura")
	TSoftObjectPtr<UStaticMesh> AuraMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/MeleeWeaponAura_VFX/Demo/Meshes/MeleeWeapons/SM_Weapons_metal_sword.SM_Weapons_metal_sword")));

	/** Flames of the burning enemies and of the flame burst, the blue flames of the frost nova and of the chilled enemies */
	UPROPERTY(EditAnywhere, Category="Skills|Effects")
	TSoftObjectPtr<UNiagaraSystem> FlameEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Vefects/Stylized_Fire/VFX/Flames/Particles/NS_Stylized_Fire_01_Once.NS_Stylized_Fire_01_Once")));

	UPROPERTY(EditAnywhere, Category="Skills|Effects")
	TSoftObjectPtr<UNiagaraSystem> BurnEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Vefects/Stylized_Fire/VFX/Flames/Particles/NS_Stylized_Fire_02_Infinite.NS_Stylized_Fire_02_Infinite")));

	/** The layers of the flame wave of the burst and the nova: a second flame shape, the flash at the start, and for the cold a second shape and a flash */
	UPROPERTY(EditAnywhere, Category="Skills|Effects")
	TSoftObjectPtr<UNiagaraSystem> FlameEffect2 = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Vefects/Stylized_Fire/VFX/Flames/Particles/NS_Stylized_Fire_02_Once.NS_Stylized_Fire_02_Once")));

	UPROPERTY(EditAnywhere, Category="Skills|Effects")
	TSoftObjectPtr<UNiagaraSystem> FlashEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Vefects/Stylized_Fire/VFX/Flames/Particles/NS_Stylized_Fire_Radial_01.NS_Stylized_Fire_Radial_01")));

	UPROPERTY(EditAnywhere, Category="Skills|Effects")
	TSoftObjectPtr<UNiagaraSystem> ChillEffect2 = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Vefects/Stylized_Fire/VFX/Water/Particles/NS_Stylized_Fire_02_Water.NS_Stylized_Fire_02_Water")));

	UPROPERTY(EditAnywhere, Category="Skills|Effects")
	TSoftObjectPtr<UNiagaraSystem> ChillFlashEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Vefects/Stylized_Fire/VFX/Water/Particles/NS_Stylized_Fire_Radial_01_Water.NS_Stylized_Fire_Radial_01_Water")));

	UPROPERTY(EditAnywhere, Category="Skills|Effects")
	TSoftObjectPtr<UNiagaraSystem> NovaEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Vefects/Stylized_Fire/VFX/Water/Particles/NS_Stylized_Fire_Radial_01_Water.NS_Stylized_Fire_Radial_01_Water")));

	UPROPERTY(EditAnywhere, Category="Skills|Effects")
	TSoftObjectPtr<UNiagaraSystem> ChillEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Vefects/Stylized_Fire/VFX/Water/Particles/NS_Stylized_Fire_01_Water.NS_Stylized_Fire_01_Water")));

	/** Flame burst: how far it reaches, in front of the hero */
	UPROPERTY(EditAnywhere, Category="Skills|Active", meta = (ClampMin = 0, Units = "cm"))
	float FlameBurstRange = 620.0f;

	/** How big the flames of the flame burst and the frost nova are. The flames of a wave grow a bit with their distance */
	UPROPERTY(EditAnywhere, Category="Skills|Active", meta = (ClampMin = 0.1))
	float FlameBurstScale = 1.1f;

	UPROPERTY(EditAnywhere, Category="Skills|Active", meta = (ClampMin = 0.1))
	float NovaFlameScale = 1.0f;

	/** The flame wave: how many flames a burst has, and how many each side of a nova. The seconds between flames as it rolls out, how long each burns,
	 *  how fast it moves, and how far each is placed off its spot at random (1 is the usual, 0 is a straight row) */
	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 1, ClampMax = 16))
	int32 WaveFlames = 6;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 5, ClampMax = 16))
	int32 NovaWaveFlames = 5;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0, Units = "s"))
	float WaveStagger = 0.06f;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0.2, Units = "s"))
	float WaveLifetime = 1.4f;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0.1))
	float WaveSpeed = 1.25f;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0))
	float WaveJitter = 1.0f;

	/** Where the wave starts in front of the hero, and how often a flame is of the second shape (every this many). The flames are stretched: how wide
	 *  and how tall they are against their usual shape, so they are tongues of fire and not domes */
	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0, Units = "cm"))
	float WaveStart = 140.0f;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 1))
	int32 WaveAltEvery = 3;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0.2))
	float WaveWidth = 0.8f;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0.2))
	float WaveHeight = 1.4f;

/** The light of the flash at the start of a wave: its brightness in candelas (0 for none) and how long it takes to die down */
	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0))
	float WaveLightIntensity = 25.0f;

	UPROPERTY(EditAnywhere, Category="Skills|Wave", meta = (ClampMin = 0.1, Units = "s"))
	float WaveLightTime = 0.8f;

	UPROPERTY(EditAnywhere, Category="Skills|Active", meta = (ClampMin = 0))
	float NovaRingScale = 0.0f;

	/** Frost nova: radius around the hero, and how long it freezes */
	UPROPERTY(EditAnywhere, Category="Skills|Active", meta = (ClampMin = 0, Units = "cm"))
	float FrostNovaRadius = 650.0f;

	UPROPERTY(EditAnywhere, Category="Skills|Active", meta = (ClampMin = 0, Units = "s"))
	float FrostNovaFreezeTime = 3.2f;

	/** Test helpers: set to true on the running hero to learn every skill, reset them, or cast the active skills */
	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugLearnAll = false;

	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugReset = false;

	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugFlameBurst = false;

	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugFrostNova = false;

	/** Test helpers: play the frost nova flames without the recharge, and set the nearest enemy (within 15 m) on fire, chilled or frozen */
	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugNovaEffect = false;

	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugGuardPop = false;

	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugIgnite = false;

	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugChill = false;

	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	bool bDebugFreeze = false;

	/** Test helper: 0 none, 1 fire, 2 ice. -1 does nothing */
	UPROPERTY(EditAnywhere, Category="Skills|Debug")
	int32 DebugElement = -1;

	/** Fires when anything about the skills changed */
	FAmnesiaSkillsChanged OnChanged;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Ranks and points */
	int32 GetRank(FName Id) const;
	bool IsLearned(FName Id) const { return GetRank(Id) > 0; }
	int32 GetTotalPoints() const;
	int32 GetSpentPoints() const;
	int32 GetFreePoints() const;

	/** The admin panel can make the skill points endless, for testing */
	bool HasInfinitePoints() const;

	/** The dragon branch is locked until the dragon is bought in the shop, and a weapon's branch until the weapon is */
	bool IsBranchLocked(EAmnesiaSkillBranch Branch) const;

	/** What to buy to open a locked branch, as a sentence for the screen */
	FString GetLockedReason(EAmnesiaSkillBranch Branch) const;

	/** True if the next rank of the skill can be bought now. If not, Reason says why */
	bool CanLearn(FName Id, FString* Reason = nullptr) const;

	/** Buys the next rank. Returns true if it worked */
	bool Learn(FName Id);

	/** Gives every point back */
	void ResetSkills();

	/** What the ranks add up to */
	const FAmnesiaSkillStats& GetStats() const { return Stats; }

	/** The element on the sword */
	EAmnesiaElement GetElement() const { return Element; }
	bool IsElementUnlocked(EAmnesiaElement Which) const;
	void SetElement(EAmnesiaElement Which);

	/** Fire, then ice, then none, skipping what is not learned yet */
	void CycleElement();

	/** The skills used with a key */
	bool IsActiveUnlocked(EAmnesiaActive Which) const;
	float GetCooldownLeft(EAmnesiaActive Which) const;
	float GetCooldownTotal(EAmnesiaActive Which) const;
	bool TryCast(EAmnesiaActive Which);

	/** The hero's sword or the dragon's fire hurt an enemy: life steal, burning and chilling */
	void OnHeroHit(AAmnesiaEnemy* Enemy, float Damage, bool bFromDragon);

	/** Called when the hero's level changed, so the points show up */
	void OnLevelChanged();

	/** The dragon takes a hit meant for the hero, if its shield is ready. Returns true if the hit was caught */
	bool TryCatchHit();

	/** 0 when the shield is down, 1 when it is ready. -1 when the dragon has no shield */
	float GetGuardCharge() const;

	/** Loads an effect and keeps it alive */
	UNiagaraSystem* GetEffect(const TSoftObjectPtr<UNiagaraSystem>& Effect);

	/** Tells the hero, the dragon and the sword about the current state. The hero calls it once its own start is done */
	void Refresh() { Apply(false); }

	/** Takes the ranks from the saved progress again, after a new game started */
	void ReloadFromProgress() { LoadFromProgress(); Apply(false); }

	/** Puts the aura of the element on the weapon in the hand */
	void ApplyAura();

	/** The waves of flames and the flash, in the colors of a style: the heavy attacks of the weapons use them too */
	void PlayWaveStyled(const FVector& Origin, float Facing, float Start, float Reach, int32 Count, float Size, EAmnesiaFxStyle Style) { PlayWave(Origin, Facing, Start, Reach, Count, Size, Style); }
	void PlayFlashStyled(const FVector& At, EAmnesiaFxStyle Style) { PlayFlash(At, Style); }

private:

	/** Rank of every learned skill */
	TMap<FName, int32> Ranks;

	FAmnesiaSkillStats Stats;
	EAmnesiaElement Element = EAmnesiaElement::None;

	/** Game time each active skill is ready again */
	float ReadyAt[2] = { 0.0f, 0.0f };

	/** Game time the dragon's shield is back, and whether it is up */
	float GuardReadyAt = 0.0f;
	bool bGuardUp = true;

	/** Systems that were loaded, kept alive here */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraSystem>> LoadedEffects;

	/** The loaded weapon shape of the aura */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> LoadedAuraMesh;

	/** Adds up the ranks into Stats */
	void Rebuild();

	/** Tells the hero, the dragon and the sword about the new state, and writes the ranks to the saved progress */
	void Apply(bool bSave);

	void LoadFromProgress();
	void SaveToProgress();

	void CastFlameBurst(AAmnesiaCharacter* Hero);
	void CastFrostNova(AAmnesiaCharacter* Hero);

	/** Plays an effect where something happens. A looping system is switched off after MaxLifetime seconds */
	void PlayEffectAt(const TSoftObjectPtr<UNiagaraSystem>& Effect, const FVector& Location, float Scale, float MaxLifetime = 4.0f);

	/** One layer of a flame wave, made now or after a delay: its size, how wide and tall it is against its usual shape, and how long it burns
	 *  before it is switched off */
	void SpawnLayer(const TSoftObjectPtr<UNiagaraSystem>& Effect, const FVector& Location, float Scale, float Lifetime, float Delay, float Width = 1.0f, float Height = 1.0f);
	void SpawnLayerNow(const TSoftObjectPtr<UNiagaraSystem>& Effect, const FVector& Location, float Scale, float Lifetime, float Width, float Height);

	/** A wave of flames rolling out from the origin to one side, as fire or as cold, from Start to Reach in front of it. The flames differ in size,
	 *  depth, height and shape */
	void PlayWave(const FVector& Origin, float Facing, float Start, float Reach, int32 Count, float Size, EAmnesiaFxStyle Style);

	/** The flash and the light at the start of a wave */
	void PlayFlash(const FVector& At, EAmnesiaFxStyle Style);

	/** The nearest living enemy to the hero, within MaxDistance */
	AAmnesiaEnemy* FindNearestEnemy(float MaxDistance) const;

	/** The flames of the frost nova: a blue ring and blue flames rolling along the ground to both sides */
	void PlayNovaEffect(const FVector& Origin);
};
