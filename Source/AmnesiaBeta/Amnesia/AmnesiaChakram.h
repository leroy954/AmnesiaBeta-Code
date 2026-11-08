#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaChakram.generated.h"

class AAmnesiaCharacter;
class AAmnesiaEnemy;
class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;

/** How far a thrown disc flies, how fast, how hard it hits, and what its skills add */
struct FAmnesiaChakramThrow
{
	float Damage = 1.0f;
	float Range = 800.0f;
	float Speed = 1800.0f;

	/** How many more enemies it jumps to after the first one it hits */
	int32 Bounces = 0;

	/** The whirl of the held throw: how long it spins at the far end, and the damage of each of its hits */
	float WhirlTime = 0.0f;
	float WhirlDamage = 0.0f;
};

/**
 *  The thrown disc of the chakram weapon.
 *  Flies straight ahead from the hero's hand, spinning, and hurts every enemy it passes. With the bounce skill it jumps from the
 *  first enemy it hits to the next ones. At the end of its flight it can whirl in place for a while (the held throw, once learned),
 *  then it comes back to the hand and hurts what it passes on the way back too. The damage goes in the hero's name.
 */
UCLASS()
class AAmnesiaChakram : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Disc;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* Glow;

public:

	AAmnesiaChakram();

	/** Sends the disc out from Start, along Facing (+1 right, -1 left) */
	void Throw(AAmnesiaCharacter* InHero, UStaticMesh* Mesh, const FVector& Start, float InFacing, const FAmnesiaChakramThrow& InThrow, const FLinearColor& Color);

	/** Turns the disc back to the hand at once */
	void Recall();

	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Half the width of the band the disc hurts in, and of the circle it hurts while whirling */
	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0, Units = "cm"))
	float HitRadius = 75.0f;

	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0, Units = "cm"))
	float WhirlRadius = 170.0f;

	/** How far the bounce looks for the next enemy */
	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0, Units = "cm"))
	float BounceReach = 650.0f;

	/** The size of the flying disc. A throw sets it from the disc in the hand: that size times FlightToHand */
	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0.1))
	float FlightScale = 2.4f;

	/** The flying disc against the one in the hand: a little bigger, so it reads in flight, but no sudden jump in size */
	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0.1))
	float FlightToHand = 1.12f;

	/** While it whirls the disc grows by this much and turns this many times faster, so the whirl reads */
	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0.1))
	float WhirlGrow = 1.5f;

	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0.1))
	float WhirlSpin = 2.6f;

	/** Turns per second of the spin */
	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0))
	float SpinRate = 3.5f;

	/** Seconds between the hits of the whirl */
	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0.05, Units = "s"))
	float WhirlHitInterval = 0.25f;

	/** The disc is caught when it comes this close to the hand */
	UPROPERTY(EditAnywhere, Category="Chakram", meta = (ClampMin = 0, Units = "cm"))
	float CatchDistance = 70.0f;

private:

	enum class EPhase : uint8
	{
		Out,
		Whirl,
		Back
	};

	void HitAround(float Radius, float Damage, TSet<TWeakObjectPtr<AActor>>* Once);
	AAmnesiaEnemy* NextBounceTarget() const;
	void StartBack();
	void StartWhirl();
	void SetDiscSize(float Scale);
	FVector HandLocation() const;

	TWeakObjectPtr<AAmnesiaCharacter> Hero;
	TWeakObjectPtr<AAmnesiaEnemy> BounceTarget;
	FAmnesiaChakramThrow Settings;
	EPhase Phase = EPhase::Out;
	float Facing = 1.0f;
	float Travelled = 0.0f;
	float WhirlLeft = 0.0f;
	float NextWhirlHit = 0.0f;
	float Age = 0.0f;
	int32 BouncesLeft = 0;

	/** The model's offset from the spin axis at scale 1, and where the flashes of the whirl go round */
	FVector DiscOffset = FVector::ZeroVector;
	float WhirlAngle = 0.0f;

	/** Every enemy is hit once on the way out and once on the way back */
	TSet<TWeakObjectPtr<AActor>> HitOut;
	TSet<TWeakObjectPtr<AActor>> HitBack;
};
