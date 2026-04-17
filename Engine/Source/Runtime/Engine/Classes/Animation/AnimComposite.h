// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Abstract base class of animation made of multiple sequences.
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Animation/AnimCompositeBase.h"
#include "AnimComposite.generated.h"

class UAnimSequence;
struct FCompactPose;

/**
* Animation Composites serve as a way to combine multiple animations together and treat them as a single unit.
*/
UCLASS(config=Engine, hidecategories=UObject, MinimalAPI, BlueprintType)
class UAnimComposite : public UAnimCompositeBase
{
	GENERATED_UCLASS_BODY()

public:
	/** Serializable data that stores section/anim pairing **/
	/** 存储部分/动画配对的可序列化数据 **/
	/** 存储部分/动画配对的可序列化数据 **/
	/** 存储部分/动画配对的可序列化数据 **/
	UPROPERTY()
	struct FAnimTrack AnimationTrack;

	/** 预览附加 BlendSpace 的基本姿势 **/
#if WITH_EDITORONLY_DATA
	/** 预览附加 BlendSpace 的基本姿势 **/
	/** Preview Base pose for additive BlendSpace **/
	/** 预览附加 BlendSpace 的基本姿势 **/
	UPROPERTY(EditAnywhere, Category=AdditiveSettings)
	TObjectPtr<UAnimSequence> PreviewBasePose;
#endif // WITH_EDITORONLY_DATA

	//~ Begin UObject Interface
 // ~ 开始 UObject 接口
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~ End UObject Interface
 // ~ 结束 UObject 接口

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	//~ Begin UAnimSequenceBase Interface
 // ~ 开始 UAnimSequenceBase 接口
	ENGINE_API virtual void HandleAssetPlayerTickedInternal(FAnimAssetTickContext &Context, const float PreviousTime, const float MoveDelta, const FAnimTickRecord &Instance, struct FAnimNotifyQueue& NotifyQueue) const override;

	virtual void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const override;
	virtual EAdditiveAnimationType GetAdditiveAnimType() const override;
	virtual bool IsValidAdditive() const override { return GetAdditiveAnimType() != AAT_None; }
	virtual void EnableRootMotionSettingFromMontage(bool bInEnableRootMotion, const ERootMotionRootLock::Type InRootMotionRootLock) override;
	virtual bool HasRootMotion() const override;
	virtual FTransform ExtractRootMotion(const FAnimExtractContext& ExtractionContext) const override final;
	virtual FTransform ExtractRootMotionFromRange(double StartTime, double EndTime, const FAnimExtractContext& ExtractionContext) const override final;
	virtual FTransform ExtractRootTrackTransform(const FAnimExtractContext& ExtractionContext, const FBoneContainer* RequiredBones) const override final;
	virtual void GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float & CurrentPosition, FAnimNotifyContext& NotifyContext) const override;
	virtual bool IsNotifyAvailable() const override;
	//~ End UAnimSequenceBase Interface
 // ~ 结束 UAnimSequenceBase 接口
	//~ Begin UAnimSequence Interface
 // ~ 开始 UAnimSequence 接口
#if WITH_EDITOR
	virtual class UAnimSequence* GetAdditiveBasePose() const override;
	virtual bool GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive = true) override;
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap) override;
	virtual void UpdateCommonTargetFrameRate() override;	
#endif
	//~ End UAnimSequence Interface
 // ~ 结束 UAnimSequence 接口

	//~ Begin UAnimCompositeBase Interface
 // ~ 开始 UAnimCompositeBase 接口
	virtual void InvalidateRecursiveAsset() override;
	virtual bool ContainRecursive(TArray<UAnimCompositeBase*>& CurrentAccumulatedList) override;
	virtual void SetCompositeLength(float InLength) override;
	virtual void PostLoad() override;
	virtual FFrameRate GetSamplingFrameRate() const override;
	//~End UAnimCompositeBase Interface
 // ~UAnimCompositeBase 接口结束
};

