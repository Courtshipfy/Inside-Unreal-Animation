// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimExecutionContext.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "Animation/AnimStateMachineTypes.h"
#include "AnimationStateMachineLibrary.generated.h"

struct FAnimNode_StateMachine;
struct FAnimNode_StateResult;

USTRUCT(BlueprintType, DisplayName = "Animation State Reference")
struct FAnimationStateResultReference : public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_StateResult FInternalNodeType;
};

USTRUCT(BlueprintType, DisplayName = "Animation State Machine")
struct FAnimationStateMachineReference : public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_StateMachine FInternalNodeType;
};

// Exposes operations to be performed on anim state machine node contexts
// 公开要在动画状态机节点上下文上执行的操作
UCLASS(Experimental, MinimalAPI)
class UAnimationStateMachineLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 从动画节点引用获取动画状态引用 */
	/** 从动画节点引用获取动画状态引用 */
	/** Get an anim state reference from an anim node reference */
	/** 从动画节点引用获取动画状态引用 */
	UFUNCTION(BlueprintCallable, Category = "State Machine", meta=(BlueprintThreadSafe, DisplayName = "Convert to Animation State", ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API void ConvertToAnimationStateResult(const FAnimNodeReference& Node, FAnimationStateResultReference& AnimationState, EAnimNodeReferenceConversionResult& Result);
	/** 从动画节点引用获取动画状态引用（纯） */
	/** 从动画节点引用获取动画状态引用（纯） */

	/** Get an anim state reference from an anim node reference (pure) */
	/** 从动画节点引用获取动画状态引用（纯） */
	UFUNCTION(BlueprintPure, Category = "State Machine", meta=(BlueprintThreadSafe, DisplayName = "Convert to Animation State"))
	static void ConvertToAnimationStateResultPure(const FAnimNodeReference& Node, FAnimationStateResultReference& AnimationState, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		ConvertToAnimationStateResult(Node, AnimationState, ConversionResult);
	/** 从动画节点引用获取动画状态机 */
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);	
	/** 从动画节点引用获取动画状态机 */
	}

	/** Get an anim state machine from an anim node reference */
	/** 从动画节点引用获取动画状态机（纯） */
	/** 从动画节点引用获取动画状态机 */
	UFUNCTION(BlueprintCallable, Category = "State Machine", meta=(BlueprintThreadSafe, DisplayName = "Convert to Animation State Machine", ExpandEnumAsExecs = "Result"))
	/** 从动画节点引用获取动画状态机（纯） */
	static ANIMGRAPHRUNTIME_API void ConvertToAnimationStateMachine(const FAnimNodeReference& Node, FAnimationStateMachineReference& AnimationState, EAnimNodeReferenceConversionResult& Result);


	/** Get an anim state machine from an anim node reference (pure) */
	/** 从动画节点引用获取动画状态机（纯） */
	UFUNCTION(BlueprintPure, Category = "State Machine", meta = (BlueprintThreadSafe, DisplayName = "Convert to Animation State Machine"))
	/** 返回节点所属状态是否正在混合 */
	static void ConvertToAnimationStateMachinePure(const FAnimNodeReference& Node, FAnimationStateMachineReference& AnimationState, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
	/** 返回节点所属状态是否正在混合 */
	/** 返回节点所属状态是否正在混合 */
		ConvertToAnimationStateMachine(Node, AnimationState, ConversionResult);
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}
	
	/** 返回节点所属状态是否正在混合 */
	/** Returns whether the state the node belongs to is blending in */
	/** 返回节点所属状态是否正在混合 */
	UFUNCTION(BlueprintPure, Category = "State Machine", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API bool IsStateBlendingIn(const FAnimUpdateContext& UpdateContext, const FAnimationStateResultReference& Node);

	/** 返回此状态机当前状态的名称 */
	/** Returns whether the state the node belongs to is blending out */
	/** 返回节点所属状态是否正在混合 */
	UFUNCTION(BlueprintPure, Category = "State Machine", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API bool IsStateBlendingOut(const FAnimUpdateContext& UpdateContext, const FAnimationStateResultReference& Node);
	/** 返回状态最相关的资产播放器的剩余动画时间 */

	/** 返回此状态机当前状态的名称 */
	/** Manually set the current state of the state machine
		NOTE: Custom blend type is not supported */
	/** 返回剩余动画时间，作为状态最相关资产播放器的持续时间的一部分 */
	UFUNCTION(BlueprintCallable, Category = "State Machine", meta=(BlueprintThreadSafe, AdvancedDisplay = "4"))
	static ANIMGRAPHRUNTIME_API void SetState(const FAnimUpdateContext& UpdateContext, const FAnimationStateMachineReference& Node, FName TargetState, float Duration
	/** 返回状态最相关的资产播放器的剩余动画时间 */
		, TEnumAsByte<ETransitionLogicType::Type> BlendType, UBlendProfile* BlendProfile, EAlphaBlendOption AlphaBlendOption, UCurveFloat* CustomBlendCurve);

	/** Returns the name of the current state of this state machine */
	/** 返回此状态机当前状态的名称 */
	/** 返回剩余动画时间，作为状态最相关资产播放器的持续时间的一部分 */
	UFUNCTION(BlueprintPure, Category = "State Machine", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FName GetState(const FAnimUpdateContext& UpdateContext, const FAnimationStateMachineReference& Node);

	/** Returns the remaining animation time of the state's most relevant asset player */
	/** 返回状态最相关的资产播放器的剩余动画时间 */
	UFUNCTION(BlueprintPure, Category = "State Machine", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetRelevantAnimTimeRemaining(const FAnimUpdateContext& UpdateContext, const FAnimationStateResultReference& Node);

	/** Returns the remaining animation time as a fraction of the duration for the state's most relevant asset player */
	/** 返回剩余动画时间，作为状态最相关资产播放器的持续时间的一部分 */
	UFUNCTION(BlueprintPure, Category = "State Machine", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetRelevantAnimTimeRemainingFraction(const FAnimUpdateContext& UpdateContext, const FAnimationStateResultReference& Node);
};
