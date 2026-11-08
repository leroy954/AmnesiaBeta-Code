#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaSound.h"
#include "AmnesiaCheckpoint.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UNiagaraComponent;
class USoundBase;

/**
 *  A resting place along the level.
 *  Walking past it lights the fire, heals the hero fully, saves the game
 *  and makes it the place the hero comes back to after dying.
 */
UCLASS()
class AAmnesiaCheckpoint : public AActor
{
	GENERATED_BODY()

	/** Walk-through trigger */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* Trigger;

	/** The fire bowl column */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** Fire light, dim until the checkpoint is lit */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* FireLight;

	/** Sparks rising from the fire once lit */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* Sparks;

	/** Where the hero respawns, relative to the checkpoint */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SpawnPoint;

protected:

	/** Order of this checkpoint along the level. Continue starts at the highest one reached */
	UPROPERTY(EditAnywhere, Category="Checkpoint")
	int32 CheckpointIndex = 0;

	/** Name shown when the checkpoint is lit */
	UPROPERTY(EditAnywhere, Category="Checkpoint")
	FText DisplayName = FText::FromString(TEXT("Pihenőhely"));

	/** Sound played when the fire lights up */
	UPROPERTY(EditAnywhere, Category="Checkpoint")
	FAmnesiaSoundSet LightSounds;

	/** Light brightness before and after lighting, in candelas */
	UPROPERTY(EditAnywhere, Category="Checkpoint")
	float DimIntensity = 15.0f;

	UPROPERTY(EditAnywhere, Category="Checkpoint")
	float LitIntensity = 110.0f;

	/** True once the hero rested here */
	bool bLit = false;

	/** Time the fire was lit, for the flare up */
	float LitTime = -100.0f;

public:

	AAmnesiaCheckpoint();

	/** Flickers the fire */
	virtual void Tick(float DeltaTime) override;

	/** Index along the level */
	int32 GetCheckpointIndex() const { return CheckpointIndex; }

	/** World transform the hero respawns at */
	FTransform GetSpawnTransform() const;

	/** Lights the fire without the heal and save, for checkpoints already reached in a saved game */
	void SetLitSilently();

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
