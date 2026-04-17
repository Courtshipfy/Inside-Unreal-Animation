// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/AnimNode_BlendListBase.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInertializationSyncScope.h"
#include "Animation/BlendProfile.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_Inertialization.h"
#include "Animation/AnimTrace.h"
#include "Animation/SkeletonRemapping.h"
#include "Animation/SkeletonRemappingRegistry.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_BlendListBase)

/////////////////////////////////////////////////////
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase
// FAnimNode_BlendListBase

void FAnimNode_BlendListBase::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_Base::Initialize_AnyThread(Context);
	const int32 NumPoses = BlendPose.Num();
	const TArray<float>& CurrentBlendTimes = GetBlendTimes();
	checkSlow(CurrentBlendTimes.Num() == NumPoses);

	PerBlendData.Reset(NumPoses);
	if (NumPoses > 0)
	{
		// If we have at least 1 pose we initialize to full weight on
  // 如果我们至少有 1 个姿势，我们将初始化为全权重
		// the first pose
  // 第一个姿势
		PerBlendData.AddZeroed(NumPoses);
		PerBlendData[0].Weight = 1.0f;

		for (int32 ChildIndex = 0; ChildIndex < NumPoses; ++ChildIndex)
		{
			BlendPose[ChildIndex].Initialize(Context);
		}
	}

	Initialize();
}

void FAnimNode_BlendListBase::Initialize()
{
	SetCurrentBlendProfile(GetBlendProfile());

	const int32 NumPoses = BlendPose.Num();

	LastActiveChildIndex = INDEX_NONE;

	EAlphaBlendOption CurrentBlendType = GetBlendType();
	UCurveFloat* CurrentCustomBlendCurve = GetCustomBlendCurve();
	for (int32 i = 0; i < PerBlendData.Num(); ++i)
	{
		FAlphaBlend& Blend = PerBlendData[i].Blend;

		Blend.SetBlendTime(0.0f);
		Blend.SetBlendOption(CurrentBlendType);
		Blend.SetCustomCurve(CurrentCustomBlendCurve);

		if (CurrentBlendProfile)
		{
			PerBlendData[i].StartAlpha = 0.0f;
		}
	}

	PerBlendData[0].Blend.SetAlpha(1.0f);
	PerBlendData[0].StartAlpha = 1.0f;
}

void FAnimNode_BlendListBase::SetCurrentBlendProfile(UBlendProfile* NewBlendProfile)
{
	CurrentBlendProfile = NewBlendProfile;
	if (CurrentBlendProfile && PerBoneSampleData.IsEmpty())
	{
		InitializePerBoneData();
	}
}

void FAnimNode_BlendListBase::InitializePerBoneData()
{
	if (CurrentBlendProfile)
	{
		const int32 NumPoses = BlendPose.Num();

		// Initialise per-bone data
  // 初始化每个骨骼的数据
		PerBoneSampleData.Empty(NumPoses);
		PerBoneSampleData.AddZeroed(NumPoses);

		for (int32 Idx = 0; Idx < NumPoses; ++Idx)
		{
			FBlendSampleData& SampleData = PerBoneSampleData[Idx];
			SampleData.SampleDataIndex = Idx;
			SampleData.PerBoneBlendData.AddZeroed(CurrentBlendProfile->GetNumBlendEntries());
		}
	}
}

void FAnimNode_BlendListBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	for(int32 ChildIndex=0; ChildIndex<BlendPose.Num(); ChildIndex++)
	{
		BlendPose[ChildIndex].CacheBones(Context);
	}
}

