// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimTypes.h"
#include "Animation/AttributesRuntime.h"
#include "BonePose.h"
#include "AnimSlotEvaluationPose.generated.h"

/** Helper struct for Slot node pose evaluation. */
/** 用于槽节点姿态评估的辅助结构。 */
USTRUCT()
struct FSlotEvaluationPose
{
	GENERATED_USTRUCT_BODY()

		/** Type of additive for pose */
		/** 姿势添加剂类型 */
		UPROPERTY()
		TEnumAsByte<EAdditiveAnimationType> AdditiveType;

	/** Weight of pose */
	/** 姿势重量 */
	UPROPERTY()
		float Weight;

	/*** ATTENTION *****/
	/*** 注意力 *****/
	/* These Pose/Curve is stack allocator. You should not use it outside of stack. */
	/* 这些姿势/曲线是堆栈分配器。您不应该在堆栈之外使用它。 */
	FCompactPose Pose;
	FBlendedCurve Curve;
	UE::Anim::FStackAttributeContainer Attributes;

	FSlotEvaluationPose()
		: AdditiveType(AAT_None)
		, Weight(0.0f)
	{
	}

	FSlotEvaluationPose(float InWeight, EAdditiveAnimationType InAdditiveType)
		: AdditiveType(InAdditiveType)
		, Weight(InWeight)
	{
	}

	FSlotEvaluationPose(FSlotEvaluationPose&& InEvaluationPose)
		: AdditiveType(InEvaluationPose.AdditiveType)
		, Weight(InEvaluationPose.Weight)
	{
		Pose.MoveBonesFrom(InEvaluationPose.Pose);
		Curve.MoveFrom(InEvaluationPose.Curve);
		Attributes.MoveFrom(InEvaluationPose.Attributes);
	}

	FSlotEvaluationPose(const FSlotEvaluationPose& InEvaluationPose) = default;
	FSlotEvaluationPose& operator=(const FSlotEvaluationPose& InEvaluationPose) = default;
};
