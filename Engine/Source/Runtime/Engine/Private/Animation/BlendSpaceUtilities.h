// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FSyncPattern
{
	// The markers that make up this pattern
	// 构成此图案的标记
	TArray<FName> MarkerNames;

	// Returns the index of the supplied name in the array of marker names
	// 返回提供的名称在标记名称数组中的索引
	// Search starts at StartIndex
	// [翻译失败: Search starts at StartIndex]
	int32 IndexOf(const FName Name, const int32 StartIndex = 0) const;
	
	// Tests the supplied pattern against ours, starting at the supplied start index
	// [翻译失败: Tests the supplied pattern against ours, starting at the supplied start index]
	bool DoOneMatch(const TArray<FName>& TestMarkerNames, const int32 StartIndex);

	// Testes the supplied pattern against ourselves. Is not a straight forward array match
	// 对照我们自己测试提供的模式。不是直接的数组匹配
	// (for example a,b,c,a would match b,c,a,a)
	// （例如 a,b,c,a 将匹配 b,c,a,a）
	bool DoesPatternMatch(const TArray<FName>& TestMarkerNames);
};

class FBlendSpaceUtilities
{
public:
	static int32 GetHighestWeightSample(const TArray<struct FBlendSampleData> &SampleDataList);
	static int32 GetHighestWeightMarkerSyncSample(const TArray<struct FBlendSampleData> &SampleDataList, const TArray<struct FBlendSample>& BlendSamples);
};
