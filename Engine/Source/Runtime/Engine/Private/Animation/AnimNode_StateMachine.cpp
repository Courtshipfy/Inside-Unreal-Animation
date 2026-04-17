// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimNode_StateMachine.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_RelevantAssetPlayerBase.h"
#include "Animation/AnimNode_TransitionResult.h"
#include "Animation/AnimNode_TransitionPoseEvaluator.h"
#include "Animation/AnimNode_Inertialization.h"
#include "Animation/AnimStats.h"
#include "Animation/AnimTrace.h"
#include "Animation/BlendProfile.h"
#include "Animation/AnimNode_LinkedAnimLayer.h"
#include "Animation/ActiveStateMachineScope.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/ExposedValueHandler.h"
#include "Logging/TokenizedMessage.h"
#include "Animation/AnimInertializationSyncScope.h"
#include "Animation/AnimNode_StateResult.h"
#include "Animation/SkeletonRemapping.h"
#include "Animation/SkeletonRemappingRegistry.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_StateMachine)

#if WITH_EDITORONLY_DATA
#endif

#define LOCTEXT_NAMESPACE "AnimNode_StateMachine"

DEFINE_LOG_CATEGORY_STATIC(LogAnimTransitionRequests, NoLogging, All);

DECLARE_CYCLE_STAT(TEXT("StateMachine SetState"), Stat_StateMachineSetState, STATGROUP_Anim);

static const FName DefaultAnimGraphName("AnimGraph");

//////////////////////////////////////////////////////////////////////////
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry
// FAnimationActiveTransitionEntry

FAnimationActiveTransitionEntry::FAnimationActiveTransitionEntry()
	: ElapsedTime(0.0f)
	, Alpha(0.0f)
	, CrossfadeDuration(0.0f)
	, NextState(INDEX_NONE)
	, PreviousState(INDEX_NONE)
	, StartNotify(INDEX_NONE)
	, EndNotify(INDEX_NONE)
	, InterruptNotify(INDEX_NONE)
	, BlendProfile(nullptr)
	, BlendOption(EAlphaBlendOption::HermiteCubic)
	, LogicType(ETransitionLogicType::TLT_StandardBlend)
	, bActive(false)
{
}

FAnimationActiveTransitionEntry::FAnimationActiveTransitionEntry(int32 NextStateID, float ExistingWeightOfNextState, FAnimationActiveTransitionEntry* ExistingTransitionForNextState, int32 PreviousStateID, const FAnimationTransitionBetweenStates& ReferenceTransitionInfo, const FAnimationPotentialTransition& PotentialTransition)
	: FAnimationActiveTransitionEntry(NextStateID, ExistingWeightOfNextState, PreviousStateID, ReferenceTransitionInfo, PotentialTransition.CrossfadeTimeAdjustment)
{
}

FAnimationActiveTransitionEntry::FAnimationActiveTransitionEntry(int32 NextStateID, float ExistingWeightOfNextState, int32 PreviousStateID, const FAnimationTransitionBetweenStates& ReferenceTransitionInfo, float CrossfadeTimeAdjustment)
	: ElapsedTime(0.0f)
	, Alpha(0.0f)
	, NextState(NextStateID)
	, PreviousState(PreviousStateID)
	, StartNotify(ReferenceTransitionInfo.StartNotify)
	, EndNotify(ReferenceTransitionInfo.EndNotify)
	, InterruptNotify(ReferenceTransitionInfo.InterruptNotify)
	, BlendProfile(ReferenceTransitionInfo.BlendProfile)
	, BlendOption(ReferenceTransitionInfo.BlendMode)
	, LogicType(ReferenceTransitionInfo.LogicType)
	, bActive(true)
{
	const float Scaler = 1.0f - ExistingWeightOfNextState;
	CrossfadeDuration = (LogicType == ETransitionLogicType::TLT_Inertialization) ? 0.0f : FMath::Max(ReferenceTransitionInfo.CrossfadeDuration - CrossfadeTimeAdjustment, 0.f) * CalculateInverseAlpha(BlendOption, Scaler);

	Blend.SetBlendTime(CrossfadeDuration);
	Blend.SetBlendOption(BlendOption);
	Blend.SetCustomCurve(ReferenceTransitionInfo.CustomCurve);
	Blend.SetValueRange(0.0f, 1.0f);
}

float FAnimationActiveTransitionEntry::CalculateInverseAlpha(EAlphaBlendOption BlendMode, float InFraction) const
{
	if(BlendMode == EAlphaBlendOption::HermiteCubic)
	{
		const float A = 4.0f / 3.0f;
		const float B = -2.0f;
		const float C = 5.0f / 3.0f;

		const float T = InFraction;
		const float TT = InFraction*InFraction;
		const float TTT = InFraction*InFraction*InFraction;

		return TTT*A + TT*B + T*C;
	}
	else
	{
		return FMath::Clamp<float>(InFraction, 0.0f, 1.0f);
	}
}

void FAnimationActiveTransitionEntry::InitializeCustomGraphLinks(const FAnimationUpdateContext& Context, const FBakedStateExitTransition& TransitionRule)
{
	if (TransitionRule.CustomResultNodeIndex != INDEX_NONE)
	{
		if (const IAnimClassInterface* AnimBlueprintClass = Context.GetAnimClass())
		{
			CustomTransitionGraph.LinkID = AnimBlueprintClass->GetAnimNodeProperties().Num() - 1 - TransitionRule.CustomResultNodeIndex; //@TODO: Crazysauce
			FAnimationInitializeContext InitContext(Context.AnimInstanceProxy, Context.SharedContext);
			CustomTransitionGraph.Initialize(InitContext);

			if (Context.AnimInstanceProxy)
			{
				for (int32 Index = 0; Index < TransitionRule.PoseEvaluatorLinks.Num(); ++Index)
				{
					FAnimNode_TransitionPoseEvaluator* PoseEvaluator = GetNodeFromPropertyIndex<FAnimNode_TransitionPoseEvaluator>(Context.AnimInstanceProxy->GetAnimInstanceObject(), AnimBlueprintClass, TransitionRule.PoseEvaluatorLinks[Index]);
					PoseEvaluators.Add(PoseEvaluator);
				}
			}
		}
	}
}

void FAnimationActiveTransitionEntry::Update(const FAnimationUpdateContext& Context, int32 CurrentStateIndex, bool& bOutFinished)
{
	bOutFinished = false;

	// Advance time
 // 提前时间
	if (bActive)
	{
		ElapsedTime += Context.GetDeltaTime();
		Blend.Update(Context.GetDeltaTime());

		// If non-zero, calculate the query alpha
  // 如果非零，则计算查询 alpha
		float QueryAlpha = 0.0f;
		if (CrossfadeDuration > 0.0f)
		{
			QueryAlpha = ElapsedTime / CrossfadeDuration;
		}

		Alpha = FAlphaBlend::AlphaToBlendOption(QueryAlpha, Blend.GetBlendOption(), Blend.GetCustomCurve());

		if (Blend.IsComplete())
		{
			bActive = false;
			bOutFinished = true;
		}

		// Update state blend data (only when we're using per-bone)
  // 更新状态混合数据（仅当我们使用每个骨骼时）
		if (BlendProfile)
		{
			for (int32 Idx = 0 ; Idx < 2 ; ++Idx)
			{
				const bool bForwards = (Idx == 0);
				StateBlendData[Idx].TotalWeight = bForwards ? Alpha : 1.0f - Alpha;
				BlendProfile->UpdateBoneWeights(StateBlendData[Idx], Blend, 0.0f, StateBlendData[Idx].TotalWeight, !bForwards);
			}

			FBlendSampleData::NormalizeDataWeight(StateBlendData);
		}
	}
}

bool FAnimationActiveTransitionEntry::Serialize(FArchive& Ar)
{
	Ar << ElapsedTime;
	Ar << Alpha;
	Ar << CrossfadeDuration;
	Ar << bActive;
	Ar << NextState;
	Ar << PreviousState;

	return true;
}

/////////////////////////////////////////////////////
// FAnimationPotentialTransition
// F动画电位转换

FAnimationPotentialTransition::FAnimationPotentialTransition()
: 	TargetState(INDEX_NONE)
,	CrossfadeTimeAdjustment(0.f)
,	TransitionRule(NULL)
{
}

bool FAnimationPotentialTransition::IsValid() const
{
	return (TargetState != INDEX_NONE) && (TransitionRule != NULL) && (TransitionRule->TransitionIndex != INDEX_NONE);
}

void FAnimationPotentialTransition::Clear()
{
	TargetState = INDEX_NONE;
	CrossfadeTimeAdjustment = 0.f;
	TransitionRule = NULL;
	SourceTransitionIndices.Reset();
 // FAnimNode_StateMachine
 // FAnimNode_StateMachine
// FAnimNode_StateMachine
// FAnimNode_StateMachine
// FAnimNode_StateMachine
// FAnimNode_StateMachine
}

// FAnimNode_StateMachine
// FAnimNode_StateMachine

/////////////////////////////////////////////////////
// FAnimNode_StateMachine
// FAnimNode_StateMachine

// Tries to get the instance information for the state machine
// 尝试获取状态机的实例信息
const FBakedAnimationStateMachine* FAnimNode_StateMachine::GetMachineDescription() const
{
	if (PRIVATE_MachineDescription != nullptr)
	{
		return PRIVATE_MachineDescription;
	}
	else
	{
		UE_LOG(LogAnimation, Warning, TEXT("FAnimNode_StateMachine: Bad machine ptr"));
		return nullptr;
	}
}

