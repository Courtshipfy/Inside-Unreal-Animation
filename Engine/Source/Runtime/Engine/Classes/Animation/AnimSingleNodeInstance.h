// Copyright Epic Games, Inc. All Rights Reserved.

/**
 *
 * This Instance only contains one AnimationAsset, and produce poses
 * Used by Preview in AnimGraph, Playing single animation in Kismet2 and etc
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Optional.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimTypes.h"
#include "AnimSingleNodeInstance.generated.h"

struct FAnimInstanceProxy;

DECLARE_DYNAMIC_DELEGATE(FPostEvaluateAnimEvent);

UCLASS(transient, NotBlueprintable, MinimalAPI)
class UAnimSingleNodeInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	// Disable compiler-generated deprecation warnings by implementing our own destructor
 // 通过实现我们自己的析构函数来禁用编译器生成的弃用警告
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	~UAnimSingleNodeInstance() {}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	/** 当前正在播放的资产 **/
	/** 当前正在播放的资产 **/
	/** Current Asset being played **/
	/** 当前正在播放的资产 **/
	UPROPERTY(Transient)
	TObjectPtr<class UAnimationAsset> CurrentAsset;
	 
	UPROPERTY(Transient)
	FPostEvaluateAnimEvent PostEvaluateAnimEvent;

	//~ Begin UAnimInstance Interface
 // ~ 开始 UAnimInstance 接口
	ENGINE_API virtual void NativeInitializeAnimation() override;
	ENGINE_API virtual void NativePostEvaluateAnimation() override;
	ENGINE_API virtual void OnMontageInstanceStopped(FAnimMontageInstance& StoppedMontageInstance) override;

protected:
	ENGINE_API virtual void Montage_Advance(float DeltaTime) override;
	//~ End UAnimInstance Interface
 // ~ 结束UAnimInstance接口
public:
	UFUNCTION(BlueprintCallable, Category="Animation")
    ENGINE_API void SetMirrorDataTable(const UMirrorDataTable* MirrorDataTable);
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API const UMirrorDataTable* GetMirrorDataTable();
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API void SetLooping(bool bIsLooping);
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API void SetPlayRate(float InPlayRate);
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API void SetReverse(bool bInReverse);
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API void SetPosition(float InPosition, bool bFireNotifies=true);
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API void SetPositionWithPreviousTime(float InPosition, float InPreviousTime, bool bFireNotifies=true);
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API void SetBlendSpacePosition(const FVector& InPosition);
	UFUNCTION(BlueprintCallable, Category="Animation")
	/* 对于特定于 AnimSequence 的 **/
	ENGINE_API void SetPlaying(bool bIsPlaying);
	/* 对于特定于 AnimSequence 的 **/
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API float GetLength();
	/* For AnimSequence specific **/
	/* 对于特定于 AnimSequence 的 **/
	/** 设置新资源 - 调用InitializeAnimation，现在我们需要MeshComponent **/
	/** 设置新资源 - 调用InitializeAnimation，现在我们需要MeshComponent **/
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API void PlayAnim(bool bIsLooping=false, float InPlayRate=1.f, float InStartPosition=0.f);
	UFUNCTION(BlueprintCallable, Category="Animation")
	/** 获取当前使用的资产 */
	/** 获取当前使用的资产 */
	ENGINE_API void StopAnim();
	/** Set New Asset - calls InitializeAnimation, for now we need MeshComponent **/
	/** 设置姿势值 */
	/** 设置新资源 - 调用InitializeAnimation，现在我们需要MeshComponent **/
	/** 设置姿势值 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	ENGINE_API virtual void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping=true, float InPlayRate=1.f);
	/** Get the currently used asset */
	/** 获取任意 BlendSpace 的当前状态 */
	/** 获取当前使用的资产 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	/** 获取任意 BlendSpace 的当前状态 */
	/** 特定于动画序列 **/
	ENGINE_API virtual UAnimationAsset* GetAnimationAsset() const;
	/** Set pose value */
	/** 设置姿势值 */
	/** 特定于动画序列 **/
	/** 自定义评估姿势 **/
 	UFUNCTION(BlueprintCallable, Category = "Animation")
 	ENGINE_API void SetPreviewCurveOverride(const FName& PoseName, float Value, bool bRemoveIfZero);

