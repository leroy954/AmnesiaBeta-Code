#include "AmnesiaHeroAnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimNodeBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void FAmnesiaHeroAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	const UAmnesiaHeroAnimInstance* Hero = CastChecked<UAmnesiaHeroAnimInstance>(InAnimInstance);

	Overlay = Hero->OverlayAnimation.Get();
	OverlayTime = Hero->OverlayTime;
	OverlayWeight = Overlay ? Hero->OverlayWeight : 0.0f;
	LowerBodyWeight = Hero->bOverlayUpperBodyOnly ? 0.0f : Hero->LowerBodyWeight;
	UpperBodyRoot = Hero->UpperBodyRoot;
}

bool FAmnesiaHeroAnimProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	// the Blueprint graph: locomotion, jumps, montages, foot placement
	EvaluateAnimationNode_WithRoot(Output, InRootNode);

	if (!Overlay || OverlayWeight <= 0.001f)
	{
		return true;
	}

	FPoseContext SwingPose(Output);
	FAnimationPoseData SwingData(SwingPose);
	Overlay->GetAnimationPose(SwingData, FAnimExtractContext(static_cast<double>(OverlayTime), false));

	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& Reference = Bones.GetReferenceSkeleton();
	const int32 UpperIndex = Reference.FindBoneIndex(UpperBodyRoot);

	for (const FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
	{
		// the root stays with the graph, so the swing never moves the hero's capsule
		if (Bone.GetInt() == 0)
		{
			continue;
		}

		const int32 MeshIndex = Bones.MakeMeshPoseIndex(Bone).GetInt();
		const bool bUpper = UpperIndex != INDEX_NONE && (MeshIndex == UpperIndex || Reference.BoneIsChildOf(MeshIndex, UpperIndex));
		const float Weight = OverlayWeight * (bUpper ? 1.0f : LowerBodyWeight);

		if (Weight > 0.001f)
		{
			FTransform Blended;
			Blended.Blend(Output.Pose[Bone], SwingPose.Pose[Bone], Weight);
			Output.Pose[Bone] = Blended;
		}
	}

	return true;
}

void UAmnesiaHeroAnimInstance::PlayOverlay(UAnimSequenceBase* Animation, float StartTime, float Duration, float PlayRate, bool bUpperBodyOnly)
{
	if (!Animation)
	{
		return;
	}

	OverlayAnimation = Animation;
	bOverlayUpperBodyOnly = bUpperBodyOnly;
	OverlayTime = StartTime;
	OverlayEndTime = FMath::Min(StartTime + Duration, Animation->GetPlayLength());
	OverlayRate = FMath::Max(PlayRate, 0.05f);
	bOverlayStopping = false;
}

void UAmnesiaHeroAnimInstance::StopOverlay()
{
	bOverlayStopping = true;
}

void UAmnesiaHeroAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// the legs swing along while standing, and keep walking, running, jumping or falling otherwise
	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	float WantedLegs = 0.0f;

	if (Movement && Movement->IsMovingOnGround() && Movement->GetCurrentAcceleration().IsNearlyZero())
	{
		const float Speed = Movement->Velocity.Size2D();
		WantedLegs = 1.0f - FMath::Clamp((Speed - 10.0f) / FMath::Max(LegsRunSpeed - 10.0f, 1.0f), 0.0f, 1.0f);
	}

	LowerBodyWeight = FMath::FInterpTo(LowerBodyWeight, WantedLegs, DeltaSeconds, 15.0f);

	if (!OverlayAnimation)
	{
		return;
	}

	OverlayTime += DeltaSeconds * OverlayRate;

	// start fading out a little before the end of the wanted part
	if (OverlayTime >= OverlayEndTime - OverlayBlendTime * OverlayRate)
	{
		bOverlayStopping = true;
	}

	OverlayTime = FMath::Min(OverlayTime, OverlayEndTime);

	const float Step = OverlayBlendTime > 0.0f ? DeltaSeconds / OverlayBlendTime : 1.0f;
	OverlayWeight = bOverlayStopping ? OverlayWeight - Step : FMath::Min(1.0f, OverlayWeight + Step);

	if (bOverlayStopping && OverlayWeight <= 0.0f)
	{
		OverlayAnimation = nullptr;
		OverlayWeight = 0.0f;
	}
}

FAnimInstanceProxy* UAmnesiaHeroAnimInstance::CreateAnimInstanceProxy()
{
	return new FAmnesiaHeroAnimProxy(this);
}

void UAmnesiaHeroAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}
