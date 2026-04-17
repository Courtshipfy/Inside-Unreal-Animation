// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimInstance.cpp: Anim Instance implementation
=============================================================================*/

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTrace.h"
#include "GameFramework/Pawn.h"
#include "Animation/AnimStats.h"
#include "UObject/Package.h"
#include "Engine/Engine.h"
#include "AnimationUtils.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimMontageEvaluationState.h"
#include "DisplayDebugHelpers.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Engine/Canvas.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_StateMachine.h"
#include "SkeletalRenderPublic.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/AnimNode_LinkedInputPose.h"
#include "Animation/AnimNode_LinkedAnimLayer.h"
#include "Animation/AnimSubsystem_Tag.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNode_SaveCachedPose.h"
#include "Animation/AnimSubsystem_SharedLinkedAnimLayers.h"
#include "ObjectTrace.h"
#if WITH_EDITOR
#include "Engine/Blueprint.h"
#include "BlueprintEditorSettings.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimInstance)

#if WITH_EDITOR
#include "Animation/DebugSkelMeshComponent.h"
#endif
/** Anim stats */
/** 动画统计 */

DEFINE_STAT(STAT_CalcSkelMeshBounds);
DEFINE_STAT(STAT_MeshObjectUpdate);
DEFINE_STAT(STAT_BlendInPhysics);
DEFINE_STAT(STAT_SkelCompUpdateTransform);
//                         -->  Physics Engine here <--
//                         --> 这里是物理引擎<--
DEFINE_STAT(STAT_UpdateRBBones);
DEFINE_STAT(STAT_UpdateRBJoints);
DEFINE_STAT(STAT_FinalizeAnimationUpdate);
DEFINE_STAT(STAT_GetAnimationPose);
DEFINE_STAT(STAT_AnimTriggerAnimNotifies);
DEFINE_STAT(STAT_RefreshBoneTransforms);
DEFINE_STAT(STAT_InterpolateSkippedFrames);
DEFINE_STAT(STAT_AnimTickTime);
DEFINE_STAT(STAT_SkinnedMeshCompTick);
DEFINE_STAT(STAT_TickUpdateRate);
DEFINE_STAT(STAT_UpdateAnimation);
DEFINE_STAT(STAT_PreUpdateAnimation);
DEFINE_STAT(STAT_PostUpdateAnimation);
DEFINE_STAT(STAT_BlueprintUpdateAnimation);
DEFINE_STAT(STAT_BlueprintPostEvaluateAnimation);
DEFINE_STAT(STAT_NativeUpdateAnimation);
DEFINE_STAT(STAT_NativeThreadSafeUpdateAnimation);
DEFINE_STAT(STAT_Montage_Advance);
DEFINE_STAT(STAT_Montage_UpdateWeight);
DEFINE_STAT(STAT_UpdateCurves);
DEFINE_STAT(STAT_UpdateCurvesToEvaluationContext);
DEFINE_STAT(STAT_UpdateCurvesPostEvaluation);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Anim Init Time"), STAT_AnimInitTime, STATGROUP_Anim, );
DEFINE_STAT(STAT_AnimInitTime);

DEFINE_STAT(STAT_AnimStateMachineUpdate);
DEFINE_STAT(STAT_AnimStateMachineFindTransition);

DEFINE_STAT(STAT_SkinPerPolyVertices);
DEFINE_STAT(STAT_UpdateTriMeshVertices);

DEFINE_STAT(STAT_AnimGameThreadTime);

DEFINE_STAT(STAT_TickAssetPlayerInstances);
DEFINE_STAT(STAT_TickAssetPlayerInstance);

CSV_DEFINE_CATEGORY_MODULE(ENGINE_API, Animation, false);

// Define AnimNotify
// 定义动画通知
DEFINE_LOG_CATEGORY(LogAnimNotify);

#define LOCTEXT_NAMESPACE "AnimInstance"

extern TAutoConsoleVariable<int32> CVarUseParallelAnimUpdate;
extern TAutoConsoleVariable<int32> CVarUseParallelAnimationEvaluation;
extern TAutoConsoleVariable<int32> CVarForceUseParallelAnimUpdate;

ENGINE_API float RK4_SPRING_INTERPOLATOR_UPDATE_RATE = 60.f;
static FAutoConsoleVariableRef CVarRK4SpringInterpolatorUpdateRate(TEXT("p.RK4SpringInterpolator.UpdateRate"), RK4_SPRING_INTERPOLATOR_UPDATE_RATE, TEXT("RK4 Spring Interpolator's rate of update"), ECVF_Default);

ENGINE_API int32 RK4_SPRING_INTERPOLATOR_MAX_ITER = 4;
static FAutoConsoleVariableRef CVarRK4SpringInterpolatorMaxIter(TEXT("p.RK4SpringInterpolator.MaxIter"), RK4_SPRING_INTERPOLATOR_MAX_ITER, TEXT("RK4 Spring Interpolator's max number of iterations"), ECVF_Default);

namespace MontageCVars
{
	bool bEarlyOutMontageWhenUninitialized = true;
	static FAutoConsoleVariableRef CVarEarlyOutMontageWhenUninitialized(
		TEXT("a.Montage.EarlyOutMontageWhenUninitialized"),
		bEarlyOutMontageWhenUninitialized,
		TEXT("Exit early when playing or stopping montage(s) if the animation instance was uninitialized while we are performing the action due to triggering montage events. Use this to avoid reading from bad memory."));

	static bool bFlushCompletedMontagesOnPlay = false;
	static FAutoConsoleVariableRef CVarFlushCompletedMontagesOnPlay(
		TEXT("a.Montage.FlushCompletedMontagesOnPlay"),
		bFlushCompletedMontagesOnPlay,
		TEXT("Whether we should flush all completed montages IMMEDIATELY when a new montage stomps the group. Use this to prevent accumulating montages when animation tick is paused, and firing all at once when we unpause."));
}

/////////////////////////////////////////////////////
// UAnimInstance
// U动画实例
/////////////////////////////////////////////////////

UAnimInstance::UAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
#if DO_CHECK
	, bPostUpdatingAnimation(false)
	, bUpdatingAnimation(false)
#endif
{
	RootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
	bNeedsUpdate = false;

	// Default to using threaded animation update.
	// 默认使用线程动画更新。
	bUseMultiThreadedAnimationUpdate = true;
	bCreatedByLinkedAnimGraph = false;
	PendingDynamicResetTeleportType = ETeleportType::None;

	bReceiveNotifiesFromLinkedInstances = false;
	bPropagateNotifiesToLinkedInstances = false;
	bUseMainInstanceMontageEvaluationData = false;

	bUninitialized = true;

	bUpdateAnimationEnabled = true;
#if DO_CHECK
	bInitializing = false;
#endif

#if WITH_EDITOR
	if(!HasAnyFlags(RF_ClassDefaultObject) && !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		FCoreUObjectDelegates::OnObjectsReinstanced.AddUObject(this, &UAnimInstance::HandleObjectsReinstanced);
	}
#endif // WITH_EDITOR
}

// Disable compiler-generated deprecation warnings by implementing our own destructor
// 通过实现我们自己的析构函数来禁用编译器生成的弃用警告
PRAGMA_DISABLE_DEPRECATION_WARNINGS
UAnimInstance::~UAnimInstance() = default;
PRAGMA_ENABLE_DEPRECATION_WARNINGS

// this is only used by montage marker based sync
// 这仅由基于蒙太奇标记的同步使用
void UAnimInstance::MakeMontageTickRecord(FAnimTickRecord& TickRecord, class UAnimMontage* Montage, float CurrentPosition, float Weight, TArray<FPassedMarker>& MarkersPassedThisTick, FMarkerTickRecord& MarkerTickRecord)
{
	TickRecord.SourceAsset = Montage;
	TickRecord.Montage.CurrentPosition = CurrentPosition;
	TickRecord.Montage.MarkersPassedThisTick = &MarkersPassedThisTick;
	TickRecord.MarkerTickRecord = &MarkerTickRecord;
	TickRecord.PlayRateMultiplier = 1.f; // we don't care here, this is alreayd applied in the montageinstance::Advance
	TickRecord.EffectiveBlendWeight = Weight;
	TickRecord.bLooping = false;
}

AActor* UAnimInstance::GetOwningActor() const
{
	USkeletalMeshComponent* OwnerComponent = GetSkelMeshComponent();
	return OwnerComponent->GetOwner();
}

APawn* UAnimInstance::TryGetPawnOwner() const
{
	USkeletalMeshComponent* OwnerComponent = GetSkelMeshComponent();
	if (AActor* OwnerActor = OwnerComponent->GetOwner())
	{
		return Cast<APawn>(OwnerActor);
	}

	return NULL;
}

void UAnimInstance::SavePoseSnapshot(FName SnapshotName)
{
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	if(USkeletalMeshComponent* SkeletalMeshComponent = GetSkelMeshComponent())
	{
		Proxy.SavePoseSnapshot(SkeletalMeshComponent, SnapshotName);
	}
}

FPoseSnapshot& UAnimInstance::AddPoseSnapshot(FName SnapshotName)
{
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	return Proxy.AddPoseSnapshot(SnapshotName);
}

void UAnimInstance::RemovePoseSnapshot(FName SnapshotName)
{
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	Proxy.RemovePoseSnapshot(SnapshotName);
}

const FPoseSnapshot* UAnimInstance::GetPoseSnapshot(FName SnapshotName) const
{
	const FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	return Proxy.GetPoseSnapshot(SnapshotName);
}

void UAnimInstance::SnapshotPose(FPoseSnapshot& Snapshot)
{
	if (USkeletalMeshComponent* SkeletalMeshComponent = GetSkelMeshComponent())
	{
		SkeletalMeshComponent->SnapshotPose(Snapshot);
	}
}

const TMap<FName, FAnimGroupInstance>& UAnimInstance::GetSyncGroupMapRead() const
{
	const FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	return Proxy.GetSyncGroupMapRead();
}

const TArray<FAnimTickRecord>& UAnimInstance::GetUngroupedActivePlayersRead()
{
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	return Proxy.GetUngroupedActivePlayersRead();
}

const TMap<FName, float>& UAnimInstance::GetAnimationCurves(EAnimCurveType InCurveType) const
{
	const FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	return Proxy.GetAnimationCurves(InCurveType);
}

void UAnimInstance::GatherDebugData(FNodeDebugData& DebugData)
{
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
	Proxy.GatherDebugData(DebugData);
}

USkeletalMeshComponent* UAnimInstance::GetOwningComponent() const
{
	return GetSkelMeshComponent();
}

UAnimInstance* UAnimInstance::Blueprint_GetMainAnimInstance() const
{
	return GetSkelMeshComponent()->GetAnimInstance();
}

UWorld* UAnimInstance::GetWorld() const
{
	// The CDO isn't owned by a SkelMeshComponent (and doesn't have a World)
	// CDO 不属于 SkelMeshComponent（并且没有世界）
	return (HasAnyFlags(RF_ClassDefaultObject) ? nullptr : GetSkelMeshComponent()->GetWorld());
}

void UAnimInstance::InitializeAnimation(bool bInDeferRootNodeInitialization)
{
	FScopeCycleCounterUObject ContextScope(this);
	SCOPE_CYCLE_COUNTER(STAT_AnimInitTime);
	LLM_SCOPE(ELLMTag::Animation);

	UninitializeAnimation();
	bUninitialized = false;

	TRACE_OBJECT_LIFETIME_BEGIN(this);

	// make sure your skeleton is initialized
	// 确保你的骨架已经初始化
	// you can overwrite different skeleton
	// 你可以覆盖不同的骨架
	USkeletalMeshComponent* OwnerComponent = GetSkelMeshComponent();
	if (OwnerComponent->GetSkeletalMeshAsset() != NULL)
	{
		CurrentSkeleton = OwnerComponent->GetSkeletalMeshAsset()->GetSkeleton();
	}
	else
	{
		CurrentSkeleton = NULL;
	}

	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
#if WITH_EDITOR
		LifeTimer = 0.0;
		CurrentLifeTimerScrubPosition = 0.0;

		// 当动画实例初始化时，如果这个实例正在被编辑器调试，则清空之前记录的调试快照数据，准备记录新的调试信息。
		if (UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(Cast<UAnimBlueprintGeneratedClass>(AnimBlueprintClass)->ClassGeneratedBy))
		{
			if (Blueprint->GetObjectBeingDebugged() == this)
			{
				// Reset the snapshot buffer
				// 重置快照缓冲区
				Cast<UAnimBlueprintGeneratedClass>(AnimBlueprintClass)->GetAnimBlueprintDebugData().ResetSnapshotBuffer();
			}
		}
#endif
	}

	// before initialize, need to recalculate required bone list
	// 初始化之前，需要重新计算所需的骨骼列表
	// 初始化之前，获得工作线程上的Animinstanceproxy，让其从USkeletalMeshComponent上获得骨骼信息并缓存下来
	RecalcRequiredBones();

	GetProxyOnGameThread<FAnimInstanceProxy>().Initialize(this);

	{
#if DO_CHECK
		// Allow us to validate callbacks within user code
		// 允许我们验证用户代码中的回调
		FGuardValue_Bitfield(bInitializing, true);
#endif

		NativeInitializeAnimation();
		BlueprintInitializeAnimation();
	}

	GetProxyOnGameThread<FAnimInstanceProxy>().InitializeRootNode(bInDeferRootNodeInitialization);

	// we can bind rules & events now the graph has been initialized
	// 现在图表已经初始化，我们可以绑定规则和事件
	GetProxyOnGameThread<FAnimInstanceProxy>().BindNativeDelegates();

	InitializeGroupedLayers(bInDeferRootNodeInitialization);

	NativeLinkedAnimationLayersInitialized();
	BlueprintLinkedAnimationLayersInitialized();
}

void UAnimInstance::UninitializeAnimation()
{
	bUninitialized = true;

	NativeUninitializeAnimation();

	if (AnimInstanceProxy != nullptr)
	{
		GetProxyOnGameThread<FAnimInstanceProxy>().Uninitialize(this);
	}

	StopAllMontages(0.f);

	if (MontageInstances.Num() > 0)
	{
		for (int32 Index = 0; Index < MontageInstances.Num(); ++Index)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[Index];
			if (ensure(MontageInstance != nullptr))
			{
				ClearMontageInstanceReferences(*MontageInstance);
				delete MontageInstance;
			}
		}

		MontageInstances.Empty();
		ActiveMontagesMap.Empty();

		OnAllMontageInstancesEnded.Broadcast();
	}

	USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();

	// Skip dispatching notify end messages during re-instancing. Various classes may be in an incomplete state so calling
	// 在重新实例期间跳过调度通知结束消息。各种类可能处于不完整状态，因此调用
	// arbitrary script is dangerous.
	// 任意脚本是危险的。
	//
	// 处理动画实例（AnimInstance）或其所属组件在注销/重构时，强制“清场”未完成的动画通知
	if(!GIsReinstancing) //热重载中 判断
	{
		if (SkelMeshComp)
		{
			// Tick currently active AnimNotifyState
			// 勾选当前活动的 AnimNotifyState
			for(int32 Index=0; Index<ActiveAnimNotifyState.Num(); Index++)
			{
				const FAnimNotifyEvent& AnimNotifyEvent = ActiveAnimNotifyState[Index];
				const FAnimNotifyEventReference& EventReference = ActiveAnimNotifyEventReference[Index];
				if (ShouldTriggerAnimNotifyState(AnimNotifyEvent.NotifyStateClass) && !AnimNotifyEvent.NotifyStateClass->IsUnreachable())
				{
#if WITH_EDITOR
					// Prevent firing notifies in animation editors if requested
					// 如果需要，防止在动画编辑器中触发通知
					if(!SkelMeshComp->IsA<UDebugSkelMeshComponent>() || AnimNotifyEvent.NotifyStateClass->ShouldFireInEditor())
#endif
					{
						TRACE_ANIM_NOTIFY(this, AnimNotifyEvent, End);
						AnimNotifyEvent.NotifyStateClass->NotifyEnd(SkelMeshComp, Cast<UAnimSequenceBase>(AnimNotifyEvent.NotifyStateClass->GetOuter()), EventReference);
					}
				}
			}
		}
	}

	ActiveAnimNotifyState.Reset();
	ActiveAnimNotifyEventReference.Reset();
	NotifyQueue.Reset(SkelMeshComp);

	SlotGroupInertializationRequestDataMap.Reset();

	// Cleanup layer nodes
	// 清理层节点
	// 循环卸载链接的子动画蓝图
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		for (FStructProperty* LayerNodeProperty : AnimBlueprintClass->GetLinkedAnimLayerNodeProperties())
		{
			if (FAnimNode_LinkedAnimLayer* Layer = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(this))
			{
				Layer->OnUninitializeAnimInstance(this);
			}
		}
	}

	if (!GIsReinstancing)
	{
		// Cleanup shared layers data (we don't use FAnimSubsystem_SharedLinkedAnimLayers::GetFromMesh here as we only want the main instance to clean the shared layers)
		// 清理共享图层数据（我们在这里不使用 FAnimSubsystem_SharedLinkedAnimLayers::GetFromMesh，因为我们只希望主实例清理共享图层）
		if (FAnimSubsystem_SharedLinkedAnimLayers* SharedLinkedAnimLayers = FindSubsystem<FAnimSubsystem_SharedLinkedAnimLayers>())
		{
			// Reset shared linked instances when the main instance in uninitialized.
			// 当主实例未初始化时重置共享链接实例。
			// This is required in part for SkeletalMeshComponent::OnUnregister so that the linked instances array isn't modified as we iterate on it to unitialize them. (see FAnimNode_LinkedLayer::CleanupSharedLinkedLayersData)
			// 这对于 SkeletalMeshComponent::OnUnregister 来说是必需的，这样当我们迭代它以统一它们时，链接的实例数组就不会被修改。 （参见 FAnimNode_LinkedLayer::CleanupSharedLinkedLayersData）
			// 通过让主实例在卸载时统一重置子系统，可以避免多个子实例在销毁过程中因同时修改共享数组而导致的迭代器失效（内存崩溃）问题
			SharedLinkedAnimLayers->Reset();
		}
	}
}

#if WITH_EDITORONLY_DATA
bool UAnimInstance::UpdateSnapshotAndSkipRemainingUpdate()
{
#if WITH_EDITOR
	// Avoid updating the instance if we're replaying the past
	// 如果我们重播过去，请避免更新实例
	if (UAnimBlueprintGeneratedClass* AnimBlueprintClass = Cast<UAnimBlueprintGeneratedClass>(GetClass()))
	{
		FAnimBlueprintDebugData& DebugData = AnimBlueprintClass->GetAnimBlueprintDebugData();
		if (DebugData.IsReplayingSnapshot())
		{
			if (UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(AnimBlueprintClass->ClassGeneratedBy))
			{
				if (Blueprint->GetObjectBeingDebugged() == this)
				{
					// Find the correct frame
					// 找到正确的框架
					DebugData.SetSnapshotIndexByTime(this, CurrentLifeTimerScrubPosition);
					return true;
				}
			}
		}
	}
#endif
	return false;
}
#endif

void UAnimInstance::UpdateMontage(float DeltaSeconds)
{
	// Don't update montages if we are using the main instance's montage eval data and we are not the main instance.
	// 如果我们使用主实例的蒙太奇评估数据并且我们不是主实例，请不要更新蒙太奇。
	if (IsUsingMainInstanceMontageEvaluationData())
	{
		if (GetOwningComponent()->GetAnimInstance() != this)
		{
			return;
		}
	}

	// update montage weight
	// 更新蒙太奇权重
	Montage_UpdateWeight(DeltaSeconds);

	// update montage should run in game thread
	// 更新蒙太奇应该在游戏线程中运行
	// if we do multi threading, make sure this stays in game thread.
	// 如果我们进行多线程，请确保它保留在游戏线程中。
	// This is because branch points need to execute arbitrary code inside this call.
	// 这是因为分支点需要在此调用内执行任意代码。
	Montage_Advance(DeltaSeconds);

#if ANIM_TRACE_ENABLED
	for (FAnimMontageInstance* MontageInstance : MontageInstances)
	{
		TRACE_ANIM_MONTAGE(this, *MontageInstance);
	}
#endif
}

void UAnimInstance::UpdateMontageSyncGroup()
{
	for (FAnimMontageInstance* MontageInstance : MontageInstances)
	{
		bool bRecordNeedsResetting = true;
		if (MontageInstance->bDidUseMarkerSyncThisTick)
		{
			const FName GroupNameToUse = MontageInstance->GetSyncGroupName();

			// that is public data, so if anybody decided to play with it
			// 这是公共数据，所以如果有人决定使用它
			if (ensure(GroupNameToUse != NAME_None))
			{
				bRecordNeedsResetting = false;
				FAnimTickRecord TickRecord(
					MontageInstance->Montage,
					MontageInstance->GetPosition(),
					MontageInstance->GetWeight(),
					MontageInstance->MarkersPassedThisTick,
					MontageInstance->MarkerTickRecord
				);
				TickRecord.DeltaTimeRecord = &MontageInstance->DeltaTimeRecord;

				UE::Anim::FAnimSyncParams Params(GroupNameToUse);
				GetProxyOnGameThread<FAnimInstanceProxy>().AddTickRecord(TickRecord, Params);

#if ANIM_TRACE_ENABLED
				FAnimationUpdateContext UpdateContext(&GetProxyOnGameThread<FAnimInstanceProxy>());
				TRACE_ANIM_TICK_RECORD(UpdateContext, TickRecord);
#endif
			}
			MontageInstance->bDidUseMarkerSyncThisTick = false;
		}
		if (bRecordNeedsResetting)
		{
			MontageInstance->MarkerTickRecord.Reset();
		}
	}
}

void UAnimInstance::UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion, EUpdateAnimationFlag UpdateFlag)
{
	LLM_SCOPE(ELLMTag::Animation);

#if WITH_EDITOR
	if(GIsReinstancing)
	{
		return;
	}
#endif

#if DO_CHECK
	checkf(!bUpdatingAnimation, TEXT("UpdateAnimation already in progress, circular detected for SkeletalMeshComponent [%s], AnimInstance [%s]"), *GetNameSafe(GetOwningComponent()),  *GetName());
	TGuardValue<bool> CircularGuard(bUpdatingAnimation, true);
#endif

	if (!IsUpdateAnimationEnabled())
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_UpdateAnimation);
	FScopeCycleCounterUObject AnimScope(this);

	// acquire the proxy as we need to update
	// 当我们需要更新时获取代理
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();

	// Apply any pending dynamics reset
	// 应用任何待处理的动态重置
	if(PendingDynamicResetTeleportType != ETeleportType::None)
	{
		Proxy.ResetDynamics(PendingDynamicResetTeleportType);
		PendingDynamicResetTeleportType = ETeleportType::None;
	}

	if (const USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent())
	{
		/**
			If we're set to OnlyTickMontagesWhenNotRendered and we haven't been recently rendered,
			then only update montages and skip everything else.
		*/
		if (SkelMeshComp->ShouldOnlyTickMontages(DeltaSeconds) || (SkelMeshComp->ShouldOnlyTickMontagesAndRefreshBones(DeltaSeconds) && !IsAnyMontagePlaying()))
		{
			/**
				Clear NotifyQueue prior to ticking montages.
				This is typically done in 'PreUpdate', but we're skipping this here since we're not updating the graph.
				A side effect of this, is that we're stopping all state notifies in the graph, until ticking resumes.
				This should be fine. But if it is ever a problem, we should keep two versions of them. One for montages and one for the graph.
			*/
			ClearQueuedAnimEvents(true);

			/**
				Reset UpdateCounter(), this will force Update to occur if Eval is triggered without an Update.
				This is to ensure that SlotNode EvaluationData is resynced to evaluate properly.
			*/
			Proxy.ResetUpdateCounter();

			UpdateMontage(DeltaSeconds);

			/**
				We intentionally skip UpdateMontageSyncGroup(), since SyncGroup update is skipped along with AnimGraph update.
				We do need to reset tick records since the montage will appear to have "jumped" if normal ticking resumes.
			*/
			for (FAnimMontageInstance* MontageInstance : MontageInstances)
			{
				MontageInstance->bDidUseMarkerSyncThisTick = false;
				MontageInstance->MarkerTickRecord.Reset();
				MontageInstance->MarkersPassedThisTick.Reset();
			};

			/**
				We also intentionally do not call UpdateMontageEvaluationData after the call to UpdateMontage.
				As we would have to call 'UpdateAnimation' on the graph as well, so weights could be in sync with this new data.
				The problem lies in the fact that 'Evaluation' can be called without a call to 'Update' prior.
				This means our data would be out of sync. So we only call UpdateMontageEvaluationData below
				when we also update the AnimGraph as well.
				This means that calls to 'Evaluation' without a call to 'Update' prior will render stale data, but that's to be expected.
			*/
			return;
		}
	}

