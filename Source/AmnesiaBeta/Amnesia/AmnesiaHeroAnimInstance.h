#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AmnesiaHeroAnimInstance.generated.h"

class UAnimSequenceBase;

/**
 *  Worker thread side of the hero anim instance.
 *  Runs the Blueprint graph as usual, then lays the sword swing over its pose.
 */
struct FAmnesiaHeroAnimProxy : public FAnimInstanceProxy
{
	FAmnesiaHeroAnimProxy() = default;

	FAmnesiaHeroAnimProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;

	/** The graph pose, with the swing on top */
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;

protected:

	const UAnimSequenceBase* Overlay = nullptr;
	float OverlayTime = 0.0f;
	float OverlayWeight = 0.0f;
	float LowerBodyWeight = 1.0f;
	FName UpperBodyRoot;
};

/**
 *  Parent class of the hero's Anim Blueprint.
 *  The graph keeps doing locomotion, jumps, montages and foot placement. Sword swings play through PlayOverlay:
 *  the upper body always follows the swing, the legs follow it only while the hero stands on the ground, so
 *  walking, running, jumping or falling keeps the legs doing that.
 */
UCLASS()
class UAmnesiaHeroAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend struct FAmnesiaHeroAnimProxy;

public:

	/** Bone the upper body starts at. It and every bone under it take the whole swing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay")
	FName UpperBodyRoot = FName("spine_01");

	/** Ground speed from which the legs walk instead of swinging. Pressing a direction counts as moving too */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay", meta = (Units = "cm/s"))
	float LegsRunSpeed = 60.0f;

	/** Fade time in and out of a swing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay", meta = (Units = "s"))
	float OverlayBlendTime = 0.08f;

	/** Plays part of an animation over the graph pose: from StartTime, for Duration seconds of the animation, at PlayRate.
	 *  bUpperBodyOnly keeps the legs with the graph even when the hero stands still (the guard) */
	void PlayOverlay(UAnimSequenceBase* Animation, float StartTime, float Duration, float PlayRate, bool bUpperBodyOnly = false);

	/** Fades the swing out early */
	void StopOverlay();

protected:

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> OverlayAnimation;

	float OverlayTime = 0.0f;
	float OverlayEndTime = 0.0f;
	float OverlayRate = 1.0f;
	float OverlayWeight = 0.0f;
	float LowerBodyWeight = 1.0f;
	bool bOverlayStopping = false;
	bool bOverlayUpperBodyOnly = false;
};
