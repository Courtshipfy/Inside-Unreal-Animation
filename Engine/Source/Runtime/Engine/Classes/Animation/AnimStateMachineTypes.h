// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "AlphaBlend.h"
#include "BlendProfile.h"
#include "AnimStateMachineTypes.generated.h"

class UCurveFloat;
class UAnimStateTransitionNode;

UENUM(BlueprintType)
enum class ETransitionRequestQueueMode : uint8
{
	Shared			UMETA(ToolTip = "Only one transition can handle this request"),
	Unique			UMETA(ToolTip = "Allows multiple transitions to handle the same request"),
};

UENUM(BlueprintType)
enum class ETransitionRequestOverwriteMode : uint8
{
	Append			UMETA(ToolTip = "This request is added whether or not another with the same name is already queued"),
	Ignore			UMETA(ToolTip = "This request is ignored if another request with the same name is already queued"),
	Overwrite		UMETA(ToolTip = "This request overwrites another request with the same name if one exists")
};

//@TODO: Document
//@TODO：文档
UENUM()
namespace ETransitionBlendMode
{
	enum Type : int
	{
		TBM_Linear UMETA(DisplayName="Linear"),
		TBM_Cubic UMETA(DisplayName="Cubic")
	};
}

//@TODO: Document
//@TODO：文档
UENUM()
namespace ETransitionLogicType
{
	enum Type : int
	{
		TLT_StandardBlend UMETA(DisplayName="Standard Blend", ToolTip="Blend smoothly from source state to destination state.\nBoth states update during the transition.\nFalls back to Inertialization on re-entry to an already active state when Fall Back to Inertialization is true."),
		TLT_Inertialization UMETA(DisplayName = "Inertialization", ToolTip="Use inertialization to extrapolate when blending between states.\nOnly one state is active at a time.\nRequires an Inertialization or Dead Blending node rootwards of this node in the graph."),
		TLT_Custom UMETA(DisplayName="Custom", ToolTip="Use a custom graph to define exactly how the blend works.")
	};
}

struct FTransitionEvent
{
	TArray<int32, TInlineAllocator<8>> ConsumedTransitions;
	double CreationTime;
	double TimeToLive;
	FName EventName;
	ETransitionRequestQueueMode QueueMode;
	ETransitionRequestOverwriteMode OverwriteMode;

	FTransitionEvent(const FName& InEventName, const double InTimeToLive, const ETransitionRequestQueueMode& InQueueMode, const ETransitionRequestOverwriteMode& InOverwriteMode)
		: TimeToLive(InTimeToLive)
		, EventName(InEventName)
		, QueueMode(InQueueMode)
		, OverwriteMode(InOverwriteMode)
	{
		CreationTime = FPlatformTime::Seconds();
	}

	bool IsValidRequest() const
	{
		return TimeToLive > 0.0;
	}

	double GetRemainingTime() const
	{
		return TimeToLive - (FPlatformTime::Seconds() - CreationTime);
	}

	bool HasExpired() const
	{
		return GetRemainingTime() <= 0.0;
	}

	bool ToBeConsumed() const
	{
		if (QueueMode == ETransitionRequestQueueMode::Shared && ConsumedTransitions.Num() > 0)
		{
			return true;
		}
		return false;
	}

	bool HasBeenHandled() const
	{
		return ConsumedTransitions.Num() > 0;
	}

	FString ToDebugString() const
	{
		FString HandledByString = *FString::JoinBy(ConsumedTransitions, TEXT(", "), [](const int32& TransitionIndex) { return FString::Printf(TEXT("%d"), TransitionIndex); });
		return FString::Printf(TEXT("%s (%.2fs) [Handled by: %s]"), *EventName.ToString(), GetRemainingTime(), *HandledByString);
	}
};

// This structure represents a baked transition rule inside a state
// 该结构表示状态内的烘焙转换规则
USTRUCT()
struct FAnimationTransitionRule
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName RuleToExecute;

	/** What RuleToExecute must return to take transition (for bidirectional transitions) */
	/** RuleToExecute 必须返回什么才能进行转换（对于双向转换） */
	UPROPERTY()
	bool TransitionReturnVal;

	UPROPERTY()
	int32 TransitionIndex;

	FAnimationTransitionRule()
		: TransitionReturnVal(true)
		, TransitionIndex(INDEX_NONE)
	{}

	FAnimationTransitionRule(int32 InTransitionState)
		: TransitionIndex(InTransitionState)
	{
	}
};

// This is the base class that both baked states and transitions use
// 这是烘焙状态和转换都使用的基类
USTRUCT()
struct FAnimationStateBase
{
	GENERATED_USTRUCT_BODY()

	// The name of this state
	// 这个州的名称
	UPROPERTY()
	FName StateName;

	FAnimationStateBase()
	{}
};

//
USTRUCT()
struct FAnimationState : public FAnimationStateBase
{
	GENERATED_USTRUCT_BODY()

