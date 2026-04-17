// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Animation/AnimTypes.h"
#include "AnimNode_BoneDrivenController.generated.h"

class UCurveFloat;
class USkeletalMeshComponent;

// Evaluation of the bone transforms relies on the size and ordering of this
// 骨骼变换的评估取决于骨骼变换的大小和顺序
// enum, if this needs to change make sure EvaluateSkeletalControl_AnyThread is updated.
// 枚举，如果需要更改，请确保更新 EvaluateSkeletalControl_AnyThread。

// The type of modification to make to the destination component(s)
// 对目标组件进行的修改类型
UENUM()
enum class EDrivenBoneModificationMode : uint8
{
	// Add the driven value to the input component value(s)
	// 将驱动值添加到输入组件值
	AddToInput,

	// Replace the input component value(s) with the driven value
	// 将输入组件值替换为驱动值
	ReplaceComponent,

	// Add the driven value to the reference pose value
	// 将驱动值添加到参考位姿值
	AddToRefPose
};

// Type of destination value to drive
// 要驱动的目标值的类型
UENUM()
enum class EDrivenDestinationMode : uint8
{	
	Bone,
	MorphTarget,
	MaterialParameter
};

/**
 * This is the runtime version of a bone driven controller, which maps part of the state from one bone to another (e.g., 2 * source.x -> target.z)
 */
USTRUCT()
struct FAnimNode_BoneDrivenController : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	// Bone to use as controller input
	// 用作控制器输入的骨骼
	UPROPERTY(EditAnywhere, Category="Source (driver)")
	FBoneReference SourceBone;

	/** Curve used to map from the source attribute to the driven attributes if present (otherwise the Multiplier will be used) */
	/** 用于从源属性映射到驱动属性（如果存在）的曲线（否则将使用乘数） */
	UPROPERTY(EditAnywhere, Category=Mapping)
	TObjectPtr<UCurveFloat> DrivingCurve;

	// Multiplier to apply to the input value (Note: Ignored when a curve is used)
	// 应用于输入值的乘数（注意：使用曲线时忽略）
	UPROPERTY(EditAnywhere, Category=Mapping)
	float Multiplier;

	// Minimum limit of the input value (mapped to RemappedMin, only used when limiting the source range)
	// 输入值的最小限制（映射到RemappedMin，仅在限制源范围时使用）
	// If this is rotation, the unit is radian
	// 如果是旋转，单位是弧度
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(EditCondition=bUseRange, DisplayName="Source Range Min"))
	double RangeMin;

	// Maximum limit of the input value (mapped to RemappedMax, only used when limiting the source range)
	// 输入值的最大限制（映射到RemappedMax，仅在限制源范围时使用）
	// If this is rotation, the unit is radian
	// 如果是旋转，单位是弧度
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(EditCondition=bUseRange, DisplayName="Source Range Max"))
	double RangeMax;

	// Minimum value to apply to the destination (remapped from the input range)
	// 适用于目标的最小值（从输入范围重新映射）
	// If this is rotation, the unit is radian
	// 如果是旋转，单位是弧度
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(EditCondition=bUseRange, DisplayName="Mapped Range Min"))
	double RemappedMin;

	// Maximum value to apply to the destination (remapped from the input range)
	// 适用于目标的最大值（从输入范围重新映射）
	// If this is rotation, the unit is radian
	// 如果是旋转，单位是弧度
	UPROPERTY(EditAnywhere, Category = Mapping, meta = (EditCondition = bUseRange, DisplayName="Mapped Range Max"))
	double RemappedMax;

	/** Name of Morph Target to drive using the source attribute */
	/** 使用源属性驱动的变形目标的名称 */
	UPROPERTY(EditAnywhere, Category = "Destination (driven)")
	FName ParameterName;

	// Bone to drive using controller input
	// 使用控制器输入驱动骨骼
	UPROPERTY(EditAnywhere, Category="Destination (driven)")
	FBoneReference TargetBone;

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TEnumAsByte<EComponentType::Type> TargetComponent_DEPRECATED;
#endif

