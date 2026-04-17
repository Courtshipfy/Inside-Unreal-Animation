// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNode_AssetPlayerBase.h"
#include "Animation/InputScaleBias.h"
#include "Animation/AnimSequenceBase.h"
#include "AnimNode_SequencePlayer.generated.h"


// Sequence player node. Not instantiated directly, use FAnimNode_SequencePlayer or FAnimNode_SequencePlayer_Standalone
// 序列播放器节点。不直接实例化，使用FAnimNode_SequencePlayer或FAnimNode_SequencePlayer_Standalone
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_SequencePlayerBase : public FAnimNode_AssetPlayerBase
{
	GENERATED_BODY()

protected:
	// Corresponding state for PlayRateScaleBiasClampConstants
 // PlayRateScaleBiasClampConstants 的对应状态
	UPROPERTY()
	FInputScaleBiasClampState PlayRateScaleBiasClampState;

public:
	// FAnimNode_AssetPlayerBase interface
 // FAnimNode_AssetPlayerBase接口
	ENGINE_API virtual float GetCurrentAssetTime() const override;
	ENGINE_API virtual float GetCurrentAssetTimePlayRateAdjusted() const override;
	ENGINE_API virtual float GetCurrentAssetLength() const override;
	virtual UAnimationAsset* GetAnimAsset() const override { return GetSequence(); }
	// End of FAnimNode_AssetPlayerBase interface
 // FAnimNode_AssetPlayerBase接口结束

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ENGINE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ENGINE_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ENGINE_API virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	ENGINE_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ENGINE_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

	ENGINE_API float GetTimeFromEnd(float CurrentNodeTime) const;
	ENGINE_API float GetEffectiveStartPosition(const FAnimationBaseContext& Context) const;

	// The animation sequence asset to play
 // 要播放的动画序列资源
	virtual UAnimSequenceBase* GetSequence() const { return nullptr; }

	// Should the animation loop back to the start when it reaches the end?
 // 当动画到达结束时是否应该循环回到开始处？
	UE_DEPRECATED(5.3, "Please use IsLooping instead.")
	virtual bool GetLoopAnimation() const final { return IsLooping(); }

protected:
	// Set the animation sequence asset to play
 // 设置要播放的动画序列资源
	virtual bool SetSequence(UAnimSequenceBase* InSequence) { return false; }

	// Set the animation to continue looping when it reaches the end
 // 设置动画到达结束时继续循环
	virtual bool SetLoopAnimation(bool bInLoopAnimation) { return false; }
	
	// The Basis in which the PlayRate is expressed in. This is used to rescale PlayRate inputs.
 // 表示 PlayRate 的基础。这用于重新缩放 PlayRate 输入。
	// For example a Basis of 100 means that the PlayRate input will be divided by 100.
 // 例如，Basis 为 100 意味着 PlayRate 输入将除以 100。
	virtual float GetPlayRateBasis() const { return 1.0f; }

	// The play rate multiplier. Can be negative, which will cause the animation to play in reverse.
 // 播放率乘数。可以为负数，这将导致动画反向播放。
	virtual float GetPlayRate() const { return 1.0f; }
	
	// Additional scaling, offsetting and clamping of PlayRate input.
 // PlayRate 输入的附加缩放、偏移和钳位。
	// Performed after PlayRateBasis.
 // 在 PlayRateBasis 之后执行。
	virtual const FInputScaleBiasClampConstants& GetPlayRateScaleBiasClampConstants() const { static FInputScaleBiasClampConstants Dummy; return Dummy; }

	// The start position [range: 0 - sequence length] to use when initializing. When looping, play will still jump back to the beginning when reaching the end.
 // 初始化时使用的起始位置[范围：0 - 序列长度]。循环时，播放到达结束时仍会跳回开头。
	virtual float GetStartPosition() const { return 0.0f; }

	// Use pose matching to choose the start position. Requires PoseSearch plugin.
 // 使用姿势匹配来选择起始位置。需要 PoseSearch 插件。
	virtual bool GetStartFromMatchingPose() const { return false; }

	// Set the start time of this node.
 // 设置该节点的开始时间。
	// @return true if the value was set (it is dynamic), or false if it could not (it is not dynamic or pin exposed)
 // @return true 如果该值已设置（它是动态的），或者 false 如果它不能设置（它不是动态的或引脚暴露的）
	virtual bool SetStartPosition(float InStartPosition) { return false; }

	// Set the play rate of this node.
 // 设置该节点的播放速率。
	// @return true if the value was set (it is dynamic), or false if it could not (it is not dynamic or pin exposed)	
 // @return true 如果该值已设置（它是动态的），或者 false 如果它不能设置（它不是动态的或引脚暴露的）
	virtual bool SetPlayRate(float InPlayRate) { return false; }
};

