// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Components/SplineComponent.h"
#include "AlphaBlend.h"
#include "AnimNode_SplineIK.generated.h"

/** 
 * The different axes we can align our bones to.
 * Note that the values are set to match up with EAxis (but without 'None')
 */
UENUM()
enum class ESplineBoneAxis : uint8
{
	None = 0 UMETA(Hidden),
	X = 1,
	Y = 2,
	Z = 3,
};

/** Data cached per bone in the chain */
/** 链中每个骨骼缓存的数据 */
/** 链中每个骨骼缓存的数据 */
/** 链中每个骨骼缓存的数据 */
USTRUCT()
struct FSplineIKCachedBoneData
{
	GENERATED_BODY()

	FSplineIKCachedBoneData()
		: Bone(NAME_None)
		, RefSkeletonIndex(INDEX_NONE)
	{}

	FSplineIKCachedBoneData(const FName& InBoneName, int32 InRefSkeletonIndex)
		: Bone(InBoneName)
		, RefSkeletonIndex(InRefSkeletonIndex)
	{}
	/** 我们所说的骨头 */

	/** 我们所说的骨头 */
	/** The bone we refer to */
	/** 我们所说的骨头 */
	/** 参考骨骼中骨骼的索引 */
	UPROPERTY()
	FBoneReference Bone;
	/** 参考骨骼中骨骼的索引 */

