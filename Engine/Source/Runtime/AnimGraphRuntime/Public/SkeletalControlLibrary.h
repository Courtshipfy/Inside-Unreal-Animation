// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "SkeletalControlLibrary.generated.h"

struct FAnimNode_SkeletalControlBase;

USTRUCT(BlueprintType)
struct FSkeletalControlReference : public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_SkeletalControlBase FInternalNodeType;
};

// Exposes operations to be performed on a skeletal control anim node
// 公开要在骨架控制动画节点上执行的操作
// Note: Experimental and subject to change!
// 注意：实验性的，可能会发生变化！
UCLASS(Experimental, MinimalAPI)
class USkeletalControlLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get a skeletal control from an anim node */
	/** 从动画节点获取骨骼控制 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Skeletal Controls", meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FSkeletalControlReference ConvertToSkeletalControl(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);

	/** Get a skeletal control from an anim node (pure) */
	/** 从动画节点获取骨骼控制（纯） */
	UFUNCTION(BlueprintPure, Category = "Animation|Skeletal Controls", meta=(BlueprintThreadSafe, DisplayName = "Convert to Skeletal Control"))
	static void ConvertToSkeletalControlPure(const FAnimNodeReference& Node, FSkeletalControlReference& SkeletalControl, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		SkeletalControl = ConvertToSkeletalControl(Node, ConversionResult);
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}
	
	/** Set the alpha value of this skeletal control */
	/** 设置此骨架控件的 alpha 值 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Skeletal Controls", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FSkeletalControlReference SetAlpha(const FSkeletalControlReference& SkeletalControl, float Alpha);

	/** Get the alpha value of this skeletal control */
	/** 获取该骨骼控制的alpha值 */
	UFUNCTION(BlueprintPure, Category = "Animation|Skeletal Controls", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetAlpha(const FSkeletalControlReference& SkeletalControl);
};
