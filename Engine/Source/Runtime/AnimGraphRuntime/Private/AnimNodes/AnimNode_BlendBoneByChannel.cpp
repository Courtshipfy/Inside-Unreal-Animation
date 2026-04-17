// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/AnimNode_BlendBoneByChannel.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimTrace.h"
#include "Animation/AnimStats.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_BlendBoneByChannel)

/////////////////////////////////////////////////////
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel
// FAnimNode_BlendBoneByChannel

void FAnimNode_BlendBoneByChannel::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_Base::Initialize_AnyThread(Context);

	A.Initialize(Context);
	B.Initialize(Context);
}

void FAnimNode_BlendBoneByChannel::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	A.CacheBones(Context);
	B.CacheBones(Context);

	// Pre-validate bone entries, so we don't waste cycles every frame figuring it out.
 // 预先验证骨骼条目，这样我们就不会浪费每一帧的周期来解决它。
	ValidBoneEntries.Reset();
	const FBoneContainer& BoneContainer = Context.AnimInstanceProxy->GetRequiredBones();
	for (FBlendBoneByChannelEntry& Entry : BoneDefinitions)
	{
		Entry.SourceBone.Initialize(BoneContainer);
		Entry.TargetBone.Initialize(BoneContainer);

		if (Entry.SourceBone.IsValidToEvaluate(BoneContainer) && Entry.TargetBone.IsValidToEvaluate(BoneContainer)
			&& (Entry.bBlendTranslation || Entry.bBlendRotation || Entry.bBlendScale))
		{
			ValidBoneEntries.Add(Entry);
		}
	}
}

void FAnimNode_BlendBoneByChannel::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FAnimNode_BlendBoneByChannel_Update);
	GetEvaluateGraphExposedInputs().Execute(Context);

	InternalBlendAlpha = AlphaScaleBias.ApplyTo(Alpha);
	bBIsRelevant = FAnimWeight::IsRelevant(InternalBlendAlpha) && (ValidBoneEntries.Num() > 0);

	A.Update(Context);
	if (bBIsRelevant)
	{
		B.Update(Context.FractionalWeight(InternalBlendAlpha));
	}

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Alpha"), InternalBlendAlpha);
}

void FAnimNode_BlendBoneByChannel::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(BlendBoneByChannel, !IsInGameThread());

	A.Evaluate(Output);

	if (bBIsRelevant)
	{
		const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

		FPoseContext PoseB(Output);
		B.Evaluate(PoseB);

		// Faster code path in local space
  // 本地空间中更快的代码路径
		if (TransformsSpace == EBoneControlSpace::BCS_BoneSpace)
		{
			const FCompactPose& SourcePose = PoseB.Pose;
			FCompactPose& TargetPose = Output.Pose;

			for (const FBlendBoneByChannelEntry& Entry : ValidBoneEntries)
			{
				const FCompactPoseBoneIndex SourceBoneIndex = Entry.SourceBone.GetCompactPoseIndex(BoneContainer);
				const FCompactPoseBoneIndex TargetBoneIndex = Entry.TargetBone.GetCompactPoseIndex(BoneContainer);

				const FTransform SourceTransform = SourcePose[SourceBoneIndex];
				FTransform& TargetTransform = TargetPose[TargetBoneIndex];

				// Blend Transforms.
    // 混合变换。
				FTransform BlendedTransform;
				BlendedTransform.Blend(TargetTransform, SourceTransform, InternalBlendAlpha);

				// Filter through channels
    // 通过渠道过滤
				{
					if (Entry.bBlendTranslation)
					{
						TargetTransform.SetTranslation(BlendedTransform.GetTranslation());
					}

					if (Entry.bBlendRotation)
					{
						TargetTransform.SetRotation(BlendedTransform.GetRotation());
					}

					if (Entry.bBlendScale)
					{
						TargetTransform.SetScale3D(BlendedTransform.GetScale3D());
					}
				}
			}
		}
		// Slower code path where local transforms have to be converted to a different space.
  // 较慢的代码路径，其中局部转换必须转换到不同的空间。
		else
		{
			FCSPose<FCompactPose> TargetPoseCmpntSpace;
			TargetPoseCmpntSpace.InitPose(Output.Pose);

			FCSPose<FCompactPose> SourcePoseCmpntSpace;
			SourcePoseCmpntSpace.InitPose(PoseB.Pose);

			TArray<FBoneTransform> QueuedModifiedBoneTransforms;

			const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

			for (const FBlendBoneByChannelEntry& Entry : ValidBoneEntries)
			{
				const FCompactPoseBoneIndex SourceBoneIndex = Entry.SourceBone.GetCompactPoseIndex(BoneContainer);
				const FCompactPoseBoneIndex TargetBoneIndex = Entry.TargetBone.GetCompactPoseIndex(BoneContainer);

				FTransform SourceTransform = SourcePoseCmpntSpace.GetComponentSpaceTransform(SourceBoneIndex);
				FTransform TargetTransform = TargetPoseCmpntSpace.GetComponentSpaceTransform(TargetBoneIndex);

				// Convert Transforms to correct space.
    // 将变换转换为正确的空间。
				FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, SourcePoseCmpntSpace, SourceTransform, SourceBoneIndex, TransformsSpace);
				FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, TargetPoseCmpntSpace, TargetTransform, TargetBoneIndex, TransformsSpace);

				// Blend Transforms.
    // 混合变换。
				FTransform BlendedTransform;
				BlendedTransform.Blend(TargetTransform, SourceTransform, InternalBlendAlpha);

				// Filter through channels
    // 通过渠道过滤
				{
					if (Entry.bBlendTranslation)
					{
						TargetTransform.SetTranslation(BlendedTransform.GetTranslation());
					}

					if (Entry.bBlendRotation)
					{
						TargetTransform.SetRotation(BlendedTransform.GetRotation());
					}

					if (Entry.bBlendScale)
					{
						TargetTransform.SetScale3D(BlendedTransform.GetScale3D());
					}
				}

				// Convert blended and filtered result back in component space.
    // 将混合和过滤的结果转换回组件空间。
				FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, TargetPoseCmpntSpace, TargetTransform, TargetBoneIndex, TransformsSpace);

				// Queue transform to be applied after all transforms have been created.
    // 创建所有转换后要应用的队列转换。
				// So we don't have parent bones affecting children bones.
    // 所以我们没有父骨骼影响子骨骼。
				QueuedModifiedBoneTransforms.Add(FBoneTransform(TargetBoneIndex, TargetTransform));
			}

			if (QueuedModifiedBoneTransforms.Num() > 0)
			{
				// Sort OutBoneTransforms so indices are in increasing order.
    // 对 OutBoneTransforms 进行排序，使索引按升序排列。
				QueuedModifiedBoneTransforms.Sort(FCompareBoneTransformIndex());

				// Apply transforms
    // 应用变换
				TargetPoseCmpntSpace.SafeSetCSBoneTransforms(QueuedModifiedBoneTransforms);

				// Turn Component Space poses back into local space.
    // 将组件空间姿势转回到局部空间。
				FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(TargetPoseCmpntSpace), Output.Pose);
			}
		}
	}
}

void FAnimNode_BlendBoneByChannel::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("(Alpha: %.1f%%)"), InternalBlendAlpha * 100);
	DebugData.AddDebugItem(DebugLine);

	A.GatherDebugData(DebugData.BranchFlow(1.f));
	B.GatherDebugData(DebugData.BranchFlow(InternalBlendAlpha));
}

