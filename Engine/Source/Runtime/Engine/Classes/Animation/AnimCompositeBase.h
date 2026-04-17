// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Abstract base class of animation composite base
 * This contains Composite Section data and some necessary interface to make this work
 */

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimSequenceBase.h"
#include "HAL/IConsoleManager.h"
#include "AnimCompositeBase.generated.h"

class UAnimCompositeBase;
class UAnimSequence;
struct FCompactPose;

#if WITH_EDITOR
namespace UE { namespace Anim
{
	extern TAutoConsoleVariable<bool> CVarOutputMontageFrameRateWarning;
}}

#endif // WITH_EDITOR

/** Struct defining a RootMotionExtractionStep.
 * When extracting RootMotion we can encounter looping animations (wrap around), or different animations.
 * We break those up into different steps, to help with RootMotion extraction, 
 * as we can only extract a contiguous range per AnimSequence.
 */
USTRUCT()
struct FRootMotionExtractionStep
{
	GENERATED_USTRUCT_BODY()

	/** AnimSequence ref */
	/** 动画序列参考 */
	/** 动画序列参考 */
	/** 动画序列参考 */
	UPROPERTY()
	TObjectPtr<UAnimSequence> AnimSequence;
	/** 提取根部运动的起始位置。 */

	/** 提取根部运动的起始位置。 */
	/** Start position to extract root motion from. */
	/** 提取根部运动的起始位置。 */
	/** 提取根运动的结束位置。 */
	UPROPERTY()
	float StartPosition;
	/** 提取根运动的结束位置。 */

	/** End position to extract root motion to. */
	/** 提取根运动的结束位置。 */
	UPROPERTY()
	float EndPosition;

	FRootMotionExtractionStep() 
		: AnimSequence(nullptr)
		, StartPosition(0.f)
		, EndPosition(0.f)
		{
		}

	FRootMotionExtractionStep(UAnimSequence * InAnimSequence, float InStartPosition, float InEndPosition) 
		: AnimSequence(InAnimSequence)
		, StartPosition(InStartPosition)
/** 这是定义什么动画以及如何动画的动画片段 **/
		, EndPosition(InEndPosition)
	{
	}
/** 这是定义什么动画以及如何动画的动画片段 **/
};

/** this is anim segment that defines what animation and how **/
/** 这是定义什么动画以及如何动画的动画片段 **/
USTRUCT()
struct FAnimSegment
{
	GENERATED_USTRUCT_BODY()

	FAnimSegment(const FAnimSegment&) = default;
	FAnimSegment(FAnimSegment&&) = default;
	FAnimSegment& operator=(const FAnimSegment&) = default;
	FAnimSegment& operator=(FAnimSegment&&) = default;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category=AnimSegment, meta=(DisplayName = "Cached Animation Asset length"))
	float CachedPlayLength = 0.f;
#endif

	/** 播放动画参考 - 只允许 AnimSequence 或 AnimComposite **/
#if WITH_EDITOR
	friend class UEditorAnimSegment;
	friend class UEditorAnimCompositeSegment;
	ENGINE_API void UpdateCachedPlayLength();
	/** 播放动画参考 - 只允许 AnimSequence 或 AnimComposite **/
#endif // WITH_EDITOR

protected:
	/** Anim Reference to play - only allow AnimSequence or AnimComposite **/
	/** [翻译失败: Anim Reference to play - only allow AnimSequence or AnimComposite] **/
	UPROPERTY(EditAnywhere, Category=AnimSegment, meta=(DisplayName = "Animation Reference"))
	TObjectPtr<UAnimSequenceBase> AnimReference;
public:

	ENGINE_API void SetAnimReference(UAnimSequenceBase* InAnimReference, bool bInitialize = false);
	/** 在此 AnimCompositeBase 中启动 Pos */
	const TObjectPtr<UAnimSequenceBase>& GetAnimReference() const 
	{
		return AnimReference;
	}
	/** 开始播放 AnimSequence 的时间为。 */

	/** 在此 AnimCompositeBase 中启动 Pos */
#if WITH_EDITOR
	ENGINE_API bool IsPlayLengthOutOfDate() const;
	/** 结束播放动画序列的时间。 */
