// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"

#include "Animation/AnimCurveUtils.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimStats.h"
#include "Animation/AnimTrace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_CopyPoseFromMesh)

/////////////////////////////////////////////////////
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh
// FAnimNode_CopyPoseFromMesh

FAnimNode_CopyPoseFromMesh::FAnimNode_CopyPoseFromMesh()
	: SourceMeshComponent(nullptr)
	, bUseAttachedParent (false)
	, bCopyCurves (false)
	, bCopyCustomAttributes(false)
	, bUseMeshPose (false)
{
}

void FAnimNode_CopyPoseFromMesh::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_Base::Initialize_AnyThread(Context);

	// Initial update of the node, so we dont have a frame-delay on setup
 // 节点的初始更新，因此我们在设置时没有帧延迟
	GetEvaluateGraphExposedInputs().Execute(Context);
}

void FAnimNode_CopyPoseFromMesh::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)

}

void FAnimNode_CopyPoseFromMesh::RefreshMeshComponent(USkeletalMeshComponent* TargetMeshComponent)
{
	auto ResetMeshComponent = [this](USkeletalMeshComponent* InMeshComponent, USkeletalMeshComponent* InTargetMeshComponent)
	{
		USkeletalMeshComponent* CurrentMeshComponent = CurrentlyUsedSourceMeshComponent.Get();
		// if current mesh exists, but not same as input mesh
  // 如果当前网格存在，但与输入网格不同
		if (CurrentMeshComponent)
		{
			// if component has been changed, reinitialize
   // 如果组件已更改，则重新初始化
			if (CurrentMeshComponent != InMeshComponent)
			{
				ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
			}
			// if component is still same but mesh has been changed, we have to reinitialize
   // 如果组件仍然相同但网格已更改，我们必须重新初始化
			else if (CurrentMeshComponent->GetSkeletalMeshAsset() != CurrentlyUsedSourceMesh.Get())
			{
				ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
			}
			else if (InTargetMeshComponent)
			{
				// see if target mesh has changed
    // 查看目标网格是否已更改
				if (InTargetMeshComponent->GetSkeletalMeshAsset() != CurrentlyUsedTargetMesh.Get())
				{
					ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
				}
			}
		}
		// if not valid, but input mesh is
  // 如果无效，但输入网格是
		else if (!CurrentMeshComponent && InMeshComponent)
		{
			ReinitializeMeshComponent(InMeshComponent, InTargetMeshComponent);
		}
	};

	if(SourceMeshComponent.IsValid())
	{
		ResetMeshComponent(SourceMeshComponent.Get(), TargetMeshComponent);
	}
	else if (bUseAttachedParent)
	{
		if (TargetMeshComponent)
		{
			// Walk up the attachment chain until we find a skeletal mesh component
   // 沿着附件链向上走，直到找到骨架网格物体组件
			USkeletalMeshComponent* ParentMeshComponent = nullptr;
			for (USceneComponent* AttachParentComp = TargetMeshComponent->GetAttachParent(); AttachParentComp != nullptr; AttachParentComp = AttachParentComp->GetAttachParent())
			{
				ParentMeshComponent = Cast<USkeletalMeshComponent>(AttachParentComp);
				if (ParentMeshComponent)
				{
					break;
				}
			}

			if (ParentMeshComponent)
			{
				ResetMeshComponent(ParentMeshComponent, TargetMeshComponent);
			}
			else
			{
				CurrentlyUsedSourceMeshComponent.Reset();
			}
		}
		else
		{
			CurrentlyUsedSourceMeshComponent.Reset();
		}
	}
	else
	{
		CurrentlyUsedSourceMeshComponent.Reset();
	}
}

