// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimationAsset.h"
#include "AlphaBlend.h"
#include "Animation/AnimStateMachineTypes.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/BlendProfile.h"
#include "AnimNode_StateMachine.generated.h"

class IAnimClassInterface;
struct FAnimNode_AssetPlayerBase;
struct FAnimNode_AssetPlayerRelevancyBase;
struct FAnimNode_StateMachine;
struct FAnimNode_TransitionPoseEvaluator;

// Information about an active transition on the transition stack
// 有关转换堆栈上的活动转换的信息
USTRUCT()
struct FAnimationActiveTransitionEntry
{
	GENERATED_USTRUCT_BODY()

	// Elapsed time for this transition
	// 此转换所用的时间
	float ElapsedTime;

	// The transition alpha between next and previous states
	// 下一个状态和上一个状态之间的转换 alpha
	float Alpha;

	// Duration of this cross-fade (may be shorter than the nominal duration specified by the state machine if the target state had non-zero weight at the start)
	// 此交叉淡入淡出的持续时间（如果目标状态在开始时具有非零权重，则可能比状态机指定的标称持续时间短）
	float CrossfadeDuration;

	// Cached Pose for this transition
	// 此转换的缓存姿势
	TArray<FTransform> InputPose;

	// Graph to run that determines the final pose for this transition
	// 要运行的图形确定此转换的最终姿势
	FPoseLink CustomTransitionGraph;

	// To and from state ids
	// 往返州 ID
	int32 NextState;

	int32 PreviousState;

	// Notifies are copied from the reference transition info
	// 通知是从参考转换信息复制的
	int32 StartNotify;

	int32 EndNotify;

	int32 InterruptNotify;
	
	TArray<FAnimNode_TransitionPoseEvaluator*> PoseEvaluators;

	// Blend data used for per-bone animation evaluation
	// 用于每骨骼动画评估的混合数据
	TArray<FBlendSampleData> StateBlendData;

	TArray<int32, TInlineAllocator<3>> SourceTransitionIndices;

protected:
	// Blend object to handle alpha interpolation
	// 混合对象以处理 alpha 插值
	FAlphaBlend Blend;

public:
	// Blend profile to use for this transition. Specifying this will make the transition evaluate per-bone
	// 用于此过渡的混合配置文件。指定此项将使过渡评估每个骨骼
	UPROPERTY()
	TObjectPtr<UBlendProfile> BlendProfile;

	// Type of blend to use
	// 使用的混合物类型
	EAlphaBlendOption BlendOption;

	TEnumAsByte<ETransitionLogicType::Type> LogicType;

	// Is this transition active?
	// 此转换是否有效？
	bool bActive;

public:
	FAnimationActiveTransitionEntry();
	FAnimationActiveTransitionEntry(int32 NextStateID, float ExistingWeightOfNextState, int32 PreviousStateID, const FAnimationTransitionBetweenStates& ReferenceTransitionInfo, float CrossfadeTimeAdjustment);
	
	UE_DEPRECATED(5.1, "Please use FAnimationActiveTransitionEntry constructor with different signature")
	FAnimationActiveTransitionEntry(int32 NextStateID, float ExistingWeightOfNextState, FAnimationActiveTransitionEntry* ExistingTransitionForNextState, int32 PreviousStateID, const FAnimationTransitionBetweenStates& ReferenceTransitionInfo, const FAnimationPotentialTransition& PotentialTransition);

	void InitializeCustomGraphLinks(const FAnimationUpdateContext& Context, const FBakedStateExitTransition& TransitionRule);

	void Update(const FAnimationUpdateContext& Context, int32 CurrentStateIndex, bool &OutFinished);
	
	void UpdateCustomTransitionGraph(const FAnimationUpdateContext& Context, FAnimNode_StateMachine& StateMachine, int32 ActiveTransitionIndex);
	void EvaluateCustomTransitionGraph(FPoseContext& Output, FAnimNode_StateMachine& StateMachine, bool IntermediatePoseIsValid, int32 ActiveTransitionIndex);

	bool Serialize(FArchive& Ar);

protected:
	float CalculateInverseAlpha(EAlphaBlendOption BlendMode, float InFraction) const;
	float CalculateAlpha(float InFraction) const;
};

USTRUCT()
struct FAnimationPotentialTransition
{
	GENERATED_USTRUCT_BODY()

	int32 TargetState;
	float CrossfadeTimeAdjustment;

	const FBakedStateExitTransition* TransitionRule;

	TArray<int32, TInlineAllocator<3>> SourceTransitionIndices;

public:
	FAnimationPotentialTransition();
	bool IsValid() const;
	void Clear();
};

