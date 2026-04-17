// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "CCDIK.h"
#include "EngineDefines.h"
#include "AnimNode_CCDIK.generated.h"

/**
*	Controller which implements the CCDIK IK approximation algorithm
*/
USTRUCT()
struct FAnimNode_CCDIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Coordinates for target location of tip bone - if EffectorLocationSpace is bone, this is the offset from Target Bone to use as target location*/
	/** 尖端骨骼目标位置的坐标 - 如果 EffectorLocationSpace 是骨骼，则这是与用作目标位置的目标骨骼的偏移量*/
	UPROPERTY(EditAnywhere, Category = Effector, meta = (PinShownByDefault))
	FVector EffectorLocation;

	/** Reference frame of Effector Transform. */
	/** 效应器变换的参考系。 */
	UPROPERTY(EditAnywhere, Category = Effector)
	TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace;

	/** If EffectorTransformSpace is a bone, this is the bone to use. **/
	/** 如果 EffectorTransformSpace 是骨骼，则这就是要使用的骨骼。 **/
	UPROPERTY(EditAnywhere, Category = Effector)
	FBoneSocketTarget EffectorTarget;

	/** Name of tip bone */
	/** 尖端骨名称 */
	UPROPERTY(EditAnywhere, Category = Solver)
	FBoneReference TipBone;

	/** Name of the root bone*/
	/** 根骨名称*/
	UPROPERTY(EditAnywhere, Category = Solver)
	FBoneReference RootBone;

	/** Tolerance for final tip location delta from EffectorLocation*/
	/** 最终尖端位置与 EffectorLocation 增量的公差*/
	UPROPERTY(EditAnywhere, Category = Solver)
	float Precision;

	/** Maximum number of iterations allowed, to control performance. */
	/** 允许的最大迭代次数，以控制性能。 */
	UPROPERTY(EditAnywhere, Category = Solver)
	int32 MaxIterations;

	/** Toggle drawing of axes to debug joint rotation*/
	/** 切换轴绘图以调试关节旋转*/
	UPROPERTY(EditAnywhere, Category = Solver)
	bool bStartFromTail;

	/** Tolerance for final tip location delta from EffectorLocation*/
	/** 最终尖端位置与 EffectorLocation 增量的公差*/
	UPROPERTY(EditAnywhere, Category = Solver)
	bool bEnableRotationLimit;

private:
	/** symmetry rotation limit per joint. Index 0 matches with root bone and last index matches with tip bone. */
	/** 每个关节的对称旋转限制。索引 0 与根骨骼匹配，最后一个索引与尖端骨骼匹配。 */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = Solver)
	TArray<float> RotationLimitPerJoints;

public:
	ANIMGRAPHRUNTIME_API FAnimNode_CCDIK();

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

private:
	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	// Convenience function to get current (pre-translation iteration) component space location of bone by bone index
	// 通过骨骼索引获取骨骼当前（翻译前迭代）组件空间位置的便捷函数
	FVector GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex);

	static FTransform GetTargetTransform(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, FBoneSocketTarget& InTarget, EBoneControlSpace Space, const FVector& InOffset);

public:
#if WITH_EDITOR
#if UE_ENABLE_DEBUG_DRAWING
	TArray<FVector> DebugLines;
#endif // #if UE_ENABLE_DEBUG_DRAWING
	// resize rotation limit array based on set up
	// 根据设置调整旋转限制数组的大小
	ANIMGRAPHRUNTIME_API void ResizeRotationLimitPerJoints(int32 NewSize);
#endif
};
