#include "AmnesiaSkills.h"
#include "AmnesiaCharacter.h"
#include "AmnesiaCompanion.h"
#include "AmnesiaEnemy.h"
#include "AmnesiaGlowLight.h"
#include "AmnesiaProgress.h"
#include "AmnesiaWeapons.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/StaticMesh.h"

// ---------------------------------------------------------------- the skills

namespace
{
	FString IconPath(const TCHAR* Id)
	{
		return FString::Printf(TEXT("/Game/Amnesia/UI/Skills/T_Skill_%s.T_Skill_%s"), Id, Id);
	}

	/** A system of the Vefects fire pack in the color set Set (Mystic, Electric, Dark...): Name is 01, 02 or Radial_01 */
	TSoftObjectPtr<UNiagaraSystem> VefectsSystem(const TCHAR* Set, const TCHAR* Name)
	{
		return TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(FString::Printf(TEXT("/Game/Vefects/Stylized_Fire/VFX/%s/Particles/NS_Stylized_Fire_%s_%s.NS_Stylized_Fire_%s_%s"), Set, Name, Set, Name, Set)));
	}
}

const TArray<FAmnesiaSkillDef>& FAmnesiaSkillTree::All()
{
	static TArray<FAmnesiaSkillDef> Skills;

	if (Skills.Num() > 0)
	{
		return Skills;
	}

	auto Add = [](const TCHAR* Id, const TCHAR* Name, const TCHAR* Detail, EAmnesiaSkillBranch Branch, EAmnesiaSkillKind Kind, int32 Tier, int32 MaxRank, int32 Cost, const TCHAR* Requires,
		float Base, float PerRank, float DisplayMultiplier, int32 Decimals, const TCHAR* Prefix, const TCHAR* Suffix)
	{
		FAmnesiaSkillDef Def;
		Def.Id = FName(Id);
		Def.Name = Name;
		Def.Detail = Detail;
		Def.Branch = Branch;
		Def.Kind = Kind;
		Def.Tier = Tier;
		Def.MaxRank = MaxRank;
		Def.Cost = Cost;
		Def.Requires = FName(Requires);
		Def.Base = Base;
		Def.PerRank = PerRank;
		Def.DisplayMultiplier = DisplayMultiplier;
		Def.ValueDecimals = Decimals;
		Def.ValuePrefix = Prefix;
		Def.ValueSuffix = Suffix;
		Def.Icon = IconPath(Id);
		Skills.Add(Def);
	};

	const EAmnesiaSkillKind Passive = EAmnesiaSkillKind::Passive;
	const EAmnesiaSkillKind Element = EAmnesiaSkillKind::Element;
	const EAmnesiaSkillKind Active = EAmnesiaSkillKind::Active;

	// Acél
	Add(TEXT("steel_power"), TEXT("Erős kar"), TEXT("A kard minden csapása keményebben üt."),
		EAmnesiaSkillBranch::Steel, Passive, 0, 3, 1, TEXT(""), 0.0f, 0.10f, 100.0f, 0, TEXT("+"), TEXT("% sebzés"));
	Add(TEXT("steel_body"), TEXT("Edzett test"), TEXT("Több életerővel vágsz neki a harcnak. A megszerzett életerő azonnal visszatölt."),
		EAmnesiaSkillBranch::Steel, Passive, 1, 3, 1, TEXT("steel_power"), 0.0f, 1.0f, 1.0f, 0, TEXT("+"), TEXT(" max életerő"));
	Add(TEXT("steel_blood"), TEXT("Vérszomj"), TEXT("Minden találat gyógyít egy keveset."),
		EAmnesiaSkillBranch::Steel, Passive, 2, 3, 1, TEXT("steel_body"), 0.0f, 0.04f, 1.0f, 2, TEXT(""), TEXT(" életerő találatonként"));
	Add(TEXT("steel_swift"), TEXT("Gyors lábak"), TEXT("Gyorsabban futsz és sprintelsz."),
		EAmnesiaSkillBranch::Steel, Passive, 3, 3, 1, TEXT("steel_blood"), 0.0f, 0.06f, 100.0f, 0, TEXT("+"), TEXT("% sebesség"));

	// Tűz
	Add(TEXT("fire_blade"), TEXT("Lángoló penge"), TEXT("A kard lángra kap: a találatok felgyújtják az ellenfelet, és a penge aurája tüzes lesz."),
		EAmnesiaSkillBranch::Fire, Element, 0, 1, 1, TEXT(""), 0.4f, 0.0f, 100.0f, 0, TEXT("A találat sebzésének "), TEXT("%-a elég"));
	Add(TEXT("fire_embers"), TEXT("Parázsló sebek"), TEXT("Az égés erősebb, és fél másodperccel tovább tart rangonként."),
		EAmnesiaSkillBranch::Fire, Passive, 1, 3, 1, TEXT("fire_blade"), 0.0f, 0.30f, 100.0f, 0, TEXT("+"), TEXT("% égési sebzés"));
	Add(TEXT("fire_burst"), TEXT("Lángcsapás"), TEXT("Q: lángtenger söpör végig előtted, sebez és felgyújt. Újratöltés: 8 mp."),
		EAmnesiaSkillBranch::Fire, Active, 2, 1, 2, TEXT("fire_blade"), 1.6f, 0.0f, 100.0f, 0, TEXT("Sebzés: "), TEXT("% a kard sebzéséből"));
	Add(TEXT("fire_fury"), TEXT("Feltüzelt harag"), TEXT("A Lángcsapás erősebb, és egy másodperccel gyorsabban tölt újra rangonként."),
		EAmnesiaSkillBranch::Fire, Passive, 3, 3, 1, TEXT("fire_burst"), 0.0f, 0.25f, 100.0f, 0, TEXT("+"), TEXT("% lángcsapás sebzés"));

	// Jég
	Add(TEXT("ice_blade"), TEXT("Fagyos penge"), TEXT("A kard megfagy: a találatok lelassítják az ellenfelet, és a penge aurája kék lesz."),
		EAmnesiaSkillBranch::Ice, Element, 0, 1, 1, TEXT(""), 0.3f, 0.0f, 100.0f, 0, TEXT("Lassítás: "), TEXT("%"));
	Add(TEXT("ice_chill"), TEXT("Dermesztő hideg"), TEXT("Erősebb lassítás, és a kard találatai néha jégbe zárják az ellenfelet. A Fagyasztó nova is tovább fagyaszt rangonként."),
		EAmnesiaSkillBranch::Ice, Passive, 1, 3, 1, TEXT("ice_blade"), 0.0f, 0.08f, 100.0f, 0, TEXT("+"), TEXT("% lassítás"));
	Add(TEXT("ice_nova"), TEXT("Fagyasztó nova"), TEXT("E: jeges lökéshullám, amely jégbe zárja a közeli ellenfeleket. A megfagyottak nem mozdulnak és több sebzést kapnak, a jég pedig összetörik és sebez. Újratöltés: 14 mp."),
		EAmnesiaSkillBranch::Ice, Active, 2, 1, 2, TEXT("ice_blade"), 0.8f, 0.0f, 100.0f, 0, TEXT("Sebzés: "), TEXT("% a kard sebzéséből"));
	Add(TEXT("ice_armor"), TEXT("Jégpáncél"), TEXT("A fagyos páncél csökkenti a kapott sebzést."),
		EAmnesiaSkillBranch::Ice, Passive, 3, 3, 1, TEXT("ice_nova"), 0.0f, 0.08f, 100.0f, 0, TEXT("-"), TEXT("% kapott sebzés"));

	// Sárkány
	Add(TEXT("dragon_fire"), TEXT("Sárkánytűz"), TEXT("A sárkány tűzgolyói erősebbek."),
		EAmnesiaSkillBranch::Dragon, Passive, 0, 3, 1, TEXT(""), 0.0f, 0.35f, 100.0f, 0, TEXT("+"), TEXT("% tűzgolyó sebzés"));
	Add(TEXT("dragon_haste"), TEXT("Gyors lehelet"), TEXT("A sárkány gyakrabban lő."),
		EAmnesiaSkillBranch::Dragon, Passive, 1, 3, 1, TEXT("dragon_fire"), 0.0f, 0.13f, 100.0f, 0, TEXT("-"), TEXT("% várakozási idő"));
	Add(TEXT("dragon_lantern"), TEXT("Fénygömb"), TEXT("A sárkány fényesebben világít: a barlang sötétjében is látsz."),
		EAmnesiaSkillBranch::Dragon, Passive, 2, 3, 1, TEXT("dragon_haste"), 0.0f, 0.50f, 100.0f, 0, TEXT("+"), TEXT("% fény"));
	Add(TEXT("dragon_guard"), TEXT("Sárkánypajzs"), TEXT("A sárkány felfog egy ütést, amely a hősnek szólt, aztán újratöltődik."),
		EAmnesiaSkillBranch::Dragon, Passive, 3, 3, 1, TEXT("dragon_lantern"), 26.0f, -6.0f, 1.0f, 0, TEXT("Újratöltés: "), TEXT(" mp"));

	// Holdkorong
	Add(TEXT("chakram_power"), TEXT("Éles perem"), TEXT("A korong pereme élesebb: minden találata keményebben üt."),
		EAmnesiaSkillBranch::Chakram, Passive, 0, 3, 1, TEXT(""), 0.0f, 0.12f, 100.0f, 0, TEXT("+"), TEXT("% korong sebzés"));
	Add(TEXT("chakram_bounce"), TEXT("Pattanó korong"), TEXT("Az első találat után a korong a legközelebbi ellenfélre pattan, rangonként eggyel többre, mielőtt visszatér."),
		EAmnesiaSkillBranch::Chakram, Passive, 1, 3, 1, TEXT("chakram_power"), 0.0f, 1.0f, 1.0f, 0, TEXT("+"), TEXT(" pattanás"));
	Add(TEXT("chakram_multi"), TEXT("Korongzápor"), TEXT("Egyszerre több korong lehet a levegőben, rangonként eggyel több (legfeljebb négy). Amíg mind kint van, nem dobhatsz újat."),
		EAmnesiaSkillBranch::Chakram, Passive, 2, 3, 1, TEXT("chakram_bounce"), 1.0f, 1.0f, 1.0f, 0, TEXT(""), TEXT(" korong egyszerre"));
	Add(TEXT("chakram_whirl"), TEXT("Örvénylő korong"), TEXT("A nehéz dobásnál (bal gomb nyomva tartva) a korong a röppályája végén másfél másodpercig örvényként pörög, és mindent megsebez maga körül."),
		EAmnesiaSkillBranch::Chakram, Passive, 3, 1, 2, TEXT("chakram_multi"), 0.5f, 0.0f, 100.0f, 0, TEXT("Sebzés: "), TEXT("% pörgésenként"));

	// Balta
	Add(TEXT("axe_power"), TEXT("Nehéz penge"), TEXT("A balta minden csapása sokkal keményebben üt."),
		EAmnesiaSkillBranch::Axe, Passive, 0, 3, 1, TEXT(""), 0.0f, 0.12f, 100.0f, 0, TEXT("+"), TEXT("% balta sebzés"));
	Add(TEXT("axe_cleave"), TEXT("Pusztító ív"), TEXT("A csapás szélesebb ívben vág, így több ellenfelet találsz el egyszerre."),
		EAmnesiaSkillBranch::Axe, Passive, 1, 3, 1, TEXT("axe_power"), 0.0f, 0.12f, 100.0f, 0, TEXT("+"), TEXT("% hatósugár"));
	Add(TEXT("axe_bleed"), TEXT("Vérző seb"), TEXT("A találatok vérzést okoznak: a sebzés egy része három másodperc alatt kicsorog."),
		EAmnesiaSkillBranch::Axe, Passive, 2, 3, 1, TEXT("axe_cleave"), 0.0f, 0.15f, 100.0f, 0, TEXT("A találat sebzésének "), TEXT("%-a vérzik ki"));
	Add(TEXT("axe_quake"), TEXT("Földrengető csapás"), TEXT("A nehéz támadás megrázza a földet: a lökéshullám sebzi és megszédíti a közeli ellenfeleket."),
		EAmnesiaSkillBranch::Axe, Passive, 3, 1, 2, TEXT("axe_bleed"), 1.8f, 0.0f, 100.0f, 0, TEXT("Sebzés: "), TEXT("% a balta sebzéséből"));

	// Varázspálca
	Add(TEXT("wand_power"), TEXT("Erősebb varázs"), TEXT("A varázspálca lökései keményebben ütnek."),
		EAmnesiaSkillBranch::Wand, Passive, 0, 3, 1, TEXT(""), 0.0f, 0.12f, 100.0f, 0, TEXT("+"), TEXT("% varázs sebzés"));
	Add(TEXT("wand_haste"), TEXT("Gyors idézés"), TEXT("Gyorsabban varázsolsz."),
		EAmnesiaSkillBranch::Wand, Passive, 1, 3, 1, TEXT("wand_power"), 0.0f, 0.08f, 100.0f, 0, TEXT("+"), TEXT("% varázslási sebesség"));
	Add(TEXT("wand_chain"), TEXT("Szétpattanó szikra"), TEXT("A varázs a célpont mellett álló ellenfeleket is megsebzi."),
		EAmnesiaSkillBranch::Wand, Passive, 2, 3, 1, TEXT("wand_haste"), 0.0f, 0.2f, 100.0f, 0, TEXT("A találat sebzésének "), TEXT("%-a a közelieket is éri"));
	Add(TEXT("wand_blast"), TEXT("Arkán robbanás"), TEXT("A nehéz támadás hatalmas varázsrobbanás: minden közeli ellenfelet megsebez és hátralök."),
		EAmnesiaSkillBranch::Wand, Passive, 3, 1, 2, TEXT("wand_chain"), 2.4f, 0.0f, 100.0f, 0, TEXT("Sebzés: "), TEXT("% a pálca sebzéséből"));

	return Skills;
}