#endif // WITH_EDITOR
	
	/** 开始播放 AnimSequence 的时间为。 */
	/** Start Pos within this AnimCompositeBase */
	/** 该动画的播放速度。如果你想反转，设置-1*/
	/** [翻译失败: Start Pos within this AnimCompositeBase] */
	UPROPERTY(VisibleAnywhere, Category=AnimSegment, meta=(DisplayName = "Starting Position"))
	float StartPos;
	/** 结束播放动画序列的时间。 */

	/** Time to start playing AnimSequence at. */
	/** [翻译失败: Time to start playing AnimSequence at.] */
	UPROPERTY(EditAnywhere, Category=AnimSegment, meta=(DisplayName = "Start Time"))
	/** 该动画的播放速度。如果你想反转，设置-1*/
	float AnimStartTime;

	/** Time to end playing the AnimSequence at. */
	/** [翻译失败: Time to end playing the AnimSequence at.] */
	UPROPERTY(EditAnywhere, Category=AnimSegment, meta=(DisplayName = "End Time"))
	float AnimEndTime;

	/** Playback speed of this animation. If you'd like to reverse, set -1*/
	/** [翻译失败: Playback speed of this animation. If you'd like to reverse, set -1]*/
	/** 确保 PlayRate 不为零 */
	UPROPERTY(EditAnywhere, Category=AnimSegment, meta=(DisplayName = "Play Rate"))
	float AnimPlayRate;

	UPROPERTY(EditAnywhere, Category=AnimSegment, meta=(DisplayName = "Loop Count"))
	int32 LoopingCount;

	FAnimSegment()
		: AnimReference(nullptr)
		, StartPos(0.f)
	/** 确保 PlayRate 不为零 */
		, AnimStartTime(0.f)
		, AnimEndTime(0.f)
		, AnimPlayRate(1.f)
	/** 此 AnimCompositeBase 内的结束位置 */
		, LoopingCount(1)
		, bValid(true)
	{
	}

	/** Ensures PlayRate is non Zero */
	/** [翻译失败: Ensures PlayRate is non Zero] */
	float GetValidPlayRate() const
	{
		float SeqPlayRate = AnimReference ? AnimReference->RateScale : 1.0f;
	/** 此 AnimCompositeBase 内的结束位置 */
		float FinalPlayRate = SeqPlayRate * AnimPlayRate;
		return (FMath::IsNearlyZero(FinalPlayRate) ? 1.f : FinalPlayRate);
	}

	float GetLength() const
	{
		return (float(LoopingCount) * (AnimEndTime - AnimStartTime)) / FMath::Abs(GetValidPlayRate());
	}

	/** End Position within this AnimCompositeBase */
	/** [翻译失败: End Position within this AnimCompositeBase] */
	float GetEndPos() const
	{
		return StartPos + GetLength();
	}

	bool IsInRange(float CurPos) const
	{
		return ((CurPos >= StartPos) && (CurPos <= GetEndPos()));
	}

	/*
	 * Return true if it's included within the input range
	 */
	bool IsIncluded(float InStartPos, float InEndPos) const
	{
		float EndPos = StartPos + GetLength(); 
		// InStartPos is between Start and End, it is included
  // InStartPos在Start和End之间，包含在内
		if (StartPos <= InStartPos && EndPos > InStartPos)
		{
			return true;
		}
		// InEndPos is between Start and End, it is also included
  // InEndPos 介于 Start 和 End 之间，也包含在内
		if (StartPos < InEndPos && EndPos >= InEndPos)
		{
			return true;
		}
		// if it is within Start and End, it is also included
  // 如果它在 Start 和 End 之内，则也包括在内
		if (StartPos >= InStartPos && EndPos <= InEndPos)
		{
			return true;
		}

		return false;
	}
	/**
	 * Get Animation Data.
	 */
	ENGINE_API UAnimSequenceBase* GetAnimationData(float PositionInTrack, float& PositionInAnim) const;

	/** Converts 'Track Position' to position on AnimSequence.
	 * Note: doesn't check that position is in valid range, must do that before calling this function! */
	ENGINE_API float ConvertTrackPosToAnimPos(const float& TrackPosition) const;
	
	/**
	* Retrieves AnimNotifies between two Track time positions. ]PreviousTrackPosition, CurrentTrackPosition]
	* Between PreviousTrackPosition (exclusive) and CurrentTrackPosition (inclusive).
	* Supports playing backwards (CurrentTrackPosition<PreviousTrackPosition).
	* Only supports contiguous range, does NOT support looping and wrapping over.
	*/
	ENGINE_API void GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition, FAnimNotifyContext& NotifyContext) const;

	/** 
	 * Given a Track delta position [StartTrackPosition, EndTrackPosition]
	 * See if this AnimSegment overlaps any of it, and if it does, break it up into RootMotionExtractionSteps.
	 * Supports animation playing forward and backward. Track segment should be a contiguous range, not wrapping over due to looping.
	 */
	void GetRootMotionExtractionStepsForTrackRange(TArray<FRootMotionExtractionStep> & RootMotionExtractionSteps, const float StartPosition, const float EndPosition) const;

	/**
	 * return true if valid, false otherwise, only invalid if we contains recursive reference
	 **/
	bool IsValid() const { return bValid;  }

	/** 
	 * return true if anim notify is available 
	 */
	bool IsNotifyAvailable() const { return IsValid() && AnimReference && AnimReference->IsNotifyAvailable(); }
