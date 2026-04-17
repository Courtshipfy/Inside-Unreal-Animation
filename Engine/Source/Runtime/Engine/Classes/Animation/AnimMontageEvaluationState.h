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
 // 要评估的蒙太奇
	TWeakObjectPtr<UAnimMontage> Montage;

	// The current blend information.
 // 当前的混合信息。
	FAlphaBlend BlendInfo;

	// The active blend profile. Montages have a profile for blending in and blending out.
 // 活动混合配置文件。蒙太奇具有混入和混出的配置文件。
	const UBlendProfile* ActiveBlendProfile;

	// The position to evaluate this montage at
 // 评估该蒙太奇的位置
	float MontagePosition;

	// The previous MontagePosition and delta leading into current
 // 先前的 MontagePosition 和引入当前的 delta
	FDeltaTimeRecord DeltaTimeRecord;

	// The linear alpha value where to start blending from. So not the blended value that already has been curve sampled.
 // 开始混合的线性 alpha 值。所以不是已经被曲线采样的混合值。
	float BlendStartAlpha;

	// Whether this montage is playing
 // 这段蒙太奇是否正在播放
	bool bIsPlaying;

	// Whether this montage is valid and not stopped
 // 该剪辑是否有效且未停止
	bool bIsActive;
};