	// Set of legal transitions out of this state; already in priority order
	// 离开该状态的一组合法转换；已按优先顺序排列
	UPROPERTY()
	TArray<FAnimationTransitionRule> Transitions;

	// The root node index (into the AnimNodeProperties array of the UAnimBlueprintGeneratedClass)
	// 根节点索引（进入 UAnimBlueprintGenerateClass 的 AnimNodeProperties 数组）
	UPROPERTY()
	int32 StateRootNodeIndex;

	// The index of the notify to fire when this state is first entered (weight within the machine becomes non-zero)
	// 首次进入此状态时触发的通知索引（机器内的重量变为非零）
	UPROPERTY()
	int32 StartNotify;

	// The index of the notify to fire when this state is finished exiting (weight within the machine becomes zero)
	// 当该状态退出完成时触发的通知索引（机器内的重量变为零）
	UPROPERTY()
	int32 EndNotify;

	// The index of the notify to fire when this state is fully entered (weight within the machine becomes one)
	// 完全进入该状态时通知触发的索引（机器内的重量变为1）
	UPROPERTY()
	int32 FullyBlendedNotify;
	
	FAnimationState()
		: FAnimationStateBase()
		, StateRootNodeIndex(INDEX_NONE)
		, StartNotify(INDEX_NONE)
		, EndNotify(INDEX_NONE)
		, FullyBlendedNotify(INDEX_NONE)
	{}
};

// This represents a baked transition
// 这代表了一个烘焙的过渡
USTRUCT()
struct FAnimationTransitionBetweenStates : public FAnimationStateBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UCurveFloat> CustomCurve;

	UPROPERTY()
	TObjectPtr<UBlendProfile> BlendProfile;
	
	// Transition-only: State being transitioned from
	// 仅转换：状态正在从
	UPROPERTY()
	int32 PreviousState;

	// Transition-only: State being transitioned to
	// Transition-only：状态正在转换到
	UPROPERTY()
	int32 NextState;

	UPROPERTY()
	float CrossfadeDuration;

	UPROPERTY()
	float MinTimeBeforeReentry;

	UPROPERTY()
	int32 StartNotify;

	UPROPERTY()
	int32 EndNotify;

	UPROPERTY()
	int32 InterruptNotify;

	UPROPERTY()
	EAlphaBlendOption BlendMode;

	UPROPERTY()
	TEnumAsByte<ETransitionLogicType::Type> LogicType;

	UPROPERTY()
	uint8 bAllowInertializationForSelfTransitions : 1;

#if WITH_EDITORONLY_DATA
	// This is only needed for the baking process, to denote which baked transitions need to reverse their prev/next state in the final step
	// 这仅在烘焙过程中需要，以表示哪些烘焙过渡需要在最后一步中反转其上一个/下一个状态
	uint8 ReverseTransition : 1;
#endif

	FAnimationTransitionBetweenStates()
		: FAnimationStateBase()
		, CustomCurve(nullptr)
		, BlendProfile(nullptr)
		, PreviousState(INDEX_NONE)
		, NextState(INDEX_NONE)
		, CrossfadeDuration(-1.0f)
		, MinTimeBeforeReentry(-1.0f)
		, StartNotify(INDEX_NONE)
		, EndNotify(INDEX_NONE)
		, InterruptNotify(INDEX_NONE)
		, BlendMode(EAlphaBlendOption::CubicInOut)
		, LogicType(ETransitionLogicType::TLT_StandardBlend)
		, bAllowInertializationForSelfTransitions(false)
#if WITH_EDITOR
		, ReverseTransition(false)
#endif
	{}
};


USTRUCT()
struct FBakedStateExitTransition
{
	GENERATED_USTRUCT_BODY()

	// The node property index for this rule
	// 该规则的节点属性索引
	UPROPERTY()
	int32 CanTakeDelegateIndex;

	// The blend graph result node index
	// 混合图结果节点索引
	UPROPERTY()
	int32 CustomResultNodeIndex;

	// The index into the machine table of transitions
	// 机器转换表的索引
	UPROPERTY()
	int32 TransitionIndex;

	// What the transition rule node needs to return to take this transition (for bidirectional transitions)
	// 转换规则节点需要返回什么才能进行此转换（对于双向转换）
	UPROPERTY()
	bool bDesiredTransitionReturnValue;

	// Automatic Transition Rule based on animation remaining time.
	// 基于动画剩余时间的自动转换规则。
	UPROPERTY()
	bool bAutomaticRemainingTimeRule;

	// Automatic Transition Rule triggering time:
	// 自动转换规则触发时间：
	//  < 0 means trigger the transition 'Crossfade Duration' seconds before the end of the asset player, so a standard blend would finish just as the asset player ends
	//  < 0 表示在资产播放器结束前几秒触发过渡“交叉淡入淡出持续时间”，因此标准混合将在资产播放器结束时完成
	// >= 0 means trigger the transition 'Automatic Rule Trigger Time' seconds before the end of the asset player
	// >= 0 表示在资产播放器结束前几秒触发转换“自动规则触发时间”
	UPROPERTY()
	float AutomaticRuleTriggerTime;

