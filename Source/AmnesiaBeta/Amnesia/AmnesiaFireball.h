#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaSound.h"
#include "AmnesiaFireball.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UParticleSystemComponent;
class UMaterialInstanceDynamic;

/**
 *  The little dragon's fireball.
 *  Flies at its target and steers a little after it, bursts on the first enemy or wall it meets,
 *  and damages the enemy in the hero's name, so the kill and its XP go to the hero.
 */
UCLASS()
class AAmnesiaFireball : public AActor
{
	GENERATED_BODY()

	/** Hits enemies and stops at the ground and walls */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* Sphere;

	/** Bright core and the softer flame around it */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Core;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Flame;

	/** Flickering fire particles around the ball */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* Fire;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* Light;

protected:

	/** Flight speed */
	UPROPERTY(EditAnywhere, Category="Fireball", meta = (Units = "cm/s"))
	float Speed = 1500.0f;

	/** How hard the ball steers after its target */
	UPROPERTY(EditAnywhere, Category="Fireball", meta = (ClampMin = 0))
	float Homing = 5.0f;

	/** Seconds before an unspent fireball fizzles out */
	UPROPERTY(EditAnywhere, Category="Fireball", meta = (Units = "s"))
	float LifeTime = 2.5f;

	/** Push on the enemy it hits */
	UPROPERTY(EditAnywhere, Category="Fireball", meta = (Units = "cm/s"))
	float Knockback = 180.0f;

	/** Sounds of the burst */
	UPROPERTY(EditAnywhere, Category="Fireball|Sound")
	FAmnesiaSoundSet ImpactSounds;

	/** Sparks spawned where it bursts */
	UPROPERTY(EditAnywhere, Category="Fireball")
	TObjectPtr<class UNiagaraSystem> BurstEffect;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlameMaterial;

	/** What the fireball flies at and who it fights for */
	TWeakObjectPtr<AActor> Target;
	TWeakObjectPtr<AActor> Hero;
	float Damage = 1.0f;

	FVector Velocity = FVector::ZeroVector;
	float SpawnTime = 0.0f;

	/** Time of the burst. Negative while flying */
	float BurstTime = -1.0f;

public:

	AAmnesiaFireball();

	virtual void Tick(float DeltaTime) override;

	/** Sends the fireball at a target, fighting for the given hero */
	void Launch(AActor* InTarget, AActor* InHero, float InDamage);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Damages an enemy, if it is one that is alive */
	bool TryHit(AActor* Actor, const FVector& Location);

	/** Bursts into a flash of fire and goes away */
	void Burst(const FVector& Location);
};
