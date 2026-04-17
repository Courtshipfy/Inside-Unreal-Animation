// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimExecutionContext.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "MirrorAnimLibrary.generated.h"

struct FAnimNode_MirrorBase;

USTRUCT(BlueprintType)
struct FMirrorAnimNodeReference : public FAnimNodeReference
{
	GENERATED_BODY()
	typedef FAnimNode_MirrorBase FInternalNodeType;
};

/** Exposes operations that can be run on a Mirror node via Anim Node Functions such as "On Become Relevant" and "On Update". */
/** 公开可通过动画节点功能（例如“相关时”和“更新时”）在镜像节点上运行的操作。 */
UCLASS()
class UMirrorAnimLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	
	/** Get a mirror node context from an anim node context */
	/** 从动画节点上下文获取镜像节点上下文 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Mirroring", meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FMirrorAnimNodeReference ConvertToMirrorNode(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);
	
	/** Get a mirror context from an anim node context (pure) */
	/** 从动画节点上下文获取镜像上下文（纯） */
	UFUNCTION(BlueprintPure, Category = "Animation|Mirroring", meta=(BlueprintThreadSafe, DisplayName = "Convert to Mirror Node"))
	static void ConvertToMirrorNodePure(const FAnimNodeReference& Node, FMirrorAnimNodeReference& MirrorNode, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		MirrorNode = ConvertToMirrorNode(Node, ConversionResult);
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}
	
	/** Set the mirror state */
	/** 设置镜像状态 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Mirroring", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FMirrorAnimNodeReference SetMirror(const FMirrorAnimNodeReference& MirrorNode, bool bInMirror);
	
	/** Set how long to blend using inertialization when switching mirrored state */
	/** 设置切换镜像状态时使用惯性进行混合的时间 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Mirroring", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FMirrorAnimNodeReference SetMirrorTransitionBlendTime(const FMirrorAnimNodeReference& MirrorNode, float InBlendTime);
	
	/** Get the mirror state */
	/** 获取镜像状态 */
	UFUNCTION(BlueprintPure, Category = "Animation|Mirroring", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API bool GetMirror(const FMirrorAnimNodeReference& MirrorNode);
	
	/** Get MirrorDataTable used to perform mirroring*/
	/** 获取用于执行镜像的MirrorDataTable*/
	UFUNCTION(BlueprintPure, Category = "Animation|Mirroring", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API UMirrorDataTable* GetMirrorDataTable(const FMirrorAnimNodeReference& MirrorNode);
	
	/** Get how long to blend using inertialization when switching mirrored state */
	/** 获取切换镜像状态时使用惯性进行混合的时间 */
	UFUNCTION(BlueprintPure, Category = "Animation|Mirroring", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetMirrorTransitionBlendTime(const FMirrorAnimNodeReference& MirrorNode);
};