//@TODO: ANIM: Need to implement WithSerializer and Identical for FAnimationActiveTransitionEntry?
//@TODO：ANIM：需要为 FAnimationActiveTransitionEntry 实现 WithSerializer 和 Identical 吗？

// State machine node
// 状态机节点
USTRUCT()
struct FAnimNode_StateMachine : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:
	// Index into the BakedStateMachines array in the owning UAnimBlueprintGeneratedClass
	// 所属 UAnimBlueprintGenerateClass 中 BakedStateMachines 数组的索引
	UPROPERTY()
	int32 StateMachineIndexInClass;

	// The maximum number of transitions that can be taken by this machine 'simultaneously' in a single frame
	// 该机器可以在单个帧中“同时”进行的最大转换数
	UPROPERTY(EditAnywhere, Category=Settings)
	int32 MaxTransitionsPerFrame;

	// The maximum number of transition requests that can be buffered at any time.
	// 任何时候可以缓冲的最大转换请求数。
	// The oldest transition requests are dropped to accommodate for newly created requests.
	// 最旧的转换请求将被删除以适应新创建的请求。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (ClampMin = "0"))
	int32 MaxTransitionsRequests = 32;

	// When the state machine becomes relevant, it is initialized into the Entry state.
	// 当状态机变得相关时，它被初始化为进入状态。
	// It then tries to take any valid transitions to possibly end up in a different state on that same frame.
	// 然后，它尝试采取任何有效的转换，以可能在同一帧上以不同的状态结束。
	// - if true, that new state starts full weight.
	// - 如果为真，则新状态开始满负荷。
	// - if false, a blend is created between the entry state and that new state.
	// - 如果为 false，则在进入状态和新状态之间创建混合。
	// In either case all visited State notifications (Begin/End) will be triggered.
	// 在任何一种情况下，所有访问过的状态通知（开始/结束）都将被触发。
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bSkipFirstUpdateTransition;

	// Reinitialize the state machine if we have become relevant to the graph
	// 如果我们与图相关，则重新初始化状态机
	// after not being ticked on the previous frame(s)
	// 未在前一帧上勾选后
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bReinitializeOnBecomingRelevant;

	// Tag Notifies with meta data such as the active state and mirroring state.  Producing this
	// 标签通过元数据（例如活动状态和镜像状态）进行通知。  生产这个
	// data has a  slight performance cost.
	// 数据有轻微的性能成本。
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bCreateNotifyMetaData;

	// Allows a conduit to be used as this state machine's entry state
	// 允许使用管道作为该状态机的入口状态
	// If a valid entry state cannot be found at runtime then this will generate a reference pose!
	// 如果在运行时找不到有效的进入状态，那么这将生成一个参考姿势！
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bAllowConduitEntryStates;
private:
	// true if it is the first update.
	// 如果是第一次更新则为 true。
	bool bFirstUpdate;

public:

	int32 GetCurrentState() const
	{
		return CurrentState;
	}

	float GetCurrentStateElapsedTime() const
	{
		return ElapsedTime;
	}

	ENGINE_API FName GetCurrentStateName() const;

	ENGINE_API bool IsTransitionActive(int32 TransIndex) const;

protected:
	// The current state within the state machine
	// 状态机内的当前状态
	int32 CurrentState;

	// Elapsed time since entering the current state
	// 自进入当前状态以来经过的时间
	float ElapsedTime;

	// Current Transition Index being evaluated
	// 当前正在评估的转型指数
	int32 EvaluatingTransitionIndex;

	// The state machine description this is an instance of
	// 这是一个实例的状态机描述
	const FBakedAnimationStateMachine* PRIVATE_MachineDescription;

	// The set of active transitions, if there are any
	// 活动转换集（如果有）
	TArray<FAnimationActiveTransitionEntry> ActiveTransitionArray;

	// The set of states in this state machine
	// 该状态机中的状态集
	TArray<FPoseLink> StatePoseLinks;
	
	// Used during transitions to make sure we don't double tick a state if it appears multiple times
	// 在转换过程中使用，以确保我们不会在某个状态出现多次时对其进行双重勾选
	TArray<int32> StatesUpdated;

	// Delegates that native code can hook into to handle state entry
	// 本机代码可以挂钩以处理状态条目的委托
	TArray<FOnGraphStateChanged> OnGraphStatesEntered;

	// Delegates that native code can hook into to handle state exits
	// 本机代码可以挂钩以处理状态退出的委托
	TArray<FOnGraphStateChanged> OnGraphStatesExited;

	// All alive transition requests that have been queued
	// 已排队的所有活动转换请求
	TArray<FTransitionEvent> QueuedTransitionEvents;

