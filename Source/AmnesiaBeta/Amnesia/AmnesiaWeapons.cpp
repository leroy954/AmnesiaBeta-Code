#include "AmnesiaWeapons.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#endif

namespace
{
	const TCHAR* PurePolyMesh(const TCHAR* Name)
	{
		static FString Path;
		Path = FString::Printf(TEXT("/Game/PurePoly/FreeLowPolyFantasyRPGWeapons/Meshes/%s.%s"), Name, Name);
		return *Path;
	}

	FString AuraSystem(const TCHAR* Name)
	{
		return FString::Printf(TEXT("/Game/MeleeWeaponAura_VFX/VFX/%.2s/NS_StylizedWeapon_%s.NS_StylizedWeapon_%s"), Name, Name, Name);
	}

	FString WeaponIcon(const TCHAR* Id)
	{
		return FString::Printf(TEXT("/Game/Amnesia/UI/Weapons/T_Weapon_%s.T_Weapon_%s"), Id, Id);
	}

	/** A Mixamo mocap animation retargeted onto the hero and made to play in place (Tools/retarget_anims.py Mixamo, Tools/anim_in_place.py) */
	FString MixamoAnimation(const TCHAR* Pack, const TCHAR* Name)
	{
		return FString::Printf(TEXT("/Game/Amnesia/Animation/Combat/Mixamo/%s/Hero/%s.%s"), Pack, Name, Name);
	}

	FAmnesiaWeaponSlash MakeSlash(const FString& Animation, float StartTime, float HitTime, float Duration, float PlayRate, float DamageScale, float Reach, float Up, float Radius)
	{
		FAmnesiaWeaponSlash Result;
		Result.Animation = Animation;
		Result.StartTime = StartTime;
		Result.HitTime = HitTime;
		Result.Duration = Duration;
		Result.PlayRate = PlayRate;
		Result.DamageScale = DamageScale;
		Result.HitOffset = FVector2D(Reach, Up);
		Result.HitRadius = Radius;
		return Result;
	}