void FAnimNode_StateMachine::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);

	IAnimClassInterface* AnimBlueprintClass = Context.GetAnimClass();

	if (const FBakedAnimationStateMachine* Machine = GetMachineDescription())
	{
		ElapsedTime = 0.0f;

		CurrentState = INDEX_NONE;

		if (Machine->States.Num() > 0)
		{
			// Create a pose link for each state we can reach
   // 为我们可以到达的每个状态创建一个姿势链接
			StatePoseLinks.Reset();
			StatePoseLinks.Reserve(Machine->States.Num());
			for (int32 StateIndex = 0; StateIndex < Machine->States.Num(); ++StateIndex)
			{
				const FBakedAnimationState& State = Machine->States[StateIndex];
				FPoseLink& StatePoseLink = StatePoseLinks.AddDefaulted_GetRef();

				// because conduits don't contain bound graphs, this link is no longer guaranteed to be valid
    // 由于管道不包含绑定图，因此不再保证此链接有效
				if (State.StateRootNodeIndex != INDEX_NONE)
				{
					StatePoseLink.LinkID = AnimBlueprintClass->GetAnimNodeProperties().Num() - 1 - State.StateRootNodeIndex; //@TODO: Crazysauce
				}

				// also initialize transitions
    // 还初始化转换
				if(State.EntryRuleNodeIndex != INDEX_NONE)
				{
					if (FAnimNode_TransitionResult* TransitionNode = GetNodeFromPropertyIndex<FAnimNode_TransitionResult>(Context.AnimInstanceProxy->GetAnimInstanceObject(), AnimBlueprintClass, State.EntryRuleNodeIndex))
					{
						TransitionNode->Initialize_AnyThread(Context);
					}
				}

				for(int32 TransitionIndex = 0; TransitionIndex < State.Transitions.Num(); ++TransitionIndex)
				{
					const FBakedStateExitTransition& TransitionRule = State.Transitions[TransitionIndex];
					if (TransitionRule.CanTakeDelegateIndex != INDEX_NONE)
					{
						if (FAnimNode_TransitionResult* TransitionNode = GetNodeFromPropertyIndex<FAnimNode_TransitionResult>(Context.AnimInstanceProxy->GetAnimInstanceObject(), AnimBlueprintClass, TransitionRule.CanTakeDelegateIndex))
						{
							TransitionNode->Initialize_AnyThread(Context);
						}
					}
				}
			}

			// Reset transition related variables
   // 重置转换相关变量
			StatesUpdated.Reset();
			ActiveTransitionArray.Reset();
			QueuedTransitionEvents.Reset();

			StateCacheBoneCounters.Reset(Machine->States.Num());
			StateCacheBoneCounters.AddDefaulted(Machine->States.Num());

			LastStateEntryTime.Reset(Machine->States.Num());
			LastStateEntryTime.AddZeroed(Machine->States.Num());

			// Move to the default state
   // 移至默认状态
			SetState(Context, Machine->InitialState, false);

			// initialize first update
   // 初始化第一次更新
			bFirstUpdate = true;
		}
	}
}

void FAnimNode_StateMachine::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) 
{
	if (const FBakedAnimationStateMachine* Machine = GetMachineDescription())
	{
		for (int32 StateIndex = 0; StateIndex < Machine->States.Num(); ++StateIndex)
		{
			if (GetStateWeight(StateIndex) > 0.f)
			{
				ConditionallyCacheBonesForState(StateIndex, Context);
			}
		}
	}

	// @TODO GetStateWeight is O(N) transitions.
 // @TODO GetStateWeight 是 O(N) 转换。
}

void FAnimNode_StateMachine::ConditionallyCacheBonesForState(int32 StateIndex, FAnimationBaseContext Context)
{
	// Only call CacheBones when needed.
 // 仅在需要时调用 CacheBones。
	check(StateCacheBoneCounters.IsValidIndex(StateIndex));

	const FGraphTraversalCounter& ProxyCachedBonesCounter = Context.AnimInstanceProxy->GetCachedBonesCounter();

	// Check both the frame and counter
 // 检查框架和计数器
	// Multiple anim instances can be linked together and their LODs can be independent
 // 多个动画实例可以链接在一起，并且它们的 LOD 可以是独立的
	// This means that a sub-instance can have its LOD change, causing the value we cache here to run ahead
 // 这意味着子实例可以更改其 LOD，从而导致我们在这里缓存的值提前运行
	// of the one from the main anim instance. When the LOD of the main instance will change, we'll reset
 // 来自主动画实例的一个。当主实例的LOD发生变化时，我们将重置
	// all the counters to match it which can cause them to run backwards
 // 所有与其匹配的计数器可能会导致它们向后运行
	if (!StateCacheBoneCounters[StateIndex].IsSynchronized_All(ProxyCachedBonesCounter))
	{
		// keep track of states that have had CacheBones called on.
  // 跟踪已调用 CacheBones 的状态。
		StateCacheBoneCounters[StateIndex].SynchronizeWith(ProxyCachedBonesCounter);

		FAnimationCacheBonesContext CacheBoneContext(Context.AnimInstanceProxy);
		StatePoseLinks[StateIndex].CacheBones(CacheBoneContext);
	}
}

const FBakedAnimationState& FAnimNode_StateMachine::GetStateInfo() const
{
	return PRIVATE_MachineDescription->States[CurrentState];
}

const FBakedAnimationState& FAnimNode_StateMachine::GetStateInfo(int32 StateIndex) const
{
	return PRIVATE_MachineDescription->States[StateIndex];
}

const int32 FAnimNode_StateMachine::GetStateIndex( const FBakedAnimationState& StateInfo ) const
{
	for (int32 Index = 0; Index < PRIVATE_MachineDescription->States.Num(); ++Index)
	{
		if( &PRIVATE_MachineDescription->States[Index] == &StateInfo )
		{
			return Index;
		}
	}

	return INDEX_NONE;
}