const FAmnesiaSkillDef* FAmnesiaSkillTree::Find(FName Id)
{
	for (const FAmnesiaSkillDef& Def : All())
	{
		if (Def.Id == Id)
		{
			return &Def;
		}
	}

	return nullptr;
}

FLinearColor FAmnesiaSkillTree::BranchColor(EAmnesiaSkillBranch Branch)
{
	switch (Branch)
	{
	case EAmnesiaSkillBranch::Fire:
		return FLinearColor(1.0f, 0.45f, 0.2f, 1.0f);

	case EAmnesiaSkillBranch::Ice:
		return FLinearColor(0.45f, 0.78f, 1.0f, 1.0f);

	case EAmnesiaSkillBranch::Dragon:
		return FLinearColor(0.55f, 0.95f, 0.5f, 1.0f);

	case EAmnesiaSkillBranch::Chakram:
		return FLinearColor(0.62f, 0.86f, 1.0f, 1.0f);

	case EAmnesiaSkillBranch::Axe:
		return FLinearColor(0.98f, 0.4f, 0.32f, 1.0f);

	case EAmnesiaSkillBranch::Wand:
		return FLinearColor(0.78f, 0.58f, 1.0f, 1.0f);

	default:
		return FLinearColor(0.95f, 0.78f, 0.38f, 1.0f);
	}
}

