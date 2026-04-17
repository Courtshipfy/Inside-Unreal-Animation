// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_SpringBone.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;

/**
 *	Simple controller that replaces or adds to the translation/rotation of a single bone.
 */

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_SpringBone : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to control. This is the main bone chain to modify from. **/
	/** 要控制的骨骼的名称。这是要修改的主骨骼链。 **/
	UPROPERTY(EditAnywhere, Category=Spring) 
	FBoneReference SpringBone;

	/** If bLimitDisplacement is true, this indicates how long a bone can stretch beyond its length in the ref-pose. */
	/** 如果 bLimitDisplacement 为 true，则表示骨骼在参考姿势中可以拉伸超出其长度的长度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Spring, meta=(EditCondition="bLimitDisplacement"))
	double MaxDisplacement;

	/** Stiffness of spring */
	/** 弹簧刚度 */
	UPROPERTY(EditAnywhere, Category=Spring, meta = (PinHiddenByDefault))
	double SpringStiffness;

	/** Damping of spring */
	/** 弹簧阻尼 */
	UPROPERTY(EditAnywhere, Category=Spring, meta = (PinHiddenByDefault))
	double SpringDamping;

	/** If spring stretches more than this, reset it. Useful for catching teleports etc */
	/** 如果弹簧拉伸超过此值，请将其重置。对于捕捉传送等很有用 */
	UPROPERTY(EditAnywhere, Category=Spring)
	double ErrorResetThresh;

	/** World-space location of the bone. */
	/** 骨骼的世界空间位置。 */
	FVector BoneLocation;

	/** World-space velocity of the bone. */
	/** 骨骼的世界空间速度。 */
	FVector BoneVelocity;

	/** Velocity of the owning actor */
	/** 所属演员的速度 */
	FVector OwnerVelocity;

	/** Cache of previous frames local bone transform so that when
	 *  we cannot simulate (timestep == 0) we can still update bone location */
	FVector LocalBoneTransform;

	/** Internal use - Amount of time we need to simulate. */
	/** 内部使用 - 我们需要模拟的时间量。 */
	float RemainingTime;

	/** Internal use - Current timestep */
	/** 内部使用 - 当前时间步长 */
	float FixedTimeStep;

	/** Internal use - Current time dilation */
	/** 内部使用-当前时间膨胀 */
	float TimeDilation;

#if WITH_EDITORONLY_DATA
	/** If true, Z position is always correct, no spring applied */
	/** 如果为 true，则 Z 位置始终正确，不应用弹簧 */
	UPROPERTY()
	bool bNoZSpring_DEPRECATED;
#endif

	/** Limit the amount that a bone can stretch from its ref-pose length. */
	/** 限制骨骼从其参考姿势长度延伸的量。 */
	UPROPERTY(EditAnywhere, Category=Spring)
	uint8 bLimitDisplacement : 1;

	/** If true take the spring calculation for translation in X */
	/** 如果为 true，则采用弹簧计算进行 X 方向的平移 */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bTranslateX : 1;

	/** If true take the spring calculation for translation in Y */
	/** 如果为 true，则采用弹簧计算进行 Y 方向的平移 */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bTranslateY : 1;

	/** If true take the spring calculation for translation in Z */
	/** 如果为 true，则采用弹簧计算进行 Z 轴平移 */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bTranslateZ : 1;

	/** If true take the spring calculation for rotation in X */
	/** 如果为 true，则采用弹簧计算 X 方向的旋转 */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bRotateX : 1;

	/** If true take the spring calculation for rotation in Y */
	/** 如果为 true，则采用 Y 轴旋转的弹簧计算 */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bRotateY : 1;

	/** If true take the spring calculation for rotation in Z */
	/** 如果为 true，则采用弹簧计算 Z 轴旋转 */
	UPROPERTY(EditAnywhere, Category=FilterChannels)
	uint8 bRotateZ : 1;

	/** Did we have a non-zero ControlStrength last frame. */
	/** 最后一帧的 ControlStrength 是否非零？ */
	uint8 bHadValidStrength : 1;

	/** Option to override owner velocity used by spring controller. */
	/** 覆盖弹簧控制器使用的所有者速度的选项。 */
	UPROPERTY(EditAnywhere, Category = OwnerVelocity, Meta = (PinHiddenByDefault))
	uint8 bOverrideOwnerVelocity : 1;

	UPROPERTY(EditAnywhere, Category = OwnerVelocity, Meta = (EditCondition = "bOverrideOwnerVelocity", PinHiddenByDefault))
	FVector OwnerVelocityOverride;

public:
	ANIMGRAPHRUNTIME_API FAnimNode_SpringBone();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual bool HasPreUpdate() const override { return true; }
	ANIMGRAPHRUNTIME_API virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
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
