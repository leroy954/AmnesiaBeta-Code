#include "AmnesiaCreatureAnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Blends two poses into an output pose */
	void BlendPoses(FPoseContext& PoseA, FPoseContext& PoseB, float WeightOfA, FPoseContext& Out)
	{
		const FAnimationPoseData DataA(PoseA);
		const FAnimationPoseData DataB(PoseB);
		FAnimationPoseData OutData(Out);

		FAnimationRuntime::BlendTwoPosesTogether(DataA, DataB, WeightOfA, OutData);
	}
}

void FAmnesiaCreatureAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	const UAmnesiaCreatureAnimInstance* Creature = CastChecked<UAmnesiaCreatureAnimInstance>(InAnimInstance);

	Locomotion.Animation = Creature->CurrentLocomotion.Get();
	Locomotion.Time = Creature->CurrentLocomotionTime;
	Locomotion.bLooping = true;

	PreviousLocomotion.Animation = Creature->PreviousLocomotion.Get();
	PreviousLocomotion.Time = Creature->PreviousLocomotionTime;
	PreviousLocomotion.bLooping = true;

	LocomotionBlendAlpha = Creature->LocomotionBlendAlpha;

	Action.Animation = Creature->CurrentAction.Get();
	Action.Time = Creature->ActionTime;
	Action.bLooping = false;

	ActionWeight = Action.Animation ? Creature->ActionWeight : 0.0f;

	bLockRootBone = Creature->bLockRootBone;
}

bool FAmnesiaCreatureAnimProxy::Evaluate(FPoseContext& Output)
{
	if (!Action.Animation || ActionWeight <= 0.0f)
	{
		// locomotion only
		EvaluateLocomotion(Output);

	} else if (ActionWeight >= 1.0f) {

		// action only
		SamplePose(Action, Output);

	} else {

		// fade between locomotion and the action
		FPoseContext LocomotionPose(Output);
		FPoseContext ActionPose(Output);

		EvaluateLocomotion(LocomotionPose);
		SamplePose(Action, ActionPose);

		BlendPoses(ActionPose, LocomotionPose, ActionWeight, Output);
	}

	// keep the root under the capsule
	if (bLockRootBone)
	{
		const FCompactPoseBoneIndex RootIndex(0);

		if (Output.Pose.IsValidIndex(RootIndex))
		{
			FTransform& Root = Output.Pose[RootIndex];
			Root.SetTranslation(FVector(0.0f, 0.0f, Root.GetTranslation().Z));
		}
	}

	return true;
}

void FAmnesiaCreatureAnimProxy::SamplePose(const FAmnesiaAnimSample& Sample, FPoseContext& Out) const
{
	if (!Sample.Animation)
	{
		Out.ResetToRefPose();
		return;
	}

	FAnimationPoseData PoseData(Out);
	Sample.Animation->GetAnimationPose(PoseData, FAnimExtractContext(static_cast<double>(Sample.Time), false, FDeltaTimeRecord(), Sample.bLooping));
}

void FAmnesiaCreatureAnimProxy::EvaluateLocomotion(FPoseContext& Out) const
{
	// cross-fade from the previous locomotion animation if we just switched
	if (PreviousLocomotion.Animation && LocomotionBlendAlpha < 1.0f)
	{
		FPoseContext CurrentPose(Out);
		FPoseContext PreviousPose(Out);

		SamplePose(Locomotion, CurrentPose);
		SamplePose(PreviousLocomotion, PreviousPose);

		BlendPoses(CurrentPose, PreviousPose, LocomotionBlendAlpha, Out);

		return;
	}

	SamplePose(Locomotion, Out);
}

float UAmnesiaCreatureAnimInstance::PlayAction(UAnimSequenceBase* Animation, float PlayRate, bool bHoldLastFrame)
{
	if (!Animation)
	{
		return 0.0f;
	}

	// restart from the beginning, fading in from whatever weight the last action had
	CurrentAction = Animation;
	ActionTime = 0.0f;
	ActionPlayRate = FMath::Max(PlayRate, 0.01f);
	bHoldActionLastFrame = bHoldLastFrame;
	bActionStopping = false;

	return Animation->GetPlayLength() / ActionPlayRate;
}

