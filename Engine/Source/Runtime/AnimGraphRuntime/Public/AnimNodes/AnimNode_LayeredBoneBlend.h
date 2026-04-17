// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimData/BoneMaskFilter.h"
#include "AnimNode_LayeredBoneBlend.generated.h"

UENUM()
enum class ELayeredBoneBlendMode : uint8
{
	BranchFilter,
	BlendMask,
};

// Layered blend (per bone); has dynamic number of blendposes that can blend per different bone sets
// 分层混合（每个骨骼）；具有动态数量的混合姿势，可以根据不同的骨骼集进行混合
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_LayeredBoneBlend : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:
	/** 源姿势 */
	/** 源姿势 */
	/** The source pose */
	/** 源姿势 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink BasePose;
	/** 每层的混合姿势 */
	/** 每层的混合姿势 */

	/** Each layer's blended pose */
	/** 每层的混合姿势 */
	/** 是否使用分支过滤器或混合蒙版来指定输入姿势每骨骼的影响 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, editfixedsize, Category=Links, meta=(BlueprintCompilerGeneratedDefaults))
	/** 是否使用分支过滤器或混合蒙版来指定输入姿势每骨骼的影响 */
	TArray<FPoseLink> BlendPoses;

	/** Whether to use branch filters or a blend mask to specify an input pose per-bone influence */
	/** 是否使用分支过滤器或混合蒙版来指定输入姿势每骨骼的影响 */
	UPROPERTY(EditAnywhere, Category = Config)
	ELayeredBoneBlendMode BlendMode;

	/** 
	 * The blend masks to use for our layer inputs. Allows the use of per-bone alphas.
	 * Blend masks are used when BlendMode is BlendMask.
	 */
	UPROPERTY(EditAnywhere, editfixedsize, Category=Config, meta=(UseAsBlendMask=true))
	TArray<TObjectPtr<UBlendProfile>> BlendMasks;

	/** 
	 * Configuration for the parts of the skeleton to blend for each layer. Allows
	 * certain parts of the tree to be blended out or omitted from the pose.
	/** 每层的权重 */
	 * LayerSetup is used when BlendMode is BranchFilter.
	 */
	/** 每层的权重 */
	UPROPERTY(EditAnywhere, editfixedsize, Category=Config)
	TArray<FInputBlendPose> LayerSetup;

	/** The weights of each layer */
	/** 每层的权重 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, editfixedsize, Category=Runtime, meta=(BlueprintCompilerGeneratedDefaults, PinShownByDefault))
	TArray<float> BlendWeights;

protected:
	// transient data to handle weight and target weight
 // 用于处理重量和目标重量的瞬态数据
	// this array changes based on required bones
 // 该数组根据所需的骨骼而变化
	TArray<FPerBoneBlendWeight> DesiredBoneBlendWeights;
	TArray<FPerBoneBlendWeight> CurrentBoneBlendWeights;

	// Per-bone weights for the skeleton. Serialized as these are only relative to the skeleton, but can potentially
 // 骨骼的每根骨骼权重。序列化，因为这些仅与骨架相关，但有可能
	// be regenerated at runtime if the GUIDs dont match
 // 如果 GUID 不匹配，则在运行时重新生成
	UPROPERTY()
	TArray<FPerBoneBlendWeight>	PerBoneBlendWeights;

	// Per-curve source pose index
 // 每条曲线源位姿索引
	TBaseBlendedCurve<FDefaultAllocator, UE::Anim::FCurveElementIndexed> CurvePoseSourceIndices;

	// Guids for skeleton used to determine whether the PerBoneBlendWeights need rebuilding
 // 用于确定 PerBoneBlendWeights 是否需要重建的骨架指南
	UPROPERTY()
	FGuid SkeletonGuid;

	// Guid for virtual bones used to determine whether the PerBoneBlendWeights need rebuilding
 // 用于确定 PerBoneBlendWeights 是否需要重建的虚拟骨骼指南
	UPROPERTY()
	FGuid VirtualBoneGuid;

	// Serial number of the required bones container
 // 所需骨骼容器的序列号
	/** 是否在网格空间或局部空间中混合骨骼旋转 */
	uint16 RequiredBonesSerialNumber;