// Sequence player node that can be used with constant folding
// 可用于常量折叠的序列播放器节点
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_SequencePlayer : public FAnimNode_SequencePlayerBase
{
	GENERATED_BODY()

protected:
	friend class UAnimGraphNode_SequencePlayer;

#if WITH_EDITORONLY_DATA
	// The group name that we synchronize with (NAME_None if it is not part of any group). 
 // 我们与之同步的组名称（如果不属于任何组，则为 NAME_None）。
	UPROPERTY(EditAnywhere, Category=Sync, meta=(FoldProperty))
	FName GroupName = NAME_None;

	// The role this node can assume within the group (ignored if GroupName is not set)
 // 该节点在组内可以承担的角色（如果未设置 GroupName，则忽略）
	UPROPERTY(EditAnywhere, Category=Sync, meta=(FoldProperty))
	TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;

	// When enabled, acting as the leader, and using marker-based sync, this asset player will not sync to the previous leader's sync position when joining a sync group and before becoming the leader but instead force everyone else to match its position.
 // 启用后，充当领导者并使用基于标记的同步，该资产播放器在加入同步组时和成为领导者之前不会同步到前一个领导者的同步位置，而是强制其他所有人匹配其位置。
	UPROPERTY(EditAnywhere, Category = Sync, meta = (FoldProperty, EditCondition = "GroupRole != EAnimGroupRole::TransitionFollower && GroupRole != EAnimGroupRole::AlwaysFollower", EditConditionHides))
	bool bOverridePositionWhenJoiningSyncGroupAsLeader = false;
	
	// How this node will synchronize with other animations.
 // 该节点如何与其他动画同步。
	UPROPERTY(EditAnywhere, Category=Sync, meta=(FoldProperty))
	EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;

	// If true, "Relevant anim" nodes that look for the highest weighted animation in a state will ignore this node
 // 如果为 true，则在某个状态中查找权重最高的动画的“相关动画”节点将忽略此节点
	UPROPERTY(EditAnywhere, Category=Relevancy, meta=(FoldProperty, PinHiddenByDefault))
	bool bIgnoreForRelevancyTest = false;
#endif

	// The animation sequence asset to play
 // 要播放的动画序列资源
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, DisallowedClasses="/Script/Engine.AnimMontage"))
	TObjectPtr<UAnimSequenceBase> Sequence = nullptr;

#if WITH_EDITORONLY_DATA
	// The Basis in which the PlayRate is expressed in. This is used to rescale PlayRate inputs.
 // 表示 PlayRate 的基础。这用于重新缩放 PlayRate 输入。
	// For example a Basis of 100 means that the PlayRate input will be divided by 100.
 // 例如，Basis 为 100 意味着 PlayRate 输入将除以 100。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, FoldProperty))
	float PlayRateBasis = 1.0f;

	// The play rate multiplier. Can be negative, which will cause the animation to play in reverse.
 // 播放率乘数。可以为负数，这将导致动画反向播放。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, FoldProperty))
	float PlayRate = 1.0f;
	
	// Additional scaling, offsetting and clamping of PlayRate input.
 // PlayRate 输入的附加缩放、偏移和钳位。
	// Performed after PlayRateBasis.
 // 在 PlayRateBasis 之后执行。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (DisplayName="PlayRateScaleBiasClamp", FoldProperty))
	FInputScaleBiasClampConstants PlayRateScaleBiasClampConstants;

	UPROPERTY()
	FInputScaleBiasClamp PlayRateScaleBiasClamp_DEPRECATED;

	// The start position between 0 and the length of the sequence to use when initializing. When looping, play will still jump back to the beginning when reaching the end.
 // 初始化时使用的介于 0 和序列长度之间的起始位置。循环时，播放到达结束时仍会跳回开头。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, FoldProperty))
	float StartPosition = 0.0f;

	// Should the animation loop back to the start when it reaches the end?
 // 当动画到达结束时是否应该循环回到开始处？
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, FoldProperty))
	bool bLoopAnimation = true;

	// Use pose matching to choose the start position. Requires PoseSearch plugin.
 // 使用姿势匹配来选择起始位置。需要 PoseSearch 插件。
	UPROPERTY(EditAnywhere, Category = PoseMatching, meta = (PinHiddenByDefault, FoldProperty))
	bool bStartFromMatchingPose = false;
