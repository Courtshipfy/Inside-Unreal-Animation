// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimNode_AssetPlayerBase.h"
#include "AnimNode_BlendSpacePlayer.generated.h"

class UBlendSpace;

//@TODO: Comment
//@TODO：评论
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendSpacePlayerBase : public FAnimNode_AssetPlayerBase
{
	GENERATED_BODY()

protected:
	// Filter used to dampen coordinate changes
	// 用于抑制坐标变化的滤波器
	FBlendFilter BlendFilter;

	// Cache of samples used to determine blend weights
	// 用于确定混合权重的样本缓存
	TArray<FBlendSampleData> BlendSampleDataCache;

	/** Previous position in the triangulation/segmentation */
	/** 三角测量/分割中的先前位置 */
	int32 CachedTriangulationIndex = -1;

	UPROPERTY(Transient)
	TObjectPtr<UBlendSpace> PreviousBlendSpace = nullptr;

public:	

	// FAnimNode_AssetPlayerBase interface
	// FAnimNode_AssetPlayerBase接口
	ANIMGRAPHRUNTIME_API virtual float GetCurrentAssetTime() const override;
	ANIMGRAPHRUNTIME_API virtual float GetCurrentAssetTimePlayRateAdjusted() const override;
	ANIMGRAPHRUNTIME_API virtual float GetCurrentAssetLength() const override;
	ANIMGRAPHRUNTIME_API virtual UAnimationAsset* GetAnimAsset() const override;
	// End of FAnimNode_AssetPlayerBase interface
	// FAnimNode_AssetPlayerBase接口结束

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// Get the amount of time from the end
	// 获取距离结束的时间量
	ANIMGRAPHRUNTIME_API float GetTimeFromEnd(float CurrentTime) const;

	// @return the current sample coordinates after going through the filtering
	// @返回经过过滤后的当前样本坐标
	FVector GetFilteredPosition() const { return BlendFilter.GetFilterLastOutput(); }

	// Forces the Position to the specified value
	// 强制将位置设置为指定值
	ANIMGRAPHRUNTIME_API void SnapToPosition(const FVector& NewPosition);

public:

	// Get the blendspace asset to play
	// 获取要播放的混合空间资源
	virtual UBlendSpace* GetBlendSpace() const PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::GetBlendSpace, return nullptr;);

	// Get the coordinates that are currently being sampled by the blendspace
	// 获取混合空间当前正在采样的坐标
	virtual FVector GetPosition() const PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::GetPosition, return FVector::Zero(););

	// The start position in [0, 1] to use when initializing. When looping, play will still jump back to the beginning when reaching the end.
	// 初始化时使用 [0, 1] 中的起始位置。循环时，播放到达结束时仍会跳回开头。
	virtual float GetStartPosition() const PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::GetStartPosition, return 0.0f;);

	// Get the play rate multiplier. Can be negative, which will cause the animation to play in reverse.
	// 获取播放率乘数。可以为负数，这将导致动画反向播放。
	virtual float GetPlayRate() const PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::GetPlayRate, return 1.0f;);

	// Should the animation loop back to the start when it reaches the end?
	// 当动画到达结束时是否应该循环回到开始处？
	UE_DEPRECATED(5.3, "Please use IsLooping instead.")
	virtual bool GetLoop() const final { return IsLooping(); }

	// Get whether we should reset the current play time when the blend space changes
	// 获取当混合空间改变时我们是否应该重置当前播放时间
	virtual bool ShouldResetPlayTimeWhenBlendSpaceChanges() const PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::ShouldResetPlayTimeWhenBlendSpaceChanges, return true;);

	// Set whether we should reset the current play time when the blend space changes
	// 设置混合空间改变时是否重置当前播放时间
	virtual bool SetResetPlayTimeWhenBlendSpaceChanges(bool bReset) PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::SetResetPlayTimeWhenBlendSpaceChanges, return false;);

	// An evaluator will be setting the play rate to zero and setting the time explicitly. ShouldTeleportToTime indicates whether we should jump to that time, or move to it playing out root motion and events etc.
	// 评估者会将播放速率设置为零并明确设置时间。 ShouldTeleportToTime 指示我们是否应该跳到该时间，或者移动到该时间播放根运动和事件等。
	virtual bool ShouldTeleportToTime() const { return false; }

	// Indicates if we are an evaluator - i.e. will be setting the time explicitly rather than letting it play out
	// 表明我们是否是评估者 - 即会明确设定时间而不是让它发挥作用
	virtual bool IsEvaluator() const { return false; }

	// Set the blendspace asset to play
	// 设置要播放的混合空间资源
	virtual bool SetBlendSpace(UBlendSpace* InBlendSpace) PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::SetBlendSpace, return false;);

	// Set the coordinates that are currently being sampled by the blendspace
	// 设置混合空间当前正在采样的坐标
	virtual bool SetPosition(FVector InPosition) PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::SetPosition, return false;);

	// Set the play rate multiplier. Can be negative, which will cause the animation to play in reverse.
	// 设置播放速率倍数。可以为负数，这将导致动画反向播放。
	virtual bool SetPlayRate(float InPlayRate) PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::SetPlayRate, return false;);

	// Set if the animation should loop back to the start when it reaches the end?
	// 设置动画到达结束时是否循环回到开始处？
	virtual bool SetLoop(bool bInLoop) PURE_VIRTUAL(FAnimNode_BlendSpacePlayerBase::SetLoop, return false;);