const int32 FAnimNode_StateMachine::GetStateIndex(FName StateName) const
{
	for (int32 Index = 0; Index < PRIVATE_MachineDescription->States.Num(); ++Index)
	{
		if (PRIVATE_MachineDescription->States[Index].StateName == StateName)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

const FAnimationTransitionBetweenStates& FAnimNode_StateMachine::GetTransitionInfo(int32 TransIndex) const
{
	return PRIVATE_MachineDescription->Transitions[TransIndex];
}

void FAnimNode_StateMachine::LogInertializationRequestError(const FAnimationUpdateContext& Context, int32 PreviousState, int32 NextState)
{
#if WITH_EDITORONLY_DATA
	const FBakedAnimationStateMachine* Machine = GetMachineDescription();
	FText Message = FText::Format(LOCTEXT("InertialTransitionError", "No Inertialization node found for request from transition '{0}' to '{1}' in state machine '{2}' in anim blueprint '{3}'. Add an Inertialization node after this state machine."),
		FText::FromName(GetStateInfo(PreviousState).StateName),
		FText::FromName(GetStateInfo(NextState).StateName),
		FText::FromName(Machine->MachineName),
		FText::FromString(GetPathNameSafe(Context.AnimInstanceProxy->GetAnimBlueprint())));
	Context.LogMessage(EMessageSeverity::Error, Message);
#endif
}

// Temporarily turned off while we track down and fix https://jira.ol.epicgames.net/browse/OR-17066
// 在我们追踪和修复期间暂时关闭 https://jira.ol.epicgames.net/browse/OR-17066
TAutoConsoleVariable<int32> CVarAnimStateMachineRelevancyReset(TEXT("a.AnimNode.StateMachine.EnableRelevancyReset"), 1, TEXT("Reset State Machine when it becomes relevant"));


void FAnimNode_StateMachine::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	Context.AnimInstanceProxy->RecordMachineWeight(StateMachineIndexInClass, Context.GetFinalBlendWeight());

	// If we just became relevant and haven't been initialized yet, then reinitialize state machine.
 // 如果我们刚刚变得相关并且尚未初始化，则重新初始化状态机。
	if (!bFirstUpdate && bReinitializeOnBecomingRelevant && UpdateCounter.HasEverBeenUpdated() && !UpdateCounter.WasSynchronizedCounter(Context.AnimInstanceProxy->GetUpdateCounter()) && (CVarAnimStateMachineRelevancyReset.GetValueOnAnyThread() == 1))
	{
		FAnimationInitializeContext InitializationContext(Context.AnimInstanceProxy, Context.SharedContext);
		Initialize_AnyThread(InitializationContext);
	}
	UpdateCounter.SynchronizeWith(Context.AnimInstanceProxy->GetUpdateCounter());

	// Remove expired transition requests
 // 删除过期的转换请求
#if WITH_EDITORONLY_DATA
	HandledTransitionEvents.Reset();
#endif
	for (int32 RequestIndex = QueuedTransitionEvents.Num() - 1; RequestIndex >= 0; --RequestIndex)
	{
		if (QueuedTransitionEvents[RequestIndex].HasExpired())
		{
			UE_LOG(LogAnimTransitionRequests, Verbose, TEXT("'%s' expired (Machine: %s)"), *QueuedTransitionEvents[RequestIndex].EventName.ToString(), *GetMachineDescription()->MachineName.ToString());
			QueuedTransitionEvents.RemoveAt(RequestIndex, EAllowShrinking::No);
		}
	}
	QueuedTransitionEvents.Shrink();

	const FBakedAnimationStateMachine* Machine = GetMachineDescription();
	if (Machine != nullptr)
	{
		if (Machine->States.Num() == 0)
		{
			return;
		}
		else if(!Machine->States.IsValidIndex(CurrentState))
		{
			// Attempting to catch a crash where the state machine has been freed.
   // 尝试捕获状态机已被释放的崩溃。
			// Reported as a symptom of a crash in UE-24732 for 4.10. This log message should not appear given changes to
   // 报告为 4.10 UE-24732 崩溃的症状。如果更改了此日志消息，则不应出现
			// re-instancing in 4.11 (see CL 2823202). If it does appear we need to spot integrate CL 2823202 (and supporting 
   // 4.11 中的重新实例（参见 CL 2823202）。如果确实出现，我们需要现场集成 CL 2823202（并支持
			// anim re-init changes, probably 2799786 & 2801372).
   // anim 重新初始化更改，可能是 2799786 和 2801372）。
			UE_LOG(LogAnimation, Warning, TEXT("FAnimNode_StateMachine::Update - Invalid current state, please report. Attempting to use state %d of %d in state machine %d (ptr 0x%" UPTRINT_x_FMT ")"), CurrentState, Machine->States.Num(), StateMachineIndexInClass, Machine);
			UE_LOG(LogAnimation, Warning, TEXT("\t\tWhen updating AnimInstance: %s"), *Context.AnimInstanceProxy->GetAnimInstanceObject()->GetName())

			return;
		}
	}
	else
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_AnimStateMachineUpdate);
#if	STATS
	// Record name of state machine we are updating
 // 记录我们正在更新的状态机名称
	FScopeCycleCounter MachineNameCycleCounter(Machine->GetStatID());
#endif // STATS

#if WITH_EDITOR
	if (DebugForcedStateIndex != INDEX_NONE)
	{
		if (PRIVATE_MachineDescription->States.IsValidIndex(DebugForcedStateIndex))
		{
			SetState(Context, DebugForcedStateIndex, true);
		}
		DebugForcedStateIndex = INDEX_NONE;
	}
#endif

	// On the first update, call the initial state's Start Notify.
 // 第一次更新时，调用初始状态的 Start Notify。
	if (bFirstUpdate)
	{
		//Handle enter notify for "first" (after initial transitions) state
  // 处理“第一”（初始转换后）状态的输入通知
		Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(GetStateInfo().StartNotify);
	}

	bool bFoundValidTransition = false;
	int32 TransitionCountThisFrame = 0;

	// Look for legal transitions to take; can move across multiple states in one frame (up to MaxTransitionsPerFrame)
 // 寻求法律上的过渡；可以在一帧中跨多个状态移动（最多 MaxTransitionsPerFrame）
	do
	{
		bFoundValidTransition = false;
		FAnimationPotentialTransition PotentialTransition;
		
		{
			SCOPE_CYCLE_COUNTER(STAT_AnimStateMachineFindTransition);

			// Evaluate possible transitions out of this state
   // 评估离开此状态的可能转换
			//@TODO: Evaluate if a set is better than an array for the probably low N encountered here
   // @TODO：对于此处遇到的可能较低的 N ，评估集合是否比数组更好
			TArray<int32, TInlineAllocator<4>> VisitedStateIndices;
			FindValidTransition(Context, GetStateInfo(), /*Out*/ PotentialTransition, /*Out*/ VisitedStateIndices);
		}

		auto CanTransition = [this, &PotentialTransition]()
		{
			if (!PotentialTransition.IsValid())
			{
				return false;
			}

			const FAnimationTransitionBetweenStates& ReferenceTransition = GetTransitionInfo(PotentialTransition.TransitionRule->TransitionIndex); //-V595

			// If transition is valid and not waiting on other conditions
   // 如果转换有效并且不等待其他条件
			// and we're not doing a transition to self, unless the self-transition can inertialize
   // 我们不会进行向自我的转变，除非自我转变可以惯性化
			if (PotentialTransition.TargetState != CurrentState || ReferenceTransition.bAllowInertializationForSelfTransitions)
			{
				return true;
			}

			return false;
		};

		if (CanTransition())
		{
			bFoundValidTransition = true;

			const FAnimationTransitionBetweenStates& ReferenceTransition = GetTransitionInfo(PotentialTransition.TransitionRule->TransitionIndex); //-V595
			TransitionToState(Context, ReferenceTransition, &PotentialTransition);

			TransitionCountThisFrame++;
		}
	}
	while (bFoundValidTransition && (TransitionCountThisFrame < MaxTransitionsPerFrame));
	
	if (bFirstUpdate)
	{
		if (bSkipFirstUpdateTransition)
		{
			// in the first update, we don't like to transition from entry state
   // 在第一次更新中，我们不喜欢从入口状态转换
			// so we throw out any transition data at the first update
   // 所以我们在第一次更新时丢弃所有转换数据
			ActiveTransitionArray.Reset();
		}
		bFirstUpdate = false;
	}

	StatesUpdated.Reset();

	bool bLastActiveTransitionRequestedInertialization = false;
	
	// Tick the individual state/states that are active
 // 勾选单个/多个处于活动状态的状态
	if (ActiveTransitionArray.Num() > 0)
	{
		// Keep track of states that have blended out to avoid recalling their anim node state functions.
  // 跟踪已混合的状态，以避免调用其动画节点状态函数。
		TSet<int32, DefaultKeyFuncs<int32>, TInlineSetAllocator<4>> BlendedOutStates;
		TArray<bool, TInlineAllocator<4>> FinishedTransitions;
		FinishedTransitions.Init(false, ActiveTransitionArray.Num());

		// Update the transitions prior to the states to ensure all transition alphas are correct
  // 在状态之前更新转换以确保所有转换 alpha 都是正确的
		for (int32 Index = 0; Index < ActiveTransitionArray.Num(); ++Index)
		{
			ActiveTransitionArray[Index].Update(Context, CurrentState, /*out*/ FinishedTransitions[Index]);
		}
		
		for (int32 Index = 0; Index < ActiveTransitionArray.Num(); ++Index)
		{
			// The custom graph will tick the needed states
   // 自定义图表将勾选所需的状态
			if (FinishedTransitions[Index])
			{
				// Trigger "Fully Blended Out" of any states that have any weight to them.
    // 触发任何对其有影响的状态的“完全混合”。
				{
					const int32 PreviousStateIndex = ActiveTransitionArray[Index].PreviousState;
					const bool bAttemptingToTransitionOutOfCurrentState = PreviousStateIndex == CurrentState;
					
					if (!bAttemptingToTransitionOutOfCurrentState && StatePoseLinks.IsValidIndex(PreviousStateIndex) && StatePoseLinks[PreviousStateIndex].LinkID != INDEX_NONE)
					{
						if (!BlendedOutStates.Contains(PreviousStateIndex) && FMath::IsNearlyZero(GetStateWeight(PreviousStateIndex)))
						{
							UE::Anim::FNodeFunctionCaller::CallFunction(static_cast<FAnimNode_StateResult*>(StatePoseLinks[PreviousStateIndex].GetLinkNode())->GetStateFullyBlendedOutFunction(), Context, *this);
							BlendedOutStates.Add(PreviousStateIndex);
						}
					}
				}
				
				// only play these events if it is the last transition (most recent, going to current state)
    // 仅在最后一次转换时播放这些事件（最近的，进入当前状态）
				if (Index == (ActiveTransitionArray.Num() - 1))
				{
					Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(ActiveTransitionArray[Index].EndNotify);
					Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(GetStateInfo().FullyBlendedNotify);

					// Handle events/calls during "Fully Blended In".
     // 在“完全融入”期间处理事件/呼叫。
					{
						const int32 NextStateIndex = ActiveTransitionArray[Index].NextState;
						
						if (StatePoseLinks.IsValidIndex(NextStateIndex))
						{
							// If our most recent state has fully blended in that means that all our previous states must also blend out and their transitions will be forced
       // 如果我们最近的状态已完全融合，则意味着我们之前的所有状态也必须融合，并且它们的转换将被强制
							// to complete, (removed from active transition array), therefore we need to call their fully blended out functions if they haven't been called already. 
       // 要完成（从活动转换数组中删除），因此如果尚未调用它们，我们需要调用它们完全混合的函数。
							if (ActiveTransitionArray.Num() > 1)
							{
								for (int32 BlendOutIndex = 0; BlendOutIndex < Index; ++BlendOutIndex)
								{
									const int32 PreviousStateIndex = ActiveTransitionArray[BlendOutIndex].PreviousState;
									const bool bAttemptingToTransitionOutOfCurrentState = PreviousStateIndex == CurrentState;

									if (!bAttemptingToTransitionOutOfCurrentState && StatePoseLinks.IsValidIndex(PreviousStateIndex) && StatePoseLinks[PreviousStateIndex].LinkID != INDEX_NONE && !BlendedOutStates.Contains(PreviousStateIndex))
									{
										UE::Anim::FNodeFunctionCaller::CallFunction(static_cast<FAnimNode_StateResult*>(StatePoseLinks[PreviousStateIndex].GetLinkNode())->GetStateFullyBlendedOutFunction(), Context, *this);
										BlendedOutStates.Add(PreviousStateIndex);
									}
								}
							}

							// Now that all possible state fully blended out functions have been called, perform state fully blended in call.
       // 现在，所有可能的状态完全混合的函数都已被调用，在调用中执行状态完全混合。
							if (StatePoseLinks.IsValidIndex(NextStateIndex) && StatePoseLinks[NextStateIndex].LinkID != INDEX_NONE)
							{
								UE::Anim::FNodeFunctionCaller::CallFunction(static_cast<FAnimNode_StateResult*>(StatePoseLinks[NextStateIndex].GetLinkNode())->GetStateFullyBlendedInFunction(), Context, *this);
							}
						}
					}
				}

				// we were the last active transition and used inertialization
    // 我们是最后一个主动过渡并使用了惯性化
				if (ActiveTransitionArray[Index].LogicType == ETransitionLogicType::TLT_Inertialization && (ActiveTransitionArray.Num() - 1 ==  Index))
				{
					bLastActiveTransitionRequestedInertialization = true;
				}
			}
			else
			{
				// transition is still active, so tick the required states
    // 转换仍然处于活动状态，因此勾选所需的状态
				UpdateTransitionStates(Context, ActiveTransitionArray[Index]);
			}
		}
		
		// remove finished transitions here, newer transitions ending means any older ones must complete as well
  // 删除此处已完成的过渡，较新的过渡结束意味着任何较旧的过渡也必须完成
		for (int32 Index = (ActiveTransitionArray.Num()-1); Index >= 0; --Index)
		{
			// if we find an inactive one, remove all older transitions and break out
   // 如果我们发现一个不活动的转换，请删除所有旧的转换并进行突破
			if (!ActiveTransitionArray[Index].bActive)
			{
				ActiveTransitionArray.RemoveAt(0, Index+1, EAllowShrinking::No);
				break;
			}
		}
	}

	//@TODO: StatesUpdated.Contains is a linear search
 // @TODO：StatesUpdated.Contains 是线性搜索
	// Update the only active state if there are no transitions still in flight
 // 如果没有仍在进行中的转换，则更新唯一的活动状态
	if (ActiveTransitionArray.Num() == 0 && !IsAConduitState(CurrentState) && !StatesUpdated.Contains(CurrentState))
	{
		UE::Anim::TOptionalScopedGraphMessage<UE::Anim::FAnimInertializationSyncScope> InertializationSync(bLastActiveTransitionRequestedInertialization, Context);
		UE::Anim::TOptionalScopedGraphMessage<UE::Anim::FActiveStateMachineScope> Message(bCreateNotifyMetaData, Context, Context, this, CurrentState);
		
		StatePoseLinks[CurrentState].Update(Context);
	}

	ElapsedTime += Context.GetDeltaTime();

	// Record state weights after transitions/updates are completed
 // 转换/更新完成后记录状态权重
	for (int32 StateIndex = 0; StateIndex < StatePoseLinks.Num(); ++StateIndex)
	{
		const float StateWeight = GetStateWeight(StateIndex);
		if(StateWeight > 0.0f)
		{
			const float CurrentStateElapsedTime = StateIndex == CurrentState ? GetCurrentStateElapsedTime() : 0.0f;
			Context.AnimInstanceProxy->RecordStateWeight(StateMachineIndexInClass, StateIndex, StateWeight, CurrentStateElapsedTime);

			TRACE_ANIM_STATE_MACHINE_STATE(Context, StateMachineIndexInClass, StateIndex, StateWeight, CurrentStateElapsedTime);
		}
	}

#if ANIM_TRACE_ENABLED
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Name"), GetMachineDescription()->MachineName);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Current State"), GetStateInfo().StateName);
	TRACE_ANIM_NODE_VALUE(
		Context,
		TEXT("Queued Transition Requests"), 
		*FString::JoinBy(QueuedTransitionEvents, TEXT(",\n"), [](const FTransitionEvent& TransitionEvent) { return TransitionEvent.ToDebugString(); })
	);
