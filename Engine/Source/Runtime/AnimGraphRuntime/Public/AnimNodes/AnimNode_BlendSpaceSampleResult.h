// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNode_Root.h"
#include "AnimNode_BlendSpaceSampleResult.generated.h"

// Root node of a blend space sample (sink node).
// 混合空间样本的根节点（接收节点）。
// We dont use AnimNode_Root to let us distinguish these nodes in the property list at link time.
// 我们不使用 AnimNode_Root 来让我们在链接时区分属性列表中的这些节点。
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendSpaceSampleResult : public FAnimNode_Root
{
	GENERATED_BODY()
};