protected:
	ANIMGRAPHRUNTIME_API void UpdateInternal(const FAnimationUpdateContext& Context);

private:
	ANIMGRAPHRUNTIME_API void Reinitialize(bool bResetTime = true);

	ANIMGRAPHRUNTIME_API const FBlendSampleData* GetHighestWeightedSample() const;
};

template<>
struct TStructOpsTypeTraits<FAnimNode_BlendSpacePlayerBase> : public TStructOpsTypeTraitsBase2<FAnimNode_BlendSpacePlayerBase>
{
	enum
	{
		WithPureVirtual = true,
	};
};


//@TODO: Comment
//@TODO：评论
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendSpacePlayer : public FAnimNode_BlendSpacePlayerBase
{
	GENERATED_BODY()

	friend class UAnimGraphNode_BlendSpaceBase;
	friend class UAnimGraphNode_BlendSpacePlayer;
	friend class UAnimGraphNode_BlendSpaceEvaluator;
	friend class UAnimGraphNode_RotationOffsetBlendSpace;
	friend class UAnimGraphNode_AimOffsetLookAt;

private:

#if WITH_EDITORONLY_DATA
	// The group name that we synchronize with (NAME_None if it is not part of any group). Note that
	// 我们与之同步的组名称（如果不属于任何组，则为 NAME_None）。注意
	// this is the name of the group used to sync the output of this node - it will not force
	// 这是用于同步该节点输出的组的名称 - 它不会强制
	// syncing of animations contained by it.
	// 同步其中包含的动画。
	UPROPERTY(EditAnywhere, Category = Sync, meta = (FoldProperty))
	FName GroupName = NAME_None;

	// The role this node can assume within the group (ignored if GroupName is not set). Note
	// 该节点在组内可以承担的角色（如果未设置 GroupName，则忽略）。笔记
	// that this is the role of the output of this node, not of animations contained by it.
	// 这是该节点输出的作用，而不是它包含的动画的作用。
	UPROPERTY(EditAnywhere, Category = Sync, meta = (FoldProperty))
	TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;

	// When enabled, acting as the leader, and using marker-based sync, this asset player will not sync to the previous leader's sync position when joining a sync group and before becoming the leader but instead force everyone else to match its position.
	// 启用后，充当领导者并使用基于标记的同步，该资产播放器在加入同步组时和成为领导者之前不会同步到前一个领导者的同步位置，而是强制其他所有人匹配其位置。
	UPROPERTY(EditAnywhere, Category = Sync, meta = (FoldProperty, EditCondition = "GroupRole != EAnimGroupRole::TransitionFollower && GroupRole != EAnimGroupRole::AlwaysFollower", EditConditionHides))
	bool bOverridePositionWhenJoiningSyncGroupAsLeader = false;
	
	// How this node will synchronize with other animations. Note that this determines how the output
	// 该节点如何与其他动画同步。请注意，这决定了输出的方式
	// of this node is used for synchronization, not of animations contained by it.
	// 该节点的属性用于同步，而不是其包含的动画。
	UPROPERTY(EditAnywhere, Category = Sync, meta = (FoldProperty))
	EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;

	// If true, "Relevant anim" nodes that look for the highest weighted animation in a state will ignore this node
	// 如果为 true，则在某个状态中查找权重最高的动画的“相关动画”节点将忽略此节点
	UPROPERTY(EditAnywhere, Category = Relevancy, meta = (FoldProperty, PinHiddenByDefault))
	bool bIgnoreForRelevancyTest = false;

	// The X coordinate to sample in the blendspace
	// 在混合空间中采样的 X 坐标
	UPROPERTY(EditAnywhere, Category = Coordinates, meta = (PinShownByDefault, FoldProperty))
	float X = 0.0f;

	// The Y coordinate to sample in the blendspace
	// 在混合空间中采样的 Y 坐标
	UPROPERTY(EditAnywhere, Category = Coordinates, meta = (PinShownByDefault, FoldProperty))
	float Y = 0.0f;

	// The play rate multiplier. Can be negative, which will cause the animation to play in reverse.
	// 播放率乘数。可以为负数，这将导致动画反向播放。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (DefaultValue = "1.0", PinHiddenByDefault, FoldProperty))
	float PlayRate = 1.0f;

	// Should the animation loop back to the start when it reaches the end?
	// 当动画到达结束时是否应该循环回到开始处？
	UPROPERTY(EditAnywhere, Category = Settings, meta = (DefaultValue = "true", PinHiddenByDefault, FoldProperty))
	bool bLoop = true;

	// Whether we should reset the current play time when the blend space changes
	// 当混合空间改变时我们是否应该重置当前的播放时间
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, FoldProperty))
	bool bResetPlayTimeWhenBlendSpaceChanges = true;

	// The start position in [0, 1] to use when initializing. When looping, play will still jump back to the beginning when reaching the end.
	// 初始化时使用 [0, 1] 中的起始位置。循环时，播放到达结束时仍会跳回开头。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (DefaultValue = "0.f", PinHiddenByDefault, FoldProperty))
	float StartPosition = 0.0f;
