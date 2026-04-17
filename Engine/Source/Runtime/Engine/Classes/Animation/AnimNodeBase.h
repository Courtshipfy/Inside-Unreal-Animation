// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectKey.h"
#include "UObject/Class.h"
#include "Engine/EngineTypes.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimCurveTypes.h"
#include "BonePose.h"
#include "Stats/StatsHierarchical.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"
#include "Animation/AnimNodeMessages.h"
#include "Animation/AnimNodeData.h"
#include "Animation/ExposedValueHandler.h"
#include "AnimNodeFunctionRef.h"

#include "AnimNodeBase.generated.h"

#define DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Method) \
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

#ifndef UE_ANIM_REMOVE_DEPRECATED_ANCESTOR_TRACKER
	#define UE_ANIM_REMOVE_DEPRECATED_ANCESTOR_TRACKER 0
#endif

class IAnimClassInterface;
class UAnimBlueprint;
class UAnimInstance;
struct FAnimInstanceProxy;
struct FAnimNode_Base;
class UProperty;
struct FPropertyAccessLibrary;
struct FAnimNodeConstantData;

/**
 * DEPRECATED - This system is now supplanted by UE::Anim::FMessageStack
 * Utility container for tracking a stack of ancestor nodes by node type during graph traversal
 * This is not an exhaustive list of all visited ancestors. During Update nodes must call
 * FAnimationUpdateContext::TrackAncestor() to appear in the tracker.
 */
struct FAnimNodeTracker
{
	using FKey = FObjectKey;
	using FNodeStack = TArray<FAnimNode_Base*, TInlineAllocator<4>>;
	using FMap = TMap<FKey, FNodeStack, TInlineSetAllocator<4>>;

	FMap Map;

	template<typename NodeType>
	static FKey GetKey()
	{
		return FKey(NodeType::StaticStruct());
	}

	template<typename NodeType>
	FKey Push(NodeType* Node)
	{
		FKey Key(GetKey<NodeType>());
		FNodeStack& Stack = Map.FindOrAdd(Key);
		Stack.Push(Node);
		return Key;
	}

	template<typename NodeType>
	NodeType* Pop()
	{
		FNodeStack* Stack = Map.Find(GetKey<NodeType>());
		return Stack ? static_cast<NodeType*>(Stack->Pop()) : nullptr;
	}

	FAnimNode_Base* Pop(FKey Key)
	{
		FNodeStack* Stack = Map.Find(Key);
		return Stack ? Stack->Pop() : nullptr;
	}

	template<typename NodeType>
	NodeType* Top() const
	{
		const FNodeStack* Stack = Map.Find(GetKey<NodeType>());
		return (Stack && Stack->Num() != 0) ? static_cast<NodeType*>(Stack->Top()) : nullptr;
	}

	void CopyTopsOnly(const FAnimNodeTracker& Source)
	{
		Map.Reset();
		Map.Reserve(Source.Map.Num());
		for (const auto& Iter : Source.Map)
		{
			if (Iter.Value.Num() != 0)
			{
				FNodeStack& Stack = Map.Add(Iter.Key);
				Stack.Push(Iter.Value.Top());
			}
		}
	}
};


/** DEPRECATED - This system is now supplanted by UE::Anim::FMessageStack - Helper RAII object to cleanup a node added to the node tracker */
/** 已弃用 - 该系统现已被 UE::Anim::FMessageStack 取代 - Helper RAII 对象，用于清理添加到节点跟踪器的节点 */
class FScopedAnimNodeTracker
{
public:
	FScopedAnimNodeTracker() = default;

	FScopedAnimNodeTracker(FAnimNodeTracker* InTracker, FAnimNodeTracker::FKey InKey)
		: Tracker(InTracker)
		, TrackedKey(InKey)
	{}

	~FScopedAnimNodeTracker()
	{
		if (Tracker && TrackedKey != FAnimNodeTracker::FKey())
		{
			Tracker->Pop(TrackedKey);
		}
	}

private:
	FAnimNodeTracker* Tracker = nullptr;
	FAnimNodeTracker::FKey TrackedKey;
};


/** Persistent state shared during animation tree update  */
/** 动画树更新期间共享的持久状态  */
struct FAnimationUpdateSharedContext
{
	FAnimationUpdateSharedContext() = default;

	// Non-copyable
	// 不可复制
	FAnimationUpdateSharedContext(FAnimationUpdateSharedContext& ) = delete;
	FAnimationUpdateSharedContext& operator=(const FAnimationUpdateSharedContext&) = delete;

#if !UE_ANIM_REMOVE_DEPRECATED_ANCESTOR_TRACKER
	UE_DEPRECATED(5.0, "Please use the message & tagging system in UE::Anim::FMessageStack")
	FAnimNodeTracker AncestorTracker;
#endif

	// Message stack used for storing scoped messages and tags during execution
	// 消息堆栈用于在执行期间存储作用域消息和标记
	UE::Anim::FMessageStack MessageStack;

	void CopyForCachedUpdate(FAnimationUpdateSharedContext& Source)
	{
#if !UE_ANIM_REMOVE_DEPRECATED_ANCESTOR_TRACKER
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		AncestorTracker.CopyTopsOnly(Source.AncestorTracker);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
		MessageStack.CopyForCachedUpdate(Source.MessageStack);
	}
};

/** Base class for update/evaluate contexts */
/** 更新/评估上下文的基类 */
struct FAnimationBaseContext
{
public:
	FAnimInstanceProxy* AnimInstanceProxy;

	FAnimationUpdateSharedContext* SharedContext;

	ENGINE_API FAnimationBaseContext();

protected:
	// DEPRECATED - Please use constructor that uses an FAnimInstanceProxy*
	// 已弃用 - 请使用使用 FAnimInstanceProxy* 的构造函数
	ENGINE_API FAnimationBaseContext(UAnimInstance* InAnimInstance);

