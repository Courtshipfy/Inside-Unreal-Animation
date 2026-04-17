// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoneIndices.h"
#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Math/MathFwd.h"
#include "Math/Transform.h"
#include "UObject/ObjectMacros.h"

#include "CCDIK.generated.h"

/** Transient structure for CCDIK node evaluation */
/** CCDIK节点评估的瞬态结构 */
/** CCDIK节点评估的瞬态结构 */
/** CCDIK节点评估的瞬态结构 */
USTRUCT()
struct FCCDIKChainLink
{
	GENERATED_USTRUCT_BODY()

	/** 组件空间中骨骼的变换。 */
public:
	/** 组件空间中骨骼的变换。 */
	/** Transform of bone in component space. */
	/** 局部空间中骨骼的变换。当它们的组件空间或父级发生变化时，这是可变的*/
	/** 组件空间中骨骼的变换。 */
	FTransform Transform;
	/** 局部空间中骨骼的变换。当它们的组件空间或父级发生变化时，这是可变的*/
	/** 该控件将输出的变换索引 */

	/** Transform of bone in local space. This is mutable as their component space changes or parents*/
	/** 局部空间中骨骼的变换。当它们的组件空间或父级发生变化时，这是可变的*/
	/** 该控件将输出的变换索引 */
	FTransform LocalTransform;

	/** Transform Index that this control will output */
	/** 该控件将输出的变换索引 */
	int32 TransformIndex;

	/** Child bones which are overlapping this bone. 
	 * They have a zero length distance, so they will inherit this bone's transformation. */
	TArray<int32> ChildZeroLengthTransformIndices;

	double CurrentAngleDelta;

	FCCDIKChainLink()
		: TransformIndex(INDEX_NONE)
		, CurrentAngleDelta(0.0)
	{
	}

	FCCDIKChainLink(const FTransform& InTransform, const FTransform& InLocalTransform, const int32& InTransformIndex)
		: Transform(InTransform)
		, LocalTransform(InLocalTransform)
		, TransformIndex(InTransformIndex)
		, CurrentAngleDelta(0.0)
	{
	}
};

namespace AnimationCore
{
	ANIMATIONCORE_API bool SolveCCDIK(TArray<FCCDIKChainLink>& InOutChain, const FVector& TargetPosition, float Precision, int32 MaxIteration, bool bStartFromTail, bool bEnableRotationLimit, const TArray<float>& RotationLimitPerJoints);
};