#if WITH_EDITOR
	if (GIsEditor)
	{
		// Update the lifetimer and see if we should use the snapshot instead
		// 更新生命周期并看看我们是否应该使用快照
		CurrentLifeTimerScrubPosition += DeltaSeconds;
		LifeTimer = FMath::Max<double>(CurrentLifeTimerScrubPosition, LifeTimer);

		if (UpdateSnapshotAndSkipRemainingUpdate())
		{
			return;
		}
	}
#endif

	PreUpdateAnimation(DeltaSeconds);

	// need to update montage BEFORE node update or Native Update.
	// 需要在节点更新或本机更新之前更新蒙太奇。
	// so that node knows where montage is
	// 这样节点就知道蒙太奇在哪里
	{
		UpdateMontage(DeltaSeconds);

		// now we know all montage has advanced
		// 现在我们知道所有蒙太奇都已经进步了
		// time to test sync groups
		// 测试同步组的时间
		UpdateMontageSyncGroup();

		// Update montage eval data, to be used by AnimGraph Update and Evaluate phases.
		// 更新蒙太奇评估数据，供 AnimGraph 更新和评估阶段使用。
		UpdateMontageEvaluationData();
	}

	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		AnimBlueprintClass->ForEachSubsystem(this, [this, DeltaSeconds](const FAnimSubsystemInstanceContext& InContext)
		{
			FAnimSubsystemUpdateContext Context(InContext, this, DeltaSeconds);
			InContext.Subsystem.OnPreUpdate_GameThread(Context);
			return EAnimSubsystemEnumeration::Continue;
		});
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_NativeUpdateAnimation);
		CSV_SCOPED_TIMING_STAT(Animation, NativeUpdate);
		NativeUpdateAnimation(DeltaSeconds);
	}
	{
		SCOPE_CYCLE_COUNTER(STAT_BlueprintUpdateAnimation);
		CSV_SCOPED_TIMING_STAT(Animation, BlueprintUpdate);
		BlueprintUpdateAnimation(DeltaSeconds);
	}

	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		AnimBlueprintClass->ForEachSubsystem(this, [this, DeltaSeconds](const FAnimSubsystemInstanceContext& InContext)
		{
			FAnimSubsystemUpdateContext Context(InContext, this, DeltaSeconds);
			InContext.Subsystem.OnPostUpdate_GameThread(Context);
			return EAnimSubsystemEnumeration::Continue;
		});
	}

	// Determine whether or not the animation should be immediately updated according to current state
	// 根据当前状态判断是否立即更新动画
	const bool bWantsImmediateUpdate = NeedsImmediateUpdate(DeltaSeconds, bNeedsValidRootMotion);

	// Determine whether or not we can or should actually immediately update the animation state
	// 确定我们是否可以或应该立即更新动画状态
	bool bShouldImmediateUpdate = bWantsImmediateUpdate;
	switch (UpdateFlag)
	{
		case EUpdateAnimationFlag::ForceParallelUpdate:
		{
			bShouldImmediateUpdate = false;
			break;
		}
	}

	if(bShouldImmediateUpdate)
	{
		// cant use parallel update, so just do the work here (we call this function here to do the work on the game thread)
		// 无法使用并行更新，所以只需在这里完成工作（我们在这里调用此函数来在游戏线程上完成工作）
		ParallelUpdateAnimation();
		PostUpdateAnimation();
	}
}

void UAnimInstance::PreUpdateAnimation(float DeltaSeconds)
{
	LLM_SCOPE(ELLMTag::Animation);
	SCOPE_CYCLE_COUNTER(STAT_PreUpdateAnimation);

	bNeedsUpdate = true;

	ClearQueuedAnimEvents(true);

	RootMotionBlendQueue.Reset();

	GetProxyOnGameThread<FAnimInstanceProxy>().PreUpdate(this, DeltaSeconds);
}

void UAnimInstance::PostUpdateAnimation()
{
	LLM_SCOPE(ELLMTag::Animation);
#if DO_CHECK
	checkf(!bPostUpdatingAnimation, TEXT("PostUpdateAnimation already in progress, recursion detected for SkeletalMeshComponent [%s], AnimInstance [%s]"), *GetNameSafe(GetOwningComponent()), *GetName());
	TGuardValue<bool> CircularGuard(bPostUpdatingAnimation, true);
#endif

	SCOPE_CYCLE_COUNTER(STAT_PostUpdateAnimation);
	check(!IsRunningParallelEvaluation());

	// Call post-update on linked instances here rather than at a higher level in SkeletalMeshComponent,
	// 在这里调用链接实例的更新后，而不是在 SkeletalMeshComponent 中的更高级别调用，
	// but only IF we are the primary anim instance. This is to cover all the cases in which PostUpdateAnimation can be called:
	// 但前提是我们是主要动画实例。这是为了涵盖可以调用 PostUpdateAnimation 的所有情况：
	// - During a character movement tick to get animated root motion.
	// - 在角色移动期间勾选以获得动画根运动。
	//    - In this case we need to PostUpdateAnimation linked instances immediately to make sure their notfies
	//    - 在这种情况下，我们需要立即 PostUpdateAnimation 链接实例以确保它们的通知
	//      are dispatched (they are queued from the call to TickAssetPlayerInstances in ParallelUpdateAnimation that will
	//      被分派（它们在 ParallelUpdateAnimation 中对 TickAssetPlayerInstances 的调用中排队，这将
	//      have been called on the game thread).
	//      已在游戏线程上调用）。
	// - Post-parallel update/evaluate task.
	// - 后并行更新/评估任务。
	//    - In the non-root motion case we need to dispatch notifies on the game thread (that were queued on a worker thread)
	//    - 在非根运动情况下，我们需要在游戏线程上分派通知（在工作线程上排队）
	//      when parallel tasks are completed.
	//      当并行任务完成时。
	if(GetSkelMeshComponent()->GetAnimInstance() == this)
	{
		for (UAnimInstance* LinkedInstance : GetSkelMeshComponent()->GetLinkedAnimInstances())
		{
			if(LinkedInstance->NeedsUpdate())
			{
				LinkedInstance->PostUpdateAnimation();
			}
		}
	}

	// Early out here if we are not set to needing an update
	// 如果我们不需要更新，请尽早出去
	if(!bNeedsUpdate)
	{
		return;
	}

	bNeedsUpdate = false;

	// acquire the proxy as we need to update
	// 当我们需要更新时获取代理
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();

	// flip read/write index
	// 翻转读/写索引
	// Do this first, as we'll be reading cached slot weights, and we want this to be up to date for this frame.
	// 首先执行此操作，因为我们将读取缓存的槽权重，并且我们希望它对于该帧是最新的。
	Proxy.FlipBufferWriteIndex();

	Proxy.PostUpdate(this);

	if(Proxy.GetExtractedRootMotion().bHasRootMotion)
	{
		FTransform ProxyTransform = Proxy.GetExtractedRootMotion().GetRootMotionTransform();
		ProxyTransform.NormalizeRotation();
		ExtractedRootMotion.Accumulate(ProxyTransform);
		Proxy.GetExtractedRootMotion().Clear();
	}

	// blend in any montage-blended root motion that we now have correct weights for
	// 混合任何蒙太奇混合的根运动，我们现在有正确的权重
	for(const FQueuedRootMotionBlend& RootMotionBlend : RootMotionBlendQueue)
	{
		const float RootMotionSlotWeight = GetSlotNodeGlobalWeight(RootMotionBlend.SlotName);
		const float RootMotionInstanceWeight = RootMotionBlend.Weight * RootMotionSlotWeight;
		ExtractedRootMotion.AccumulateWithBlend(RootMotionBlend.Transform, RootMotionInstanceWeight);
	}

	// We may have just partially blended root motion, so make it up to 1 by
	// 我们可能只是部分混合了根部运动，因此将其设为 1
	// blending in identity too
	// 也融入了身份
	if (ExtractedRootMotion.bHasRootMotion)
	{
		ExtractedRootMotion.MakeUpToFullWeight();
	}

#if WITH_EDITOR && 0
	{
		// Take a snapshot if the scrub control is locked to the end, we are playing, and we are the one being debugged
		// 如果scrub控件锁定到最后，拍个快照，我们正在玩，我们就是被调试的
		if (UAnimBlueprintGeneratedClass* AnimBlueprintClass = Cast<UAnimBlueprintGeneratedClass>(GetClass()))
		{
			if (UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(AnimBlueprintClass->ClassGeneratedBy))
			{
				if (Blueprint->GetObjectBeingDebugged() == this)
				{
					if ((CurrentLifeTimerScrubPosition == LifeTimer) && (Proxy.GetDeltaSeconds() > 0.0f))
					{
						AnimBlueprintClass->GetAnimBlueprintDebugData().TakeSnapshot(this);
					}
				}
			}
		}
	}
#endif
}

void UAnimInstance::DispatchQueuedAnimEvents()
{
	// now trigger Notifies
	// [翻译失败: now trigger Notifies]
	TriggerAnimNotifies(GetProxyOnGameThread<FAnimInstanceProxy>().GetDeltaSeconds());

	// Trigger Montage end events after notifies. In case Montage ending ends abilities or other states, we make sure notifies are processed before montage events.
	// [翻译失败: Trigger Montage end events after notifies. In case Montage ending ends abilities or other states, we make sure notifies are processed before montage events.]
	TriggerQueuedMontageEvents();

	// After queued Montage Events have been dispatched, it's now safe to delete invalid Montage Instances.
	// 调度排队的 Montage 事件后，现在可以安全地删除无效的 Montage 实例。
	// And dispatch 'OnAllMontageInstancesEnded'
	// 并调度“OnAllMontageInstancesEnded”
	for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
	{
		// Should never be null
		// 永远不应该为空
		FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
		ensure(MontageInstance);
		if (MontageInstance && !MontageInstance->IsValid())
		{
			// Make sure we've cleared our references before deleting memory
			// 确保在删除内存之前我们已经清除了引用
			ClearMontageInstanceReferences(*MontageInstance);

			delete MontageInstance;
			MontageInstances.RemoveAt(InstanceIndex);
			--InstanceIndex;

			if (MontageInstances.Num() == 0)
			{
				OnAllMontageInstancesEnded.Broadcast();
			}
		}
	}
}

void UAnimInstance::ClearQueuedAnimEvents(bool bShouldUpdateActiveAnimNotifiesSinceLastTick)
{
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();

	if (bShouldUpdateActiveAnimNotifiesSinceLastTick)
	{
		// Before clearing notify queue we need to ensure we record the notifies that were triggered last tick.
		// 在清除通知队列之前，我们需要确保记录最后一次触发的通知。
		Proxy.UpdateActiveAnimNotifiesSinceLastTick(NotifyQueue);
	}
	else
	{
		// In the case this function is manually called multiple times a tick, we need to ensure we keep track of previous anim notifies properly.
		// 如果这个函数在一个周期内被手动调用多次，我们需要确保正确跟踪以前的动画通知。
		Proxy.ActiveAnimNotifiesSinceLastTick.Append(NotifyQueue.AnimNotifies);
	}

	NotifyQueue.Reset(GetSkelMeshComponent());
}

void UAnimInstance::ParallelUpdateAnimation()
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().UpdateAnimation();
}

bool UAnimInstance::NeedsImmediateUpdate(float DeltaSeconds, bool bNeedsValidRootMotion) const
{
	const bool bUseParallelUpdateAnimation = (GetDefault<UEngine>()->bAllowMultiThreadedAnimationUpdate && bUseMultiThreadedAnimationUpdate) || (CVarForceUseParallelAnimUpdate.GetValueOnGameThread() != 0);
#if WITH_EDITOR
	UAnimBlueprintGeneratedClass* GeneratedClass = Cast<UAnimBlueprintGeneratedClass>(GetClass());
	UBlueprint* Blueprint = GeneratedClass ? Cast<UBlueprint>(GeneratedClass->ClassGeneratedBy) : nullptr;
#endif

	return
		(bNeedsValidRootMotion && RootMotionMode == ERootMotionMode::RootMotionFromEverything) ||
		!CanRunParallelWork() ||
		GIntraFrameDebuggingGameThread ||
#if WITH_EDITOR
		// Force the debugged object to run its anim graph on the game thread if it is being debugged
		// 如果正在调试，则强制被调试对象在游戏线程上运行其动画图
		// Also force onto the game thread if breakpoints are set. This will force the anim BP to break even if
		// [翻译失败: Also force onto the game thread if breakpoints are set. This will force the anim BP to break even if]
		// the target object is not set for debug
		// [翻译失败: the target object is not set for debug]
		// This ensures that it uses the persistent ubergraph frame and debugging facilities are available like
		// [翻译失败: This ensures that it uses the persistent ubergraph frame and debugging facilities are available like]
		// watches, breakpoints etc.
		// [翻译失败: watches, breakpoints etc.]
		(Blueprint && (Blueprint->GetObjectBeingDebugged() == this || FKismetDebugUtilities::BlueprintHasBreakpoints(Blueprint))) ||
#endif
		CVarUseParallelAnimUpdate.GetValueOnGameThread() == 0 ||
		CVarUseParallelAnimationEvaluation.GetValueOnGameThread() == 0 ||
		!bUseParallelUpdateAnimation ||
		DeltaSeconds == 0.0f;
}

bool UAnimInstance::NeedsUpdate() const
{
	return bNeedsUpdate;
}

bool UAnimInstance::IsInitialized() const
{
	return !bUninitialized;
}

void UAnimInstance::PreEvaluateAnimation()
{
	GetProxyOnGameThread<FAnimInstanceProxy>().PreEvaluateAnimation(this);
}

bool UAnimInstance::ParallelCanEvaluate(const USkeletalMesh* InSkeletalMesh) const
{
	const FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
	return Proxy.GetRequiredBones().IsValid() && (Proxy.GetRequiredBones().GetAsset() == InSkeletalMesh);
}

void UAnimInstance::ParallelEvaluateAnimation(bool bForceRefPose, const USkeletalMesh* InSkeletalMesh, FBlendedHeapCurve& OutCurve, FCompactPose& OutPose)
{
	UE::Anim::FHeapAttributeContainer Attributes;
	FParallelEvaluationData EvalData = { OutCurve, OutPose, Attributes };
	ParallelEvaluateAnimation(bForceRefPose, InSkeletalMesh, EvalData);
}

void UAnimInstance::ParallelEvaluateAnimation(bool bForceRefPose, const USkeletalMesh* InSkeletalMesh, FParallelEvaluationData& OutEvaluationData)
{
	LLM_SCOPE(ELLMTag::Animation);
	FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
	OutEvaluationData.OutPose.SetBoneContainer(&Proxy.GetRequiredBones());

	FMemMark Mark(FMemStack::Get());
	// Push cached pose scope to constrain cached pose lifetime to within this evaluate pass only (as cached poses are
	// [翻译失败: Push cached pose scope to constrain cached pose lifetime to within this evaluate pass only (as cached poses are]
	// allocated with the above FMemMark)
	// 分配有上述 FMemMark）
	UE::Anim::FCachedPoseScope CachedPoseScope;

	if( !bForceRefPose )
	{
		// Create an evaluation context
		// 创建评估上下文
		FPoseContext EvaluationContext(&Proxy);
		EvaluationContext.ResetToRefPose();

		// Run the anim blueprint
		// 运行动画蓝图
		Proxy.EvaluateAnimation(EvaluationContext);

		// Move the curves
		// 移动曲线
		OutEvaluationData.OutCurve.CopyFrom(EvaluationContext.Curve);
		OutEvaluationData.OutPose.CopyBonesFrom(EvaluationContext.Pose);

		OutEvaluationData.OutAttributes.CopyFrom(EvaluationContext.CustomAttributes);
	}
	else
	{
		OutEvaluationData.OutPose.ResetToRefPose();
	}
}

void UAnimInstance::PostEvaluateAnimation()
{
	LLM_SCOPE(ELLMTag::Animation);
	NativePostEvaluateAnimation();

	{
		SCOPE_CYCLE_COUNTER(STAT_BlueprintPostEvaluateAnimation);
		BlueprintPostEvaluateAnimation();
	}

	GetProxyOnGameThread<FAnimInstanceProxy>().PostEvaluate(this);
}

void UAnimInstance::NativeInitializeAnimation()
{
}

void UAnimInstance::NativeLinkedAnimationLayersInitialized()
{
}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
}

void UAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
}

void UAnimInstance::NativePostEvaluateAnimation()
{
}

void UAnimInstance::NativeUninitializeAnimation()
{
}

void UAnimInstance::NativeBeginPlay()
{

}

void UAnimInstance::AddNativeTransitionBinding(const FName& MachineName, const FName& PrevStateName, const FName& NextStateName, const FCanTakeTransition& NativeTransitionDelegate, const FName& TransitionName)
{
	GetProxyOnGameThread<FAnimInstanceProxy>().AddNativeTransitionBinding(MachineName, PrevStateName, NextStateName, NativeTransitionDelegate, TransitionName);
}

bool UAnimInstance::HasNativeTransitionBinding(const FName& MachineName, const FName& PrevStateName, const FName& NextStateName, FName& OutBindingName)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().HasNativeTransitionBinding(MachineName, PrevStateName, NextStateName, OutBindingName);
}

void UAnimInstance::AddNativeStateEntryBinding(const FName& MachineName, const FName& StateName, const FOnGraphStateChanged& NativeEnteredDelegate)
{
	GetProxyOnGameThread<FAnimInstanceProxy>().AddNativeStateEntryBinding(MachineName, StateName, NativeEnteredDelegate);
}

bool UAnimInstance::HasNativeStateEntryBinding(const FName& MachineName, const FName& StateName, FName& OutBindingName)
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().HasNativeStateEntryBinding(MachineName, StateName, OutBindingName);
}

void UAnimInstance::AddNativeStateExitBinding(const FName& MachineName, const FName& StateName, const FOnGraphStateChanged& NativeExitedDelegate)
{
	GetProxyOnGameThread<FAnimInstanceProxy>().AddNativeStateExitBinding(MachineName, StateName, NativeExitedDelegate);
}

bool UAnimInstance::HasNativeStateExitBinding(const FName& MachineName, const FName& StateName, FName& OutBindingName)
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().HasNativeStateExitBinding(MachineName, StateName, OutBindingName);
}

void OutputCurveMap(TMap<FName, float>& CurveMap, UCanvas* Canvas, FDisplayDebugManager& DisplayDebugManager, float Indent)
{
	TArray<FName> Names;
	CurveMap.GetKeys(Names);
	Names.Sort(FNameLexicalLess());
	for (FName CurveName : Names)
	{
		FString CurveEntry = FString::Printf(TEXT("%s: %.3f"), *CurveName.ToString(), CurveMap[CurveName]);
		DisplayDebugManager.DrawString(CurveEntry, Indent);
	}
}

void OutputTickRecords(const TArray<FAnimTickRecord>& Records, UCanvas* Canvas, float Indent, const int32 HighlightIndex, FLinearColor TextColor, FLinearColor HighlightColor, FLinearColor InactiveColor, FDisplayDebugManager& DisplayDebugManager, bool bFullBlendspaceDisplay)
{
	for (int32 PlayerIndex = 0; PlayerIndex < Records.Num(); ++PlayerIndex)
	{
		const FAnimTickRecord& Player = Records[PlayerIndex];

		// Assuming highlight index is the group leader index
		// 假设亮点索引是组领导者索引
		if (HighlightIndex != INDEX_NONE && Player.bIsExclusiveLeader && PlayerIndex != HighlightIndex)
		{
			DisplayDebugManager.SetLinearDrawColor(FLinearColor::Red);
			FString PlayerEntry = FString::Printf(TEXT("%i) %s (%s) W(%.f%%) - Push to ungrouped as it failed to be leader."),
			PlayerIndex, *Player.SourceAsset->GetName(), *Player.SourceAsset->GetClass()->GetName(), Player.EffectiveBlendWeight*100.f);
			DisplayDebugManager.DrawString(PlayerEntry, Indent);
			continue;
		}

		DisplayDebugManager.SetLinearDrawColor((PlayerIndex == HighlightIndex) ? HighlightColor : TextColor);

		FString PlayerEntry = FString::Printf(TEXT("%i) %s (%s) W(%.f%%)"),
			PlayerIndex, *Player.SourceAsset->GetName(), *Player.SourceAsset->GetClass()->GetName(), Player.EffectiveBlendWeight*100.f);

		// See if we have access to SequenceLength
		// 看看我们是否有权访问 SequenceLength
		if (UAnimSequenceBase* AnimSeqBase = Cast<UAnimSequenceBase>(Player.SourceAsset))
		{
			PlayerEntry += FString::Printf(TEXT(" P(%.2f/%.2f)"),
				Player.TimeAccumulator != nullptr ? *Player.TimeAccumulator : 0.f, AnimSeqBase->GetPlayLength());
		}
		else
		{
			PlayerEntry += FString::Printf(TEXT(" P(%.2f)"),
				Player.TimeAccumulator != nullptr ? *Player.TimeAccumulator : 0.f);
		}

		// Part of a sync group
		// 同步组的一部分
		if (HighlightIndex != INDEX_NONE)
		{
			FName PrevMarkerName = NAME_None;
			FName NextMarkerName = NAME_None;

			TArray<FAnimSyncMarker> * AuthoredMarkerNames = nullptr;

			if (auto AnimSequence = Cast<UAnimSequence>(Player.SourceAsset))
			{
				AuthoredMarkerNames = &AnimSequence->AuthoredSyncMarkers;
			}
			else if (auto AnimMontage = Cast<UAnimMontage>(Player.SourceAsset))
			{
				AuthoredMarkerNames = &AnimMontage->MarkerData.AuthoredSyncMarkers;
			}

			if (AuthoredMarkerNames)
			{
				if (Player.MarkerTickRecord->PreviousMarker.MarkerIndex >= 0)
				{
					PrevMarkerName = (*AuthoredMarkerNames)[Player.MarkerTickRecord->PreviousMarker.MarkerIndex].MarkerName;
				}

				if (Player.MarkerTickRecord->NextMarker.MarkerIndex >= 0)
				{
					NextMarkerName = (*AuthoredMarkerNames)[Player.MarkerTickRecord->NextMarker.MarkerIndex].MarkerName;
				}
			}

			PlayerEntry += FString::Printf(TEXT(" Prev(%s, i:%d, t:%.3f) Next(%s, i:%d, t:%.3f)"),
				*PrevMarkerName.ToString(), Player.MarkerTickRecord->PreviousMarker.MarkerIndex, Player.MarkerTickRecord->PreviousMarker.TimeToMarker,
				*NextMarkerName.ToString(), Player.MarkerTickRecord->NextMarker.MarkerIndex, Player.MarkerTickRecord->NextMarker.TimeToMarker);
		}

		DisplayDebugManager.DrawString(PlayerEntry, Indent);

		if (UBlendSpace* BlendSpace = Cast<UBlendSpace>(Player.SourceAsset))
		{
			if (bFullBlendspaceDisplay && Player.BlendSpace.BlendSampleDataCache && Player.BlendSpace.BlendSampleDataCache->Num() > 0)
			{
				TArray<FBlendSampleData> SampleData = *Player.BlendSpace.BlendSampleDataCache;
				SampleData.Sort([](const FBlendSampleData& L, const FBlendSampleData& R) { return L.SampleDataIndex < R.SampleDataIndex; });

				FIndenter BlendspaceIndent(Indent);
				const FVector BlendSpacePosition(Player.BlendSpace.BlendSpacePositionX, Player.BlendSpace.BlendSpacePositionY, 0.f);
				FString BlendspaceHeader = FString::Printf(TEXT("Blendspace Input (%s)"), *BlendSpacePosition.ToString());
				DisplayDebugManager.DrawString(BlendspaceHeader, Indent);

				const TArray<FBlendSample>& BlendSamples = BlendSpace->GetBlendSamples();

				int32 WeightedSampleIndex = 0;

				for (int32 SampleIndex = 0; SampleIndex < BlendSamples.Num(); ++SampleIndex)
				{
					const FBlendSample& BlendSample = BlendSamples[SampleIndex];

					float Weight = 0.f;
					for (; WeightedSampleIndex < SampleData.Num(); ++WeightedSampleIndex)
					{
						FBlendSampleData& WeightedSample = SampleData[WeightedSampleIndex];
						if (WeightedSample.SampleDataIndex == SampleIndex)
						{
							Weight += WeightedSample.GetClampedWeight();
						}
						else if (WeightedSample.SampleDataIndex > SampleIndex)
						{
							break;
						}
					}

					FIndenter SampleIndent(Indent);

					DisplayDebugManager.SetLinearDrawColor((Weight > 0.f) ? TextColor : InactiveColor);

					FString SampleEntry = FString::Printf(TEXT("%s W:%.1f%%"), *BlendSample.Animation->GetName(), Weight*100.f);
					DisplayDebugManager.DrawString(SampleEntry, Indent);
				}
			}
		}
	}
}