#if WITH_EDITORONLY_DATA
	TRACE_ANIM_NODE_VALUE(
		Context,
		TEXT("Consumed Transition Requests"),
		*FString::JoinBy(HandledTransitionEvents, TEXT(",\n"), [](const FTransitionEvent& TransitionEvent) { return TransitionEvent.EventName.ToString(); })
	);
#endif //WITH_EDITORONLY_DATA
#endif //ANIM_TRACE_ENABLED
}

float FAnimNode_StateMachine::GetRelevantAnimTimeRemaining(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const
{
	if (const FAnimNode_AssetPlayerRelevancyBase* AssetPlayer = GetRelevantAssetPlayerInterfaceFromState(InAnimInstanceProxy, GetStateInfo(StateIndex)))
	{
		if (AssetPlayer->GetAnimAsset())
		{
			return AssetPlayer->GetCurrentAssetLength() - AssetPlayer->GetCurrentAssetTimePlayRateAdjusted();
		}
	}

	return MAX_flt;
}

float FAnimNode_StateMachine::GetRelevantAnimTimeRemainingFraction(const FAnimInstanceProxy* InAnimInstanceProxy, int32 StateIndex) const
{
	if (const FAnimNode_AssetPlayerRelevancyBase* AssetPlayer = GetRelevantAssetPlayerInterfaceFromState(InAnimInstanceProxy, GetStateInfo(StateIndex)))
	{
		if (AssetPlayer->GetAnimAsset())
		{
			float Length = AssetPlayer->GetCurrentAssetLength();
			if (Length > 0.0f)
			{
				return (Length - AssetPlayer->GetCurrentAssetTimePlayRateAdjusted()) / Length;
			}
		}
	}

	return 1.f;
}

const FAnimNode_AssetPlayerRelevancyBase* FAnimNode_StateMachine::GetRelevantAssetPlayerInterfaceFromState(const FAnimInstanceProxy* InAnimInstanceProxy, const FBakedAnimationState& StateInfo) const
{
	const FAnimNode_AssetPlayerRelevancyBase* ResultPlayer = nullptr;
	float MaxWeight = 0.0f;

	auto EvaluatePlayerWeight = [&MaxWeight, &ResultPlayer](const FAnimNode_AssetPlayerRelevancyBase* Player)
	{
		if (!Player->GetIgnoreForRelevancyTest() && Player->GetCachedBlendWeight() > MaxWeight)
		{
			MaxWeight = Player->GetCachedBlendWeight();
			ResultPlayer = Player;
		}
	};

	for (const int32& PlayerIdx : StateInfo.PlayerNodeIndices)
	{
		if (const FAnimNode_AssetPlayerRelevancyBase* Player = InAnimInstanceProxy->GetNodeFromIndex<FAnimNode_AssetPlayerRelevancyBase>(PlayerIdx))
		{
			EvaluatePlayerWeight(Player);
		}
	}

	// Get all layer node indices that are part of this state
 // 获取属于该状态的所有层节点索引
	for (const int32& LinkedAnimNodeIdx : StateInfo.LayerNodeIndices)
	{
		// Try and retrieve the actual node object
  // 尝试检索实际的节点对象
		if (const FAnimNode_LinkedAnimGraph* LinkedAnimGraph = InAnimInstanceProxy->GetNodeFromIndex<FAnimNode_LinkedAnimGraph>(LinkedAnimNodeIdx))
		{
			// Retrieve the AnimInstance running for this linked anim graph/layer
   // 检索为此链接的动画图/图层运行的 AnimInstance
			if (const UAnimInstance* CurrentTarget = LinkedAnimGraph->GetTargetInstance<UAnimInstance>())
			{
				FName GraphName;
				if (const FAnimNode_LinkedAnimLayer* LinkedAnimLayer = InAnimInstanceProxy->GetNodeFromIndex<FAnimNode_LinkedAnimLayer>(LinkedAnimNodeIdx))
				{
					GraphName = LinkedAnimLayer->Layer;
				}
				else
				{
					GraphName = DefaultAnimGraphName;
				}

				// Retrieve all asset player nodes from the corresponding Anim blueprint class and apply same logic to find highest weighted asset player 
    // 从相应的 Anim 蓝图类中检索所有资产播放器节点，并应用相同的逻辑来查找权重最高的资产播放器
				TArray<const FAnimNode_AssetPlayerRelevancyBase*> PlayerNodesInLayer = CurrentTarget->GetInstanceRelevantAssetPlayers(GraphName);
				for (const FAnimNode_AssetPlayerRelevancyBase* Player : PlayerNodesInLayer)
				{
					EvaluatePlayerWeight(Player);
				}
			}
		}
	}
	return ResultPlayer;
}

bool FAnimNode_StateMachine::FindValidTransition(const FAnimationUpdateContext& Context, const FBakedAnimationState& StateInfo, /*out*/ FAnimationPotentialTransition& OutPotentialTransition, /*out*/ TArray<int32, TInlineAllocator<4>>& OutVisitedStateIndices)
{
	// There is a possibility we'll revisit states connected through conduits,
 // 我们有可能会重新审视通过管道连接的州，
	// so we can avoid doing unnecessary work (and infinite loops) by caching off states we have already checked
 // 因此我们可以通过缓存我们已经检查过的状态来避免做不必要的工作（和无限循环）
	const int32 CheckingStateIndex = GetStateIndex(StateInfo);
	if (OutVisitedStateIndices.Contains(CheckingStateIndex))
	{
		return false;
	}
	OutVisitedStateIndices.Add(CheckingStateIndex);

	const IAnimClassInterface* AnimBlueprintClass = Context.GetAnimClass();

	// Conduit 'states' have an additional entry rule which must be true to consider taking any transitions via the conduit
 // 管道“状态”有一个附加的条目规则，该规则必须正确才能考虑通过管道进行任何转换
	//@TODO: It would add flexibility to be able to define this on normal state nodes as well, assuming the dual-graph editing is sorted out
 // @TODO：假设双图编辑已解决，那么在正常状态节点上定义它也会增加灵活性
	if (FAnimNode_TransitionResult* StateEntryRuleNode = GetNodeFromPropertyIndex<FAnimNode_TransitionResult>(Context.AnimInstanceProxy->GetAnimInstanceObject(), AnimBlueprintClass, StateInfo.EntryRuleNodeIndex))
	{
		if (StateEntryRuleNode->NativeTransitionDelegate.IsBound())
		{
			// attempt to evaluate native rule
   // 尝试评估本机规则
			StateEntryRuleNode->bCanEnterTransition = StateEntryRuleNode->NativeTransitionDelegate.Execute();
		}
		else
		{
			// Execute it and see if we can take this rule
   // 执行一下看看是否能接受这个规则
			StateEntryRuleNode->GetEvaluateGraphExposedInputs().Execute(Context);
		}

		// not ok, back out
  // 不行，退出
		if (!StateEntryRuleNode->bCanEnterTransition)
		{
			return false;
		}
	}

	const double WorldTime = Context.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld()->GetTimeSeconds();

	const int32 NumTransitions = StateInfo.Transitions.Num();
	for (int32 TransitionIndex = 0; TransitionIndex < NumTransitions; ++TransitionIndex)
	{
		const FBakedStateExitTransition& TransitionRule = StateInfo.Transitions[TransitionIndex];
		if (TransitionRule.CanTakeDelegateIndex == INDEX_NONE)
		{
			continue;
		}

		const FAnimationTransitionBetweenStates& TransitionInfo = GetTransitionInfo(TransitionRule.TransitionIndex);
		const int32 NextState = TransitionInfo.NextState;
		const FBakedAnimationState& NextStateInfo = GetStateInfo(NextState);

		// Check transition de-bounce time
  // 检查转换去抖时间
		if (TransitionInfo.MinTimeBeforeReentry >= 0.0f && (WorldTime - LastStateEntryTime[NextState]) < TransitionInfo.MinTimeBeforeReentry)
		{
			continue;
		}

		FAnimNode_TransitionResult* ResultNode = GetNodeFromPropertyIndex<FAnimNode_TransitionResult>(Context.AnimInstanceProxy->GetAnimInstanceObject(), AnimBlueprintClass, TransitionRule.CanTakeDelegateIndex);
		float CrossfadeTimeAdjustment = 0.f;

		// If we require a valid sync group, check that first.
  // 如果我们需要有效的同步组，请先检查它。
		if ((TransitionRule.SyncGroupNameToRequireValidMarkersRule != NAME_None)
			&& !Context.AnimInstanceProxy->IsSyncGroupValid(TransitionRule.SyncGroupNameToRequireValidMarkersRule))
		{
			ResultNode->bCanEnterTransition = false;
		}
		else if (ResultNode->NativeTransitionDelegate.IsBound())
		{
			// attempt to evaluate native rule
   // 尝试评估本机规则
			ResultNode->bCanEnterTransition = ResultNode->NativeTransitionDelegate.Execute();
		}
		else if (TransitionRule.bAutomaticRemainingTimeRule)
		{
			bool bCanEnterTransition = false;
			if (const FAnimNode_AssetPlayerRelevancyBase* RelevantPlayer = GetRelevantAssetPlayerInterfaceFromState(Context, StateInfo))
			{
				if (const UAnimationAsset* AnimAsset = RelevantPlayer->GetAnimAsset())
				{
					float AnimTimeRemaining = AnimAsset->GetPlayLength() - RelevantPlayer->GetAccumulatedTime();

					// Check whether the player looped last frame
     // 检查播放器是否循环播放最后一帧
					if (RelevantPlayer->IsLooping() && AnimTimeRemaining > 0.f)
					{
						if (const FDeltaTimeRecord* DeltaTimeRecord = RelevantPlayer->GetDeltaTimeRecord())
						{
							if (DeltaTimeRecord->IsPreviousValid())
							{
								const float DeltaTimeFromPlayer = RelevantPlayer->GetAccumulatedTime() - DeltaTimeRecord->GetPrevious();
								const float DeltaTimeFromRecord = DeltaTimeRecord->Delta;

								// deal with negative play rates
        // 处理负播放率
								if (DeltaTimeFromPlayer * DeltaTimeFromRecord < 0.f)
								{
									AnimTimeRemaining = 0.f;
								}
							}
						}
					}

					// For transitions that go to a conduit the user is not able to edit the transition's cross fade duration,
     // 对于进入管道的过渡，用户无法编辑过渡的交叉淡入淡出持续时间，
					// therefore we force the cross fade duration to zero to ensure the transition is always triggerred upon reaching the end of the animation.
     // 因此，我们将交叉淡入淡出持续时间强制为零，以确保在到达动画结尾时始终触发过渡。
					const float CrossfadeDuration = GetStateInfo(TransitionInfo.NextState).bIsAConduit ? 0.0f : TransitionInfo.CrossfadeDuration;

					// Allow for used to determine the automatic transition trigger time or fallback to using cross fade duration.
     // 允许用于确定自动转换触发时间或回退到使用交叉淡入淡出持续时间。
					const float TransitionTriggerTime = (TransitionRule.AutomaticRuleTriggerTime >= 0.0f) ? TransitionRule.AutomaticRuleTriggerTime : CrossfadeDuration;
					CrossfadeTimeAdjustment = TransitionTriggerTime - AnimTimeRemaining;

					// Trigger transition only if we have "CrossfadeTimeAdjustment" seconds left before reaching animation end boundary.
     // 仅当到达动画结束边界之前还剩“CrossfadeTimeAdjustment”秒时才触发过渡。
					bCanEnterTransition = (CrossfadeTimeAdjustment >= 0.f);
				}
			}
			
			ResultNode->bCanEnterTransition = bCanEnterTransition;
		}			
		else 
		{
			// Execute it and see if we can take this rule
   // 执行一下看看是否能接受这个规则
			ResultNode->GetEvaluateGraphExposedInputs().Execute(Context);
		}

		if (ResultNode->bCanEnterTransition == TransitionRule.bDesiredTransitionReturnValue)
		{
			// if next state is a conduit we want to check for transitions using that state as the root
   // 如果下一个状态是一个管道，我们要检查使用该状态作为根的转换
			if (NextStateInfo.bIsAConduit)
			{
				if (FindValidTransition(Context, NextStateInfo, /*out*/ OutPotentialTransition, /*out*/ OutVisitedStateIndices))
				{
					OutPotentialTransition.SourceTransitionIndices.Add(TransitionRule.TransitionIndex);

					return true;
				}					
			}
			// otherwise we have found a content state, so we can record our potential transition
   // 否则我们已经找到了内容状态，因此我们可以记录潜在的转换
			else
			{
				// clear out any potential transition we already have
    // 清除我们已有的任何潜在过渡
				OutPotentialTransition.Clear();

				// fill out the potential transition information
    // 填写潜在的过渡信息
				OutPotentialTransition.TransitionRule = &TransitionRule;
				OutPotentialTransition.TargetState = NextState;
				OutPotentialTransition.CrossfadeTimeAdjustment = CrossfadeTimeAdjustment;
				OutPotentialTransition.SourceTransitionIndices.Add(TransitionRule.TransitionIndex);

				return true;
			}
		}
	}

	return false;
}

void FAnimNode_StateMachine::UpdateTransitionStates(const FAnimationUpdateContext& Context, FAnimationActiveTransitionEntry& Transition)
{
	if (Transition.bActive)
	{
		switch (Transition.LogicType)
		{
		case ETransitionLogicType::TLT_StandardBlend:
			{
				// update both states
    // 更新两个状态
				{
					FAnimationUpdateContext StateContext = Context.FractionalWeight(GetStateWeight(Transition.PreviousState));
					UpdateState(Transition.PreviousState, Transition.PreviousState == CurrentState ? StateContext : StateContext.AsInactive());
				}
				{
					FAnimationUpdateContext StateContext = Context.FractionalWeight(GetStateWeight(Transition.NextState));
					UpdateState(Transition.NextState, (Transition.NextState == CurrentState) ? StateContext : StateContext.AsInactive());
				}
			}
			break;

		case ETransitionLogicType::TLT_Inertialization:
			{
				UE::Anim::TScopedGraphMessage<UE::Anim::FAnimInertializationSyncScope> InertializationSync(Context);

				// update target state
    // 更新目标状态
				UpdateState(Transition.NextState, (Transition.NextState == CurrentState) ? Context : Context.AsInactive());
			}
			break;

		case ETransitionLogicType::TLT_Custom:
			{
				if (Transition.CustomTransitionGraph.LinkID != INDEX_NONE)
				{
					Transition.CustomTransitionGraph.Update(Context);

					for (TArray<FAnimNode_TransitionPoseEvaluator*>::TIterator PoseEvaluatorListIt = Transition.PoseEvaluators.CreateIterator(); PoseEvaluatorListIt; ++PoseEvaluatorListIt)
					{
						FAnimNode_TransitionPoseEvaluator* Evaluator = *PoseEvaluatorListIt;
						if (Evaluator->InputNodeNeedsUpdate(Context))
						{
							const bool bUsePreviousState = (Evaluator->DataSource == EEvaluatorDataSource::EDS_SourcePose);
							const int32 EffectiveStateIndex = bUsePreviousState ? Transition.PreviousState : Transition.NextState;
							FAnimationUpdateContext ContextToUse = Context.FractionalWeight(bUsePreviousState ? (1.0f - Transition.Alpha) : Transition.Alpha);
							UpdateState(EffectiveStateIndex, (EffectiveStateIndex == CurrentState) ? ContextToUse : ContextToUse.AsInactive());
						}
					}
				}
			}
			break;

		default:
			break;
		}
	}
}

void FAnimNode_StateMachine::Evaluate_AnyThread(FPoseContext& Output)
{
	if (const FBakedAnimationStateMachine* Machine = GetMachineDescription())
	{
		const bool bCurrentStateIsConduit = IsAConduitState(CurrentState);
#if WITH_EDITORONLY_DATA
		if (bCurrentStateIsConduit)
		{ 
			UAnimBlueprint* AnimBlueprint = Output.AnimInstanceProxy->GetAnimBlueprint();

			FText Message = FText::Format(LOCTEXT("CurrentStateIsConduitWarning", "Current state is a conduit and will reset to ref pose. This can happen if a conduit is an entry state and there isn't at least one valid transition to take. State Machine({0}), Conduit({1}), AnimBlueprint({2})."),
				FText::FromName(Machine->MachineName), FText::FromName(GetCurrentStateName()), FText::FromString(GetPathNameSafe(AnimBlueprint)));
			Output.LogMessage(EMessageSeverity::Error, Message);
		}
#else
		ensureMsgf(!bCurrentStateIsConduit, TEXT("Current state is a conduit and will reset to ref pose. State Machine(%s), Conduit(%s), AnimInstance(%s)."),
			*Machine->MachineName.ToString(), *GetCurrentStateName().ToString(), *Output.AnimInstanceProxy->GetAnimInstanceName());
#endif

		if (bCurrentStateIsConduit || Machine->States.Num() == 0 || !Machine->States.IsValidIndex(CurrentState))
		{
			Output.Pose.ResetToRefPose();
			return;
		}
	}
	else
	{
		Output.Pose.ResetToRefPose();
		return;
	}

	ANIM_MT_SCOPE_CYCLE_COUNTER(EvaluateAnimStateMachine, !IsInGameThread());
	
	if (ActiveTransitionArray.Num() > 0)
	{
		check(Output.AnimInstanceProxy->GetSkeleton());
		
		check(StateCachedPoses.Num() == 0);
		StateCachedPoses.AddZeroed(StatePoseLinks.Num());

		//each transition stomps over the last because they will already include the output from the transition before it
  // 每个转换都会踩在最后一个转换之上，因为它们已经包含了之前转换的输出
		for (int32 Index = 0; Index < ActiveTransitionArray.Num(); ++Index)
		{
			// if there is any source pose, blend it here
   // 如果有任何源姿势，请在此处混合
			FAnimationActiveTransitionEntry& ActiveTransition = ActiveTransitionArray[Index];
			
			// when evaluating multiple transitions we need to store the pose from previous results
   // 当评估多个转换时，我们需要存储之前结果的姿势
			// so we can feed the next transitions
   // 这样我们就可以进行下一个转换
			const bool bIntermediatePoseIsValid = Index > 0;

			if (ActiveTransition.bActive)
			{
				switch (ActiveTransition.LogicType)
				{
				case ETransitionLogicType::TLT_StandardBlend:
					EvaluateTransitionStandardBlend(Output, ActiveTransition, bIntermediatePoseIsValid);
					break;
				case ETransitionLogicType::TLT_Inertialization:
					EvaluateState(ActiveTransition.NextState, Output);
					break;
				case ETransitionLogicType::TLT_Custom:
					EvaluateTransitionCustomBlend(Output, ActiveTransition, bIntermediatePoseIsValid);
					break;
				default:
					break;
				}
			}
		}

		// Ensure that all of the resulting rotations are normalized
  // 确保所有产生的旋转都已标准化
		Output.Pose.NormalizeRotations();

		// Clear our cache
  // 清除我们的缓存
		for (auto CachedPose : StateCachedPoses)
		{
			delete CachedPose;
		}
		StateCachedPoses.Reset();
	}
	else if (!IsAConduitState(CurrentState))
	{
		// Make sure CacheBones has been called before evaluating.
  // 确保在评估之前已调用 CacheBones。
		ConditionallyCacheBonesForState(CurrentState, Output);

		// Evaluate the current state
  // 评估当前状态
		StatePoseLinks[CurrentState].Evaluate(Output);
	}
}


void FAnimNode_StateMachine::EvaluateTransitionStandardBlend(FPoseContext& Output, FAnimationActiveTransitionEntry& Transition, bool bIntermediatePoseIsValid)
{
	if (bIntermediatePoseIsValid)
	{
		FPoseContext PreviousStateResult(Output); 
		PreviousStateResult = Output;
		const FPoseContext& NextStateResult = EvaluateState(Transition.NextState, Output);
		EvaluateTransitionStandardBlendInternal(Output, Transition, PreviousStateResult, NextStateResult);
	}
	else
	{
		const FPoseContext& PreviousStateResult = EvaluateState(Transition.PreviousState, Output);
		const FPoseContext& NextStateResult = EvaluateState(Transition.NextState, Output);
		EvaluateTransitionStandardBlendInternal(Output, Transition, PreviousStateResult, NextStateResult);
	}
}

void FAnimNode_StateMachine::EvaluateTransitionStandardBlendInternal(FPoseContext& Output, FAnimationActiveTransitionEntry& Transition, const FPoseContext& PreviousStateResult, const FPoseContext& NextStateResult)
{	
	// Blend it in
 // 将其混合进去
	const ScalarRegister VPreviousWeight(1.0f - Transition.Alpha);
	const ScalarRegister VWeight(Transition.Alpha);

	// If we have a blend profile we need to blend per bone.
 // 如果我们有混合配置文件，我们需要对每个骨骼进行混合。
	if (Transition.BlendProfile)
	{
		const FBoneContainer& RequiredBones = Output.AnimInstanceProxy->GetRequiredBones();
		TSharedPtr<IInterpolationIndexProvider::FPerBoneInterpolationData> Data = Transition.BlendProfile->GetPerBoneInterpolationData(Output.AnimInstanceProxy->GetSkeleton());

		// If we have some skeleton remapping and the source data comes from another skeleton.
  // 如果我们有一些骨架重新映射并且源数据来自另一个骨架。
		// This is a slightly slower path, so we made two branches, one with remapping and one without.
  // 这是一条稍微慢一些的路径，因此我们创建了两个分支，一个有重新映射，一个没有。
		const FSkeletonRemapping& Remapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(Transition.BlendProfile->GetSkeleton(), Output.AnimInstanceProxy->GetSkeleton());
		if (Remapping.IsValid())
		{
			for (const FCompactPoseBoneIndex TargetBoneIndex : Output.Pose.ForEachBoneIndex())
			{
				const FSkeletonPoseBoneIndex SourceSkelBoneIndex(Remapping.GetSourceSkeletonBoneIndex(TargetBoneIndex.GetInt()));
				const FCompactPoseBoneIndex SourceBoneIndex = FCompactPoseBoneIndex(RequiredBones.GetCompactPoseIndexFromSkeletonPoseIndex(SourceSkelBoneIndex));
				const int32 PerBoneIndex = (SourceBoneIndex != INDEX_NONE) ? Transition.BlendProfile->GetPerBoneInterpolationIndex(SourceBoneIndex, RequiredBones, Data.Get()) : INDEX_NONE;

				// Use defined per-bone scale if the bone has a scale specified in the blend profile.
    // 如果骨骼具有混合配置文件中指定的比例，则使用定义的每骨骼比例。
				const ScalarRegister FirstWeight = (PerBoneIndex != INDEX_NONE) ? ScalarRegister(Transition.StateBlendData[1].PerBoneBlendData[PerBoneIndex]) : ScalarRegister(VPreviousWeight);
				const ScalarRegister SecondWeight = (PerBoneIndex != INDEX_NONE) ? ScalarRegister(Transition.StateBlendData[0].PerBoneBlendData[PerBoneIndex]) : ScalarRegister(VWeight);
				Output.Pose[TargetBoneIndex] = PreviousStateResult.Pose[TargetBoneIndex] * FirstWeight;
				Output.Pose[TargetBoneIndex].AccumulateWithShortestRotation(NextStateResult.Pose[TargetBoneIndex], SecondWeight);
			}
		}
		else // There is no skeleton remapping or we are using the same skeleton as the source.
		{
			for (const FCompactPoseBoneIndex TargetBoneIndex : Output.Pose.ForEachBoneIndex())
			{
				const int32 PerBoneIndex = Transition.BlendProfile->GetPerBoneInterpolationIndex(TargetBoneIndex, RequiredBones, Data.Get());

				// Use defined per-bone scale if the bone has a scale specified in the blend profile.
    // 如果骨骼具有混合配置文件中指定的比例，则使用定义的每骨骼比例。
				const ScalarRegister FirstWeight = (PerBoneIndex != INDEX_NONE) ? ScalarRegister(Transition.StateBlendData[1].PerBoneBlendData[PerBoneIndex]) : ScalarRegister(VPreviousWeight);
				const ScalarRegister SecondWeight = (PerBoneIndex != INDEX_NONE) ? ScalarRegister(Transition.StateBlendData[0].PerBoneBlendData[PerBoneIndex]) : ScalarRegister(VWeight);
				Output.Pose[TargetBoneIndex] = PreviousStateResult.Pose[TargetBoneIndex] * FirstWeight;
				Output.Pose[TargetBoneIndex].AccumulateWithShortestRotation(NextStateResult.Pose[TargetBoneIndex], SecondWeight);
			}
		}
	}
	else
	{
		for(FCompactPoseBoneIndex BoneIndex : Output.Pose.ForEachBoneIndex())
		{
			Output.Pose[BoneIndex] = PreviousStateResult.Pose[BoneIndex] * VPreviousWeight;
			Output.Pose[BoneIndex].AccumulateWithShortestRotation(NextStateResult.Pose[BoneIndex], VWeight);
		}
	}
	
	// blend curve in
 // 混合曲线
	Output.Curve.Override(PreviousStateResult.Curve, 1.0 - Transition.Alpha);
	Output.Curve.Accumulate(NextStateResult.Curve, Transition.Alpha);
		
	// reset attributes container as it can contain a previously handled active transition (and attributes expects blending into an empty container)
 // 重置属性容器，因为它可以包含先前处理的活动过渡（并且属性期望混合到空容器中）
	Output.CustomAttributes.Empty();
	UE::Anim::Attributes::BlendAttributes({ PreviousStateResult.CustomAttributes, NextStateResult.CustomAttributes }, { 1.0f - Transition.Alpha, Transition.Alpha }, { 0, 1 }, Output.CustomAttributes);
}

void FAnimNode_StateMachine::EvaluateTransitionCustomBlend(FPoseContext& Output, FAnimationActiveTransitionEntry& Transition, bool bIntermediatePoseIsValid)
{
	if (Transition.CustomTransitionGraph.LinkID != INDEX_NONE)
	{
		for (TArray<FAnimNode_TransitionPoseEvaluator*>::TIterator PoseEvaluatorListIt(Transition.PoseEvaluators); PoseEvaluatorListIt; ++PoseEvaluatorListIt)
		{
			FAnimNode_TransitionPoseEvaluator* Evaluator = *PoseEvaluatorListIt;
			if (Evaluator->InputNodeNeedsEvaluate())
			{
				// All input evaluators that use the intermediate pose can grab it from the current output.
    // 所有使用中间姿势的输入评估器都可以从当前输出中获取它。
				const bool bUseIntermediatePose = bIntermediatePoseIsValid && (Evaluator->DataSource == EEvaluatorDataSource::EDS_SourcePose);

				// otherwise we need to evaluate the nodes they reference
    // 否则我们需要评估它们引用的节点
				if (!bUseIntermediatePose)
				{
					const bool bUsePreviousState = (Evaluator->DataSource == EEvaluatorDataSource::EDS_SourcePose);
					const int32 EffectiveStateIndex = bUsePreviousState ? Transition.PreviousState : Transition.NextState;
					const FPoseContext& PoseEvalResult = EvaluateState(EffectiveStateIndex, Output);

					// push transform to node.
     // 将变换推送到节点。
					Evaluator->CachePose(PoseEvalResult);
				}
				else
				{
					// push transform to node.
     // 将变换推送到节点。
					Evaluator->CachePose(Output);
				}
			}
		}

		FPoseContext StatePoseResult(Output);
		Transition.CustomTransitionGraph.Evaluate(StatePoseResult);

		// First pose will just overwrite the destination
  // 第一个姿势只会覆盖目的地
		for (const FCompactPoseBoneIndex BoneIndex : Output.Pose.ForEachBoneIndex())
		{
			Output.Pose[BoneIndex] = StatePoseResult.Pose[BoneIndex];
		}

		// Copy curve over also, replacing current.
  // 也复制曲线，替换当前曲线。
		Output.Curve.CopyFrom(StatePoseResult.Curve);

		// Copy attributes over, replacing current.
  // 复制属性，替换当前属性。
		Output.CustomAttributes.CopyFrom(StatePoseResult.CustomAttributes);
	}
}

void FAnimNode_StateMachine::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("(%s->%s)"), *GetMachineDescription()->MachineName.ToString(), *GetStateInfo().StateName.ToString());

	DebugData.AddDebugItem(DebugLine);
	for (int32 PoseIndex = 0; PoseIndex < StatePoseLinks.Num(); ++PoseIndex)
	{
		FString StateName = FString::Printf(TEXT("(State: %s)"), *GetStateInfo(PoseIndex).StateName.ToString());
		StatePoseLinks[PoseIndex].GatherDebugData(DebugData.BranchFlow(GetStateWeight(PoseIndex), StateName));
	}
}