#endif

public:
	// FAnimNode_SequencePlayerBase interface
 // FAnimNode_SequencePlayerBase接口
	ENGINE_API virtual bool SetSequence(UAnimSequenceBase* InSequence) override;
	ENGINE_API virtual bool SetLoopAnimation(bool bInLoopAnimation) override;
	ENGINE_API virtual UAnimSequenceBase* GetSequence() const override;
	ENGINE_API virtual float GetPlayRateBasis() const override;
	ENGINE_API virtual float GetPlayRate() const override;
	ENGINE_API virtual const FInputScaleBiasClampConstants& GetPlayRateScaleBiasClampConstants() const override;
	ENGINE_API virtual float GetStartPosition() const override;
	ENGINE_API virtual bool GetStartFromMatchingPose() const override;
	ENGINE_API virtual bool SetStartPosition(float InStartPosition) override;
	ENGINE_API virtual bool SetPlayRate(float InPlayRate) override;

	// FAnimNode_AssetPlayerBase interface
 // FAnimNode_AssetPlayerBase接口
	ENGINE_API virtual FName GetGroupName() const override;
	ENGINE_API virtual EAnimGroupRole::Type GetGroupRole() const override;
	ENGINE_API virtual bool GetOverridePositionWhenJoiningSyncGroupAsLeader() const override;
	ENGINE_API virtual EAnimSyncMethod GetGroupMethod() const override;
	ENGINE_API virtual bool IsLooping() const override;
	ENGINE_API virtual bool GetIgnoreForRelevancyTest() const override;
	ENGINE_API virtual bool SetGroupName(FName InGroupName) override;
	ENGINE_API virtual bool SetGroupRole(EAnimGroupRole::Type InRole) override;
	ENGINE_API virtual bool SetOverridePositionWhenJoiningSyncGroupAsLeader(bool InOverridePositionWhenJoiningSyncGroupAsLeader) override;
	ENGINE_API virtual bool SetGroupMethod(EAnimSyncMethod InMethod) override;
	ENGINE_API virtual bool SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest) override;
};

// Sequence player node that can be used standalone (without constant folding)
// 可以独立使用的序列播放器节点（无需不断折叠）
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_SequencePlayer_Standalone : public FAnimNode_SequencePlayerBase
{	
	GENERATED_BODY()

protected:
	// The group name that we synchronize with (NAME_None if it is not part of any group). 
 // 我们与之同步的组名称（如果不属于任何组，则为 NAME_None）。
	UPROPERTY(EditAnywhere, Category=Sync)
	FName GroupName = NAME_None;

	// The role this node can assume within the group (ignored if GroupName is not set)
 // 该节点在组内可以承担的角色（如果未设置 GroupName，则忽略）
	UPROPERTY(EditAnywhere, Category=Sync)
	TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;

	// When enabled, acting as the leader, and using marker-based sync, this asset player will not sync to the previous leader's sync position when joining a sync group and before becoming the leader but instead force everyone else to match its position.
 // 启用后，充当领导者并使用基于标记的同步，该资产播放器在加入同步组时和成为领导者之前不会同步到前一个领导者的同步位置，而是强制其他所有人匹配其位置。
	UPROPERTY(EditAnywhere, Category = Sync, meta = (EditCondition = "GroupRole != EAnimGroupRole::TransitionFollower && GroupRole != EAnimGroupRole::AlwaysFollower", EditConditionHides))
	bool bOverridePositionWhenJoiningSyncGroupAsLeader = false;
	
	// How this node will synchronize with other animations.
 // 该节点如何与其他动画同步。
	UPROPERTY(EditAnywhere, Category=Sync)
	EAnimSyncMethod Method = EAnimSyncMethod::DoNotSync;

	// If true, "Relevant anim" nodes that look for the highest weighted animation in a state will ignore this node
 // 如果为 true，则在某个状态中查找权重最高的动画的“相关动画”节点将忽略此节点
	UPROPERTY(EditAnywhere, Category=Relevancy, meta=(PinHiddenByDefault))
	bool bIgnoreForRelevancyTest = false;

	// The animation sequence asset to play
 // 要播放的动画序列资源
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, DisallowedClasses="/Script/Engine.AnimMontage"))
	TObjectPtr<UAnimSequenceBase> Sequence = nullptr;

	// The Basis in which the PlayRate is expressed in. This is used to rescale PlayRate inputs.
 // 表示 PlayRate 的基础。这用于重新缩放 PlayRate 输入。
	// For example a Basis of 100 means that the PlayRate input will be divided by 100.
 // 例如，Basis 为 100 意味着 PlayRate 输入将除以 100。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	float PlayRateBasis = 1.0f;

	// The play rate multiplier. Can be negative, which will cause the animation to play in reverse.
 // 播放率乘数。可以为负数，这将导致动画反向播放。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	float PlayRate = 1.0f;
	
	// Additional scaling, offsetting and clamping of PlayRate input.
 // PlayRate 输入的附加缩放、偏移和钳位。
	// Performed after PlayRateBasis.
 // 在 PlayRateBasis 之后执行。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (DisplayName="PlayRateScaleBiasClamp"))
	FInputScaleBiasClampConstants PlayRateScaleBiasClampConstants;

	// The start position between 0 and the length of the sequence to use when initializing. When looping, play will still jump back to the beginning when reaching the end.
 // 初始化时使用的介于 0 和序列长度之间的起始位置。循环时，播放到达结束时仍会跳回开头。
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	float StartPosition = 0.0f;

	// Should the animation loop back to the start when it reaches the end?
 // 当动画到达结束时是否应该循环回到开始处？
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	bool bLoopAnimation = true;

	// Use pose matching to choose the start position. Requires PoseSearch plugin.
 // 使用姿势匹配来选择起始位置。需要 PoseSearch 插件。
	UPROPERTY(EditAnywhere, Category = PoseMatching, meta = (PinHiddenByDefault))
	bool bStartFromMatchingPose = false;

