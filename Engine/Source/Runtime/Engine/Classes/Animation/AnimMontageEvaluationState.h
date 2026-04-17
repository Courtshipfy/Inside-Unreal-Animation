// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimMontage.h"

struct FMontageEvaluationState
{
	FMontageEvaluationState(UAnimMontage* InMontage, float InPosition, FDeltaTimeRecord InDeltaTimeRecord, bool bInIsPlaying, bool bInIsActive, const FAlphaBlend& InBlendInfo, const UBlendProfile* InActiveBlendProfile, float InBlendStartAlpha)
		: Montage(InMontage)
		, BlendInfo(InBlendInfo)
		, ActiveBlendProfile(InActiveBlendProfile)
		, MontagePosition(InPosition)
		, DeltaTimeRecord(InDeltaTimeRecord)
		, BlendStartAlpha(InBlendStartAlpha)
		, bIsPlaying(bInIsPlaying)
		, bIsActive(bInIsActive)
	{
	}

	// The montage to evaluate
	// [翻译失败: The montage to evaluate]
	TWeakObjectPtr<UAnimMontage> Montage;

	// The current blend information.
	// [翻译失败: The current blend information.]
	FAlphaBlend BlendInfo;

	// The active blend profile. Montages have a profile for blending in and blending out.
	// [翻译失败: The active blend profile. Montages have a profile for blending in and blending out.]
	const UBlendProfile* ActiveBlendProfile;

	// The position to evaluate this montage at
	// [翻译失败: The position to evaluate this montage at]
	float MontagePosition;

	// The previous MontagePosition and delta leading into current
	// [翻译失败: The previous MontagePosition and delta leading into current]
	FDeltaTimeRecord DeltaTimeRecord;

	// The linear alpha value where to start blending from. So not the blended value that already has been curve sampled.
	// [翻译失败: The linear alpha value where to start blending from. So not the blended value that already has been curve sampled.]
	float BlendStartAlpha;

	// Whether this montage is playing
	// 这段蒙太奇是否正在播放
	bool bIsPlaying;

	// Whether this montage is valid and not stopped
	// 该剪辑是否有效且未停止
	bool bIsActive;
};
