// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_BlendSpaceGraphBase.generated.h"

class UBlendSpace;

// Allows multiple animations to be blended between based on input parameters
// 允许根据输入参数混合多个动画
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendSpaceGraphBase : public FAnimNode_Base
{
	GENERATED_BODY()

	friend class UAnimGraphNode_BlendSpaceGraphBase;

	// @return the blendspace that this node uses
 // @return 该节点使用的混合空间
	const UBlendSpace* GetBlendSpace() const { return BlendSpace; }

	// @return the current sample coordinates that this node is using to sample the blendspace
 // @返回该节点用于对混合空间进行采样的当前采样坐标
	FVector GetPosition() const { return FVector(X, Y, 0); }

	// @return the current sample coordinates after going through the filtering
 // @返回经过过滤后的当前样本坐标
	FVector GetFilteredPosition() const { return BlendFilter.GetFilterLastOutput(); }

#if WITH_EDITORONLY_DATA
	// Set the node to preview a supplied sample value
 // 设置节点以预览提供的样本值
	ANIMGRAPHRUNTIME_API void SetPreviewPosition(FVector InVector);
#endif

	// Forces the Position to the specified value
 // 强制将位置设置为指定值
	ANIMGRAPHRUNTIME_API void SnapToPosition(const FVector& NewPosition);

protected:
	// The X coordinate to sample in the blendspace
 // 在混合空间中采样的 X 坐标
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Coordinates, meta = (PinShownByDefault, AllowPrivateAccess))
	float X = 0.0f;

	// The Y coordinate to sample in the blendspace
 // 在混合空间中采样的 Y 坐标
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Coordinates, meta = (PinShownByDefault, AllowPrivateAccess))
	float Y = 0.0f;

	// The group name that we synchronize with (NAME_None if it is not part of any group). Note that
 // 我们与之同步的组名称（如果不属于任何组，则为 NAME_None）。注意
	// this is the name of the group used to sync the output of this node - it will not force
 // 这是用于同步该节点输出的组的名称 - 它不会强制
	// syncing of animations contained by it. Animations inside this Blend Space have their own
 // 同步其中包含的动画。此混合空间内的动画有自己的
	// options for syncing.
 // 同步选项。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sync, meta = (NeverAsPin, AllowPrivateAccess))
	FName GroupName = NAME_None;

	// The role this Blend Space can assume within the group (ignored if GroupName is not set). Note
 // 此混合空间在组内可以承担的角色（如果未设置 GroupName，则忽略）。笔记
	// that this is the role of the output of this node, not of animations contained by it.
 // 这是该节点输出的作用，而不是它包含的动画的作用。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sync, meta = (NeverAsPin, AllowPrivateAccess))
	TEnumAsByte<EAnimGroupRole::Type> GroupRole = EAnimGroupRole::CanBeLeader;

	// The internal blendspace asset to play
 // 要播放的内部混合空间资源
	/** 三角测量/分割中的先前位置 */
	UPROPERTY()
	TObjectPtr<const UBlendSpace> BlendSpace = nullptr;

	// Pose links for each sample in the blendspace
 // 为混合空间中的每个样本设置链接
	UPROPERTY()
	TArray<FPoseLink> SamplePoseLinks;

protected:
	// FIR filter applied to inputs
 // FIR 滤波器应用于输入
	FBlendFilter BlendFilter;

	// Cache of sampled data, updated each frame
 // 采样数据缓存，每帧更新
	TArray<FBlendSampleData> BlendSampleDataCache;

	/** 三角测量/分割中的先前位置 */
	/** Previous position in the triangulation/segmentation */
	/** 三角测量/分割中的先前位置 */
	int32 CachedTriangulationIndex = -1;

#if WITH_EDITORONLY_DATA
	// Preview blend params - set in editor only
 // 预览混合参数 - 仅在编辑器中设置
	FVector PreviewPosition = FVector::ZeroVector;

	// Whether to use the preview blend params
 // 是否使用预览混合参数
	bool bUsePreviewPosition = false;
#endif

	// Internal update handler, skipping evaluation of exposed inputs
 // 内部更新处理程序，跳过公开输入的评估
	ANIMGRAPHRUNTIME_API void UpdateInternal(const FAnimationUpdateContext& Context);

protected:
	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束
};
