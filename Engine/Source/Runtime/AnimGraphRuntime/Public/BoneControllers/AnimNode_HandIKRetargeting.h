// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_HandIKRetargeting.generated.h"

class USkeletalMeshComponent;

/**
 * Node to handle re-targeting of Hand IK bone chain.
 * It looks at position in Mesh Space of Left and Right FK bones, and moves Left and Right IK bones to those.
 * based on HandFKWeight. (0 = favor left hand, 1 = favor right hand, 0.5 = equal weight).
 * This is used so characters of different proportions can handle the same props.
 */

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_HandIKRetargeting : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Bone for Right Hand FK */
	/** 右手 FK 骨头 */
	/** 右手 FK 骨头 */
	/** 右手 FK 骨头 */
	UPROPERTY(EditAnywhere, Category = "HandIKRetargeting")
	FBoneReference RightHandFK;
	/** 左手 FK 骨头 */

	/** 左手 FK 骨头 */
	/** Bone for Left Hand FK */
	/** 左手 FK 骨头 */
	/** 右手 IK 骨骼 */
	UPROPERTY(EditAnywhere, Category = "HandIKRetargeting")
	FBoneReference LeftHandFK;
	/** 右手 IK 骨骼 */

	/** 左手 IK 骨骼 */
	/** Bone for Right Hand IK */
	/** 右手 IK 骨骼 */
	UPROPERTY(EditAnywhere, Category = "HandIKRetargeting")
	/** 左手 IK 骨骼 */
	/** IK 骨骼移动。 */
	FBoneReference RightHandIK;

	/** Bone for Left Hand IK */
	/** 左手 IK 骨骼 */
	/** IK 骨骼移动。 */
	UPROPERTY(EditAnywhere, Category = "HandIKRetargeting")
	FBoneReference LeftHandIK;

	/** 偏向哪只手。 0.5 表示双手权重相等，1 = 右手，0 = 左手。 */
	/** IK Bones to move. */
	/** IK 骨骼移动。 */
	UPROPERTY(EditAnywhere, Category = "HandIKRetargeting")
	TArray<FBoneReference> IKBonesToMove;

	/** 偏向哪只手。 0.5 表示双手权重相等，1 = 右手，0 = 左手。 */
	// Alpha values per axis to apply on the resulting retargeting translation
 // 每个轴的 Alpha 值应用于生成的重定向翻译
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HandIKRetargeting", meta = (PinHiddenByDefault))
	FVector PerAxisAlpha;

	/** Which hand to favor. 0.5 is equal weight for both, 1 = right hand, 0 = left hand. */
	/** 偏向哪只手。 0.5 表示双手权重相等，1 = 右手，0 = 左手。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HandIKRetargeting", meta = (PinShownByDefault))
	float HandFKWeight;

	ANIMGRAPHRUNTIME_API FAnimNode_HandIKRetargeting();

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