void UAnimInstance::DisplayDebugInstance(FDisplayDebugManager& DisplayDebugManager, float& Indent)
{
	DisplayDebugManager.SetLinearDrawColor(FLinearColor::Green);

	if (USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent())
	{
		const int32 MaxLODIndex = SkelMeshComp->MeshObject ? (SkelMeshComp->MeshObject->GetSkeletalMeshRenderData().LODRenderData.Num() - 1) : INDEX_NONE;
		FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();

		FString DebugText = FString::Printf(TEXT("LOD(%d/%d) UpdateCounter(%d) EvalCounter(%d) CacheBoneCounter(%d) InitCounter(%d) DeltaSeconds(%.3f)"),
			SkelMeshComp->GetPredictedLODLevel(), MaxLODIndex, Proxy.GetUpdateCounter().Get(), Proxy.GetEvaluationCounter().Get(),
			Proxy.GetCachedBonesCounter().Get(), Proxy.GetInitializationCounter().Get(), Proxy.GetDeltaSeconds());

		DisplayDebugManager.DrawString(DebugText, Indent);

		if (SkelMeshComp->ShouldUseUpdateRateOptimizations())
		{
			if (FAnimUpdateRateParameters* UROParams = SkelMeshComp->AnimUpdateRateParams)
			{
				DebugText = FString::Printf(TEXT("URO Rate(%d) SkipUpdate(%d) SkipEval(%d) Interp(%d)"),
					UROParams->UpdateRate, UROParams->ShouldSkipUpdate(), UROParams->ShouldSkipEvaluation(),
					UROParams->ShouldInterpolateSkippedFrames());

				DisplayDebugManager.DrawString(DebugText, Indent);
			}
		}
	}
}

void UAnimInstance::DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
#if ENABLE_DRAW_DEBUG
	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();

	float Indent = 0.f;

	FLinearColor TextYellow(0.86f, 0.69f, 0.f);
	FLinearColor TextWhite(0.9f, 0.9f, 0.9f);
	FLinearColor ActiveColor(0.1f, 0.6f, 0.1f);
	FLinearColor InactiveColor(0.2f, 0.2f, 0.2f);
	FLinearColor PoseSourceColor(0.5f, 0.25f, 0.5f);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetLinearDrawColor(TextYellow);

	static FName CAT_SyncGroups(TEXT("SyncGroups"));
	static FName CAT_Montages(TEXT("Montages"));
	static FName CAT_Graph(TEXT("Graph"));
	static FName CAT_Curves(TEXT("Curves"));
	static FName CAT_Notifies(TEXT("Notifies"));
	static FName CAT_FullAnimGraph(TEXT("FullGraph"));
	static FName CAT_FullBlendspaceDisplay(TEXT("FullBlendspaceDisplay"));

	const bool bShowSyncGroups = DebugDisplay.IsCategoryToggledOn(CAT_SyncGroups, true);
	const bool bShowMontages = DebugDisplay.IsCategoryToggledOn(CAT_Montages, true);
	const bool bShowGraph = DebugDisplay.IsCategoryToggledOn(CAT_Graph, true);
	const bool bShowCurves = DebugDisplay.IsCategoryToggledOn(CAT_Curves, true);
	const bool bShowNotifies = DebugDisplay.IsCategoryToggledOn(CAT_Notifies, true);
	const bool bFullGraph = DebugDisplay.IsCategoryToggledOn(CAT_FullAnimGraph, false);
	const bool bFullBlendspaceDisplay = DebugDisplay.IsCategoryToggledOn(CAT_FullBlendspaceDisplay, true);

	FString Heading = FString::Printf(TEXT("Animation: %s"), *GetName());
	DisplayDebugManager.DrawString(Heading, Indent);

	{
		FIndenter CustomDebugIndent(Indent);
		DisplayDebugInstance(DisplayDebugManager, Indent);
	}

	if (bShowGraph && Proxy.HasRootNode())
	{
		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Anim Node Tree"));
		DisplayDebugManager.DrawString(Heading, Indent);

		const float NodeIndent = 8.f;
		const float LineIndent = 4.f;
		const float AttachLineLength = NodeIndent - LineIndent;

		FIndenter AnimNodeTreeIndent(Indent);

		DebugDataCounter.Increment();
		FNodeDebugData NodeDebugData(this);
		Proxy.GatherDebugData(NodeDebugData);

		TArray<FNodeDebugData::FFlattenedDebugData> FlattenedData = NodeDebugData.GetFlattenedDebugData();

		TArray<FVector2D> IndentLineStartCoord; // Index represents indent level, track the current starting point for that

		int32 PrevChainID = -1;

		for (FNodeDebugData::FFlattenedDebugData& Line : FlattenedData)
		{
			if (!Line.IsOnActiveBranch() && !bFullGraph)
			{
				continue;
			}
			float CurrIndent = Indent + (Line.Indent * NodeIndent);
			float CurrLineYBase = DisplayDebugManager.GetYPos() + DisplayDebugManager.GetMaxCharHeight();

			if (PrevChainID != Line.ChainID)
			{
				const int32 HalfStep = int32(DisplayDebugManager.GetMaxCharHeight() / 2);
				DisplayDebugManager.ShiftYDrawPosition(float(HalfStep)); // Extra spacing to delimit different chains, CurrLineYBase now
				// roughly represents middle of text line, so we can use it for line drawing
				// 大致代表文本行的中间，所以我们可以用它来画线

				//Handle line drawing
				//手柄线描
				int32 VerticalLineIndex = Line.Indent - 1;
				if (IndentLineStartCoord.IsValidIndex(VerticalLineIndex))
				{
					FVector2D LineStartCoord = IndentLineStartCoord[VerticalLineIndex];
					IndentLineStartCoord[VerticalLineIndex] = FVector2D(DisplayDebugManager.GetXPos(), CurrLineYBase);

					// If indent parent is not in same column, ignore line.
					// 如果缩进父级不在同一列中，则忽略该行。
					if (FMath::IsNearlyEqual(LineStartCoord.X, DisplayDebugManager.GetXPos()))
					{
						float EndX = DisplayDebugManager.GetXPos() + CurrIndent;
						float StartX = EndX - AttachLineLength;

						//horizontal line to node
						//到节点的水平线
						DrawDebugCanvas2DLine(Canvas, FVector(StartX, CurrLineYBase, 0.f), FVector(EndX, CurrLineYBase, 0.f), ActiveColor);

						//vertical line
						//垂线
						DrawDebugCanvas2DLine(Canvas, FVector(StartX, LineStartCoord.Y, 0.f), FVector(StartX, CurrLineYBase, 0.f), ActiveColor);
					}
				}

				CurrLineYBase += HalfStep; // move CurrYLineBase back to base of line
			}

			// Update our base position for subsequent line drawing
			// 更新我们的基本位置以供后续画线
			if (!IndentLineStartCoord.IsValidIndex(Line.Indent))
			{
				IndentLineStartCoord.AddZeroed(Line.Indent + 1 - IndentLineStartCoord.Num());
			}
			IndentLineStartCoord[Line.Indent] = FVector2D(DisplayDebugManager.GetXPos(), CurrLineYBase);

			PrevChainID = Line.ChainID;
			FLinearColor ItemColor = Line.bPoseSource ? PoseSourceColor : ActiveColor;
			DisplayDebugManager.SetLinearDrawColor(Line.IsOnActiveBranch() ? ItemColor : InactiveColor);
			DisplayDebugManager.DrawString(Line.DebugLine, CurrIndent);
		}
	}

	if (bShowSyncGroups)
	{
		FIndenter AnimIndent(Indent);

		//Display Sync Groups
		//显示同步组
		const FAnimInstanceProxy::FSyncGroupMap& SyncGroupMap = GetProxyOnGameThread<FAnimInstanceProxy>().GetSyncGroupMapRead();
		const TArray<FAnimTickRecord>& UngroupedActivePlayers = GetProxyOnGameThread<FAnimInstanceProxy>().GetUngroupedActivePlayersRead();

		Heading = FString::Printf(TEXT("SyncGroups: %i"), SyncGroupMap.Num());
		DisplayDebugManager.DrawString(Heading, Indent);

		for (const auto& SyncGroupPair : SyncGroupMap)
		{
			FIndenter GroupIndent(Indent);
			const FAnimGroupInstance& SyncGroup = SyncGroupPair.Value;

			DisplayDebugManager.SetLinearDrawColor(TextYellow);

			FString GroupLabel = FString::Printf(TEXT("Group %s - Players %i"), *SyncGroupPair.Key.ToString(), SyncGroup.ActivePlayers.Num());
			DisplayDebugManager.DrawString(GroupLabel, Indent);

			if (SyncGroup.ActivePlayers.Num() > 0)
			{
				check(SyncGroup.GroupLeaderIndex != -1);
				OutputTickRecords(SyncGroup.ActivePlayers, Canvas, Indent, SyncGroup.GroupLeaderIndex, TextWhite, ActiveColor, InactiveColor, DisplayDebugManager, bFullBlendspaceDisplay);
			}
		}

		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Ungrouped: %i"), UngroupedActivePlayers.Num());
		DisplayDebugManager.DrawString(Heading, Indent);

		DisplayDebugManager.SetLinearDrawColor(TextWhite);

		OutputTickRecords(UngroupedActivePlayers, Canvas, Indent, -1, TextWhite, ActiveColor, InactiveColor, DisplayDebugManager, bFullBlendspaceDisplay);
	}

	if (bShowMontages)
	{
		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Montages: %i"), MontageInstances.Num());
		DisplayDebugManager.DrawString(Heading, Indent);

		for (int32 MontageIndex = 0; MontageIndex < MontageInstances.Num(); ++MontageIndex)
		{
			FIndenter PlayerIndent(Indent);

			FAnimMontageInstance* MontageInstance = MontageInstances[MontageIndex];

			if (MontageInstance != nullptr && MontageInstance->Montage != nullptr)
			{
				DisplayDebugManager.SetLinearDrawColor((MontageInstance->IsActive()) ? ActiveColor : TextWhite);

				FString MontageEntry = FString::Printf(TEXT("%i) %s CurrSec: %s NextSec: %s W:%.2f DW:%.2f"), MontageIndex, *MontageInstance->Montage->GetName(), *MontageInstance->GetCurrentSection().ToString(), *MontageInstance->GetNextSection().ToString(), MontageInstance->GetWeight(), MontageInstance->GetDesiredWeight());
				DisplayDebugManager.DrawString(MontageEntry, Indent);
			}
		}
	}

	if (bShowNotifies)
	{
		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Active Notify States: %i"), ActiveAnimNotifyState.Num());
		DisplayDebugManager.DrawString(Heading, Indent);

		DisplayDebugManager.SetLinearDrawColor(TextWhite);

		for (int32 NotifyIndex = 0; NotifyIndex < ActiveAnimNotifyState.Num(); ++NotifyIndex)
		{
			FIndenter NotifyIndent(Indent);

			const FAnimNotifyEvent& NotifyState = ActiveAnimNotifyState[NotifyIndex];

			FString NotifyEntry = FString::Printf(TEXT("%i) %s Class: %s Dur:%.3f"), NotifyIndex, *NotifyState.NotifyName.ToString(), *NotifyState.NotifyStateClass->GetName(), NotifyState.GetDuration());
			DisplayDebugManager.DrawString(NotifyEntry, Indent);
		}
	}

	if (bShowCurves)
	{
		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Curves"));
		DisplayDebugManager.DrawString(Heading, Indent);

		{
			FIndenter CurveIndent(Indent);

			Heading = FString::Printf(TEXT("Morph Curves: %i"), Proxy.GetAnimationCurves(EAnimCurveType::MorphTargetCurve).Num());
			DisplayDebugManager.DrawString(Heading, Indent);

			DisplayDebugManager.SetLinearDrawColor(TextWhite);

			{
				FIndenter MorphCurveIndent(Indent);
				OutputCurveMap(Proxy.GetAnimationCurves(EAnimCurveType::MorphTargetCurve), Canvas, DisplayDebugManager, Indent);
			}

			DisplayDebugManager.SetLinearDrawColor(TextYellow);

			Heading = FString::Printf(TEXT("Material Curves: %i"), Proxy.GetAnimationCurves(EAnimCurveType::MaterialCurve).Num());
			DisplayDebugManager.DrawString(Heading, Indent);

			DisplayDebugManager.SetLinearDrawColor(TextWhite);

			{
				FIndenter MaterialCurveIndent(Indent);
				OutputCurveMap(Proxy.GetAnimationCurves(EAnimCurveType::MaterialCurve), Canvas, DisplayDebugManager, Indent);
			}

			DisplayDebugManager.SetLinearDrawColor(TextYellow);

			Heading = FString::Printf(TEXT("Event Curves: %i"), Proxy.GetAnimationCurves(EAnimCurveType::AttributeCurve).Num());
			DisplayDebugManager.DrawString(Heading, Indent);

			DisplayDebugManager.SetLinearDrawColor(TextWhite);

			{
				FIndenter EventCurveIndent(Indent);
				OutputCurveMap(Proxy.GetAnimationCurves(EAnimCurveType::AttributeCurve), Canvas, DisplayDebugManager, Indent);
			}
		}
	}
#endif // ENABLE_DRAW_DEBUG
}

void UAnimInstance::ResetDynamics(ETeleportType InTeleportType)
{
	PendingDynamicResetTeleportType = FMath::Max(InTeleportType, PendingDynamicResetTeleportType);
}

void UAnimInstance::ResetDynamics()
{
	LLM_SCOPE(ELLMTag::Animation);
	ResetDynamics(ETeleportType::ResetPhysics);
}


int32 UAnimInstance::GetLODLevel() const
{
	USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();
	check(SkelMeshComp)

	return SkelMeshComp->GetPredictedLODLevel();
}

void UAnimInstance::RecalcRequiredBones()
{
	LLM_SCOPE(ELLMTag::Animation);
	USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();
	check( SkelMeshComp )

	if( SkelMeshComp->GetSkeletalMeshAsset() && SkelMeshComp->GetSkeletalMeshAsset()->GetSkeleton() )
	{
		GetProxyOnGameThread<FAnimInstanceProxy>().RecalcRequiredBones(SkelMeshComp, SkelMeshComp->GetSkeletalMeshAsset());
	}
	else if( CurrentSkeleton != NULL )
	{
		GetProxyOnGameThread<FAnimInstanceProxy>().RecalcRequiredBones(SkelMeshComp, CurrentSkeleton);
	}
}

void UAnimInstance::RecalcRequiredCurves(const UE::Anim::FCurveFilterSettings& InCurveFilterSettings)
{
	GetProxyOnGameThread<FAnimInstanceProxy>().RecalcRequiredCurves(InCurveFilterSettings);
}

void UAnimInstance::RecalcRequiredCurves(const FCurveEvaluationOption& CurveEvalOption)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	GetProxyOnGameThread<FAnimInstanceProxy>().RecalcRequiredCurves(CurveEvalOption);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponentChecked() const
{
	return CastChecked<USkeletalMeshComponent>(GetOuter());
}

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponentUnchecked() const
{
	return Cast<USkeletalMeshComponent>(GetOuter());
}

void UAnimInstance::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (!Ar.IsLoading() || !Ar.IsSaving())
	{
		Ar << GetProxyOnAnyThread<FAnimInstanceProxy>().GetRequiredBones();
	}
}

bool UAnimInstance::CanTransitionSignature() const
{
	return false;
}

void UAnimInstance::BeginDestroy()
{
#if DO_CHECK
	if(GetOuter() && GetOuter()->IsA<USkeletalMeshComponent>())
	{
		check(!IsRunningParallelEvaluation());
	}
#endif
	if(AnimInstanceProxy != nullptr)
	{
		DestroyAnimInstanceProxy(AnimInstanceProxy);
		AnimInstanceProxy = nullptr;
	}

#if WITH_EDITOR
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		FCoreUObjectDelegates::OnObjectsReinstanced.RemoveAll(this);
	}
#endif // WITH_EDITOR

	Super::BeginDestroy();

	TRACE_OBJECT_LIFETIME_END(this);
}

void UAnimInstance::PostInitProperties()
{
	Super::PostInitProperties();

	if(AnimInstanceProxy == nullptr)
	{
		AnimInstanceProxy = CreateAnimInstanceProxy();
		check(AnimInstanceProxy != nullptr);
	}
}

void UAnimInstance::AddCurveValue(const FName& CurveName, float Value, bool bMorphtarget, bool bMaterial)
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().AddCurveValue(CurveName, Value, bMorphtarget, bMaterial);
}

void UAnimInstance::UpdateCurvesToComponents(USkeletalMeshComponent* Component /*= nullptr*/)
{
	// update curves to component
	// 将曲线更新为组件
	if (Component)
	{
		// this is only any thread because EndOfFrameUpdate update can restart render state
		// 这只是任何线程，因为 EndOfFrameUpdate 更新可以重新启动渲染状态
		// and this needs to restart from worker thread
		// 这需要从工作线程重新启动
		// EndOfFrameUpdate is done after all tick is updated, so in theory you shouldn't have
		// EndOfFrameUpdate 在所有刻度更新后完成，所以理论上你不应该
		FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
		Component->ApplyAnimationCurvesToComponent(&Proxy.GetAnimationCurves(EAnimCurveType::MaterialCurve), &Proxy.GetAnimationCurves(EAnimCurveType::MorphTargetCurve));
	}
}

void UAnimInstance::AppendAnimationCurveList(EAnimCurveType Type, TMap<FName, float>& InOutCurveList) const
{
	InOutCurveList.Append(GetProxyOnAnyThread<FAnimInstanceProxy>().GetAnimationCurves(Type));
}

void UAnimInstance::GetAnimationCurveList(EAnimCurveType Type, TMap<FName, float>& InOutCurveList) const
{
	AppendAnimationCurveList(Type, InOutCurveList);
}

const TMap<FName, float>& UAnimInstance::GetAnimationCurveList(EAnimCurveType Type) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetAnimationCurves(Type);
}

void UAnimInstance::RefreshCurves(USkeletalMeshComponent* Component)
{
	UpdateCurvesToComponents(Component);
}

void UAnimInstance::ResetAnimationCurves()
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().ResetAnimationCurves();
}

void UAnimInstance::CopyCurveValues(const UAnimInstance& InSourceInstance)
{
	FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
	const FAnimInstanceProxy& SourceProxy = InSourceInstance.GetProxyOnAnyThread<FAnimInstanceProxy>();

	for (uint8 i = 0; i < (uint8)EAnimCurveType::MaxAnimCurveType; i++)
	{
		Proxy.GetAnimationCurves((EAnimCurveType)i) = SourceProxy.GetAnimationCurves((EAnimCurveType)i);
	}
}

void UAnimInstance::UpdateCurvesToEvaluationContext(const FAnimationEvaluationContext& InContext)
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().UpdateCurvesToEvaluationContext(InContext);
}

void UAnimInstance::UpdateCurvesPostEvaluation()
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().UpdateCurvesPostEvaluation(GetSkelMeshComponent());
}

bool UAnimInstance::HasMorphTargetCurves() const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetAnimationCurves(EAnimCurveType::MorphTargetCurve).Num() > 0;
}

bool UAnimInstance::HasActiveCurves() const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().HasActiveCurves();
}