void FAnimNode_BlendListBase::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	GetEvaluateGraphExposedInputs().Execute(Context);

	const int32 NumPoses = BlendPose.Num();
	const TArray<float>& CurrentBlendTimes = GetBlendTimes();
	checkSlow(PerBlendData.Num() == NumPoses);
	bool bRequestedInertializationOnActiveChildIndexChange = false;
	
	if (NumPoses > 0)
	{
		// Handle a change in the active child index; adjusting the target weights
  // 处理活动子索引的变化；调整目标权重
		const int32 ChildIndex = GetActiveChildIndex();
		
		if (ChildIndex != LastActiveChildIndex)
		{
			SetCurrentBlendProfile(GetBlendProfile());	// Blend profile can change based on the active child

			bool LastChildIndexIsInvalid = (LastActiveChildIndex == INDEX_NONE);
			
			const float CurrentWeight = PerBlendData[ChildIndex].Weight;
			const float DesiredWeight = 1.0f;
			const float WeightDifference = FMath::Clamp<float>(FMath::Abs<float>(DesiredWeight - CurrentWeight), 0.0f, 1.0f);

			// scale by the weight difference since we want always consistency:
   // 按重量差异进行缩放，因为我们希望始终保持一致性：
			// - if you're moving from 0 to full weight 1, it will use the normal blend time
   // - 如果您从 0 移动到全重 1，它将使用正常的混合时间
			// - if you're moving from 0.5 to full weight 1, it will get there in half the time
   // - 如果你从 0.5 移动到全权重 1，它会在一半的时间内到达那里
			float RemainingBlendTime;
			if (LastChildIndexIsInvalid)
			{
				RemainingBlendTime = 0.0f;
			}
			else if (GetTransitionType() == EBlendListTransitionType::Inertialization)
			{
				UE::Anim::IInertializationRequester* InertializationRequester = Context.GetMessage<UE::Anim::IInertializationRequester>();
				if (InertializationRequester)
				{
					FInertializationRequest Request;
					Request.Duration = CurrentBlendTimes[ChildIndex];
					Request.BlendProfile = CurrentBlendProfile;
					Request.bUseBlendMode = true;
					Request.BlendMode = GetBlendType();
					Request.CustomBlendCurve = GetCustomBlendCurve();
#if ANIM_TRACE_ENABLED
					Request.NodeId = Context.GetCurrentNodeId();
					Request.AnimInstance = Context.AnimInstanceProxy->GetAnimInstanceObject();
#endif

					InertializationRequester->RequestInertialization(Request);
					InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
					bRequestedInertializationOnActiveChildIndexChange = true;
				}
				else
				{
					FAnimNode_Inertialization::LogRequestError(Context, BlendPose[ChildIndex]);
				}
				
				RemainingBlendTime = 0.0f;
			}
			else
			{
				RemainingBlendTime = CurrentBlendTimes[ChildIndex] * WeightDifference;
			}

			for (int32 i = 0; i < PerBlendData.Num(); ++i)
			{
				PerBlendData[i].RemainingTime = RemainingBlendTime;
			}

			// If we have a valid previous child and we're instantly blending - update that pose with zero weight
   // 如果我们有一个有效的前一个孩子并且我们立即混合 - 以零权重更新该姿势
			if(RemainingBlendTime == 0.0f && !LastChildIndexIsInvalid)
			{
				BlendPose[LastActiveChildIndex].Update(Context.FractionalWeight(0.0f));
			}

			for(int32 i = 0; i < PerBlendData.Num(); ++i)
			{
				FAlphaBlend& Blend = PerBlendData[i].Blend;

				Blend.SetBlendTime(RemainingBlendTime);

				if(i == ChildIndex)
				{
					Blend.SetValueRange(PerBlendData[i].Weight, 1.0f);

					if (CurrentBlendProfile)
					{
						Blend.ResetAlpha();
						PerBlendData[i].StartAlpha = Blend.GetAlpha();
					}
				}
				else
				{
					Blend.SetValueRange(PerBlendData[i].Weight, 0.0f);
				}

				if (CurrentBlendProfile)
				{
					PerBlendData[i].StartAlpha = Blend.GetAlpha();
				}
			}

			// When bResetChildOnActivation is true and the weight of the new child is zero, we'll reinitialize the child.
   // 当 bResetChildOnActivation 为 true 并且新子项的权重为零时，我们将重新初始化该子项。
			if (GetChildUpdateMode() == EBlendListChildUpdateMode::ResetChildOnActivate && CurrentWeight <= ZERO_ANIMWEIGHT_THRESH)
			{
				FAnimationInitializeContext ReinitializeContext(Context.AnimInstanceProxy, Context.SharedContext);

				// reinitialize
    // 重新初始化
				BlendPose[ChildIndex].Initialize(ReinitializeContext);
			}

			LastActiveChildIndex = ChildIndex;
		}

		// Advance the weights
  // 推进重量
		//@TODO: This means we advance even in a frame where the target weights/times just got modified; is that desirable?
  // @TODO：这意味着即使在目标权重/时间刚刚修改的框架中我们也会前进；这是可取的吗？
		float SumWeight = 0.0f;
		for (int32 i = 0; i < PerBlendData.Num(); ++i)
		{
			float& BlendWeight = PerBlendData[i].Weight;

			FAlphaBlend& Blend = PerBlendData[i].Blend;
			Blend.Update(Context.GetDeltaTime());
			BlendWeight = Blend.GetBlendedValue();

			SumWeight += BlendWeight;
		}

		// Renormalize the weights
  // 重新标准化权重
		if ((SumWeight > ZERO_ANIMWEIGHT_THRESH) && (FMath::Abs<float>(SumWeight - 1.0f) > ZERO_ANIMWEIGHT_THRESH))
		{
			float ReciprocalSum = 1.0f / SumWeight;
			for (int32 i = 0; i < PerBlendData.Num(); ++i)
			{
				PerBlendData[i].Weight *= ReciprocalSum;
			}
		}

		// Update our active children
  // 更新我们活跃的孩子
		for (int32 i = 0; i < BlendPose.Num(); ++i)
		{
			const float BlendWeight = PerBlendData[i].Weight;
			if (BlendWeight > ZERO_ANIMWEIGHT_THRESH)
			{
				FAnimationUpdateContext ChildContext = Context.FractionalWeight(BlendWeight);
				
				UE::Anim::TOptionalScopedGraphMessage<UE::Anim::FAnimInertializationSyncScope> InertializationSync(bRequestedInertializationOnActiveChildIndexChange, ChildContext);
				BlendPose[i].Update((i == ChildIndex) ? ChildContext : ChildContext.AsInactive());
			}
			else if (GetChildUpdateMode() == EBlendListChildUpdateMode::AlwaysTickChildren)
			{
				// With always update children on, even if weight is 0 we will update the child
    // 启用始终更新子项后，即使权重为 0，我们也会更新子项
				FAnimationUpdateContext ChildContext = Context.FractionalWeight(FAnimWeight::GetSmallestRelevantWeight());
				UE::Anim::TOptionalScopedGraphMessage<UE::Anim::FAnimInertializationSyncScope> InertializationSync(bRequestedInertializationOnActiveChildIndexChange, ChildContext);
				BlendPose[i].Update((i == ChildIndex) ? ChildContext : ChildContext.AsInactive());
			}
		}

		// If we're using a blend profile, extract the scales and build blend sample data
  // 如果我们使用混合配置文件，请提取比例并构建混合样本数据
		if (CurrentBlendProfile)
		{
			for(int32 i = 0; i < BlendPose.Num(); ++i)
			{
				FBlendSampleData& PoseSampleData = PerBoneSampleData[i];
				const FBlendData& BlendData = PerBlendData[i];
				PoseSampleData.TotalWeight = BlendData.Weight;
				const bool bInverse = (CurrentBlendProfile->Mode == EBlendProfileMode::WeightFactor) ? (ChildIndex != i) : false;
				CurrentBlendProfile->UpdateBoneWeights(PoseSampleData, BlendData.Blend, BlendData.StartAlpha, BlendData.Weight, bInverse);
			}

			FBlendSampleData::NormalizeDataWeight(PerBoneSampleData);
		}
	}

