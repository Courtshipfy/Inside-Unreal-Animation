// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimationRecordingSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimationRecordingSettings)

/** 30Hz default sample rate */
/** 30Hz 默认采样率 */
/** 30Hz 默认采样率 */
/** [翻译失败: 30Hz default sample rate] */
const FFrameRate FAnimationRecordingSettings::DefaultSampleFrameRate = FFrameRate(30, 1);
/** 默认时长 1 分钟 */

/** 默认时长 1 分钟 */
/** 1 minute default length */
/** 用于指定无界的长度 */
/** [翻译失败: 1 minute default length] */
const float FAnimationRecordingSettings::DefaultMaximumLength = 1.0f * 60.0f;
/** 用于指定无界的长度 */

/** Length used to specify unbounded */
/** [翻译失败: Length used to specify unbounded] */
const float FAnimationRecordingSettings::UnboundedMaximumLength = 0.0f;