void UAmnesiaCreatureAnimInstance::StopAction()
{
	if (CurrentAction)
	{
		bActionStopping = true;
	}
}

bool UAmnesiaCreatureAnimInstance::IsPlayingAction() const
{
	return CurrentAction && !bActionStopping;
}

void UAmnesiaCreatureAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// pick the locomotion animation from the owner's speed. Pawns report their movement, other actors their root velocity
	const AActor* Owner = GetOwningActor();
	const float Speed = Owner ? Owner->GetVelocity().Size2D() : 0.0f;

	UAnimSequenceBase* DesiredLocomotion = IdleAnimation;
	float LocomotionPlayRate = 1.0f;

	if (Speed > RunThreshold && RunAnimation)
	{
		DesiredLocomotion = RunAnimation;
		LocomotionPlayRate = Speed / RunAnimationSpeed;

	} else if (Speed > WalkThreshold && WalkAnimation) {

		DesiredLocomotion = WalkAnimation;
		LocomotionPlayRate = Speed / WalkAnimationSpeed;
	}

	LocomotionPlayRate = FMath::Clamp(LocomotionPlayRate, 0.5f, 2.0f);

	// start a cross-fade when the locomotion animation changes
	if (DesiredLocomotion != CurrentLocomotion)
	{
		PreviousLocomotion = CurrentLocomotion;
		PreviousLocomotionTime = CurrentLocomotionTime;

		CurrentLocomotion = DesiredLocomotion;
		CurrentLocomotionTime = 0.0f;

		LocomotionBlendAlpha = PreviousLocomotion ? 0.0f : 1.0f;
	}

	CurrentLocomotionTime = AdvanceLoopingTime(CurrentLocomotion, CurrentLocomotionTime, DeltaSeconds * LocomotionPlayRate);
	PreviousLocomotionTime = AdvanceLoopingTime(PreviousLocomotion, PreviousLocomotionTime, DeltaSeconds);

	LocomotionBlendAlpha = LocomotionBlendTime > 0.0f ? FMath::Min(1.0f, LocomotionBlendAlpha + DeltaSeconds / LocomotionBlendTime) : 1.0f;

	// advance the action and fade it in or out
	if (CurrentAction)
	{
		const float Length = CurrentAction->GetPlayLength();

		ActionTime = FMath::Min(ActionTime + DeltaSeconds * ActionPlayRate, Length);

		// start fading out just before the end, unless we hold the last frame
		if (!bHoldActionLastFrame && ActionTime >= Length - ActionBlendTime * ActionPlayRate)
		{
			bActionStopping = true;
		}

		const float FadeStep = ActionBlendTime > 0.0f ? DeltaSeconds / ActionBlendTime : 1.0f;

		ActionWeight = bActionStopping ? ActionWeight - FadeStep : FMath::Min(1.0f, ActionWeight + FadeStep);

		// drop the action once it has faded out
		if (bActionStopping && ActionWeight <= 0.0f)
		{
			CurrentAction = nullptr;
			ActionWeight = 0.0f;
		}
	}
}

FAnimInstanceProxy* UAmnesiaCreatureAnimInstance::CreateAnimInstanceProxy()
{
	return new FAmnesiaCreatureAnimProxy(this);
}

void UAmnesiaCreatureAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}

float UAmnesiaCreatureAnimInstance::AdvanceLoopingTime(const UAnimSequenceBase* Animation, float Time, float DeltaTime)
{
	if (!Animation)
	{
		return 0.0f;
	}

	const float Length = Animation->GetPlayLength();

	return Length > 0.0f ? FMath::Fmod(Time + DeltaTime, Length) : 0.0f;
}