void FAnimNode_StateMachine::SetStateInternal(int32 NewStateIndex)
{
	checkSlow(PRIVATE_MachineDescription);
	ensure(!IsAConduitState(NewStateIndex) || bAllowConduitEntryStates);
	CurrentState = FMath::Clamp<int32>(NewStateIndex, 0, PRIVATE_MachineDescription->States.Num() - 1);
	check(CurrentState == NewStateIndex);
	ElapsedTime = 0.0f;
}

void FAnimNode_StateMachine::SetState(const FAnimationBaseContext& Context, int32 NewStateIndex, bool bAllowReEntry)
{
	SCOPE_CYCLE_COUNTER(Stat_StateMachineSetState);

	if (NewStateIndex != CurrentState || bAllowReEntry)
	{
		const int32 PrevStateIndex = CurrentState;
		if(CurrentState != INDEX_NONE && CurrentState < OnGraphStatesExited.Num())
		{
			OnGraphStatesExited[CurrentState].ExecuteIfBound(*this, CurrentState, NewStateIndex);
		}

		if (StatePoseLinks.IsValidIndex(CurrentState) && StatePoseLinks[CurrentState].LinkID != INDEX_NONE)
		{
			UE::Anim::FNodeFunctionCaller::CallFunction(static_cast<FAnimNode_StateResult*>(StatePoseLinks[CurrentState].GetLinkNode())->GetStateExitFunction(), Context, *this);
		}

		bool bForceReset = false;

		if(PRIVATE_MachineDescription->States.IsValidIndex(NewStateIndex))
		{
			const FBakedAnimationState& BakedCurrentState = PRIVATE_MachineDescription->States[NewStateIndex];
			bForceReset = BakedCurrentState.bAlwaysResetOnEntry;
		}

		// Determine if the new state is active or not
  // 确定新状态是否处于活动状态
		const bool bAlreadyActive = GetStateWeight(NewStateIndex) > 0.0f;

		SetStateInternal(NewStateIndex);

		// Record entry time for potential de-bouncing
  // 记录潜在去抖的进入时间
		LastStateEntryTime[CurrentState] = Context.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld()->GetTimeSeconds();

		// Clear any currently cached blend weights for asset player nodes.
  // 清除资产播放器节点当前缓存的所有混合权重。
		// This stops any zero length blends holding on to old weights
  // 这会阻止任何零长度混合保留旧权重
		for(int32 PlayerIndex : GetStateInfo(CurrentState).PlayerNodeIndices)
		{
			if(FAnimNode_AssetPlayerRelevancyBase* Player = Context.AnimInstanceProxy->GetMutableNodeFromIndex<FAnimNode_AssetPlayerRelevancyBase>(PlayerIndex))
			{
				Player->ClearCachedBlendWeight();
			}
		}

		// Clear any currently cached blend weights for asset player nodes in layers.
  // 清除图层中资产播放器节点的所有当前缓存的混合权重。
		for (const int32& LinkedAnimNodeIdx : GetStateInfo(CurrentState).LayerNodeIndices)
		{
			// Try and retrieve the actual node object
   // 尝试检索实际的节点对象
			if (FAnimNode_LinkedAnimGraph* LinkedAnimGraph = Context.AnimInstanceProxy->GetMutableNodeFromIndex<FAnimNode_LinkedAnimGraph>(LinkedAnimNodeIdx))
			{
				// Retrieve the AnimInstance running for this layer
    // 检索该层运行的 AnimInstance
				if (UAnimInstance* CurrentTarget = LinkedAnimGraph->GetTargetInstance<UAnimInstance>())
				{
					FName GraphName;
					if (const FAnimNode_LinkedAnimLayer* LinkedAnimLayer = Context.AnimInstanceProxy->GetNodeFromIndex<FAnimNode_LinkedAnimLayer>(LinkedAnimNodeIdx))
					{
						GraphName = LinkedAnimLayer->Layer;
					}
					else
					{
						GraphName = DefaultAnimGraphName;
					}

					// Retrieve all asset player nodes from the corresponding Anim blueprint class and clear their cached blend weight
     // 从相应的动画蓝图类中检索所有资产播放器节点并清除其缓存的混合权重
					TArray<FAnimNode_AssetPlayerRelevancyBase*> PlayerNodesInLayer = CurrentTarget->GetMutableInstanceRelevantAssetPlayers(GraphName);
					for (FAnimNode_AssetPlayerRelevancyBase* Player : PlayerNodesInLayer)
					{
						Player->ClearCachedBlendWeight();
					}
				}
			}
		}

		if ((!bAlreadyActive || bForceReset) && !IsAConduitState(NewStateIndex))
		{
			// Initialize the new state since it's not part of an active transition (and thus not still initialized)
   // 初始化新状态，因为它不是活动转换的一部分（因此尚未初始化）
			FAnimationInitializeContext InitContext(Context.AnimInstanceProxy, Context.SharedContext);
			StatePoseLinks[NewStateIndex].Initialize(InitContext);

			// Also call cache bones if needed
   // 如果需要的话还可以调用缓存骨骼
			// Note dont call CacheBones here if we are in the process of whole-graph initialization as a 'never updated' counter
   // 请注意，如果我们正在将整个图初始化为“从未更新”计数器，则不要在此处调用 CacheBones
			// will not perform its 'minimal update guard' duty and every call will end up getting though, performing duplicate work
   // 不会执行其“最小更新保护”职责，并且每次调用最终都会被执行，执行重复的工作
			// over Save/UseCachedPose boundaries etc.
   // 超过 Save/UseCachedPose 边界等。
			// This is OK because CacheBones is actually called before updating the graph anyway after whole-graph initialization
   // 这是可以的，因为 CacheBones 实际上是在整个图初始化之后更新图之前调用的
			if(Context.AnimInstanceProxy->GetCachedBonesCounter().HasEverBeenUpdated())
			{
				ConditionallyCacheBonesForState(NewStateIndex, Context);
			}
		}

		if (StatePoseLinks.IsValidIndex(CurrentState) && StatePoseLinks[CurrentState].LinkID != INDEX_NONE)
		{
			UE::Anim::FNodeFunctionCaller::CallFunction(static_cast<FAnimNode_StateResult*>(StatePoseLinks[CurrentState].GetLinkNode())->GetStateEntryFunction(), Context, *this);
		}

		if(CurrentState != INDEX_NONE && CurrentState < OnGraphStatesEntered.Num())
		{
			OnGraphStatesEntered[CurrentState].ExecuteIfBound(*this, PrevStateIndex, CurrentState);
		}
	}
}

