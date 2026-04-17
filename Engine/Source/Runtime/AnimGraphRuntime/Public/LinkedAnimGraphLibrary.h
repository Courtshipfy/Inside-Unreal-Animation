// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "LinkedAnimGraphLibrary.generated.h"

struct FAnimNode_LinkedAnimGraph;

USTRUCT(BlueprintType)
struct FLinkedAnimGraphReference : public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_LinkedAnimGraph FInternalNodeType;
};

// Exposes operations to be performed on anim node contexts
// 公开要在动画节点上下文上执行的操作
UCLASS(MinimalAPI)
class ULinkedAnimGraphLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get a linked anim graph reference from an anim node reference */
	/** 从动画节点引用获取链接的动画图引用 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Linked Anim Graphs", meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FLinkedAnimGraphReference ConvertToLinkedAnimGraph(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);

	/** Get a linked anim graph reference from an anim node reference (pure) */
	/** 从动画节点引用获取链接的动画图引用（纯） */
	UFUNCTION(BlueprintPure, Category = "Animation|Linked Anim Graphs", meta=(BlueprintThreadSafe, DisplayName = "Convert to Linked Anim Graph"))
	static void ConvertToLinkedAnimGraphPure(const FAnimNodeReference& Node, FLinkedAnimGraphReference& LinkedAnimGraph, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		LinkedAnimGraph = ConvertToLinkedAnimGraph(Node, ConversionResult);
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}
	
	/** Returns whether the node hosts an instance (e.g. linked anim graph or layer) */
	/** 返回节点是否托管实例（例如链接的动画图或图层） */
	UFUNCTION(BlueprintPure, Category = "Animation|Linked Anim Graphs", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API bool HasLinkedAnimInstance(const FLinkedAnimGraphReference& Node);

	/** Get the linked instance is hosted by this node. If the node does not host an instance then HasLinkedAnimInstance will return false */
	/** 获取此节点托管的链接实例。如果节点没有托管实例，则 HasLinkedAnimInstance 将返回 false */
	UFUNCTION(BlueprintPure, Category = "Animation|Linked Anim Graphs", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API UAnimInstance* GetLinkedAnimInstance(const FLinkedAnimGraphReference& Node);
};
