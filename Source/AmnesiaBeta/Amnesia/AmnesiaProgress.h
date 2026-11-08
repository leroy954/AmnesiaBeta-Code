#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Scalability.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AmnesiaProgress.generated.h"

/**
 *  Everything that survives death, restarts and quitting the game
 */
UCLASS()
class UAmnesiaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	/** Hero level */
	UPROPERTY()
	int32 Level = 1;

	/** XP towards the next level */
	UPROPERTY()
	int32 XP = 0;

	/** Index of the last checkpoint the hero rested at. -1 means none yet */
	UPROPERTY()
	int32 CheckpointIndex = -1;

	/** Where the hero respawns */
	UPROPERTY()
	FTransform CheckpointTransform;

	/** Master volume, 0 to 1 */
	UPROPERTY()
	float MasterVolume = 0.8f;

	/** If false, the camera doesn't shake on hits */
	UPROPERTY()
	bool bScreenShake = true;

	/** If false, damage numbers are hidden */
	UPROPERTY()
	bool bDamageNumbers = true;

	/** Graphics quality: 0 low, 1 medium, 2 high, 3 max */
	UPROPERTY()
	int32 GraphicsQuality = 3;

	/** Rank of every learned skill, by skill id */
	UPROPERTY()
	TMap<FName, int32> SkillRanks;

	/** The element on the sword: 0 none, 1 fire, 2 ice */
	UPROPERTY()
	uint8 SkillElement = 0;

	/** Gold in the purse, from killed enemies */
	UPROPERTY()
	int32 Gold = 0;

	/** Ids of the things bought in the shop */
	UPROPERTY()
	TSet<FName> Owned;

	/** The weapon in the hero's hand, an id of the weapon table. None means the sword */
	UPROPERTY()
	FName EquippedWeapon;

	/** Ids of the cats that were found (petted) */
	UPROPERTY()
	TSet<FName> PettedCats;

	/** Skill points the cats gave: one for finding all of them */
	UPROPERTY()
	int32 CatSkillPoints = 0;
};

/**
 *  Keeps the hero's progress across respawns and writes it to disk.
 *  The respawned hero is a new actor, so its level and XP live here.
 */
UCLASS()
class UAmnesiaProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/** Current progress */
	UPROPERTY()
	TObjectPtr<UAmnesiaSaveGame> Progress;

	/** The quality levels the editor had before the game changed them, put back when the game ends */
	Scalability::FQualityLevels OriginalQualityLevels;
	bool bHasOriginalQualityLevels = false;

	/** The ids of the cats that are in the loaded level: each cat registers when it starts. Not saved */
	TSet<FName> KnownCats;

public:

	/** Slot the progress is saved in */
	static const FString SlotName;

	/** Loads the settings, so they apply from the main menu on */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Gives the editor its own quality levels back */
	virtual void Deinitialize() override;

	/** Returns the subsystem of a world context, or nullptr */
	static UAmnesiaProgressSubsystem* Get(const UObject* WorldContext);

	/** Current progress, never null */
	UAmnesiaSaveGame* GetProgress();

	/** True if a saved game exists on disk */
	bool HasSavedGame() const;

	/** Throws away the progress but keeps the settings */
	void StartNewGame();

	/** Loads the progress from disk. Returns false if there is none */
	bool LoadGame();

	/** Writes the progress to disk */
	void SaveGame();

	/** Stores the hero's level and XP */
	void StoreHero(int32 Level, int32 XP);

	/** Stores a checkpoint and saves */
	void StoreCheckpoint(int32 Index, const FTransform& Transform);

	/** Gold: killed enemies give it, the shop takes it. SpendGold returns false if there is not enough */
	int32 GetGold() { return GetProgress()->Gold; }
	void AddGold(int32 Amount);
	bool SpendGold(int32 Amount);

	/** The things bought in the shop */
	bool Owns(FName Item) { return GetProgress()->Owned.Contains(Item); }
	void GiveItem(FName Item);

	/** Test helper of the admin panel: the skill points never run out. Not saved */
	bool bInfiniteSkillPoints = false;

	/** The cats of the zones. A cat tells its id when the level starts, so the counter knows how many there are to find */
	void RegisterCat(FName Id);
	int32 GetCatTotal() const { return KnownCats.Num(); }

	/** True if the cat was found before */
	bool HasPettedCat(FName Id) { return GetProgress()->PettedCats.Contains(Id); }

	/** Marks a cat as found. True the first time, false if it was found before */
	bool PetCat(FName Id);

	/** How many of the cats of the level were found */
	int32 GetCatsFound();

	/** Applies the volume setting to the audio device of the world. The engine puts it back to full on every level load,
	 *  so the HUD applies it again when a level starts */
	void ApplyAudioSettings();

	/** Applies the graphics quality setting, through the engine's quality groups (shadows, lighting, resolution and so on) */
	void ApplyGraphicsSettings();

	/** True when the player picked Continue, so the lit checkpoints stay lit */
	bool bContinueFromCheckpoint = false;

	/** True until the first hero of a continued game moved to the checkpoint */
	bool bPendingCheckpointStart = false;

	/** True when the level reloads for Continue, so the main menu doesn't come up again */
	bool bSkipMainMenuOnce = false;
};

/** The dragon in the shop: its id in the saved list of bought things. It unlocks the dragon skills and brings the dragon along */
inline FName AmnesiaDragonItemId()
{
	return FName(TEXT("dragon"));
}
