// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "Animation/AnimCurveTypes.h"
#include "CommonAnimTypes.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_TwistCorrectiveNode.generated.h"

/** Reference Bone Frame */
/** 参考骨架 */
USTRUCT()
struct FReferenceBoneFrame
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "FReferenceBoneFrame")
	FBoneReference Bone;

	UPROPERTY(EditAnywhere, Category = "FReferenceBoneFrame")
	FAxis Axis;

	FReferenceBoneFrame() {};
};

/**
 * This is the node that apply corrective morphtarget for twist 
 * Good example is that if you twist your neck too far right or left, you're going to see odd stretch shape of neck, 
 * This node can detect the angle and apply morphtarget curve 
 * This isn't the twist control node for bone twist
 */
USTRUCT()
struct FAnimNode_TwistCorrectiveNode : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Base Frame of the reference for the twist node */
	/** 扭曲节点参考的基架 */
	UPROPERTY(EditAnywhere, Category="Reference Frame")
	FReferenceBoneFrame BaseFrame;

	// Transform component to use as input
	// 转换组件以用作输入
	UPROPERTY(EditAnywhere, Category="Reference Frame")
	FReferenceBoneFrame TwistFrame;

	/** Normal of the Plane that we'd like to calculate angle calculation from in BaseFrame. Please note we're looking for Normal Axis */
	/** 我们想要在 BaseFrame 中计算角度的平面法线。请注意，我们正在寻找法线轴 */
	UPROPERTY(EditAnywhere, Category = "Reference Frame")
	FAxis TwistPlaneNormalAxis;

	// @todo since this isn't used yet, I'm commenting it out. 
	// @todo 因为尚未使用它，所以我将其注释掉。
	// The plan is to support mapping curve between input to output
	// 该计划是支持输入到输出之间的映射曲线
	//UPROPERTY(EditAnywhere, Category = "Mapping")
	//UPROPERTY（EditAnywhere，类别 =“映射”）
	//FAlphaBlend MappingCurve;
	//FAlphaBlend 映射曲线；

 	// Maximum limit of the input value (mapped to RemappedMax, only used when limiting the source range)
 	// 输入值的最大限制（映射到RemappedMax，仅在限制源范围时使用）
 	// We can't go more than 180 right now because this is dot product driver
 	// 我们现在不能超过 180，因为这是点积驱动程序
	UPROPERTY(EditAnywhere, Category = "Mapping", meta = (UIMin = 0.f, ClampMin = 0.f, UIMax = 90.f, ClampMax = 90.f, DisplayName = "Max Angle In Degree"))
 	float RangeMax;

 	// Minimum value to apply to the destination (remapped from the input range)
 	// 适用于目标的最小值（从输入范围重新映射）
	UPROPERTY(EditAnywhere, Category = "Mapping", meta = (EditCondition = bUseRange, DisplayName = "Mapped Range Min"))
 	float RemappedMin;
 
 	// Maximum value to apply to the destination (remapped from the input range)
 	// 适用于目标的最大值（从输入范围重新映射）
	UPROPERTY(EditAnywhere, Category = "Mapping", meta = (EditCondition = bUseRange, DisplayName = "Mapped Range Max"))
 	float RemappedMax;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FAnimCurveParam Curve_DEPRECATED;
#endif

	UPROPERTY(EditAnywhere, Category = "Output Curve")
	FName CurveName;

public:
	ANIMGRAPHRUNTIME_API FAnimNode_TwistCorrectiveNode();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)  override;
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	// Type traits support
	// 类型特征支持
	ANIMGRAPHRUNTIME_API bool Serialize(FArchive& Ar);
	ANIMGRAPHRUNTIME_API void PostSerialize(const FArchive& Ar);

protected:
	
	// FAnimNode_SkeletalControlBase protected interface
	// FAnimNode_SkeletalControlBase 受保护接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

private:
	// Reference Pose Angle
	// 参考位姿角
	float ReferenceAngle;
	// Ranged Max in Radian, so that we don't keep having to convert
	// 最大范围以弧度表示，这样我们就不必一直进行转换
	float RangeMaxInRadian;

	// Get Reference Axis from the MeshBases
	// 从 MeshBase 获取参考轴
	FVector GetReferenceAxis(FCSPose<FCompactPose>& MeshBases, const FReferenceBoneFrame& Reference) const;
	// Get Angle of Base, and Twist from Reference Bone Transform
	// 从参考骨骼变换中获取基础角度和扭曲
	float	GetAngle(const FVector& Base, const FVector& Twist, const FTransform& ReferencetBoneTransform) const;
};

template<>
struct TStructOpsTypeTraits<FAnimNode_TwistCorrectiveNode> : public TStructOpsTypeTraitsBase2<FAnimNode_TwistCorrectiveNode>
{
	enum
	{
		WithSerializer = true,
		WithPostSerialize = true
	};
};
