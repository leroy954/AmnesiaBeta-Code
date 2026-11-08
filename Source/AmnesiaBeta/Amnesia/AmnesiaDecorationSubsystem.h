#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AmnesiaDecorationSubsystem.generated.h"

/**
 *  Turns off collision on purely decorative actors when the game starts.
 *  Tag an actor with "Decoration" to keep it from blocking the hero and the enemies.
 *  Gameplay collision lives on separate actors, like the ground and the invisible walls.
 */
UCLASS()
class UAmnesiaDecorationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Actor tag that marks decorations */
	static const FName DecorationTag;

	/** Listens for streamed in levels */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Stops listening for streamed in levels */
	virtual void Deinitialize() override;

	/** Turns off collision on the decorations that are already loaded */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:

	/** Only runs in game and PIE worlds, so the editor keeps its collision for placing things */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Turns off collision on the decorations of a level that was streamed in */
	void OnLevelAddedToWorld(ULevel* Level, UWorld* World);

	/** Turns off collision on every decoration in a list of actors */
	static void DisableDecorationCollision(const TArray<TObjectPtr<AActor>>& Actors);

	/** Level added delegate handle */
	FDelegateHandle LevelAddedHandle;
};
