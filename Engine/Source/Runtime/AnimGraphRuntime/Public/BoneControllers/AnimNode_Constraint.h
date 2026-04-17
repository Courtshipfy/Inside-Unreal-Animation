// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Constraint.h"
#include "EngineDefines.h"
#include "AnimNode_Constraint.generated.h"

/** 
 * Enum value to describe how you'd like to maintain offset
 */
UENUM()
enum class EConstraintOffsetOption : uint8
{
	None, // no offset
	Offset_RefPose // offset is based on reference pose
};


/** 
 * Constraint Set up
 *
 */
USTRUCT()
struct FConstraint
{
	GENERATED_USTRUCT_BODY()

	/** Target Bone this is constraint to */
	/** 目标骨骼这是约束 */
	/** 目标骨骼这是约束 */
	/** 目标骨骼这是约束 */
	UPROPERTY(EditAnywhere, Category = FConstraint)
	FBoneReference TargetBone;

	/** Maintain offset based on refpose or not.
	 * 
	 * None - no offset
	 * Offset_RefPose - offset is created based on reference pose
	 * 
	 * In the future, we'd like to support custom offset, not just based on ref pose
	 */
	UPROPERTY(EditAnywhere, Category = FConstraint)
	EConstraintOffsetOption	OffsetOption;
	/** 什么变换类型受到约束 - 平移、旋转、缩放或父级。父级覆盖所有组件 */

	/** 什么变换类型受到约束 - 平移、旋转、缩放或父级。父级覆盖所有组件 */
	/** What transform type is constraint to - Translation, Rotation, Scale OR Parent. Parent overrides all component */
	/** 什么变换类型受到约束 - 平移、旋转、缩放或父级。父级覆盖所有组件 */
	/** 每轴过滤器选项 - 应用于本地空间而不是世界空间 */
	UPROPERTY(EditAnywhere, Category = FConstraint)
	ETransformConstraintType TransformType;
	/** 每轴过滤器选项 - 应用于本地空间而不是世界空间 */

	/** 瞬态约束数据索引 */
	/** Per axis filter options - applied in their local space not in world space */
	/** 每轴过滤器选项 - 应用于本地空间而不是世界空间 */
	UPROPERTY(EditAnywhere, Category = FConstraint)
	/** 瞬态约束数据索引 */
	FFilterOptionPerAxis PerAxis;
	
	/** transient constraint data index */
	/** 瞬态约束数据索引 */
	int32 ConstraintDataIndex;

	void Initialize(const FBoneContainer& RequiredBones)
	{
		TargetBone.Initialize(RequiredBones);
	}

	bool IsValidToEvaluate(const FBoneContainer& RequiredBones) const
	{
		return TargetBone.IsValidToEvaluate(RequiredBones) && PerAxis.IsValid();
	}

	FConstraint()
		: OffsetOption(EConstraintOffsetOption::Offset_RefPose)
		, TransformType(ETransformConstraintType::Translation)
		, ConstraintDataIndex(INDEX_NONE)
	{}
};
/**
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 **/
 *	Constraint node to parent or world transform for rotation/translation
 */
USTRUCT()
struct FAnimNode_Constraint : public FAnimNode_SkeletalControlBase
	/** 约束条件列表 */
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 **/
{
	GENERATED_USTRUCT_BODY()

	/** 权重数据 - 后期编辑同步到 ConstraintSetups */
	/** Name of bone to control. This is the main bone chain to modify from. **/
	/** 约束条件列表 */
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 **/
	UPROPERTY(EditAnywhere, Category = SkeletalControl) 
	FBoneReference BoneToModify;

	/** 权重数据 - 后期编辑同步到 ConstraintSetups */
	/** List of constraints */
	/** 约束条件列表 */
	UPROPERTY(EditAnywhere, Category = Constraints)
	TArray<FConstraint> ConstraintSetup;

	/** Weight data - post edit syncs up to ConstraintSetups */
	/** 权重数据 - 后期编辑同步到 ConstraintSetups */
	UPROPERTY(EditAnywhere, editfixedsize, Category = Runtime, meta = (PinShownByDefault))
	TArray<float> ConstraintWeights;

	ANIMGRAPHRUNTIME_API FAnimNode_Constraint();

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

	// FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口结束
	/** 调试绘制缓存数据 */

#if WITH_EDITOR
	ANIMGRAPHRUNTIME_API void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp) const;
	/** 调试绘制缓存数据 */
#endif // WITH_EDITOR

private:
	// FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口结束

	TArray<FConstraintData>	ConstraintData;

#if UE_ENABLE_DEBUG_DRAWING
	/** Debug draw cached data */
	/** 调试绘制缓存数据 */
	FTransform CachedOriginalTransform;
	FTransform CachedConstrainedTransform;
	TArray<FTransform> CachedTargetTransforms;
#endif // UE_BUILD_SHIPPING
};
