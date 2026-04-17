// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_Root.generated.h"

// Root node of an animation tree (sink)
// 动画树的根节点（接收器）
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_Root : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink Result;

#if WITH_EDITORONLY_DATA
protected:
	/** The name of this root node, used to identify the output of this graph. Filled in by the compiler, propagated from the parent graph. */
	/** 该根节点的名称，用于标识该图的输出。由编译器填充，从父图传播。 */
	UPROPERTY(meta=(FoldProperty, BlueprintCompilerGeneratedDefaults))
	FName Name;

	/** The group of this root node, used to group this output with others when used in a layer. */
	/** 该根节点的组，用于在层中使用时将此输出与其他输出分组。 */
	UPROPERTY(meta=(FoldProperty))
	FName LayerGroup = DefaultSharedGroup;

	UPROPERTY(meta= (DeprecatedProperty, DeprecationMessage = "Please, use LayerGroup"))
	FName Group_DEPRECATED;
#endif

public:	
	ENGINE_API FAnimNode_Root();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ENGINE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ENGINE_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ENGINE_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ENGINE_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ENGINE_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

#if WITH_EDITORONLY_DATA
	/** Set the name of this root node, used to identify the output of this graph */
	/** 设置这个根节点的名称，用于标识这个图的输出 */
	void SetName(FName InName) { Name = InName; }

	/** Set the group of this root node, used to group this output with others when used in a layer. */
	/** 设置此根节点的组，用于在层中使用时将此输出与其他输出分组。 */
	void SetGroup(FName InGroup) { LayerGroup = InGroup; }

#endif

	/** Get the name of this root node, used to identify the output of this graph */
	/** 获取这个根节点的名称，用于标识这个图的输出 */
	ENGINE_API FName GetName() const;

	/** Get the group of this root node, used to group this output with others when used in a layer. */
	/** 获取此根节点的组，用于在层中使用时将此输出与其他输出分组。 */
	ENGINE_API FName GetGroup() const;

#if WITH_EDITORONLY_DATA
	/** Default shared group used by layers. */
	/** 图层使用的默认共享组。 */
	ENGINE_API static FName DefaultSharedGroup;
#endif

	friend class UAnimGraphNode_Root;
};
