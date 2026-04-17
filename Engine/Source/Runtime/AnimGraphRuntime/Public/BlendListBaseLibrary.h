// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimExecutionContext.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "BlendListBaseLibrary.generated.h"

struct FAnimNode_BlendListBase;
USTRUCT(BlueprintType)
struct FBlendListBaseReference : public FAnimNodeReference
{
	GENERATED_BODY()
	typedef FAnimNode_BlendListBase FInternalNodeType;
};

// Exposes operations to be performed on anim state machine node contexts
// 公开要在动画状态机节点上下文上执行的操作
UCLASS(Experimental, MinimalAPI)
class UBlendListBaseLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** 从动画节点上下文获取混合列表基本上下文。 */
	/** 从动画节点上下文获取混合列表基本上下文。 */
	/** Get a blend list base context from an anim node context. */
	/** 从动画节点上下文获取混合列表基本上下文。 */
	UFUNCTION(BlueprintCallable, Category = "Blend List Base", meta = (BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FBlendListBaseReference ConvertToBlendListBase(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);
	/** 将目标混合列表节点重置为从空白状态执行下一个混合 */
	/** 将目标混合列表节点重置为从空白状态执行下一个混合 */

	/** Reset target blend list node to that the next blend is executed from a blank state */
	/** 将目标混合列表节点重置为从空白状态执行下一个混合 */
	UFUNCTION(BlueprintCallable, Category = "Blend List Base", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API void ResetNode(const FBlendListBaseReference& BlendListBase);


};