FString FAmnesiaSkillTree::BranchName(EAmnesiaSkillBranch Branch)
{
	switch (Branch)
	{
	case EAmnesiaSkillBranch::Fire:
		return TEXT("TŰZ");

	case EAmnesiaSkillBranch::Ice:
		return TEXT("JÉG");

	case EAmnesiaSkillBranch::Dragon:
		return TEXT("SÁRKÁNY");

	case EAmnesiaSkillBranch::Chakram:
		return TEXT("HOLDKORONG");

	case EAmnesiaSkillBranch::Axe:
		return TEXT("BALTA");

	case EAmnesiaSkillBranch::Wand:
		return TEXT("VARÁZSPÁLCA");

	default:
		return TEXT("ACÉL");
	}
}

FString FAmnesiaSkillTree::ElementName(EAmnesiaElement Element)
{
	switch (Element)
	{
	case EAmnesiaElement::Fire:
		return TEXT("Tűz");

	case EAmnesiaElement::Ice:
		return TEXT("Jég");

	default:
		return TEXT("Acél");
	}
}

float FAmnesiaSkillTree::Value(const FAmnesiaSkillDef& Def, int32 Rank)
{
	return Rank > 0 ? Def.Base + Def.PerRank * Rank : 0.0f;
}

FString FAmnesiaSkillTree::ValueText(const FAmnesiaSkillDef& Def, int32 Rank)
{
	if (Def.ValuePrefix.IsEmpty() && Def.ValueSuffix.IsEmpty())
	{
		return FString();
	}

	const float Shown = Value(Def, Rank) * Def.DisplayMultiplier;

	return FString::Printf(TEXT("%s%.*f%s"), *Def.ValuePrefix, Def.ValueDecimals, Shown, *Def.ValueSuffix);
}

// ---------------------------------------------------------------- the component

UAmnesiaSkillComponent::UAmnesiaSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAmnesiaSkillComponent::BeginPlay()
{
	Super::BeginPlay();

	// the hero is not done starting yet: it applies the stats itself once it is
	LoadFromProgress();
	Rebuild();
}

void UAmnesiaSkillComponent::LoadFromProgress()
{
	Ranks.Reset();
	Element = EAmnesiaElement::None;

	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		const UAmnesiaSaveGame* Progress = ProgressSystem->GetProgress();

		for (const TPair<FName, int32>& Pair : Progress->SkillRanks)
		{
			// skills that were removed from the list in the meantime are dropped
			if (const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(Pair.Key))
			{
				Ranks.Add(Pair.Key, FMath::Clamp(Pair.Value, 0, Def->MaxRank));
			}
		}

		Element = static_cast<EAmnesiaElement>(FMath::Clamp<int32>(Progress->SkillElement, 0, 2));
	}
}

void UAmnesiaSkillComponent::SaveToProgress()
{
	if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
	{
		UAmnesiaSaveGame* Progress = ProgressSystem->GetProgress();
		Progress->SkillRanks = Ranks;
		Progress->SkillElement = static_cast<uint8>(Element);

		ProgressSystem->SaveGame();
	}
}

int32 UAmnesiaSkillComponent::GetRank(FName Id) const
{
	const int32* Rank = Ranks.Find(Id);
	return Rank ? *Rank : 0;
}

int32 UAmnesiaSkillComponent::GetTotalPoints() const
{
	if (const AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner()))
	{
		// the cats give a point for finding all of them
		UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);
		const int32 FromCats = ProgressSystem ? ProgressSystem->GetProgress()->CatSkillPoints : 0;

		return FMath::Max(Hero->GetHeroLevel() - 1, 0) * PointsPerLevel + BonusPoints + FromCats;
	}

	return 0;
}

int32 UAmnesiaSkillComponent::GetFreePoints() const
{
	return HasInfinitePoints() ? 999 : FMath::Max(GetTotalPoints() - GetSpentPoints(), 0);
}

bool UAmnesiaSkillComponent::HasInfinitePoints() const
{
	const UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);
	return ProgressSystem && ProgressSystem->bInfiniteSkillPoints;
}

