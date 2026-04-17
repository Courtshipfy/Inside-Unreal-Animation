// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimInstance.h"
#include "AnimNode_TransitionResult.generated.h"

// Root node of a state machine transition graph
// 状态机转移图的根节点
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_TransitionResult : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Result, meta=(PinShownByDefault))
	bool bCanEnterTransition;

	/** 检查转换时使用的本机委托 */
	/** 检查转换时使用的本机委托 */
	/** Native delegate to use when checking transition */
	/** 检查转换时使用的本机委托 */
	FCanTakeTransition NativeTransitionDelegate;

public:	
	ENGINE_API FAnimNode_TransitionResult();

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ENGINE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ENGINE_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ENGINE_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ENGINE_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ENGINE_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束
};