#if ANIM_TRACE_ENABLED
	const int32 ChildIndex = GetActiveChildIndex();
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Active Index"), ChildIndex);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Active Weight"), PerBlendData[ChildIndex].Weight);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Active Blend Time"), CurrentBlendTimes[ChildIndex]);
#endif
}

void FAnimNode_BlendListBase::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	ANIM_MT_SCOPE_CYCLE_COUNTER(BlendPosesInGraph, !IsInGameThread());

	// Build local arrays to pass to the BlendPosesTogether runtime
 // 构建本地数组以传递给 BlendPosesTogether 运行时
	TArray<int32, TInlineAllocator<8>> PosesToEvaluate;
	TArray<float, TInlineAllocator<8>> BlendWeights;
	const int32 PerBlendDataCount = PerBlendData.Num();
	for(int32 BlendIndex = 0; BlendIndex < PerBlendDataCount; ++BlendIndex)
	{
		const FBlendData& BlendData = PerBlendData[BlendIndex];
		BlendWeights.Add(BlendData.Weight);
		if(BlendData.Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			PosesToEvaluate.Add(BlendIndex);
		}
	}

	const int32 NumPoses = PosesToEvaluate.Num();

	if ((NumPoses > 0) && (BlendPose.Num() == BlendWeights.Num()))
	{
		if(NumPoses == 1 && FAnimWeight::IsFullWeight(BlendWeights[PosesToEvaluate[0]]) && CurrentBlendProfile == nullptr)
		{
			// Single full weight pose - pass-through fast common case
   // 单一全重量姿势 - 快速通过常见情况
			BlendPose[PosesToEvaluate[0]].Evaluate(Output);
		}
		else
		{
			// Scratch arrays for evaluation, stack allocated
   // 用于评估的临时数组，分配堆栈
			TArray<FCompactPose, TInlineAllocator<8>> FilteredPoses;
			TArray<FBlendedCurve, TInlineAllocator<8>> FilteredCurve;
			TArray<UE::Anim::FStackAttributeContainer, TInlineAllocator<8>> FilteredAttributes;

			FilteredPoses.SetNum(NumPoses, EAllowShrinking::No);
			FilteredCurve.SetNum(NumPoses, EAllowShrinking::No);
			FilteredAttributes.SetNum(NumPoses, EAllowShrinking::No);

			int32 NumActivePoses = 0;
			for (int32 i = 0; i < PosesToEvaluate.Num(); ++i)
			{
				int32 PoseIndex = PosesToEvaluate[i];

				FPoseContext EvaluateContext(Output);

				FPoseLink& CurrentPose = BlendPose[PoseIndex];
				CurrentPose.Evaluate(EvaluateContext);

				FilteredPoses[i].MoveBonesFrom(EvaluateContext.Pose);
				FilteredCurve[i].MoveFrom(EvaluateContext.Curve);
				FilteredAttributes[i].MoveFrom(EvaluateContext.CustomAttributes);
			}

			FAnimationPoseData OutAnimationPoseData(Output);
		
			// Use the calculated blend sample data if we're blending per-bone
   // 如果我们要按骨骼混合，请使用计算出的混合样本数据
			if (CurrentBlendProfile)
			{
				const USkeleton* TargetSkeleton = Output.Pose.GetBoneContainer().GetSkeletonAsset();
				const USkeleton* SourceSkeleton = CurrentBlendProfile->OwningSkeleton;
				const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(SourceSkeleton, TargetSkeleton);
				if (SkeletonRemapping.IsValid())
				{
					FAnimationRuntime::BlendPosesTogetherPerBoneRemapped(FilteredPoses, FilteredCurve, FilteredAttributes, CurrentBlendProfile, PerBoneSampleData, PosesToEvaluate, SkeletonRemapping, OutAnimationPoseData);
				}
				else
				{
					FAnimationRuntime::BlendPosesTogetherPerBone(FilteredPoses, FilteredCurve, FilteredAttributes, CurrentBlendProfile, PerBoneSampleData, PosesToEvaluate, OutAnimationPoseData);
				}
			}
			else
			{
				FAnimationRuntime::BlendPosesTogether(FilteredPoses, FilteredCurve, FilteredAttributes, BlendWeights, PosesToEvaluate, OutAnimationPoseData);
			}
		}
	}
	else
	{
		Output.ResetToRefPose();
	}
}

