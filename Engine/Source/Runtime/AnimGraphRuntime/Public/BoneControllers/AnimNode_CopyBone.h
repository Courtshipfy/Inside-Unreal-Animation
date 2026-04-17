// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_CopyBone.generated.h"

class USkeletalMeshComponent;

/**
 *	Simple controller to copy a bone's transform to another one.
 */

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_CopyBone : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Source Bone Name to get transform from */
	/** 从中获取变换的源骨骼名称 */
	UPROPERTY(EditAnywhere, Category = Copy)
	FBoneReference SourceBone;

	/** Name of bone to control. This is the main bone chain to modify from. **/
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 **/
	UPROPERTY(EditAnywhere, Category=Copy) 
	FBoneReference TargetBone;

	/** If Translation should be copied */
	/** 如果翻译应该被复制 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Copy, meta=(PinShownByDefault))
	bool bCopyTranslation;

	/** If Rotation should be copied */
	/** 是否应复制旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Copy, meta=(PinShownByDefault))
	bool bCopyRotation;

	/** If Scale should be copied */
	/** 如果应复制比例 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Copy, meta=(PinShownByDefault))
	bool bCopyScale;

	/** Space to convert transforms into prior to copying components */
	/** 在复制组件之前将变换转换为的空间 */
	UPROPERTY(EditAnywhere, Category = Copy)
	TEnumAsByte<EBoneControlSpace> ControlSpace;

	ANIMGRAPHRUNTIME_API FAnimNode_CopyBone();

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