	ENGINE_API FAnimationBaseContext(FAnimInstanceProxy* InAnimInstanceProxy, FAnimationUpdateSharedContext* InSharedContext = nullptr);

public:
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FAnimationBaseContext(FAnimationBaseContext&&) = default;
	FAnimationBaseContext(const FAnimationBaseContext&) = default;
	FAnimationBaseContext& operator=(FAnimationBaseContext&&) = default;
	FAnimationBaseContext& operator=(const FAnimationBaseContext&) = default;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

public:
	// Get the Blueprint IAnimClassInterface associated with this context, if there is one.
	// 获取与此上下文关联的蓝图 IAnimClassInterface（如果有）。
	// Note: This can return NULL, so check the result.
	// 注意：这可能会返回 NULL，因此请检查结果。
	ENGINE_API IAnimClassInterface* GetAnimClass() const;

	// Get the anim instance associated with the current proxy
	// 获取与当前代理关联的动画实例
	ENGINE_API UObject* GetAnimInstanceObject() const;

#if WITH_EDITORONLY_DATA
	// Get the AnimBlueprint associated with this context, if there is one.
	// 获取与此上下文关联的 AnimBlueprint（如果有）。
	// Note: This can return NULL, so check the result.
	// 注意：这可能会返回 NULL，因此请检查结果。
	ENGINE_API UAnimBlueprint* GetAnimBlueprint() const;
#endif //WITH_EDITORONLY_DATA

#if !UE_ANIM_REMOVE_DEPRECATED_ANCESTOR_TRACKER
	template<typename NodeType>
	UE_DEPRECATED(5.0, "Please use the message & tagging system in UE::Anim::FMessageStack")
	FScopedAnimNodeTracker TrackAncestor(NodeType* Node) const {
		if (ensure(SharedContext != nullptr))
		{
			FAnimNodeTracker::FKey Key = SharedContext->AncestorTracker.Push<NodeType>(Node);
			return FScopedAnimNodeTracker(&SharedContext->AncestorTracker, Key);
		}

		return FScopedAnimNodeTracker();
	}
#endif

#if !UE_ANIM_REMOVE_DEPRECATED_ANCESTOR_TRACKER
	template<typename NodeType>
	UE_DEPRECATED(5.0, "Please use the message & tagging system in UE::Anim::FMessageStack")
	NodeType* GetAncestor() const {
		if (ensure(SharedContext != nullptr))
		{
			FAnimNode_Base* Node = SharedContext->AncestorTracker.Top<NodeType>();
			return static_cast<NodeType*>(Node);
		}
		
		return nullptr;
	}
#endif

	// Get the innermost scoped message of the specified type
	// 获取指定类型的最内层作用域消息
	template<typename TGraphMessageType>
	TGraphMessageType* GetMessage() const
	{
		if (ensure(SharedContext != nullptr))
		{
			TGraphMessageType* Message = nullptr;

			SharedContext->MessageStack.TopMessage<TGraphMessageType>([&Message](TGraphMessageType& InMessage)
			{
				Message = &InMessage;
			});

			return Message;
		}
		
		return nullptr;
	}

	// Find the innermost scoped message of the specified type matching the condition of InFunction
	// 查找与 InFunction 的条件匹配的指定类型的最内层作用域消息
	template<typename TGraphMessageType>
	TGraphMessageType* FindMessage(TFunctionRef<bool(TGraphMessageType&)> InFunction) const
	{
		if (ensure(SharedContext != nullptr))
		{
			TGraphMessageType* Message = nullptr;
			SharedContext->MessageStack.ForEachMessage<TGraphMessageType>([&Message, &InFunction](TGraphMessageType& InMessage)
			{
				if (InFunction(InMessage))
				{
					Message = &InMessage;
					return UE::Anim::FMessageStack::EEnumerate::Stop;
				}
				return UE::Anim::FMessageStack::EEnumerate::Continue;
			});
				
			return Message;
		}

		return nullptr;
	}

	// Get the innermost scoped message of the specified type
	// 获取指定类型的最内层作用域消息
	template<typename TGraphMessageType>
	TGraphMessageType& GetMessageChecked() const
	{
		check(SharedContext != nullptr);

		TGraphMessageType* Message = nullptr;

		SharedContext->MessageStack.TopMessage<TGraphMessageType>([&Message](TGraphMessageType& InMessage)
		{
			Message = &InMessage;
		});

		check(Message != nullptr);

		return *Message;
	}
	
	void SetNodeId(int32 InNodeId)
	{ 
		PreviousNodeId = CurrentNodeId;
		CurrentNodeId = InNodeId;
	}

	void SetNodeIds(const FAnimationBaseContext& InContext)
	{ 
		CurrentNodeId = InContext.CurrentNodeId;
		PreviousNodeId = InContext.PreviousNodeId;
	}

	// Get the current node Id, set when we recurse into graph traversal functions from pose links
	// 获取当前节点Id，当我们从位姿链接递归到图遍历函数时设置
	int32 GetCurrentNodeId() const { return CurrentNodeId; }

	// Get the previous node Id, set when we recurse into graph traversal functions from pose links
	// 获取前一个节点Id，当我们从位姿链接递归到图遍历函数时设置
	int32 GetPreviousNodeId() const { return PreviousNodeId; }

	// Get whether the graph branch of this context is active (i.e. NOT blending out). 
	// 获取此上下文的图形分支是否处于活动状态（即不混合）。
	bool IsActive() const { return bIsActive; }

protected:
	
	// Whether this context belongs to graph branch (i.e. NOT blending out).
	// 此上下文是否属于图分支（即不混合）。
	bool bIsActive = true;

	// The current node ID, set when we recurse into graph traversal functions from pose links
	// 当前节点 ID，当我们从位姿链接递归到图遍历函数时设置
	int32 CurrentNodeId;

	// The previous node ID, set when we recurse into graph traversal functions from pose links
	// 前一个节点 ID，当我们从位姿链接递归到图遍历函数时设置
	int32 PreviousNodeId;

protected:

	/** Interface for node contexts to register log messages with the proxy */
	/** 节点上下文向代理注册日志消息的接口 */
	ENGINE_API void LogMessageInternal(FName InLogType, const TSharedRef<FTokenizedMessage>& InMessage) const;
};


