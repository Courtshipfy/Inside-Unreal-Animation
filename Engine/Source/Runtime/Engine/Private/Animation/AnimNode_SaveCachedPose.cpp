// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimNode_SaveCachedPose.h"
#include "Animation/AnimStats.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimTrace.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_SaveCachedPose)

IMPLEMENT_ANIMGRAPH_MESSAGE(UE::Anim::FCachedPoseSkippedUpdateHandler);

namespace UE::Anim
{

/** Holds references to scoped cached poses to ensure lifetime is correct */
/** 保存对范围内缓存姿势的引用，以确保生命周期正确 */
/** 保存对范围内缓存姿势的引用，以确保生命周期正确 */
/** 保存对范围内缓存姿势的引用，以确保生命周期正确 */
struct FCachedPoseThreadContext : TThreadSingleton<FCachedPoseThreadContext>
	/** 缓存姿势 - 请注意，所有内存都是在当前内存堆栈分配器上分配的 */
{
	/** 缓存姿势 - 请注意，所有内存都是在当前内存堆栈分配器上分配的 */
	/** Cached pose - note that all memory is allocated on the current mem stack allocator */
	/** 缓存姿势 - 请注意，所有内存都是在当前内存堆栈分配器上分配的 */
	struct FCachedPose
	{
		FCompactPose Pose;
		FBlendedCurve Curve;
	/** 缓存的姿势范围 */
		UE::Anim::FStackAttributeContainer Attributes;
	};
	/** 缓存的姿势范围 */

	/** Cached pose scope */
	/** 缓存的姿势范围 */
	struct FCachedPoseScopeInternal
	/** 添加缓存的姿势 */
	{
		// Map of nodes to cached poses
  // 节点到缓存姿势的映射
	/** 添加缓存的姿势 */
		TMap<const FAnimNode_SaveCachedPose*, FCachedPose, TInlineSetAllocator<32>> CachedPoses;
	};

	/** Adds a cached pose */
	/** 添加缓存的姿势 */
	const FCachedPose& AddCachedPose(const FAnimNode_SaveCachedPose* InNode, FCompactPose& InPose, FBlendedCurve& InCurve, UE::Anim::FStackAttributeContainer& InAttributes)
	{
		check(Scopes.Num() > 0);

		FCachedPose& CachedPose = Scopes.Top().CachedPoses.Add(InNode);

	/** 查找缓存的姿势 */
		CachedPose.Pose.MoveBonesFrom(InPose);
		CachedPose.Curve.MoveFrom(InCurve);
		CachedPose.Attributes.MoveFrom(InAttributes);
	/** 查找缓存的姿势 */

		return CachedPose;
	}

	/** Finds a cached pose */
	/** [翻译失败: Finds a cached pose] */
	const FCachedPose* FindCachedPose(const FAnimNode_SaveCachedPose* InNode) const
	/** 当前线程的所有缓存姿势范围 */
	{
		if(Scopes.Num() > 0)
		{
			return Scopes.Top().CachedPoses.Find(InNode);
	/** 当前线程的所有缓存姿势范围 */
		}

		return nullptr;
	}

 // FAnimNode_SaveCachedPose
 // FAnimNode_SaveCachedPose
 // FAnimNode_SaveCachedPose
 // FAnimNode_SaveCachedPose
	/** All cached pose scopes for the current thread */
	/** [翻译失败: All cached pose scopes for the current thread] */
	TArray<FCachedPoseScopeInternal, TInlineAllocator<1>> Scopes;
};

FCachedPoseScope::FCachedPoseScope()
{
// FAnimNode_SaveCachedPose
// FAnimNode_SaveCachedPose
	FCachedPoseThreadContext::Get().Scopes.Push(FCachedPoseThreadContext::FCachedPoseScopeInternal());
}

FCachedPoseScope::~FCachedPoseScope()
{
// FAnimNode_SaveCachedPose
// FAnimNode_SaveCachedPose
	FCachedPoseThreadContext::Get().Scopes.Pop();
}

}