float UAnimInstance::GetDeltaSeconds() const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetDeltaSeconds();
}

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_AnimTriggerAnimNotifies);
	USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();

	// Array that will replace the 'ActiveAnimNotifyState' at the end of this function.
	// 将替换该函数末尾的“ActiveAnimNotifyState”的数组。
	TArray<FAnimNotifyEvent> NewActiveAnimNotifyState;
	NewActiveAnimNotifyState.Reserve(NotifyQueue.AnimNotifies.Num());

	TArray<FAnimNotifyEventReference> NewActiveAnimNotifyEventReference;
	NewActiveAnimNotifyEventReference.Reserve(NotifyQueue.AnimNotifies.Num());


	// AnimNotifyState freshly added that need their 'NotifyBegin' event called.
	// 新添加的 AnimNotifyState 需要调用其“NotifyBegin”事件。
	TArray<const FAnimNotifyEvent *> NotifyStateBeginEvent;
	TArray<const FAnimNotifyEventReference *> NotifyStateBeginEventReference;

	for (int32 Index=0; Index<NotifyQueue.AnimNotifies.Num(); Index++)
	{
		if(const FAnimNotifyEvent* AnimNotifyEvent = NotifyQueue.AnimNotifies[Index].GetNotify())
		{
			// AnimNotifyState
			// 动画通知状态
			if (AnimNotifyEvent->NotifyStateClass)
			{
				int32 ExistingItemIndex = INDEX_NONE;

				if (ActiveAnimNotifyState.Find(*AnimNotifyEvent, ExistingItemIndex))
				{
					check(ActiveAnimNotifyState.Num() == ActiveAnimNotifyEventReference.Num());
					ActiveAnimNotifyState.RemoveAtSwap(ExistingItemIndex, EAllowShrinking::No);
					ActiveAnimNotifyEventReference.RemoveAtSwap(ExistingItemIndex, EAllowShrinking::No);
				}
				else
				{
					NotifyStateBeginEvent.Add(AnimNotifyEvent);
					NotifyStateBeginEventReference.Add(&NotifyQueue.AnimNotifies[Index]);
				}

				NewActiveAnimNotifyState.Add(*AnimNotifyEvent);
				FAnimNotifyEventReference& EventRef = NewActiveAnimNotifyEventReference.Add_GetRef(NotifyQueue.AnimNotifies[Index]);
				EventRef.SetNotify(&NewActiveAnimNotifyState.Top());
				continue;
			}

			// Trigger non 'state' AnimNotifies
			// 触发非“状态”AnimNotify
			TriggerSingleAnimNotify(NotifyQueue.AnimNotifies[Index]);
		}
	}

	// Send end notification to AnimNotifyState not active anymore.
	// 向不再活动的 AnimNotifyState 发送结束通知。
	for (int32 Index = 0; Index < ActiveAnimNotifyState.Num(); ++Index)
	{
		const FAnimNotifyEvent& AnimNotifyEvent = ActiveAnimNotifyState[Index];
		const FAnimNotifyEventReference& EventReference = ActiveAnimNotifyEventReference[Index];
		if (AnimNotifyEvent.NotifyStateClass && ShouldTriggerAnimNotifyState(AnimNotifyEvent.NotifyStateClass))
		{
#if WITH_EDITOR
			// Prevent firing notifies in animation editors if requested
			// 如果需要，防止在动画编辑器中触发通知
			if((SkelMeshComp && !SkelMeshComp->IsA<UDebugSkelMeshComponent>()) || AnimNotifyEvent.NotifyStateClass->ShouldFireInEditor())
#endif
			{
				TRACE_ANIM_NOTIFY(this, AnimNotifyEvent, End);
				AnimNotifyEvent.NotifyStateClass->NotifyEnd(SkelMeshComp, Cast<UAnimSequenceBase>(AnimNotifyEvent.NotifyStateClass->GetOuter()), EventReference);
			}
		}
		// The NotifyEnd callback above may have triggered actor destruction and the tear down
		// 上面的 NotifyEnd 回调可能触发了 actor 销毁和拆卸
		// of this instance via UninitializeAnimation which empties ActiveAnimNotifyState.
		// 该实例通过 UninitializeAnimation 清空 ActiveAnimNotifyState。
		// If that happened, we should stop iterating the ActiveAnimNotifyState array
		// 如果发生这种情况，我们应该停止迭代 ActiveAnimNotifyState 数组
		if (ActiveAnimNotifyState.IsValidIndex(Index) == false)
		{
			ensureMsgf(false, TEXT("UAnimInstance::ActiveAnimNotifyState has been invalidated by NotifyEnd. AnimInstance: %s, Owning Component: %s, Owning Actor: %s "), *GetNameSafe(this), *GetNameSafe(GetOwningComponent()), *GetNameSafe(GetOwningActor()));
			return;
		}
	}

	check(NotifyStateBeginEventReference.Num() == NotifyStateBeginEvent.Num());
	for (int32 Index = 0; Index < NotifyStateBeginEvent.Num(); Index++)
	{
		const FAnimNotifyEvent* AnimNotifyEvent = NotifyStateBeginEvent[Index];
		const FAnimNotifyEventReference * AnimNotifyEventReference = NotifyStateBeginEventReference[Index];
		if (ShouldTriggerAnimNotifyState(AnimNotifyEvent->NotifyStateClass))
		{
#if WITH_EDITOR
			// Prevent firing notifies in animation editors if requested
			// 如果需要，防止在动画编辑器中触发通知
			if((SkelMeshComp && !SkelMeshComp->IsA<UDebugSkelMeshComponent>()) || AnimNotifyEvent->NotifyStateClass->ShouldFireInEditor())
#endif
			{
				TRACE_ANIM_NOTIFY(this, *AnimNotifyEvent, Begin);
				AnimNotifyEvent->NotifyStateClass->NotifyBegin(SkelMeshComp, Cast<UAnimSequenceBase>(AnimNotifyEvent->NotifyStateClass->GetOuter()), AnimNotifyEvent->GetDuration(), *AnimNotifyEventReference);
			}
		}
	}

	// Switch our arrays.
	// 切换我们的阵列。
	ActiveAnimNotifyState = MoveTemp(NewActiveAnimNotifyState);
	ActiveAnimNotifyEventReference = MoveTemp(NewActiveAnimNotifyEventReference);
	// Tick currently active AnimNotifyState
	// 勾选当前活动的 AnimNotifyState
	for (int32 Index = 0; Index < ActiveAnimNotifyState.Num(); Index++)
	{
		const FAnimNotifyEvent& AnimNotifyEvent = ActiveAnimNotifyState[Index];
		const FAnimNotifyEventReference& EventReference = ActiveAnimNotifyEventReference[Index];
		if (ShouldTriggerAnimNotifyState(AnimNotifyEvent.NotifyStateClass))
		{
#if WITH_EDITOR
			// Prevent firing notifies in animation editors if requested
			// 如果需要，防止在动画编辑器中触发通知
			if((SkelMeshComp && !SkelMeshComp->IsA<UDebugSkelMeshComponent>()) || AnimNotifyEvent.NotifyStateClass->ShouldFireInEditor())
#endif
			{
				TRACE_ANIM_NOTIFY(this, AnimNotifyEvent, Tick);
				AnimNotifyEvent.NotifyStateClass->NotifyTick(SkelMeshComp, Cast<UAnimSequenceBase>(AnimNotifyEvent.NotifyStateClass->GetOuter()), DeltaSeconds, EventReference);
			}
		}
	}
}

void UAnimInstance::TriggerSingleAnimNotify(const FAnimNotifyEvent* AnimNotifyEvent)
{
	FAnimNotifyEventReference EventReference(AnimNotifyEvent, this);

	TriggerSingleAnimNotify(EventReference);
}

void UAnimInstance::TriggerSingleAnimNotify(FAnimNotifyEventReference& EventReference)
{
	// This is for non 'state' anim notifies.
	// 这是针对非“状态”动画通知的。
	const FAnimNotifyEvent* AnimNotifyEvent = EventReference.GetNotify();
	if (AnimNotifyEvent && (AnimNotifyEvent->NotifyStateClass == NULL))
	{
		if (HandleNotify(*AnimNotifyEvent))
		{
			return;
		}

		if (AnimNotifyEvent->Notify != nullptr)
		{
#if WITH_EDITOR
			// Prevent firing notifies in animation editors if requested
			// 如果需要，防止在动画编辑器中触发通知
			if(!GetSkelMeshComponent()->IsA<UDebugSkelMeshComponent>() || AnimNotifyEvent->Notify->ShouldFireInEditor())
#endif
			{
				// Implemented notify: just call Notify. UAnimNotify will forward this to the event which will do the work.
				// 实现通知：只需调用Notify即可。 UAnimNotify 会将其转发给执行该工作的事件。
				TRACE_ANIM_NOTIFY(this, *AnimNotifyEvent, Event);
				AnimNotifyEvent->Notify->Notify(GetSkelMeshComponent(), Cast<UAnimSequenceBase>(AnimNotifyEvent->Notify->GetOuter()), EventReference);
			}
		}
		else if (AnimNotifyEvent->NotifyName != NAME_None)
		{
			// Custom Event based notifies. These will call a AnimNotify_* function on the AnimInstance.
			// 基于自定义事件的通知。这些将调用 AnimInstance 上的 AnimNotify_* 函数。
			FName FuncName = AnimNotifyEvent->GetNotifyEventName(EventReference.GetMirrorDataTable());

			auto NotifyAnimInstance = [this, AnimNotifyEvent, FuncName](UAnimInstance* InAnimInstance)
			{
				check(InAnimInstance);

				if (InAnimInstance == this || InAnimInstance->bReceiveNotifiesFromLinkedInstances)
				{
					TRACE_ANIM_NOTIFY(this, *AnimNotifyEvent, Event);

					UFunction* Function = InAnimInstance->FindFunction(FuncName);
					if (Function)
					{
						// if parameter is none, add event
						// 如果参数为无，则添加事件
						if (Function->NumParms == 0)
						{
							InAnimInstance->ProcessEvent(Function, nullptr);
						}
						else if ((Function->NumParms == 1) && (CastField<FObjectProperty>(Function->PropertyLink) != nullptr))
						{
							struct FAnimNotifierHandler_Parms
							{
								UAnimNotify* Notify;
							};

							FAnimNotifierHandler_Parms Parms;
							Parms.Notify = AnimNotifyEvent->Notify;
							InAnimInstance->ProcessEvent(Function, &Parms);
						}
						else
						{
							// Actor has event, but with different parameters. Print warning
							// Actor 有事件，但参数不同。打印警告
							UE_LOG(LogAnimNotify, Warning, TEXT("Anim notifier named %s, but the parameter number does not match or not of the correct type"), *FuncName.ToString());
						}
					}
				}
			};


			if (FSimpleMulticastDelegate* ExistingDelegate = ExternalNotifyHandlers.Find(FuncName))
			{
				ExistingDelegate->Broadcast();
			}

			if(bPropagateNotifiesToLinkedInstances)
			{
				GetSkelMeshComponent()->ForEachAnimInstance(NotifyAnimInstance);
			}
			else
			{
				NotifyAnimInstance(this);
			}
		}
	}
}

void UAnimInstance::EndNotifyStates()
{
	USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();

	for (int32 Index = 0; Index < ActiveAnimNotifyState.Num(); Index++)
	{
		const FAnimNotifyEvent& Event = ActiveAnimNotifyState[Index];
		const FAnimNotifyEventReference& EventReference = ActiveAnimNotifyEventReference[Index];
		if (UAnimNotifyState* NotifyState = Event.NotifyStateClass)
		{
#if WITH_EDITOR
			// Prevent firing notifies in animation editors if requested
			// 如果需要，防止在动画编辑器中触发通知
			if(!SkelMeshComp->IsA<UDebugSkelMeshComponent>() || NotifyState->ShouldFireInEditor())
#endif
			{
				TRACE_ANIM_NOTIFY(this, Event, End);
				NotifyState->NotifyEnd(SkelMeshComp, Cast<UAnimSequenceBase>(NotifyState->GetOuter()), EventReference);
			}
		}
	}
	ActiveAnimNotifyState.Reset();
	ActiveAnimNotifyEventReference.Reset();
}

//to debug montage weight
//调试蒙太奇权重
#define DEBUGMONTAGEWEIGHT 0

float UAnimInstance::GetSlotNodeGlobalWeight(const FName& SlotNodeName) const
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetSlotNodeGlobalWeight(SlotNodeName);
}

float UAnimInstance::GetSlotMontageGlobalWeight(const FName& SlotNodeName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetSlotMontageGlobalWeight(SlotNodeName);
}

float UAnimInstance::GetSlotMontageLocalWeight(const FName& SlotNodeName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetSlotMontageLocalWeight(SlotNodeName);
}

float UAnimInstance::CalcSlotMontageLocalWeight(const FName& SlotNodeName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().CalcSlotMontageLocalWeight(SlotNodeName);
}

float UAnimInstance::GetCurveValue(FName CurveName) const
{
	float Value = 0.f;
	GetCurveValue(CurveName, Value);
	return Value;
}

bool UAnimInstance::GetCurveValue(FName CurveName, float& OutValue) const
{
	const FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();

	const float* Value = Proxy.GetAnimationCurves(EAnimCurveType::AttributeCurve).Find(CurveName);
	if (Value)
	{
		OutValue = *Value;
		return true;
	}

	return false;
}

bool UAnimInstance::GetCurveValueWithDefault(FName CurveName, float DefaultValue, float& OutValue)
{
	OutValue = DefaultValue;
	return GetCurveValue(CurveName, OutValue);
}

void UAnimInstance::GetActiveCurveNames(EAnimCurveType CurveType, TArray<FName>& OutNames) const
{
	TMap<FName, float> ActiveCurves;

	AppendAnimationCurveList(CurveType, ActiveCurves);
	ActiveCurves.GetKeys(OutNames);
}

void UAnimInstance::GetAllCurveNames(TArray<FName>& OutNames) const
{
	GetActiveCurveNames(EAnimCurveType::AttributeCurve, OutNames);
}

void UAnimInstance::OverrideCurveValue(FName CurveName, float Value)
{
	FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();

	TMap<FName, float>& AnimationCurves = Proxy.GetAnimationCurves(EAnimCurveType::AttributeCurve);
	AnimationCurves.FindOrAdd(CurveName) = Value;
}

void UAnimInstance::SetRootMotionMode(TEnumAsByte<ERootMotionMode::Type> Value)
{
	RootMotionMode = Value;
}


FName UAnimInstance::GetCurrentStateName(int32 MachineIndex)
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		const TArray<FStructProperty*>& AnimNodeProperties = AnimBlueprintClass->GetAnimNodeProperties();
		if ((MachineIndex >= 0) && (MachineIndex < AnimNodeProperties.Num()))
		{
			const int32 InstancePropertyIndex = AnimNodeProperties.Num() - 1 - MachineIndex; //@TODO: ANIMREFACTOR: Reverse indexing

			FStructProperty* MachineInstanceProperty = AnimNodeProperties[InstancePropertyIndex];
			checkSlow(MachineInstanceProperty->Struct->IsChildOf(FAnimNode_StateMachine::StaticStruct()));

			FAnimNode_StateMachine* MachineInstance = MachineInstanceProperty->ContainerPtrToValuePtr<FAnimNode_StateMachine>(this);

			return MachineInstance->GetCurrentStateName();
		}
	}

	return NAME_None;
}

void UAnimInstance::Montage_UpdateWeight(float DeltaSeconds)
{
	if (MontageInstances.IsEmpty())
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_Montage_UpdateWeight);

	// go through all montage instances, and update them
	// 遍历所有蒙太奇实例并更新它们
	// and make sure their weight is updated properly
	// 并确保他们的体重正确更新
	for (int32 I=0; I<MontageInstances.Num(); ++I)
	{
		if ( MontageInstances[I] )
		{
			MontageInstances[I]->UpdateWeight(DeltaSeconds);
		}
	}
}

void UAnimInstance::Montage_Advance(float DeltaSeconds)
{
	// We're about to tick montages, queue their events to they're triggered after batched anim notifies.
	// 我们将勾选蒙太奇，将其事件排队，以便在批量动画通知后触发它们。
	bQueueMontageEvents = true;

	if (MontageInstances.IsEmpty())
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_Montage_Advance);

	// go through all montage instances, and update them
	// 遍历所有蒙太奇实例并更新它们
	// and make sure their weight is updated properly
	// 并确保他们的体重正确更新
	for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
	{
		FAnimMontageInstance* const MontageInstance = MontageInstances[InstanceIndex];
		// should never be NULL
		// [翻译失败: should never be NULL]
		ensure(MontageInstance);
		if (MontageInstance && IsValid(MontageInstance->Montage))
		{
			bool const bUsingBlendedRootMotion = (RootMotionMode == ERootMotionMode::RootMotionFromEverything);
			bool const bNoRootMotionExtraction = (RootMotionMode == ERootMotionMode::NoRootMotionExtraction);

			// Extract root motion if we are using blend root motion (RootMotionFromEverything) or if we are set to extract root
			// [翻译失败: Extract root motion if we are using blend root motion (RootMotionFromEverything) or if we are set to extract root]
			// motion AND we are the active root motion instance. This is so we can make root motion deterministic for networking when
			// [翻译失败: motion AND we are the active root motion instance. This is so we can make root motion deterministic for networking when]
			// we are not using RootMotionFromEverything
			// 我们没有使用 RootMotionFromEverything
			bool const bExtractRootMotion = !MontageInstance->IsRootMotionDisabled() && (bUsingBlendedRootMotion || (!bNoRootMotionExtraction && (MontageInstance == GetRootMotionMontageInstance())));

			FRootMotionMovementParams LocalExtractedRootMotion;
			FRootMotionMovementParams* RootMotionParams = nullptr;
			if (bExtractRootMotion)
			{
				RootMotionParams = (RootMotionMode != ERootMotionMode::IgnoreRootMotion) ? &ExtractedRootMotion : &LocalExtractedRootMotion;
			}

			MontageInstance->MontageSync_PreUpdate();
			MontageInstance->Advance(DeltaSeconds, RootMotionParams, bUsingBlendedRootMotion);

			// If MontageInstances has been modified while executing MontageInstance->Advance(), MontageInstance is unsafe to
			// [翻译失败: If MontageInstances has been modified while executing MontageInstance->Advance(), MontageInstance is unsafe to]
			// access further. This happens for example if MontageInstance->Advance() triggers an anim notify in which the user
			// [翻译失败: access further. This happens for example if MontageInstance->Advance() triggers an anim notify in which the user]
			// destroys the owning actor which in turn calls UninitializeAnimation(), or when the anim notify causes any montage
			// 销毁拥有的 Actor，进而调用 UninitializeAnimation()，或者当动画通知导致任何蒙太奇时
			// to stop or start playing. We just check here if the current MontageInstance is still safe to access.
			// 停止或开始播放。我们只是在这里检查当前的 MontageInstance 是否仍然可以安全访问。
			if (!MontageInstances.IsValidIndex(InstanceIndex) || MontageInstances[InstanceIndex] != MontageInstance)
			{
				break;
			}

			MontageInstance->MontageSync_PostUpdate();

#if DO_CHECK && WITH_EDITORONLY_DATA && 0
			// We need to re-check IsValid() here because Advance() could have terminated this Montage.
			// 我们需要在这里重新检查 IsValid()，因为 Advance() 可能会终止此 Montage。
			if (MontageInstance.IsValid())
			{
				// print blending time and weight and montage name
				// 打印混合时间和重量以及蒙太奇名称
				UE_LOG(LogAnimMontage, Warning, TEXT("%d. Montage (%s), DesiredWeight(%0.2f), CurrentWeight(%0.2f), BlendingTime(%0.2f)"),
					I + 1, *MontageInstance->Montage->GetName(), MontageInstance->GetDesiredWeight(), MontageInstance->GetWeight(),
					MontageInstance->GetBlendTime());
			}
#endif
		}
	}
}

void UAnimInstance::RequestSlotGroupInertialization(FName InSlotGroupName, float Duration, const UBlendProfile* BlendProfile)
{
	FInertializationRequest Request;
	Request.Duration = Duration;
	Request.BlendProfile = BlendProfile;

	// Must add this on both the anim instance and proxy's map, as this could called after UAnimInstance::UpdateMontageEvaluationData.
	// 必须将其添加到动画实例和代理的地图上，因为这可以在 UAnimInstance::UpdateMontageEvaluationData 之后调用。
	SlotGroupInertializationRequestDataMap.FindOrAdd(InSlotGroupName) = Request;
	GetProxyOnAnyThread<FAnimInstanceProxy>().GetSlotGroupInertializationRequestDataMap().FindOrAdd(InSlotGroupName) = Request;
}

void UAnimInstance::ConditionalFlushCompletedMontages()
{
	if (MontageInstances.IsEmpty())
	{
		// No montages, nothing to do.
		// [翻译失败: No montages, nothing to do.]
		return;
	}

	const USkeletalMeshComponent* MeshComp = GetSkelMeshComponent();
	check(MeshComp);

	// If we ticked this frame, then montages are not paused
	// [翻译失败: If we ticked this frame, then montages are not paused]
	// If we don't tick animation, then montages are paused
	// 如果我们不勾选动画，那么蒙太奇就会暂停
	// If we don't tick the pose, then montages are paused
	// 如果我们不勾选姿势，那么蒙太奇就会暂停
	const bool bTickedThisFrame = MeshComp->PoseTickedThisFrame();
	const bool bShouldTickAnimation = MeshComp->ShouldTickAnimation();
	const bool bShouldTickPose = MeshComp->ShouldTickPose();
	const bool bShouldFlush = !bTickedThisFrame && (!bShouldTickAnimation || !bShouldTickPose);

	if (bShouldFlush)
	{
		uint32 PrevMontageFlushFrame = LastMontageFlushFrame;

		// Don't care about roll over, just care about uniqueness (and 32-bits should give plenty).
		// 不关心翻转，只关心唯一性（32 位应该足够了）。
		LastMontageFlushFrame = static_cast<uint32>(GFrameCounter);

		// If we already flushed montages this frame, then there is no need to do so again
		// 如果我们已经刷新了该帧的蒙太奇，则无需再次执行此操作
		const bool bFlushedMontagesThisFrame = LastMontageFlushFrame == PrevMontageFlushFrame;

		if (!bFlushedMontagesThisFrame)
		{
			for (int32 InstanceIndex = MontageInstances.Num() - 1; InstanceIndex >= 0; InstanceIndex--)
			{
				if (MontageInstances.IsValidIndex(InstanceIndex))
				{
					FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
					if (MontageInstance && MontageInstance->IsValid() && MontageInstance->IsStopped() && MontageInstance->GetBlend().IsComplete())
					{
						// Need this to trigger Montage ended events.
						// [翻译失败: Need this to trigger Montage ended events.]
						MontageInstance->Terminate();

						// UninitializeAnimation() empties the MontagesInstances array thus we need to exit early.
						// [翻译失败: UninitializeAnimation() empties the MontagesInstances array thus we need to exit early.]
						if (bUninitialized && MontageCVars::bEarlyOutMontageWhenUninitialized)
						{
							return;
						}
					}
				}
			}

			// After triggering the Montage Blend Out Events, it's now safe to delete invalid Montage Instances.
			// [翻译失败: After triggering the Montage Blend Out Events, it's now safe to delete invalid Montage Instances.]
			// And dispatch 'OnAllMontageInstancesEnded'
			// 并调度“OnAllMontageInstancesEnded”
			for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
			{
				// Should never be null
				// 永远不应该为空
				FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
				ensure(MontageInstance);
				if (MontageInstance && !MontageInstance->IsValid())
				{
					// Make sure we've cleared our references before deleting memory
					// 确保在删除内存之前我们已经清除了引用
					ClearMontageInstanceReferences(*MontageInstance);

					delete MontageInstance;
					MontageInstances.RemoveAt(InstanceIndex);
					--InstanceIndex;

					if (MontageInstances.Num() == 0)
					{
						OnAllMontageInstancesEnded.Broadcast();
					}
				}
			}
		}
	}
}

void UAnimInstance::RequestMontageInertialization(const UAnimMontage* Montage, const FInertializationRequest& Request)
{
	if (Montage)
	{
		// Adds a new request or overwrites an existing one
		// 添加新请求或覆盖现有请求
		// We always overwrite with the last request, instead of using the shortest one (differs from AnimNode_Inertialization), because we expect the last montage played/stopped to take precedence
		// 我们总是用最后一个请求覆盖，而不是使用最短的请求（与 AnimNode_Inertialization 不同），因为我们希望最后一个播放/停止的蒙太奇优先
		SlotGroupInertializationRequestDataMap.FindOrAdd(Montage->GetGroupName()) = Request;
	}
}