void FAnimNode_CopyPoseFromMesh::PreUpdate(const UAnimInstance* InAnimInstance)
{
	QUICK_SCOPE_CYCLE_COUNTER(FAnimNode_CopyPoseFromMesh_PreUpdate);

	RefreshMeshComponent(InAnimInstance->GetSkelMeshComponent());

	USkeletalMeshComponent* CurrentMeshComponent = CurrentlyUsedSourceMeshComponent.IsValid() ? CurrentlyUsedSourceMeshComponent.Get() : nullptr;

	if (CurrentMeshComponent && CurrentMeshComponent->GetSkeletalMeshAsset() && CurrentMeshComponent->IsRegistered())
	{
		// If our source is running under leader-pose, then get bone data from there
  // 如果我们的源在领导者姿势下运行，则从那里获取骨骼数据
		if(USkeletalMeshComponent* LeaderPoseComponent = Cast<USkeletalMeshComponent>(CurrentMeshComponent->LeaderPoseComponent.Get()))
		{
			CurrentMeshComponent = LeaderPoseComponent;
		}

		// re-check mesh component validity as it may have changed to leader
  // 重新检查网格组件有效性，因为它可能已更改为领导者
		if(CurrentMeshComponent->GetSkeletalMeshAsset() && CurrentMeshComponent->IsRegistered())
		{
			const bool bUROInSync = CurrentMeshComponent->ShouldUseUpdateRateOptimizations() && CurrentMeshComponent->AnimUpdateRateParams != nullptr && CurrentMeshComponent->AnimUpdateRateParams == InAnimInstance->GetSkelMeshComponent()->AnimUpdateRateParams;
			const bool bUsingExternalInterpolation = CurrentMeshComponent->IsUsingExternalInterpolation();
			const TArray<FTransform>& CachedComponentSpaceTransforms = CurrentMeshComponent->GetCachedComponentSpaceTransforms();
			const bool bArraySizesMatch = CachedComponentSpaceTransforms.Num() == CurrentMeshComponent->GetComponentSpaceTransforms().Num();

			// Copy source array from the appropriate location
   // 从适当的位置复制源数组
			SourceMeshTransformArray.Reset();
			SourceMeshTransformArray.Append((bUROInSync || bUsingExternalInterpolation) && bArraySizesMatch ? CachedComponentSpaceTransforms : CurrentMeshComponent->GetComponentSpaceTransforms());

			// Ref skeleton is need for parent index lookups later, so store it now
   // 稍后父索引查找需要引用骨架，所以现在存储它
			CurrentlyUsedMesh = CurrentMeshComponent->GetSkeletalMeshAsset();

			if(bCopyCurves)
			{
				if (CurrentMeshComponent->bEnableAnimation == false)
				{
					// Assume this is using the anim next path. Only curves directly in the mesh component are valid.
     // 假设这是使用动画下一个路径。只有直接位于网格组件中的曲线才有效。
					// @TODO: Replace the path below with this one after validating performance and behavior.
     // @TODO：验证性能和行为后，将下面的路径替换为该路径。
					SourceCurves.CopyFrom(CurrentMeshComponent->GetAnimCurves());
				}
				else if (UAnimInstance* SourceAnimInstance = CurrentMeshComponent->GetAnimInstance())
				{
					// Potential optimization/tradeoff: If we stored the curve results on the mesh component in non-editor scenarios, this would be
     // 潜在的优化/权衡：如果我们在非编辑器场景中将曲线结果存储在网格组件上，这将是
					// much faster (but take more memory). As it is, we need to translate the map stored on the anim instance.
     // 速度更快（但占用更多内存）。事实上，我们需要翻译存储在动画实例上的地图。
					const TMap<FName, float>& AnimCurveList = SourceAnimInstance->GetAnimationCurveList(EAnimCurveType::AttributeCurve);
					UE::Anim::FCurveUtils::BuildUnsorted(SourceCurves, AnimCurveList);
				}
				else
				{
					SourceCurves.Empty();
				}
			}

			if (bCopyCustomAttributes)
			{
				SourceCustomAttributes.CopyFrom(CurrentMeshComponent->GetCustomAttributes());
			}
		}
		else
		{
			CurrentlyUsedMesh.Reset();
		}
	}
}

void FAnimNode_CopyPoseFromMesh::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	// This introduces a frame of latency in setting the pin-driven source component,
 // 这在设置引脚驱动源组件时引入了延迟帧，
	// but we cannot do the work to extract transforms on a worker thread as it is not thread safe.
 // 但我们无法在工作线程上提取转换，因为它不是线程安全的。
	GetEvaluateGraphExposedInputs().Execute(Context);

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Component"), *GetNameSafe(CurrentlyUsedSourceMeshComponent.IsValid() ? CurrentlyUsedSourceMeshComponent.Get() : nullptr));
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Mesh"), *GetNameSafe(CurrentlyUsedSourceMeshComponent.IsValid() ? CurrentlyUsedSourceMeshComponent.Get()->GetSkeletalMeshAsset() : nullptr));
}

