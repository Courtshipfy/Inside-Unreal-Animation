// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneIndices.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_Fabrik.generated.h"

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;

/**
*	Controller which implements the FABRIK IK approximation algorithm -  see http://www.academia.edu/9165835/FABRIK_A_fast_iterative_solver_for_the_Inverse_Kinematics_problem for details
*/

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_Fabrik : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Coordinates for target location of tip bone - if EffectorLocationSpace is bone, this is the offset from Target Bone to use as target location*/
	/** 尖端骨骼目标位置的坐标 - 如果 EffectorLocationSpace 是骨骼，则这是与用作目标位置的目标骨骼的偏移量*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector, meta = (PinShownByDefault))
	FTransform EffectorTransform;

	/** If EffectorTransformSpace is a bone, this is the bone to use. **/
	/** 如果 EffectorTransformSpace 是骨骼，则这就是要使用的骨骼。 **/
	UPROPERTY(EditAnywhere, Category = EndEffector)
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

	/** Reference frame of Effector Transform. */
	/** 效应器变换的参考系。 */
	UPROPERTY(EditAnywhere, Category = EndEffector)
	TEnumAsByte<enum EBoneControlSpace> EffectorTransformSpace;

	UPROPERTY(EditAnywhere, Category = EndEffector)
	TEnumAsByte<enum EBoneRotationSource> EffectorRotationSource;

#if WITH_EDITORONLY_DATA
	/** Toggle drawing of axes to debug joint rotation*/
	/** 切换轴绘图以调试关节旋转*/
	UPROPERTY(EditAnywhere, Category = Solver)
	bool bEnableDebugDraw;

	/** If EffectorTransformSpace is a bone, this is the bone to use. **/
	/** 如果 EffectorTransformSpace 是骨骼，则这就是要使用的骨骼。 **/
	UPROPERTY()
	FBoneReference EffectorTransformBone_DEPRECATED;
#endif

public:
	ANIMGRAPHRUNTIME_API FAnimNode_Fabrik();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	ANIMGRAPHRUNTIME_API virtual void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const;

private:
	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	// Convenience function to get current (pre-translation iteration) component space location of bone by bone index
	// 通过骨骼索引获取骨骼当前（翻译前迭代）组件空间位置的便捷函数
	FVector GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex);
	static FTransform GetTargetTransform(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, FBoneSocketTarget& InTarget, EBoneControlSpace Space, const FTransform& InOffset);
#if WITH_EDITORONLY_DATA
	// Cached CS location when in editor for debug drawing
	// 在编辑器中进行调试绘图时缓存 CS 位置
	FTransform CachedEffectorCSTransform;
#endif
};