/** Initialization context passed around during animation tree initialization */
/** 动画树初始化期间传递的初始化上下文 */
struct FAnimationInitializeContext : public FAnimationBaseContext
{
public:
	FAnimationInitializeContext(FAnimInstanceProxy* InAnimInstanceProxy, FAnimationUpdateSharedContext* InSharedContext = nullptr)
		: FAnimationBaseContext(InAnimInstanceProxy, InSharedContext)
	{
	}
};

/**
 * Context passed around when RequiredBones array changed and cached bones indices have to be refreshed.
 * (RequiredBones array changed because of an LOD switch for example)
 */
struct FAnimationCacheBonesContext : public FAnimationBaseContext
{
public:
	FAnimationCacheBonesContext(FAnimInstanceProxy* InAnimInstanceProxy)
		: FAnimationBaseContext(InAnimInstanceProxy)
	{
	}

	FAnimationCacheBonesContext WithNodeId(int32 InNodeId) const
	{ 
		FAnimationCacheBonesContext Result(*this);
		Result.SetNodeId(InNodeId);
		return Result; 
	}
};

/** Update context passed around during animation tree update */
/** 更新动画树更新期间传递的上下文 */
struct FAnimationUpdateContext : public FAnimationBaseContext
{
private:
	float CurrentWeight;
	float RootMotionWeightModifier;

	float DeltaTime;

public:
	FAnimationUpdateContext(FAnimInstanceProxy* InAnimInstanceProxy = nullptr)
		: FAnimationBaseContext(InAnimInstanceProxy)
		, CurrentWeight(1.0f)
		, RootMotionWeightModifier(1.0f)
		, DeltaTime(0.0f)
	{
	}

	FAnimationUpdateContext(FAnimInstanceProxy* InAnimInstanceProxy, float InDeltaTime, FAnimationUpdateSharedContext* InSharedContext = nullptr)
		: FAnimationBaseContext(InAnimInstanceProxy, InSharedContext)
		, CurrentWeight(1.0f)
		, RootMotionWeightModifier(1.0f)
		, DeltaTime(InDeltaTime)
	{
	}


	FAnimationUpdateContext(const FAnimationUpdateContext& Copy, FAnimInstanceProxy* InAnimInstanceProxy)
		: FAnimationBaseContext(InAnimInstanceProxy, Copy.SharedContext)
		, CurrentWeight(Copy.CurrentWeight)
		, RootMotionWeightModifier(Copy.RootMotionWeightModifier)
		, DeltaTime(Copy.DeltaTime)
	{
		CurrentNodeId = Copy.CurrentNodeId;
		PreviousNodeId = Copy.PreviousNodeId;
	}

public:
	FAnimationUpdateContext WithOtherProxy(FAnimInstanceProxy* InAnimInstanceProxy) const
	{
		return FAnimationUpdateContext(*this, InAnimInstanceProxy);
	}

	FAnimationUpdateContext WithOtherSharedContext(FAnimationUpdateSharedContext* InSharedContext) const
	{
		FAnimationUpdateContext Result(*this);
		Result.SharedContext = InSharedContext;

		// This is currently only used in the case of cached poses, where we dont want to preserve the previous node, so clear it here
		// 目前仅在缓存姿势的情况下使用，我们不想保留前一个节点，所以在这里清除它
	//	Result.PreviousNodeId = INDEX_NONE;
	//	结果.PreviousNodeId = INDEX_NONE;

		return Result;
	}

	FAnimationUpdateContext AsInactive() const
	{
		FAnimationUpdateContext Result(*this);
		Result.bIsActive = false;

		return Result;
	}

	FAnimationUpdateContext FractionalWeight(float WeightMultiplier) const
	{
		FAnimationUpdateContext Result(*this);
		Result.CurrentWeight = CurrentWeight * WeightMultiplier;

		return Result;
	}

	FAnimationUpdateContext FractionalWeightAndRootMotion(float WeightMultiplier, float RootMotionMultiplier) const
	{
		FAnimationUpdateContext Result(*this);
		Result.CurrentWeight = CurrentWeight * WeightMultiplier;
		Result.RootMotionWeightModifier = RootMotionWeightModifier * RootMotionMultiplier;

		return Result;
	}

	FAnimationUpdateContext FractionalWeightAndTime(float WeightMultiplier, float TimeMultiplier) const
	{
		FAnimationUpdateContext Result(*this);
		Result.DeltaTime = DeltaTime * TimeMultiplier;
		Result.CurrentWeight = CurrentWeight * WeightMultiplier;
		return Result;
	}

	FAnimationUpdateContext FractionalWeightTimeAndRootMotion(float WeightMultiplier, float TimeMultiplier, float RootMotionMultiplier) const
	{
		FAnimationUpdateContext Result(*this);
		Result.DeltaTime = DeltaTime * TimeMultiplier;
		Result.CurrentWeight = CurrentWeight * WeightMultiplier;
		Result.RootMotionWeightModifier = RootMotionWeightModifier * RootMotionMultiplier;

		return Result;
	}

	FAnimationUpdateContext WithNodeId(int32 InNodeId) const
	{ 
		FAnimationUpdateContext Result(*this);
		Result.SetNodeId(InNodeId);
		return Result; 
	}

	// Returns persistent state that is tracked through animation tree update
	// 返回通过动画树更新跟踪的持久状态
	FAnimationUpdateSharedContext* GetSharedContext() const
	{
		return SharedContext;
	}

	// Returns the final blend weight contribution for this stage
	// 返回此阶段的最终混合权重贡献
	float GetFinalBlendWeight() const { return CurrentWeight; }

	// Returns the weight modifier for root motion (as root motion weight wont always match blend weight)
	// 返回根运动的权重修改器（因为根运动权重并不总是与混合权重匹配）
	float GetRootMotionWeightModifier() const { return RootMotionWeightModifier; }

	// Returns the delta time for this update, in seconds
	// 返回此更新的增量时间（以秒为单位）
	float GetDeltaTime() const { return DeltaTime; }

	// Log update message
	// 记录更新消息
	void LogMessage(const TSharedRef<FTokenizedMessage>& InMessage) const { LogMessageInternal("Update", InMessage); }
	void LogMessage(EMessageSeverity::Type InSeverity, FText InMessage) const { LogMessage(FTokenizedMessage::Create(InSeverity, InMessage)); }
};


