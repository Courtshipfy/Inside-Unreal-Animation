// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_ObserveBone.generated.h"

class USkeletalMeshComponent;

/**
 *	Debugging node that displays the current value of a bone in a specific space.
 */
USTRUCT()
struct FAnimNode_ObserveBone : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to observe. */
	/** 要观察的骨骼的名称。 */
	/** 要观察的骨骼的名称。 */
	/** 要观察的骨骼的名称。 */
	UPROPERTY(EditAnywhere, Category=SkeletalControl)
	FBoneReference BoneToObserve;
	/** 显示骨骼变换的参考框架。 */

	/** 显示骨骼变换的参考框架。 */
	/** Reference frame to display the bone transform in. */
	/** 显示骨骼变换的参考框架。 */
	/** 显示与参考姿势的差异？ */
	UPROPERTY(EditAnywhere, Category=SkeletalControl)
	TEnumAsByte<EBoneControlSpace> DisplaySpace;
	/** 显示与参考姿势的差异？ */

	/** 所观察骨骼的平移。 */
	/** Show the difference from the reference pose? */
	/** 显示与参考姿势的差异？ */
	UPROPERTY(EditAnywhere, Category=SkeletalControl)
	/** 所观察骨骼的平移。 */
	/** 观察骨骼的旋转。 */
	bool bRelativeToRefPose;

	/** Translation of the bone being observed. */
	/** 所观察骨骼的平移。 */
	/** 正在观察的骨头的鳞片。 */
	/** 观察骨骼的旋转。 */
	UPROPERTY()
	FVector Translation;

	/** Rotation of the bone being observed. */
	/** 正在观察的骨头的鳞片。 */
	/** 观察骨骼的旋转。 */
	UPROPERTY()
	FRotator Rotation;

	/** Scale of the bone being observed. */
	/** 正在观察的骨头的鳞片。 */
	UPROPERTY()
	FVector Scale;

public:
	ANIMGRAPHRUNTIME_API FAnimNode_ObserveBone();

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
};
