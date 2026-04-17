// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_Slot.generated.h"

// An animation slot node normally acts as a passthru, but a montage or PlaySlotAnimation call from
// 动画槽节点通常充当通路，但蒙太奇或 PlaySlotAnimation 调用来自
// game code can cause an animation to blend in and be played on the slot temporarily, overriding the
// 游戏代码可以导致动画混合并暂时在插槽上播放，从而覆盖
// Source input.
// 源输入。
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_Slot : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	// The source input, passed thru to the output unless a montage or slot animation is currently playing
	// 源输入，传递到输出，除非当前正在播放蒙太奇或插槽动画
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink Source;

	// The name of this slot, exposed to gameplay code, etc...
	// 该插槽的名称、暴露的游戏代码等......
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(CustomizeProperty))
	FName SlotName;

	//Whether we should continue to update the source pose regardless of whether it would be used.
	//无论是否使用源位姿，我们是否应该继续更新源位姿。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool bAlwaysUpdateSourcePose;

protected:
	virtual void PostEvaluateSourcePose(FPoseContext& SourceContext) {}

	FSlotNodeWeightInfo WeightData;
	FGraphTraversalCounter SlotNodeInitializationCounter;

public:	
	ANIMGRAPHRUNTIME_API FAnimNode_Slot();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束
};