/////////////////////////////////////////////////////
// FAnimNode_SaveCachedPose
// FAnimNode_SaveCachedPose
// FAnimNode_SaveCachedPose
// FAnimNode_SaveCachedPose

FAnimNode_SaveCachedPose::FAnimNode_SaveCachedPose()
	: GlobalWeight(0.0f)
{
}

void FAnimNode_SaveCachedPose::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	// StateMachines cause reinitialization on state changes.
 // 状态机会导致状态更改时重新初始化。
	// we only want to let them through if we're not relevant as to not create a pop.
 // 如果我们与不创建流行音乐无关，我们只想让它们通过。
	if (!InitializationCounter.IsSynchronized_Counter(Context.AnimInstanceProxy->GetInitializationCounter())
		|| (UpdateCounter.HasEverBeenUpdated() && !UpdateCounter.WasSynchronizedCounter(Context.AnimInstanceProxy->GetUpdateCounter())))
	{
		InitializationCounter.SynchronizeWith(Context.AnimInstanceProxy->GetInitializationCounter());

		FAnimNode_Base::Initialize_AnyThread(Context);

		// Initialize the subgraph
  // 初始化子图
		Pose.Initialize(Context);
	}
}

void FAnimNode_SaveCachedPose::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	if (!CachedBonesCounter.IsSynchronized_All(Context.AnimInstanceProxy->GetCachedBonesCounter()))
	{
		CachedBonesCounter.SynchronizeWith(Context.AnimInstanceProxy->GetCachedBonesCounter());

		// Pose will be out of date, so reset the evaluation counter
  // 姿势将会过时，因此请重置评估计数器
		EvaluationCounter.Reset();
		
		// Cache bones in the subgraph
  // 在子图中缓存骨骼
		Pose.CacheBones(Context);
	}
}

void FAnimNode_SaveCachedPose::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	FCachedUpdateContext& CachedUpdate = CachedUpdateContexts.AddDefaulted_GetRef();

	// Make a minimal copy of the shared context for cached updates
 // 为缓存更新创建共享上下文的最小副本
	if (FAnimationUpdateSharedContext* SharedContext = Context.GetSharedContext())
	{
		CachedUpdate.SharedContext = MakeShared<FAnimationUpdateSharedContext>();
		CachedUpdate.SharedContext->CopyForCachedUpdate(*SharedContext);
	}

	// Store this context for the post update
 // 存储此上下文以供更新后使用
	CachedUpdate.Context = Context.WithOtherSharedContext(CachedUpdate.SharedContext.Get());

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Name"), CachePoseName);
}

void FAnimNode_SaveCachedPose::Evaluate_AnyThread(FPoseContext& Output)
{
	using namespace UE::Anim;

	FCachedPoseThreadContext& CachedPoseThreadContext = FCachedPoseThreadContext::Get();
	const FCachedPoseThreadContext::FCachedPose* CachedPose = nullptr;

	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(SaveCachedPose, !IsInGameThread());

	// Note that we check here for IsSynchronized_All to deal with cases like Sequencer:
 // 请注意，我们在此处检查 IsSynchronized_All 以处理 Sequencer 之类的情况：
	// In these cases the counter can stay zeroed between unbound/bound updates and this can cause issues
 // 在这些情况下，计数器可以在未绑定/绑定更新之间保持为零，这可能会导致问题
	// with using out-of-date cached data (stack-allocated from a previous frame).
 // 使用过时的缓存数据（从前一帧堆栈分配）。
	const bool bSynchronized = EvaluationCounter.IsSynchronized_All(Output.AnimInstanceProxy->GetEvaluationCounter());
	if(bSynchronized)
	{
		// Synchronized, so check whether we have a cached curve to use
  // 已同步，因此检查我们是否有缓存的曲线可供使用
		CachedPose = CachedPoseThreadContext.FindCachedPose(this);
	}

	const bool bShouldCachePose = !bSynchronized || CachedPose == nullptr;

	if (bShouldCachePose)
	{
		EvaluationCounter.SynchronizeWith(Output.AnimInstanceProxy->GetEvaluationCounter());

		FPoseContext CachingContext(Output);
		Pose.Evaluate(CachingContext);

		CachedPose = &CachedPoseThreadContext.AddCachedPose(this, CachingContext.Pose, CachingContext.Curve, CachingContext.CustomAttributes);
	}

	// Return the cached result
 // 返回缓存结果
	Output.Pose.CopyBonesFrom(CachedPose->Pose);
	Output.Curve.CopyFrom(CachedPose->Curve);
	Output.CustomAttributes.CopyFrom(CachedPose->Attributes);
}

