// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "Curves/CurveFloat.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "BoneControllers/AnimNode_AnimDynamics.h"
#include "AnimNode_Trail.generated.h"

class USkeletalMeshComponent;

// in the future, we might use this for stretch set up as well
// 将来，我们也可能用它来进行拉伸设置
// for now this is unserializable, and transient only
// 目前这是不可序列化的，并且只是暂时的
struct FPerJointTrailSetup
{
	/** How quickly we 'relax' the bones to their animated positions. */
	/** 我们将骨骼“放松”到其动画位置的速度有多快。 */
	float	TrailRelaxationSpeedPerSecond;
};

USTRUCT()
struct FRotationLimit
{
	GENERATED_BODY()

	FRotationLimit()
		: LimitMin(-180, -180, -180)
		, LimitMax(+180, +180, +180)
	{}

	UPROPERTY(EditAnywhere, Category = Angular, meta = (UIMin = "-180", UIMax = "180", ClampMin = "-180", ClampMax = "180"))
	FVector LimitMin;

	UPROPERTY(EditAnywhere, Category = Angular, meta = (UIMin = "-180", UIMax = "180", ClampMin = "-180", ClampMax = "180"))
	FVector LimitMax;
};

/**
 * Trail Controller
 */

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_Trail : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** LocalToWorld used last frame, used for building transform between frames. */
	/** LocalToWorld 使用最后一帧，用于构建帧之间的转换。 */
	FTransform		OldBaseTransform;

	/** Reference to the active bone in the hierarchy to modify. */
	/** 引用层次结构中要修改的活动骨骼。 */
	UPROPERTY(EditAnywhere, Category=Trail)
	FBoneReference TrailBone;

	/** Number of bones above the active one in the hierarchy to modify. ChainLength should be at least 2. */
	/** 层次结构中要修改的活动骨骼上方的骨骼数量。 ChainLength 应至少为 2。 */
	UPROPERTY(EditAnywhere, Category = Trail, meta = (ClampMin = "2", UIMin = "2"))
	int32	ChainLength;

	/** Axis of the bones to point along trail. */
	/** 骨骼的轴沿着轨迹指向。 */
	UPROPERTY(EditAnywhere, Category=Trail)
	TEnumAsByte<EAxis::Type>	ChainBoneAxis;

	/** Invert the direction specified in ChainBoneAxis. */
	/** 反转 ChainBoneAxis 中指定的方向。 */
	UPROPERTY(EditAnywhere, Category=Trail)
	uint8 bInvertChainBoneAxis:1;

	/** Limit the amount that a bone can stretch from its ref-pose length. */
	/** 限制骨骼从其参考姿势长度延伸的量。 */
	UPROPERTY(EditAnywhere, Category=Limit)
	uint8 bLimitStretch:1;

	/** Limit the amount that a bone can stretch from its ref-pose length. */
	/** 限制骨骼从其参考姿势长度延伸的量。 */
	UPROPERTY(EditAnywhere, Category = Limit)
	uint8 bLimitRotation:1;

	/** Whether to evaluate planar limits */
	/** 是否评估平面限制 */
	UPROPERTY(EditAnywhere, Category=Limit)
	uint8 bUsePlanarLimit:1;

	/** Whether 'fake' velocity should be applied in actor or world space. */
	/** 是否应在演员或世界空间中应用“假”速度。 */
	UPROPERTY(EditAnywhere,  Category=Velocity)
	uint8 bActorSpaceFakeVel:1;

	/** Fix up rotation to face child for the parent*/
	/** 修复父母面对孩子的旋转*/
	UPROPERTY(EditAnywhere, Category = Rotation)
	uint8 bReorientParentToChild:1;

	/** Did we have a non-zero ControlStrength last frame. */
	/** 最后一帧的 ControlStrength 是否非零？ */
	uint8 bHadValidStrength:1;

#if WITH_EDITORONLY_DATA
	/** Enable Debug in the PIE. This doesn't work in game*/
	/** 在 PIE 中启用调试。这在游戏中不起作用*/
	UPROPERTY(EditAnywhere, Category = Debug)
	uint8 bEnableDebug:1;

	/** Show Base Motion */
	/** 显示基本运动 */
	UPROPERTY(EditAnywhere, Category = Debug)
	uint8 bShowBaseMotion:1;

	/** Show Trail Location **/
	/** 显示路线位置 **/
	UPROPERTY(EditAnywhere, Category = Debug)
	uint8 bShowTrailLocation:1;

	/** Show Planar Limits **/
	/** 显示平面限制 **/
	UPROPERTY(EditAnywhere, Category = Debug)
	uint8 bShowLimit:1;

	/** This is used by selection node. Use this transient flag. */
	/** 这由选择节点使用。使用这个瞬态标志。 */
	uint8 bEditorDebugEnabled:1;

	/** Debug Life Time */
	/** 调试寿命 */
	UPROPERTY(EditAnywhere, Category = Debug)
	float DebugLifeTime;

	/** How quickly we 'relax' the bones to their animated positions. Deprecated. Replaced to TrailRelaxationCurve */
	/** 我们将骨骼“放松”到其动画位置的速度有多快。已弃用。替换为 TrailRelaxationCurve */
	UPROPERTY()
	float TrailRelaxation_DEPRECATED;