bool UAmnesiaSkillComponent::IsBranchLocked(EAmnesiaSkillBranch Branch) const
{
	UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this);

	if (Branch == EAmnesiaSkillBranch::Dragon)
	{
		return !ProgressSystem || !ProgressSystem->Owns(AmnesiaDragonItemId());
	}

	// the branch of a weapon opens when the weapon is bought
	if (const FAmnesiaWeaponDef* Weapon = FAmnesiaWeapons::FindByBranch(Branch))
	{
		return !ProgressSystem || !ProgressSystem->Owns(Weapon->Id);
	}

	return false;
}

FString UAmnesiaSkillComponent::GetLockedReason(EAmnesiaSkillBranch Branch) const
{
	if (Branch == EAmnesiaSkillBranch::Dragon)
	{
		return TEXT("Zárolva: vedd meg a sárkányt a boltban");
	}

	if (const FAmnesiaWeaponDef* Weapon = FAmnesiaWeapons::FindByBranch(Branch))
	{
		return FString::Printf(TEXT("Zárolva: vedd meg %s a boltban"), *Weapon->NameAccusative);
	}

	return FString();
}

int32 UAmnesiaSkillComponent::GetSpentPoints() const
{
	int32 Spent = 0;

	for (const TPair<FName, int32>& Pair : Ranks)
	{
		if (const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(Pair.Key))
		{
			Spent += Def->Cost * Pair.Value;
		}
	}

	return Spent;
}

bool UAmnesiaSkillComponent::CanLearn(FName Id, FString* Reason) const
{
	auto Fail = [Reason](const FString& Text)
	{
		if (Reason)
		{
			*Reason = Text;
		}

		return false;
	};

	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(Id);

	if (!Def)
	{
		return Fail(TEXT("Ismeretlen képesség"));
	}

	if (GetRank(Id) >= Def->MaxRank)
	{
		return Fail(TEXT("Már a legmagasabb szinten van"));
	}

	if (IsBranchLocked(Def->Branch))
	{
		return Fail(GetLockedReason(Def->Branch));
	}

	if (!Def->Requires.IsNone() && GetRank(Def->Requires) <= 0)
	{
		const FAmnesiaSkillDef* Needed = FAmnesiaSkillTree::Find(Def->Requires);
		return Fail(FString::Printf(TEXT("Előbb tanuld meg: %s"), Needed ? *Needed->Name : TEXT("?")));
	}

	if (GetFreePoints() < Def->Cost)
	{
		return Fail(FString::Printf(TEXT("Nincs elég képességpont (%d kell)"), Def->Cost));
	}

	return true;
}

bool UAmnesiaSkillComponent::Learn(FName Id)
{
	if (!CanLearn(Id))
	{
		return false;
	}

	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(Id);

	Ranks.FindOrAdd(Id) += 1;

	// the first element learned goes on the sword at once
	if (Def->Kind == EAmnesiaSkillKind::Element && Element == EAmnesiaElement::None)
	{
		Element = Id == FName("fire_blade") ? EAmnesiaElement::Fire : EAmnesiaElement::Ice;
	}

	if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner()))
	{
		Hero->PlayLevelUpSound();
	}

	Apply(true);
	return true;
}

void UAmnesiaSkillComponent::ResetSkills()
{
	Ranks.Reset();
	Element = EAmnesiaElement::None;
	ReadyAt[0] = 0.0f;
	ReadyAt[1] = 0.0f;
	Apply(true);
}

void UAmnesiaSkillComponent::Rebuild()
{
	const float PreviousGuard = Stats.GuardCooldown;

	auto Rank = [this](const TCHAR* Id)
	{
		return GetRank(FName(Id));
	};

	auto Value = [this](const TCHAR* Id)
	{
		const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(FName(Id));
		return Def ? FAmnesiaSkillTree::Value(*Def, GetRank(FName(Id))) : 0.0f;
	};

	Stats = FAmnesiaSkillStats();

	Stats.DamageMult = 1.0f + Value(TEXT("steel_power"));
	Stats.BonusMaxHP = Value(TEXT("steel_body"));
	Stats.LifeOnHit = Value(TEXT("steel_blood"));
	Stats.SpeedMult = 1.0f + Value(TEXT("steel_swift"));

	Stats.bFireBlade = Rank(TEXT("fire_blade")) > 0;
	Stats.BurnScale = Value(TEXT("fire_blade")) + Value(TEXT("fire_embers"));
	Stats.BurnTime = 3.0f + 0.5f * Rank(TEXT("fire_embers"));
	Stats.bFlameBurst = Rank(TEXT("fire_burst")) > 0;
	Stats.BurstDamageScale = Value(TEXT("fire_burst")) * (1.0f + Value(TEXT("fire_fury")));
	Stats.BurstCooldown = 8.0f - 1.0f * Rank(TEXT("fire_fury"));

	Stats.bIceBlade = Rank(TEXT("ice_blade")) > 0;
	Stats.ChillSlow = FMath::Min(Value(TEXT("ice_blade")) + Value(TEXT("ice_chill")), 0.6f);
	Stats.ChillTime = 2.5f + 0.8f * Rank(TEXT("ice_chill"));
	Stats.bFrostNova = Rank(TEXT("ice_nova")) > 0;
	Stats.DamageTakenMult = 1.0f - Value(TEXT("ice_armor"));
	Stats.NovaFreezeTime = FrostNovaFreezeTime + 0.4f * Rank(TEXT("ice_chill"));
	Stats.FreezeChance = Stats.bIceBlade ? 0.1f + 0.06f * Rank(TEXT("ice_chill")) : 0.0f;
	Stats.IceFreezeTime = 1.4f + 0.3f * Rank(TEXT("ice_chill"));

	Stats.DragonDamageMult = 1.0f + Value(TEXT("dragon_fire"));
	Stats.DragonCooldownMult = FMath::Max(1.0f - Value(TEXT("dragon_haste")), 0.4f);
	Stats.DragonLightMult = 1.0f + Value(TEXT("dragon_lantern"));
	Stats.GuardCooldown = Value(TEXT("dragon_guard"));

	Stats.ChakramDamageMult = 1.0f + Value(TEXT("chakram_power"));
	Stats.ChakramBounces = Rank(TEXT("chakram_bounce"));
	Stats.ChakramExtraDiscs = Rank(TEXT("chakram_multi"));
	Stats.bChakramWhirl = Rank(TEXT("chakram_whirl")) > 0;
	Stats.ChakramWhirlScale = Value(TEXT("chakram_whirl"));

	Stats.AxeDamageMult = 1.0f + Value(TEXT("axe_power"));
	Stats.AxeRadiusMult = 1.0f + Value(TEXT("axe_cleave"));
	Stats.AxeBleedScale = Value(TEXT("axe_bleed"));
	Stats.bAxeQuake = Rank(TEXT("axe_quake")) > 0;
	Stats.AxeQuakeScale = Value(TEXT("axe_quake"));

	Stats.WandDamageMult = 1.0f + Value(TEXT("wand_power"));
	Stats.WandRateMult = 1.0f + Value(TEXT("wand_haste"));
	Stats.WandSplash = Value(TEXT("wand_chain"));
	Stats.bWandBlast = Rank(TEXT("wand_blast")) > 0;
	Stats.WandBlastScale = Value(TEXT("wand_blast"));

	// a shield that was just learned starts ready
	if (PreviousGuard <= 0.0f && Stats.GuardCooldown > 0.0f)
	{
		bGuardUp = true;
	}

	// an element that is not unlocked can't stay on the sword
	if (!IsElementUnlocked(Element))
	{
		Element = EAmnesiaElement::None;
	}
}

