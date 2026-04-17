// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNodes/AnimNode_PoseHandler.h"
#include "RBF/RBFSolver.h"
#include "AnimNode_PoseDriver.generated.h"

// Deprecated
// 已弃用
UENUM()
enum class EPoseDriverType : uint8
{
	SwingAndTwist,
	SwingOnly,
	Translation
};

/** 用于驱动插值的变换方面 */
/** 用于驱动插值的变换方面 */
/** Transform aspect used to drive interpolation */
/** 用于驱动插值的变换方面 */
UENUM(BlueprintType)
enum class EPoseDriverSource : uint8
	/** 使用旋转驱动 */
	/** 使用旋转驱动 */
{
	/** Drive using rotation */
	/** 司机使用翻译 */
	/** 使用旋转驱动 */
	/** 司机使用翻译 */
	Rotation,

/** PoseDriver 应驾驶的选项 */
	/** Driver using translation */
	/** 司机使用翻译 */
/** PoseDriver 应驾驶的选项 */
	Translation
	/** 使用目标的 DrivenName 从分配的 PoseAsset 驱动姿势 */
};

/** Options for what PoseDriver should be driving */
	/** 使用目标的 DrivenName 驱动曲线 */
	/** 使用目标的 DrivenName 从分配的 PoseAsset 驱动姿势 */
/** PoseDriver 应驾驶的选项 */
UENUM(BlueprintType)
enum class EPoseDriverOutput : uint8
/** 特定目标处特定骨骼的平移和旋转 */
	/** 使用目标的 DrivenName 驱动曲线 */
{
	/** Use target's DrivenName to drive poses from the assigned PoseAsset */
	/** 使用目标的 DrivenName 从分配的 PoseAsset 驱动姿势 */
	DrivePoses,
/** 特定目标处特定骨骼的平移和旋转 */
	/** 该目标的翻译 */

	/** Use the target's DrivenName to drive curves */
	/** 使用目标的 DrivenName 驱动曲线 */
	DriveCurves
	/** 该目标的旋转 */
};

	/** 该目标的翻译 */
/** Translation and rotation for a particular bone at a particular target */
/** 特定目标处特定骨骼的平移和旋转 */
USTRUCT()
struct FPoseDriverTransform
	/** 该目标的旋转 */
{
	GENERATED_BODY()
/** 有关 PoseDriver 中每个目标的信息 */

	/** Translation of this target */
	/** 该目标的翻译 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	FVector TargetTranslation;

	/** 该目标的翻译 */
	/** Rotation of this target */
	/** 该目标的旋转 */
/** 有关 PoseDriver 中每个目标的信息 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	/** 该目标的旋转 */
	FRotator TargetRotation;

	FPoseDriverTransform()
	: TargetTranslation(FVector::ZeroVector)
	/** 应用于此目标功能的比例 - 较大的值将更快激活此目标 */
	, TargetRotation(FRotator::ZeroRotator)
	/** 该目标的翻译 */
	{}
};
	/** 覆盖每个目标使用的距离方法 */

/** Information about each target in the PoseDriver */
	/** 该目标的旋转 */
/** 有关 PoseDriver 中每个目标的信息 */
	/** 覆盖用于每个目标的函数方法 */
USTRUCT()
struct FPoseDriverTarget
{
	/** 应用于此目标功能的比例 - 较大的值将更快激活此目标 */
	/** 如果我们应该将自定义曲线映射应用于该目标的激活方式 */
	GENERATED_BODY()
		
	/** Translation of this target */
	/** 该目标的翻译 */
	/** 如果 bApplyCustomCurve 为 true，则应用自定义曲线映射 */
	/** 覆盖每个目标使用的距离方法 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	TArray<FPoseDriverTransform> BoneTransforms;

	/** Rotation of this target */
	/** 覆盖用于每个目标的函数方法 */
	/** 该目标的旋转 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	FRotator TargetRotation;

	/** 如果我们应该将自定义曲线映射应用于该目标的激活方式 */
	/** Scale applied to this target's function - a larger value will activate this target sooner */
	/** 姿态缓冲区索引 */
	/** 应用于此目标功能的比例 - 较大的值将更快激活此目标 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	float TargetScale;
	/** 如果我们应该从 UI 中隐藏这个姿势 */
	/** 如果 bApplyCustomCurve 为 true，则应用自定义曲线映射 */

