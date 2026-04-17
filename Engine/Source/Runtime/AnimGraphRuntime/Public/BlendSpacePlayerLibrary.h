// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimExecutionContext.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "BlendSpacePlayerLibrary.generated.h"

struct FAnimNode_BlendSpacePlayer;

USTRUCT(BlueprintType)
struct FBlendSpacePlayerReference : public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_BlendSpacePlayer FInternalNodeType;
};

/**
 * Exposes operations to be performed on a blend space player anim node.
 */
UCLASS(MinimalAPI)
class UBlendSpacePlayerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get a blend space player context from an anim node context. */
	/** 从动画节点上下文获取混合空间玩家上下文。 */
	/** 从动画节点上下文获取混合空间玩家上下文。 */
	/** 从动画节点上下文获取混合空间玩家上下文。 */
	UFUNCTION(BlueprintCallable, Category = "Blend Space Player", meta = (BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FBlendSpacePlayerReference ConvertToBlendSpacePlayer(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);
	/** 从动画节点上下文（纯）获取混合空间玩家上下文。 */

	/** 从动画节点上下文（纯）获取混合空间玩家上下文。 */
	/** Get a blend space player context from an anim node context (pure). */
	/** 从动画节点上下文（纯）获取混合空间玩家上下文。 */
	UFUNCTION(BlueprintPure, Category = "Blend Space Player", meta = (BlueprintThreadSafe, DisplayName = "Convert to Blend Space Player"))
	static void ConvertToBlendSpacePlayerPure(const FAnimNodeReference& Node, FBlendSpacePlayerReference& BlendSpacePlayer, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		BlendSpacePlayer = ConvertToBlendSpacePlayer(Node, ConversionResult);
	/** 设置混合空间播放器的当前 BlendSpace。 */
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}
	/** 设置混合空间播放器的当前 BlendSpace。 */

	/** 使用内部混合时间设置混合空间播放器的当前 BlendSpace。 */
	/** Set the current BlendSpace of the blend space player. */
	/** 设置混合空间播放器的当前 BlendSpace。 */
	UFUNCTION(BlueprintCallable, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	/** 使用内部混合时间设置混合空间播放器的当前 BlendSpace。 */
	/** 设置当混合空间播放器的 BlendSpace 更改时是否重置当前播放时间。 */
	static ANIMGRAPHRUNTIME_API FBlendSpacePlayerReference SetBlendSpace(const FBlendSpacePlayerReference& BlendSpacePlayer, UBlendSpace* BlendSpace);

	/** Set the current BlendSpace of the blend space player with an interial blend time. */
	/** 使用内部混合时间设置混合空间播放器的当前 BlendSpace。 */
	/** 设置混合空间播放器的播放速率。 */
	/** 设置当混合空间播放器的 BlendSpace 更改时是否重置当前播放时间。 */
	UFUNCTION(BlueprintCallable, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FBlendSpacePlayerReference SetBlendSpaceWithInertialBlending(const FAnimUpdateContext& UpdateContext, const FBlendSpacePlayerReference& BlendSpacePlayer, UBlendSpace* BlendSpace, float BlendTime = 0.2f);

	/** 设置混合空间播放器的循环。 */
	/** Set whether the current play time should reset when BlendSpace changes of the blend space player. */
	/** 设置混合空间播放器的播放速率。 */
	/** 设置当混合空间播放器的 BlendSpace 更改时是否重置当前播放时间。 */
	UFUNCTION(BlueprintCallable, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	/** 获取混合空间播放器的当前 BlendSpace。 */
	static ANIMGRAPHRUNTIME_API FBlendSpacePlayerReference SetResetPlayTimeWhenBlendSpaceChanges(const FBlendSpacePlayerReference& BlendSpacePlayer, bool bReset);
	
	/** 设置混合空间播放器的循环。 */
	/** Set the play rate of the blend space player. */
	/** 获取混合空间播放器的当前位置。 */
	/** 设置混合空间播放器的播放速率。 */
	UFUNCTION(BlueprintCallable, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FBlendSpacePlayerReference SetPlayRate(const FBlendSpacePlayerReference& BlendSpacePlayer, float PlayRate);
	/** 获取混合空间播放器的当前 BlendSpace。 */
	/** 获取混合空间播放器的当前起始位置。 */

	/** Set the loop of the blend space player. */
	/** 设置混合空间播放器的循环。 */
	UFUNCTION(BlueprintCallable, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	/** 获取混合空间播放器的当前播放速率。 */
	/** 获取混合空间播放器的当前位置。 */
	static ANIMGRAPHRUNTIME_API FBlendSpacePlayerReference SetLoop(const FBlendSpacePlayerReference& BlendSpacePlayer, bool bLoop);

	/** Get the current BlendSpace of the blend space player. */
	/** 获取混合空间播放器的当前循环。  */
	/** 获取混合空间播放器的当前 BlendSpace。 */
	/** 获取混合空间播放器的当前起始位置。 */
	UFUNCTION(BlueprintPure, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API UBlendSpace* GetBlendSpace(const FBlendSpacePlayerReference& BlendSpacePlayer);
	/** 获取当混合空间播放器的 BlendSpace 发生变化时是否重置当前播放时间的当前值。 */

	/** Get the current position of the blend space player. */
	/** 获取混合空间播放器的当前播放速率。 */
	/** 获取混合空间播放器的当前位置。 */
	/** 强制将位置设置为指定值 */
	UFUNCTION(BlueprintPure, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FVector GetPosition(const FBlendSpacePlayerReference& BlendSpacePlayer);

	/** 获取混合空间播放器的当前循环。  */
	/** Get the current start position of the blend space player. */
	/** 获取混合空间播放器的当前起始位置。 */
	UFUNCTION(BlueprintPure, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetStartPosition(const FBlendSpacePlayerReference& BlendSpacePlayer);
	/** 获取当混合空间播放器的 BlendSpace 发生变化时是否重置当前播放时间的当前值。 */

	/** Get the current play rate of the blend space player. */
	/** 获取混合空间播放器的当前播放速率。 */
	UFUNCTION(BlueprintPure, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	/** 强制将位置设置为指定值 */
	static ANIMGRAPHRUNTIME_API float GetPlayRate(const FBlendSpacePlayerReference& BlendSpacePlayer);

	/** Get the current loop of the blend space player.  */
	/** 获取混合空间播放器的当前循环。  */
	UFUNCTION(BlueprintPure, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API bool GetLoop(const FBlendSpacePlayerReference& BlendSpacePlayer);

	/** Get the current value of whether the current play time should reset when BlendSpace changes of the blend space player. */
	/** 获取当混合空间播放器的 BlendSpace 发生变化时是否重置当前播放时间的当前值。 */
	UFUNCTION(BlueprintPure, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API bool ShouldResetPlayTimeWhenBlendSpaceChanges(const FBlendSpacePlayerReference& BlendSpacePlayer); 

	/** Forces the Position to the specified value */
	/** 强制将位置设置为指定值 */
	UFUNCTION(BlueprintCallable, Category = "Blend Space Player", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API void SnapToPosition(const FBlendSpacePlayerReference& BlendSpacePlayer, FVector NewPosition);
};



