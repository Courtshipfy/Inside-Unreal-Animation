// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimNode_RelevantAssetPlayerBase.h"
#include "Animation/AnimSync.h"
#include "AnimNode_AssetPlayerBase.generated.h"

/* Base class for any asset playing anim node */
/* 任何播放动画节点的资源的基类 */
/* 任何播放动画节点的资源的基类 */
/* 任何播放动画节点的资源的基类 */
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_AssetPlayerBase : public FAnimNode_AssetPlayerRelevancyBase
{
	GENERATED_BODY()

	friend class UAnimGraphNode_AssetPlayerBase;

	FAnimNode_AssetPlayerBase() = default;
	/** 用于设置目的的初始化功能 */

	/** 用于设置目的的初始化功能 */
	/** Initialize function for setup purposes */
	/** 用于设置目的的初始化功能 */
	ENGINE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;

	/** Update the node, marked final so we can always handle blendweight caching.
	 *  Derived classes should implement UpdateAssetPlayer
	/** 资产播放器的更新方法，由派生类实现 */
	 */
	ENGINE_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) final override;
	/** 资产播放器的更新方法，由派生类实现 */

	/** Update method for the asset player, to be implemented by derived classes */
	/** 资产播放器的更新方法，由派生类实现 */
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) {};

	// Create a tick record for this node
 // 为该节点创建tick记录
	ENGINE_API void CreateTickRecordForNode(const FAnimationUpdateContext& Context, UAnimSequenceBase* Sequence, bool bLooping, float PlayRate, bool bIsEvaluator);

	// Get the sync group name we are using
 // 获取我们正在使用的同步组名称
	virtual FName GetGroupName() const { return NAME_None; }

	// Get the sync group role we are using
 // 获取我们正在使用的同步组角色
	virtual EAnimGroupRole::Type GetGroupRole() const { return EAnimGroupRole::CanBeLeader; }

	// Get the sync group method we are using
 // 获取我们正在使用的同步组方法
	virtual EAnimSyncMethod GetGroupMethod() const { return EAnimSyncMethod::DoNotSync; }

	// Get the flag that determines if this asset player will not sync to the previous leader's sync position when joining a sync group and before becoming the leader but instead force everyone else to match its position.
 // 获取标志，确定该资产播放器在加入同步组时以及成为领导者之前是否不会同步到前一个领导者的同步位置，而是强制其他人匹配其位置。
	virtual bool GetOverridePositionWhenJoiningSyncGroupAsLeader() const { return false; }

	// Set the sync group name we are using
 // 设置我们正在使用的同步组名称
	virtual bool SetGroupName(FName InGroupName) { return false; }

 // --- FAnimNode_RelevantAssetPlayerBase ---
 // --- FAnimNode_RelevantAssetPlayerBase ---
 // --- FAnimNode_RelevantAssetPlayerBase ---
 // --- FAnimNode_RelevantAssetPlayerBase ---
	// Set the sync group role we are using
 // 设置我们正在使用的同步组角色
	// --- FAnimNode_RelevantAssetPlayerBase ---
 // --- FAnimNode_RelevantAssetPlayerBase ---
	virtual bool SetGroupRole(EAnimGroupRole::Type InRole) { return false; }

	// --- FAnimNode_RelevantAssetPlayerBase ---
 // --- FAnimNode_RelevantAssetPlayerBase ---
	// Set the sync group method we are using
 // 设置我们正在使用的同步组方法
	virtual bool SetGroupMethod(EAnimSyncMethod InMethod) { return false; }

	// Set the flag that determines if this asset player will not sync to the previous leader's sync position when joining a sync group and before becoming the leader but instead force everyone else to match its position. 
 // 设置标志，确定该资产播放器在加入同步组时以及成为领导者之前是否不会同步到前一个领导者的同步位置，而是强制其他人匹配其位置。
	virtual bool SetOverridePositionWhenJoiningSyncGroupAsLeader(bool InOverridePositionWhenJoiningSyncGroupAsLeader) { return false; }
	
	// --- FAnimNode_RelevantAssetPlayerBase ---
 // --- FAnimNode_RelevantAssetPlayerBase ---
	// --- FAnimNode_RelevantAssetPlayerBase ---
 // --- FAnimNode_RelevantAssetPlayerBase ---
	ENGINE_API virtual float GetAccumulatedTime() const override;
	ENGINE_API virtual void SetAccumulatedTime(float NewTime) override;
	ENGINE_API virtual float GetCachedBlendWeight() const override;
	ENGINE_API virtual void ClearCachedBlendWeight() override;
	ENGINE_API virtual float GetCurrentAssetTimePlayRateAdjusted() const override;
	ENGINE_API virtual const FDeltaTimeRecord* GetDeltaTimeRecord() const override;
	/** 使用基于标记的同步时存储有关当前标记位置的数据*/
	// --- End of FAnimNode_RelevantAssetPlayerBase ---
 // --- FAnimNode_RelevantAssetPlayerBase 结束 ---

	/** 该节点最后遇到的混合权重 */
private:
	/** 使用基于标记的同步时存储有关当前标记位置的数据*/
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	int32 GroupIndex_DEPRECATED = INDEX_NONE;
	/** 该节点最后遇到的混合权重 */
	/** 用于引用该节点中资产的累计时间 */

	UPROPERTY()
	EAnimSyncGroupScope GroupScope_DEPRECATED = EAnimSyncGroupScope::Local;
	/** 用于引用该节点中资产的累计时间 */
#endif
	/** 前一帧InternalTimeAccumulator值和进入当前帧的有效增量时间 */
	
protected:
	/** 前一帧InternalTimeAccumulator值和进入当前帧的有效增量时间 */
	/** Store data about current marker position when using marker based syncing*/
	/** 跟踪我们之前是否已经满重。当体重达到 0 时重置*/
	/** 使用基于标记的同步时存储有关当前标记位置的数据*/
	/** 跟踪我们之前是否已经满重。当体重达到 0 时重置*/
	FMarkerTickRecord MarkerTickRecord;

	/** 获取该资产播放器生成的刻度记录的同步参数 */
	/** 获取该资产播放器生成的刻度记录的同步参数 */
	/** Last encountered blendweight for this node */
	/** 该节点最后遇到的混合权重 */
	UPROPERTY(BlueprintReadWrite, Transient, Category=DoNotEdit)
	float BlendWeight = 0.0f;

	/** Accumulated time used to reference the asset in this node */
	/** 用于引用该节点中资产的累计时间 */
	UPROPERTY(BlueprintReadWrite, Transient, Category=DoNotEdit)
	float InternalTimeAccumulator = 0.0f;
	
	/** Previous frame InternalTimeAccumulator value and effective delta time leading into the current frame */
	/** 前一帧InternalTimeAccumulator值和进入当前帧的有效增量时间 */
	FDeltaTimeRecord DeltaTimeRecord;

	/** Track whether we have been full weight previously. Reset when we reach 0 weight*/
	/** 跟踪我们之前是否已经满重。当体重达到 0 时重置*/
	bool bHasBeenFullWeight = false;

	/** Get sync parameters for the tick record produced by this asset player */
	/** 获取该资产播放器生成的刻度记录的同步参数 */
	ENGINE_API UE::Anim::FAnimSyncParams GetSyncParams(bool bRequestedInertialization) const;
};
