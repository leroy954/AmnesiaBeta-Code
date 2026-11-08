#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmnesiaFrost.generated.h"

class UMaterialInstanceDynamic;
class USoundBase;
class UStaticMeshComponent;

/**
 *  The ice around a frozen enemy: crystals grow out of the ground around its body, and when the ice breaks they fly apart.
 *  Only to look at: no collision, no shadows, and nothing in it hurts. The enemy decides when it grows and when it breaks.
 */
UCLASS()
class AAmnesiaFrost : public AActor
{
	GENERATED_BODY()

public:

	AAmnesiaFrost();

	virtual void Tick(float DeltaTime) override;

	/** Grows the crystals around Target, whose body is Radius wide and HalfHeight tall (the capsule), and follows it from now on */
	void Grow(AActor* Target, float Radius, float HalfHeight);

	/** The ice breaks: the crystals fly apart and the actor is gone a moment later */
	void Shatter();

	/** The ice is gone at once, with no pieces */
	void Melt();

	/** Seconds the crystals take to grow, and how long the pieces fly */
	UPROPERTY(EditAnywhere, Category="Frost", meta = (ClampMin = 0.05, Units = "s"))
	float GrowTime = 0.28f;

	UPROPERTY(EditAnywhere, Category="Frost", meta = (ClampMin = 0.2, Units = "s"))
	float ShatterTime = 1.0f;

	/** The color and the look of the crystals: the status overlay material with an icy color, bright at the rim */
	UPROPERTY(EditAnywhere, Category="Frost")
	FLinearColor CrystalColor = FLinearColor(0.55f, 0.88f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Frost")
	float CrystalRimBoost = 3.2f;

	UPROPERTY(EditAnywhere, Category="Frost")
	float CrystalRimPower = 1.5f;

	/** Sounds of the ice forming and breaking */
	UPROPERTY(EditAnywhere, Category="Frost")
	TSoftObjectPtr<USoundBase> FreezeSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/World/SW_Ice_Freeze_01.SW_Ice_Freeze_01")));

	UPROPERTY(EditAnywhere, Category="Frost")
	TSoftObjectPtr<USoundBase> ShatterSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/World/SW_Ice_Shatter_01.SW_Ice_Shatter_01")));

private:

	/** What a crystal does: where it stands, how big it gets and when it starts to grow, and how it flies once the ice breaks */
	struct FPiece
	{
		FVector BaseLocation = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		float FullScale = 1.0f;
		float Delay = 0.0f;

		FVector Velocity = FVector::ZeroVector;
		FVector Spin = FVector::ZeroVector;
		FVector FlyScale = FVector::OneVector;
	};

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Meshes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Crystal;

	TArray<FPiece> Pieces;

	float Age = 0.0f;
	float ShatterAge = 0.0f;
	bool bShattering = false;

	void PlaySoundAt(const TSoftObjectPtr<USoundBase>& Sound, float Volume);
};