void FAnimNode_StateMachine::TransitionToState(const FAnimationUpdateContext& Context, const FAnimationTransitionBetweenStates& TransitionInfo, const FAnimationPotentialTransition* BakedTransitionInfo)
{
	// let the latest transition know it has been interrupted
 // 让最新的转换知道它已被中断
	if ((ActiveTransitionArray.Num() > 0) && ActiveTransitionArray[ActiveTransitionArray.Num() - 1].bActive)
	{
		Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(ActiveTransitionArray[ActiveTransitionArray.Num() - 1].InterruptNotify);
	}

	const int32 PreviousState = CurrentState;
	const int32 NextState = TransitionInfo.NextState;

	// Fire off Notifies for state transition
 // 关闭状态转换通知
	Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(GetStateInfo(PreviousState).EndNotify);
	Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(GetStateInfo(NextState).StartNotify);

	FAnimationActiveTransitionEntry* PreviousTransitionForNextState = nullptr;
	for (int32 i = ActiveTransitionArray.Num() - 1; i >= 0; --i)
	{
		FAnimationActiveTransitionEntry& TransitionEntry = ActiveTransitionArray[i];
		if (TransitionEntry.PreviousState == NextState)
		{
			PreviousTransitionForNextState = &TransitionEntry;
			break;
		}
	}

	// Don't add a transition if the previous state is a conduit (likely means we're finding the best entry state)
 // 如果前一个状态是管道，则不要添加转换（可能意味着我们正在找到最佳的进入状态）
	if (!IsAConduitState(PreviousState))
	{

		// If we have baked transition rule info available, use it
  // 如果我们有可用的烘焙转换规则信息，请使用它
		float CrossFadeTimeAdjustment = 0.0f;
		const FBakedStateExitTransition* BakedExitTransition = nullptr;
		TArray<int32, TInlineAllocator<3>> SourceTransitionIndices;
		if (BakedTransitionInfo)
		{
			CrossFadeTimeAdjustment = BakedTransitionInfo->CrossfadeTimeAdjustment;
			BakedExitTransition = BakedTransitionInfo->TransitionRule;
			SourceTransitionIndices = BakedTransitionInfo->SourceTransitionIndices;
		}

		// Get the current weight of the next state, which may be non-zero
  // 获取下一个状态的当前权重，可能非零
		const float ExistingWeightOfNextState = GetStateWeight(NextState);

		// Push the transition onto the stack
  // 将转换推入堆栈
		FAnimationActiveTransitionEntry& NewTransition = ActiveTransitionArray.Emplace_GetRef(NextState, ExistingWeightOfNextState, PreviousState, TransitionInfo, CrossFadeTimeAdjustment);
		if ((TransitionInfo.LogicType == ETransitionLogicType::TLT_Custom) && BakedExitTransition)
		{
			NewTransition.InitializeCustomGraphLinks(Context, *BakedExitTransition);
		}

		// Initialize blend data if necessary
  // 如有必要，初始化混合数据
		if (NewTransition.BlendProfile)
		{
			NewTransition.StateBlendData.AddZeroed(2);
			NewTransition.StateBlendData[0].PerBoneBlendData.AddZeroed(NewTransition.BlendProfile->GetNumBlendEntries());
			NewTransition.StateBlendData[1].PerBoneBlendData.AddZeroed(NewTransition.BlendProfile->GetNumBlendEntries());
		}

		auto ShouldInertialize = [MachineDescription = PRIVATE_MachineDescription, NextState, &TransitionInfo, &ExistingWeightOfNextState]()
		{
			// We are set to inertialize
   // 我们准备惯性化
			if (TransitionInfo.LogicType == ETransitionLogicType::TLT_Inertialization)
			{
				return true;
			}
			// Fall back if we are allowed to and the next state has weight, to hide a pop if the state resets
   // 如果我们被允许并且下一个状态具有权重，则回退，以便在状态重置时隐藏弹出窗口
			else if (TransitionInfo.bAllowInertializationForSelfTransitions && ExistingWeightOfNextState > 0.0f)
			{
				const FBakedAnimationState& BakedCurrentState = MachineDescription->States[NextState];
				// If the target state will not reset, then there's no need to inertialize.
    // 如果目标状态不会重置，则无需惯性化。
				if (BakedCurrentState.bAlwaysResetOnEntry)
				{
					return true;
				}
			}
			return false;
		};

		if (ShouldInertialize())
		{
			UE::Anim::IInertializationRequester* InertializationRequester = Context.GetMessage<UE::Anim::IInertializationRequester>();
			if (InertializationRequester)
			{
				FInertializationRequest Request;
				Request.Duration = TransitionInfo.CrossfadeDuration;
				Request.BlendProfile = TransitionInfo.BlendProfile;
				Request.bUseBlendMode = true;
				Request.BlendMode = TransitionInfo.BlendMode;
				Request.CustomBlendCurve = TransitionInfo.CustomCurve;
#if ANIM_TRACE_ENABLED
				Request.DescriptionString = FText::Format(LOCTEXT("InertializationRequestDescription", 
					"\"{0}\" Transition from \"{1}\" to \"{2}\""), 
					FText::FromName(GetMachineDescription()->MachineName),
					FText::FromName(GetStateInfo(TransitionInfo.PreviousState).StateName),
					FText::FromName(GetStateInfo(TransitionInfo.NextState).StateName)).ToString();
				Request.NodeId = Context.GetCurrentNodeId();
				Request.AnimInstance = Context.AnimInstanceProxy->GetAnimInstanceObject();
#endif

				InertializationRequester->RequestInertialization(Request);
				InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
			}
			else
			{
				LogInertializationRequestError(Context, PreviousState, NextState);
			}
		}

		NewTransition.SourceTransitionIndices = SourceTransitionIndices;

		if (!bFirstUpdate || (bFirstUpdate && !bSkipFirstUpdateTransition))
		{
			Context.AnimInstanceProxy->AddAnimNotifyFromGeneratedClass(NewTransition.StartNotify);
		}
	}

	ConsumeMarkedTransitionEvents();
	SetState(Context, NextState, TransitionInfo.bAllowInertializationForSelfTransitions);
}

