// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNodeFunctionRef.h"
#include "AnimNode_CallFunction.generated.h"

// When to call the function during the execution of the animation graph
// 动画图执行过程中何时调用该函数
UENUM()
enum class EAnimFunctionCallSite
{
	// Called when the node is initialized - i.e. it becomes weighted/relevant in the graph (before child nodes are initialized)
	// 在节点初始化时调用 - 即它在图中变得加权/相关（在初始化子节点之前）
	OnInitialize,
	
	// Called when the node is updated (before child nodes are updated)
	// 节点更新时调用（子节点更新之前）
	OnUpdate,

	// Called when the node is updated for the first time with a valid weight
	// 当节点第一次使用有效权重更新时调用
	OnBecomeRelevant,
	
	// Called when the node is evaluated (before child nodes are evaluated)
	// 在计算节点时调用（在计算子节点之前）
    OnEvaluate,

	// Called when the node is initialized - i.e. it becomes weighted/relevant in the graph (after child nodes are initialized)
	// 在节点初始化时调用 - 即它在图中变得加权/相关（子节点初始化后）
    OnInitializePostRecursion UMETA(DisplayName = "On Initialize (Post Recursion)"),
	
	// Called when the node is updated (after child nodes are updated)
	// 节点更新时调用（子节点更新后）
	OnUpdatePostRecursion UMETA(DisplayName = "On Update (Post Recursion)"),

	// Called when the node is updated for the first time with a valid weight (after child nodes are updated)
	// 当节点第一次使用有效权重更新时调用（子节点更新后）
    OnBecomeRelevantPostRecursion UMETA(DisplayName = "On Become Relevant (Post Recursion)"),

	// Called when the node is evaluated (after child nodes are evaluated)
	// 在计算节点时调用（在计算子节点之后）
	OnEvaluatePostRecursion UMETA(DisplayName = "On Evaluate (Post Recursion)"),

	// Called when the node is updated, was at full weight and beings to blend out. Called before child nodes are
	// 当节点更新时调用，处于全权重状态并且正在混合。在子节点之前调用
	// updated
	// 已更新
	OnStartedBlendingOut,

	// Called when the node is updated, was at zero weight and beings to blend in. Called before child nodes are updated
	// 当节点更新、权重为零并且要混合时调用。在更新子节点之前调用
    OnStartedBlendingIn,

	// Called when the node is updated, was at non-zero weight and finishes blending out. Called before child nodes are
	// 当节点更新、权重非零并完成混合时调用。在子节点之前调用
	// updated (note that this is necessarily not called within the graph update but from outside)
	// 更新（请注意，这不一定是在图形更新内调用，而是从外部调用）
	// @TODO: Not currently supported, needs subsystem support!
	// @TODO：当前不支持，需要子系统支持！
    OnFinishedBlendingOut UMETA(Hidden),

	// Called when the node is updated, was at non-zero weight and becomes full weight. Called before child nodes are
	// 当节点更新、权重非零并变为满权重时调用。在子节点之前调用
    // updated
    // 已更新
    OnFinishedBlendingIn,
};

/** Calls specified user-defined events/functions during anim graph execution */
/** 在动画图执行期间调用指定的用户定义事件/函数 */
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_CallFunction : public FAnimNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Links)
	FPoseLink Source;

#if WITH_EDITORONLY_DATA
	// Function to call
	// 要调用的函数
	UPROPERTY(meta=(FoldProperty))
	FAnimNodeFunctionRef Function;
#endif
	
	// Counter used to determine relevancy
	// 用于确定相关性的计数器
	FGraphTraversalCounter Counter;

	// Weight to determine blend-related call sites
	// 确定与混合相关的调用站点的权重
	float CurrentWeight = 0.0f;
	
	//  When to call the function during the execution of the animation graph
	//  动画图执行过程中何时调用该函数
	UPROPERTY(EditAnywhere, Category="Function")
	EAnimFunctionCallSite CallSite = EAnimFunctionCallSite::OnUpdate;
		
	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// Calls the function we hold if the callsite matches the one we have set
	// 如果调用点与我们设置的调用点匹配，则调用我们持有的函数
	ANIMGRAPHRUNTIME_API void CallFunctionFromCallSite(EAnimFunctionCallSite InCallSite, const FAnimationBaseContext& InContext) const;

	// Get the function held on this node
	// 获取该节点持有的函数
	ANIMGRAPHRUNTIME_API const FAnimNodeFunctionRef& GetFunction() const;
};
