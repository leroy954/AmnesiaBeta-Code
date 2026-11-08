#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaSound.h"
#include "AmnesiaGiant.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UAnimSequenceBase;

/**
 *  A giant armored stone golem standing in the background, a guardian that barely moves.
 *  It breathes in its idle, now and then shifts with a slow heavy motion and a deep rumble,
 *  and slowly turns to keep an eye on the hero when the hero is near. It never fights.
 */
UCLASS()
class AAmnesiaGiant : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Hammer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Shield;

protected:

	/** Breathing idle, played slowly */
	UPROPERTY(EditAnywhere, Category="Giant")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	/** Slow shifts played now and then */
	UPROPERTY(EditAnywhere, Category="Giant")
	TArray<TObjectPtr<UAnimSequenceBase>> ShiftAnimations;

	/** How slowly everything plays: big things move slowly */
	UPROPERTY(EditAnywhere, Category="Giant", meta = (ClampMin = 0.05))
	float PlayRate = 0.5f;

	/** Seconds between shifts */
	UPROPERTY(EditAnywhere, Category="Giant", meta = (Units = "s"))
	float MinShiftInterval = 10.0f;

	UPROPERTY(EditAnywhere, Category="Giant", meta = (Units = "s"))
	float MaxShiftInterval = 20.0f;

	/** The giant turns towards the hero while the hero is closer than this, at TurnSpeed degrees per second, at most MaxTurn away from its start */
	UPROPERTY(EditAnywhere, Category="Giant", meta = (Units = "cm"))
	float WatchRange = 9000.0f;

	UPROPERTY(EditAnywhere, Category="Giant", meta = (Units = "deg"))
	float TurnSpeed = 4.0f;

	UPROPERTY(EditAnywhere, Category="Giant", meta = (Units = "deg"))
	float MaxTurn = 35.0f;

	/** Yaw of the mesh against the actor, so the model faces the actor's forward axis */
	UPROPERTY(EditAnywhere, Category="Giant")
	float MeshYawOffset = -90.0f;

	/** The rumble and the shake only happen once the hero is past this X, the end of the cave: the giant is not heard from the earlier zones */
	UPROPERTY(EditAnywhere, Category="Giant", meta = (Units = "cm"))
	float HearFromHeroX = 54000.0f;

	/** Deep rumble with each shift */
	UPROPERTY(EditAnywhere, Category="Giant")
	FAmnesiaSoundSet RumbleSounds;

	/** Camera shake on the hero with each shift, when near */
	UPROPERTY(EditAnywhere, Category="Giant")
	float ShakeStrength = 4.0f;

	float NextShiftTime = 0.0f;
	float StartYaw = 0.0f;

public:

	AAmnesiaGiant();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;
};