void UAmnesiaSkillComponent::Apply(bool bSave)
{
	Rebuild();

	if (bSave)
	{
		SaveToProgress();
	}

	ApplyAura();

	if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner()))
	{
		Hero->OnSkillsChanged();
	}

	OnChanged.Broadcast();
}

bool UAmnesiaSkillComponent::IsElementUnlocked(EAmnesiaElement Which) const
{
	switch (Which)
	{
	case EAmnesiaElement::Fire:
		return Stats.bFireBlade;

	case EAmnesiaElement::Ice:
		return Stats.bIceBlade;

	default:
		return true;
	}
}

void UAmnesiaSkillComponent::SetElement(EAmnesiaElement Which)
{
	if (!IsElementUnlocked(Which) || Which == Element)
	{
		return;
	}

	Element = Which;
	Apply(true);
}

void UAmnesiaSkillComponent::CycleElement()
{
	const EAmnesiaElement Order[3] = { EAmnesiaElement::None, EAmnesiaElement::Fire, EAmnesiaElement::Ice };
	const int32 Start = static_cast<int32>(Element);

	for (int32 Step = 1; Step <= 3; ++Step)
	{
		const EAmnesiaElement Candidate = Order[(Start + Step) % 3];

		if (IsElementUnlocked(Candidate))
		{
			SetElement(Candidate);
			return;
		}
	}
}

void UAmnesiaSkillComponent::ApplyAura()
{
	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner());
	UNiagaraComponent* Aura = Hero ? Hero->GetWeaponAura() : nullptr;

	if (!Aura)
	{
		return;
	}

	// the aura of an element goes over whatever the hero holds, and with no element the weapon has its own
	const FAmnesiaWeaponDef* Weapon = Hero->GetEquippedWeaponDef();
	TSoftObjectPtr<UNiagaraSystem> WeaponAura;

	if (Weapon && !Weapon->AuraPath.IsEmpty())
	{
		WeaponAura = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(Weapon->AuraPath));
	}

	const TSoftObjectPtr<UNiagaraSystem>& Wanted = Element == EAmnesiaElement::Fire ? FireAura : (Element == EAmnesiaElement::Ice ? IceAura : (WeaponAura.IsNull() ? SteelAura : WeaponAura));
	UNiagaraSystem* System = GetEffect(Wanted);

	if (System && Aura->GetAsset() != System)
	{
		// the blueprint's own color overrides belong to the old system, so they are dropped
		Aura->SetAsset(System, true);
		Aura->Activate(true);
	}

	// the glow goes over the shape of the sword, whichever system is on: set again after every switch, since a new system starts without it
	if (!LoadedAuraMesh && !AuraMesh.IsNull())
	{
		LoadedAuraMesh = AuraMesh.LoadSynchronous();
	}

	// the shape of the model in the hand, or the sword's
	UStaticMesh* Shape = Hero->GetWeaponPropMesh();

	if (!Shape)
	{
		Shape = LoadedAuraMesh;
	}

	if (Shape)
	{
		Aura->SetVariableStaticMesh(FName("User.01 - Mesh -> Weapon"), Shape);
	}

	// a weapon whose glow hides its shape has it dimmed, while the weapon's own aura is on
	if (Weapon && Weapon->AuraEmissive.A > 0.0f && Element == EAmnesiaElement::None && !WeaponAura.IsNull())
	{
		const FLinearColor Emissive = Weapon->AuraEmissive;
		const FLinearColor Overlay(Weapon->AuraOverlay, Weapon->AuraOverlay, Weapon->AuraOverlay, 1.0f);
		TWeakObjectPtr<UNiagaraComponent> WeakAura(Aura);
		TWeakObjectPtr<UNiagaraSystem> WeakSystem(System);

		// only while the same system is still on the weapon
		auto Dim = [WeakAura, WeakSystem, Emissive, Overlay](float Nudge)
		{
			UNiagaraComponent* Component = WeakAura.Get();

			if (Component && Component->GetAsset() == WeakSystem.Get())
			{
				Component->SetVariableLinearColor(FName("User.03 - Color -> Emissive"), FLinearColor(Emissive.R * Nudge, Emissive.G * Nudge, Emissive.B * Nudge, 1.0f));
				Component->SetVariableLinearColor(FName("User.03 - Color -> Overlay"), Overlay);
				Component->SetVariableLinearColor(FName("User.03 - Color -> Overlay Noise"), Overlay);
			}
		};

		Dim(1.0f);

		// a system that has just been set does not take the emissive colour in the first time. Put in again once it runs, a hair different,
		// since a value that equals the stored one is skipped
		FTimerHandle DimHandle;
		GetWorld()->GetTimerManager().SetTimer(DimHandle, FTimerDelegate::CreateLambda([Dim]() { Dim(1.001f); }), 0.3f, false);
	}
}

UNiagaraSystem* UAmnesiaSkillComponent::GetEffect(const TSoftObjectPtr<UNiagaraSystem>& Effect)
{
	if (Effect.IsNull())
	{
		return nullptr;
	}

	UNiagaraSystem* System = Effect.LoadSynchronous();

	if (System)
	{
		LoadedEffects.AddUnique(System);
	}

	return System;
}

