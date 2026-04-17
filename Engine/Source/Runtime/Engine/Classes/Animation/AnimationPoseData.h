// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FCompactPose;
struct FBlendedCurve;
struct FPoseContext;
struct FSlotEvaluationPose;

namespace UE { namespace Anim { struct FStackAttributeContainer; } }

/** Structure used for passing around animation pose related data throughout the Animation Runtime */
/** 用于在整个动画运行时传递动画姿势相关数据的结构 */
struct FAnimationPoseData
{
	ENGINE_API FAnimationPoseData(FPoseContext& InPoseContext);
	ENGINE_API FAnimationPoseData(FSlotEvaluationPose& InSlotPoseContext);
	ENGINE_API FAnimationPoseData(FCompactPose& InPose, FBlendedCurve& InCurve, UE::Anim::FStackAttributeContainer& InAttributes);
	
	/** No default constructor, or assignment */
	/** 没有默认构造函数或赋值 */
	FAnimationPoseData() = delete;
	FAnimationPoseData& operator=(FAnimationPoseData&& Other) = delete;

	/** Getters for the wrapped structures */
	/** 用于包裹结构的吸气剂 */
	ENGINE_API const FCompactPose& GetPose() const;
	ENGINE_API FCompactPose& GetPose();
	ENGINE_API const FBlendedCurve& GetCurve() const;
	ENGINE_API FBlendedCurve& GetCurve();
	ENGINE_API const UE::Anim::FStackAttributeContainer& GetAttributes() const;
	ENGINE_API UE::Anim::FStackAttributeContainer& GetAttributes();

protected:
	FCompactPose& Pose;
	FBlendedCurve& Curve;
	UE::Anim::FStackAttributeContainer& Attributes;
};