void UAnimInstance::RequestMontageInertialization(const UAnimMontage* Montage, float Duration, const UBlendProfile* BlendProfile)
{
	FInertializationRequest Request;
	Request.Duration = Duration;
	Request.BlendProfile = BlendProfile;

	RequestMontageInertialization(Montage, Request);
}

void UAnimInstance::QueueMontageBlendingOutEvent(const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent)
{
	if (bQueueMontageEvents)
	{
		QueuedMontageBlendingOutEvents.Add(MontageBlendingOutEvent);
	}
	else
	{
		TriggerMontageBlendingOutEvent(MontageBlendingOutEvent);
	}
}

void UAnimInstance::QueueMontageBlendedInEvent(const FQueuedMontageBlendedInEvent& MontageBlendedInEvent)
{
	if (bQueueMontageEvents)
	{
		QueuedMontageBlendedInEvents.Add(MontageBlendedInEvent);
	}
	else
	{
		TriggerMontageBlendedInEvent(MontageBlendedInEvent);
	}
}

void UAnimInstance::TriggerMontageBlendingOutEvent(const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent)
{
	MontageBlendingOutEvent.Delegate.ExecuteIfBound(MontageBlendingOutEvent.Montage, MontageBlendingOutEvent.bInterrupted);
	OnMontageBlendingOut.Broadcast(MontageBlendingOutEvent.Montage, MontageBlendingOutEvent.bInterrupted);
}

void UAnimInstance::TriggerMontageBlendedInEvent(const FQueuedMontageBlendedInEvent& MontageBlendedInEvent)
{
	MontageBlendedInEvent.Delegate.ExecuteIfBound(MontageBlendedInEvent.Montage);
	OnMontageBlendedIn.Broadcast(MontageBlendedInEvent.Montage);
}

void UAnimInstance::QueueMontageEndedEvent(const FQueuedMontageEndedEvent& MontageEndedEvent)
{
	if (bQueueMontageEvents)
	{
		QueuedMontageEndedEvents.Add(MontageEndedEvent);
	}
	else
	{
		TriggerMontageEndedEvent(MontageEndedEvent);
	}
}

void UAnimInstance::QueueMontageSectionChangedEvent(const FQueuedMontageSectionChangedEvent& MontageSectionChangedEvent)
{
	if (bQueueMontageEvents)
	{
		QueuedMontageSectionChangedEvents.Add(MontageSectionChangedEvent);
	}
	else
	{
		TriggerMontageSectionChangedEvent(MontageSectionChangedEvent);
	}
}

void UAnimInstance::TriggerMontageEndedEvent(const FQueuedMontageEndedEvent& MontageEndedEvent)
{
	// Send end notifications for anim notify state when we are stopped
	// 当我们停止时发送动画通知状态的结束通知
	USkeletalMeshComponent* SkelMeshComp = GetOwningComponent();

	if (SkelMeshComp != nullptr)
	{
		for (int32 Index = ActiveAnimNotifyState.Num() - 1; ActiveAnimNotifyState.IsValidIndex(Index); --Index)
		{
			const FAnimNotifyEvent& AnimNotifyEvent = ActiveAnimNotifyState[Index];
			const FAnimNotifyEventReference& EventReference = ActiveAnimNotifyEventReference[Index];
			UAnimMontage* NotifyMontage = Cast<UAnimMontage>(AnimNotifyEvent.NotifyStateClass->GetOuter());

			// Grab the montage instance ID from the notify's event context
			// 从通知的事件上下文中获取蒙太奇实例 ID
			int32 EventReferenceMontageInstanceID = INDEX_NONE;
			const UE::Anim::FAnimNotifyMontageInstanceContext* ActiveMontageContext = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
			if (ActiveMontageContext)
			{
				EventReferenceMontageInstanceID = ActiveMontageContext->MontageInstanceID;
			}

			// Compare against the montage instance ID to prevent ending notify states from other instances of the same montage
			// 与蒙太奇实例 ID 进行比较，以防止来自同一蒙太奇的其他实例的结束通知状态
			if (NotifyMontage && (EventReferenceMontageInstanceID == MontageEndedEvent.MontageInstanceID))
			{
				if (ShouldTriggerAnimNotifyState(AnimNotifyEvent.NotifyStateClass))
				{
#if WITH_EDITOR
					// Prevent firing notifies in animation editors if requested
					// 如果需要，防止在动画编辑器中触发通知
					if(!SkelMeshComp->IsA<UDebugSkelMeshComponent>() || AnimNotifyEvent.NotifyStateClass->ShouldFireInEditor())
#endif
					{
						TRACE_ANIM_NOTIFY(this, AnimNotifyEvent, End);
						AnimNotifyEvent.NotifyStateClass->NotifyEnd(SkelMeshComp, NotifyMontage, EventReference);
					}
				}

				if (ActiveAnimNotifyState.IsValidIndex(Index))
				{
					check(ActiveAnimNotifyState.Num() == ActiveAnimNotifyEventReference.Num());
					ActiveAnimNotifyState.RemoveAtSwap(Index);
					ActiveAnimNotifyEventReference.RemoveAtSwap(Index);
				}
				else
				{
					// The NotifyEnd callback above may have triggered actor destruction and the tear down
					// [翻译失败: The NotifyEnd callback above may have triggered actor destruction and the tear down]
					// of this instance via UninitializeAnimation which empties ActiveAnimNotifyState.
					// [翻译失败: of this instance via UninitializeAnimation which empties ActiveAnimNotifyState.]
					// If that happened, we should stop iterating the ActiveAnimNotifyState array and bail
					// 如果发生这种情况，我们应该停止迭代 ActiveAnimNotifyState 数组并进行保释
					// without attempting to send MontageEnded events.
					// 而不尝试发送 MontageEnded 事件。
					return;
				}
			}
		}
	}

	MontageEndedEvent.Delegate.ExecuteIfBound(MontageEndedEvent.Montage, MontageEndedEvent.bInterrupted);
	OnMontageEnded.Broadcast(MontageEndedEvent.Montage, MontageEndedEvent.bInterrupted);
}

void UAnimInstance::TriggerMontageSectionChangedEvent(const FQueuedMontageSectionChangedEvent& MontageSectionChangedEvent)
{
	MontageSectionChangedEvent.Delegate.ExecuteIfBound(MontageSectionChangedEvent.Montage, MontageSectionChangedEvent.SectionName, MontageSectionChangedEvent.bLooped);
	OnMontageSectionChanged.Broadcast(MontageSectionChangedEvent.Montage, MontageSectionChangedEvent.SectionName, MontageSectionChangedEvent.bLooped);
}

void UAnimInstance::TriggerQueuedMontageEvents()
{
	// We don't need to queue montage events anymore.
	// 我们不再需要对蒙太奇事件进行排队。
	bQueueMontageEvents = false;

	// Trigger Montage blending out before Ended events.
	// 在结束事件之前触发蒙太奇混合。
	if (QueuedMontageBlendingOutEvents.Num() > 0)
	{
		for (const FQueuedMontageBlendingOutEvent& MontageBlendingOutEvent : QueuedMontageBlendingOutEvents)
		{
			TriggerMontageBlendingOutEvent(MontageBlendingOutEvent);
		}
		QueuedMontageBlendingOutEvents.Reset();
	}

	if (QueuedMontageBlendedInEvents.Num() > 0)
	{
		for (const FQueuedMontageBlendedInEvent& MontageBlendedInEvent : QueuedMontageBlendedInEvents)
		{
			TriggerMontageBlendedInEvent(MontageBlendedInEvent);
		}
		QueuedMontageBlendedInEvents.Reset();
	}

	if (QueuedMontageSectionChangedEvents.Num() > 0)
	{
		for (const FQueuedMontageSectionChangedEvent& MontageSectionChangedEvent : QueuedMontageSectionChangedEvents)
		{
			TriggerMontageSectionChangedEvent(MontageSectionChangedEvent);
		}
		QueuedMontageSectionChangedEvents.Reset();
	}

	if (QueuedMontageEndedEvents.Num() > 0)
	{
		for (const FQueuedMontageEndedEvent& MontageEndedEvent : QueuedMontageEndedEvents)
		{
			TriggerMontageEndedEvent(MontageEndedEvent);
		}
		QueuedMontageEndedEvents.Reset();
	}
}

UAnimMontage* UAnimInstance::PlaySlotAnimationAsDynamicMontage(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime, float BlendOutTime, float InPlayRate, int32 LoopCount, float BlendOutTriggerTime, float InTimeToStartMontageAt)
{
	FMontageBlendSettings BlendInSettings(BlendInTime);
	FMontageBlendSettings BlendOutSettings(BlendOutTime);
	return PlaySlotAnimationAsDynamicMontage_WithBlendSettings(Asset, SlotNodeName, BlendInSettings, BlendOutSettings, InPlayRate, LoopCount, BlendOutTriggerTime, InTimeToStartMontageAt);
}

UAnimMontage* UAnimInstance::PlaySlotAnimationAsDynamicMontage_WithBlendArgs(UAnimSequenceBase* Asset, FName SlotNodeName, const FAlphaBlendArgs& BlendIn, const FAlphaBlendArgs& BlendOut, float InPlayRate, int32 LoopCount, float BlendOutTriggerTime, float InTimeToStartMontageAt)
{
	FMontageBlendSettings BlendInSettings(BlendIn);
	FMontageBlendSettings BlendOutSettings(BlendOut);
	return PlaySlotAnimationAsDynamicMontage_WithBlendSettings(Asset, SlotNodeName, BlendInSettings, BlendOutSettings, InPlayRate, LoopCount, BlendOutTriggerTime, InTimeToStartMontageAt);
}

UAnimMontage* UAnimInstance::PlaySlotAnimationAsDynamicMontage_WithBlendSettings(UAnimSequenceBase* Asset, FName SlotNodeName, const FMontageBlendSettings& BlendInSettings, const FMontageBlendSettings& BlendOutSettings, float InPlayRate, int32 LoopCount, float BlendOutTriggerTime, float InTimeToStartMontageAt)
{
	if (Asset && Asset->GetSkeleton())
	{
		// create asset using the information
		// 使用信息创建资产
		UAnimMontage* NewMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage_WithBlendSettings(Asset, SlotNodeName, BlendInSettings, BlendOutSettings, InPlayRate, LoopCount, BlendOutTriggerTime);

		if (NewMontage)
		{
			// if playing is successful, return the montage to allow more control if needed
			// 如果播放成功，则返回蒙太奇以在需要时进行更多控制
			float PlayTime = Montage_Play(NewMontage, InPlayRate, EMontagePlayReturnType::MontageLength, InTimeToStartMontageAt);
			return PlayTime > 0.0f ? NewMontage : nullptr;
		}
	}

	return nullptr;
}

void UAnimInstance::StopSlotAnimation(float InBlendOutTime, FName SlotNodeName)
{
	// stop temporary montage
	// 停止临时蒙太奇
	// when terminate (in the Montage_Advance), we have to lose reference to the temporary montage
	// [翻译失败: when terminate (in the Montage_Advance), we have to lose reference to the temporary montage]
	if (SlotNodeName != NAME_None)
	{
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			// check if this is playing
			// [翻译失败: check if this is playing]
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			// make sure what is active right now is transient that we created by request
			// [翻译失败: make sure what is active right now is transient that we created by request]
			if (MontageInstance && MontageInstance->IsActive() && MontageInstance->IsPlaying())
			{
				UAnimMontage* CurMontage = MontageInstance->Montage;
				if (CurMontage && CurMontage->GetOuter() == GetTransientPackage())
				{
					// Check each track, in practice there should only be one on these
					// [翻译失败: Check each track, in practice there should only be one on these]
					for (int32 SlotTrackIndex = 0; SlotTrackIndex < CurMontage->SlotAnimTracks.Num(); SlotTrackIndex++)
					{
						const FSlotAnimationTrack* AnimTrack = &CurMontage->SlotAnimTracks[SlotTrackIndex];
						if (AnimTrack && AnimTrack->SlotName == SlotNodeName)
						{
							// Found it
							// 找到了
							MontageInstance->Stop(FAlphaBlend(InBlendOutTime));
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		// Stop all
		// 全部停止
		Montage_Stop(InBlendOutTime);
	}
}

bool UAnimInstance::IsPlayingSlotAnimation(const UAnimSequenceBase* Asset, FName SlotNodeName) const
{
	UAnimMontage* Montage = nullptr;
	return IsPlayingSlotAnimation(Asset, SlotNodeName, Montage);
}

bool UAnimInstance::IsPlayingSlotAnimation(const UAnimSequenceBase* Asset, FName SlotNodeName, UAnimMontage*& OutMontage) const
{
	for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
	{
		// check if this is playing
		// 检查这是否正在播放
		FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
		// make sure what is active right now is transient that we created by request
		// 确保现在活动的内容是我们根据请求创建的瞬态内容
		if (MontageInstance && MontageInstance->IsActive() && MontageInstance->IsPlaying())
		{
			UAnimMontage* CurMontage = MontageInstance->Montage;
			if (CurMontage && CurMontage->GetOuter() == GetTransientPackage())
			{
				const FAnimTrack* AnimTrack = CurMontage->GetAnimationData(SlotNodeName);
				if (AnimTrack && AnimTrack->AnimSegments.Num() == 1)
				{
					OutMontage = CurMontage;
					return (AnimTrack->AnimSegments[0].GetAnimReference() == Asset);
				}
			}
		}
	}

	return false;
}

bool UAnimInstance::IsSlotActive(FName SlotNodeName) const
{
   if (SlotNodeName != NAME_None)
   {
      const FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();

      for (const FMontageEvaluationState & EvaluationState : Proxy.GetMontageEvaluationData())
      {
         if (EvaluationState.Montage.IsValid() && EvaluationState.Montage->IsValidSlot(SlotNodeName) && EvaluationState.bIsActive)
         {
            return true;
         }
      }
   }

   return false;
}

float UAnimInstance::Blueprint_GetSlotMontageLocalWeight(FName SlotNodeName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetSlotMontageLocalWeight(SlotNodeName);
}

float UAnimInstance::Montage_PlayInternal(UAnimMontage* MontageToPlay, const FMontageBlendSettings& BlendInSettings, float InPlayRate /*= 1.f*/, EMontagePlayReturnType ReturnValueType /*= EMontagePlayReturnType::MontageLength*/, float InTimeToStartMontageAt /*= 0.f*/, bool bStopAllMontages /*= true*/)
{
	LLM_SCOPE(ELLMTag::Animation);

	if (MontageToPlay && (MontageToPlay->GetPlayLength() > 0.f) && MontageToPlay->HasValidSlotSetup())
	{
		if (CurrentSkeleton && MontageToPlay->GetSkeleton())
		{
			const FName NewMontageGroupName = MontageToPlay->GetGroupName();
			if (bStopAllMontages)
			{
				// Enforce 'a single montage at once per group' rule
				// 强制执行“每个组一次使用一个剪辑”规则
				StopAllMontagesByGroupName(NewMontageGroupName, BlendInSettings);

				// Early out since we might have called UninitializeAnimation() when trigger montage blending out events above.
				// 早期，因为我们可能在触发蒙太奇混合上面的事件时调用了 UninitializeAnimation()。
				if (bUninitialized && MontageCVars::bEarlyOutMontageWhenUninitialized)
				{
					return 0.0f;
				}

				if (MontageCVars::bFlushCompletedMontagesOnPlay)
				{
					ConditionalFlushCompletedMontages();
				}
			}

			// Enforce 'a single root motion montage at once' rule.
			// [翻译失败: Enforce 'a single root motion montage at once' rule.]
			if (MontageToPlay->bEnableRootMotionTranslation || MontageToPlay->bEnableRootMotionRotation)
			{
				FAnimMontageInstance* ActiveRootMotionMontageInstance = GetRootMotionMontageInstance();
				if (ActiveRootMotionMontageInstance)
				{
					ActiveRootMotionMontageInstance->Stop(BlendInSettings);

					// Early out since we might have called UninitializeAnimation() when trigger montage blending out events above.
					// [翻译失败: Early out since we might have called UninitializeAnimation() when trigger montage blending out events above.]
					if (bUninitialized && MontageCVars::bEarlyOutMontageWhenUninitialized)
					{
						return 0.0f;
					}
				}
			}

			FAnimMontageInstance* NewInstance = new FAnimMontageInstance(this);
			check(NewInstance);

			const float MontageLength = MontageToPlay->GetPlayLength();

			NewInstance->Initialize(MontageToPlay);
			NewInstance->Play(InPlayRate, BlendInSettings);
			NewInstance->SetPosition(FMath::Clamp(InTimeToStartMontageAt, 0.f, MontageLength));
			MontageInstances.Add(NewInstance);
			ActiveMontagesMap.Add(MontageToPlay, NewInstance);

			// If we are playing root motion, set this instance as the one providing root motion.
			// 如果我们正在播放根运动，请将此实例设置为提供根运动的实例。
			if (MontageToPlay->HasRootMotion())
			{
				RootMotionMontageInstance = NewInstance;
			}

			OnMontageStarted.Broadcast(MontageToPlay);

			UE_LOG(LogAnimMontage, Verbose, TEXT("Montage_Play: AnimMontage: %s,  (DesiredWeight:%0.2f, Weight:%0.2f)"),
				*NewInstance->Montage->GetName(), NewInstance->GetDesiredWeight(), NewInstance->GetWeight());

			return (ReturnValueType == EMontagePlayReturnType::MontageLength) ? MontageLength : (MontageLength / (InPlayRate*MontageToPlay->RateScale));
		}
		else
		{
			UE_LOG(LogAnimMontage, Warning, TEXT("Playing a Montage (%s) for the wrong Skeleton (%s) instead of (%s)."),
				*GetNameSafe(MontageToPlay), *GetNameSafe(CurrentSkeleton), *GetNameSafe(MontageToPlay->GetSkeleton()));
		}
	}

	return 0.f;
}

/** Play a Montage. Returns Length of Montage in seconds. Returns 0.f if failed to play. */
/** 播放蒙太奇。返回蒙太奇的长度（以秒为单位）。如果播放失败则返回0.f。 */
float UAnimInstance::Montage_Play(UAnimMontage* MontageToPlay, float InPlayRate/*= 1.f*/, EMontagePlayReturnType ReturnValueType, float InTimeToStartMontageAt, bool bStopAllMontages /*= true*/)
{
	FMontageBlendSettings BlendSettings;
	if (MontageToPlay)
	{
		BlendSettings.Blend = MontageToPlay->BlendIn;
		BlendSettings.BlendMode = MontageToPlay->BlendModeIn;
		BlendSettings.BlendProfile = MontageToPlay->BlendProfileIn;
	}

	return Montage_PlayInternal(MontageToPlay, BlendSettings, InPlayRate, ReturnValueType, InTimeToStartMontageAt, bStopAllMontages);
}

float UAnimInstance::Montage_PlayWithBlendIn(UAnimMontage* MontageToPlay, const FAlphaBlendArgs& BlendIn, float InPlayRate /*= 1.f*/, EMontagePlayReturnType ReturnValueType /*= EMontagePlayReturnType::MontageLength*/, float InTimeToStartMontageAt/*=0.f*/, bool bStopAllMontages /*= true*/)
{
	FMontageBlendSettings BlendSettings;
	BlendSettings.Blend = BlendIn;

	if (MontageToPlay)
	{
		BlendSettings.BlendMode = MontageToPlay->BlendModeIn;
		BlendSettings.BlendProfile = MontageToPlay->BlendProfileIn;
	}

	return Montage_PlayInternal(MontageToPlay, BlendSettings, InPlayRate, ReturnValueType, InTimeToStartMontageAt, bStopAllMontages);
}

float UAnimInstance::Montage_PlayWithBlendSettings(UAnimMontage* MontageToPlay, const FMontageBlendSettings& BlendInSettings, float InPlayRate /*= 1.f*/, EMontagePlayReturnType ReturnValueType /*= EMontagePlayReturnType::MontageLength*/, float InTimeToStartMontageAt/*=0.f*/, bool bStopAllMontages /*= true*/)
{
	return Montage_PlayInternal(MontageToPlay, BlendInSettings, InPlayRate, ReturnValueType, InTimeToStartMontageAt, bStopAllMontages);
}

void UAnimInstance::Montage_StopInternal(TFunctionRef<FMontageBlendSettings(const FAnimMontageInstance*)> AlphaBlendSelectorFunction, const UAnimMontage* Montage /*= nullptr*/)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->Stop(AlphaBlendSelectorFunction(MontageInstance));
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// 如果没有蒙太奇参考，请在所有活动的参考上执行此操作。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->Stop(AlphaBlendSelectorFunction(MontageInstance));

				// UninitializeAnimation() empties the MontagesInstances array thus we need to exit early.
				// [翻译失败: UninitializeAnimation() empties the MontagesInstances array thus we need to exit early.]
				if (bUninitialized && MontageCVars::bEarlyOutMontageWhenUninitialized)
				{
					return;
				}
			}
		}
	}
}

void UAnimInstance::Montage_Stop(float InBlendOutTime, const UAnimMontage* Montage)
{
	auto AlphaBlendFromInstanceAndInBlendOutTime = [InBlendOutTime](const FAnimMontageInstance* InMontageInstance)
	{
		FMontageBlendSettings BlendOutSettings;
		if (const UAnimMontage* InstanceMontage = InMontageInstance->Montage)
		{
			//Grab all settings from the montage, except BlendTime
			//[翻译失败: Grab all settings from the montage, except BlendTime]
			BlendOutSettings.Blend = InstanceMontage->BlendOut;
			BlendOutSettings.Blend.BlendTime = InBlendOutTime;
			BlendOutSettings.BlendMode = InstanceMontage->BlendModeOut;
			BlendOutSettings.BlendProfile = InstanceMontage->BlendProfileOut;
		}

		return BlendOutSettings;
	};

	Montage_StopInternal(AlphaBlendFromInstanceAndInBlendOutTime, Montage);
}

void UAnimInstance::Montage_StopWithBlendOut(const FAlphaBlendArgs& BlendOutArgs, const UAnimMontage* Montage /*= NULL*/)
{
	auto AlphaBlendPassthrough = [BlendOutArgs](const FAnimMontageInstance* InMontageInstance)
	{
		FMontageBlendSettings BlendOutSettings;
		if (const UAnimMontage* InstanceMontage = InMontageInstance->Montage)
		{
			//Grab all settings from the montage, except the FAlphaBlend
			//[翻译失败: Grab all settings from the montage, except the FAlphaBlend]
			BlendOutSettings.Blend = BlendOutArgs;
			BlendOutSettings.BlendMode = InstanceMontage->BlendModeOut;
			BlendOutSettings.BlendProfile = InstanceMontage->BlendProfileOut;
		}

		return BlendOutSettings;
	};

	Montage_StopInternal(AlphaBlendPassthrough, Montage);
}

void UAnimInstance::Montage_StopWithBlendSettings(const FMontageBlendSettings& BlendOutSettings, const UAnimMontage* Montage /*= nullptr*/)
{
	auto AlphaBlendPassthrough = [BlendOutSettings](const FAnimMontageInstance* InMontageInstance)
	{
		return BlendOutSettings;
	};

	Montage_StopInternal(AlphaBlendPassthrough, Montage);
}

void UAnimInstance::Montage_StopGroupByName(float InBlendOutTime, FName GroupName)
{
	for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
	{
		FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
		if (MontageInstance && MontageInstance->Montage && MontageInstance->IsActive() && (MontageInstance->Montage->GetGroupName() == GroupName))
		{
			MontageInstances[InstanceIndex]->Stop(FAlphaBlend(MontageInstance->Montage->BlendOut, InBlendOutTime));
		}
	}
}