	TArray<FAmnesiaWeaponDef> Build()
	{
		TArray<FAmnesiaWeaponDef> List;

		// the sword: the hero's own, nothing to buy. Its swings and its look come from the Blueprint
		{
			FAmnesiaWeaponDef& Sword = List.AddDefaulted_GetRef();
			Sword.Id = FAmnesiaWeapons::SwordId();
			Sword.Name = TEXT("Kard");
			Sword.NameAccusative = TEXT("a kardot");
			Sword.NameUpper = TEXT("KARD");
			Sword.Color = FLinearColor(0.95f, 0.78f, 0.38f);

			// three mocap sword cuts (the times are animation seconds: where the cut starts, the hit after that, how much plays)
			const TCHAR* Pack = TEXT("Sword_and_Shield_Pack");
			Sword.Combo.Add(MakeSlash(MixamoAnimation(Pack, TEXT("sword_and_shield_attack__4_")), 0.05f, 0.45f, 0.80f, 1.35f, 1.0f, 130.0f, 40.0f, 115.0f));
			Sword.Combo.Add(MakeSlash(MixamoAnimation(Pack, TEXT("sword_and_shield_slash__3_")), 0.15f, 0.66f, 1.10f, 1.35f, 1.2f, 160.0f, 10.0f, 130.0f));
			Sword.Combo.Add(MakeSlash(MixamoAnimation(Pack, TEXT("sword_and_shield_attack__2_")), 0.0f, 0.57f, 1.05f, 1.3f, 1.5f, 190.0f, 0.0f, 110.0f));
		}

		// the chakram: a disc thrown from the hand. Every click throws it (the disc throw of the mage animations, only the throw itself),
		// it flies forward spinning, hurts everything it passes, and comes back to the hand
		{
			FAmnesiaWeaponDef& Chakram = List.AddDefaulted_GetRef();
			Chakram.Id = FName(TEXT("chakram"));
			Chakram.Name = TEXT("Holdkorong");
			Chakram.NameAccusative = TEXT("a Holdkorongot");
			Chakram.NameUpper = TEXT("HOLDKORONG");
			Chakram.Subtitle = TEXT("Fegyver  •  dobókorong");
			Chakram.Detail = TEXT("Ezüstös, éles peremű korong. Eldobod, pörögve átrepül az ellenfeleken, aztán visszatér a kezedbe.");
			Chakram.Perks = { TEXT("Bal gombbal eldobod, oda és vissza is sebez"), TEXT("Új képességág: Holdkorong") };
			Chakram.LongDetail = TEXT("Ezüstös, éles peremű korong, a holdfény hidegét őrzi. A bal gombbal eldobod: pörögve előrerepül, minden ellenfelet megsebez, akin átrepül, aztán visszatér a kezedbe, és visszafelé is sebez. Nyomva tartva messzebbre és erősebben dobod. Amíg vissza nem tér, nem dobhatsz újra. A képességeivel élesebb lesz, ellenfélről ellenfélre pattan, egyszerre több korongot dobhatsz, és megtanulhatod, hogy a messzi pontján örvényként pörögjön.");
			Chakram.Controls = TEXT("Bal gomb: dobás (újra, ha visszatért)   •   Bal gomb nyomva tartva: erős dobás   •   Jobb gomb nyomva tartva: védekezés");
			Chakram.Price = 250;
			Chakram.Branch = EAmnesiaSkillBranch::Chakram;
			Chakram.Color = FLinearColor(0.62f, 0.86f, 1.0f);
			Chakram.IconPath = WeaponIcon(TEXT("chakram"));
			Chakram.MeshPath = PurePolyMesh(TEXT("SM_PP_Theme_05_Chakram_001"));
			// held by the bar across its middle, its face to the camera
			Chakram.HandRotation = FRotator(90.0f, 0.0f, 0.0f);
			Chakram.HandScale = FVector(2.3f);
			Chakram.AuraPath = AuraSystem(TEXT("V1_05"));
			Chakram.AuraEmissive = FLinearColor(0.12f, 0.9f, 2.4f, 1.0f);
			Chakram.AuraOverlay = 0.12f;
			Chakram.DamageMult = 0.9f;
			Chakram.RateMult = 1.0f;
			Chakram.ReachMult = 1.0f;
			Chakram.Special = EAmnesiaWeaponSpecial::Whirl;
			Chakram.SpecialSkill = FName(TEXT("chakram_whirl"));
			// a mocap frisbee throw: the disc leaves the hand when the arm whips forward
			Chakram.Heavy = MakeSlash(MixamoAnimation(TEXT("Single"), TEXT("Frisbee_Throw")), 0.30f, 0.47f, 0.90f, 1.3f, 1.6f, 0.0f, 0.0f, 0.0f);
			Chakram.bHasHeavy = true;
			Chakram.bThrowsDisc = true;
		}

		// the axe: slow and wide and heavy
		{
			FAmnesiaWeaponDef& Axe = List.AddDefaulted_GetRef();
			Axe.Id = FName(TEXT("axe"));
			Axe.Name = TEXT("Kőtörő bárd");
			Axe.NameAccusative = TEXT("a Kőtörő bárdot");
			Axe.NameUpper = TEXT("KŐTÖRŐ BÁRD");
			Axe.Subtitle = TEXT("Fegyver  •  nehéz csapások");
			Axe.Detail = TEXT("Rovátkolt élű, nehéz bárd, amellyel a bányászok falakat törtek. Lassan lendül, de minden csapása széles sebet vág.");
			Axe.Perks = { TEXT("Nagy sebzés, széles vágás"), TEXT("Új képességág: Balta") };
			Axe.LongDetail = TEXT("Rovátkolt élű, nehéz bárd, amellyel a bányászok falakat törtek. Lassabban lendül, mint a kard, de minden csapása szélesebb ívben vág, és sokkal keményebben üt. A képességeivel vérzést okozhatsz vele, még szélesebben csaphatsz, és a nehéz csapása megrázza a földet is: a lökéshullám megsebzi és megszédíti a közeli ellenfeleket.");
			Axe.Controls = TEXT("Bal gomb: nehéz csapások   •   Bal gomb nyomva tartva: erős csapás   •   Jobb gomb nyomva tartva: védekezés");
			Axe.Price = 350;
			Axe.Branch = EAmnesiaSkillBranch::Axe;
			Axe.Color = FLinearColor(0.98f, 0.4f, 0.32f);
			Axe.IconPath = WeaponIcon(TEXT("axe"));
			Axe.MeshPath = PurePolyMesh(TEXT("SM_PP_Theme_11_Axe_One_Handed_002"));
			Axe.HandRotation = FRotator(-90.0f, 180.0f, -90.0f);
			Axe.AuraPath = AuraSystem(TEXT("V1_05"));
			Axe.AuraEmissive = FLinearColor(0.034f, 1.8f, 2.5f, 1.0f);
			Axe.AuraOverlay = 0.12f;
			Axe.DamageMult = 1.9f;
			Axe.RateMult = 0.8f;
			Axe.ReachMult = 1.0f;
			Axe.RadiusMult = 1.35f;
			Axe.Special = EAmnesiaWeaponSpecial::Quake;
			Axe.SpecialSkill = FName(TEXT("axe_quake"));

			// three whole mocap axe blows, each with its own wind up and follow through (a combo animation cut into parts stopped in the middle of
			// the next wind up), and a big high spin for the heavy attack (the quake comes with it). Slow and heavy on purpose. The side cut was left
			// out: its deep lunge looks like a dive when it plays in place
			const TCHAR* Pack = TEXT("Pro_Melee_Axe_Pack");
			Axe.Combo.Add(MakeSlash(MixamoAnimation(Pack, TEXT("standing_melee_attack_backhand")), 0.45f, 0.58f, 1.20f, 1.25f, 1.0f, 150.0f, 20.0f, 125.0f));
			Axe.Combo.Add(MakeSlash(MixamoAnimation(Pack, TEXT("standing_melee_attack_downward")), 0.05f, 0.79f, 1.30f, 1.25f, 1.2f, 150.0f, 0.0f, 130.0f));
			Axe.Combo.Add(MakeSlash(MixamoAnimation(Pack, TEXT("standing_melee_attack_360_low")), 0.20f, 0.77f, 1.20f, 1.25f, 1.5f, 170.0f, 0.0f, 120.0f));
			Axe.Heavy = MakeSlash(MixamoAnimation(Pack, TEXT("standing_melee_attack_360_high")), 0.20f, 0.83f, 1.55f, 1.0f, 2.0f, 160.0f, 20.0f, 140.0f);
			Axe.bHasHeavy = true;
		}

		// the wand: every click casts a fire bolt, a quick one on a tap and the full one when the button is held
		{
			FAmnesiaWeaponDef& Wand = List.AddDefaulted_GetRef();
			Wand.Id = FName(TEXT("wand"));
			Wand.Name = TEXT("Ködjáró pálca");
			Wand.NameAccusative = TEXT("a Ködjáró pálcát");
			Wand.NameUpper = TEXT("KÖDJÁRÓ PÁLCA");
			Wand.Subtitle = TEXT("Fegyver  •  varázslat");
			Wand.Detail = TEXT("Csontfehér pálca, amelyből köd kél. Minden kattintásra tüzes lövedéket lő, ami messzire repül; nyomva tartva erősebbet.");
			Wand.Perks = { TEXT("Bal gombbal tüzes lövedék"), TEXT("Új képességág: Varázspálca") };
			Wand.LongDetail = TEXT("Csontfehér pálca, amelyből köd kél. Nem közelharci fegyver: a bal gombbal tüzes lövedéket lő, ami egészen a célpontig repül, nyomva tartva pedig erősebb, kétszer akkorát sebző lövedéket. A sebzése kisebb, mint a kardé, ezért a képességei sokat számítanak: erősíthetsz rajta, gyorsíthatsz, a találat a közeli ellenfeleket is sebzi, és egy hatalmas varázsrobbanást is megtanulhatsz.");
			Wand.Controls = TEXT("Bal gomb: tüzes lövedék   •   Bal gomb nyomva tartva: erős lövedék   •   Jobb gomb nyomva tartva: védekezés");
			Wand.Price = 450;
			Wand.Branch = EAmnesiaSkillBranch::Wand;
			Wand.Color = FLinearColor(0.78f, 0.58f, 1.0f);
			Wand.IconPath = WeaponIcon(TEXT("wand"));
			Wand.MeshPath = PurePolyMesh(TEXT("SM_PP_Theme_08_Wand_001"));
			Wand.HandRotation = FRotator(0.0f, 0.0f, 90.0f);
			Wand.AuraPath = AuraSystem(TEXT("V2_07"));
			Wand.DamageMult = 0.85f;
			Wand.RateMult = 1.0f;
			Wand.ReachMult = 1.0f;
			Wand.Special = EAmnesiaWeaponSpecial::Blast;
			Wand.SpecialSkill = FName(TEXT("wand_blast"));
			// a mocap one-handed cast: the bolt leaves when the hand thrusts forward
			// the legs of this cast crouch low, so only the arms and the body play it: the hero stays standing
			Wand.Heavy = MakeSlash(MixamoAnimation(TEXT("Pro_Magic_Pack"), TEXT("Standing_1H_Magic_Attack_01")), 0.35f, 0.50f, 0.95f, 1.0f, 2.2f, 330.0f, 40.0f, 140.0f);
			Wand.Heavy.bUpperBodyOnly = true;
			Wand.bHasHeavy = true;
			Wand.bShootsBolt = true;
		}

		return List;
	}

