// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_UseCachedPose.generated.h"

USTRUCT()
struct FAnimNode_UseCachedPose : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	// Note: This link is intentionally not public; it's wired up during compilation
 // 注意：此链接故意不公开；它是在编译期间连接的
	UPROPERTY()
	FPoseLink LinkToCachingNode;

	/** 故意不暴露，由 AnimBlueprintCompiler 设置 */
	/** 故意不暴露，由 AnimBlueprintCompiler 设置 */
	/** Intentionally not exposed, set by AnimBlueprintCompiler */
	/** 故意不暴露，由 AnimBlueprintCompiler 设置 */
	UPROPERTY(meta=(BlueprintCompilerGeneratedDefaults))
	FName CachePoseName;

public:	
	ENGINE_API FAnimNode_UseCachedPose();

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
