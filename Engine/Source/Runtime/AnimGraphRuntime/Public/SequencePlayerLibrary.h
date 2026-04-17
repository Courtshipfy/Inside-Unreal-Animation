// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimExecutionContext.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "SequencePlayerLibrary.generated.h"

struct FAnimNode_SequencePlayer;

USTRUCT(BlueprintType)
struct FSequencePlayerReference : public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_SequencePlayer FInternalNodeType;
};

// Exposes operations to be performed on a sequence player anim node
// 公开要在序列播放器动画节点上执行的操作
// Note: Experimental and subject to change!
// 注意：实验性的，可能会发生变化！
UCLASS(Experimental, MinimalAPI)
class USequencePlayerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/** 从动画节点上下文获取序列播放器上下文 */
public:
	/** 从动画节点上下文获取序列播放器上下文 */
	/** Get a sequence player context from an anim node context */
	/** 从动画节点上下文获取序列播放器上下文 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Sequences", meta = (BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	/** 从动画节点上下文获取序列播放器上下文（纯） */
	/** 从动画节点上下文获取序列播放器上下文（纯） */
	static ANIMGRAPHRUNTIME_API FSequencePlayerReference ConvertToSequencePlayer(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);

	/** Get a sequence player context from an anim node context (pure) */
	/** 从动画节点上下文获取序列播放器上下文（纯） */
	UFUNCTION(BlueprintPure, Category = "Animation|Sequences", meta = (BlueprintThreadSafe, DisplayName = "Convert to Sequence Player"))
	static void ConvertToSequencePlayerPure(const FAnimNodeReference& Node, FSequencePlayerReference& SequencePlayer, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		SequencePlayer = ConvertToSequencePlayer(Node, ConversionResult);
	/** 设置序列播放器当前累计时间 */
	/** 设置序列播放器当前累计时间 */
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}

	/** Set the current accumulated time of the sequence player */
	/** 设置序列播放器当前累计时间 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FSequencePlayerReference SetAccumulatedTime(const FSequencePlayerReference& SequencePlayer, float Time);

	/** 
	 * Set the start position of the sequence player. 
	 * If this is called from On Become Relevant or On Initial Update then it should be accompanied by a call to
	/** 设置序列播放器的播放速率 */
	 * SetAccumulatedTime to achieve the desired effect of resetting the play time of a sequence player.
	/** 设置序列播放器的播放速率 */
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	/** 设置序列播放器的当前序列 */
	static ANIMGRAPHRUNTIME_API FSequencePlayerReference SetStartPosition(const FSequencePlayerReference& SequencePlayer, float StartPosition);

	/** 设置序列播放器的当前序列 */
	/** Set the play rate of the sequence player */
	/** 使用惯性混合时间设置序列播放器的当前序列 */
	/** 设置序列播放器的播放速率 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FSequencePlayerReference SetPlayRate(const FSequencePlayerReference& SequencePlayer, float PlayRate);
	/** 使用惯性混合时间设置序列播放器的当前序列 */
	/** 获取序列播放器的当前序列 - 已弃用，请使用纯版本 */

	/** Set the current sequence of the sequence player */
	/** 设置序列播放器的当前序列 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	/** 获取序列播放器当前的序列 */
	/** 获取序列播放器的当前序列 - 已弃用，请使用纯版本 */
	static ANIMGRAPHRUNTIME_API FSequencePlayerReference SetSequence(const FSequencePlayerReference& SequencePlayer, UAnimSequenceBase* Sequence);

	/** Set the current sequence of the sequence player with an inertial blend time */
	/** 获取序列播放器当前累计时间 */
	/** 使用惯性混合时间设置序列播放器的当前序列 */
	/** 获取序列播放器当前的序列 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FSequencePlayerReference SetSequenceWithInertialBlending(const FAnimUpdateContext& UpdateContext, const FSequencePlayerReference& SequencePlayer, UAnimSequenceBase* Sequence, float BlendTime = 0.2f);
	/** 获取序列播放器的起始位置 */

	/** Get the current sequence of the sequence player - DEPRECATED, please use pure version */
	/** 获取序列播放器当前累计时间 */
	/** 获取序列播放器的当前序列 - 已弃用，请使用纯版本 */
	/** 获取序列播放器的播放速率 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Sequences", meta = (BlueprintThreadSafe, DeprecatedFunction))
	static ANIMGRAPHRUNTIME_API FSequencePlayerReference GetSequence(const FSequencePlayerReference& SequencePlayer, UPARAM(Ref) UAnimSequenceBase*& SequenceBase);
	
	/** 获取序列播放器的起始位置 */
	/** 获取序列播放器的循环状态 */
	/** Get the current sequence of the sequence player */
	/** 获取序列播放器当前的序列 */
	UFUNCTION(BlueprintPure, Category = "Animation|Sequences", meta = (BlueprintThreadSafe, DisplayName = "Get Sequence"))
	static ANIMGRAPHRUNTIME_API UAnimSequenceBase* GetSequencePure(const FSequencePlayerReference& SequencePlayer);
	/** 如果需要特定的动画持续时间，则返回播放此动画时提供的播放速率 */
	/** 获取序列播放器的播放速率 */

	/** Gets the current accumulated time of the sequence player */
	/** 获取序列播放器当前累计时间 */
	UFUNCTION(BlueprintPure, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	/** 获取序列播放器的循环状态 */
	static ANIMGRAPHRUNTIME_API float GetAccumulatedTime(const FSequencePlayerReference& SequencePlayer);

	/** Get the start position of the sequence player */
	/** 获取序列播放器的起始位置 */
	/** 如果需要特定的动画持续时间，则返回播放此动画时提供的播放速率 */
	UFUNCTION(BlueprintPure, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetStartPosition(const FSequencePlayerReference& SequencePlayer);

	/** Get the play rate of the sequence player */
	/** 获取序列播放器的播放速率 */
	UFUNCTION(BlueprintPure, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetPlayRate(const FSequencePlayerReference& SequencePlayer);

	/** Get the looping state of the sequence player */
	/** 获取序列播放器的循环状态 */
	UFUNCTION(BlueprintPure, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API bool GetLoopAnimation(const FSequencePlayerReference& SequencePlayer);

	/** Returns the Play Rate to provide when playing this animation if a specific animation duration is desired */
	/** 如果需要特定的动画持续时间，则返回播放此动画时提供的播放速率 */
	UFUNCTION(BlueprintPure, Category = "Animation|Sequences", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float ComputePlayRateFromDuration(const FSequencePlayerReference& SequencePlayer, float Duration = 1.f);
};