	TArray<FAmnesiaWeaponDef>& Table()
	{
		static TArray<FAmnesiaWeaponDef> List = Build();
		return List;
	}

#if WITH_EDITOR
	bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, float& Out)
	{
		double Value = 0.0;

		if (Object->TryGetNumberField(Field, Value))
		{
			Out = static_cast<float>(Value);
			return true;
		}

		return false;
	}

	bool ReadVector(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FVector& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values;

		if (Object->TryGetArrayField(Field, Values) && Values->Num() == 3)
		{
			Out = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
			return true;
		}

		// a single number is a uniform scale
		double Single = 0.0;

		if (Object->TryGetNumberField(Field, Single))
		{
			Out = FVector(Single);
			return true;
		}

		return false;
	}

	bool ReadRotator(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FRotator& Out)
	{
		FVector Vector;

		// pitch, yaw, roll
		if (ReadVector(Object, Field, Vector))
		{
			Out = FRotator(Vector.X, Vector.Y, Vector.Z);
			return true;
		}

		return false;
	}

	void ReadSlash(const TSharedPtr<FJsonObject>& Object, FAmnesiaWeaponSlash& Slash)
	{
		Object->TryGetStringField(TEXT("Animation"), Slash.Animation);
		ReadNumber(Object, TEXT("StartTime"), Slash.StartTime);
		ReadNumber(Object, TEXT("HitTime"), Slash.HitTime);
		ReadNumber(Object, TEXT("Duration"), Slash.Duration);
		ReadNumber(Object, TEXT("PlayRate"), Slash.PlayRate);
		ReadNumber(Object, TEXT("DamageScale"), Slash.DamageScale);
		ReadNumber(Object, TEXT("HitRadius"), Slash.HitRadius);
		ReadNumber(Object, TEXT("Step"), Slash.Step);
		Object->TryGetBoolField(TEXT("UpperBodyOnly"), Slash.bUpperBodyOnly);

		float Reach = Slash.HitOffset.X;
		float Up = Slash.HitOffset.Y;
		ReadNumber(Object, TEXT("Reach"), Reach);
		ReadNumber(Object, TEXT("Up"), Up);
		Slash.HitOffset = FVector2D(Reach, Up);
	}

	FDateTime LastTuningTime = FDateTime::MinValue();
