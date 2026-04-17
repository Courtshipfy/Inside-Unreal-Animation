// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimationAsset.h"
#include "Animation/PoseAsset.h"
#include "Animation/AnimNode_AssetPlayerBase.h"
#include "AnimNode_PoseHandler.generated.h"

// Evaluates a point in an anim sequence, using a specific time input rather than advancing time internally.
// 使用特定的时间输入而不是在内部提前时间来评估动画序列中的点。
// Typically the playback position of the animation for this node will represent something other than time, like jump height.
// 通常，该节点的动画播放位置将表示时间以外的其他内容，例如跳跃高度。
// This node will not trigger any notifies present in the associated sequence.
// 该节点不会触发关联序列中存在的任何通知。
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_PoseHandler : public FAnimNode_AssetPlayerBase
{
	GENERATED_USTRUCT_BODY()
public:
	// The animation sequence asset to evaluate
	// 要评估的动画序列资产
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	TObjectPtr<UPoseAsset> PoseAsset;

public:	
	FAnimNode_PoseHandler()
		:PoseAsset(nullptr)
	{
	}

	// FAnimNode_AssetPlayerBase interface
	// FAnimNode_AssetPlayerBase接口
	virtual float GetCurrentAssetTime() const { return 0.f; }
	virtual float GetCurrentAssetLength() const { return 0.f; }
	// End of FAnimNode_AssetPlayerBase interface
	// FAnimNode_AssetPlayerBase接口结束

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// FAnimNode_AssetPlayerBase Interface
	// FAnimNode_AssetPlayerBase接口
	virtual float GetAccumulatedTime() const {return 0.f;}
	virtual void SetAccumulatedTime(float NewTime) {}
	virtual UAnimationAsset* GetAnimAsset() const {return PoseAsset;}
	// End of FAnimNode_AssetPlayerBase Interface
	// FAnimNode_AssetPlayerBase接口结束

#if WITH_EDITORONLY_DATA
	// Set the pose asset to use for this node 
	// 设置用于该节点的姿势资源
	ANIMGRAPHRUNTIME_API void SetPoseAsset(UPoseAsset* InPoseAsset);
#endif
	
protected:
	/** Called after CurrentPoseAsset is changed.  */
	/** CurrentPoseAsset 更改后调用。  */
	virtual void OnPoseAssetChange() {}

	TWeakObjectPtr<UPoseAsset> CurrentPoseAsset;
	FAnimExtractContext PoseExtractContext;
	// weight to blend pose per joint - has to be cached whenever cache bones for LOD
	// 每个关节混合姿势的权重 - 每当为 LOD 缓存骨骼时都必须缓存
	// note this is for mesh bone
	// 注意这是针对网格骨骼的
	TArray<float> BoneBlendWeights;

	/* Rebuild pose list */
	/* 重建姿势列表 */
	ANIMGRAPHRUNTIME_API virtual void RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset);

	/** Cache bone blend weights - called when pose asset changes */
	/** 缓存骨骼混合权重 - 在姿势资源更改时调用 */
	ANIMGRAPHRUNTIME_API void CacheBoneBlendWeights(FAnimInstanceProxy* InstanceProxy);
	
private:
	ANIMGRAPHRUNTIME_API void UpdatePoseAssetProperty(struct FAnimInstanceProxy* InstanceProxy);
};