/** Evaluation context passed around during animation tree evaluation */
/** 动画树评估期间传递的评估上下文 */
struct FPoseContext : public FAnimationBaseContext
{
public:
	/* These Pose/Curve/Attributes are allocated using MemStack. You should not use it outside of stack. */
	/* 这些姿势/曲线/属性是使用 MemStack 分配的。您不应该在堆栈之外使用它。 */
	FCompactPose	Pose;
	FBlendedCurve	Curve;
	UE::Anim::FStackAttributeContainer CustomAttributes;

public:
	friend class FScopedExpectsAdditiveOverride;
	
	// This constructor allocates a new uninitialized pose for the specified anim instance
	// 此构造函数为指定的动画实例分配一个新的未初始化姿势
	FPoseContext(FAnimInstanceProxy* InAnimInstanceProxy, bool bInExpectsAdditivePose = false)
		: FAnimationBaseContext(InAnimInstanceProxy)
		, bExpectsAdditivePose(bInExpectsAdditivePose)
	{
		InitializeImpl(InAnimInstanceProxy);
	}

	// This constructor allocates a new uninitialized pose, copying non-pose state from the source context
	// 此构造函数分配一个新的未初始化姿势，从源上下文复制非姿势状态
	FPoseContext(const FPoseContext& SourceContext, bool bInOverrideExpectsAdditivePose = false)
		: FAnimationBaseContext(SourceContext.AnimInstanceProxy)
		, bExpectsAdditivePose(SourceContext.bExpectsAdditivePose || bInOverrideExpectsAdditivePose)
	{
		InitializeImpl(SourceContext.AnimInstanceProxy);

		CurrentNodeId = SourceContext.CurrentNodeId;
		PreviousNodeId = SourceContext.PreviousNodeId;
	}

	// This constructor allocates a new uninitialized pose, using the provided BoneContainer (when there is no AnimInstanceProxy available)
	// 此构造函数使用提供的 BoneContainer 分配一个新的未初始化姿势（当没有可用的 AnimInstanceProxy 时）
	FPoseContext(const FBoneContainer& InRequiredBones, bool bInExpectsAdditivePose = false)
		: FAnimationBaseContext()
		, bExpectsAdditivePose(bInExpectsAdditivePose)
	{
		checkSlow(InRequiredBones.IsValid());
		Pose.SetBoneContainer(&InRequiredBones);
		Curve.InitFrom(InRequiredBones);
	}

	UE_DEPRECATED(5.2, "This function will be made private. It should never be called externally, use the constructor instead.")
	void Initialize(FAnimInstanceProxy* InAnimInstanceProxy) { InitializeImpl(InAnimInstanceProxy); }

	// Log evaluation message
	// 记录评估消息
	void LogMessage(const TSharedRef<FTokenizedMessage>& InMessage) const { LogMessageInternal("Evaluate", InMessage); }
	void LogMessage(EMessageSeverity::Type InSeverity, FText InMessage) const { LogMessage(FTokenizedMessage::Create(InSeverity, InMessage)); }

	void ResetToRefPose()
	{
		if (bExpectsAdditivePose)
		{
			Pose.ResetToAdditiveIdentity();
		}
		else
		{
			Pose.ResetToRefPose();
		}
	}

	void ResetToAdditiveIdentity()
	{
		Pose.ResetToAdditiveIdentity();
	}

	bool ContainsNaN() const
	{
		return Pose.ContainsNaN();
	}

	bool IsNormalized() const
	{
		return Pose.IsNormalized();
	}

	FPoseContext& operator=(const FPoseContext& Other)
	{
		if (AnimInstanceProxy != Other.AnimInstanceProxy)
		{
			InitializeImpl(AnimInstanceProxy);
		}

		Pose = Other.Pose;
		Curve = Other.Curve;
		CustomAttributes = Other.CustomAttributes;
		bExpectsAdditivePose = Other.bExpectsAdditivePose;
		return *this;
	}

	// Is this pose expected to be additive
	// 这个姿势预计会是累加的吗
	bool ExpectsAdditivePose() const { return bExpectsAdditivePose; }

private:
	ENGINE_API void InitializeImpl(FAnimInstanceProxy* InAnimInstanceProxy);

	// Is this pose expected to be an additive pose
	// 这个姿势预计是一个附加姿势吗
	bool bExpectsAdditivePose;
};

// Helper for modifying and resetting ExpectsAdditivePose on a FPoseContext
// 用于修改和重置 FPoseContext 上的 ExpectsAdditivePose 的帮助程序
class FScopedExpectsAdditiveOverride
{
public:
	FScopedExpectsAdditiveOverride(FPoseContext& InContext, bool bInExpectsAdditive)
		: Context(InContext)
	{
		bPreviousValue = Context.ExpectsAdditivePose();
		Context.bExpectsAdditivePose = bInExpectsAdditive;
	}
	
	~FScopedExpectsAdditiveOverride()
	{
		Context.bExpectsAdditivePose = bPreviousValue;
	}
private:
	FPoseContext& Context;
	bool bPreviousValue;
};
	


/** Evaluation context passed around during animation tree evaluation */
/** 动画树评估期间传递的评估上下文 */
struct FComponentSpacePoseContext : public FAnimationBaseContext
{
public:
	FCSPose<FCompactPose>	Pose;
	FBlendedCurve			Curve;
	UE::Anim::FStackAttributeContainer CustomAttributes;

public:
	// This constructor allocates a new uninitialized pose for the specified anim instance
	// 此构造函数为指定的动画实例分配一个新的未初始化姿势
	FComponentSpacePoseContext(FAnimInstanceProxy* InAnimInstanceProxy)
		: FAnimationBaseContext(InAnimInstanceProxy)
	{
		// No need to initialize, done through FA2CSPose::AllocateLocalPoses
		// 无需初始化，通过FA2CSPose::AllocateLocalPoses完成
	}