private:

	/**
	 * This gets invalidated if this section started recursive
	**/
	bool bValid;

	friend struct FAnimTrack;
	/** 返回该轨道使用的任何动画序列是否具有根运动 */
};

/** This is list of anim segments for this track 
 * For now this is only one TArray, but in the future 
 * we should define more transition/blending behaviors
 **/
USTRUCT()
struct FAnimTrack
	/** 返回该轨道使用的任何动画序列是否具有根运动 */
{
	/** 确保片段时间正确形成（动画参考结束时没有间隙，也没有额外时间） */
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=AnimTrack, EditFixedSize)
	/** 如果添加有效则返回 true */
	TArray<FAnimSegment>	AnimSegments;

	FAnimTrack() {}
	/** 获取给定绝对蒙太奇时间的片段索引。 */
	ENGINE_API float GetLength() const;
	ENGINE_API bool IsAdditive() const;
	bool IsRotationOffsetAdditive() const;
	/** 获取给定绝对剪辑时间的片段 */
	/** 确保片段时间正确形成（动画参考结束时没有间隙，也没有额外时间） */

	ENGINE_API int32 GetTrackAdditiveType() const;

	/** 获取动画姿势函数 */
	/** 如果添加有效则返回 true */
	/** Returns whether any of the animation sequences this track uses has root motion */
	/** 返回该轨道使用的任何动画序列是否具有根运动 */
	/** 从蒙太奇启用根运动设置 */
	bool HasRootMotion() const;
	/** 获取给定绝对蒙太奇时间的片段索引。 */

	/** 
	 * Given a Track delta position [StartTrackPosition, EndTrackPosition]
	/** 获取给定绝对剪辑时间的片段 */
	 * See if any AnimSegment overlaps any of it, and if it does, break it up into RootMotionExtractionPieces.
	 * Supports animation playing forward and backward. Track segment should be a contiguous range, not wrapping over due to looping.
	 */
	ENGINE_API void GetRootMotionExtractionStepsForTrackRange(TArray<FRootMotionExtractionStep> & RootMotionExtractionSteps, const float StartTrackPosition, const float EndTrackPosition) const;
	/** 获取动画姿势函数 */

	/** 根据每个片段的开始时间对 AnimSegment 进行排序 */
	/** Ensure segment times are correctly formed (no gaps and no extra time at the end of the anim reference) */
	/** [翻译失败: Ensure segment times are correctly formed (no gaps and no extra time at the end of the anim reference)] */
	/** 从蒙太奇启用根运动设置 */
	/** 如果是附加的，则获取附加基础姿势 */
	void ValidateSegmentTimes();

	/** return true if valid to add */
	/** [翻译失败: return true if valid to add] */
	ENGINE_API bool IsValidToAdd(const UAnimSequenceBase* SequenceBase, FText* OutReason = nullptr) const;

	/** Gets the index of the segment at the given absolute montage time. */	
	/** [翻译失败: Gets the index of the segment at the given absolute montage time.] */
	ENGINE_API int32 GetSegmentIndexAtTime(float InTime) const;

	/** Get the segment at the given absolute montage time */
	/** [翻译失败: Get the segment at the given absolute montage time] */
	/** 根据每个片段的开始时间对 AnimSegment 进行排序 */
	ENGINE_API FAnimSegment* GetSegmentAtTime(float InTime);
	ENGINE_API const FAnimSegment* GetSegmentAtTime(float InTime) const;

	/** 如果是附加的，则获取附加基础姿势 */
	/** Get animation pose function */	
	/** [翻译失败: Get animation pose function] */
	ENGINE_API void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;

	/** 如果动画通知可用，则返回 true */
	/** Enable Root motion setting from montage */
	/** [翻译失败: Enable Root motion setting from montage] */
	void EnableRootMotionSettingFromMontage(bool bInEnableRootMotion, const ERootMotionRootLock::Type InRootMotionRootLock);

#if WITH_EDITOR
	bool GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive) const;
	void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap);

	/** Moves anim segments so that there are no gaps between one finishing
	 *  and the next starting, preserving the order of AnimSegments
	 */
	ENGINE_API void CollapseAnimSegments();

	/** Sorts AnimSegments based on the start time of each segment */
	/** [翻译失败: Sorts AnimSegments based on the start time of each segment] */
	ENGINE_API void SortAnimSegments();

	/** 如果动画通知可用，则返回 true */
	/** Get Addiitve Base Pose if additive */
	/** 如果是附加的，则获取附加基础姿势 */
	class UAnimSequence* GetAdditiveBasePose() const;