void UAmnesiaSkillComponent::PlayEffectAt(const TSoftObjectPtr<UNiagaraSystem>& Effect, const FVector& Location, float Scale, float MaxLifetime)
{
	UNiagaraSystem* System = GetEffect(Effect);

	if (!System)
	{
		return;
	}

	if (UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, Location, FRotator::ZeroRotator, FVector(1.0f), true, true))
	{
		Component->SetVariableFloat(FName("User.Scale Overall"), Scale);

		// a system that loops would burn on forever
		TWeakObjectPtr<UNiagaraComponent> Weak(Component);

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Weak]()
		{
			if (Weak.IsValid())
			{
				Weak->DestroyComponent();
			}

		}), MaxLifetime, false);
	}
}

void UAmnesiaSkillComponent::SpawnLayer(const TSoftObjectPtr<UNiagaraSystem>& Effect, const FVector& Location, float Scale, float Lifetime, float Delay, float Width, float Height)
{
	if (Effect.IsNull() || Scale <= 0.0f)
	{
		return;
	}

	if (Delay <= 0.0f)
	{
		SpawnLayerNow(Effect, Location, Scale, Lifetime, Width, Height);
		return;
	}

	TWeakObjectPtr<UAmnesiaSkillComponent> Weak(this);
	const TSoftObjectPtr<UNiagaraSystem> Copy = Effect;

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Weak, Copy, Location, Scale, Lifetime, Width, Height]()
	{
		if (Weak.IsValid())
		{
			Weak->SpawnLayerNow(Copy, Location, Scale, Lifetime, Width, Height);
		}

	}), Delay, false);
}

void UAmnesiaSkillComponent::SpawnLayerNow(const TSoftObjectPtr<UNiagaraSystem>& Effect, const FVector& Location, float Scale, float Lifetime, float Width, float Height)
{
	UNiagaraSystem* System = GetEffect(Effect);

	if (!System)
	{
		return;
	}

	UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, Location, FRotator::ZeroRotator, FVector(1.0f), true, true);

	if (!Component)
	{
		return;
	}

	// the systems expose their size, their life and their speed: a short life and a quick one make the fire flicker instead of hang
	Component->SetVariableFloat(FName("User.Scale Overall"), Scale);
	Component->SetVariableFloat(FName("User.Lifetime Min"), Lifetime);
	Component->SetVariableFloat(FName("User.Lifetime Max"), Lifetime * 1.15f);
	Component->SetVariableFloat(FName("User.Overall Speed"), WaveSpeed);
	Component->SetVariableFloat(FName("User.Scale Size Horizontal"), Width);
	Component->SetVariableFloat(FName("User.Scale Size Vertical"), Height);

	// a system that loops stops making flames when its time is up, and goes when the last of them died
	TWeakObjectPtr<UNiagaraComponent> Weak(Component);

	FTimerHandle StopHandle;
	GetWorld()->GetTimerManager().SetTimer(StopHandle, FTimerDelegate::CreateLambda([Weak]()
	{
		if (Weak.IsValid())
		{
			Weak->Deactivate();
		}

	}), Lifetime, false);

	FTimerHandle KillHandle;
	GetWorld()->GetTimerManager().SetTimer(KillHandle, FTimerDelegate::CreateLambda([Weak]()
	{
		if (Weak.IsValid())
		{
			Weak->DestroyComponent();
		}

	}), Lifetime * 1.15f + 1.5f, false);
}

void UAmnesiaSkillComponent::PlayWave(const FVector& Origin, float Facing, float Start, float Reach, int32 Count, float Size, EAmnesiaFxStyle Style)
{
	TSoftObjectPtr<UNiagaraSystem> Main = FlameEffect;
	TSoftObjectPtr<UNiagaraSystem> Alt = FlameEffect2;

	switch (Style)
	{
	case EAmnesiaFxStyle::Cold:
		Main = ChillEffect;
		Alt = ChillEffect2;
		break;

	case EAmnesiaFxStyle::Mystic:
		Main = VefectsSystem(TEXT("Mystic"), TEXT("01"));
		Alt = VefectsSystem(TEXT("Mystic"), TEXT("02"));
		break;

	case EAmnesiaFxStyle::Spark:
		Main = VefectsSystem(TEXT("Electric"), TEXT("01"));
		Alt = VefectsSystem(TEXT("Electric"), TEXT("02"));
		break;

	case EAmnesiaFxStyle::Dark:
		Main = VefectsSystem(TEXT("Dark"), TEXT("01"));
		Alt = VefectsSystem(TEXT("Dark"), TEXT("02"));
		break;

	default:
		break;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Part = Count > 1 ? static_cast<float>(Index) / (Count - 1) : 0.0f;

		// each flame sits a little off its spot, in front of or behind the others, and differs in size and height, so the row is not a pattern
		const float Along = Start + (Reach - Start) * Part + FMath::FRandRange(-28.0f, 28.0f) * WaveJitter;
		const float Depth = FMath::FRandRange(-45.0f, 45.0f) * WaveJitter;
		const float Rise = FMath::FRandRange(-12.0f, 12.0f) * WaveJitter;
		const float Big = Size * FMath::Lerp(0.8f, 1.25f, Part) * FMath::FRandRange(0.85f, 1.15f);
		const float Delay = Index * WaveStagger;
		const FVector At = Origin + FVector(Facing * Along, Depth, -62.0f + Rise);
		const bool bAlt = WaveAltEvery > 0 && Index % WaveAltEvery == WaveAltEvery - 1;

		SpawnLayer(bAlt ? Alt : Main, At, Big, WaveLifetime, Delay, WaveWidth * FMath::FRandRange(0.85f, 1.15f), WaveHeight * FMath::FRandRange(0.85f, 1.2f));
	}
}

void UAmnesiaSkillComponent::PlayFlash(const FVector& At, EAmnesiaFxStyle Style)
{
	TSoftObjectPtr<UNiagaraSystem> Flash = FlashEffect;
	FLinearColor LightColor(1.0f, 0.55f, 0.2f);

	switch (Style)
	{
	case EAmnesiaFxStyle::Cold:
		Flash = ChillFlashEffect;
		LightColor = FLinearColor(0.5f, 0.8f, 1.0f);
		break;

	case EAmnesiaFxStyle::Mystic:
		Flash = VefectsSystem(TEXT("Mystic"), TEXT("Radial_01"));
		LightColor = FLinearColor(0.75f, 0.5f, 1.0f);
		break;

	case EAmnesiaFxStyle::Spark:
		Flash = VefectsSystem(TEXT("Electric"), TEXT("Radial_01"));
		LightColor = FLinearColor(1.0f, 0.9f, 0.4f);
		break;

	case EAmnesiaFxStyle::Dark:
		Flash = VefectsSystem(TEXT("Dark"), TEXT("Radial_01"));
		LightColor = FLinearColor(0.6f, 0.35f, 0.9f);
		break;

	default:
		break;
	}

	SpawnLayer(Flash, At, 1.3f, 0.8f, 0.0f);

	if (WaveLightIntensity <= 0.0f)
	{
		return;
	}

	// the light of the flash, it lights the grass and the hero for a moment
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AAmnesiaGlowLight* Light = GetWorld()->SpawnActor<AAmnesiaGlowLight>(AAmnesiaGlowLight::StaticClass(), At + FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, Params))
	{
		Light->Configure(LightColor, WaveLightIntensity, 900.0f, 0.3f);
		Light->FadeOut(WaveLightTime);
	}
}

