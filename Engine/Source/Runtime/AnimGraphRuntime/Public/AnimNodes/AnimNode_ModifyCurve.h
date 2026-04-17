// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_ModifyCurve.generated.h"

UENUM()
enum class EModifyCurveApplyMode : uint8
{
	/** Add new value to input curve value */
	/** 将新值添加到输入曲线值 */
	Add,

	/** Scale input value by new value */
	/** 按新值缩放输入值 */
	Scale,

	/** Blend input with new curve value, using Alpha setting on the node */
	/** 使用节点上的 Alpha 设置将输入与新曲线值混合 */
	Blend,

	/** Blend the new curve value with the last curve value using Alpha to determine the weighting (.5 is a moving average, higher values react to new values faster, lower slower) */
	/** 使用 Alpha 将新曲线值与最后一个曲线值混合以确定权重（0.5 是移动平均值，较高的值对新值的反应较快，较低的较慢） */
	WeightedMovingAverage,

	/** Remaps the new curve value between the CurveValues entry and 1.0 (.5 in CurveValues makes 0.51 map to 0.02) */
	/** 重新映射 CurveValues 条目和 1.0 之间的新曲线值（CurveValues 中的 0.5 使 0.51 映射到 0.02） */
	RemapCurve
};

/** Easy way to modify curve values on a pose */
/** 修改姿势曲线值的简单方法 */
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_ModifyCurve : public FAnimNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadWrite, Category = Links)
	FPoseLink SourcePose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, editfixedsize, Category = ModifyCurve, meta = (PinHiddenByDefault))
	TMap<FName, float> CurveMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, editfixedsize, Category = ModifyCurve, meta = (BlueprintCompilerGeneratedDefaults, PinShownByDefault))
	TArray<float> CurveValues;

	UPROPERTY(meta = (BlueprintCompilerGeneratedDefaults))
	TArray<FName> CurveNames;

	UE_DEPRECATED(5.3, "LastCurveValues is no longer used.")
	TArray<float> LastCurveValues;

	UE_DEPRECATED(5.3, "LastCurveMapValues is no longer used.")
	TMap<FName, float> LastCurveMapValues;

	UE_DEPRECATED(5.3, "bInitializeLastValuesMap is no longer used.")
	bool bInitializeLastValuesMap;

	FBlendedHeapCurve LastCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModifyCurve, meta = (PinShownByDefault))
	float Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModifyCurve)
	EModifyCurveApplyMode ApplyMode;

	ANIMGRAPHRUNTIME_API FAnimNode_ModifyCurve();
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FAnimNode_ModifyCurve(const FAnimNode_ModifyCurve&) = default;
	FAnimNode_ModifyCurve& operator=(const FAnimNode_ModifyCurve&) = default;
	ANIMGRAPHRUNTIME_API PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;

	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

#if WITH_EDITOR
	/** Add new curve being modified */
	/** 添加正在修改的新曲线 */
	ANIMGRAPHRUNTIME_API void AddCurve(const FName& InName, float InValue);
	/** Remove a curve from being modified */
	/** 删除正在修改的曲线 */
	ANIMGRAPHRUNTIME_API void RemoveCurve(int32 PoseIndex);
#endif // WITH_EDITOR

private:
	ANIMGRAPHRUNTIME_API float ProcessCurveOperation(float CurrentValue, float NewValue) const;
	ANIMGRAPHRUNTIME_API float ProcessCurveWMAOperation(float CurrentValue, float LastValue) const;
};
