// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "Animation/AnimExecutionContext.h"
#include "AnimExecutionContextLibrary.generated.h"

class UAnimInstance;

// Exposes operations to be performed on anim node contexts
// 公开要在动画节点上下文上执行的操作
UCLASS(MinimalAPI)
class UAnimExecutionContextLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
#if WITH_EDITOR
	/** Prototype function for thread-safe anim node calls */
	/** 线程安全动画节点调用的原型函数 */
	UFUNCTION(BlueprintInternalUseOnly, meta=(BlueprintThreadSafe))
	void Prototype_ThreadSafeAnimNodeCall(const FAnimExecutionContext& Context, const FAnimNodeReference& Node) {}

	/** Prototype function for thread-safe anim update calls */
	/** 线程安全动画更新调用的原型函数 */
	UFUNCTION(BlueprintInternalUseOnly, meta=(BlueprintThreadSafe))
	void Prototype_ThreadSafeAnimUpdateCall(const FAnimUpdateContext& Context, const FAnimNodeReference& Node) {}	
#endif
	
	/** Get the anim instance that hosts this context */
	/** 获取托管此上下文的动画实例 */
	UFUNCTION(BlueprintPure, Category = "Animation|Utilities", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API UAnimInstance* GetAnimInstance(const FAnimExecutionContext& Context);

	/** Internal compiler use only - Get a reference to an anim node by index */
	/** 仅供内部编译器使用 - 通过索引获取对动画节点的引用 */
	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta=(BlueprintThreadSafe, DefaultToSelf = "Instance"))
	static ANIMGRAPHRUNTIME_API FAnimNodeReference GetAnimNodeReference(UAnimInstance* Instance, int32 Index);

	/** Convert to an initialization context */
	/** 转换为初始化上下文 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Utilities", meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FAnimInitializationContext ConvertToInitializationContext(const FAnimExecutionContext& Context, EAnimExecutionContextConversionResult& Result);
	
	/** Convert to an update context */
	/** 转换为更新上下文 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Utilities", meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FAnimUpdateContext ConvertToUpdateContext(const FAnimExecutionContext& Context, EAnimExecutionContextConversionResult& Result);

	/** Get the current delta time in seconds */
	/** 获取当前增量时间（以秒为单位） */
	UFUNCTION(BlueprintPure, Category = "Animation|Utilities", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetDeltaTime(const FAnimUpdateContext& Context);

	/** Get the current weight of this branch of the graph */
	/** 获取图的该分支的当前权重 */
	UFUNCTION(BlueprintPure, Category = "Animation|Utilities", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API float GetCurrentWeight(const FAnimUpdateContext& Context);
	
	/** Get whether this branch of the graph is active (i.e. NOT blending out). */
	/** 获取图表的这个分支是否处于活动状态（即不混合）。 */
	UFUNCTION(BlueprintPure, Category = "Animation|Utilities", meta=(BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API bool IsActive(const FAnimExecutionContext& Context);

	/** Convert to a pose context */
	/** 转换为姿势上下文 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Utilities", meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FAnimPoseContext ConvertToPoseContext(const FAnimExecutionContext& Context, EAnimExecutionContextConversionResult& Result);

	/** Convert to a component space pose context */
	/** 转换为组件空间姿势上下文 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Utilities", meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FAnimComponentSpacePoseContext ConvertToComponentSpacePoseContext(const FAnimExecutionContext& Context, EAnimExecutionContextConversionResult& Result);
};