void FAnimNode_SaveCachedPose::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("%s:"), *CachePoseName.ToString());

	if (FNodeDebugData* SaveCachePoseDebugDataPtr = DebugData.GetCachePoseDebugData(GlobalWeight))
	{
		SaveCachePoseDebugDataPtr->AddDebugItem(DebugLine);
		Pose.GatherDebugData(*SaveCachePoseDebugDataPtr);
	}
}

void FAnimNode_SaveCachedPose::PostGraphUpdate()
{
	GlobalWeight = 0.f;

	// Update GlobalWeight based on highest weight calling us.
 // 根据致电我们的最高权重更新 GlobalWeight。
	const int32 NumContexts = CachedUpdateContexts.Num();
	if (NumContexts > 0)
	{
		GlobalWeight = CachedUpdateContexts[0].Context.GetFinalBlendWeight();
		int32 MaxWeightIdx = 0;
		for (int32 CurrIdx = 1; CurrIdx < NumContexts; ++CurrIdx)
		{
			const float BlendWeight = CachedUpdateContexts[CurrIdx].Context.GetFinalBlendWeight();
			if (BlendWeight > GlobalWeight)
			{
				GlobalWeight = BlendWeight;
				MaxWeightIdx = CurrIdx;
			}
		}

		// Update the max weighted pose node
  // 更新最大加权姿态节点
		{
			TRACE_SCOPED_ANIM_NODE(CachedUpdateContexts[MaxWeightIdx].Context);
			Pose.Update(CachedUpdateContexts[MaxWeightIdx].Context);
		}

		// Update any branches that will be skipped
  // 更新任何将被跳过的分支
		UE::Anim::FMessageStack& MessageStack = CachedUpdateContexts[MaxWeightIdx].SharedContext->MessageStack;
		if(MessageStack.HasMessage<UE::Anim::FCachedPoseSkippedUpdateHandler>())
		{
			// Grab handles to all the execution paths that we are not proceeding with
   // 获取我们未继续处理的所有执行路径的句柄
			TArray<UE::Anim::FMessageStack, TInlineAllocator<4>> SkippedMessageStacks;
			for (int32 CurrIdx = 0; CurrIdx < NumContexts; ++CurrIdx)
			{
				if (CurrIdx != MaxWeightIdx)
				{
					SkippedMessageStacks.Add(MoveTemp(CachedUpdateContexts[CurrIdx].SharedContext->MessageStack));
				}
			}

			// Broadcast skipped update message to interested parties
   // 向感兴趣的各方广播跳过的更新消息
			MessageStack.ForEachMessage<UE::Anim::FCachedPoseSkippedUpdateHandler>([&SkippedMessageStacks](UE::Anim::FCachedPoseSkippedUpdateHandler& InMessage)
			{
				// Fire the event to inform listeners of 'skipped' paths
    // 触发事件以通知侦听器“跳过”的路径
				InMessage.OnUpdatesSkipped(SkippedMessageStacks);

				// We only want the topmost registered node here, so early out
    // 我们只想要这里最顶层的注册节点，所以尽早退出
				return UE::Anim::FMessageStack::EEnumerate::Stop;
			});
		}
	}

	CachedUpdateContexts.Reset();
}

