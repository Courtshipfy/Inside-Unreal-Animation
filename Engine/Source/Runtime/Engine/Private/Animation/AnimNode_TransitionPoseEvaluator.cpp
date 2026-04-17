// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimNode_TransitionPoseEvaluator.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimTrace.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_TransitionPoseEvaluator)

/////////////////////////////////////////////////////
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator
// FAnimNode_TransitionPoseEvaluator

FAnimNode_TransitionPoseEvaluator::FAnimNode_TransitionPoseEvaluator()
	: FramesToCachePose(1)
	, CacheFramesRemaining(1)
	, DataSource(EEvaluatorDataSource::EDS_SourcePose)
	, EvaluatorMode(EEvaluatorMode::EM_Standard)
{
}

void FAnimNode_TransitionPoseEvaluator::SetupCacheFrames()
{
	if (EvaluatorMode == EEvaluatorMode::EM_Freeze)
	{
		// EM_Freeze must evaluate 1 frame to get the initial pose. This cached frame will not call update, only evaluate
  // EM_Freeze 必须评估 1 帧才能获得初始姿势。这个缓存的帧不会调用更新，只会评估
		CacheFramesRemaining = 1;
	}
	else if (EvaluatorMode == EEvaluatorMode::EM_DelayedFreeze)
	{
		// EM_DelayedFreeze can evaluate multiple frames, but must evaluate at least one.
  // EM_DelayedFreeze 可以评估多个帧​​，但必须至少评估一帧。
		CacheFramesRemaining = FMath::Max(FramesToCachePose, 1);
	}
}

void FAnimNode_TransitionPoseEvaluator::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{	
	FAnimNode_Base::Initialize_AnyThread(Context);
	SetupCacheFrames();
}

void FAnimNode_TransitionPoseEvaluator::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) 
{
	if (!CachedBonesCounter.IsSynchronized_All(Context.AnimInstanceProxy->GetCachedBonesCounter()))
	{
		CachedBonesCounter.SynchronizeWith(Context.AnimInstanceProxy->GetCachedBonesCounter());

		// Pose will be out of date, so reset the evaluation counter
  // 姿势将会过时，因此请重置评估计数器
		SetupCacheFrames();
		
		const FBoneContainer& RequiredBone = Context.AnimInstanceProxy->GetRequiredBones();
		CachedPose.SetBoneContainer(&RequiredBone);
		CachedCurve.InitFrom(RequiredBone);
	}
}

void FAnimNode_TransitionPoseEvaluator::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	if (!CachedBonesCounter.IsSynchronized_All(Context.AnimInstanceProxy->GetCachedBonesCounter()))
	{
		CachedBonesCounter.SynchronizeWith(Context.AnimInstanceProxy->GetCachedBonesCounter());

		// Pose will be out of date, so reset the # of cached frames
  // 姿势将过时，因此请重置缓存帧的数量
		SetupCacheFrames();
	}

	// updating is all handled in state machine
 // 更新全部在状态机中处理
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Cached Frames Remaining"), CacheFramesRemaining);
}

void FAnimNode_TransitionPoseEvaluator::Evaluate_AnyThread(FPoseContext& Output)
{	
	// the cached pose is evaluated in the state machine and set via CachePose(). 
 // 缓存的姿势在状态机中评估并通过 CachePose() 设置。
	// This is because we need information about the transition that is not available at this level
 // 这是因为我们需要有关此级别不可用的转换的信息
	Output.Pose.CopyBonesFrom(CachedPose);
	Output.Curve.CopyFrom(CachedCurve);
	Output.CustomAttributes.CopyFrom(CachedAttributes);

	if ((EvaluatorMode != EEvaluatorMode::EM_Standard) && (CacheFramesRemaining > 0))
	{
		CacheFramesRemaining = FMath::Max(CacheFramesRemaining - 1, 0);
	}
}

void FAnimNode_TransitionPoseEvaluator::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	
	DebugLine += FString::Printf(TEXT("(Cached Frames Remaining: %i)"), CacheFramesRemaining);
	DebugData.AddDebugItem(DebugLine);
}

bool FAnimNode_TransitionPoseEvaluator::InputNodeNeedsUpdate(const FAnimationUpdateContext& Context) const
{
	// EM_Standard mode always updates and EM_DelayedFreeze mode only updates if there are cache frames remaining
 // EM_Standard 模式始终更新，EM_DelayedFreeze 模式仅在存在剩余缓存帧时更新
	return (EvaluatorMode == EEvaluatorMode::EM_Standard) || ((EvaluatorMode == EEvaluatorMode::EM_DelayedFreeze) && (CacheFramesRemaining > 0)) || !CachedBonesCounter.IsSynchronized_All(Context.AnimInstanceProxy->GetCachedBonesCounter());
}

bool FAnimNode_TransitionPoseEvaluator::InputNodeNeedsEvaluate() const
{
	return (EvaluatorMode == EEvaluatorMode::EM_Standard) || (CacheFramesRemaining > 0);
}

void FAnimNode_TransitionPoseEvaluator::CachePose(const FPoseContext& PoseToCache)
{
	CachedPose.CopyBonesFrom(PoseToCache.Pose);
	CachedPose.NormalizeRotations();
	CachedCurve.CopyFrom(PoseToCache.Curve);
	CachedAttributes.CopyFrom(PoseToCache.CustomAttributes);
}