float FAnimNode_StateMachine::GetStateWeight(int32 StateIndex) const
{
	const int32 NumTransitions = ActiveTransitionArray.Num();
	if (NumTransitions > 0)
	{
		// Determine the overall weight of the state here.
  // 在此确定该州的总体权重。
		float TotalWeight = 0.0f;
		for (int32 Index = 0; Index < NumTransitions; ++Index)
		{
			const FAnimationActiveTransitionEntry& Transition = ActiveTransitionArray[Index];

			float SourceWeight = (1.0f - Transition.Alpha);

			// After the first transition, so source weight is the fraction of how much all previous transitions contribute to the final weight.
   // 在第一次转换之后，源权重是所有先前转换对最终权重的贡献的分数。
			// So if our second transition is 50% complete, and our target state was 80% of the first transition, then that number will be multiplied by this weight
   // 因此，如果我们的第二次转换完成了 50%，并且我们的目标状态是第一次转换的 80%，那么该数字将乘以该权重
			if (Index > 0)
			{
				TotalWeight *= SourceWeight;
			}
			//during the first transition the source weight represents the actual state weight
   // 在第一次转换期间，源权重代表实际状态权重
			else if (Transition.PreviousState == StateIndex)
			{
				TotalWeight += SourceWeight;
			}

			// The next state weight is the alpha of this transition. We always just add the value, it will be reduced down if there are any newer transitions
   // 下一个状态权重是该转换的 alpha。我们总是只是添加值，如果有任何更新的转换，它会减少
			if (Transition.NextState == StateIndex)
			{
				TotalWeight += Transition.Alpha;
			}

		}

		return FMath::Clamp<float>(TotalWeight, 0.0f, 1.0f);
	}
	else
	{
		return (StateIndex == CurrentState) ? 1.0f : 0.0f;
	}
}

