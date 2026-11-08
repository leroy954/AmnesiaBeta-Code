#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AmnesiaCreatureAnimInstance.generated.h"

class UAnimSequenceBase;

/** One animation sampled by the creature anim proxy */
struct FAmnesiaAnimSample
{
	/** Animation to sample */
	const UAnimSequenceBase* Animation = nullptr;

	/** Time to sample the animation at */
	float Time = 0.0f;

	/** If true, sampling wraps around the end of the animation */
	bool bLooping = true;
};

/**
 *  Worker thread side of the creature anim instance.
 *  Samples and blends the animations picked on the game thread.
 */
struct FAmnesiaCreatureAnimProxy : public FAnimInstanceProxy
{
	FAmnesiaCreatureAnimProxy() = default;

	FAmnesiaCreatureAnimProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	/** Copies the animation state from the anim instance */
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;

	/** Builds the final pose without an anim graph */
	virtual bool Evaluate(FPoseContext& Output) override;

protected:

	/** Locomotion animation playing now */
	FAmnesiaAnimSample Locomotion;

	/** Locomotion animation we're fading out of */
	FAmnesiaAnimSample PreviousLocomotion;

	/** Weight of the current locomotion animation against the previous one */
	float LocomotionBlendAlpha = 1.0f;

	/** One-shot action playing on top of locomotion */
	FAmnesiaAnimSample Action;

	/** Weight of the action against locomotion */
	float ActionWeight = 0.0f;

	/** If true, the root bone is kept in place horizontally */
	bool bLockRootBone = true;

	/** Samples one animation into a pose */
	void SamplePose(const FAmnesiaAnimSample& Sample, FPoseContext& Out) const;

	/** Builds the locomotion pose, cross-fading from the previous locomotion animation */
	void EvaluateLocomotion(FPoseContext& Out) const;
};

/**
 *  Graph-free anim instance for creatures that come with plain animation sequences.
 *  - Picks idle, walk or run from the owner's ground speed and cross-fades between them
 *  - Plays one-shot actions like attacks, hit reactions and deaths on top
 *  Set it as the Anim Class of a skeletal mesh. The owner fills in the animations at runtime.
 */
UCLASS(Transient, NotBlueprintable)
class UAmnesiaCreatureAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend struct FAmnesiaCreatureAnimProxy;

public:

	/** Animation played while standing still */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	/** Animation played while walking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion")
	TObjectPtr<UAnimSequenceBase> WalkAnimation;

	/** Animation played while running */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion")
	TObjectPtr<UAnimSequenceBase> RunAnimation;

	/** Ground speed the walk animation was made for. The walk play rate follows the actual speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion", meta = (ClampMin = 1, Units = "cm/s"))
	float WalkAnimationSpeed = 150.0f;

	/** Ground speed the run animation was made for. The run play rate follows the actual speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion", meta = (ClampMin = 1, Units = "cm/s"))
	float RunAnimationSpeed = 350.0f;

	/** Ground speed above which the walk animation is used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion", meta = (ClampMin = 0, Units = "cm/s"))
	float WalkThreshold = 10.0f;

	/** Ground speed above which the run animation is used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion", meta = (ClampMin = 0, Units = "cm/s"))
	float RunThreshold = 250.0f;

	/** Cross-fade time between locomotion animations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion", meta = (ClampMin = 0, ClampMax = 2, Units = "s"))
	float LocomotionBlendTime = 0.25f;

	/** Fade time in and out of actions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Actions", meta = (ClampMin = 0, ClampMax = 2, Units = "s"))
	float ActionBlendTime = 0.15f;

	/** If true, the root bone is kept in place so animations with root motion don't drift away from the capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion")
	bool bLockRootBone = true;

public:

	/** Plays a one-shot action on top of locomotion. Returns its length in seconds */
	UFUNCTION(BlueprintCallable, Category="Creature Animation")
	float PlayAction(UAnimSequenceBase* Animation, float PlayRate = 1.0f, bool bHoldLastFrame = false);

	/** Fades out the current action */
	UFUNCTION(BlueprintCallable, Category="Creature Animation")
	void StopAction();

	/** Returns true while an action is playing and not fading out */
	UFUNCTION(BlueprintPure, Category="Creature Animation")
	bool IsPlayingAction() const;

protected:

	/** Picks the locomotion animation and advances all animation times */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Creates the graph-free proxy */
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	/** Destroys the graph-free proxy */
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

	/** Advances a looping animation time */
	static float AdvanceLoopingTime(const UAnimSequenceBase* Animation, float Time, float DeltaTime);

	/** Locomotion animation playing now */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> CurrentLocomotion;

	/** Locomotion animation we're fading out of */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> PreviousLocomotion;

	/** Action playing on top of locomotion */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> CurrentAction;

	/** Time into the current locomotion animation */
	float CurrentLocomotionTime = 0.0f;

	/** Time into the previous locomotion animation */
	float PreviousLocomotionTime = 0.0f;

	/** Weight of the current locomotion animation against the previous one */
	float LocomotionBlendAlpha = 1.0f;

	/** Time into the current action */
	float ActionTime = 0.0f;

	/** Play rate of the current action */
	float ActionPlayRate = 1.0f;

	/** Weight of the action against locomotion */
	float ActionWeight = 0.0f;

	/** If true, the action freezes on its last frame instead of fading out */
	bool bHoldActionLastFrame = false;

	/** If true, the action is fading out */
	bool bActionStopping = false;
};