	// This constructor allocates a new uninitialized pose, copying non-pose state from the source context
	// 此构造函数分配一个新的未初始化姿势，从源上下文复制非姿势状态
	FComponentSpacePoseContext(const FComponentSpacePoseContext& SourceContext)
		: FAnimationBaseContext(SourceContext.AnimInstanceProxy)
	{
		// No need to initialize, done through FA2CSPose::AllocateLocalPoses
		// 无需初始化，通过FA2CSPose::AllocateLocalPoses完成

		CurrentNodeId = SourceContext.CurrentNodeId;
		PreviousNodeId = SourceContext.PreviousNodeId;
	}

	// Note: this copy assignment operator copies the whole object but the copy constructor only copies part of the object.
	// 注意：此复制赋值运算符复制整个对象，但复制构造函数仅复制对象的一部分。
	FComponentSpacePoseContext& operator=(const FComponentSpacePoseContext&) = default;

	ENGINE_API void ResetToRefPose();

	ENGINE_API bool ContainsNaN() const;
	ENGINE_API bool IsNormalized() const;
};

/**
 * We pass array items by reference, which is scary as TArray can move items around in memory.
 * So we make sure to allocate enough here so it doesn't happen and crash on us.
 */
#define ANIM_NODE_DEBUG_MAX_CHAIN 50
#define ANIM_NODE_DEBUG_MAX_CHILDREN 12
#define ANIM_NODE_DEBUG_MAX_CACHEPOSE 20

struct FNodeDebugData
{
private:
	struct DebugItem
	{
		DebugItem(FString Data, bool bInPoseSource) : DebugData(Data), bPoseSource(bInPoseSource) {}

		/** This node item's debug text to display. */
		/** 要显示的此节点项的调试文本。 */
		FString DebugData;

		/** Whether we are supplying a pose instead of modifying one (e.g. an playing animation). */
		/** 我们是否提供一个姿势而不是修改一个姿势（例如播放动画）。 */
		bool bPoseSource;

		/** Nodes that we are connected to. */
		/** 我们连接到的节点。 */
		TArray<FNodeDebugData> ChildNodeChain;
	};

	/** This nodes final contribution weight (based on its own weight and the weight of its parents). */
	/** 该节点最终贡献权重（基于其自身权重及其父节点权重）。 */
	float AbsoluteWeight;

	/** Nodes that we are dependent on. */
	/** 我们所依赖的节点。 */
	TArray<DebugItem> NodeChain;

	/** Additional info provided, used in GetNodeName. States machines can provide the state names for the Root Nodes to use for example. */
	/** 提供了在 GetNodeName 中使用的附加信息。例如，状态机可以提供状态名称供根节点使用。 */
	FString NodeDescription;

	/** Pointer to RootNode */
	/** 指向根节点的指针 */
	FNodeDebugData* RootNodePtr;

	/** SaveCachePose Nodes */
	/** SaveCachePose 节点 */
	TArray<FNodeDebugData> SaveCachePoseNodes;

public:
	struct FFlattenedDebugData
	{
		FFlattenedDebugData(FString Line, float AbsWeight, int32 InIndent, int32 InChainID, bool bInPoseSource) : DebugLine(Line), AbsoluteWeight(AbsWeight), Indent(InIndent), ChainID(InChainID), bPoseSource(bInPoseSource){}
		FString DebugLine;
		float AbsoluteWeight;
		int32 Indent;
		int32 ChainID;
		bool bPoseSource;

		bool IsOnActiveBranch() { return FAnimWeight::IsRelevant(AbsoluteWeight); }
	};

	FNodeDebugData(const class UAnimInstance* InAnimInstance) 
		: AbsoluteWeight(1.f), RootNodePtr(this), AnimInstance(InAnimInstance)
	{
		SaveCachePoseNodes.Reserve(ANIM_NODE_DEBUG_MAX_CACHEPOSE);
	}
	
	FNodeDebugData(const class UAnimInstance* InAnimInstance, const float AbsWeight, FString InNodeDescription, FNodeDebugData* InRootNodePtr)
		: AbsoluteWeight(AbsWeight)
		, NodeDescription(InNodeDescription)
		, RootNodePtr(InRootNodePtr)
		, AnimInstance(InAnimInstance) 
	{}

	ENGINE_API void AddDebugItem(FString DebugData, bool bPoseSource = false);
	ENGINE_API FNodeDebugData& BranchFlow(float BranchWeight, FString InNodeDescription = FString());
	ENGINE_API FNodeDebugData* GetCachePoseDebugData(float GlobalWeight);

	template<class Type>
	FString GetNodeName(Type* Node)
	{
		FString FinalString = FString::Printf(TEXT("%s<W:%.1f%%> %s"), *Node->StaticStruct()->GetName(), AbsoluteWeight*100.f, *NodeDescription);
		NodeDescription.Empty();
		return FinalString;
	}

	ENGINE_API void GetFlattenedDebugData(TArray<FFlattenedDebugData>& FlattenedDebugData, int32 Indent, int32& ChainID);

	TArray<FFlattenedDebugData> GetFlattenedDebugData()
	{
		TArray<FFlattenedDebugData> Data;
		int32 ChainID = 0;
		GetFlattenedDebugData(Data, 0, ChainID);
		return Data;
	}

	// Anim instance that we are generating debug data for
	// 我们正在为其生成调试数据的动画实例
	const UAnimInstance* AnimInstance;
};

/** The display mode of editable values on an animation node. */
/** 动画节点上可编辑值的显示模式。 */
UENUM()
namespace EPinHidingMode
{
	enum Type : int
	{
		/** Never show this property as a pin, it is only editable in the details panel (default for everything but FPoseLink properties). */
		/** 切勿将此属性显示为引脚，它只能在详细信息面板中编辑（除 FPoseLink 属性外的所有内容均默认）。 */
		NeverAsPin,

		/** Hide this property by default, but allow the user to expose it as a pin via the details panel. */
		/** 默认情况下隐藏此属性，但允许用户通过详细信息面板将其公开为引脚。 */
		PinHiddenByDefault,

		/** Show this property as a pin by default, but allow the user to hide it via the details panel. */
		/** 默认情况下将此属性显示为图钉，但允许用户通过详细信息面板隐藏它。 */
		PinShownByDefault,