AAmnesiaEnemy* UAmnesiaSkillComponent::FindNearestEnemy(float MaxDistance) const
{
	const AActor* Owner = GetOwner();
	AAmnesiaEnemy* Best = nullptr;
	float BestDistance = MaxDistance;

	if (!Owner)
	{
		return nullptr;
	}

	for (TActorIterator<AAmnesiaEnemy> It(GetWorld()); It; ++It)
	{
		if (It->GetHealth() <= 0.0f || It->IsDormant())
		{
			continue;
		}

		const float Distance = FVector::Dist(It->GetActorLocation(), Owner->GetActorLocation());

		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = *It;
		}
	}

	return Best;
}

void UAmnesiaSkillComponent::PlayNovaEffect(const FVector& Origin)
{
	// the ring at the hero's feet, a tight one: from the side only its flames over the ground show
	if (NovaRingScale > 0.0f)
	{
		PlayEffectAt(NovaEffect, Origin + FVector(0.0f, 0.0f, -80.0f), NovaRingScale, 3.0f);
	}

	// blue flames rolling out to both sides, seen from the side, and the cold flash where it started
	PlayFlash(Origin + FVector(0.0f, 0.0f, -40.0f), EAmnesiaFxStyle::Cold);
	PlayWave(Origin, 1.0f, 90.0f, FrostNovaRadius, NovaWaveFlames, NovaFlameScale, EAmnesiaFxStyle::Cold);
	PlayWave(Origin, -1.0f, 90.0f, FrostNovaRadius, NovaWaveFlames, NovaFlameScale, EAmnesiaFxStyle::Cold);
}

// ---------------------------------------------------------------- the active skills

bool UAmnesiaSkillComponent::IsActiveUnlocked(EAmnesiaActive Which) const
{
	return Which == EAmnesiaActive::FlameBurst ? Stats.bFlameBurst : Stats.bFrostNova;
}

float UAmnesiaSkillComponent::GetCooldownTotal(EAmnesiaActive Which) const
{
	return Which == EAmnesiaActive::FlameBurst ? Stats.BurstCooldown : Stats.NovaCooldown;
}

float UAmnesiaSkillComponent::GetCooldownLeft(EAmnesiaActive Which) const
{
	return FMath::Max(ReadyAt[static_cast<int32>(Which)] - GetWorld()->GetTimeSeconds(), 0.0f);
}

bool UAmnesiaSkillComponent::TryCast(EAmnesiaActive Which)
{
	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner());

	if (!Hero || Hero->GetHealth() <= 0.0f || !IsActiveUnlocked(Which) || GetCooldownLeft(Which) > 0.0f)
	{
		return false;
	}

	if (Which == EAmnesiaActive::FlameBurst)
	{
		CastFlameBurst(Hero);

	} else {

		CastFrostNova(Hero);
	}

	ReadyAt[static_cast<int32>(Which)] = GetWorld()->GetTimeSeconds() + GetCooldownTotal(Which);
	OnChanged.Broadcast();

	return true;
}

void UAmnesiaSkillComponent::CastFlameBurst(AAmnesiaCharacter* Hero)
{
	const float Facing = Hero->GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;
	const FVector Origin = Hero->GetActorLocation();
	const float Damage = Hero->GetMeleeDamage() * Stats.BurstDamageScale;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Hero);

	const FVector Center = Origin + FVector(Facing * (FlameBurstRange * 0.5f + 40.0f), 0.0f, 20.0f);
	const FVector Extent(FlameBurstRange * 0.5f, 220.0f, 190.0f);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams, FCollisionShape::MakeBox(Extent), QueryParams);

	TSet<AAmnesiaEnemy*> Done;
	bool bFirst = true;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AAmnesiaEnemy* Enemy = Cast<AAmnesiaEnemy>(Overlap.GetActor());

		if (!Enemy || Done.Contains(Enemy) || Enemy->GetHealth() <= 0.0f || Enemy->IsDormant())
		{
			continue;
		}

		Done.Add(Enemy);

		Enemy->ApplyDamage(Damage, Hero, Enemy->GetActorLocation(), FVector(Facing * 380.0f, 0.0f, 200.0f));
		Enemy->ApplyBurn(Damage * Stats.BurnScale / Stats.BurnTime, Stats.BurnTime, Hero, GetEffect(BurnEffect));

		if (bFirst)
		{
			bFirst = false;
			Hero->OnAttackLanded(Enemy, Enemy->GetActorLocation(), Damage);
		}
	}

	// a wave of flames rolls out in front of the hero, from a flash
	PlayFlash(Origin + FVector(Facing * 60.0f, 0.0f, -20.0f), EAmnesiaFxStyle::Fire);
	PlayWave(Origin, Facing, WaveStart, FlameBurstRange, WaveFlames, FlameBurstScale, EAmnesiaFxStyle::Fire);

	Hero->AddCameraShake(11.0f);
	Hero->PlayCastSound();
}

void UAmnesiaSkillComponent::CastFrostNova(AAmnesiaCharacter* Hero)
{
	const FVector Origin = Hero->GetActorLocation();
	const FAmnesiaSkillDef* Def = FAmnesiaSkillTree::Find(FName("ice_nova"));
	const float Damage = Hero->GetMeleeDamage() * (Def ? Def->Base : 0.8f);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Hero);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(FrostNovaRadius), QueryParams);

	TSet<AAmnesiaEnemy*> Done;
	bool bFirst = true;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AAmnesiaEnemy* Enemy = Cast<AAmnesiaEnemy>(Overlap.GetActor());

		if (!Enemy || Done.Contains(Enemy) || Enemy->GetHealth() <= 0.0f || Enemy->IsDormant())
		{
			continue;
		}

		Done.Add(Enemy);

		const float Away = Enemy->GetActorLocation().X >= Origin.X ? 1.0f : -1.0f;

		Enemy->ApplyDamage(Damage, Hero, Enemy->GetActorLocation(), FVector(Away * 300.0f, 0.0f, 160.0f));
		Enemy->ApplyFreeze(Stats.NovaFreezeTime, GetEffect(ChillEffect), Damage * 0.6f, Hero);

		if (bFirst)
		{
			bFirst = false;
			Hero->OnAttackLanded(Enemy, Enemy->GetActorLocation(), Damage);
		}
	}

	// the shock wave spreads from the hero's feet
	PlayNovaEffect(Origin);

	Hero->AddCameraShake(9.0f);
	Hero->PlayCastSound();
}