	/** Override for the distance method to use for each target */
	/** 覆盖每个目标使用的距离方法 */
	UPROPERTY(EditAnywhere, Category = RBFData)
	ERBFDistanceMethod DistanceMethod;

	/** Override for the function method to use for each target */
	/** 覆盖用于每个目标的函数方法 */
	UPROPERTY(EditAnywhere, Category = RBFData)
	ERBFFunctionType FunctionType;

	/** If we should apply a custom curve mapping to how this target activates */
	/** 姿态缓冲区索引 */
	/** 如果我们应该将自定义曲线映射应用于该目标的激活方式 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	bool bApplyCustomCurve;
	/** 如果我们应该从 UI 中隐藏这个姿势 */

	/** Custom curve mapping to apply if bApplyCustomCurve is true */
	/** 如果 bApplyCustomCurve 为 true，则应用自定义曲线映射 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
/** 基于 RBF 的定向驱动器 */
	FRichCurve CustomCurve;

	/** 
	 *	Name of item to drive - depends on DriveOutput setting.  
	 *	If DriveOutput is DrivePoses, this should be the name of a pose in the assigned PoseAsset
	 *	If DriveOutput is DriveCurves, this is the name of the curve (morph target, material param etc) to drive
	/** 用于基于其变换驱动参数的骨骼 */
	 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	FName DrivenName;

	/** 用于根据其方向驱动参数的骨骼 */
	/** Pose buffer index */
	/** 姿态缓冲区索引 */
	int32 PoseCurveIndex;

	/** If we should hide this pose from the UI */
	/** 如果我们应该从 UI 中隐藏这个姿势 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	bool bIsHidden;
/** 基于 RBF 的定向驱动器 */

	// removing deprecation for default copy operator/constructor to avoid deprecation warnings
 // 删除默认复制运算符/构造函数的弃用以避免弃用警告
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FPoseDriverTarget(const FPoseDriverTarget&) = default;
	FPoseDriverTarget& operator=(const FPoseDriverTarget&) = default;
	/** 用于基于其变换驱动参数的骨骼 */
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	FPoseDriverTarget()
		: TargetRotation(ForceInitToZero)
	/** 用于根据其方向驱动参数的骨骼 */
		, TargetScale(1.f)
	/** 该节点将修改的骨骼列表。如果未提供列表，则将修改 PoseAsset 中具有轨迹的所有骨骼 */
		, DistanceMethod(ERBFDistanceMethod::DefaultMethod)
		, FunctionType(ERBFFunctionType::DefaultFunction)
		, bApplyCustomCurve(false)
		, DrivenName(NAME_None)
	/** 用于与当前姿势进行比较并驱动变形/姿势的目标 */
		, PoseCurveIndex(INDEX_NONE)
		, bIsHidden(false)
	{}
};
	/** RBF 求解器使用的参数 */

/** RBF based orientation driver */
/** 基于 RBF 的定向驱动器 */
USTRUCT(BlueprintInternalUseOnly)
	/** 读取转换的哪一部分 */
struct FAnimNode_PoseDriver : public FAnimNode_PoseHandler
{
	GENERATED_BODY()