	// Additional rule around SyncGroup requiring Valid Markers
	// 关于需要有效标记的 SyncGroup 的附加规则
	UPROPERTY()
	FName SyncGroupNameToRequireValidMarkersRule;

	UPROPERTY()
	TArray<int32> PoseEvaluatorLinks;

	FBakedStateExitTransition()
		: CanTakeDelegateIndex(INDEX_NONE)
		, CustomResultNodeIndex(INDEX_NONE)
		, TransitionIndex(INDEX_NONE)
		, bDesiredTransitionReturnValue(true)
		, bAutomaticRemainingTimeRule(false)
		, AutomaticRuleTriggerTime(-1.f)
		, SyncGroupNameToRequireValidMarkersRule(NAME_None)
	{
	}
};


//
USTRUCT()
struct FBakedAnimationState
{
	GENERATED_USTRUCT_BODY()

	// Indices into the property array for player nodes in the state
	// 状态中玩家节点的属性数组的索引
	UPROPERTY()
	TArray<int32> PlayerNodeIndices;

	// Indices into the property array for layer nodes in the state
	// 状态中层节点的属性数组的索引
	UPROPERTY()
	TArray<int32> LayerNodeIndices;
	
	// Set of legal transitions out of this state; already in priority order
	// 离开该状态的一组合法转换；已按优先顺序排列
	UPROPERTY()
	TArray<FBakedStateExitTransition> Transitions;

	// The name of this state
	// 这个州的名称
	UPROPERTY()
	FName StateName;

	// The root node index (into the AnimNodeProperties array of the UAnimBlueprintGeneratedClass)
	// 根节点索引（进入 UAnimBlueprintGeneeratedClass 的 AnimNodeProperties 数组）
	UPROPERTY()
	int32 StateRootNodeIndex;

	UPROPERTY()
	int32 StartNotify;

	UPROPERTY()
	int32 EndNotify;

	UPROPERTY()
	int32 FullyBlendedNotify;
	
	UPROPERTY()
	int32 EntryRuleNodeIndex;

	// Whether or not this state will ALWAYS reset it's state on reentry, regardless of remaining weight
	// 无论剩余重量如何，此状态是否总是在重新进入时重置其状态
	UPROPERTY()
	bool bAlwaysResetOnEntry;

	UPROPERTY()
	bool bIsAConduit;

public:
	FBakedAnimationState()
		: StateRootNodeIndex(INDEX_NONE)
		, StartNotify(INDEX_NONE)
		, EndNotify(INDEX_NONE)
		, FullyBlendedNotify(INDEX_NONE)
		, EntryRuleNodeIndex(INDEX_NONE)
		, bAlwaysResetOnEntry(false)
		, bIsAConduit(false)
	{}
};

USTRUCT()
struct FBakedAnimationStateMachine
{
	GENERATED_USTRUCT_BODY()

	// Name of this machine (primarily for debugging purposes)
	// 本机名称（主要用于调试目的）
	UPROPERTY()
	FName MachineName;

	// Index of the initial state that the machine will start in
	// 机器启动的初始状态索引
	UPROPERTY()
	int32 InitialState;

	// List of all states this machine can be in
	// 该机器可能处于的所有状态列表
	UPROPERTY()
	TArray<FBakedAnimationState> States;

	// List of all transitions between states
	// 状态之间所有转换的列表
	UPROPERTY()
	TArray<FAnimationTransitionBetweenStates> Transitions;

	// Cached StatID for this state machine
	// 此状态机的缓存 StatID
	STAT(mutable TStatId StatID;)

public:
	FBakedAnimationStateMachine()
		: InitialState(INDEX_NONE)
	{}

	// Finds a state by name or INDEX_NONE if no such state exists
	// 按名称查找状态，如果不存在此类状态，则按 INDEX_NONE 查找
	ENGINE_API int32 FindStateIndex(const FName& StateName) const;

	// Find the index of a transition from StateNameFrom to StateNameTo
	// 查找从 StateNameFrom 到 StateNameTo 的转换索引
	ENGINE_API int32 FindTransitionIndex(const FName& InStateNameFrom, const FName& InStateNameTo) const;
	ENGINE_API int32 FindTransitionIndex(const int32 InStateIdxFrom, const int32 InStateIdxTo) const;

#if STATS
	/** Get the StatID for timing this state machine */
	/** 获取用于对该状态机计时的 StatID */
	inline TStatId GetStatID() const
	{
		if (!StatID.IsValidStat())
		{
			StatID = FDynamicStats::CreateStatId<FStatGroup_STATGROUP_Anim>(MachineName);
		}
		return StatID;
	}
#endif // STATS
};

UCLASS()
class UAnimStateMachineTypes : public UObject
{
	GENERATED_UCLASS_BODY()
};