public:
	/** 将蒙太奇插槽设置为预览 */
	/** 自定义评估姿势 **/
	/** Gets the current state of any BlendSpace */
	/** 获取任意 BlendSpace 的当前状态 */
	/** 根据时间跳跃更新蒙太奇权重（因为这不会由 SetPosition 处理） */
	ENGINE_API void GetBlendSpaceState(FVector& OutPosition, FVector& OutFilteredPosition) const;

	/** 将蒙太奇插槽设置为预览 */
	/** 如果我们的资源是混合空间，则更新混合空间样本列表 */
	/** AnimSequence specific **/
	/** 特定于动画序列 **/
	ENGINE_API void StepForward();
	/** 检查我们当前是否正在玩 */
	/** 根据时间跳跃更新蒙太奇权重（因为这不会由 SetPosition 处理） */
	ENGINE_API void StepBackward();

	/** 检查我们当前是否正在反向播放 */
	/** custom evaluate pose **/
	/** 如果我们的资源是混合空间，则更新混合空间样本列表 */
	/** 自定义评估姿势 **/
	/** 检查我们当前是否正在循环 */
	ENGINE_API virtual void RestartMontage(UAnimMontage * Montage, FName FromSection = FName());
	ENGINE_API void SetMontageLoop(UAnimMontage* Montage, bool bIsLooping, FName StartingSection = FName());
	/** 检查我们当前是否正在玩 */
	/** 获取当前播放时间 */

	/** Set the montage slot to preview */
	/** 将蒙太奇插槽设置为预览 */
	/** 获取当前播放速率倍数 */
	/** 检查我们当前是否正在反向播放 */
	ENGINE_API void SetMontagePreviewSlot(FName PreviewSlot);

	/** 获取当前正在播放的资源。可以返回NULL */
	/** Updates montage weights based on a jump in time (as this wont be handled by SetPosition) */
	/** 检查我们当前是否正在循环 */
	/** 根据时间跳跃更新蒙太奇权重（因为这不会由 SetPosition 处理） */
	/** 获取最后一个过滤器的输出 */
	ENGINE_API void UpdateMontageWeightForTimeSkip(float TimeDifference);

	/** 获取当前播放时间 */
	/** 设置动画插值类型覆盖。如果未设置，则不会覆盖。 */
	/** Updates the blendspace samples list in the case of our asset being a blendspace */
	/** 如果我们的资源是混合空间，则更新混合空间样本列表 */
	ENGINE_API void UpdateBlendspaceSamples(FVector InBlendInput);
	/** 获取动画插值类型覆盖。如果未设置，则不会覆盖。 */
	/** 获取当前播放速率倍数 */

	/** Check whether we are currently playing */
	/** 检查我们当前是否正在玩 */
	/** 获取当前正在播放的资源。可以返回NULL */
	ENGINE_API bool IsPlaying() const;

	/** Check whether we are currently playing in reverse */
	/** 获取最后一个过滤器的输出 */
	/** 检查我们当前是否正在反向播放 */
	ENGINE_API bool IsReverse() const;

	/** 设置动画插值类型覆盖。如果未设置，则不会覆盖。 */
	/** Check whether we are currently looping */
	/** 检查我们当前是否正在循环 */
	ENGINE_API bool IsLooping() const;
	/** 获取动画插值类型覆盖。如果未设置，则不会覆盖。 */

	/** Get the current playback time */
	/** 获取当前播放时间 */
	ENGINE_API float GetCurrentTime() const;

	/** Get the current play rate multiplier */
	/** 获取当前播放速率倍数 */
	ENGINE_API float GetPlayRate() const;

	/** Get the currently playing asset. Can return NULL */
	/** 获取当前正在播放的资源。可以返回NULL */
	ENGINE_API UAnimationAsset* GetCurrentAsset();

	/** Get the last filter output */
	/** 获取最后一个过滤器的输出 */
	ENGINE_API FVector GetFilterLastOutput();

	/** Set animation interpolation type override. If not set, it will not override. */
	/** 设置动画插值类型覆盖。如果未设置，则不会覆盖。 */
	ENGINE_API void SetInterpolationOverride(TOptional<EAnimInterpolationType> InterpolationType);

	/** Get animation interpolation type override. If not set, it will not override. */
	/** 获取动画插值类型覆盖。如果未设置，则不会覆盖。 */
 	ENGINE_API TOptional<EAnimInterpolationType> GetInterpolationOverride() const;


protected:
	ENGINE_API virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};