	/** Index of the bone in the reference skeleton */
	/** 参考骨骼中骨骼的索引 */
	UPROPERTY()
	int32 RefSkeletonIndex;
};

	/** 样条线延伸的根骨骼的名称 **/
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_SplineIK : public FAnimNode_SkeletalControlBase
{
	/** 样条线延伸的根骨骼的名称 **/
	/** 样条链末端骨骼的名称。此后的骨骼将不会被控制器更改。 */
	GENERATED_BODY()

	/** Name of root bone from which the spline extends **/
	/** 样条线延伸的根骨骼的名称 **/
	/** 用作曲线方向的受控骨骼的轴（即样条线的方向）。 */
	/** 样条链末端骨骼的名称。此后的骨骼将不会被控制器更改。 */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	FBoneReference StartBone;

	/** 如果我们直接指定样条曲线中的点数 */
	/** Name of bone at the end of the spline chain. Bones after this will not be altered by the controller. */
	/** 用作曲线方向的受控骨骼的轴（即样条线的方向）。 */
	/** 样条链末端骨骼的名称。此后的骨骼将不会被控制器更改。 */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	/** 如果我们不自动计算，则样条曲线中的点数 */
	FBoneReference EndBone;

	/** 如果我们直接指定样条曲线中的点数 */
	/** Axis of the controlled bone (ie the direction of the spline) to use as the direction for the curve. */
	/** 应用于样条点的变换 **/
	/** 用作曲线方向的受控骨骼的轴（即样条线的方向）。 */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	ESplineBoneAxis BoneAxis;
	/** 如果我们不自动计算，则样条曲线中的点数 */
	/** 样条线的整体滚动，应用于沿样条线方向的其他旋转之上 */

	/** The number of points in the spline if we are specifying it directly */
	/** 如果我们直接指定样条曲线中的点数 */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	/** 起始骨骼的扭曲。扭曲根据扭曲混合沿着样条线插值。 */
	/** 应用于样条点的变换 **/
	bool bAutoCalculateSpline;

	/** The number of points in the spline if we are not auto-calculating */
	/** 末端骨头的扭曲。扭曲根据扭曲混合沿着样条线插值。 */
	/** 如果我们不自动计算，则样条曲线中的点数 */
	/** 样条线的整体滚动，应用于沿样条线方向的其他旋转之上 */
	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (ClampMin = 2, UIMin = 2, EditCondition = "!bAutoCalculateSpline"))
	int32 PointCount;
	/** 如何沿样条线长度插入扭曲 */

	/** Transforms applied to spline points **/
	/** 起始骨骼的扭曲。扭曲根据扭曲混合沿着样条线插值。 */
	/** 应用于样条点的变换 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = "Parameters", meta = (BlueprintCompilerGeneratedDefaults, PinHiddenByDefault))
	TArray<FTransform> ControlPoints;

	/** 末端骨头的扭曲。扭曲根据扭曲混合沿着样条线插值。 */
	/** Overall roll of the spline, applied on top of other rotations along the direction of the spline */
	/** 样条线的整体滚动，应用于沿样条线方向的其他旋转之上 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (PinHiddenByDefault))
	/** 沿样条线距骨骼受约束的起点的距离 */
	float Roll;
	/** 如何沿样条线长度插入扭曲 */

	/** The twist of the start bone. Twist is interpolated along the spline according to Twist Blend. */
	/** 起始骨骼的扭曲。扭曲根据扭曲混合沿着样条线插值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (PinHiddenByDefault))
	float TwistStart;

	/** The twist of the end bone. Twist is interpolated along the spline according to Twist Blend. */
	/** 末端骨头的扭曲。扭曲根据扭曲混合沿着样条线插值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (PinHiddenByDefault))
	float TwistEnd;

	/** 沿样条线距骨骼受约束的起点的距离 */
	/** How to interpolate twist along the length of the spline */
	/** 如何沿样条线长度插入扭曲 */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	/** 对样条曲线的只读访问 */
	FAlphaBlend TwistBlend;

	/**
	/** 对变换曲线的只读访问 */
	 * The maximum stretch allowed when fitting bones to the spline. 0.0 means bones do not stretch their length,
	 * 1.0 means bones stretch to the length of the spline
	 */
	/** 获取样条线的变换样条点（在组件空间中） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (PinHiddenByDefault))
	float Stretch;

	/** 获取样条线的指定句柄变换（在组件空间中） */
	/** The distance along the spline from the start from which bones are constrained */
	/** 沿样条线距骨骼受约束的起点的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (PinHiddenByDefault))
	/** 为样条线设置指定的手柄变换（在组件空间中） */
	float Offset;

	/** 对样条曲线的只读访问 */
	/** 为样条线设置指定的手柄位置（在零部件空间中） */
	ANIMGRAPHRUNTIME_API FAnimNode_SplineIK();

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	/** 为样条线设置指定的控制柄旋转（在零部件空间中） */
	/** 对变换曲线的只读访问 */
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	ANIMGRAPHRUNTIME_API virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	/** 为样条线设置指定的控制柄比例（在组件空间中） */
	/** 获取样条线的变换样条点（在组件空间中） */
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束
	/** 获取我们正在使用的样条变换的数量 */
	/** 获取样条线的指定句柄变换（在组件空间中） */

	// FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口
	/** 构建骨骼参考并从提供的参考骨架重新分配变换 */
	/** 为样条线设置指定的手柄变换（在组件空间中） */
	ANIMGRAPHRUNTIME_API virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口结束
	/** 从参考姿势构建样条线 */
	/** 为样条线设置指定的手柄位置（在零部件空间中） */

	/** Read-only access to spline curves */
	/** 为样条线设置指定的控制柄旋转（在零部件空间中） */
	/** 使用我们的控制点变换样条线 */
	/** 对样条曲线的只读访问 */
	const FSplineCurves& GetSplineCurves() const { return BoneSpline; }

	/** 使用我们的线性近似来确定与球体的最早相交 */
	/** 为样条线设置指定的控制柄比例（在组件空间中） */
	/** Read-only access to transformed curves */
	/** 对变换曲线的只读访问 */
	const FSplineCurves& GetTransformedSplineCurves() const { return TransformedSpline; }
	/** 获取我们正在使用的样条变换的数量 */

	/** Get transformed spline point (in component space) for the spline */
	/** 获取样条线的变换样条点（在组件空间中） */
	/** 获取指定样条 alpha 处的当前扭曲值 */
	/** 构建骨骼参考并从提供的参考骨架重新分配变换 */
	ANIMGRAPHRUNTIME_API FTransform GetTransformedSplinePoint(int32 TransformIndex) const;

	/** 变换样条线 */
	/** Get specified handle transform (in component space) for the spline */
	/** 获取样条线的指定句柄变换（在组件空间中） */
	/** 从参考姿势构建样条线 */
	/** 样条曲线的分段线性近似，在创建和变形时重新计算 */
	ANIMGRAPHRUNTIME_API FTransform GetControlPoint(int32 TransformIndex) const;

	/** Set specified handle transform (in component space) for the spline */
	/** 我们内部维护的样条线 */
	/** 为样条线设置指定的手柄变换（在组件空间中） */
	/** 使用我们的控制点变换样条线 */
	ANIMGRAPHRUNTIME_API void SetControlPoint(int32 TransformIndex, const FTransform& InTransform);
	/** 样条线最初应用于骨架时的缓存样条线长度 */

	/** Set specified handle location (in component space) for the spline */
	/** 使用我们的线性近似来确定与球体的最早相交 */
	/** IK 链中骨骼从开始到结束的缓存数据 */
	/** 为样条线设置指定的手柄位置（在零部件空间中） */
	ANIMGRAPHRUNTIME_API void SetControlPointLocation(int32 TransformIndex, const FVector& InLocation);

	/** 缓存的骨骼长度。与 CachedBoneReferences 大小相同 */
	/** Set specified handle rotation (in component space) for the spline */
	/** 为样条线设置指定的控制柄旋转（在零部件空间中） */
	ANIMGRAPHRUNTIME_API void SetControlPointRotation(int32 TransformIndex, const FQuat& InRotation);
	/** 缓存骨骼偏移旋转。与 CachedBoneReferences 大小相同 */

	/** Set specified handle scale (in component space) for the spline */
	/** 获取指定样条 alpha 处的当前扭曲值 */
	/** 为样条线设置指定的控制柄比例（在组件空间中） */
	ANIMGRAPHRUNTIME_API void SetControlPointScale(int32 TransformIndex, const FVector& InScale);

	/** 变换样条线 */
	/** Get the number of spline transforms we are using */
	/** 获取我们正在使用的样条变换的数量 */
	int32 GetNumControlPoints() const { return ControlPoints.Num(); }
	/** 样条曲线的分段线性近似，在创建和变形时重新计算 */

	/** Build bone references & reallocate transforms from the supplied ref skeleton */
	/** 构建骨骼参考并从提供的参考骨架重新分配变换 */
	/** 我们内部维护的样条线 */
	ANIMGRAPHRUNTIME_API void GatherBoneReferences(const FReferenceSkeleton& RefSkeleton);