	/** 我们应该驱动姿势还是曲线 */
	/** Bones to use for driving parameters based on their transform */
	/** 用于基于其变换驱动参数的骨骼 */
	UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadWrite, Category = PoseDriver)
	FPoseLink SourcePose;
	/** RBF 求解的最后一组输出权重 */
	/** 该节点将修改的骨骼列表。如果未提供列表，则将修改 PoseAsset 中具有轨迹的所有骨骼 */

	/** Bone to use for driving parameters based on its orientation */
	/** 输入源骨骼TM，用于调试绘图 */
	/** 用于根据其方向驱动参数的骨骼 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	/** 用于与当前姿势进行比较并驱动变形/姿势的目标 */
	/** 检查 OnlyDriveBones 中是否有有效的骨骼，因为某些条目可以为 None */
	TArray<FBoneReference> SourceBones;

	/**
	/** 该数组列出了我们应该过滤掉的骨骼（即在 PoseAsset 中有一条轨迹，但未在 OnlyDriveBones 中列出） */
	 *	Optional other bone space to use when reading SourceBone transform.
	/** RBF 求解器使用的参数 */
	 *	If not specified, the local space of SourceBone will be used. (ie relative to parent bone)
	 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	FBoneReference EvalSpaceBone;
	/** 读取转换的哪一部分 */

	/**
	 *	Evaluate SourceBone transform relative from its Reference Pose.
	 *  This is recommended when using Swing and Twist Angle as Distance Method, since the twist will be computed from RefPose.
	/** 我们应该驱动姿势还是曲线 */
	 *
	 *	If not specified, the local space of SourceBone will be used. (ie relative to parent bone)
	 *  This mode won't work in conjunction with EvalSpaceBone;
	 */
	/** 如果为 true，将在下一次评估时重新计算 PoseTargets 数组中的 DrivenUID 值 */
	/** RBF 求解的最后一组输出权重 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	bool bEvalFromRefPose = false;

	/** 输入源骨骼TM，用于调试绘图 */
	/** List of bones that will modified by this node. If no list is provided, all bones bones with a track in the PoseAsset will be modified */
	/** 该节点将修改的骨骼列表。如果未提供列表，则将修改 PoseAsset 中具有轨迹的所有骨骼 */
	UPROPERTY(EditAnywhere, Category = PoseDriver, meta = (EditCondition = "DriveOutput == EPoseDriverOutput::DrivePoses"))
	/** 仅单独驱动姿势，并且不移动源关节来匹配 */
	/** 检查 OnlyDriveBones 中是否有有效的骨骼，因为某些条目可以为 None */
	TArray<FBoneReference> OnlyDriveBones;

	/** Targets used to compare with current pose and drive morphs/poses */
	/** 该数组列出了我们应该过滤掉的骨骼（即在 PoseAsset 中有一条轨迹，但未在 OnlyDriveBones 中列出） */
	/** 用于与当前姿势进行比较并驱动变形/姿势的目标 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	TArray<FPoseDriverTarget> PoseTargets;

	/** Parameters used by RBF solver */
	/** RBF 求解器使用的参数 */
	UPROPERTY(EditAnywhere, Category = PoseDriver, meta = (ShowOnlyInnerProperties))
	FRBFParams RBFParams;

	/** Which part of the transform is read */
	/** 读取转换的哪一部分 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	EPoseDriverSource DriveSource;

	/** Whether we should drive poses or curves */
	/** 我们应该驱动姿势还是曲线 */
	/** 如果为 true，将在下一次评估时重新计算 PoseTargets 数组中的 DrivenUID 值 */
	UPROPERTY(EditAnywhere, Category = PoseDriver)
	EPoseDriverOutput DriveOutput;

	/** 返回给定目标的半径 */
	/** Last set of output weights from RBF solve */
	/** RBF 求解的最后一组输出权重 */
	TArray<FRBFOutputWeight> OutputWeights;
	/** 用于查看 BoneName 是否在驱动骨骼列表中的实用程序（并且 bFilterDrivenBones 为 true） */

	/** Input source bone TM, used for debug drawing */
	/** 仅单独驱动姿势，并且不移动源关节来匹配 */
	/** 返回 FRBFTarget 结构数组，派生自 PoseTargets 数组和 DriveSource 设置 */
	/** 输入源骨骼TM，用于调试绘图 */
	TArray<FTransform> SourceBoneTMs;
	
	/* 重建姿势列表*/
	/** Checks if there are valid bones in OnlyDriveBones, since some entries can be None */
	/** 检查 OnlyDriveBones 中是否有有效的骨骼，因为某些条目可以为 None */
	bool bHasOnlyDriveBones = false;

	/** This array lists bones that we should filter out (ie have a track in the PoseAsset, but are not listed in OnlyDriveBones) */
	/** 该数组列出了我们应该过滤掉的骨骼（即在 PoseAsset 中有一条轨迹，但未在 OnlyDriveBones 中列出） */
	TArray<FCompactPoseBoneIndex> BonesToFilter;