void UAnimInstance::Montage_Pause(const UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->Pause();
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// [翻译失败: If no Montage reference, do it on all active ones.]
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->Pause();
			}
		}
	}
}

void UAnimInstance::Montage_Resume(const UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance && !MontageInstance->IsPlaying())
		{
			MontageInstance->SetPlaying(true);
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// [翻译失败: If no Montage reference, do it on all active ones.]
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive() && !MontageInstance->IsPlaying())
			{
				MontageInstance->SetPlaying(true);
			}
		}
	}
}

void UAnimInstance::Montage_JumpToSection(FName SectionName, const UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			bool const bEndOfSection = (MontageInstance->GetPlayRate() < 0.f);
			MontageInstance->JumpToSectionName(SectionName, bEndOfSection);
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// [翻译失败: If no Montage reference, do it on all active ones.]
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				bool const bEndOfSection = (MontageInstance->GetPlayRate() < 0.f);
				MontageInstance->JumpToSectionName(SectionName, bEndOfSection);
			}
		}
	}
}

void UAnimInstance::Montage_JumpToSectionsEnd(FName SectionName, const UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			bool const bEndOfSection = (MontageInstance->GetPlayRate() >= 0.f);
			MontageInstance->JumpToSectionName(SectionName, bEndOfSection);
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// [翻译失败: If no Montage reference, do it on all active ones.]
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				bool const bEndOfSection = (MontageInstance->GetPlayRate() >= 0.f);
				MontageInstance->JumpToSectionName(SectionName, bEndOfSection);
			}
		}
	}
}

void UAnimInstance::Montage_SetNextSection(FName SectionNameToChange, FName NextSection, const UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->SetNextSectionName(SectionNameToChange, NextSection);
		}
	}
	else
	{
		bool bFoundOne = false;

		// If no Montage reference, do it on all active ones.
		// 如果没有蒙太奇参考，请在所有活动的参考上执行此操作。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->SetNextSectionName(SectionNameToChange, NextSection);
				bFoundOne = true;
			}
		}

		if (!bFoundOne)
		{
			bFoundOne = true;
		}
	}
}

void UAnimInstance::Montage_SetPlayRate(const UAnimMontage* Montage, float NewPlayRate)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->SetPlayRate(NewPlayRate);
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// 如果没有蒙太奇参考，请在所有活动的参考上执行此操作。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->SetPlayRate(NewPlayRate);
			}
		}
	}
}

bool UAnimInstance::Montage_IsActive(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return true;
		}
	}
	else
	{
		// If no Montage reference, return true if there is any active montage.
		// 如果没有 Montage 引用，则如果有任何活动的 montage，则返回 true。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return true;
			}
		}
	}

	return false;
}

bool UAnimInstance::Montage_IsPlaying(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return MontageInstance->IsPlaying();
		}
	}
	else
	{
		// If no Montage reference, return true if there is any active playing montage.
		// 如果没有蒙太奇引用，则如果有任何活动的播放蒙太奇，则返回 true。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive() && MontageInstance->IsPlaying())
			{
				return true;
			}
		}
	}

	return false;
}

FName UAnimInstance::Montage_GetCurrentSection(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return MontageInstance->GetCurrentSection();
		}
	}
	else
	{
		// If no Montage reference, get first active one.
		// 如果没有蒙太奇参考，则获取第一个活动的参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return MontageInstance->GetCurrentSection();
			}
		}
	}

	return NAME_None;
}

void UAnimInstance::Montage_SetEndDelegate(FOnMontageEnded& InOnMontageEnded, UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->OnMontageEnded = InOnMontageEnded;
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// 如果没有蒙太奇参考，请在所有活动的参考上执行此操作。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->OnMontageEnded = InOnMontageEnded;
			}
		}
	}
}

void UAnimInstance::Montage_SetBlendingOutDelegate(FOnMontageBlendingOutStarted& InOnMontageBlendingOut, UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->OnMontageBlendingOutStarted = InOnMontageBlendingOut;
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// 如果没有蒙太奇参考，请在所有活动的参考上执行此操作。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->OnMontageBlendingOutStarted = InOnMontageBlendingOut;
			}
		}
	}
}


void UAnimInstance::Montage_SetBlendedInDelegate(FOnMontageBlendedInEnded& InOnMontageBlendedIn, UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->OnMontageBlendedInEnded = InOnMontageBlendedIn;
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// 如果没有蒙太奇参考，请在所有活动的参考上执行此操作。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->OnMontageBlendedInEnded = InOnMontageBlendedIn;
			}
		}
	}
}

FOnMontageEnded* UAnimInstance::Montage_GetEndedDelegate(UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return &MontageInstance->OnMontageEnded;
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		// 如果没有蒙太奇参考，请使用找到的第一个活动参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return &MontageInstance->OnMontageEnded;
			}
		}
	}

	return nullptr;
}

FOnMontageBlendingOutStarted* UAnimInstance::Montage_GetBlendingOutDelegate(UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return &MontageInstance->OnMontageBlendingOutStarted;
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		// 如果没有蒙太奇参考，请使用找到的第一个活动参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return &MontageInstance->OnMontageBlendingOutStarted;
			}
		}
	}

	return NULL;
}

void UAnimInstance::Montage_SetSectionChangedDelegate(FOnMontageSectionChanged& InOnMontageSectionChanged, UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->OnMontageSectionChanged = InOnMontageSectionChanged;
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// 如果没有蒙太奇参考，请在所有活动的参考上执行此操作。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->OnMontageSectionChanged = InOnMontageSectionChanged;
			}
		}
	}
}

FOnMontageSectionChanged* UAnimInstance::Montage_GetSectionChangedDelegate(UAnimMontage* Montage)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return &MontageInstance->OnMontageSectionChanged;
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		// 如果没有蒙太奇参考，请使用找到的第一个活动参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return &MontageInstance->OnMontageSectionChanged;
			}
		}
	}

	return NULL;
}

void UAnimInstance::Montage_SetPosition(const UAnimMontage* Montage, float NewPosition)
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			MontageInstance->SetPosition(NewPosition);
		}
	}
	else
	{
		// If no Montage reference, do it on all active ones.
		// 如果没有蒙太奇参考，请在所有活动的参考上执行此操作。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				MontageInstance->SetPosition(NewPosition);
			}
		}
	}
}

float UAnimInstance::Montage_GetPosition(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		const FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return MontageInstance->GetPosition();
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		// 如果没有蒙太奇参考，请使用找到的第一个活动参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			const FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return MontageInstance->GetPosition();
			}
		}
	}

	return 0.f;
}

bool UAnimInstance::Montage_GetIsStopped(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		const FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		return (!MontageInstance); // Not active == Stopped.
	}
	return true;
}

float UAnimInstance::Montage_GetBlendTime(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		const FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return MontageInstance->GetBlendTime();
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		// 如果没有蒙太奇参考，请使用找到的第一个活动参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			const FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return MontageInstance->GetBlendTime();
			}
		}
	}

	return 0.f;
}

float UAnimInstance::Montage_GetPlayRate(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		const FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return MontageInstance->GetPlayRate();
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		// 如果没有蒙太奇参考，请使用找到的第一个活动参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			const FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return MontageInstance->GetPlayRate();
			}
		}
	}

	return 0.f;
}

float UAnimInstance::Montage_GetEffectivePlayRate(const UAnimMontage* Montage) const
{
	if (Montage)
	{
		const FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return MontageInstance->GetPlayRate() * Montage->RateScale;
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		// 如果没有蒙太奇参考，请使用找到的第一个活动参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			const FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive() && MontageInstance->Montage)
			{
				return MontageInstance->GetPlayRate() * MontageInstance->Montage->RateScale;
			}
		}
	}

	return 0.f;
}

bool UAnimInstance::DynamicMontage_IsPlayingFrom(const UAnimSequenceBase* Animation) const
{
	if (!Animation)
	{
		return false;
	}

	if (const UAnimMontage* AnimMontage = Cast<UAnimMontage>(Animation))
	{
		return Montage_IsPlaying(AnimMontage);
	}

	for (const TPair<UAnimMontage*, FAnimMontageInstance*>& ActiveMontage : ActiveMontagesMap)
	{
		if (ActiveMontage.Key->IsDynamicMontage() && ActiveMontage.Key->GetFirstAnimReference() == Animation)
		{
			return ActiveMontage.Value->IsPlaying();
		}
	}

	return false;
}

void UAnimInstance::MontageSync_Follow(const UAnimMontage* MontageFollower, const UAnimInstance* OtherAnimInstance, const UAnimMontage* MontageLeader)
{
	if (!MontageFollower || !OtherAnimInstance || !MontageLeader)
	{
		return;
	}

	FAnimMontageInstance* FollowerMontageInstance = GetActiveInstanceForMontage(MontageFollower);
	FAnimMontageInstance* LeaderMontageInstance = OtherAnimInstance->GetActiveInstanceForMontage(MontageLeader);
	if (!FollowerMontageInstance || !LeaderMontageInstance)
	{
		return;
	}

	FollowerMontageInstance->MontageSync_Follow(LeaderMontageInstance);
}

void UAnimInstance::MontageSync_StopFollowing(const UAnimMontage* MontageFollower)
{
	if (!MontageFollower)
	{
		return;
	}

	FAnimMontageInstance* FollowerMontageInstance = GetActiveInstanceForMontage(MontageFollower);
	if (!FollowerMontageInstance)
	{
		return;
	}

	FollowerMontageInstance->MontageSync_StopFollowing();
}

int32 UAnimInstance::Montage_GetNextSectionID(UAnimMontage const* const Montage, int32 const& CurrentSectionID) const
{
	if (Montage)
	{
		FAnimMontageInstance* MontageInstance = GetActiveInstanceForMontage(Montage);
		if (MontageInstance)
		{
			return MontageInstance->GetNextSectionID(CurrentSectionID);
		}
	}
	else
	{
		// If no Montage reference, use first active one found.
		// 如果没有蒙太奇参考，请使用找到的第一个活动参考。
		for (int32 InstanceIndex = 0; InstanceIndex < MontageInstances.Num(); InstanceIndex++)
		{
			FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
			if (MontageInstance && MontageInstance->IsActive())
			{
				return MontageInstance->GetNextSectionID(CurrentSectionID);
			}
		}
	}

	return INDEX_NONE;
}

bool UAnimInstance::IsAnyMontagePlaying() const
{
	return (MontageInstances.Num() > 0);
}

UAnimMontage* UAnimInstance::GetCurrentActiveMontage() const
{
	// Start from end, as most recent instances are added at the end of the queue.
	// 从末尾开始，因为最近的实例被添加到队列的末尾。
	int32 const NumInstances = MontageInstances.Num();
	for (int32 InstanceIndex = NumInstances - 1; InstanceIndex >= 0; InstanceIndex--)
	{
		const FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
		if (MontageInstance && MontageInstance->IsActive())
		{
			return MontageInstance->Montage;
		}
	}

	return NULL;
}

FAnimMontageInstance* UAnimInstance::GetActiveMontageInstance() const
{
	// Start from end, as most recent instances are added at the end of the queue.
	// 从末尾开始，因为最近的实例被添加到队列的末尾。
	int32 const NumInstances = MontageInstances.Num();
	for (int32 InstanceIndex = NumInstances - 1; InstanceIndex >= 0; InstanceIndex--)
	{
		FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
		if (MontageInstance && MontageInstance->IsActive())
		{
			return MontageInstance;
		}
	}

	return NULL;
}

void UAnimInstance::StopAllMontages(float BlendOut)
{
	for (int32 Index = MontageInstances.Num() - 1; Index >= 0; Index--)
	{
		MontageInstances[Index]->Stop(FAlphaBlend(BlendOut), true);

		// UninitializeAnimation() empties the MontagesInstances array thus we need to exit early.
		// UninitializeAnimation() 清空 MontagesInstances 数组，因此我们需要提前退出。
		if (bUninitialized && MontageCVars::bEarlyOutMontageWhenUninitialized)
		{
			return;
		}
	}
}

void UAnimInstance::StopAllMontagesByGroupName(FName InGroupName, const FAlphaBlend& BlendOut)
{
	FMontageBlendSettings BlendOutSettings;
	BlendOutSettings.Blend = BlendOut;
	StopAllMontagesByGroupName(InGroupName, BlendOutSettings);
}

void UAnimInstance::StopAllMontagesByGroupName(FName InGroupName, const FMontageBlendSettings& BlendOutSettings)
{
	for (int32 InstanceIndex = MontageInstances.Num() - 1; InstanceIndex >= 0; InstanceIndex--)
	{
		FAnimMontageInstance* MontageInstance = MontageInstances[InstanceIndex];
		if (MontageInstance && MontageInstance->Montage && (MontageInstance->Montage->GetGroupName() == InGroupName))
		{
			MontageInstance->Stop(BlendOutSettings, true);

			// UninitializeAnimation() empties the MontagesInstances array thus we need to exit early.
			// UninitializeAnimation() 清空 MontagesInstances 数组，因此我们需要提前退出。
			if (bUninitialized && MontageCVars::bEarlyOutMontageWhenUninitialized)
			{
				return;
			}
		}
	}
}

void UAnimInstance::OnMontageInstanceStopped(FAnimMontageInstance& StoppedMontageInstance)
{
	ClearMontageInstanceReferences(StoppedMontageInstance);
}

void UAnimInstance::ClearMontageInstanceReferences(FAnimMontageInstance& InMontageInstance)
{
	if (UAnimMontage* MontageStopped = InMontageInstance.Montage)
	{
		// Remove instance for Active List.
		// 删除活动列表的实例。
		FAnimMontageInstance** AnimInstancePtr = ActiveMontagesMap.Find(MontageStopped);
		if (AnimInstancePtr && (*AnimInstancePtr == &InMontageInstance))
		{
			ActiveMontagesMap.Remove(MontageStopped);
		}
	}
	else
	{
		// If Montage ref is nullptr, it's possible the instance got terminated already and that is fine.
		// 如果 Montage ref 为 nullptr，则实例可能已终止，这很好。
		// Make sure it's been removed from our ActiveMap though
		// 确保它已从我们的 ActiveMap 中删除
		if (ActiveMontagesMap.FindKey(&InMontageInstance) != nullptr)
		{
			UE_LOG(LogAnimation, Warning, TEXT("%s: null montage found in the montage instance array!!"), *GetName());
		}
	}

	// Clear RootMotionMontageInstance
	// 清除 RootMotionMontageInstance
	if (RootMotionMontageInstance == &InMontageInstance)
	{
		RootMotionMontageInstance = nullptr;
	}

	// Clear any active synchronization
	// 清除所有活动同步
	InMontageInstance.MontageSync_StopFollowing();
	InMontageInstance.MontageSync_StopLeading();
}

FAnimNode_LinkedInputPose* UAnimInstance::GetLinkedInputPoseNode(FName InLinkedInputName, FName InGraph)
{
	const FAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
	if(InLinkedInputName == NAME_None && InGraph == NAME_None)
	{
		return Proxy.DefaultLinkedInstanceInputNode;
	}
	else if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		if(InLinkedInputName == NAME_None)
		{
			InLinkedInputName = FAnimNode_LinkedInputPose::DefaultInputPoseName;
		}
		if(InGraph == NAME_None)
		{
			InGraph = NAME_AnimGraph;
		}
		for(const FAnimBlueprintFunction& AnimBlueprintFunction : AnimBlueprintClass->GetAnimBlueprintFunctions())
		{
			if(AnimBlueprintFunction.Name == InGraph)
			{
				check(AnimBlueprintFunction.InputPoseNames.Num() == AnimBlueprintFunction.InputPoseNodeProperties.Num());
				for(int32 InputIndex = 0; InputIndex < AnimBlueprintFunction.InputPoseNames.Num(); ++InputIndex)
				{
					if(AnimBlueprintFunction.InputPoseNames[InputIndex] == InLinkedInputName && AnimBlueprintFunction.InputPoseNodeProperties[InputIndex] != nullptr)
					{
						FAnimNode_LinkedInputPose* LinkedInput = AnimBlueprintFunction.InputPoseNodeProperties[InputIndex]->ContainerPtrToValuePtr<FAnimNode_LinkedInputPose>(this);
						check(LinkedInput->Name == InLinkedInputName);
						return LinkedInput;
					}
				}
			}
		}
	}

	return nullptr;
}

UAnimInstance* UAnimInstance::GetLinkedAnimGraphInstanceByTag(FName InTag) const
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		if (const FAnimSubsystem_Tag* TagSubsystem = AnimBlueprintClass->FindSubsystem<FAnimSubsystem_Tag>())
		{
			const FAnimNode_LinkedAnimGraph* LinkedAnimGraph = TagSubsystem->FindNodeByTag<FAnimNode_LinkedAnimGraph>(InTag, this);
			if(LinkedAnimGraph)
			{
				return LinkedAnimGraph->GetTargetInstance<UAnimInstance>();
			}
		}
	}

	return nullptr;
}

void UAnimInstance::GetLinkedAnimGraphInstancesByTag(FName InTag, TArray<UAnimInstance*>& OutLinkedInstances) const
{
	if(UAnimInstance* Instance = GetLinkedAnimGraphInstanceByTag(InTag))
	{
		OutLinkedInstances.Add(Instance);
	}
}

void UAnimInstance::LinkAnimGraphByTag(FName InTag, TSubclassOf<UAnimInstance> InClass)
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		if (const FAnimSubsystem_Tag* TagSubsystem = AnimBlueprintClass->FindSubsystem<FAnimSubsystem_Tag>())
		{
			FAnimNode_LinkedAnimGraph* LinkedAnimGraph = TagSubsystem->FindNodeByTag<FAnimNode_LinkedAnimGraph>(InTag, this);
			if (LinkedAnimGraph)
			{
				LinkedAnimGraph->SetAnimClass(InClass, this);
			}
		}
	}
}