#endif

	// The blendspace asset to play
	// 要播放的混合空间资源
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	TObjectPtr<UBlendSpace> BlendSpace = nullptr;

public:

	// FAnimNode_AssetPlayerBase interface
	// FAnimNode_AssetPlayerBase接口
	ANIMGRAPHRUNTIME_API virtual FName GetGroupName() const override;
	ANIMGRAPHRUNTIME_API virtual EAnimGroupRole::Type GetGroupRole() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetOverridePositionWhenJoiningSyncGroupAsLeader() const override;
	ANIMGRAPHRUNTIME_API virtual EAnimSyncMethod GetGroupMethod() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetIgnoreForRelevancyTest() const override;
	ANIMGRAPHRUNTIME_API virtual bool IsLooping() const override;
	ANIMGRAPHRUNTIME_API virtual bool SetGroupName(FName InGroupName) override;
	ANIMGRAPHRUNTIME_API virtual bool SetGroupRole(EAnimGroupRole::Type InRole) override;
	ANIMGRAPHRUNTIME_API virtual bool SetOverridePositionWhenJoiningSyncGroupAsLeader(bool InOverridePositionWhenJoiningSyncGroupAsLeader) override;;
	ANIMGRAPHRUNTIME_API virtual bool SetGroupMethod(EAnimSyncMethod InMethod) override;
	ANIMGRAPHRUNTIME_API virtual bool SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest) override;
	// End of FAnimNode_AssetPlayerBase interface
	// FAnimNode_AssetPlayerBase接口结束

	// FAnimNode_BlendSpacePlayerBase interface
	// FAnimNode_BlendSpacePlayerBase接口
	ANIMGRAPHRUNTIME_API virtual UBlendSpace* GetBlendSpace() const override;
	ANIMGRAPHRUNTIME_API virtual FVector GetPosition() const override;
	ANIMGRAPHRUNTIME_API virtual float GetStartPosition() const override;
	ANIMGRAPHRUNTIME_API virtual float GetPlayRate() const override;
	ANIMGRAPHRUNTIME_API virtual bool ShouldResetPlayTimeWhenBlendSpaceChanges() const override;
	ANIMGRAPHRUNTIME_API virtual bool SetResetPlayTimeWhenBlendSpaceChanges(bool bReset) override;
	ANIMGRAPHRUNTIME_API virtual bool SetBlendSpace(UBlendSpace* InBlendSpace) override;
	ANIMGRAPHRUNTIME_API virtual bool SetPosition(FVector InPosition) override;
	ANIMGRAPHRUNTIME_API virtual bool SetPlayRate(float InPlayRate) override;
	ANIMGRAPHRUNTIME_API virtual bool SetLoop(bool bInLoop) override;
	// End of FAnimNode_BlendSpacePlayerBase interface
	// FAnimNode_BlendSpacePlayerBase接口结束
};

