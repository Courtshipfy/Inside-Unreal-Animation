// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNode_BlendSpaceEvaluator.generated.h"

// Evaluates a BlendSpace at a specific using a specific time input rather than advancing time
// 使用特定时间输入而不是提前时间来评估特定的 BlendSpace
// internally. Typically the playback position of the animation for this node will represent
// 内部。通常，该节点的动画播放位置将表示
// something other than time, like jump height. Note that events output from the sequences playing
// 除了时间之外的其他因素，例如跳跃高度。请注意，播放序列的事件输出
// and being blended together should not be used. In addition, synchronization of animations
// 且不应混合在一起使用。另外，动画的同步
// will potentially be discontinuous if the blend weights are updated, as the leader/follower changes.
// 如果随着领导者/跟随者的变化而更新混合权重，则可能会不连续。
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendSpaceEvaluator : public FAnimNode_BlendSpacePlayer
{
	GENERATED_USTRUCT_BODY()

public:
	/**
	 * Normalized time between [0,1]. The actual length of a blendspace is dynamic based on the coordinate, 
	 * so it is exposed as a normalized value. Note that treating this as a "time" value that increases (and wraps)
	 * will not result in the same output as you would get from using a BlendSpace player. The output events
	 * may not be as expected, and synchronization will sometimes be discontinuous if the leader/follower 
	 * animations change as a result of changing the blend weights (even if that is done smoothly).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(PinShownByDefault))
	float NormalizedTime;

	/** 
	 * If true, teleport to normalized time, does NOT advance time (does not trigger notifies, does not 
	 * extract Root Motion, etc.). If false, will advance time (will trigger notifies, extract root motion 
	 * if applicable, etc). 
	 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	bool bTeleportToNormalizedTime = true;

public:	
	ANIMGRAPHRUNTIME_API FAnimNode_BlendSpaceEvaluator();

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

	// FAnimNode_BlendSpacePlayer interface
 // FAnimNode_BlendSpacePlayer接口
	ANIMGRAPHRUNTIME_API virtual float GetPlayRate() const override;
	virtual bool ShouldTeleportToTime() const override { return bTeleportToNormalizedTime; }
	virtual bool IsEvaluator() const override { return true; }
	// End of FAnimNode_BlendSpacePlayer
 // FAnimNode_BlendSpacePlayer 结束

};
