// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_ModifyBone.generated.h"

class USkeletalMeshComponent;

UENUM(BlueprintType)
enum EBoneModificationMode : int
{
	/** The modifier ignores this channel (keeps the existing bone translation, rotation, or scale). */
	/** 修改器忽略此通道（保留现有的骨骼平移、旋转或缩放）。 */
	BMM_Ignore UMETA(DisplayName = "Ignore"),

	/** The modifier replaces the existing translation, rotation, or scale. */
	/** 修改器替换现有的平移、旋转或缩放。 */
	BMM_Replace UMETA(DisplayName = "Replace Existing"),

	/** The modifier adds to the existing translation, rotation, or scale. */
	/** 修改器添加到现有的平移、旋转或缩放。 */
	BMM_Additive UMETA(DisplayName = "Add to Existing")
};

/**
 *	Simple controller that replaces or adds to the translation/rotation of a single bone.
 */
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_ModifyBone : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to control. This is the main bone chain to modify from. **/
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 **/
	UPROPERTY(EditAnywhere, Category=SkeletalControl) 
	FBoneReference BoneToModify;

	/** New translation of bone to apply. */
	/** 要应用的骨骼的新翻译。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Translation, meta=(PinShownByDefault))
	FVector Translation;

	/** New rotation of bone to apply. */
	/** 要应用的骨骼的新旋转。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Rotation, meta=(PinShownByDefault))
	FRotator Rotation;

	/** New Scale of bone to apply. This is only worldspace. */
	/** 要应用的新骨骼比例。这只是世界空间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Scale, meta=(PinShownByDefault))
	FVector Scale;

	/** Whether and how to modify the translation of this bone. */
	/** 是否以及如何修改该骨骼的平移。 */
	UPROPERTY(EditAnywhere, Category=Translation)
	TEnumAsByte<EBoneModificationMode> TranslationMode;

	/** Whether and how to modify the translation of this bone. */
	/** 是否以及如何修改该骨骼的平移。 */
	UPROPERTY(EditAnywhere, Category=Rotation)
	TEnumAsByte<EBoneModificationMode> RotationMode;

	/** Whether and how to modify the translation of this bone. */
	/** 是否以及如何修改该骨骼的平移。 */
	UPROPERTY(EditAnywhere, Category=Scale)
	TEnumAsByte<EBoneModificationMode> ScaleMode;

	/** Reference frame to apply Translation in. */
	/** 应用翻译的参考框架。 */
	UPROPERTY(EditAnywhere, Category=Translation)
	TEnumAsByte<enum EBoneControlSpace> TranslationSpace;

	/** Reference frame to apply Rotation in. */
	/** 应用旋转的参考系。 */
	UPROPERTY(EditAnywhere, Category=Rotation)
	TEnumAsByte<enum EBoneControlSpace> RotationSpace;

	/** Reference frame to apply Scale in. */
	/** 应用缩放的参考系。 */
	UPROPERTY(EditAnywhere, Category=Scale)
	TEnumAsByte<enum EBoneControlSpace> ScaleSpace;

	ANIMGRAPHRUNTIME_API FAnimNode_ModifyBone();

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