void UAnimInstance::PerformLinkedLayerOverlayOperation(TSubclassOf<UAnimInstance> InClass, TFunctionRef<UClass*(UClass* InClass, FAnimNode_LinkedAnimLayer*)> InClassSelectorFunction, bool bInDeferSubGraphInitialization)
{
#if DO_CHECK
	if(bInitializing)
	{
		UE_LOG(LogAnimation, Warning, TEXT("Performing linked layer operations in initialization may produce unexpected results, please use the BlueprintLinkedAnimationLayersInitialized event to perform linked layer operations."));
	}
#endif

	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		UClass* NewClass = InClass.Get();
		if (NewClass && IAnimClassInterface::GetFromClass(NewClass) == nullptr)
		{
			// Invalid layer class (i.e passing in a native C++ class instead of an animation blueprint)
			// 图层类无效（即传入本机 C++ 类而不是动画蓝图）
			UE_LOG(LogAnimation, Warning, TEXT("Performing linked layer operations with an invalid layer class: %s"), *NewClass->GetName());
			return;
		}

		USkeletalMeshComponent* MeshComp = GetSkelMeshComponent();

		MeshComp->ForEachAnimInstance([](UAnimInstance* InInstance)
		{
			// Make sure we have valid objects on all instances as initialization can route back
			// 确保我们在所有实例上都有有效的对象，因为初始化可以路由回来
			// out of linked instances into other graphs, including 'this'
			// 从链接实例到其他图表，包括“this”
			InInstance->GetProxyOnAnyThread<FAnimInstanceProxy>().InitializeObjects(InInstance);
		});

		// Map of group name->nodes, per class, to run under that group instance
		// 每个类的组名称->节点的映射，在该组实例下运行
		TMap<UClass*, TMap<FName, TArray<FAnimNode_LinkedAnimLayer*, TInlineAllocator<4>>, TInlineSetAllocator<4>>, TInlineSetAllocator<4>> LayerNodesToSet;

		for(const FStructProperty* LayerNodeProperty : AnimBlueprintClass->GetLinkedAnimLayerNodeProperties())
		{
			FAnimNode_LinkedAnimLayer* Layer = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(this);

			// If the class is null, then reset to default (which can be null)
			// [翻译失败: If the class is null, then reset to default (which can be null)]
			UClass* ClassToEvaluate = NewClass != nullptr ? NewClass : Layer->InstanceClass.Get();

			if(ClassToEvaluate != nullptr)
			{
				// Now check whether the layer is implemented by the class
				// [翻译失败: Now check whether the layer is implemented by the class]
				IAnimClassInterface* NewAnimClassInterface = IAnimClassInterface::GetFromClass(ClassToEvaluate);
				if(const FAnimBlueprintFunction* FoundFunction = IAnimClassInterface::FindAnimBlueprintFunction(NewAnimClassInterface, Layer->Layer))
				{
					if(FoundFunction->bImplemented)
					{
						UClass* ClassToSet = InClassSelectorFunction(NewClass, Layer);
						TMap<FName, TArray<FAnimNode_LinkedAnimLayer*, TInlineAllocator<4>>, TInlineSetAllocator<4>>& ClassLayerNodesToSet = LayerNodesToSet.FindOrAdd(ClassToSet);
						TArray<FAnimNode_LinkedAnimLayer*, TInlineAllocator<4>>& LayerNodes = ClassLayerNodesToSet.FindOrAdd(FoundFunction->Group);
						LayerNodes.Add(Layer);
					}
				}
			}
			else
			{
				// Add null classes so we clear the node's instance below
				// [翻译失败: Add null classes so we clear the node's instance below]
				UClass* ClassToSet = InClassSelectorFunction(NewClass, Layer);
				TMap<FName, TArray<FAnimNode_LinkedAnimLayer*, TInlineAllocator<4>>, TInlineSetAllocator<4>>& ClassLayerNodesToSet = LayerNodesToSet.FindOrAdd(ClassToSet);
				TArray<FAnimNode_LinkedAnimLayer*, TInlineAllocator<4>>& LayerNodes = ClassLayerNodesToSet.FindOrAdd(NAME_None);
				LayerNodes.Add(Layer);
			}
		}

		auto UnlinkLayerNodesInInstance = [](UAnimInstance* InAnimInstance, TArrayView<FAnimNode_LinkedAnimLayer*> InLayerNodes)
		{
			const IAnimClassInterface* const NewLinkedInstanceClass = IAnimClassInterface::GetFromClass(InAnimInstance->GetClass());
			for (const FStructProperty* const LayerNodeProperty : NewLinkedInstanceClass->GetLinkedAnimLayerNodeProperties())
			{
				FAnimNode_LinkedAnimLayer* const LinkedAnimLayerNode = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(InAnimInstance);
				const bool bExternalLink = InLayerNodes.ContainsByPredicate([LinkedAnimLayerNode](const FAnimNode_LinkedAnimLayer* Layer) { return Layer->Layer == LinkedAnimLayerNode->Layer; });
				if (bExternalLink)
				{
					LinkedAnimLayerNode->DynamicUnlink(InAnimInstance);
				}
			}
		};

		auto InitializeAndCacheBonesForLinkedRoot = [](FAnimNode_LinkedAnimLayer* InLayerNode, FAnimInstanceProxy& InThisProxy, UAnimInstance* InLinkedInstance, FAnimInstanceProxy& InLinkedProxy)
		{
			InLinkedProxy.InitializeObjects(InLinkedInstance);

			if (InLinkedInstance->GetSkelMeshComponent()->GetSkeletalMeshAsset() != nullptr)
			{
				FAnimationInitializeContext InitContext(&InThisProxy);
				InLayerNode->InitializeSubGraph_AnyThread(InitContext);
				FAnimationCacheBonesContext CacheBonesContext(&InThisProxy);
				InLayerNode->CacheBonesSubGraph_AnyThread(CacheBonesContext);
			}
		};

		for (TPair<UClass*, TMap<FName, TArray<FAnimNode_LinkedAnimLayer*, TInlineAllocator<4>>, TInlineSetAllocator<4>>> ClassLayerNodesToSet : LayerNodesToSet)
		{
			for (TPair<FName, TArray<FAnimNode_LinkedAnimLayer*, TInlineAllocator<4>>> LayerPair : ClassLayerNodesToSet.Value)
			{
				if (FAnimSubsystem_SharedLinkedAnimLayers* SharedLinkedAnimLayers = FAnimSubsystem_SharedLinkedAnimLayers::GetFromMesh(MeshComp))
				{
					// Shared instances path
					// 共享实例路径
					UClass* ClassToSet = ClassLayerNodesToSet.Key;
					for (FAnimNode_LinkedAnimLayer* LayerNode : LayerPair.Value)
					{
						// Disallow setting the same class as this instance, which would create infinite recursion
						// 禁止设置与此实例相同的类，这将创建无限递归
						if (ClassToSet != nullptr && ClassToSet != GetClass())
						{
							UAnimInstance* TargetInstance = LayerNode->GetTargetInstance<UAnimInstance>();

							// Skip setting if the class is the same
							// 如果班级相同则跳过设置
							if (TargetInstance == nullptr || ClassToSet != TargetInstance->GetClass())
							{
								bool bIsNewInstance;
 								const FName FunctionToLink = LayerNode->GetDynamicLinkFunctionName();
								UAnimInstance* LinkedInstance = SharedLinkedAnimLayers->AddLinkedFunction(this, ClassToSet, FunctionToLink, bIsNewInstance);

								if (bIsNewInstance)
								{
									// Unlink any layer nodes in the new linked instance, as they may have been hooked up to self in InitializeAnimation above.
									// 取消链接新链接实例中的任何图层节点，因为它们可能已在上面的 InitializeAnimation 中连接到 self。
									UnlinkLayerNodesInInstance(LinkedInstance, LayerPair.Value);
								}

								// Mark function as linked
								// 将函数标记为已链接
								LayerNode->SetLinkedLayerInstance(this, LinkedInstance);

								// Propagate notify flags. If any nodes have this set then we need to propagate to the group.
								// 传播通知标志。如果任何节点有这个集合，那么我们需要传播到该组。
								LinkedInstance->bPropagateNotifiesToLinkedInstances |= LayerNode->bPropagateNotifiesToLinkedInstances;
								LinkedInstance->bReceiveNotifiesFromLinkedInstances |= LayerNode->bReceiveNotifiesFromLinkedInstances;

								if (!bInDeferSubGraphInitialization)
								{
									FAnimInstanceProxy& ThisProxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
									FAnimInstanceProxy& LinkedProxy = LinkedInstance->GetProxyOnAnyThread<FAnimInstanceProxy>();

									// Initialize the correct parts of the linked instance
									// 初始化链接实例的正确部分
									InitializeAndCacheBonesForLinkedRoot(LayerNode, ThisProxy, LinkedInstance, LinkedProxy);
								}
							}
						}
						else
						{
							LayerNode->SetLinkedLayerInstance(this, nullptr);

							if (!bInDeferSubGraphInitialization)
							{
								UAnimInstance* LinkedInstance = LayerNode->GetTargetInstance<UAnimInstance>();
								if (LayerNode->LinkedRoot && LinkedInstance)
								{
									FAnimInstanceProxy& ThisProxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
									FAnimInstanceProxy& LinkedProxy = LinkedInstance->GetProxyOnAnyThread<FAnimInstanceProxy>();
									InitializeAndCacheBonesForLinkedRoot(LayerNode, ThisProxy, LinkedInstance, LinkedProxy);
								}
							}
						}
					}
				}
				else if (LayerPair.Key == NAME_None)
				{
					// Ungrouped path - each layer gets a separate instance
					// 未分组的路径 - 每层都有一个单独的实例
					for (FAnimNode_LinkedAnimLayer* LayerNode : LayerPair.Value)
					{
						UClass* ClassToSet = ClassLayerNodesToSet.Key;

						// Disallow setting the same class as this instance, which would create infinite recursion
						// 禁止设置与此实例相同的类，这将创建无限递归
						if (ClassToSet != nullptr && ClassToSet != GetClass())
						{
							UAnimInstance* TargetInstance = LayerNode->GetTargetInstance<UAnimInstance>();

							// Skip setting if the class is the same
							// 如果班级相同则跳过设置
							if (TargetInstance == nullptr || ClassToSet != TargetInstance->GetClass())
							{
								UAnimInstance* NewLinkedInstance = NewObject<UAnimInstance>(MeshComp, ClassToSet);
								NewLinkedInstance->bCreatedByLinkedAnimGraph = true;
								NewLinkedInstance->bPropagateNotifiesToLinkedInstances = LayerNode->bPropagateNotifiesToLinkedInstances;
								NewLinkedInstance->bReceiveNotifiesFromLinkedInstances = LayerNode->bReceiveNotifiesFromLinkedInstances;
								NewLinkedInstance->InitializeAnimation();

								if(MeshComp->HasBegunPlay())
								{
									NewLinkedInstance->NativeBeginPlay();
									NewLinkedInstance->BlueprintBeginPlay();
								}

								// Unlink any layer nodes in the new linked instance, as they may have been hooked up to self in InitializeAnimation above.
								// 取消链接新链接实例中的任何图层节点，因为它们可能已在上面的 InitializeAnimation 中连接到 self。
								UnlinkLayerNodesInInstance(NewLinkedInstance, LayerPair.Value);

								LayerNode->SetLinkedLayerInstance(this, NewLinkedInstance);

								if (!bInDeferSubGraphInitialization)
								{
									// Initialize the correct parts of the linked instance
									// 初始化链接实例的正确部分
									if (LayerNode->LinkedRoot)
									{
										FAnimInstanceProxy& ThisProxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
										FAnimInstanceProxy& LinkedProxy = NewLinkedInstance->GetProxyOnAnyThread<FAnimInstanceProxy>();
										InitializeAndCacheBonesForLinkedRoot(LayerNode, ThisProxy, NewLinkedInstance, LinkedProxy);
									}
								}

								MeshComp->GetLinkedAnimInstances().Add(NewLinkedInstance);
							}
						}
						else
						{
							LayerNode->SetLinkedLayerInstance(this, nullptr);

							if (!bInDeferSubGraphInitialization)
							{
								UAnimInstance* LinkedInstance = LayerNode->GetTargetInstance<UAnimInstance>();
								if (LayerNode->LinkedRoot && LinkedInstance)
								{
									FAnimInstanceProxy& ThisProxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
									FAnimInstanceProxy& LinkedProxy = LinkedInstance->GetProxyOnAnyThread<FAnimInstanceProxy>();
									InitializeAndCacheBonesForLinkedRoot(LayerNode, ThisProxy, LinkedInstance, LinkedProxy);
								}
							}
						}
					}
				}
				else
				{
					// Grouped path - each group gets an instance
					// 分组路径 - 每个组都有一个实例
					// If the class is null, then reset to default (which can be null)
					// 如果类为空，则重置为默认值（可以为空）
					FAnimNode_LinkedAnimLayer* FirstLayerNode = LayerPair.Value[0];
					UClass* ClassToSet = ClassLayerNodesToSet.Key;

					// Disallow setting the same class as this instance, which would create infinite recursion
					// 禁止设置与此实例相同的类，这将创建无限递归
					if (ClassToSet != nullptr && ClassToSet != GetClass())
					{
						UAnimInstance* TargetInstance = FirstLayerNode->GetTargetInstance<UAnimInstance>();

						// Skip setting if the class is the same
						// 如果班级相同则跳过设置
						if (TargetInstance == nullptr || ClassToSet != TargetInstance->GetClass())
						{
							// Create and add one linked instance for this group
							// 为此组创建并添加一个链接实例
							UAnimInstance* NewLinkedInstance = NewObject<UAnimInstance>(MeshComp, ClassToSet);
							NewLinkedInstance->bCreatedByLinkedAnimGraph = true;
							NewLinkedInstance->InitializeAnimation();

							if(MeshComp->HasBegunPlay())
							{
								NewLinkedInstance->NativeBeginPlay();
								NewLinkedInstance->BlueprintBeginPlay();
							}

							// Unlink any layer nodes in the new linked instance, as they may have been hooked up to self in InitializeAnimation above.
							// 取消链接新链接实例中的任何图层节点，因为它们可能已在上面的 InitializeAnimation 中连接到 self。
							UnlinkLayerNodesInInstance(NewLinkedInstance, LayerPair.Value);

							for(FAnimNode_LinkedAnimLayer* LayerNode : LayerPair.Value)
							{
								LayerNode->SetLinkedLayerInstance(this, NewLinkedInstance);

								// Propagate notify flags. If any nodes have this set then we need to propagate to the group.
								// 传播通知标志。如果任何节点有这个集合，那么我们需要传播到该组。
								NewLinkedInstance->bPropagateNotifiesToLinkedInstances |= LayerNode->bPropagateNotifiesToLinkedInstances;
								NewLinkedInstance->bReceiveNotifiesFromLinkedInstances |= LayerNode->bReceiveNotifiesFromLinkedInstances;
							}

							if (!bInDeferSubGraphInitialization)
							{
								FAnimInstanceProxy& ThisProxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
								FAnimInstanceProxy& LinkedProxy = NewLinkedInstance->GetProxyOnAnyThread<FAnimInstanceProxy>();

								// Initialize the correct parts of the linked instance
								// 初始化链接实例的正确部分
								for (FAnimNode_LinkedAnimLayer* LayerNode : LayerPair.Value)
								{
									if (LayerNode->LinkedRoot)
									{
										InitializeAndCacheBonesForLinkedRoot(LayerNode, ThisProxy, NewLinkedInstance, LinkedProxy);
									}
								}
							}

							MeshComp->GetLinkedAnimInstances().Add(NewLinkedInstance);
						}
					}
					else
					{
						// Clear the node's instance - we didnt find a class to use
						// 清除节点的实例 - 我们没有找到要使用的类
						for (FAnimNode_LinkedAnimLayer* LayerNode : LayerPair.Value)
						{
							LayerNode->SetLinkedLayerInstance(this, nullptr);

							if (!bInDeferSubGraphInitialization)
							{
								UAnimInstance* LinkedInstance = LayerNode->GetTargetInstance<UAnimInstance>();
								if (LayerNode->LinkedRoot && LinkedInstance)
								{
									FAnimInstanceProxy& ThisProxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
									FAnimInstanceProxy& LinkedProxy = LinkedInstance->GetProxyOnAnyThread<FAnimInstanceProxy>();
									InitializeAndCacheBonesForLinkedRoot(LayerNode, ThisProxy, LinkedInstance, LinkedProxy);
								}
							}
						}
					}
				}
			}
		}

#if DO_CHECK
		// Verify required bones are consistent now we may have spawned a new instance.
		// 验证所需的骨骼是否一致，现在我们可能已经生成了一个新实例。
		// If required bones arrays for linked instances are not built to the same LOD, then when running the anim graph
		// 如果链接实例所需的骨骼数组未构建到相同的 LOD，则在运行动画图时
		// we can get problems/asserts trying to blend curves/poses of differing sizes (see FORT-354970, for example).
		// [翻译失败: we can get problems/asserts trying to blend curves/poses of differing sizes (see FORT-354970, for example).]
		// If required bones are flagged for update we are assuming that RefreshBoneTransforms will end up rectifying
		// 如果所需的骨骼被标记为更新，我们假设 RefreshBoneTransforms 最终将进行纠正
		// any inconsistencies.
		// 任何不一致之处。
		// We also skip this check during re-instancing as main & linked instances may get their re-initialization in
		// 我们还在重新实例期间跳过此检查，因为主实例和链接实例可能会在以下位置重新初始化：
		// a random order depending on what is being compiled.
		// 随机顺序取决于正在编译的内容。
		if(!GIsReinstancing && MeshComp->GetAnimInstance() && MeshComp->bRequiredBonesUpToDate)
		{
			const int32 RootLOD = MeshComp->GetAnimInstance()->GetRequiredBones().GetCalculatedForLOD();
			for(UAnimInstance* LinkedInstance : MeshComp->GetLinkedAnimInstances())
			{
				check(RootLOD == LinkedInstance->GetRequiredBones().GetCalculatedForLOD());
			}

			if(MeshComp->GetPostProcessInstance())
			{
				check(RootLOD == MeshComp->GetPostProcessInstance()->GetRequiredBones().GetCalculatedForLOD());
			}
		}
#endif
	}
}

void UAnimInstance::LinkAnimClassLayers(TSubclassOf<UAnimInstance> InClass)
{
	auto SelectResolvedClassIfValid = [](UClass* InResolvedClass, FAnimNode_LinkedAnimLayer* InLayerNode)
	{
		if(InResolvedClass != nullptr)
		{
			// If we have a valid resolved class, use that as an overlay
			// [翻译失败: If we have a valid resolved class, use that as an overlay]
			return InResolvedClass;
		}
		else
		{
			// Otherwise use the default (which can be null)
			// [翻译失败: Otherwise use the default (which can be null)]
			return InLayerNode->InstanceClass.Get();
		}
	};

	if (GetSkelMeshComponent()->IsRegistered())
	{
		PerformLinkedLayerOverlayOperation(InClass, SelectResolvedClassIfValid);
	}
}

void UAnimInstance::UnlinkAnimClassLayers(TSubclassOf<UAnimInstance> InClass)
{
	auto ConditionallySelectDefaultClass = [](UClass* InResolvedClass, FAnimNode_LinkedAnimLayer* InLayerNode) -> UClass*
	{
		if (InLayerNode->GetTargetInstance<UAnimInstance>() == nullptr)
		{
			return nullptr;
		}

		if (InResolvedClass != nullptr && InLayerNode->GetTargetInstance<UAnimInstance>()->GetClass() == InResolvedClass)
		{
			// Reset to default if the classes match
			// 如果类别匹配，则重置为默认值
			return InLayerNode->InstanceClass.Get();
		}
		else
		{
			// No change
			// 没有变化
			return InLayerNode->GetTargetInstance<UAnimInstance>()->GetClass();
		}
	};

	if (GetSkelMeshComponent()->IsRegistered())
	{
		PerformLinkedLayerOverlayOperation(InClass, ConditionallySelectDefaultClass);
	}
}


void UAnimInstance::InitializeGroupedLayers(bool bInDeferSubGraphInitialization)
{
	auto SelectResolvedClassIfValid = [](UClass* InResolvedClass, FAnimNode_LinkedAnimLayer* InLayerNode)
	{
		if(InResolvedClass != nullptr)
		{
			// If we have a valid resolved class, use that as an overlay
			// 如果我们有一个有效的解析类，请将其用作覆盖层
			return InResolvedClass;
		}
		else
		{
			// Otherwise use the default (which can be null)
			// 否则使用默认值（可以为空）
			return InLayerNode->InstanceClass.Get();
		}
	};

	if (GetSkelMeshComponent()->IsRegistered())
	{
		PerformLinkedLayerOverlayOperation(nullptr, SelectResolvedClassIfValid, bInDeferSubGraphInitialization);
	}
}

void UAnimInstance::AddExternalNotifyHandler(UObject* ExternalHandlerObject, FName NotifyEventName)
{
	if (ExternalHandlerObject)
	{
		check (ExternalHandlerObject->FindFunction(NotifyEventName));
		ExternalNotifyHandlers.FindOrAdd(NotifyEventName).AddUFunction(ExternalHandlerObject, NotifyEventName);
	}
}

void UAnimInstance::RemoveExternalNotifyHandler(UObject* ExternalHandlerObject, FName NotifyEventName)
{
	if (FSimpleMulticastDelegate* ExistingDelegate = ExternalNotifyHandlers.Find(NotifyEventName))
	{
		ExistingDelegate->RemoveAll(ExternalHandlerObject);
	}
}

FAnimSubsystemInstance* UAnimInstance::FindSubsystem(UScriptStruct* InSubsystemType)
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		FAnimSubsystemInstance* Subsystem = nullptr;

		AnimBlueprintClass->ForEachSubsystem(this, [&Subsystem, InSubsystemType](const FAnimSubsystemInstanceContext& InContext)
		{
			if(InContext.SubsystemInstanceStruct == InSubsystemType)
			{
				Subsystem = &InContext.SubsystemInstance;
				return EAnimSubsystemEnumeration::Stop;
			}

			return EAnimSubsystemEnumeration::Continue;
		});

		return Subsystem;
	}

	return nullptr;
}

UAnimInstance* UAnimInstance::GetLinkedAnimLayerInstanceByGroup(FName InGroup) const
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		for(const FStructProperty* LayerNodeProperty : AnimBlueprintClass->GetLinkedAnimLayerNodeProperties())
		{
			const FAnimNode_LinkedAnimLayer* Layer = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(this);

			TArray<UClass*, TInlineAllocator<4>> ClassesForGroups;
			if (UAnimInstance* LayerInstance = Layer->GetTargetInstance<UAnimInstance>())
			{
				ClassesForGroups.Add(LayerInstance->GetClass());
			}
			if (UClass* InstanceClass = Layer->InstanceClass.Get())
			{
				ClassesForGroups.Add(InstanceClass);
			}
			if(UClass* InterfaceClass = Layer->Interface.Get())
			{
				ClassesForGroups.Add(InterfaceClass);
			}

			ClassesForGroups.Add(GetClass());

			for(UClass* ClassForGroups : ClassesForGroups)
			{
				IAnimClassInterface* AnimClassInterfaceForGroups = IAnimClassInterface::GetFromClass(ClassForGroups);
				if(const FAnimBlueprintFunction* FoundFunction = IAnimClassInterface::FindAnimBlueprintFunction(AnimClassInterfaceForGroups, Layer->Layer))
				{
					if(InGroup == FoundFunction->Group)
					{
						return Layer->GetTargetInstance<UAnimInstance>();
					}
				}
			}
		}
	}

	return nullptr;
}

void UAnimInstance::GetLinkedAnimLayerInstancesByGroup(FName InGroup, TArray<UAnimInstance*>& OutLinkedInstances) const
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		for (const FStructProperty* LayerNodeProperty : AnimBlueprintClass->GetLinkedAnimLayerNodeProperties())
		{
			const FAnimNode_LinkedAnimLayer* Layer = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(this);

			TArray<UClass*, TInlineAllocator<4>> ClassesForGroups;
			if (UAnimInstance* LayerInstance = Layer->GetTargetInstance<UAnimInstance>())
			{
				ClassesForGroups.Add(LayerInstance->GetClass());
			}
			if (UClass* InstanceClass = Layer->InstanceClass.Get())
			{
				ClassesForGroups.Add(InstanceClass);
			}
			if (UClass* InterfaceClass = Layer->Interface.Get())
			{
				ClassesForGroups.Add(InterfaceClass);
			}

			ClassesForGroups.Add(GetClass());

			for (UClass* ClassForGroups : ClassesForGroups)
			{
				IAnimClassInterface* AnimClassInterfaceForGroups = IAnimClassInterface::GetFromClass(ClassForGroups);
				if (const FAnimBlueprintFunction* FoundFunction = IAnimClassInterface::FindAnimBlueprintFunction(AnimClassInterfaceForGroups, Layer->Layer))
				{
					UAnimInstance* TargetInstance = Layer->GetTargetInstance<UAnimInstance>();
					if (InGroup == FoundFunction->Group && !OutLinkedInstances.Contains(TargetInstance))
					{
						OutLinkedInstances.Add(TargetInstance);
					}
				}
			}
		}
	}
}

UAnimInstance* UAnimInstance::GetLinkedAnimLayerInstanceByGroupAndClass(FName InGroup, TSubclassOf<UAnimInstance> InClass) const
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		for (const FStructProperty* LayerNodeProperty : AnimBlueprintClass->GetLinkedAnimLayerNodeProperties())
		{
			const FAnimNode_LinkedAnimLayer* Layer = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(this);
			UAnimInstance* TargetInstance = Layer->GetTargetInstance<UAnimInstance>();
			if (TargetInstance && TargetInstance->GetClass() == InClass.Get())
			{
				UClass* ClassForGroups;
				if (UClass* InterfaceClass = Layer->Interface.Get())
				{
					ClassForGroups = InterfaceClass;
				}
				else
				{
					ClassForGroups = GetClass();
				}

				IAnimClassInterface* AnimClassInterfaceForGroups = IAnimClassInterface::GetFromClass(ClassForGroups);
				if (const FAnimBlueprintFunction* FoundFunction = IAnimClassInterface::FindAnimBlueprintFunction(AnimClassInterfaceForGroups, Layer->Layer))
				{
					if (InGroup == FoundFunction->Group)
					{
						return TargetInstance;
					}
				}
			}
		}
	}

	return nullptr;
}

UAnimInstance* UAnimInstance::GetLinkedAnimLayerInstanceByClass(TSubclassOf<UAnimInstance> InClass, bool bCheckForChildClass) const
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		for(const FStructProperty* LayerNodeProperty : AnimBlueprintClass->GetLinkedAnimLayerNodeProperties())
		{
			const FAnimNode_LinkedAnimLayer* Layer = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(this);
			UAnimInstance* TargetInstance = Layer->GetTargetInstance<UAnimInstance>();

			if (TargetInstance && TargetInstance->GetClass())
			{
				if ((bCheckForChildClass && TargetInstance->GetClass()->IsChildOf(InClass)) || (!bCheckForChildClass && TargetInstance->GetClass() == InClass.Get()))
				{
					return TargetInstance;
				}
			}
		}
	}

	return nullptr;
}

FAnimMontageInstance* UAnimInstance::GetActiveInstanceForMontage(const UAnimMontage* Montage) const
{
	FAnimMontageInstance* const* FoundInstancePtr = ActiveMontagesMap.Find(Montage);
	return FoundInstancePtr ? *FoundInstancePtr : nullptr;
}

FAnimMontageInstance* UAnimInstance::GetInstanceForMontage(const UAnimMontage* Montage) const
{
	for (FAnimMontageInstance* MontageInstance : MontageInstances)
	{
		if (MontageInstance && MontageInstance->Montage == Montage)
		{
			return MontageInstance;
		}
	}

	return nullptr;
}

FAnimMontageInstance* UAnimInstance::GetMontageInstanceForID(int32 MontageInstanceID)
{
	for (FAnimMontageInstance* MontageInstance : MontageInstances)
	{
		if (MontageInstance && MontageInstance->GetInstanceID() == MontageInstanceID)
		{
			return MontageInstance;
		}
	}

	return nullptr;
}

FAnimMontageInstance* UAnimInstance::GetRootMotionMontageInstance() const
{
	return RootMotionMontageInstance;
}

FRootMotionMovementParams UAnimInstance::ConsumeExtractedRootMotion(float Alpha)
{
	if (Alpha < ZERO_ANIMWEIGHT_THRESH)
	{
		return FRootMotionMovementParams();
	}
	else if (Alpha > (1.f - ZERO_ANIMWEIGHT_THRESH))
	{
		FRootMotionMovementParams RootMotion = ExtractedRootMotion;
		ExtractedRootMotion.Clear();
		return RootMotion;
	}
	else
	{
		return ExtractedRootMotion.ConsumeRootMotion(Alpha);
	}
}

