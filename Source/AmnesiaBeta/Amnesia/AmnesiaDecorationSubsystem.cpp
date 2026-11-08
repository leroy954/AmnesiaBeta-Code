#include "AmnesiaDecorationSubsystem.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

const FName UAmnesiaDecorationSubsystem::DecorationTag(TEXT("Decoration"));

void UAmnesiaDecorationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UAmnesiaDecorationSubsystem::OnLevelAddedToWorld);
}

void UAmnesiaDecorationSubsystem::Deinitialize()
{
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);

	Super::Deinitialize();
}

void UAmnesiaDecorationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	for (const ULevel* Level : InWorld.GetLevels())
	{
		if (Level)
		{
			DisableDecorationCollision(Level->Actors);
		}
	}
}

bool UAmnesiaDecorationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAmnesiaDecorationSubsystem::OnLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	if (Level && World == GetWorld())
	{
		DisableDecorationCollision(Level->Actors);
	}
}

void UAmnesiaDecorationSubsystem::DisableDecorationCollision(const TArray<TObjectPtr<AActor>>& Actors)
{
	for (AActor* Actor : Actors)
	{
		if (Actor && Actor->ActorHasTag(DecorationTag))
		{
			Actor->SetActorEnableCollision(false);
		}
	}
}
