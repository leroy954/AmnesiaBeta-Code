#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaSound.h"
#include "AmnesiaPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class USoundBase;

/**
 *  A glowing healing orb. Bobs in place, drifts to the hero when it gets close and heals on touch.
 *  Enemies drop them, and a few sit along the path.
 */
UCLASS()
class AAmnesiaPickup : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* Sphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Orb;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Halo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* Glow;

protected:

	/** Fraction of the hero's max HP restored */
	UPROPERTY(EditAnywhere, Category="Pickup", meta = (ClampMin = 0, ClampMax = 1))
	float HealFraction = 0.35f;

	/** Distance at which the orb starts drifting to the hero */
	UPROPERTY(EditAnywhere, Category="Pickup", meta = (Units = "cm"))
	float MagnetRange = 320.0f;

	/** Orb color */
	UPROPERTY(EditAnywhere, Category="Pickup")
	FLinearColor OrbColor = FLinearColor(0.35f, 1.0f, 0.45f);

	/** Sound played on pickup, on top of the hero's heal sound */
	UPROPERTY(EditAnywhere, Category="Pickup")
	FAmnesiaSoundSet PickupSounds;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OrbMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HaloMaterial;

	/** Resting position the orb bobs around */
	FVector HomeLocation = FVector::ZeroVector;

	/** Launch velocity right after being dropped, so the orb pops out of the enemy */
	FVector PopVelocity = FVector::ZeroVector;

	/** Game time the pickup appeared. It can't be collected for a moment after a drop */
	float SpawnTime = 0.0f;

	bool bCollected = false;

public:

	AAmnesiaPickup();

	virtual void Tick(float DeltaTime) override;

	/** Gives the orb a little hop, for drops */
	void Pop(const FVector& Velocity);

protected:

	virtual void BeginPlay() override;

	/** Heals the hero and goes away */
	void Collect(class AAmnesiaCharacter* Hero);
};
