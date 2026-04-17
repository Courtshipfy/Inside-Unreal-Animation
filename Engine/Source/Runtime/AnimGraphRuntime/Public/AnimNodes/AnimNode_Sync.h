// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSync.h"
#include "AnimNode_Sync.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_Sync : public FAnimNode_Base
{
	GENERATED_BODY()

	friend class UAnimGraphNode_Sync;

public:
	// Get the group name that we synchronize with
	// 获取我们同步的组名
	FName GetGroupName() const { return GroupName; }

private:
	UPROPERTY(EditAnywhere, Category = Links)
	FPoseLink Source;

	// The group name that we synchronize with (NAME_None if it is not part of any group). 
	// 我们与之同步的组名称（如果不属于任何组，则为 NAME_None）。
	UPROPERTY(EditAnywhere, Category = Settings)
	FName GroupName;

	// The role this animation can assume within the group (ignored if GroupName is not set)
	// 该动画在组内可以扮演的角色（如果未设置 GroupName，则忽略）
	UPROPERTY(EditAnywhere, Category = Settings)
	TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;

private:
	// FAnimNode_Base
	// FAnimNode_Base
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
};