// ---------------------------------------------------------------- hits and the dragon's shield

void UAmnesiaSkillComponent::OnHeroHit(AAmnesiaEnemy* Enemy, float Damage, bool bFromDragon)
{
	AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner());

	if (!Enemy || !Hero || Enemy->GetHealth() <= 0.0f)
	{
		return;
	}

	// only the sword steals life
	if (!bFromDragon && Stats.LifeOnHit > 0.0f)
	{
		Hero->HealBy(Stats.LifeOnHit, true);
	}

	// the dragon's fire carries the element too, but weaker
	const float Weight = bFromDragon ? 0.5f : 1.0f;

	if (Element == EAmnesiaElement::Fire && Stats.bFireBlade)
	{
		Enemy->ApplyBurn(Damage * Stats.BurnScale * Weight / Stats.BurnTime, Stats.BurnTime, Hero, GetEffect(BurnEffect));

	} else if (Element == EAmnesiaElement::Ice && Stats.bIceBlade) {

		Enemy->ApplyChill(Stats.ChillSlow * Weight, Stats.ChillTime, GetEffect(ChillEffect));

		// now and then the cold wins: the enemy is frozen in ice for a moment
		if (!bFromDragon && FMath::FRand() < Stats.FreezeChance)
		{
			Enemy->ApplyFreeze(Stats.IceFreezeTime, GetEffect(ChillEffect), Damage * 0.3f, Hero);
		}
	}
}

void UAmnesiaSkillComponent::OnLevelChanged()
{
	OnChanged.Broadcast();
}

bool UAmnesiaSkillComponent::TryCatchHit()
{
	if (Stats.GuardCooldown <= 0.0f || !bGuardUp)
	{
		return false;
	}

	bGuardUp = false;
	GuardReadyAt = GetWorld()->GetTimeSeconds() + Stats.GuardCooldown;

	if (const AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner()))
	{
		// a flash around the hero in the color of the element, with a few small flames to each side, and the dragon flinches
		const EAmnesiaFxStyle Style = Element == EAmnesiaElement::Ice ? EAmnesiaFxStyle::Cold : EAmnesiaFxStyle::Fire;
		const FVector HeroAt = Hero->GetActorLocation();

		PlayFlash(HeroAt + FVector(0.0f, 0.0f, -30.0f), Style);
		PlayWave(HeroAt, 1.0f, 70.0f, 190.0f, 2, 0.7f, Style);
		PlayWave(HeroAt, -1.0f, 70.0f, 190.0f, 2, 0.7f, Style);

		for (TActorIterator<AAmnesiaCompanion> It(GetWorld()); It; ++It)
		{
			It->Flinch();
		}
	}

	OnChanged.Broadcast();
	return true;
}

float UAmnesiaSkillComponent::GetGuardCharge() const
{
	if (Stats.GuardCooldown <= 0.0f)
	{
		return -1.0f;
	}

	if (bGuardUp)
	{
		return 1.0f;
	}

	return FMath::Clamp(1.0f - (GuardReadyAt - GetWorld()->GetTimeSeconds()) / Stats.GuardCooldown, 0.0f, 1.0f);
}

void UAmnesiaSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bGuardUp && Stats.GuardCooldown > 0.0f && GetWorld()->GetTimeSeconds() >= GuardReadyAt)
	{
		bGuardUp = true;
		OnChanged.Broadcast();
	}

	// test helpers, set on the running hero
	if (bDebugLearnAll)
	{
		bDebugLearnAll = false;

		for (const FAmnesiaSkillDef& Def : FAmnesiaSkillTree::All())
		{
			Ranks.Add(Def.Id, Def.MaxRank);
		}

		// everything includes the dragon and the weapons
		if (UAmnesiaProgressSubsystem* ProgressSystem = UAmnesiaProgressSubsystem::Get(this))
		{
			ProgressSystem->GiveItem(AmnesiaDragonItemId());

			for (const FAmnesiaWeaponDef& Weapon : FAmnesiaWeapons::All())
			{
				if (Weapon.Id != FAmnesiaWeapons::SwordId())
				{
					ProgressSystem->GiveItem(Weapon.Id);
				}
			}
		}

		Apply(true);

		if (AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner()))
		{
			Hero->SummonDragon();
		}
	}

	if (bDebugReset)
	{
		bDebugReset = false;
		ResetSkills();
	}

	if (DebugElement >= 0)
	{
		const EAmnesiaElement Wanted = static_cast<EAmnesiaElement>(FMath::Clamp(DebugElement, 0, 2));
		DebugElement = -1;
		SetElement(Wanted);
	}

	if (bDebugFlameBurst)
	{
		bDebugFlameBurst = false;
		TryCast(EAmnesiaActive::FlameBurst);
	}

	if (bDebugGuardPop)
	{
		bDebugGuardPop = false;
		TryCatchHit();
	}

	if (bDebugNovaEffect)
	{
		bDebugNovaEffect = false;

		if (const AActor* Owner = GetOwner())
		{
			PlayNovaEffect(Owner->GetActorLocation());
		}
	}

	if (bDebugIgnite || bDebugChill || bDebugFreeze)
	{
		AAmnesiaEnemy* Target = FindNearestEnemy(1500.0f);
		AAmnesiaCharacter* Hero = Cast<AAmnesiaCharacter>(GetOwner());

		if (Target && bDebugIgnite)
		{
			Target->ApplyBurn(0.15f, 8.0f, Hero, GetEffect(BurnEffect));
		}

		if (Target && bDebugChill)
		{
			Target->ApplyChill(0.5f, 8.0f, GetEffect(ChillEffect));
		}

		if (Target && bDebugFreeze)
		{
			Target->ApplyFreeze(8.0f, GetEffect(ChillEffect));
		}

		bDebugIgnite = false;
		bDebugChill = false;
		bDebugFreeze = false;
	}

	if (bDebugFrostNova)
	{
		bDebugFrostNova = false;
		TryCast(EAmnesiaActive::FrostNova);
	}
}