protected:
	/** 样条线最初应用于骨架时的缓存样条线长度 */
	/** Build spline from reference pose */
	/** 从参考姿势构建样条线 */
	ANIMGRAPHRUNTIME_API void BuildBoneSpline(const FReferenceSkeleton& RefSkeleton);
	/** IK 链中骨骼从开始到结束的缓存数据 */

protected:
	/** Transform the spline using our control points */
	/** 缓存的骨骼长度。与 CachedBoneReferences 大小相同 */
	/** 使用我们的控制点变换样条线 */
	ANIMGRAPHRUNTIME_API void TransformSpline();

	/** 缓存骨骼偏移旋转。与 CachedBoneReferences 大小相同 */
	/** Use our linear approximation to determine the earliest intersection with a sphere */
	/** 使用我们的线性近似来确定与球体的最早相交 */
	ANIMGRAPHRUNTIME_API float FindParamAtFirstSphereIntersection(const FVector& InOrigin, float InRadius, int32& StartingLinearIndex);

private:
	// FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口结束

	/** Get the current twist value at the specified spline alpha */
	/** 获取指定样条 alpha 处的当前扭曲值 */
	ANIMGRAPHRUNTIME_API float GetTwist(float InAlpha, float TotalSplineAlpha);

	/** Transformed spline */
	/** 变换样条线 */
	FSplineCurves TransformedSpline;

	/** Piecewise linear approximation of the spline, recalculated on creation and deformation */
	/** 样条曲线的分段线性近似，在创建和变形时重新计算 */
	TArray<FSplinePositionLinearApproximation> LinearApproximation;

	/** Spline we maintain internally */
	/** 我们内部维护的样条线 */
	FSplineCurves BoneSpline;

	/** Cached spline length from when the spline was originally applied to the skeleton */
	/** 样条线最初应用于骨架时的缓存样条线长度 */
	float OriginalSplineLength;

	/** Cached data for bones in the IK chain, from start to end */
	/** IK 链中骨骼从开始到结束的缓存数据 */
	TArray<FSplineIKCachedBoneData> CachedBoneReferences;

	/** Cached bone lengths. Same size as CachedBoneReferences */
	/** 缓存的骨骼长度。与 CachedBoneReferences 大小相同 */
	TArray<float> CachedBoneLengths;

	/** Cached bone offset rotations. Same size as CachedBoneReferences */
	/** 缓存骨骼偏移旋转。与 CachedBoneReferences 大小相同 */
	TArray<FQuat> CachedOffsetRotations;
};