		/** Always show this property as a pin; it never makes sense to edit it in the details panel (default for FPoseLink properties). */
		/** 始终将此属性显示为图钉；在详细信息面板中编辑它没有任何意义（FPoseLink 属性的默认值）。 */
		AlwaysAsPin
	};
}

#define ENABLE_ANIMGRAPH_TRAVERSAL_DEBUG 0

/** A pose link to another node */
/** 到另一个节点的姿势链接 */
USTRUCT(BlueprintInternalUseOnly)
struct FPoseLinkBase
{
	GENERATED_USTRUCT_BODY()

protected:
	/** The non serialized node pointer. */
	/** 非序列化节点指针。 */
	FAnimNode_Base* LinkedNode;

public:
	/** Serialized link ID, used to build the non-serialized pointer map. */
	/** 序列化链接ID，用于构建非序列化指针映射。 */
	UPROPERTY(meta=(BlueprintCompilerGeneratedDefaults))
	int32 LinkID;

#if WITH_EDITORONLY_DATA
	/** The source link ID, used for debug visualization. */
	/** 源链接 ID，用于调试可视化。 */
	UPROPERTY(meta=(BlueprintCompilerGeneratedDefaults))
	int32 SourceLinkID;
#endif

#if ENABLE_ANIMGRAPH_TRAVERSAL_DEBUG
	FGraphTraversalCounter InitializationCounter;
	FGraphTraversalCounter CachedBonesCounter;
	FGraphTraversalCounter UpdateCounter;
	FGraphTraversalCounter EvaluationCounter;
#endif

protected:
#if DO_CHECK
	/** Flag to prevent reentry when dealing with circular trees. */
	/** 处理圆形树时用于防止重入的标记。 */
	bool bProcessed;
#endif

public:
	FPoseLinkBase()
		: LinkedNode(nullptr)
		, LinkID(INDEX_NONE)
#if WITH_EDITORONLY_DATA
		, SourceLinkID(INDEX_NONE)
#endif
#if DO_CHECK
		, bProcessed(false)
#endif
	{
	}

	// Interface
	// 界面

	ENGINE_API void Initialize(const FAnimationInitializeContext& Context);
	ENGINE_API void CacheBones(const FAnimationCacheBonesContext& Context);
	ENGINE_API void Update(const FAnimationUpdateContext& Context);
	ENGINE_API void GatherDebugData(FNodeDebugData& DebugData);

	/** Try to re-establish the linked node pointer. */
	/** 尝试重新建立链接节点指针。 */
	ENGINE_API void AttemptRelink(const FAnimationBaseContext& Context);

	/** This only used by custom handlers, and it is advanced feature. */
	/** 这仅由自定义处理程序使用，并且是高级功能。 */
	ENGINE_API void SetLinkNode(FAnimNode_Base* NewLinkNode);

	/** This only used when dynamic linking other graphs to this one. */
	/** 仅当将其他图动态链接到此图时才使用此选项。 */
	ENGINE_API void SetDynamicLinkNode(struct FPoseLinkBase* InPoseLink);

	/** This only used by custom handlers, and it is advanced feature. */
	/** 这仅由自定义处理程序使用，并且是高级功能。 */
	ENGINE_API FAnimNode_Base* GetLinkNode();
};

#define ENABLE_ANIMNODE_POSE_DEBUG 0

/** A local-space pose link to another node */
/** 局部空间姿态链接到另一个节点 */
USTRUCT(BlueprintInternalUseOnly)
struct FPoseLink : public FPoseLinkBase
{
	GENERATED_USTRUCT_BODY()

public:
	// Interface
	// 界面
	ENGINE_API void Evaluate(FPoseContext& Output);

#if ENABLE_ANIMNODE_POSE_DEBUG
private:
	// forwarded pose data from the wired node which current node's skeletal control is not applied yet
	// 从当前节点的骨骼控制尚未应用的有线节点转发的姿势数据
	FCompactHeapPose CurrentPose;
#endif //#if ENABLE_ANIMNODE_POSE_DEBUG
};

/** A component-space pose link to another node */
/** 组件空间姿势链接到另一个节点 */
USTRUCT(BlueprintInternalUseOnly)
struct FComponentSpacePoseLink : public FPoseLinkBase
{
	GENERATED_USTRUCT_BODY()

public:
	// Interface
	// 界面
	ENGINE_API void EvaluateComponentSpace(FComponentSpacePoseContext& Output);
};

/**
 * This is the base of all runtime animation nodes
 *
 * To create a new animation node:
 *   Create a struct derived from FAnimNode_Base - this is your runtime node
 *   Create a class derived from UAnimGraphNode_Base, containing an instance of your runtime node as a member - this is your visual/editor-only node
 */
USTRUCT()
struct FAnimNode_Base
{
	GENERATED_BODY()

