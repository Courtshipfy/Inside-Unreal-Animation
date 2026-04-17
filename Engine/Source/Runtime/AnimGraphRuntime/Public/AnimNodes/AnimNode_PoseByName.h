// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimNodes/AnimNode_PoseHandler.h"
#include "AnimNode_PoseByName.generated.h"

// Evaluates a point in an anim sequence, using a specific time input rather than advancing time internally.
// 使用特定的时间输入而不是在内部提前时间来评估动画序列中的点。
// Typically the playback position of the animation for this node will represent something other than time, like jump height.
// 通常，该节点的动画播放位置将表示时间以外的其他内容，例如跳跃高度。
// This node will not trigger any notifies present in the associated sequence.
// 该节点不会触发关联序列中存在的任何通知。
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_PoseByName : public FAnimNode_PoseHandler
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	FName PoseName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float PoseWeight;

private:
	/** Cached current pose name used for invalidation */
	/** 缓存的当前姿势名称用于失效 */
	FName CurrentPoseName;

public:	
	FAnimNode_PoseByName()
		: PoseWeight(1.f)
		, CurrentPoseName(NAME_None)
	{
	}

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束
private:
	ANIMGRAPHRUNTIME_API virtual void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) override;
};