#if WITH_EDITORONLY_DATA
	// The set of transition requests handled this update
	// 处理此更新的转换请求集
	TArray<FTransitionEvent> HandledTransitionEvents;
#endif

private:
	TArray<FPoseContext*> StateCachedPoses;

	FGraphTraversalCounter UpdateCounter;

	TArray<FGraphTraversalCounter> StateCacheBoneCounters;

	// The last time that each respective state was entered
	// 上次进入各个状态的时间
	TArray<double> LastStateEntryTime; 

#if WITH_EDITOR
	int32 DebugForcedStateIndex = INDEX_NONE;
#endif

public:
	FAnimNode_StateMachine()
		: StateMachineIndexInClass(0)
		, MaxTransitionsPerFrame(3)
		, bSkipFirstUpdateTransition(true)
		, bReinitializeOnBecomingRelevant(true)
		, bCreateNotifyMetaData(true)
		, bAllowConduitEntryStates(false)
		, bFirstUpdate(true)
		, CurrentState(INDEX_NONE)
		, ElapsedTime(0.0f)
		, PRIVATE_MachineDescription(NULL)
	{
	}

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ENGINE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ENGINE_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ENGINE_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ENGINE_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ENGINE_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	ENGINE_API void ConditionallyCacheBonesForState(int32 StateIndex, FAnimationBaseContext Context);

	// Returns the blend weight of the specified state, as calculated by the last call to Update()
	// 返回指定状态的混合权重，由上次调用 Update() 计算得出
	ENGINE_API float GetStateWeight(int32 StateIndex) const;

	ENGINE_API const FBakedAnimationState& GetStateInfo(int32 StateIndex) const;
	ENGINE_API const FAnimationTransitionBetweenStates& GetTransitionInfo(int32 TransIndex) const;
	
	ENGINE_API bool IsValidTransitionIndex(int32 TransitionIndex) const;

	/** Cache the internal machine description */
	/** 缓存内部机器描述 */
	ENGINE_API void CacheMachineDescription(IAnimClassInterface* AnimBlueprintClass);

	ENGINE_API void SetState(const FAnimationBaseContext& Context, int32 NewStateIndex, bool bAllowReEntry);
	ENGINE_API void TransitionToState(const FAnimationUpdateContext& Context, const FAnimationTransitionBetweenStates& TransitionInfo, const FAnimationPotentialTransition* BakedTransitionInfo = nullptr);
	ENGINE_API const int32 GetStateIndex(FName StateName) const;