	/** 
	 * Called when the node first runs. If the node is inside a state machine or cached pose branch then this can be called multiple times. 
	 * This can be called on any thread.
	 * @param	Context		Context structure providing access to relevant data
	 */
	ENGINE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context);

	/** 
	 * Called to cache any bones that this node needs to track (e.g. in a FBoneReference). 
	 * This is usually called at startup when LOD switches occur.
	 * This can be called on any thread.
	 * @param	Context		Context structure providing access to relevant data
	 */
	ENGINE_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context);

	/** 
	 * Called to update the state of the graph relative to this node.
	 * Generally this should configure any weights (etc.) that could affect the poses that
	 * will need to be evaluated. This function is what usually executes EvaluateGraphExposedInputs.
	 * This can be called on any thread.
	 * @param	Context		Context structure providing access to relevant data
	 */
	ENGINE_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context);

	/** 
	 * Called to evaluate local-space bones transforms according to the weights set up in Update().
	 * You should implement either Evaluate or EvaluateComponentSpace, but not both of these.
	 * This can be called on any thread.
	 * @param	Output		Output structure to write pose or curve data to. Also provides access to relevant data as a context.
	 */
	ENGINE_API virtual void Evaluate_AnyThread(FPoseContext& Output);

	/** 
	 * Called to evaluate component-space bone transforms according to the weights set up in Update().
	 * You should implement either Evaluate or EvaluateComponentSpace, but not both of these.
	 * This can be called on any thread.
	 * @param	Output		Output structure to write pose or curve data to. Also provides access to relevant data as a context.
	 */	
	ENGINE_API virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output);

	/**
	 * Called to gather on-screen debug data. 
	 * This is called on the game thread.
	 * @param	DebugData	Debug data structure used to output any relevant data
	 */
	virtual void GatherDebugData(FNodeDebugData& DebugData)
	{ 
		DebugData.AddDebugItem(FString::Printf(TEXT("Non Overriden GatherDebugData! (%s)"), *DebugData.GetNodeName(this)));
	}

	/**
	 * Whether this node can run its Update() call on a worker thread.
	 * This is called on the game thread.
	 * If any node in a graph returns false from this function, then ALL nodes will update on the game thread.
	 */
	virtual bool CanUpdateInWorkerThread() const { return true; }

	/**
	 * Override this to indicate that PreUpdate() should be called on the game thread (usually to 
	 * gather non-thread safe data) before Update() is called.
	 * Note that this is called at load on the UAnimInstance CDO to avoid needing to call this at runtime.
	 * This is called on the game thread.
	 */
	virtual bool HasPreUpdate() const { return false; }

	/** Override this to perform game-thread work prior to non-game thread Update() being called */
	/** 覆盖此设置以在调用非游戏线程 Update() 之前执行游戏线程工作 */
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) {}

	/**
	 * For nodes that implement some kind of simulation, return true here so ResetDynamics() gets called
	 * when things like teleports, time skips etc. occur that might require special handling.
	 * Note that this is called at load on the UAnimInstance CDO to avoid needing to call this at runtime.
	 * This is called on the game thread.
	 */
	virtual bool NeedsDynamicReset() const { return false; }

	/** Called to help dynamics-based updates to recover correctly from large movements/teleports */
	/** 被要求帮助基于动态的更新从大的移动/传送中正确恢复 */
	ENGINE_API virtual void ResetDynamics(ETeleportType InTeleportType);

	/** Called after compilation */
	/** 编译后调用 */
	virtual void PostCompile(const class USkeleton* InSkeleton) {}

	/** 
	 * For nodes that need some kind of initialization that is not dependent on node relevancy 
	 * (i.e. it is insufficient or inefficient to use Initialize_AnyThread), return true here.
	 * Note that this is called at load on the UAnimInstance CDO to avoid needing to call this at runtime.
	 */
	virtual bool NeedsOnInitializeAnimInstance() const { return false; }

	virtual ~FAnimNode_Base() {}

	/** Deprecated functions */
	/** 已弃用的函数 */
	UE_DEPRECATED(4.20, "Please use ResetDynamics with an ETeleportPhysics flag instead")
	virtual void ResetDynamics() {}
	UE_DEPRECATED(5.0, "Please use IGraphMessage instead")
	virtual bool WantsSkippedUpdates() const { return false; }
	UE_DEPRECATED(5.0, "Please use IGraphMessage instead")
	virtual void OnUpdatesSkipped(TArrayView<const FAnimationUpdateContext*> SkippedUpdateContexts) {}
	UE_DEPRECATED(5.0, "Please use the OverrideAssets API on UAnimGraphNode_Base to opt-in to child anim BP override functionality, or per-node specific asset override calls.")
	virtual void OverrideAsset(class UAnimationAsset* NewAsset) {}
	
	// The default handler for graph-exposed inputs:
	// 图形公开输入的默认处理程序：
	ENGINE_API const FExposedValueHandler& GetEvaluateGraphExposedInputs() const;

	// Initialization function for the default handler for graph-exposed inputs, used only by instancing code:
	// 图形公开输入的默认处理程序的初始化函数，仅由实例代码使用：
	UE_DEPRECATED(5.0, "Exposed value handlers are now accessed via FAnimNodeConstantData")
	void SetExposedValueHandler(const FExposedValueHandler* Handler) { }

	// Get this node's index. The node index provides a unique key into its location within the class data
	// 获取该节点的索引。节点索引提供了一个唯一的键来了解其在类数据中的位置
	int32 GetNodeIndex() const
	{
		check(NodeData);
		return NodeData->GetNodeIndex();
	}

	// Get the anim class that this node is hosted within
	// 获取该节点所在的动画类
	const IAnimClassInterface* GetAnimClassInterface() const
	{
		check(NodeData);
		return &NodeData->GetAnimClassInterface();
	}
	
protected:
	// Get anim node constant/folded data of the specified type given the identifier. Do not use directly - use GET_ANIM_NODE_DATA
	// 给定标识符，获取指定类型的动画节点常量/折叠数据。不要直接使用 - 使用 GET_ANIM_NODE_DATA
	template<typename DataType>
	const DataType& GetData(UE::Anim::FNodeDataId InId, const UObject* InObject = nullptr) const
	{
#if WITH_EDITORONLY_DATA
		if(NodeData)
		{
			return *static_cast<const DataType*>(NodeData->GetData(InId, this, InObject));
		}
		else
		{
			return *InId.GetProperty()->ContainerPtrToValuePtr<const DataType>(this);
		}
#else
		check(NodeData);
		return *static_cast<const DataType*>(NodeData->GetData(InId, this, InObject));
#endif
	}

	// Get anim node constant/folded data of the specified type given the identifier. Do not use directly - use GET_MUTABLE_ANIM_NODE_DATA
	// 给定标识符，获取指定类型的动画节点常量/折叠数据。不要直接使用 - 使用 GET_MUTABLE_ANIM_NODE_DATA
	// Note: will assert if data is not held on the instance/dynamic. Use GetInstanceDataPtr/GET_INSTANCE_ANIM_NODE_DATA_PTR if the value
	// 注意：如果数据未保存在实例/动态上，将断言。如果该值使用 GetInstanceDataPtr/GET_INSTANCE_ANIM_NODE_DATA_PTR
	// might not be mutable, which will return null.
	// 可能不可变，这将返回 null。