#endif // WITH_EDITORONLY_DATA

	/** To avoid hitches causing stretch of trail, you can use MaxDeltaTime to clamp the long delta time. If you want 30 fps to set it to 0.03333f ( = 1/30 ).  */
	/** 为了避免出现问题导致轨迹拉伸，您可以使用 MaxDeltaTime 来限制长增量时间。如果您想要 30 fps，请将其设置为 0.03333f (= 1/30)。  */
	UPROPERTY(EditAnywhere, Category = Limit)
	float MaxDeltaTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trail, meta = (PinHiddenByDefault))
	float RelaxationSpeedScale;

	/** How quickly we 'relax' the bones to their animated positions. Time 0 will map to top root joint, time 1 will map to the bottom joint. */
	/** 我们将骨骼“放松”到其动画位置的速度有多快。时间 0 将映射到顶部根关节，时间 1 将映射到底部关节。 */
	UPROPERTY(EditAnywhere, Category=Trail, meta=(CustomizeProperty))
	FRuntimeFloatCurve TrailRelaxationSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trail)
	FInputScaleBiasClamp RelaxationSpeedScaleInputProcessor;

	UPROPERTY(EditAnywhere, EditFixedSize, Category = Limit, meta = (EditCondition =bLimitRotation))
	TArray<FRotationLimit> RotationLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = Limit, meta = (PinHiddenByDefault, EditCondition = bLimitRotation))
	TArray<FVector> RotationOffsets;

	/** List of available planar limits for this node */
	/** 该节点的可用平面限制列表 */
	UPROPERTY(EditAnywhere, Category=Limit, meta = (EditCondition = bUsePlanarLimit))
	TArray<FAnimPhysPlanarLimit> PlanarLimits;

	/** If bLimitStretch is true, this indicates how long a bone can stretch beyond its length in the ref-pose. */
	/** 如果 bLimitStretch 为 true，则表示骨骼在参考姿势中可以拉伸超出其长度的长度。 */
	UPROPERTY(EditAnywhere, Category=Limit)
	float	StretchLimit;

	/** 'Fake' velocity applied to bones. */
	/** 应用于骨骼的“假”速度。 */
	UPROPERTY(EditAnywhere, Category=Velocity, meta = (PinHiddenByDefault))
	FVector	FakeVelocity;

	/** Base Joint to calculate velocity from. If none, it will use Component's World Transform. . */
	/** 用于计算速度的基础关节。如果没有，它将使用组件的世界变换。 。 */
	UPROPERTY(EditAnywhere, Category=Velocity)
	FBoneReference BaseJoint;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	float TrailBoneRotationBlendAlpha_DEPRECATED;
#endif // WITH_EDITORONLY_DATA
	/* How to set last bone rotation. It copies from previous joint if alpha is 0.f, or 1.f will use animated pose 
	 * This alpha dictates the blend between parent joint and animated pose, and how much you'd like to use animated pose for
	 */
	UPROPERTY(EditAnywhere, Category = Rotation, meta = (EditCondition = bReorientParentToChild))
	float LastBoneRotationAnimAlphaBlend;

	/** Internal use - we need the timestep to do the relaxation in CalculateNewBoneTransforms. */
	/** 内部使用 - 我们需要时间步来在CalculateNewBoneTransforms 中进行松弛。 */
	float	ThisTimstep;

	/** Component-space locations of the bones from last frame. Each frame these are moved towards their 'animated' locations. */
	/** 上一帧中骨骼的组件空间位置。这些每一帧都会移向它们的“动画”位置。 */
	TArray<FVector>	TrailBoneLocations;

	/** Per Joint Trail Set up*/
	/** 每个关节轨迹设置*/
	TArray<FPerJointTrailSetup> PerJointTrailData;

#if WITH_EDITORONLY_DATA
	/** debug transient data to draw debug better */
	/** 调试瞬态数据以更好地进行调试 */
	TArray<FColor>	TrailDebugColors;
	TArray<FColor>	PlaneDebugColors;
#endif // WITH_EDITORONLY_DATA

	ANIMGRAPHRUNTIME_API FAnimNode_Trail();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	ANIMGRAPHRUNTIME_API void PostLoad();

#if WITH_EDITOR
	ANIMGRAPHRUNTIME_API void EnsureChainSize();
#endif // WITH_EDITOR
private:
	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	FVector GetAlignVector(EAxis::Type AxisOption, bool bInvert);

	// skeleton index
	// 骨架指数
	TArray<int32> ChainBoneIndices;
};