protected:
	// Tries to get the instance information for the state machine
	// 尝试获取状态机的实例信息
	ENGINE_API const FBakedAnimationStateMachine* GetMachineDescription() const;

	ENGINE_API void SetStateInternal(int32 NewStateIndex);

	ENGINE_API const FBakedAnimationState& GetStateInfo() const;
	ENGINE_API const int32 GetStateIndex(const FBakedAnimationState& StateInfo) const;
	
	// finds the highest priority valid transition, information pass via the OutPotentialTransition variable.
	// 找到最高优先级的有效转换，信息通过 OutPotentialTransition 变量传递。
	// OutVisitedStateIndices will let you know what states were checked, but is also used to make sure we don't get stuck in an infinite loop or recheck states
	// OutVisitedStateIndices 会让您知道检查了哪些状态，但也用于确保我们不会陷入无限循环或重新检查状态
	ENGINE_API bool FindValidTransition(const FAnimationUpdateContext& Context, 
							const FBakedAnimationState& StateInfo,
							/*OUT*/ FAnimationPotentialTransition& OutPotentialTransition,
							/*OUT*/ TArray<int32, TInlineAllocator<4>>& OutVisitedStateIndices);

	// Helper function that will update the states associated with a transition
	// 将更新与转换相关的状态的辅助函数
	ENGINE_API void UpdateTransitionStates(const FAnimationUpdateContext& Context, FAnimationActiveTransitionEntry& Transition);

	// helper function to test if a state is a conduit
	// 用于测试状态是否为管道的辅助函数
	ENGINE_API bool IsAConduitState(int32 StateIndex) const;

	// helper functions for calling update and evaluate on state nodes
	// 用于在状态节点上调用更新和评估的辅助函数
	ENGINE_API void UpdateState(int32 StateIndex, const FAnimationUpdateContext& Context);
	ENGINE_API const FPoseContext& EvaluateState(int32 StateIndex, const FPoseContext& Context);

	// transition type evaluation functions
	// 过渡型评价函数
	ENGINE_API void EvaluateTransitionStandardBlend(FPoseContext& Output, FAnimationActiveTransitionEntry& Transition, bool bIntermediatePoseIsValid);
	ENGINE_API void EvaluateTransitionStandardBlendInternal(FPoseContext& Output, FAnimationActiveTransitionEntry& Transition, const FPoseContext& PreviousStateResult, const FPoseContext& NextStateResult);
	ENGINE_API void EvaluateTransitionCustomBlend(FPoseContext& Output, FAnimationActiveTransitionEntry& Transition, bool bIntermediatePoseIsValid);

	// Get the time remaining in seconds for the most relevant animation in the source state 
	// 获取源状态中最相关动画的剩余时间（以秒为单位）
	ENGINE_API float GetRelevantAnimTimeRemaining(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const;
	float GetRelevantAnimTimeRemaining(const FAnimationUpdateContext& Context, int32 StateIndex) const
	{
		return GetRelevantAnimTimeRemaining(Context.AnimInstanceProxy, StateIndex);
	}

	// Get the time remaining as a fraction of the duration for the most relevant animation in the source state 
	// 获取源状态中最相关动画的剩余时间（作为持续时间的一部分）
	ENGINE_API float GetRelevantAnimTimeRemainingFraction(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const;
	float GetRelevantAnimTimeRemainingFraction(const FAnimationUpdateContext& Context, int32 StateIndex) const
	{
		return GetRelevantAnimTimeRemainingFraction(Context.AnimInstanceProxy, StateIndex);
	}

	UE_DEPRECATED(5.1, "Please use GetRelevantAssetPlayerInterfaceFromState")
	const FAnimNode_AssetPlayerBase* GetRelevantAssetPlayerFromState(const FAnimInstanceProxy* InAnimInstanceProxy, const FBakedAnimationState& StateInfo) const
	{
		return nullptr;
	}

	UE_DEPRECATED(5.1, "Please use GetRelevantAssetPlayerInterfaceFromState")
	const FAnimNode_AssetPlayerBase* GetRelevantAssetPlayerFromState(const FAnimationUpdateContext& Context, const FBakedAnimationState& StateInfo) const
	{
		return nullptr;
	}

	ENGINE_API const FAnimNode_AssetPlayerRelevancyBase* GetRelevantAssetPlayerInterfaceFromState(const FAnimInstanceProxy* InAnimInstanceProxy, const FBakedAnimationState& StateInfo) const;
	const FAnimNode_AssetPlayerRelevancyBase* GetRelevantAssetPlayerInterfaceFromState(const FAnimationUpdateContext& Context, const FBakedAnimationState& StateInfo) const
	{
		return GetRelevantAssetPlayerInterfaceFromState(Context.AnimInstanceProxy, StateInfo);
	}

	ENGINE_API virtual void LogInertializationRequestError(const FAnimationUpdateContext& Context, int32 PreviousState, int32 NextState);

	/** Queues a new transition request, returns true if the transition request was successfully queued */
	/** 将新的转换请求排队，如果转换请求成功排队，则返回 true */
	ENGINE_API bool RequestTransitionEvent(const FTransitionEvent& InTransitionEvent);

	/** Removes all queued transition requests with the given event name */
	/** 删除具有给定事件名称的所有排队转换请求 */
	ENGINE_API void ClearTransitionEvents(const FName& EventName);

	/** Removes all queued transition requests*/
	/** 删除所有排队的转换请求*/
	ENGINE_API void ClearAllTransitionEvents();

	/** Returns whether or not the given event transition request has been queued */
	/** 返回给定的事件转换请求是否已排队 */
	ENGINE_API bool QueryTransitionEvent(const int32 TransitionIndex, const FName& EventName) const;

	/** Behaves like QueryTransitionEvent but additionally marks the event for consumption */
	/** 行为类似于 QueryTransitionEvent 但另外标记事件以供使用 */
	ENGINE_API bool QueryAndMarkTransitionEvent(const int32 TransitionIndex, const FName& EventName);

	/** Removes all marked events that are queued */
	/** 删除排队的所有标记事件 */
	ENGINE_API void ConsumeMarkedTransitionEvents();

#if WITH_EDITOR
	void DebugForceState(int32 InStateIndex)
	{
		DebugForcedStateIndex = InStateIndex;
	}
#endif

public:
	friend struct FAnimInstanceProxy;
	friend class UAnimationStateMachineLibrary;
	friend class FAnimationBlueprintEditor;
};
