// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNode_BlendSpaceGraphBase.h"
#include "AnimNode_BlendSpaceGraph.generated.h"

// Allows multiple animations to be blended between based on input parameters
// 允许根据输入参数混合多个动画
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendSpaceGraph : public FAnimNode_BlendSpaceGraphBase
{
	GENERATED_BODY()

	// @return the sync group that this blendspace uses
	// @return 此混合空间使用的同步组
	FName GetGroupName() const { return GroupName; }
};