//@TODO: Comment
//@TODO：评论
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendSpacePlayer_Standalone : public FAnimNode_BlendSpacePlayerBase
{
	GENERATED_BODY()

private:

	// The group name that we synchronize with (NAME_None if it is not part of any group). Note that
	// 我们与之同步的组名称（如果不属于任何组，则为 NAME_None）。注意
	// this is the name of the group used to sync the output of this node - it will not force
	// 这是用于同步该节点输出的组的名称 - 它不会强制
	// syncing of animations contained by it. Animations inside this Blend Space have their own
	// 同步其中包含的动画。此混合空间内的动画有自己的
	// options for syncing.
	// 同步选项。
	UPROPERTY(EditAnywhere, Category = Sync)
	FName GroupName = NAME_None;

	// The role this Blend Space can assume within the group (ignored if GroupName is not set). Note
	// 此混合空间在组内可以承担的角色（如果未设置 GroupName，则忽略）。笔记
	// that this is the role of the output of this node, not of animations contained by it.
	// 这是该节点输出的作用，而不是它包含的动画的作用。
	UPROPERTY(EditAnywhere, Category = Sync)
	TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;

	// When enabled, acting as the leader, and using marker-based sync, this asset player will not sync to the previous leader's sync position when joining a sync group and before becoming the leader but instead force everyone else to match its position.
	// 启用后，充当领导者并使用基于标记的同步，该资产播放器在加入同步组时和成为领导者之前不会同步到前一个领导者的同步位置，而是强制其他所有人匹配其位置。
	UPROPERTY(EditAnywhere, Category = Sync, meta = (EditCondition = "GroupRole != EAnimGroupRole::TransitionFollower && GroupRole != EAnimGroupRole::AlwaysFollower", EditConditionHides))
	bool bOverridePositionWhenJoiningSyncGroupAsLeader = false;
	
	// How this asset will synchronize with other assets. Note that this determines how the output
	// 该资产如何与其他资产同步。请注意，这决定了输出的方式
	// of this node is used for synchronization, not of animations contained by it.
	// 该节点的属性用于同步，而不是其包含的动画。
	UPROPERTY(EditAnywhere, Category = Sync)
	EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;

	// If true, "Relevant anim" nodes that look for the highest weighted animation in a state will ignore this node
	// 如果为 true，则在某个状态中查找权重最高的动画的“相关动画”节点将忽略此节点
	UPROPERTY(EditAnywhere, Category = Relevancy, meta = (PinHiddenByDefault))
	bool bIgnoreForRelevancyTest = false;

	// The X coordinate to sample in the blendspace
	// 在混合空间中采样的 X 坐标
	UPROPERTY(EditAnywhere, Category = Coordinates, meta = (PinShownByDefault))
	float X = 0.0f;

	// The Y coordinate to sample in the blendspace
	// 在混合空间中采样的 Y 坐标
	UPROPERTY(EditAnywhere, Category = Coordinates, meta = (PinShownByDefault))
	float Y = 0.0f;

	// The play rate multiplier. Can be negative, which will cause the animation to play in reverse.
	// 播放率乘数。可以为负数，这将导致动画反向播放。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (DefaultValue = "1.0", PinHiddenByDefault))
	float PlayRate = 1.0f;

	// Should the animation loop back to the start when it reaches the end?
	// 当动画到达结束时是否应该循环回到开始处？
	UPROPERTY(EditAnywhere, Category = Settings, meta = (DefaultValue = "true", PinHiddenByDefault))
	bool bLoop = true;

	// Whether we should reset the current play time when the blend space changes
	// 当混合空间改变时我们是否应该重置当前的播放时间
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	bool bResetPlayTimeWhenBlendSpaceChanges = true;

	// The start position in [0, 1] to use when initializing. When looping, play will still jump back to the beginning when reaching the end.
	// 初始化时使用 [0, 1] 中的起始位置。循环时，播放到达结束时仍会跳回开头。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (DefaultValue = "0.f", PinHiddenByDefault))
	float StartPosition = 0.0f;

	// The blendspace asset to play
	// 要播放的混合空间资源
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	TObjectPtr<UBlendSpace> BlendSpace = nullptr;

public:

	// FAnimNode_AssetPlayerBase interface
	// FAnimNode_AssetPlayerBase接口
	virtual FName GetGroupName() const override { return GroupName; }
	virtual EAnimGroupRole::Type GetGroupRole() const override { return GroupRole; }
	virtual EAnimSyncMethod GetGroupMethod() const override { return Method; }
	virtual bool GetOverridePositionWhenJoiningSyncGroupAsLeader() const override { return bOverridePositionWhenJoiningSyncGroupAsLeader; };
	virtual bool GetIgnoreForRelevancyTest() const override { return bIgnoreForRelevancyTest; }
	virtual bool IsLooping() const override { return bLoop; }
	virtual bool SetGroupName(FName InGroupName) override { GroupName = InGroupName; return true; }
	virtual bool SetGroupRole(EAnimGroupRole::Type InRole) override { GroupRole = InRole; return true; }
	virtual bool SetGroupMethod(EAnimSyncMethod InMethod) override { Method = InMethod; return true; }
	virtual bool SetOverridePositionWhenJoiningSyncGroupAsLeader(bool InOverridePositionWhenJoiningSyncGroupAsLeader) override { bOverridePositionWhenJoiningSyncGroupAsLeader = InOverridePositionWhenJoiningSyncGroupAsLeader; return true; };
	virtual bool SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest) override { bIgnoreForRelevancyTest = bInIgnoreForRelevancyTest; return true; }
	// End of FAnimNode_AssetPlayerBase interface
	// FAnimNode_AssetPlayerBase接口结束

	// FAnimNode_BlendSpacePlayerBase interface
	// FAnimNode_BlendSpacePlayerBase接口
	virtual UBlendSpace* GetBlendSpace() const override { return BlendSpace; }
	virtual FVector GetPosition() const override { return FVector(X, Y, 0.0); }
	virtual float GetStartPosition() const override { return StartPosition; }
	virtual float GetPlayRate() const override { return PlayRate; }
	virtual bool ShouldResetPlayTimeWhenBlendSpaceChanges() const override { return bResetPlayTimeWhenBlendSpaceChanges; }
	virtual bool SetResetPlayTimeWhenBlendSpaceChanges(bool bReset) override { bResetPlayTimeWhenBlendSpaceChanges = bReset; return true; }
	virtual bool SetBlendSpace(UBlendSpace* InBlendSpace) override { BlendSpace = InBlendSpace; return true; }
	virtual bool SetPosition(FVector InPosition) override { X = static_cast<float>(InPosition[0]); Y = static_cast<float>(InPosition[1]); return true; }
	virtual bool SetPlayRate(float InPlayRate) override { PlayRate = InPlayRate; return true; }
	virtual bool SetLoop(bool bInLoop) override { bLoop = bInLoop; return true; }
	// End of FAnimNode_BlendSpacePlayerBase interface
	// FAnimNode_BlendSpacePlayerBase接口结束
};