public:
	// FAnimNode_SequencePlayerBase interface
 // FAnimNode_SequencePlayerBase接口
	virtual bool SetSequence(UAnimSequenceBase* InSequence) override { Sequence = InSequence; return true; }
	virtual bool SetLoopAnimation(bool bInLoopAnimation) override { bLoopAnimation = bInLoopAnimation; return true; }
	virtual UAnimSequenceBase* GetSequence() const override { return Sequence; }
	virtual float GetPlayRateBasis() const override { return PlayRateBasis; }
	virtual float GetPlayRate() const override { return PlayRate; }
	virtual const FInputScaleBiasClampConstants& GetPlayRateScaleBiasClampConstants() const override { return PlayRateScaleBiasClampConstants; }
	virtual float GetStartPosition() const override { return StartPosition; }
	virtual bool GetStartFromMatchingPose() const override { return bStartFromMatchingPose; }

	// FAnimNode_AssetPlayerBase interface
 // FAnimNode_AssetPlayerBase接口
	virtual FName GetGroupName() const override { return GroupName; }
	virtual EAnimGroupRole::Type GetGroupRole() const override { return GroupRole; }
	virtual EAnimSyncMethod GetGroupMethod() const override { return Method; }
	virtual bool GetOverridePositionWhenJoiningSyncGroupAsLeader() const override { return bOverridePositionWhenJoiningSyncGroupAsLeader; }
	virtual bool IsLooping() const override { return bLoopAnimation; }
	virtual bool GetIgnoreForRelevancyTest() const override { return bIgnoreForRelevancyTest; }
	virtual bool SetGroupName(FName InGroupName) override { GroupName = InGroupName; return true; }
	virtual bool SetGroupRole(EAnimGroupRole::Type InRole) override { GroupRole = InRole; return true; }
	virtual bool SetGroupMethod(EAnimSyncMethod InMethod) override { Method = InMethod; return true; }
	virtual bool SetOverridePositionWhenJoiningSyncGroupAsLeader(bool InOverridePositionWhenJoiningSyncGroupAsLeader) override { bOverridePositionWhenJoiningSyncGroupAsLeader = InOverridePositionWhenJoiningSyncGroupAsLeader; return true; }	
	virtual bool SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest) override { bIgnoreForRelevancyTest = bInIgnoreForRelevancyTest; return true; }
	virtual bool SetStartPosition(float InStartPosition) override { StartPosition = InStartPosition; return  true; }
	virtual bool SetPlayRate(float InPlayRate) override { PlayRate = InPlayRate; return true; }
};
