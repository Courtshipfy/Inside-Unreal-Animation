// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneIndices.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "CommonAnimTypes.h"
#include "EngineDefines.h"
#include "AnimNode_LookAt.generated.h"

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;

UENUM()
/** Various ways to interpolate TAlphaBlend. */
/** 插值 TAlphaBlend 的各种方法。 */
namespace EInterpolationBlend
{
	enum Type : int
	{
		Linear,
		Cubic,
		Sinusoidal,
		EaseInOutExponent2,
		EaseInOutExponent3,
		EaseInOutExponent4,
		EaseInOutExponent5,
		MAX
	};
}

/**
 *	Simple controller that make a bone to look at the point or another bone
 */
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_LookAt : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to control. This is the main bone chain to modify from. **/
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 **/
	UPROPERTY(EditAnywhere, Category=SkeletalControl) 
	FBoneReference BoneToModify;

	/** Target socket to look at. Used if LookAtBone is empty. - You can use  LookAtLocation if you need offset from this point. That location will be used in their local space. **/
	/** 要查看的目标套接字。如果 LookAtBone 为空，则使用。 - 如果您需要从该点偏移，您可以使用 LookAtLocation。该位置将在其本地空间中使用。 **/
	UPROPERTY(EditAnywhere, Category = Target)
	FBoneSocketTarget LookAtTarget;

	/** Target Offset. It's in world space if LookAtBone is empty or it is based on LookAtBone or LookAtSocket in their local space*/
	/** 目标偏移。如果 LookAtBone 为空或者它基于本地空间中的 LookAtBone 或 LookAtSocket，则它位于世界空间中*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Target, meta = (PinHiddenByDefault))
	FVector LookAtLocation;

	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FAxis LookAt_Axis;

	/** Whether or not to use Look up axis */
	/** 是否使用查找轴 */
	UPROPERTY(EditAnywhere, Category=SkeletalControl)
	bool bUseLookUpAxis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkeletalControl, meta=(PinHiddenByDefault))
	TEnumAsByte<EInterpolationBlend::Type>	InterpolationType;

	UPROPERTY(EditAnywhere, Category = SkeletalControl)
	FAxis LookUp_Axis;

	/** Look at Clamp value in degrees - if your look at axis is Z, only X, Y degree of clamp will be used */
	/** 查看以度为单位的夹紧值 - 如果您的观察轴是 Z，则仅使用夹紧的 X、Y 度数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkeletalControl, meta=(PinHiddenByDefault))
	float LookAtClamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkeletalControl, meta=(PinHiddenByDefault))
	float	InterpolationTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkeletalControl, meta=(PinHiddenByDefault))
	float	InterpolationTriggerThreashold;

#if WITH_EDITORONLY_DATA
	/** Target Bone to look at - You can use  LookAtLocation if you need offset from this point. That location will be used in their local space. **/
	/** 要查看的目标骨骼 - 如果需要从该点偏移，可以使用 LookAtLocation。该位置将在其本地空间中使用。 **/
	UPROPERTY()
	FBoneReference LookAtBone_DEPRECATED;

	UPROPERTY()
	FName LookAtSocket_DEPRECATED;

	/** Look at axis, which axis to align to look at point */
	/** 看轴，看点要对齐哪个轴 */
	UPROPERTY() 
	TEnumAsByte<EAxisOption::Type>	LookAtAxis_DEPRECATED;

	/** Custom look up axis in local space. Only used if LookAtAxis==EAxisOption::Custom */
	/** 本地空间中的自定义查找轴。仅当 LookAtAxis==EAxisOption::Custom 时使用 */
	UPROPERTY()
	FVector	CustomLookAtAxis_DEPRECATED;

	/** Look up axis in local space */
	/** 在局部空间中查找轴 */
	UPROPERTY()
	TEnumAsByte<EAxisOption::Type>	LookUpAxis_DEPRECATED;

	/** Custom look up axis in local space. Only used if LookUpAxis==EAxisOption::Custom */
	/** 本地空间中的自定义查找轴。仅当 LookUpAxis==EAxisOption::Custom 时使用 */
	UPROPERTY()
	FVector	CustomLookUpAxis_DEPRECATED;
#endif

	// in the future, it would be nice to have more options, -i.e. lag, interpolation speed
	// 将来，如果有更多选择就好了，-i.e.滞后、插补速度
	ANIMGRAPHRUNTIME_API FAnimNode_LookAt();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	FVector GetCachedTargetLocation() const {	return 	CachedCurrentTargetLocation;	}

#if WITH_EDITOR
	ANIMGRAPHRUNTIME_API void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp) const;
#endif // WITH_EDITOR

private:
	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	/** Turn a linear interpolated alpha into the corresponding AlphaBlendType */
	/** 将线性插值 alpha 转换为相应的 AlphaBlendType */
	static ANIMGRAPHRUNTIME_API float AlphaToBlendType(float InAlpha, EInterpolationBlend::Type BlendType);

	/** Debug transient data */
	/** 调试瞬态数据 */
	FVector CurrentLookAtLocation;

	/** Current Target Location */
	/** 当前目标位置 */
	FVector CurrentTargetLocation;
	FVector PreviousTargetLocation;

	/** Current Alpha */
	/** 当前阿尔法 */
	float AccumulatedInterpoolationTime;


#if UE_ENABLE_DEBUG_DRAWING
	/** Debug draw cached data */
	/** 调试绘制缓存数据 */
	FTransform CachedOriginalTransform;
	FTransform CachedLookAtTransform;
	FTransform CachedTargetCoordinate;
	FVector CachedPreviousTargetLocation;
	FVector CachedCurrentLookAtLocation;
#endif // UE_ENABLE_DEBUG_DRAWING
	FVector CachedCurrentTargetLocation;

protected:
	ANIMGRAPHRUNTIME_API virtual void ModifyPoseFromDeltaRotation(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms,	FTransform& InOutBoneToModifyTransform,	const FQuat& DeltaRotation);
};