#endif
}

const TArray<FAmnesiaWeaponDef>& FAmnesiaWeapons::All()
{
	return Table();
}

const FAmnesiaWeaponDef* FAmnesiaWeapons::Find(FName Id)
{
	for (const FAmnesiaWeaponDef& Def : Table())
	{
		if (Def.Id == Id)
		{
			return &Def;
		}
	}

	return nullptr;
}

const FAmnesiaWeaponDef* FAmnesiaWeapons::FindByBranch(EAmnesiaSkillBranch Branch)
{
	for (const FAmnesiaWeaponDef& Def : Table())
	{
		if (Def.Id != SwordId() && Def.Branch == Branch)
		{
			return &Def;
		}
	}

	return nullptr;
}

bool FAmnesiaWeapons::ReloadTuning()
{
#if WITH_EDITOR
	const FString Path = FPaths::ProjectSavedDir() / TEXT("WeaponTuning.json");
	const FDateTime Time = IFileManager::Get().GetTimeStamp(*Path);

	if (Time == LastTuningTime)
	{
		return false;
	}

	LastTuningTime = Time;

	// the numbers of the table again, then the file on top of them
	Table() = Build();

	FString Text;

	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		return true;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponTuning.json cannot be read"));
		return true;
	}

	for (FAmnesiaWeaponDef& Def : Table())
	{
		const TSharedPtr<FJsonObject>* Object;

		if (!Root->TryGetObjectField(Def.Id.ToString(), Object))
		{
			continue;
		}

		ReadVector(*Object, TEXT("HandLocation"), Def.HandLocation);
		ReadRotator(*Object, TEXT("HandRotation"), Def.HandRotation);
		ReadVector(*Object, TEXT("HandScale"), Def.HandScale);
		ReadVector(*Object, TEXT("AuraLocation"), Def.AuraLocation);
		ReadRotator(*Object, TEXT("AuraRotation"), Def.AuraRotation);
		ReadVector(*Object, TEXT("AuraScale"), Def.AuraScale);
		(*Object)->TryGetStringField(TEXT("AuraPath"), Def.AuraPath);
		(*Object)->TryGetStringField(TEXT("MeshPath"), Def.MeshPath);
		ReadNumber(*Object, TEXT("DamageMult"), Def.DamageMult);
		ReadNumber(*Object, TEXT("RateMult"), Def.RateMult);
		ReadNumber(*Object, TEXT("ReachMult"), Def.ReachMult);
		ReadNumber(*Object, TEXT("RadiusMult"), Def.RadiusMult);

		const TArray<TSharedPtr<FJsonValue>>* Combo;

		if ((*Object)->TryGetArrayField(TEXT("Combo"), Combo))
		{
			Def.Combo.Reset();

			for (const TSharedPtr<FJsonValue>& Value : *Combo)
			{
				const TSharedPtr<FJsonObject>* SlashObject;

				if (Value->TryGetObject(SlashObject))
				{
					FAmnesiaWeaponSlash NewSlash;
					ReadSlash(*SlashObject, NewSlash);
					Def.Combo.Add(NewSlash);
				}
			}
		}

		const TSharedPtr<FJsonObject>* HeavyObject;

		if ((*Object)->TryGetObjectField(TEXT("Heavy"), HeavyObject))
		{
			ReadSlash(*HeavyObject, Def.Heavy);
			Def.bHasHeavy = true;
		}
	}

	return true;
#else
	return false;
#endif
}
