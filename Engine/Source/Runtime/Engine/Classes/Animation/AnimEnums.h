// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimEnums.generated.h"

UENUM()
/** Root Bone Lock options when extracting Root Motion. */
/** [翻译失败: Root Bone Lock options when extracting Root Motion.] */
namespace ERootMotionRootLock
{
	enum Type : int
	{
		/** Use reference pose root bone position. */
		/** [翻译失败: Use reference pose root bone position.] */
		RefPose,

		/** Use root bone position on first frame of animation. */
		/** [翻译失败: Use root bone position on first frame of animation.] */
		AnimFirstFrame,

		/** FTransform::Identity. */
		/** FTransform::身份。 */
		Zero
	};
}

UENUM()
namespace ERootMotionMode
{
	enum Type : int
	{
		/** Leave root motion in animation. */
		/** 将根运动保留在动画中。 */
		NoRootMotionExtraction,

		/** Extract root motion but do not apply it. */
		/** 提取根运动但不应用它。 */
		IgnoreRootMotion,

		/** Root motion is taken from all animations contributing to the final pose, not suitable for network multiplayer setups. */
		/** 根运动取自对最终姿势有贡献的所有动画，不适合网络多人游戏设置。 */
		RootMotionFromEverything,

		/** Root motion is only taken from montages, suitable for network multiplayer setups. */
		/** 根运动仅取自蒙太奇，适用于网络多人游戏设置。 */
		RootMotionFromMontagesOnly,
	};
}

/**
* For an additive animation, indicates what the animation is relative to.
*/
UENUM()
enum EAdditiveBasePoseType : int
{
	/** Will be deprecated. */
	/** 将被弃用。 */
	ABPT_None UMETA(DisplayName = "None"),
	/** Use the Skeleton's ref pose as base. */
	/** 使用骨骼的参考姿势作为基础。 */
	ABPT_RefPose UMETA(DisplayName = "Skeleton Reference Pose"),
	/** Use a whole animation as a base pose. BasePoseSeq must be set. */
	/** 使用整个动画作为基本姿势。必须设置 BasePoseSeq。 */
	ABPT_AnimScaled UMETA(DisplayName = "Selected animation scaled"),
	/** Use one frame of an animation as a base pose. BasePoseSeq and RefFrameIndex must be set (RefFrameIndex will be clamped). */
	/** 使用动画的一帧作为基本姿势。必须设置 BasePoseSeq 和 RefFrameIndex（RefFrameIndex 将被限制）。 */
	ABPT_AnimFrame UMETA(DisplayName = "Selected animation frame"),
	/** Use one frame of this animation. RefFrameIndex must be set (RefFrameIndex will be clamped). */
	/** 使用此动画的一帧。必须设置 RefFrameIndex（RefFrameIndex 将被限制）。 */
	ABPT_LocalAnimFrame UMETA(DisplayName = "Frame from this animation"),
	ABPT_MAX,
};


/**
* Indicates animation data compression format.
*/
UENUM()
enum AnimationCompressionFormat : int
{
	ACF_None,
	ACF_Float96NoW,
	ACF_Fixed48NoW,
	ACF_IntervalFixed32NoW,
	ACF_Fixed32NoW,
	ACF_Float32NoW,
	ACF_Identity,
	ACF_MAX UMETA(Hidden),
};