void UAnimInstance::SetMorphTarget(FName MorphTargetName, float Value)
{
	USkeletalMeshComponent* Component = GetOwningComponent();
	if (Component)
	{
		Component->SetMorphTarget(MorphTargetName, Value);
	}
}

void UAnimInstance::ClearMorphTargets()
{
	USkeletalMeshComponent* Component = GetOwningComponent();
	if (Component)
	{
		Component->ClearMorphTargets();
	}
}

float UAnimInstance::CalculateDirection(const FVector& Velocity, const FRotator& BaseRotation) const
{
	if (!Velocity.IsNearlyZero())
	{
		FMatrix RotMatrix = FRotationMatrix(BaseRotation);
		FVector ForwardVector = RotMatrix.GetScaledAxis(EAxis::X);
		FVector RightVector = RotMatrix.GetScaledAxis(EAxis::Y);
		FVector NormalizedVel = Velocity.GetSafeNormal2D();

		// get a cos(alpha) of forward vector vs velocity
		// 获取前向矢量与速度的 cos(alpha)
		float ForwardCosAngle = FVector::DotProduct(ForwardVector, NormalizedVel);
		// now get the alpha and convert to degree
		// 现在获取 alpha 并转换为度数
		float ForwardDeltaDegree = FMath::RadiansToDegrees(FMath::Acos(ForwardCosAngle));

		// depending on where right vector is, flip it
		// 根据正确向量的位置，翻转它
		float RightCosAngle = FVector::DotProduct(RightVector, NormalizedVel);
		if (RightCosAngle < 0)
		{
			ForwardDeltaDegree *= -1;
		}

		return ForwardDeltaDegree;
	}

	return 0.f;
}

void UAnimInstance::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UAnimInstance* This = CastChecked<UAnimInstance>(InThis);

	// go through all montage instances, and update them
	// 遍历所有蒙太奇实例并更新它们
	// and make sure their weight is updated properly
	// 并确保他们的体重正确更新
	for (int32 I=0; I<This->MontageInstances.Num(); ++I)
	{
		if( This->MontageInstances[I] )
		{
			This->MontageInstances[I]->AddReferencedObjects(Collector);
		}
	}

	// the queued montage events also reference montage, and we want to keep those montages around if they are queued to trigger
	// 排队的蒙太奇事件也引用蒙太奇，如果它们排队等待触发，我们希望保留这些蒙太奇
	for (int32 I = 0; I < This->QueuedMontageBlendingOutEvents.Num(); ++I)
	{
		Collector.AddReferencedObject(This->QueuedMontageBlendingOutEvents[I].Montage);
	}

	for (int32 I = 0; I < This->QueuedMontageBlendedInEvents.Num(); ++I)
	{
		Collector.AddReferencedObject(This->QueuedMontageBlendedInEvents[I].Montage);
	}

	for (int32 I = 0; I < This->QueuedMontageEndedEvents.Num(); ++I)
	{
		Collector.AddReferencedObject(This->QueuedMontageEndedEvents[I].Montage);
	}

	for (int32 I = 0; I < This->QueuedMontageSectionChangedEvents.Num(); ++I)
	{
		Collector.AddReferencedObject(This->QueuedMontageSectionChangedEvents[I].Montage);
	}

	if (This->AnimInstanceProxy)
	{
		This->AnimInstanceProxy->AddReferencedObjects(This, Collector);
	}

	Super::AddReferencedObjects(This, Collector);
}
//
void UAnimInstance::LockAIResources(bool bLockMovement, bool LockAILogic)
{
	UE_LOG(LogAnimation, Error, TEXT("%s: LockAIResources is no longer supported. Please use LockAIResourcesWithAnimation instead."), *GetName());
}

void UAnimInstance::UnlockAIResources(bool bUnlockMovement, bool UnlockAILogic)
{
	UE_LOG(LogAnimation, Error, TEXT("%s: UnlockAIResources is no longer supported. Please use UnlockAIResourcesWithAnimation instead."), *GetName());
}

bool UAnimInstance::GetTimeToClosestMarker(FName SyncGroup, FName MarkerName, float& OutMarkerTime) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetTimeToClosestMarker(SyncGroup, MarkerName, OutMarkerTime);
}

bool UAnimInstance::HasMarkerBeenHitThisFrame(FName SyncGroup, FName MarkerName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().HasMarkerBeenHitThisFrame(SyncGroup, MarkerName);
}

bool UAnimInstance::IsSyncGroupBetweenMarkers(FName InSyncGroupName, FName PreviousMarker, FName NextMarker, bool bRespectMarkerOrder) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().IsSyncGroupBetweenMarkers(InSyncGroupName, PreviousMarker, NextMarker, bRespectMarkerOrder);
}

FMarkerSyncAnimPosition UAnimInstance::GetSyncGroupPosition(FName InSyncGroupName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetSyncGroupPosition(InSyncGroupName);
}

void UAnimInstance::UpdateMontageEvaluationData()
{
	if (IsUsingMainInstanceMontageEvaluationData())
	{
		const USkeletalMeshComponent* Comp = GetOwningComponent();
		if (Comp && Comp->GetAnimInstance() != this)
		{
			// If we're using the main instance's montage eval data
			// 如果我们使用主实例的蒙太奇评估数据
			// and we're not the main instance, then skip updating this instance's montage eval data
			// 并且我们不是主实例，然后跳过更新该实例的蒙太奇评估数据
			return;
		}
	}

	FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();

	Proxy.GetMontageEvaluationData().Reset(MontageInstances.Num());
	UE_LOG(LogAnimMontage, Verbose, TEXT("UpdateMontageEvaluationData Starting: Owner: %s"),	*GetNameSafe(GetOwningActor()));

	for (FAnimMontageInstance* MontageInstance : MontageInstances)
	{
		// although montage can advance with 0.f weight, it is fine to filter by weight here
		// 虽然蒙太奇可以以 0.f 权重前进，但这里按权重过滤就可以了
		// because we don't want to evaluate them if 0 weight
		// 因为如果权重为 0，我们不想评估它们
		if (MontageInstance->Montage && MontageInstance->GetWeight() > ZERO_ANIMWEIGHT_THRESH)
		{
			UE_LOG(LogAnimMontage, Verbose, TEXT("UpdateMontageEvaluationData : AnimMontage: %s,  (DesiredWeight:%0.2f, Weight:%0.2f)"),
						*MontageInstance->Montage->GetName(), MontageInstance->GetDesiredWeight(), MontageInstance->GetWeight());

			Proxy.GetMontageEvaluationData().Add(
				FMontageEvaluationState
				(
					MontageInstance->Montage,
					MontageInstance->GetPosition(),
					MontageInstance->DeltaTimeRecord,
					MontageInstance->bPlaying,
					MontageInstance->IsActive(),
					MontageInstance->GetBlend(),
					MontageInstance->GetActiveBlendProfile(),
					MontageInstance->GetBlendStartAlpha()
				));
		}
	}

	Proxy.GetSlotGroupInertializationRequestDataMap() = SlotGroupInertializationRequestDataMap;

	// Reset inertialization requests every frame.
	// 每帧重置惯性化请求。
	// If the request is missed by the graph (i.e. the slot node is not relevant), we assume what brought it back to relevancy will handle the blend instead.
	// 如果图表错过了请求（即槽节点不相关），我们假设使其恢复相关性的将处理混合。
	SlotGroupInertializationRequestDataMap.Reset();
}

float UAnimInstance::GetInstanceAssetPlayerLength(int32 AssetPlayerIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceAssetPlayerLength(AssetPlayerIndex);
}

float UAnimInstance::GetInstanceAssetPlayerTime(int32 AssetPlayerIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceAssetPlayerTime(AssetPlayerIndex);
}

float UAnimInstance::GetInstanceAssetPlayerTimeFraction(int32 AssetPlayerIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceAssetPlayerTimeFraction(AssetPlayerIndex);
}

float UAnimInstance::GetInstanceAssetPlayerTimeFromEndFraction(int32 AssetPlayerIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceAssetPlayerTimeFromEndFraction(AssetPlayerIndex);
}

float UAnimInstance::GetInstanceAssetPlayerTimeFromEnd(int32 AssetPlayerIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceAssetPlayerTimeFromEnd(AssetPlayerIndex);
}

float UAnimInstance::GetInstanceMachineWeight(int32 MachineIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceMachineWeight(MachineIndex);
}

float UAnimInstance::GetInstanceStateWeight(int32 MachineIndex, int32 StateIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceStateWeight(MachineIndex, StateIndex);
}

float UAnimInstance::GetInstanceCurrentStateElapsedTime(int32 MachineIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceCurrentStateElapsedTime(MachineIndex);
}

float UAnimInstance::GetInstanceTransitionCrossfadeDuration(int32 MachineIndex, int32 TransitionIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceTransitionCrossfadeDuration(MachineIndex, TransitionIndex);
}

float UAnimInstance::GetInstanceTransitionTimeElapsed(int32 MachineIndex, int32 TransitionIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceTransitionTimeElapsed(MachineIndex, TransitionIndex);
}

float UAnimInstance::GetInstanceTransitionTimeElapsedFraction(int32 MachineIndex, int32 TransitionIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceTransitionTimeElapsedFraction(MachineIndex, TransitionIndex);
}

float UAnimInstance::GetRelevantAnimTimeRemaining(int32 MachineIndex, int32 StateIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetRelevantAnimTimeRemaining(MachineIndex, StateIndex);
}

float UAnimInstance::GetRelevantAnimTimeRemainingFraction(int32 MachineIndex, int32 StateIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetRelevantAnimTimeRemainingFraction(MachineIndex, StateIndex);
}

float UAnimInstance::GetRelevantAnimLength(int32 MachineIndex, int32 StateIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetRelevantAnimLength(MachineIndex, StateIndex);
}

float UAnimInstance::GetRelevantAnimTime(int32 MachineIndex, int32 StateIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetRelevantAnimTime(MachineIndex, StateIndex);
}

float UAnimInstance::GetRelevantAnimTimeFraction(int32 MachineIndex, int32 StateIndex)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetRelevantAnimTimeFraction(MachineIndex, StateIndex);
}

bool UAnimInstance::CheckOnInstanceAndMainInstance(TFunctionRef<bool (FAnimInstanceProxy* )> ProxyLambdaFunc)
{
	FAnimInstanceProxy& InstanceProxy = GetProxyOnAnyThread<FAnimInstanceProxy>();
	if (ProxyLambdaFunc(&InstanceProxy))
	{
		return true;
	}

	FAnimInstanceProxy* MainInstanceProxy = InstanceProxy.GetMainInstanceProxy();
	if (MainInstanceProxy && MainInstanceProxy != &InstanceProxy)
	{
		return ProxyLambdaFunc(MainInstanceProxy);
	}
	return false;
}

uint32 UAnimInstance::GetLastMontageFlushFrame() const
{
	return LastMontageFlushFrame;
}

bool UAnimInstance::WasAnimNotifyStateActiveInAnyState(TSubclassOf<UAnimNotifyState> AnimNotifyStateType)
{
	return CheckOnInstanceAndMainInstance([&AnimNotifyStateType](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyStateActiveInAnyState(AnimNotifyStateType);
		}
	);
}

bool UAnimInstance::WasAnimNotifyStateActiveInStateMachine(int32 MachineIndex, TSubclassOf<UAnimNotifyState> AnimNotifyStateType)
{
	return CheckOnInstanceAndMainInstance([&MachineIndex, &AnimNotifyStateType](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyStateActiveInStateMachine(MachineIndex, AnimNotifyStateType);
		}
	);
}

bool UAnimInstance::WasAnimNotifyStateActiveInSourceState(int32 MachineIndex, int32 StateIndex, TSubclassOf<UAnimNotifyState> AnimNotifyStateType)
{
	return CheckOnInstanceAndMainInstance([&MachineIndex, &StateIndex, &AnimNotifyStateType](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyStateActiveInSourceState(MachineIndex, StateIndex, AnimNotifyStateType);
		}
	);
}

bool UAnimInstance::WasAnimNotifyTriggeredInSourceState(int32 MachineIndex, int32 StateIndex,  TSubclassOf<UAnimNotify> AnimNotifyType)
{
	return CheckOnInstanceAndMainInstance([&MachineIndex, &StateIndex, &AnimNotifyType](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyTriggeredInSourceState(MachineIndex, StateIndex, AnimNotifyType);
		}
	);
}

bool UAnimInstance::WasAnimNotifyNameTriggeredInSourceState(int32 MachineIndex, int32 StateIndex, FName NotifyName)
{
	return CheckOnInstanceAndMainInstance([&MachineIndex, &StateIndex, &NotifyName](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyNameTriggeredInSourceState(MachineIndex, StateIndex, NotifyName);
		}
	);
}

bool UAnimInstance::WasAnimNotifyTriggeredInStateMachine(int32 MachineIndex, TSubclassOf<UAnimNotify> AnimNotifyType)
{
	return CheckOnInstanceAndMainInstance([&MachineIndex, &AnimNotifyType](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyTriggeredInStateMachine(MachineIndex, AnimNotifyType);
		}
	);
}

bool UAnimInstance::WasAnimNotifyNameTriggeredInStateMachine(int32 MachineIndex, FName NotifyName)
{
	return CheckOnInstanceAndMainInstance([&MachineIndex, &NotifyName](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyNameTriggeredInStateMachine(MachineIndex, NotifyName);
		}
	);
}

bool UAnimInstance::WasAnimNotifyTriggeredInAnyState(TSubclassOf<UAnimNotify> AnimNotifyType)
{
	return CheckOnInstanceAndMainInstance([&AnimNotifyType](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyTriggeredInAnyState(AnimNotifyType);
		}
	);
}

bool UAnimInstance::WasAnimNotifyNameTriggeredInAnyState(FName NotifyName)
{
	return CheckOnInstanceAndMainInstance([&NotifyName](FAnimInstanceProxy* Proxy)
		{
			return Proxy->WasAnimNotifyNameTriggeredInAnyState(NotifyName);
		}
	);
}

const FAnimNode_StateMachine* UAnimInstance::GetStateMachineInstance(int32 MachineIndex) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetStateMachineInstance(MachineIndex);
}

const FAnimNode_StateMachine* UAnimInstance::GetStateMachineInstanceFromName(FName MachineName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetStateMachineInstanceFromName(MachineName);
}

const FBakedAnimationStateMachine* UAnimInstance::GetStateMachineInstanceDesc(FName MachineName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetStateMachineInstanceDesc(MachineName);
}

const FAnimNode_AssetPlayerRelevancyBase* UAnimInstance::GetRelevantAssetPlayerInterfaceFromState(int32 MachineIndex, int32 StateIndex) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetRelevantAssetPlayerInterfaceFromState(MachineIndex, StateIndex);
}

const UBlendProfile* UAnimInstance::GetBlendProfileByName(FName InBlendProfileName) const
{
	if (CurrentSkeleton)
	{
		return CurrentSkeleton->GetBlendProfile(InBlendProfileName);
	}

	return nullptr;
}

int32 UAnimInstance::GetStateMachineIndex(FName MachineName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetStateMachineIndex(MachineName);
}

void UAnimInstance::GetStateMachineIndexAndDescription(FName InMachineName, int32& OutMachineIndex, const FBakedAnimationStateMachine** OutMachineDescription)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetStateMachineIndexAndDescription(InMachineName, OutMachineIndex, OutMachineDescription);
}

const FBakedAnimationStateMachine* UAnimInstance::GetMachineDescription(IAnimClassInterface* AnimBlueprintClass, FAnimNode_StateMachine* MachineInstance)
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetMachineDescription(AnimBlueprintClass, MachineInstance);
}

int32 UAnimInstance::GetInstanceAssetPlayerIndex(FName MachineName, FName StateName, FName AssetName) const
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetInstanceAssetPlayerIndex(MachineName, StateName, AssetName);
}

TArray<const FAnimNode_AssetPlayerBase*> UAnimInstance::GetInstanceAssetPlayers(const FName& GraphName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceAssetPlayers(GraphName);
}

TArray<FAnimNode_AssetPlayerBase*> UAnimInstance::GetMutableInstanceAssetPlayers(const FName& GraphName)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetMutableInstanceAssetPlayers(GraphName);
}

TArray<const FAnimNode_AssetPlayerRelevancyBase*> UAnimInstance::GetInstanceRelevantAssetPlayers(const FName& GraphName) const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetInstanceRelevantAssetPlayers(GraphName);
}

TArray<FAnimNode_AssetPlayerRelevancyBase*> UAnimInstance::GetMutableInstanceRelevantAssetPlayers(const FName& GraphName)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetMutableInstanceRelevantAssetPlayers(GraphName);
}

int32 UAnimInstance::GetSyncGroupIndexFromName(FName SyncGroupName) const
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetSyncGroupIndexFromName(SyncGroupName);
}

bool UAnimInstance::HandleNotify(const FAnimNotifyEvent& AnimNotifyEvent)
{
	return false;
}

bool UAnimInstance::IsRunningParallelEvaluation() const
{
	USkeletalMeshComponent* Comp = GetOwningComponent();
	if (Comp && Comp->GetAnimInstance() == this)
	{
		return Comp->IsRunningParallelEvaluation();
	}
	return false;
}

FAnimInstanceProxy* UAnimInstance::CreateAnimInstanceProxy()
{
	return new FAnimInstanceProxy(this);
}

void UAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}

bool UAnimInstance::ShouldTriggerAnimNotifyState(const UAnimNotifyState* AnimNotifyState) const
{
	if (ensureMsgf(AnimNotifyState != nullptr, TEXT("UAnimInstance::ShouldTriggerAnimNotifyState: AnimNotifyState is null on AnimInstance %s. ActiveAnimNotifyState array size is: %d"), *GetNameSafe(this), ActiveAnimNotifyState.Num()))
	{
		return true;
	}
	return false;
}

bool UAnimInstance::IsSkeletalMeshComponent(const UObject* Object)
{
	return Object && Object->IsA<USkeletalMeshComponent>();
}

void UAnimInstance::HandleExistingParallelEvaluationTask(USkeletalMeshComponent* Component)
{
	bool bBlockOnTask = true;
	bool bPerformPostAnimEvaluation = true;
	Component->HandleExistingParallelEvaluationTask(bBlockOnTask, bPerformPostAnimEvaluation);
}

void UAnimInstance::RecordMachineWeight(const int32 InMachineClassIndex, const float InMachineWeight)
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().RecordMachineWeight(InMachineClassIndex, InMachineWeight);
}

void UAnimInstance::RecordStateWeight(const int32 InMachineClassIndex, const int32 InStateIndex, const float InStateWeight, const float InElapsedTime)
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().RecordStateWeight(InMachineClassIndex, InStateIndex, InStateWeight, InElapsedTime);
}

const FGraphTraversalCounter& UAnimInstance::GetUpdateCounter() const
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetUpdateCounter();
}

FBoneContainer& UAnimInstance::GetRequiredBones()
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetRequiredBones();
}

const FBoneContainer& UAnimInstance::GetRequiredBones() const
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetRequiredBones();
}

const FBoneContainer& UAnimInstance::GetRequiredBonesOnAnyThread() const
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().GetRequiredBones();
}

void UAnimInstance::QueueRootMotionBlend(const FTransform& RootTransform, const FName& SlotName, float Weight)
{
	RootMotionBlendQueue.Add(FQueuedRootMotionBlend(RootTransform, SlotName, Weight));
}

#if WITH_EDITOR
void UAnimInstance::HandleObjectsReinstanced(const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	static IConsoleVariable* UseLegacyAnimInstanceReinstancingBehavior = IConsoleManager::Get().FindConsoleVariable(TEXT("bp.UseLegacyAnimInstanceReinstancingBehavior"));
	if(UseLegacyAnimInstanceReinstancingBehavior == nullptr || !UseLegacyAnimInstanceReinstancingBehavior->GetBool())
	{
		bool bThisObjectWasReinstanced = false;

		for(const TPair<UObject*, UObject*>& ObjectPair : OldToNewInstanceMap)
		{
			if(ObjectPair.Value == this)
			{
				bThisObjectWasReinstanced = true;
				break;
			}
		}

		if(bThisObjectWasReinstanced)
		{
			USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
			if(MeshComponent && MeshComponent->GetSkeletalMeshAsset())
			{
				TRACE_OBJECT_LIFETIME_BEGIN(this);
				// Minimally reinit proxy (i.e. dont call per-node initialization) unless we are in an editor preview world (i.e. we are in the anim BP editor)
				// 至少重新初始化代理（即不调用每个节点初始化），除非我们处于编辑器预览世界中（即我们处于动画 BP 编辑器中）
				UWorld* World = GetWorld();
				if(World && World->WorldType == EWorldType::EditorPreview)
				{
					InitializeAnimation(false);
				}
				else
				{
					RecalcRequiredBones();

					FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
					Proxy.Initialize(this);
					Proxy.InitializeCachedClassData();
					Proxy.InitializeRootNode_WithRoot(Proxy.RootNode);
				}

				MeshComponent->ClearMotionVector();
			}
		}

		// Forward to custom property-based nodes even if it wasnt this object that was reinstanced, as they may reference different objects that may
		// 即使不是重新实例化的对象，也会转发到基于自定义属性的节点，因为它们可能引用不同的对象
		// also have been reinstanced
		// 也已被重新实例化
		if(IAnimClassInterface* AnimClassInterface = IAnimClassInterface::GetFromClass(GetClass()))
		{
			for(const FStructProperty* NodeProperty : AnimClassInterface->GetAnimNodeProperties())
			{
				if(NodeProperty && NodeProperty->Struct && NodeProperty->Struct->IsChildOf(FAnimNode_CustomProperty::StaticStruct()))
				{
					FAnimNode_CustomProperty* CustomPropertyNode = NodeProperty->ContainerPtrToValuePtr<FAnimNode_CustomProperty>(this);
					CustomPropertyNode->HandleObjectsReinstanced(OldToNewInstanceMap);
				}
			}
		}
	}
}
#endif

bool UAnimInstance::RequestTransitionEvent(const FName EventName, const double RequestTimeout, const ETransitionRequestQueueMode QueueMode, const ETransitionRequestOverwriteMode OverwriteMode)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().RequestTransitionEvent(EventName, RequestTimeout, QueueMode, OverwriteMode);
}

void UAnimInstance::ClearTransitionEvents(const FName EventName)
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().ClearTransitionEvents(EventName);
}

void UAnimInstance::ClearAllTransitionEvents()
{
	GetProxyOnAnyThread<FAnimInstanceProxy>().ClearAllTransitionEvents();
}

bool UAnimInstance::QueryTransitionEvent(int32 MachineIndex, int32 TransitionIndex, FName EventName)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().QueryTransitionEvent(MachineIndex, TransitionIndex, EventName);
}

bool UAnimInstance::QueryAndMarkTransitionEvent(int32 MachineIndex, int32 TransitionIndex, FName EventName)
{
	return GetProxyOnAnyThread<FAnimInstanceProxy>().QueryAndMarkTransitionEvent(MachineIndex, TransitionIndex, EventName);
}

#if WITH_EDITOR
bool UAnimInstance::IsBeingDebugged() const
{
	if (IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		if (UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(Cast<UAnimBlueprintGeneratedClass>(AnimBlueprintClass)->ClassGeneratedBy))
		{
			return AnimBlueprint->IsObjectBeingDebugged(this);
		}
	}

	return false;
}
#endif

#undef LOCTEXT_NAMESPACE