bool FAnimNode_StateMachine::IsTransitionActive(int32 TransIndex) const
{
	for (int32 Index = 0; Index < ActiveTransitionArray.Num(); ++Index)
	{
		if (ActiveTransitionArray[Index].SourceTransitionIndices.Contains(TransIndex))
		{
			return true;
		}
	}

	return false;
}

bool FAnimNode_StateMachine::RequestTransitionEvent(const FTransitionEvent& InTransitionEvent)
{
	if (!InTransitionEvent.IsValidRequest() || MaxTransitionsRequests <= 0)
	{
		return false;
	}
	
	const int32 ExistingEventIndex = QueuedTransitionEvents.IndexOfByPredicate([InTransitionEvent](const FTransitionEvent& Transition)
	{
		return Transition.EventName.IsEqual(InTransitionEvent.EventName);
	});

	if (ExistingEventIndex == INDEX_NONE || InTransitionEvent.OverwriteMode == ETransitionRequestOverwriteMode::Append)
	{
		UE_LOG(LogAnimTransitionRequests, Verbose, TEXT("Creating new '%s' request (Machine: %s)"), *InTransitionEvent.EventName.ToString(), *GetMachineDescription()->MachineName.ToString());

		ensure(QueuedTransitionEvents.Num() <= MaxTransitionsRequests);
		if (QueuedTransitionEvents.Num() == MaxTransitionsRequests)
		{
			UE_LOG(LogAnimTransitionRequests, Warning, TEXT("Transition request cap reached, dropping old requests (Machine: %s)"), *InTransitionEvent.EventName.ToString(), *GetMachineDescription()->MachineName.ToString());
			QueuedTransitionEvents.Pop(EAllowShrinking::No);
		}
		QueuedTransitionEvents.Insert(InTransitionEvent, 0);

		return true;
	}
	else if (InTransitionEvent.OverwriteMode == ETransitionRequestOverwriteMode::Ignore)
	{
		UE_LOG(LogAnimTransitionRequests, Verbose, TEXT("Ignoring new '%s' request (Machine %s)"), *InTransitionEvent.EventName.ToString(), *GetMachineDescription()->MachineName.ToString());
		return false;
	}
	else if (InTransitionEvent.OverwriteMode == ETransitionRequestOverwriteMode::Overwrite)
	{
		UE_LOG(LogAnimTransitionRequests, Verbose, TEXT("Overwriting '%s' request (Machine %s)"), *InTransitionEvent.EventName.ToString(), *GetMachineDescription()->MachineName.ToString());
		QueuedTransitionEvents[ExistingEventIndex] = InTransitionEvent;
		return true;
	}
	else
	{
		ensure(false);
	}

	return false;
}

void FAnimNode_StateMachine::ClearTransitionEvents(const FName& EventName)
{
	for (int32 RequestIndex = QueuedTransitionEvents.Num() - 1; RequestIndex >= 0; --RequestIndex)
	{
		if (QueuedTransitionEvents[RequestIndex].EventName.IsEqual(EventName))
		{
			UE_LOG(LogAnimTransitionRequests, Verbose, TEXT("Clearing '%s' request (Machine %s)"), *EventName.ToString(), *GetMachineDescription()->MachineName.ToString());
			QueuedTransitionEvents.RemoveAt(RequestIndex, EAllowShrinking::No);
		}
	}
	QueuedTransitionEvents.Shrink();
}

void FAnimNode_StateMachine::ClearAllTransitionEvents()
{
	UE_LOG(LogAnimTransitionRequests, Verbose, TEXT("Clearing all request (Machine %s)"), *GetMachineDescription()->MachineName.ToString());
	QueuedTransitionEvents.Reset();
}

bool FAnimNode_StateMachine::QueryTransitionEvent(const int32 TransitionIndex, const FName& EventName) const
{
	// Assumes QueuedTransitionEvents is sorted by request creation time, i.e. index 0 is newest request, and that
 // 假设 QueuedTransitionEvents 按请求创建时间排序，即索引 0 是最新请求，并且
	// ContainsByPredicate returns the first (newest) request
 // ContainsByPredicate 返回第一个（最新）请求
	return QueuedTransitionEvents.ContainsByPredicate([EventName, TransitionIndex](const FTransitionEvent& Transition)
	{
		return Transition.EventName.IsEqual(EventName) && !Transition.ConsumedTransitions.Contains(TransitionIndex);
	});
}

bool FAnimNode_StateMachine::QueryAndMarkTransitionEvent(const int32 TransitionIndex, const FName& EventName)
{
	// Assumes QueuedTransitionEvents is sorted by request creation time, i.e. index 0 is newest request, and that
 // 假设 QueuedTransitionEvents 按请求创建时间排序，即索引 0 是最新请求，并且
	// IndexOfByPredicate returns the first (newest) request
 // IndexOfByPredicate 返回第一个（最新）请求
	const int32 EventIndex = QueuedTransitionEvents.IndexOfByPredicate([EventName, TransitionIndex](const FTransitionEvent& Transition)
	{
		return Transition.EventName.IsEqual(EventName) && !Transition.ConsumedTransitions.Contains(TransitionIndex);
	});

	if (EventIndex != INDEX_NONE)
	{
		UE_LOG(LogAnimTransitionRequests, Verbose, TEXT("Marking '%s' (Machine: %s, Transition: %d)"), *EventName.ToString(), *GetMachineDescription()->MachineName.ToString(), TransitionIndex);
		ensure(!QueuedTransitionEvents[EventIndex].ConsumedTransitions.Contains(TransitionIndex));
		QueuedTransitionEvents[EventIndex].ConsumedTransitions.Add(TransitionIndex);
		return true;
	}
	return false;
}

void FAnimNode_StateMachine::ConsumeMarkedTransitionEvents()
{
	for (int32 RequestIndex = QueuedTransitionEvents.Num() - 1; RequestIndex >= 0; --RequestIndex)
	{
		if (QueuedTransitionEvents[RequestIndex].ToBeConsumed())
		{
			UE_LOG(LogAnimTransitionRequests, Verbose, TEXT("Consuming '%s' (Machine: %s)"), *QueuedTransitionEvents[RequestIndex].EventName.ToString(), *GetMachineDescription()->MachineName.ToString());
#if WITH_EDITORONLY_DATA
			HandledTransitionEvents.Add(QueuedTransitionEvents[RequestIndex]);
#endif
			QueuedTransitionEvents.RemoveAt(RequestIndex, EAllowShrinking::No);
		}
	}
	QueuedTransitionEvents.Shrink();
}

void FAnimNode_StateMachine::UpdateState(int32 StateIndex, const FAnimationUpdateContext& Context)
{
	if ((StateIndex != INDEX_NONE) && !StatesUpdated.Contains(StateIndex) && !IsAConduitState(StateIndex))
	{
		UE::Anim::TOptionalScopedGraphMessage<UE::Anim::FActiveStateMachineScope> Message(bCreateNotifyMetaData, Context, Context, this, StateIndex);
		StatesUpdated.Add(StateIndex);
		StatePoseLinks[StateIndex].Update(Context);
	}
}


const FPoseContext& FAnimNode_StateMachine::EvaluateState(int32 StateIndex, const FPoseContext& Context)
{
	check(StateCachedPoses.Num() == StatePoseLinks.Num());

	FPoseContext* CachePosePtr = StateCachedPoses[StateIndex];
	if (CachePosePtr == nullptr)
	{
		CachePosePtr = new FPoseContext(Context);
		check(CachePosePtr);
		StateCachedPoses[StateIndex] = CachePosePtr;

		if (!IsAConduitState(StateIndex))
		{
			// Make sure CacheBones has been called before evaluating.
   // 确保在评估之前已调用 CacheBones。
			ConditionallyCacheBonesForState(StateIndex, Context);

			StatePoseLinks[StateIndex].Evaluate(*CachePosePtr);
		}
	}

	return *CachePosePtr;
}

bool FAnimNode_StateMachine::IsAConduitState(int32 StateIndex) const
{
	return ((PRIVATE_MachineDescription != nullptr) && (PRIVATE_MachineDescription->States.IsValidIndex(StateIndex))) ? GetStateInfo(StateIndex).bIsAConduit : false;
}

bool FAnimNode_StateMachine::IsValidTransitionIndex(int32 TransitionIndex) const
{
	return PRIVATE_MachineDescription->Transitions.IsValidIndex(TransitionIndex);
}

FName FAnimNode_StateMachine::GetCurrentStateName() const
{
	if(PRIVATE_MachineDescription->States.IsValidIndex(CurrentState))
	{
		return GetStateInfo().StateName;
	}
	return NAME_None;
}

void FAnimNode_StateMachine::CacheMachineDescription(IAnimClassInterface* AnimBlueprintClass)
{
	PRIVATE_MachineDescription = AnimBlueprintClass->GetBakedStateMachines().IsValidIndex(StateMachineIndexInClass) ? &(AnimBlueprintClass->GetBakedStateMachines()[StateMachineIndexInClass]) : nullptr;
}

#undef LOCTEXT_NAMESPACE