#if WITH_EDITORONLY_DATA
	template<typename DataType>
	DataType& GetMutableData(UE::Anim::FNodeDataId InId, UObject* InObject = nullptr)
	{
		if(NodeData)
		{
			return *static_cast<DataType*>(NodeData->GetMutableData(InId, this, InObject));
		}
		else
		{
			return *InId.GetProperty()->ContainerPtrToValuePtr<DataType>(this);
		}
	}
#endif

	// Get anim node mutable data of the specified type given the identifier. Do not use directly - use GET_INSTANCE_ANIM_NODE_DATA_PTR
	// 获取给定标识符的指定类型的动画节点可变数据。不要直接使用 - 使用 GET_INSTANCE_ANIM_NODE_DATA_PTR
	// @return nullptr if the data is not mutable/dynamic
	// 如果数据不是可变/动态的，则@return nullptr
	template<typename DataType>
	DataType* GetInstanceDataPtr(UE::Anim::FNodeDataId InId, UObject* InObject = nullptr)
	{
#if WITH_EDITORONLY_DATA	
		if(NodeData)
		{
			return static_cast<DataType*>(NodeData->GetInstanceData(InId, this, InObject));
		}
		else
		{
			return InId.GetProperty()->ContainerPtrToValuePtr<DataType>(this);
		}
#else
		check(NodeData);
		return static_cast<DataType*>(NodeData->GetInstanceData(InId, this, InObject));
#endif
	}
	
protected:
	/** return true if enabled, otherwise, return false. This is utility function that can be used per node level */
	/** 如果启用则返回 true，否则返回 false。这是可以在每个节点级别使用的实用函数 */
	ENGINE_API bool IsLODEnabled(FAnimInstanceProxy* AnimInstanceProxy);

	/** Get the LOD level at which this node is enabled. Node is enabled if the current LOD is less than or equal to this threshold. */
	/** 获取启用此节点的 LOD 级别。如果当前 LOD 小于或等于此阈值，则启用节点。 */
	virtual int32 GetLODThreshold() const { return INDEX_NONE; }

	/** Called once, from game thread as the parent anim instance is created */
	/** 在创建父动画实例时从游戏线程调用一次 */
	ENGINE_API virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance);

	friend struct FAnimInstanceProxy;

private:
	// Access functions
	// 访问功能
	ENGINE_API const FAnimNodeFunctionRef& GetInitialUpdateFunction() const;
	ENGINE_API const FAnimNodeFunctionRef& GetBecomeRelevantFunction() const;
	ENGINE_API const FAnimNodeFunctionRef& GetUpdateFunction() const;
	
private:
	friend class IAnimClassInterface;
	friend class UAnimBlueprintGeneratedClass;
	friend struct UE::Anim::FNodeDataId;
	friend struct UE::Anim::FNodeFunctionCaller;
	friend class UAnimGraphNode_Base;
	friend struct FPoseLinkBase;

	// Set the cached ptr to the constant/folded data for this node
	// 将缓存的 ptr 设置为该节点的常量/折叠数据
	void SetNodeData(const FAnimNodeData& InNodeData) { NodeData = &InNodeData; }

	// Reference to the constant/folded data for this node
	// 引用该节点的常量/折叠数据
	const FAnimNodeData* NodeData = nullptr;

#if WITH_EDITORONLY_DATA
	// Function called on initial update
	// 初始更新时调用的函数
	UPROPERTY(meta=(FoldProperty))
	FAnimNodeFunctionRef InitialUpdateFunction;

	// Function called on become relevant
	// 调用的函数变得相关
	UPROPERTY(meta=(FoldProperty))
	FAnimNodeFunctionRef BecomeRelevantFunction;

	// Function called on update
	// 更新时调用的函数
	UPROPERTY(meta=(FoldProperty))
	FAnimNodeFunctionRef UpdateFunction;
#endif
};

#if WITH_EDITORONLY_DATA
#define VERIFY_ANIM_NODE_MEMBER_TYPE(Type, Identifier) static_assert(std::is_same_v<decltype(Identifier), Type>, "Incorrect return type used");
#else
#define VERIFY_ANIM_NODE_MEMBER_TYPE(Type, Identifier)
#endif

#define GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier) \
	[this]() -> UE::Anim::FNodeDataId \
	{ \
		VERIFY_ANIM_NODE_MEMBER_TYPE(Type, Identifier) \
		static UE::Anim::FNodeDataId CachedId_##Identifier; \
		if(!CachedId_##Identifier.IsValid()) \
		{ \
			static const FName AnimName_##Identifier(#Identifier); \
			CachedId_##Identifier = UE::Anim::FNodeDataId(AnimName_##Identifier, this, StaticStruct()); \
		} \
		return CachedId_##Identifier; \
	}() \

// Get some (potentially folded) anim node data. Only usable from within an anim node.
// 获取一些（可能折叠的）动画节点数据。只能在动画节点内使用。
// This caches the node data ID in static contained in a local lambda for improved performance
// 这会缓存本地 lambda 中包含的静态节点数据 ID，以提高性能
#define GET_ANIM_NODE_DATA(Type, Identifier) (GetData<Type>(GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier)))

// Get some anim node data that should be held on an instance. Only usable from within an anim node.
// 获取一些应该保存在实例上的动画节点数据。只能在动画节点内使用。
// @return nullptr if the data is not held on an instance (i.e. it is in constant sparse class data)
// @return nullptr 如果数据未保存在实例上（即它位于常量稀疏类数据中）
// This caches the node data ID in static contained in a local lambda for improved performance
// 这会缓存本地 lambda 中包含的静态节点数据 ID，以提高性能
#define GET_INSTANCE_ANIM_NODE_DATA_PTR(Type, Identifier) (GetInstanceDataPtr<Type>(GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier)))

#if WITH_EDITORONLY_DATA
// Editor-only way of accessing mutable anim node data but with internal checks
// 访问可变动画节点数据的仅限编辑器的方式，但具有内部检查
#define GET_MUTABLE_ANIM_NODE_DATA(Type, Identifier) (GetMutableData<Type>(GET_ANIM_NODE_DATA_ID_INTERNAL(Type, Identifier)))
#endif