public:
	// Type of destination to drive, currently either bone or morph target
	// 要驱动的目的地类型，当前为骨骼或变形目标
	UPROPERTY(EditAnywhere, Category = "Destination (driven)")
	EDrivenDestinationMode DestinationMode;

	// The type of modification to make to the destination component(s)
	// 对目标组件进行的修改类型
	UPROPERTY(EditAnywhere, Category="Destination (driven)")
	EDrivenBoneModificationMode ModificationMode;

	// Transform component to use as input
	// 转换组件以用作输入
	UPROPERTY(EditAnywhere, Category="Source (driver)")
	TEnumAsByte<EComponentType::Type> SourceComponent;

	// Whether or not to clamp the driver value and remap it before scaling it
	// 是否在缩放驱动值之前限制并重新映射它
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(DisplayName="Remap Source"))
	uint8 bUseRange : 1;

	// Affect the X component of translation on the target bone
	// 影响目标骨骼上平移的 X 分量
	UPROPERTY(EditAnywhere, Category="Destination (driven)", meta=(DisplayName="X"))
	uint8 bAffectTargetTranslationX : 1;

	// Affect the Y component of translation on the target bone
	// 影响目标骨骼上平移的 Y 分量
	UPROPERTY(EditAnywhere, Category="Destination (driven)", meta=(DisplayName="Y"))
	uint8 bAffectTargetTranslationY : 1;

	// Affect the Z component of translation on the target bone
	// 影响目标骨骼上平移的 Z 分量
	UPROPERTY(EditAnywhere, Category="Destination (driven)", meta=(DisplayName="Z"))
	uint8 bAffectTargetTranslationZ : 1;

	// Affect the X component of rotation on the target bone
	// 影响目标骨骼上旋转的 X 分量
	UPROPERTY(EditAnywhere, Category="Destination (driven)", meta=(DisplayName="X"))
	uint8 bAffectTargetRotationX : 1;

	// Affect the Y component of rotation on the target bone
	// 影响目标骨骼上旋转的 Y 分量
	UPROPERTY(EditAnywhere, Category="Destination (driven)", meta=(DisplayName="Y"))
	uint8 bAffectTargetRotationY : 1;

	// Affect the Z component of rotation on the target bone
	// 影响目标骨骼上旋转的 Z 分量
	UPROPERTY(EditAnywhere, Category="Destination (driven)", meta=(DisplayName="Z"))
	uint8 bAffectTargetRotationZ : 1;

	// Affect the X component of scale on the target bone
	// 影响目标骨骼上缩放的 X 分量
	UPROPERTY(EditAnywhere, Category = "Destination (driven)", meta=(DisplayName="X"))
	uint8 bAffectTargetScaleX : 1;

	// Affect the Y component of scale on the target bone
	// 影响目标骨骼上缩放的 Y 分量
	UPROPERTY(EditAnywhere, Category = "Destination (driven)", meta=(DisplayName="Y"))
	uint8 bAffectTargetScaleY : 1;

	// Affect the Z component of scale on the target bone
	// 影响目标骨骼上缩放的 Z 分量
	UPROPERTY(EditAnywhere, Category="Destination (driven)", meta=(DisplayName="Z"))
	uint8 bAffectTargetScaleZ : 1;

public:
	ANIMGRAPHRUNTIME_API FAnimNode_BoneDrivenController();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	ANIMGRAPHRUNTIME_API virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context);
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

#if WITH_EDITORONLY_DATA
	// Upgrade a node from the output enum to the output bits (change made in FAnimationCustomVersion::BoneDrivenControllerMatchingMaya)
	// 将节点从输出枚举升级到输出位（在 FAnimationCustomVersion::BoneDrivenControllerMatchingMaya 中进行的更改）
	ANIMGRAPHRUNTIME_API void ConvertTargetComponentToBits();
#endif

protected:
	
	// FAnimNode_SkeletalControlBase protected interface
	// FAnimNode_SkeletalControlBase 受保护接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

	/** Extracts the value used to drive the target bone or parameter */
	/** 提取用于驱动目标骨骼或参数的值 */
	ANIMGRAPHRUNTIME_API const double ExtractSourceValue(const FTransform& InCurrentBoneTransform, const FTransform& InRefPoseBoneTransform);
	// End of FAnimNode_SkeletalControlBase protected interface
	// FAnimNode_SkeletalControlBase 受保护接口结束
};
