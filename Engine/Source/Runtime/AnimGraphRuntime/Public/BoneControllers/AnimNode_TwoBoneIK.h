// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "CommonAnimTypes.h"
#include "AnimNode_TwoBoneIK.generated.h"

class USkeletalMeshComponent;

/**
 * Simple 2 Bone IK Controller.
 */

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_TwoBoneIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()
	
	/** Name of bone to control. This is the main bone chain to modify from. **/
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 **/
	UPROPERTY(EditAnywhere, Category=IK)
	FBoneReference IKBone;

	/** Limits to use if stretching is allowed. This value determines when to start stretch. For example, 0.9 means once it reaches 90% of the whole length of the limb, it will start apply. */
	/** 如果允许拉伸，则限制使用。该值决定何时开始拉伸。例如，0.9 表示一旦达到肢体全长的 90%，就会开始应用。 */
	UPROPERTY(EditAnywhere, Category=IK, meta = (editcondition = "bAllowStretching", ClampMin = "0.0", UIMin = "0.0"))
	double StartStretchRatio;

	/** Limits to use if stretching is allowed. This value determins what is the max stretch scale. For example, 1.5 means it will stretch until 150 % of the whole length of the limb.*/
	/** 如果允许拉伸，则限制使用。该值决定了最大拉伸比例。例如，1.5 表示它将拉伸到肢体整个长度的 150%。*/
	UPROPERTY(EditAnywhere, Category= IK, meta = (editcondition = "bAllowStretching", ClampMin = "0.0", UIMin = "0.0"))
	double MaxStretchScale;

#if WITH_EDITORONLY_DATA
	/** Limits to use if stretching is allowed - old property DEPRECATED */
	/** 如果允许拉伸，则使用限制 - 旧属性已弃用 */
	UPROPERTY()
	FVector2D StretchLimits_DEPRECATED;

	/** Whether or not to apply twist on the chain of joints. This clears the twist value along the TwistAxis */
	/** 是否在关节链上施加扭曲。这会清除沿 TwistAxis 的扭曲值 */
	UPROPERTY()
	bool bNoTwist_DEPRECATED;

	/** If JointTargetSpaceBoneName is a bone, this is the bone to use. **/
	/** 如果 JointTargetSpaceBoneName 是骨骼，则这就是要使用的骨骼。 **/
	UPROPERTY()
	FName JointTargetSpaceBoneName_DEPRECATED;

	/** If EffectorLocationSpace is a bone, this is the bone to use. **/
	/** 如果 EffectorLocationSpace 是骨骼，则这就是要使用的骨骼。 **/
	UPROPERTY()
	FName EffectorSpaceBoneName_DEPRECATED;
#endif

	/** Effector Location. Target Location to reach. */
	/** 效应器位置。要到达的目标位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effector, meta = (PinShownByDefault))
	FVector EffectorLocation;

	// cached limb index for upper
	// 上层的缓存肢体索引
	FCompactPoseBoneIndex CachedUpperLimbIndex;

	UPROPERTY(EditAnywhere, Category=Effector)
	FBoneSocketTarget EffectorTarget;

	/** Joint Target Location. Location used to orient Joint bone. **/
	/** 联合目标位置。用于定向关节骨的位置。 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=JointTarget, meta=(PinShownByDefault))
	FVector JointTargetLocation;

	// cached limb index for lower
	// 缓存肢体索引较低
	FCompactPoseBoneIndex CachedLowerLimbIndex;

	UPROPERTY(EditAnywhere, Category = JointTarget)
	FBoneSocketTarget JointTarget;

	/** Specify which axis it's aligned. Used when removing twist */
	/** 指定它对齐的轴。去除扭曲时使用 */
	UPROPERTY(EditAnywhere, Category = IK, meta = (editcondition = "!bAllowTwist"))
	FAxis TwistAxis;

	/** Reference frame of Effector Location. */
	/** 效应器位置的参考系。 */
	UPROPERTY(EditAnywhere, Category=Effector)
	TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace;

	/** Reference frame of Joint Target Location. */
	/** 联合目标定位参考系。 */
	UPROPERTY(EditAnywhere, Category=JointTarget)
	TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace;

	/** Should stretching be allowed, to be prevent over extension */
	/** 是否允许拉伸，以防止过度拉伸 */
	UPROPERTY(EditAnywhere, Category=IK)
	uint8 bAllowStretching:1;

	/** Set end bone to use End Effector rotation */
	/** 设置末端骨骼以使用末端执行器旋转 */
	UPROPERTY(EditAnywhere, Category=IK)
	uint8 bTakeRotationFromEffectorSpace : 1;

	/** Keep local rotation of end bone */
	/** 保持端骨局部旋转 */
	UPROPERTY(EditAnywhere, Category = IK)
	uint8 bMaintainEffectorRelRot : 1;

	/** Whether or not to apply twist on the chain of joints. This clears the twist value along the TwistAxis */
	/** 是否在关节链上施加扭曲。这会清除沿 TwistAxis 的扭曲值 */
	UPROPERTY(EditAnywhere, Category = IK)
	uint8 bAllowTwist : 1;

	ANIMGRAPHRUNTIME_API FAnimNode_TwoBoneIK();

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
#if WITH_EDITOR
	ANIMGRAPHRUNTIME_API void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp) const;
#endif // WITH_EDITOR
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束
	static ANIMGRAPHRUNTIME_API FTransform GetTargetTransform(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, FBoneSocketTarget& InTarget, EBoneControlSpace Space, const FVector& InOffset);
private:
	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

#if WITH_EDITOR
	FVector CachedJoints[3];
	FVector CachedJointTargetPos;
#endif // WITH_EDITOR
};
