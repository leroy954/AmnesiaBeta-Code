#pragma once

#include "CoreMinimal.h"
#include "AmnesiaSkills.h"

/** What the heavy attack of a weapon turns into once the last skill of the weapon's branch is learned */
enum class EAmnesiaWeaponSpecial : uint8
{
	None,

	/** Axe: the ground shakes, everything near is hurt and stunned */
	Quake,

	/** Chakram: the held throw whirls at the far end of its flight, hurting everything around */
	Whirl,

	/** Wand: a big blast of magic in front of the hero */
	Blast
};

/** One swing, thrust or cast of a weapon: a part of an animation and the place where it hits */
struct FAmnesiaWeaponSlash
{
	/** Asset path of the animation. Empty means the sword's own animation */
	FString Animation;

	/** Where in the animation the move starts, the time of the hit after the start, and how much of the animation plays (animation seconds) */
	float StartTime = 0.0f;
	float HitTime = 0.3f;
	float Duration = 0.7f;
	float PlayRate = 1.0f;

	/** Damage as a multiple of the hero's melee damage */
	float DamageScale = 1.0f;

	/** Where the hit check reaches (forward, up) and how fat it is */
	FVector2D HitOffset = FVector2D(120.0f, 20.0f);
	float HitRadius = 110.0f;

	/** A push forward at the start of the move, so a thrust steps into the enemy (cm/s, 0 = none) */
	float Step = 0.0f;

	/** Only the upper body plays the move, the legs keep standing (a cast whose legs crouch) */
	bool bUpperBodyOnly = false;
};

/** A weapon of the hero. The sword is the first one and always owned; the others are sold in the shop, and each opens a branch of skills */
struct FAmnesiaWeaponDef
{
	FName Id;
	FString Name;

	/** The name with the accusative ending, for "vedd meg a lándzsát a boltban" */
	FString NameAccusative;

	/** The name in capital letters, for the banners: FString::ToUpper leaves the accented letters alone */
	FString NameUpper;

	/** The shop's texts */
	FString Subtitle;
	FString Detail;
	TArray<FString> Perks;

	/** The detail page of the shop: what the weapon is and does, and which buttons use it */
	FString LongDetail;
	FString Controls;
	int32 Price = 0;

	EAmnesiaSkillBranch Branch = EAmnesiaSkillBranch::Steel;
	FLinearColor Color = FLinearColor::White;

	/** The picture in the shop and the model in the hand: where it sits in the hand socket */
	FString IconPath;
	FString MeshPath;
	FVector HandLocation = FVector::ZeroVector;
	FRotator HandRotation = FRotator::ZeroRotator;
	FVector HandScale = FVector::OneVector;

	/** The aura effect the weapon has without an element, and where it sits on the model */
	FString AuraPath;
	FVector AuraLocation = FVector::ZeroVector;
	FRotator AuraRotation = FRotator::ZeroRotator;
	FVector AuraScale = FVector::OneVector;

	/** For an aura that covers the shape of the model: the glow is dimmed to these, so the model shows. An alpha of zero leaves the aura as it is */
	FLinearColor AuraEmissive = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	float AuraOverlay = 1.0f;

	/** Damage, speed and reach against the sword. The radius is how fat the hit check is */
	float DamageMult = 1.0f;
	float RateMult = 1.0f;
	float ReachMult = 1.0f;
	float RadiusMult = 1.0f;

	/** The heavy attack that the last skill of the branch unlocks */
	EAmnesiaWeaponSpecial Special = EAmnesiaWeaponSpecial::None;
	FName SpecialSkill;

	/** The swings of the ground combo and the heavy swing. A weapon with no combo uses the sword's swings */
	TArray<FAmnesiaWeaponSlash> Combo;
	FAmnesiaWeaponSlash Heavy;
	bool bHasHeavy = false;

	/** The heavy swing is a magic bolt that flies forward, not a blow in hand */
	bool bShootsBolt = false;

	/** Every swing throws the weapon itself, which flies out and comes back (the chakram) */
	bool bThrowsDisc = false;
};

/** The weapons of the game, written in AmnesiaWeapons.cpp */
struct FAmnesiaWeapons
{
	/** Every weapon, the sword first */
	static const TArray<FAmnesiaWeaponDef>& All();

	static const FAmnesiaWeaponDef* Find(FName Id);

	/** The weapon whose skills a branch holds, or null for the branches that are not a weapon's */
	static const FAmnesiaWeaponDef* FindByBranch(EAmnesiaSkillBranch Branch);

	static FName SwordId()
	{
		return FName(TEXT("sword"));
	}

	/** In the editor the numbers of the weapons can be changed without a build: Saved/WeaponTuning.json is read on top of the table. Call it before the
	 *  weapon is equipped. Returns true if the file changed since the last time */
	static bool ReloadTuning();
};