void FAnimNode_BlendListBase::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	const int32 NumPoses = BlendPose.Num();
	const int32 ChildIndex = GetActiveChildIndex();

	FString DebugLine = GetNodeName(DebugData);
	const TArray<float>& CurrentBlendTimes = GetBlendTimes();
	DebugLine += FString::Printf(TEXT("(Active: (%i/%i) Weight: %.1f%% Time %.3f)"), ChildIndex+1, NumPoses, PerBlendData[ChildIndex].Weight*100.f, CurrentBlendTimes[ChildIndex]);

	DebugData.AddDebugItem(DebugLine);
	
	for(int32 Pose = 0; Pose < NumPoses; ++Pose)
	{
		BlendPose[Pose].GatherDebugData(DebugData.BranchFlow(PerBlendData[Pose].Weight));
	}
}

const TArray<float>& FAnimNode_BlendListBase::GetBlendTimes() const
{
	return GET_ANIM_NODE_DATA(TArray<float>, BlendTime);
}

EBlendListTransitionType FAnimNode_BlendListBase::GetTransitionType() const
{
	return GET_ANIM_NODE_DATA(EBlendListTransitionType, TransitionType);
}

EAlphaBlendOption FAnimNode_BlendListBase::GetBlendType() const
{
	return GET_ANIM_NODE_DATA(EAlphaBlendOption, BlendType);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
bool FAnimNode_BlendListBase::GetResetChildOnActivation() const
{
	return GetChildUpdateMode() == EBlendListChildUpdateMode::ResetChildOnActivate;
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

EBlendListChildUpdateMode FAnimNode_BlendListBase::GetChildUpdateMode() const
{
	return GET_ANIM_NODE_DATA(EBlendListChildUpdateMode, ChildUpateMode);
}

UCurveFloat* FAnimNode_BlendListBase::GetCustomBlendCurve() const
{
	return GET_ANIM_NODE_DATA(TObjectPtr<UCurveFloat>, CustomBlendCurve);
}

UBlendProfile* FAnimNode_BlendListBase::GetBlendProfile() const
{
	return GET_ANIM_NODE_DATA(TObjectPtr<UBlendProfile>, BlendProfile);
}
