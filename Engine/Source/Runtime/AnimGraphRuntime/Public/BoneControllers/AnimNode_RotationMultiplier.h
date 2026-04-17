// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_RotationMultiplier.generated.h"

class USkeletalMeshComponent;

/**
 *	Simple controller that multiplies scalar value to the translation/rotation/scale of a single bone.
 */

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_RotationMultiplier : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to control. This is the main bone chain to modify from. */
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 */
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 */
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 */
	UPROPERTY(EditAnywhere, Category=Multiplier) 
	FBoneReference	TargetBone;
	/** 从中获取变换的源 */

	/** 从中获取变换的源 */
	/** Source to get transform from */
	/** 从中获取变换的源 */
	UPROPERTY(EditAnywhere, Category=Multiplier)
	FBoneReference	SourceBone;

	// To make these to be easily pin-hookable, I'm not making it struct, but each variable
 // 为了使这些易于挂钩，我没有将其设为结构体，而是将每个变量设为
	// 0.f is invalid, and default
 // 0.f 无效，默认
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Multiplier, meta=(PinShownByDefault))
	float	Multiplier;

	UPROPERTY(EditAnywhere, Category=Multiplier)
	TEnumAsByte<EBoneAxis> RotationAxisToRefer;
	
	UPROPERTY(EditAnywhere, Category = Multiplier)
	bool bIsAdditive;	

	ANIMGRAPHRUNTIME_API FAnimNode_RotationMultiplier();

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
	// Extract Delta Quat of rotation around Axis of animation and reference pose for the SourceBoneIndex
 // 提取围绕动画轴旋转的 Delta Quat 和 SourceBoneIndex 的参考姿势
	FQuat ExtractAngle(const FTransform& RefPoseTransform, const FTransform& LocalBoneTransform, const EBoneAxis Axis);
	// Multiply scalar value Multiplier to the delta Quat of SourceBone Index's rotation
 // 将标量值乘以 SourceBone Index 旋转的 delta Quat
	FQuat MultiplyQuatBasedOnSourceIndex(const FTransform& RefPoseTransform, const FTransform& LocalBoneTransform, const EBoneAxis Axis, float InMultiplier, const FQuat& ReferenceQuat);

	// FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口结束
};