public:
	/*
	/** 是否在网格空间或局部空间中混合骨骼旋转 */
	/** 是否在根空间或网格空间中混合骨骼旋转 */
	 * Max LOD that this node is allowed to run
	 * For example if you have LODThreshold to be 2, it will run until LOD 2 (based on 0 index)
	 * when the component LOD becomes 3, it will stop update/evaluate
	/** 是否在根空间或网格空间中混合骨骼旋转 */
	 * currently transition would be issue and that has to be re-visited
	/** 是否在网格空间或局部空间中混合骨骼比例 */
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "LOD Threshold"))
	/** 是否在网格空间或局部空间中混合骨骼比例 */
	int32 LODThreshold;

	/** 如何将图层混合在一起 */
	/** Whether to blend bone rotations in mesh space or in local space */
	/** 如何将图层混合在一起 */
	/** 是否在网格空间或局部空间中混合骨骼旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Config)
	bool bMeshSpaceRotationBlend;
	/** 借出根运动时是否合并根骨骼的每骨骼混合权重 */
	/** 借出根运动时是否合并根骨骼的每骨骼混合权重 */

	/** Whether to blend bone rotations in root space or in mesh space */
	/** 是否在根空间或网格空间中混合骨骼旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Config, Meta = (Editcondition = bMeshSpaceRotationBlend))
	bool bRootSpaceRotationBlend;

	/** Whether to blend bone scales in mesh space or in local space */
	/** 是否在网格空间或局部空间中混合骨骼比例 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Config)
	bool bMeshSpaceScaleBlend;
	
	/** How to blend the layers together */
	/** 如何将图层混合在一起 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Config)
	TEnumAsByte<enum ECurveBlendOption::Type> CurveBlendOption;

	/** Whether to incorporate the per-bone blend weight of the root bone when lending root motion */
	/** 借出根运动时是否合并根骨骼的每骨骼混合权重 */
	UPROPERTY(EditAnywhere, Category = Config)
	bool bBlendRootMotionBasedOnRootBone;

	bool bHasRelevantPoses;

	FAnimNode_LayeredBoneBlend()
		: BlendMode(ELayeredBoneBlendMode::BranchFilter)
		, RequiredBonesSerialNumber(0)
		, LODThreshold(INDEX_NONE)
		, bMeshSpaceRotationBlend(false)
		, bRootSpaceRotationBlend(false)
		, bMeshSpaceScaleBlend(false)
		, CurveBlendOption(ECurveBlendOption::Override)
		, bBlendRootMotionBasedOnRootBone(true)
		, bHasRelevantPoses(false)
	{
	}

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

	void AddPose()
	{
		BlendWeights.Add(1.f);
		BlendPoses.AddDefaulted();
		SyncBlendMasksAndLayers();
	}

	void RemovePose(int32 PoseIndex)
	{
		BlendWeights.RemoveAt(PoseIndex);
		BlendPoses.RemoveAt(PoseIndex);

		if (BlendMasks.IsValidIndex(PoseIndex)) 
		{ 
			BlendMasks.RemoveAt(PoseIndex);
		}

		if (LayerSetup.IsValidIndex(PoseIndex)) 
		{ 
			LayerSetup.RemoveAt(PoseIndex); 
		}
		// just in case, we've seen problems.
  // 以防万一，我们已经看到了问题。
		SyncBlendMasksAndLayers();
	}

	// Set the blend mask for the specified input pose
 // 设置指定输入​​姿势的混合蒙版
	ANIMGRAPHRUNTIME_API void SetBlendMask(int32 InPoseIndex, UBlendProfile* InBlendMask);
	
	// Invalidate the cached per-bone blend weights from the skeleton
 // 使骨骼中缓存的每骨骼混合权重无效
	void InvalidatePerBoneBlendWeights() { RequiredBonesSerialNumber = 0; SkeletonGuid = FGuid(); VirtualBoneGuid = FGuid(); }
	
	// Invalidates the cached bone data so it is recalculated the next time this node is updated
 // 使缓存的骨骼数据无效，以便下次更新此节点时重新计算
	void InvalidateCachedBoneData() { RequiredBonesSerialNumber = 0; }

private:

	// just for the graph constructor to call.
 // 只是为了调用图形构造函数。
	void AddFirstPose()
	{
		check(BlendWeights.IsEmpty());
		BlendWeights.Add(1.f);
		BlendPoses.AddDefaulted();
		check(BlendMode == ELayeredBoneBlendMode::BranchFilter);
		LayerSetup.AddDefaulted();
	}

	// Synchronize the number of BlendMasks or Layers with the number of BlendPoses.
 // 将 BlendMask 或图层的数量与 BlendPoses 的数量同步。
	ANIMGRAPHRUNTIME_API void SyncBlendMasksAndLayers();
	
	// Rebuild cache per bone blend weights from the skeleton
 // 从骨架重建每个骨骼混合权重的缓存
	ANIMGRAPHRUNTIME_API void RebuildPerBoneBlendWeights(const USkeleton* InSkeleton);

	// Check whether per-bone blend weights are valid according to the skeleton (GUID check)
 // 根据骨架检查每个骨骼的混合权重是否有效（GUID检查）
	ANIMGRAPHRUNTIME_API bool ArePerBoneBlendWeightsValid(const USkeleton* InSkeleton) const;

	// Update cached data if required
 // 如果需要更新缓存数据
	ANIMGRAPHRUNTIME_API void UpdateCachedBoneData(const FBoneContainer& RequiredBones, const USkeleton* Skeleton);

	friend class UAnimGraphNode_LayeredBoneBlend;
};