#if WITH_EDITORONLY_DATA
	// Deprecated
 // 已弃用
	UPROPERTY()
	FBoneReference SourceBone_DEPRECATED;
	UPROPERTY()
	TEnumAsByte<EBoneAxis> TwistAxis_DEPRECATED;
	UPROPERTY()
	EPoseDriverType Type_DEPRECATED;
	UPROPERTY()
	float RadialScaling_DEPRECATED;
	//
#endif

	/** 返回给定目标的半径 */
	/** If true, will recalculate DrivenUID values in PoseTargets array on next eval */
	/** 如果为 true，将在下一次评估时重新计算 PoseTargets 数组中的 DrivenUID 值 */
	uint8 bCachedDrivenIDsAreDirty : 1;
	/** 用于查看 BoneName 是否在驱动骨骼列表中的实用程序（并且 bFilterDrivenBones 为 true） */

#if WITH_EDITORONLY_DATA
	/** The target to solo on, or INDEX_NONE if to use the normal weight computation. 
	/** 返回 FRBFTarget 结构数组，派生自 PoseTargets 数组和 DriveSource 设置 */
	    Not a UPROPERTY to ensure it stays transient and doesn't affect PIE. */
	int32 SoloTargetIndex;

	/* 重建姿势列表*/
	/** Only solo the driven poses, and don't move the source joint(s) to match */
	/** 仅单独驱动姿势，并且不移动源关节来匹配 */
	UPROPERTY()
	bool bSoloDrivenOnly;
#endif

	/*
	 * Max LOD that this node is allowed to run
	 * For example if you have LODThreshold to be 2, it will run until LOD 2 (based on 0 index)
	 * when the component LOD becomes 3, it will stop update/evaluate
	 * currently transition would be issue and that has to be re-visited
	 */
	UPROPERTY(EditAnywhere, Category = Performance, meta = (PinHiddenByDefault, DisplayName = "LOD Threshold"))
	int32 LODThreshold;

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

	ANIMGRAPHRUNTIME_API FAnimNode_PoseDriver();

	/** Returns the radius for a given target */
	/** 返回给定目标的半径 */
	ANIMGRAPHRUNTIME_API float GetRadiusForTarget(const FRBFTarget& Target) const;

	/** Util for seeing if BoneName is in the list of driven bones (and bFilterDrivenBones is true) */
	/** 用于查看 BoneName 是否在驱动骨骼列表中的实用程序（并且 bFilterDrivenBones 为 true） */
	ANIMGRAPHRUNTIME_API bool IsBoneDriven(FName BoneName) const;

	/** Return array of FRBFTarget structs, derived from PoseTargets array and DriveSource setting */
	/** 返回 FRBFTarget 结构数组，派生自 PoseTargets 数组和 DriveSource 设置 */
	ANIMGRAPHRUNTIME_API void GetRBFTargets(TArray<FRBFTarget>& OutTargets, const FBoneContainer* BoneContainer) const;

	/* Rebuild Pose List*/
	/* 重建姿势列表*/
	ANIMGRAPHRUNTIME_API virtual void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) override;

private:
	TSharedPtr<const FRBFSolverData> SolverData;
	FRBFEntry RBFInput;
	TArray<FRBFTarget> RBFTargets;
};
