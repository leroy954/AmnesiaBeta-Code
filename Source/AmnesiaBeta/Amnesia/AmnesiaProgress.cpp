#include "AmnesiaProgress.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

const FString UAmnesiaProgressSubsystem::SlotName(TEXT("AmnesiaProgress"));

void UAmnesiaProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// start from the saved game if there is one, so the settings carry over
	if (!LoadGame())
	{
		Progress = NewObject<UAmnesiaSaveGame>(this);
	}

	ApplyAudioSettings();
	ApplyGraphicsSettings();
}

void UAmnesiaProgressSubsystem::Deinitialize()
{
	// the quality chosen in the game is not the editor's: its viewports go back to what they had
	if (bHasOriginalQualityLevels && GIsEditor)
	{
		Scalability::SetQualityLevels(OriginalQualityLevels);
		bHasOriginalQualityLevels = false;
	}

	Super::Deinitialize();
}

UAmnesiaProgressSubsystem* UAmnesiaProgressSubsystem::Get(const UObject* WorldContext)
{
	if (const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UAmnesiaProgressSubsystem>();
		}
	}

	return nullptr;
}

UAmnesiaSaveGame* UAmnesiaProgressSubsystem::GetProgress()
{
	if (!Progress)
	{
		Progress = NewObject<UAmnesiaSaveGame>(this);
	}

	return Progress;
}

bool UAmnesiaProgressSubsystem::HasSavedGame() const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void UAmnesiaProgressSubsystem::StartNewGame()
{
	UAmnesiaSaveGame* Old = GetProgress();

	UAmnesiaSaveGame* Fresh = NewObject<UAmnesiaSaveGame>(this);
	Fresh->MasterVolume = Old->MasterVolume;
	Fresh->bScreenShake = Old->bScreenShake;
	Fresh->bDamageNumbers = Old->bDamageNumbers;
	Fresh->GraphicsQuality = Old->GraphicsQuality;

	Progress = Fresh;
	bContinueFromCheckpoint = false;

	SaveGame();
}

bool UAmnesiaProgressSubsystem::LoadGame()
{
	if (!HasSavedGame())
	{
		return false;
	}

	if (UAmnesiaSaveGame* Loaded = Cast<UAmnesiaSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
	{
		Progress = Loaded;
		return true;
	}

	return false;
}

void UAmnesiaProgressSubsystem::SaveGame()
{
	UGameplayStatics::SaveGameToSlot(GetProgress(), SlotName, 0);
}

void UAmnesiaProgressSubsystem::StoreHero(int32 Level, int32 XP)
{
	GetProgress()->Level = Level;
	GetProgress()->XP = XP;
}

void UAmnesiaProgressSubsystem::StoreCheckpoint(int32 Index, const FTransform& Transform)
{
	GetProgress()->CheckpointIndex = Index;
	GetProgress()->CheckpointTransform = Transform;

	SaveGame();
}

void UAmnesiaProgressSubsystem::AddGold(int32 Amount)
{
	UAmnesiaSaveGame* Current = GetProgress();
	Current->Gold = FMath::Max(Current->Gold + Amount, 0);
}

bool UAmnesiaProgressSubsystem::SpendGold(int32 Amount)
{
	UAmnesiaSaveGame* Current = GetProgress();

	if (Amount < 0 || Current->Gold < Amount)
	{
		return false;
	}

	Current->Gold -= Amount;
	return true;
}

void UAmnesiaProgressSubsystem::GiveItem(FName Item)
{
	GetProgress()->Owned.Add(Item);
}

void UAmnesiaProgressSubsystem::RegisterCat(FName Id)
{
	if (!Id.IsNone())
	{
		KnownCats.Add(Id);
	}
}

bool UAmnesiaProgressSubsystem::PetCat(FName Id)
{
	bool bWasThere = false;
	GetProgress()->PettedCats.Add(Id, &bWasThere);

	return !bWasThere;
}

int32 UAmnesiaProgressSubsystem::GetCatsFound()
{
	int32 Found = 0;

	for (const FName& Id : GetProgress()->PettedCats)
	{
		if (KnownCats.Contains(Id))
		{
			++Found;
		}
	}

	return Found;
}

void UAmnesiaProgressSubsystem::ApplyAudioSettings()
{
	const float Volume = GetProgress()->MasterVolume;

	// the audio device of the world that plays. In the editor every play session gets a device of its own
	// and the main one belongs to the editor, so setting the volume there does nothing for the game
	if (const UWorld* World = GetWorld())
	{
		if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
		{
			AudioDevice->SetTransientPrimaryVolume(Volume);
			return;
		}
	}

	// a game that has no world yet has a single device, the main one
	if (GEngine && !GIsEditor)
	{
		if (FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
		{
			AudioDevice->SetTransientPrimaryVolume(Volume);
		}
	}
}

void UAmnesiaProgressSubsystem::ApplyGraphicsSettings()
{
	if (!bHasOriginalQualityLevels)
	{
		OriginalQualityLevels = Scalability::GetQualityLevels();
		bHasOriginalQualityLevels = true;
	}

	// the engine's own groups: shadows, lighting, reflections, effects, post process, textures, view distance and resolution
	Scalability::FQualityLevels Levels;
	Levels.SetFromSingleQualityLevel(FMath::Clamp(GetProgress()->GraphicsQuality, 0, 3));
	Scalability::SetQualityLevels(Levels);
}