void FAnimNode_CopyPoseFromMesh::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(CopyPoseFromMesh, !IsInGameThread());

	FCompactPose& OutPose = Output.Pose;
	OutPose.ResetToRefPose();
	USkeletalMesh* CurrentMesh = CurrentlyUsedMesh.IsValid() ? CurrentlyUsedMesh.Get() : nullptr;
	if(SourceMeshTransformArray.Num() > 0 && CurrentMesh)
	{
		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();

		if (bUseMeshPose)
		{
			FCSPose<FCompactPose> MeshPoses;
			MeshPoses.InitPose(OutPose);

			for (FCompactPoseBoneIndex PoseBoneIndex : OutPose.ForEachBoneIndex())
			{
				const FMeshPoseBoneIndex MeshBoneIndex = RequiredBones.MakeMeshPoseIndex(PoseBoneIndex);
				const int32* Value = BoneMapToSource.Find(MeshBoneIndex.GetInt());
 				if (Value && SourceMeshTransformArray.IsValidIndex(*Value))
				{
					const int32 SourceBoneIndex = *Value;
					MeshPoses.SetComponentSpaceTransform(PoseBoneIndex, SourceMeshTransformArray[SourceBoneIndex]);
				}
			}

			FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(MeshPoses, OutPose);
		}
		else
		{
			for (FCompactPoseBoneIndex PoseBoneIndex : OutPose.ForEachBoneIndex())
			{
				const FMeshPoseBoneIndex MeshBoneIndex = RequiredBones.MakeMeshPoseIndex(PoseBoneIndex);
				const int32* Value = BoneMapToSource.Find(MeshBoneIndex.GetInt());
				if (Value && SourceMeshTransformArray.IsValidIndex(*Value))
				{
					const int32 SourceBoneIndex = *Value;
					const int32 ParentIndex = CurrentMesh->GetRefSkeleton().GetParentIndex(SourceBoneIndex);
					const FCompactPoseBoneIndex MyParentIndex = RequiredBones.GetParentBoneIndex(PoseBoneIndex);
					// only apply if I also have parent, otherwise, it should apply the space bases
     // 仅当我也有父母时才适用，否则，它应该应用太空基地
					if (SourceMeshTransformArray.IsValidIndex(ParentIndex) && MyParentIndex != INDEX_NONE)
					{
						const FTransform& ParentTransform = SourceMeshTransformArray[ParentIndex];
						const FTransform& ChildTransform = SourceMeshTransformArray[SourceBoneIndex];
						OutPose[PoseBoneIndex] = ChildTransform.GetRelativeTransform(ParentTransform);
					}
					else
					{
						OutPose[PoseBoneIndex] = SourceMeshTransformArray[SourceBoneIndex];
					}
				}
			}
		}
	}

	if (bCopyCurves)
	{
		Output.Curve.CopyFrom(SourceCurves);
	}

	if (bCopyCustomAttributes)
	{	
		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
		UE::Anim::Attributes::CopyAndRemapAttributes(SourceCustomAttributes, Output.CustomAttributes, SourceBoneToTarget, RequiredBones);		
	}
}

void FAnimNode_CopyPoseFromMesh::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += FString::Printf(TEXT("('%s')"), *GetNameSafe(CurrentlyUsedSourceMeshComponent.IsValid() ? CurrentlyUsedSourceMeshComponent.Get()->GetSkeletalMeshAsset() : nullptr));
	DebugData.AddDebugItem(DebugLine, true);
}

void FAnimNode_CopyPoseFromMesh::ReinitializeMeshComponent(USkeletalMeshComponent* NewSourceMeshComponent, USkeletalMeshComponent* TargetMeshComponent)
{
	CurrentlyUsedSourceMeshComponent.Reset();
	// reset source mesh
 // 重置源网格
	CurrentlyUsedSourceMesh.Reset();
	CurrentlyUsedTargetMesh.Reset();
	BoneMapToSource.Reset();

	if (TargetMeshComponent && IsValid(NewSourceMeshComponent) && NewSourceMeshComponent->GetSkeletalMeshAsset())
	{
		USkeletalMesh* SourceSkelMesh = NewSourceMeshComponent->GetSkeletalMeshAsset();
		USkeletalMesh* TargetSkelMesh = TargetMeshComponent->GetSkeletalMeshAsset();
		
		if (IsValid(SourceSkelMesh) && !SourceSkelMesh->HasAnyFlags(RF_NeedPostLoad) &&
			IsValid(TargetSkelMesh) && !TargetSkelMesh->HasAnyFlags(RF_NeedPostLoad))
		{
			CurrentlyUsedSourceMeshComponent = NewSourceMeshComponent;
			CurrentlyUsedSourceMesh = SourceSkelMesh;
			CurrentlyUsedTargetMesh = TargetSkelMesh;

			if (SourceSkelMesh == TargetSkelMesh)
			{
				for(int32 ComponentSpaceBoneId = 0; ComponentSpaceBoneId < SourceSkelMesh->GetRefSkeleton().GetNum(); ++ComponentSpaceBoneId)
				{
					BoneMapToSource.Add(ComponentSpaceBoneId, ComponentSpaceBoneId);
				}
			}
			else
			{
				const int32 SplitBoneIndex = (RootBoneToCopy != NAME_Name)? TargetSkelMesh->GetRefSkeleton().FindBoneIndex(RootBoneToCopy) : INDEX_NONE;
				for (int32 ComponentSpaceBoneId = 0; ComponentSpaceBoneId < TargetSkelMesh->GetRefSkeleton().GetNum(); ++ComponentSpaceBoneId)
				{
					if (SplitBoneIndex == INDEX_NONE || ComponentSpaceBoneId == SplitBoneIndex
						|| TargetSkelMesh->GetRefSkeleton().BoneIsChildOf(ComponentSpaceBoneId, SplitBoneIndex))
					{
						FName BoneName = TargetSkelMesh->GetRefSkeleton().GetBoneName(ComponentSpaceBoneId);
						BoneMapToSource.Add(ComponentSpaceBoneId, SourceSkelMesh->GetRefSkeleton().FindBoneIndex(BoneName));
					}
				}
			}

			if (bCopyCustomAttributes)
			{
				SourceBoneToTarget.Reserve(BoneMapToSource.Num());
				Algo::Transform(BoneMapToSource, SourceBoneToTarget, [](const TPair<int32, int32>& Pair)
				{
					return TPair<int32, int32>(Pair.Value, Pair.Key);
				});
			}
		}
	}
}