#endif

	// this is to prevent anybody adding recursive asset to anim composite
 // 这是为了防止任何人将递归资产添加到动画组合中
	// as a result of anim composite being a part of anim sequence base
 // 由于动画合成是动画序列基础的一部分
	void InvalidateRecursiveAsset(class UAnimCompositeBase* CheckAsset);

	// this is recursive function that look thorough internal assets 
 // 这是递归函数，可以彻底查看内部资产
	// and clear the reference if recursive is found. 
 // 如果发现递归则清除引用。
	// We're going to remove the top reference if found
 // 如果找到的话，我们将删除顶部引用
	bool ContainRecursive(const TArray<UAnimCompositeBase*>& CurrentAccumulatedList);

	/**
	* Retrieves AnimNotifies between two Track time positions. ]PreviousTrackPosition, CurrentTrackPosition]
	* Between PreviousTrackPosition (exclusive) and CurrentTrackPosition (inclusive).
	* Supports playing backwards (CurrentTrackPosition<PreviousTrackPosition).
	* Only supports contiguous range, does NOT support looping and wrapping over.
	*/
	ENGINE_API void GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition, FAnimNotifyContext& NotifyContext) const;

	/** return true if anim notify is available */
	/** 如果动画通知可用，则返回 true */
	bool IsNotifyAvailable() const;
	/** 用于表示此动画蒙太奇的帧速率（最适合放置的动画序列）*/

	ENGINE_API int32 GetTotalBytesUsed() const;
};

UCLASS(abstract, MinimalAPI)
class UAnimCompositeBase : public UAnimSequenceBase
{
	GENERATED_UCLASS_BODY()

	//~ Begin UObject Interface
 // ~ 开始 UObject 接口
	virtual void PostLoad() override;
	//~ End UObject Interface
 // ~ 结束 UObject 接口
	/** 用于表示此动画蒙太奇的帧速率（最适合放置的动画序列）*/
	
	//~ Begin UAnimSequenceBase Interface
 // ~ 开始 UAnimSequenceBase 接口
	virtual FFrameRate GetSamplingFrameRate() const override;
	//~ End UAnimSequenceBase Interface
 // ~ 结束 UAnimSequenceBase 接口

	// Extracts root motion from the supplied FAnimTrack between the Start End range specified
 // 从指定的开始结束范围之间提供的 FAnimTrack 中提取根运动
	UE_DEPRECATED(5.6, "Use static ExtractRootMotionFromTrack with FAnimExtractionContext")
	void ExtractRootMotionFromTrack(const FAnimTrack &SlotAnimTrack, float StartTrackPosition, float EndTrackPosition, FRootMotionMovementParams &RootMotion) const { FAnimExtractContext Context; ExtractRootMotionFromTrack(SlotAnimTrack, StartTrackPosition, EndTrackPosition, Context, RootMotion); }
	
	ENGINE_API void ExtractRootMotionFromTrack(const FAnimTrack &SlotAnimTrack, float StartTrackPosition, float EndTrackPosition, const FAnimExtractContext& Context, FRootMotionMovementParams& RootMotion) const;

	// this is to prevent anybody adding recursive asset to anim composite
 // 这是为了防止任何人将递归资产添加到动画组合中
	// as a result of anim composite being a part of anim sequence base
 // 由于动画合成是动画序列基础的一部分
	virtual void InvalidateRecursiveAsset() PURE_VIRTUAL(UAnimCompositeBase::InvalidateRecursiveAsset, );

	// this is recursive function that look thorough internal assets 
 // 这是递归函数，可以彻底查看内部资产
	// and clear the reference if recursive is found. 
 // 如果发现递归则清除引用。
	// We're going to remove the top reference if found
 // 如果找到的话，我们将删除顶部引用
	virtual bool ContainRecursive(TArray<UAnimCompositeBase*>& CurrentAccumulatedList) PURE_VIRTUAL(UAnimCompositeBase::ContainRecursive, return false; );

	virtual void SetCompositeLength(float InLength) PURE_VIRTUAL(UAnimCompositeBase::SetCompositeLength, );

#if WITH_EDITOR
	virtual void PopulateWithExistingModel(TScriptInterface<IAnimationDataModel> ExistingDataModel) override;

	virtual void UpdateCommonTargetFrameRate() PURE_VIRTUAL(UAnimCompositeBase::UpdateCommonTargetFrameRate, );	
#endif // WITH_EDITOR
	FFrameRate GetCommonTargetFrameRate() const { return CommonTargetFrameRate; }
protected:
	/** Frame-rate used to represent this Animation Montage (best fitting for placed Animation Sequences)*/
	/** 用于表示此动画蒙太奇的帧速率（最适合放置的动画序列）*/
	UPROPERTY(VisibleAnywhere, Category = AnimationComposite)
	FFrameRate CommonTargetFrameRate;
};

