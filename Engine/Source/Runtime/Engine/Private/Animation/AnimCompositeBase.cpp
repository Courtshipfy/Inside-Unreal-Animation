// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimCompositeBase.cpp: Anim Composite base class that contains AnimTrack data structure/interface
=============================================================================*/ 

#include "Animation/AnimCompositeBase.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimComposite.h"
#include "Animation/AnimNotifyQueue.h"
#include "BonePose.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"
#include "EngineLogs.h"
#include "UObject/LinkerLoad.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimCompositeBase)

#if WITH_EDITOR
namespace UE
{
	namespace Anim
	{		
		TAutoConsoleVariable<bool> CVarOutputMontageFrameRateWarning(
			TEXT("a.OutputMontageFrameRateWarning"),
			false,
			TEXT("If true will warn the user about Animation Montages/Composites composed of incompatible animation assets (incompatible frame-rates)."));
	}
}
#endif // WITH_EDITOR

///////////////////////////////////////////////////////
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
// FAnimSegment
///////////////////////////////////////////////////////

UAnimSequenceBase* FAnimSegment::GetAnimationData(float PositionInTrack, float& PositionInAnim) const
{
	if( bValid && IsInRange(PositionInTrack) )
	{
		if(AnimReference)
		{
			const float ValidPlayRate = GetValidPlayRate();

			// this result position should be pure position within animation
   // 这个结果位置应该是动画中的纯位置
			float Delta = (PositionInTrack - StartPos);

			// LoopingCount should not be zero, and it should not get here, but just in case
   // LoopingCount 不应该为零，也不应该到达这里，但以防万一
			if (LoopingCount > 1)
			{
				// we need to consider looping count
    // 我们需要考虑循环计数
				float AnimPlayLength = (AnimEndTime - AnimStartTime) / FMath::Abs(ValidPlayRate);
				Delta = FMath::Fmod(Delta, AnimPlayLength);
			}

			if (ValidPlayRate > 0.f)
			{
				PositionInAnim = AnimStartTime + Delta * ValidPlayRate;
			}
			else
			{
				PositionInAnim = AnimEndTime + Delta * ValidPlayRate;
			}

			return AnimReference;
		}
	}

	return nullptr;
}

/** Converts 'Track Position' to position on AnimSequence.
 * Note: doesn't check that position is in valid range, must do that before calling this function! */
float FAnimSegment::ConvertTrackPosToAnimPos(const float& TrackPosition) const
{
	const float PlayRate = GetValidPlayRate();
	const float AnimLength = (AnimEndTime - AnimStartTime);
	const float AnimPositionUnWrapped = (TrackPosition - StartPos) * PlayRate;

	// Figure out how many times animation is allowed to be looped.
 // 计算出允许动画循环多少次。
	const float LoopCount = FMath::Min(FMath::FloorToInt(FMath::Abs(AnimPositionUnWrapped) / AnimLength), FMath::Max(LoopingCount-1, 0));
	// Position within AnimSequence
 // 动画序列中的位置
	const float AnimPoint = (PlayRate >= 0.f) ? AnimStartTime : AnimEndTime;

	const float AnimPosition = AnimPoint + (AnimPositionUnWrapped - float(LoopCount) * AnimLength);
	
	return AnimPosition;
}

void FAnimSegment::GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition, FAnimNotifyContext& NotifyContext) const
{
	const bool bTrackPlayingBackwards = (PreviousTrackPosition > CurrentTrackPosition);
	const float SegmentStartPos = StartPos;
	const float SegmentEndPos = StartPos + GetLength();
	const bool bZeroTrackPositionDelta = CurrentTrackPosition == PreviousTrackPosition;

	// if track range overlaps segment
 // 如果轨道范围与段重叠
	if( bTrackPlayingBackwards 
		? ((CurrentTrackPosition < SegmentEndPos) && (PreviousTrackPosition > SegmentStartPos)) 
		: ((PreviousTrackPosition < SegmentEndPos) && (CurrentTrackPosition > SegmentStartPos)) 
		)
	{
		// Only allow AnimSequences for now. Other types will need additional support.
  // 目前仅允许 AnimSequences。其他类型将需要额外的支持。
		UAnimSequenceBase* AnimSequenceBase = AnimReference;
		if(AnimSequenceBase)
		{
			const float ValidPlayRate = GetValidPlayRate();
			const float AbsValidPlayRate = FMath::Abs(ValidPlayRate);

			// Get starting position, closest overlap.
   // 获取起始位置，最接近的重叠。
			float AnimStartPosition = ConvertTrackPosToAnimPos( bTrackPlayingBackwards ? FMath::Min(PreviousTrackPosition, SegmentEndPos) : FMath::Max(PreviousTrackPosition, SegmentStartPos) );
			AnimStartPosition = FMath::Clamp(AnimStartPosition, AnimStartTime, AnimEndTime);

			// When looping, the current track position could exceed the current segment (anim montage loops the track position after firing notifies)
   // 循环时，当前轨道位置可能超出当前片段（动画蒙太奇在触发通知后循环轨道位置）
			// We need to make sure to clamp the current/previous track positions within our segment
   // 我们需要确保将当前/之前的轨道位置限制在我们的段内
			float TrackTimeToGo = FMath::Abs(FMath::Clamp(CurrentTrackPosition, SegmentStartPos, SegmentEndPos) - FMath::Clamp(PreviousTrackPosition, SegmentStartPos, SegmentEndPos));

			// The track can be playing backwards and the animation can be playing backwards, so we
   // 曲目可以向后播放，动画也可以向后播放，所以我们
			// need to combine to work out what direction we are traveling through the animation
   // 需要结合起来找出我们在动画中行进的方向
			bool bAnimPlayingBackwards = bTrackPlayingBackwards ^ (ValidPlayRate < 0.f);
			const float ResetStartPosition = bAnimPlayingBackwards ? AnimEndTime : AnimStartTime;

			// Abstract out end point since animation can be playing forward or backward.
   // 由于动画可以向前或向后播放，所以抽象出终点。
			const float AnimEndPoint = bAnimPlayingBackwards ? AnimStartTime : AnimEndTime;

			for(int32 IterationsLeft=FMath::Max(LoopingCount, 1); ((IterationsLeft > 0) && (TrackTimeToGo > 0.f || bZeroTrackPositionDelta)); --IterationsLeft)
			{
				// Track time left to reach end point of animation.
    // 跟踪到达动画终点的剩余时间。
				const float TrackTimeToAnimEndPoint = (AnimEndPoint - AnimStartPosition) / AbsValidPlayRate;

				// If our time left is shorter than time to end point, no problem. End there.
    // 如果剩下的时间比到达终点的时间短，没问题。到此结束。
				// This will also run if we arrive with bZeroTrackPositionDelta == true, as TrackTimeToGo == 0.f
    // 如果我们以 bZeroTrackPositionDelta == true 到达，这也将运行，因为 TrackTimeToGo == 0.f
				if( FMath::Abs(TrackTimeToGo) < FMath::Abs(TrackTimeToAnimEndPoint) )
				{
					const float PlayRate = ValidPlayRate * (bTrackPlayingBackwards ? -1.f : 1.f);
					const float AnimEndPosition = (TrackTimeToGo * PlayRate) + AnimStartPosition;
					AnimSequenceBase->GetAnimNotifiesFromDeltaPositions(AnimStartPosition, AnimEndPosition, NotifyContext);
					break;
				}
				// Otherwise we hit the end point of the animation first...
    // 否则我们首先到达动画的终点......
				else
				{
					// Add that piece for extraction.
     // 添加该部分以进行提取。
					AnimSequenceBase->GetAnimNotifiesFromDeltaPositions(AnimStartPosition, AnimEndPoint, NotifyContext);

					// decrease our TrackTimeToGo if we have to do another iteration.
     // 如果我们必须进行另一次迭代，则减少 TrackTimeToGo。
					// and put ourselves back at the beginning of the animation.
     // 并将我们自己带回到动画的开头。
					TrackTimeToGo -= TrackTimeToAnimEndPoint;
					AnimStartPosition = ResetStartPosition;
				}
			}
		}
	}
}

/** 
 * Given a Track delta position [StartTrackPosition, EndTrackPosition]
 * See if this AnimSegment overlaps any of it, and if it does, break them up into a sequence of FRootMotionExtractionStep.
 * Supports animation playing forward and backward. Track range should be a contiguous range, not wrapping over due to looping.
 */
void FAnimSegment::GetRootMotionExtractionStepsForTrackRange(TArray<FRootMotionExtractionStep> & RootMotionExtractionSteps, const float StartTrackPosition, const float EndTrackPosition) const
{
	if( StartTrackPosition == EndTrackPosition )
	{
		return;
	}

	if (!bValid || !AnimReference)
	{
		return;
	}

		//check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
  // check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
	const bool bTrackPlayingBackwards = (StartTrackPosition > EndTrackPosition);

	const float SegmentStartPos = StartPos;
	const float SegmentEndPos = StartPos + GetLength();

	// if range overlaps segment
 // 如果范围与段重叠
	if (bTrackPlayingBackwards
		? ((EndTrackPosition < SegmentEndPos) && (StartTrackPosition > SegmentStartPos)) 
		: ((StartTrackPosition < SegmentEndPos) && (EndTrackPosition > SegmentStartPos)) 
		)
	{
		const float ValidPlayRate = GetValidPlayRate();
		const float AbsValidPlayRate = FMath::Abs(ValidPlayRate);

		const float StartTrackPositionForSegment = bTrackPlayingBackwards ? FMath::Min(StartTrackPosition, SegmentEndPos) : FMath::Max(StartTrackPosition, SegmentStartPos);
  // check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
  // check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
		const float EndTrackPositionForSegment = bTrackPlayingBackwards ? FMath::Max(EndTrackPosition, SegmentStartPos) : FMath::Min(EndTrackPosition, SegmentEndPos);
  // check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
  // check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );

		// Get starting position, closest overlap.
  // 获取起始位置，最接近的重叠。
		//check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
  // check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
		float AnimStartPosition = ConvertTrackPosToAnimPos(StartTrackPositionForSegment);
		AnimStartPosition = FMath::Clamp(AnimStartPosition, AnimStartTime, AnimEndTime);
		//check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
  // check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
		//check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
  // check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
		float TrackTimeToGo = FMath::Abs(EndTrackPositionForSegment - StartTrackPositionForSegment);

		// The track can be playing backwards and the animation can be playing backwards, so we
  // 曲目可以向后播放，动画也可以向后播放，所以我们
		// need to combine to work out what direction we are traveling through the animation
  // 需要结合起来找出我们在动画中行进的方向
		bool bAnimPlayingBackwards = bTrackPlayingBackwards ^ (ValidPlayRate < 0.f);
		const float ResetStartPosition = bAnimPlayingBackwards ? AnimEndTime : AnimStartTime;

		// Abstract out end point since animation can be playing forward or backward.
  // 由于动画可以向前或向后播放，所以抽象出终点。
		const float AnimEndPoint = bAnimPlayingBackwards ? AnimStartTime : AnimEndTime;

		// Only allow AnimSequences for now. Other types will need additional support.
  // 目前仅允许 AnimSequences。其他类型将需要额外的支持。
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimReference);
		UAnimComposite* AnimComposite = Cast<UAnimComposite>(AnimReference);

		if (AnimSequence || AnimComposite)
		{
			for(int32 IterationsLeft=FMath::Max(LoopingCount, 1); ((IterationsLeft > 0) && (TrackTimeToGo > 0.f)); --IterationsLeft)
			{
				// Track time left to reach end point of animation.
    // 跟踪到达动画终点的剩余时间。
				const float TrackTimeToAnimEndPoint = (AnimEndPoint - AnimStartPosition) / ValidPlayRate;

				// If our time left is shorter than time to end point, no problem. End there.
    // 如果剩下的时间比到达终点的时间短，没问题。到此结束。
				if( FMath::Abs(TrackTimeToGo) < FMath::Abs(TrackTimeToAnimEndPoint) )
				{
					const float PlayRate = ValidPlayRate * (bTrackPlayingBackwards ? -1.f : 1.f);
					const float AnimEndPosition = (TrackTimeToGo * PlayRate) + AnimStartPosition;
					if (AnimSequence)
					{
						RootMotionExtractionSteps.Add(FRootMotionExtractionStep(AnimSequence, AnimStartPosition, AnimEndPosition));
					}
					else if (AnimComposite)
					{
						AnimComposite->AnimationTrack.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, AnimStartPosition, AnimEndPosition);
					}
					break;
				}
				// Otherwise we hit the end point of the animation first...
    // 否则我们首先到达动画的终点......
				else
				{
					// Add that piece for extraction.
     // 添加该部分以进行提取。
					if (AnimSequence)
					{
						RootMotionExtractionSteps.Add(FRootMotionExtractionStep(AnimSequence, AnimStartPosition, AnimEndPoint));
					}
					else if (AnimComposite)
					{
						AnimComposite->AnimationTrack.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, AnimStartPosition, AnimEndPoint);
					}

					// decrease our TrackTimeToGo if we have to do another iteration.
     // 如果我们必须进行另一次迭代，则减少 TrackTimeToGo。
					// and put ourselves back at the beginning of the animation.
     // 并将我们自己带回到动画的开头。
					TrackTimeToGo -= TrackTimeToAnimEndPoint;
					AnimStartPosition = ResetStartPosition;
				}
			}
		}
	}
}

///////////////////////////////////////////////////////
// FAnimTrack
// 动画轨迹
///////////////////////////////////////////////////////
bool FAnimTrack::HasRootMotion() const
{
	for (const FAnimSegment& AnimSegment : AnimSegments)
	{
		const UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference();
		if (AnimSegment.bValid && AnimReference && AnimReference->HasRootMotion())
		{
			return true;
		}
	}
	return false;
}

#if WITH_EDITOR
class UAnimSequence* FAnimTrack::GetAdditiveBasePose() const
{
	if (IsAdditive())
	{
		for (const FAnimSegment& AnimSegment : AnimSegments)
		{
			UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference();
			UAnimSequence* BasePose = AnimReference ? AnimReference->GetAdditiveBasePose() : nullptr;
			if (BasePose)
			{
				return BasePose;
			}
		}
	}
	return nullptr;
}
#endif

/** 
 * Given a Track delta position [StartTrackPosition, EndTrackPosition]
 * See if any AnimSegment overlaps any of it, and if any do, break them up into a sequence of FRootMotionExtractionStep.
 * Supports animation playing forward and backward. Track range should be a contiguous range, not wrapping over due to looping.
 */
void FAnimTrack::GetRootMotionExtractionStepsForTrackRange(TArray<FRootMotionExtractionStep> & RootMotionExtractionSteps, const float StartTrackPosition, const float EndTrackPosition) const
{
	// must extract root motion in right order sequentially
 // 必须按正确的顺序连续提取根运动
	const bool bPlayingBackwards = (StartTrackPosition > EndTrackPosition);
	if( bPlayingBackwards )
	{
		for(int32 AnimSegmentIndex=AnimSegments.Num()-1; AnimSegmentIndex>=0; AnimSegmentIndex--)
		{
			const FAnimSegment& AnimSegment = AnimSegments[AnimSegmentIndex];
			AnimSegment.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, StartTrackPosition, EndTrackPosition);
		}
	}
	else
	{
		for(int32 AnimSegmentIndex=0; AnimSegmentIndex<AnimSegments.Num(); AnimSegmentIndex++)
		{
			const FAnimSegment& AnimSegment = AnimSegments[AnimSegmentIndex];
			AnimSegment.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, StartTrackPosition, EndTrackPosition);
		}
	}
}

float FAnimTrack::GetLength() const
{
	float TotalLength = 0.f;

	// in the future, if we're more clear about exactly what requirement is for segments, 
 // 将来，如果我们更清楚细分市场的具体要求，
	// this can be optimized. For now this is slow. 
 // 这是可以优化的。目前来说这很慢。
	for (const FAnimSegment& AnimSegment : AnimSegments)
	{
		const float EndFrame = AnimSegment.StartPos + AnimSegment.GetLength();
		if ( EndFrame > TotalLength )
		{
			TotalLength = EndFrame;
		}
	}

	return TotalLength;
}

bool FAnimTrack::IsAdditive() const
{
	// this function just checks first animation to verify if this is additive or not
 // 该函数仅检查第一个动画以验证这是否是附加的
	// if first one is additive, it returns true, 
 // 如果第一个是可加的，则返回 true，
	// the best way to handle isn't really practical. If I do go through all of them
 // 最好的处理方法并不实际。如果我真的经历了所有这些
	// and if they mismatch, what can I do? That should be another verification function when this is created
 // 如果它们不匹配，我该怎么办？创建时应该是另一个验证函数
	// it will look visually wrong if something mismatches, but nothing really is better solution than that. 
 // 如果某些东西不匹配，它在视觉上看起来会是错误的，但没有什么比这更好的解决方案了。
	// in editor, when this is created, the test has to be done to verify all are matches. 
 // 在编辑器中，当创建它时，必须进行测试以验证所有是否匹配。
	if (AnimSegments.Num() > 0)
	{
		const FAnimSegment& AnimSegment = AnimSegments[0];
		const UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference();
		return (AnimReference && AnimSegment.bValid && AnimReference->IsValidAdditive() ); //-V612
	}

	return false;
}

bool FAnimTrack::IsRotationOffsetAdditive() const
{
	// this function just checks first animation to verify if this is additive or not
 // 该函数仅检查第一个动画以验证这是否是附加的
	// if first one is additive, it returns true, 
 // 如果第一个是可加的，则返回 true，
	// the best way to handle isn't really practical. If I do go through all of them
 // 最好的处理方法并不实际。如果我真的经历了所有这些
	// and if they mismatch, what can I do? That should be another verification function when this is created
 // 如果它们不匹配，我该怎么办？创建时应该是另一个验证函数
	// it will look visually wrong if something mismatches, but nothing really is better solution than that. 
 // 如果某些东西不匹配，它在视觉上看起来会是错误的，但没有什么比这更好的解决方案了。
	// in editor, when this is created, the test has to be done to verify all are matches. 
 // 在编辑器中，当创建它时，必须进行测试以验证所有是否匹配。
	if (AnimSegments.Num() > 0)
	{
		const FAnimSegment& AnimSegment = AnimSegments[0];
		const UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference();
		if (AnimReference && AnimReference->IsValidAdditive())
		{
			return (AnimReference->GetAdditiveAnimType() == AAT_RotationOffsetMeshSpace);
		}
	}

	return false;
}

int32 FAnimTrack::GetTrackAdditiveType() const
{
	// this function just checks first animation to verify the type
 // 该函数仅检查第一个动画以验证类型
	// the best way to handle isn't really practical. If I do go through all of them
 // 最好的处理方法并不实际。如果我真的经历了所有这些
	// and if they mismatch, what can I do? That should be another verification function when this is created
 // 如果它们不匹配，我该怎么办？创建时应该是另一个验证函数
	// it will look visually wrong if something mismatches, but nothing really is better solution than that. 
 // 如果某些东西不匹配，它在视觉上看起来会是错误的，但没有什么比这更好的解决方案了。
	// in editor, when this is created, the test has to be done to verify all are matches. 
 // 在编辑器中，当创建它时，必须进行测试以验证所有是否匹配。

	if( AnimSegments.Num() > 0 )
	{
		const FAnimSegment& AnimSegment = AnimSegments[0];
		if (const UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference())
		{
			return AnimReference->GetAdditiveAnimType();
		}
	}
	return -1;
}

void FAnimTrack::ValidateSegmentTimes()
{
	// rearrange, make sure there are no gaps between and all start times are correctly set
 // 重新安排，确保之间没有间隙，并且所有开始时间均已正确设置
	if(AnimSegments.Num() > 0)
	{
		AnimSegments[0].StartPos = 0.0f;
		for(int32 SegmentIndex = 0; SegmentIndex < AnimSegments.Num(); SegmentIndex++)
		{
			FAnimSegment& AnimSegment = AnimSegments[SegmentIndex];
			if(SegmentIndex > 0)
			{
				AnimSegment.StartPos = AnimSegments[SegmentIndex - 1].StartPos + AnimSegments[SegmentIndex - 1].GetLength();
			}

			const UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference();
			if(AnimReference && AnimSegment.AnimEndTime > AnimReference->GetPlayLength())
			{
				AnimSegment.AnimEndTime = AnimReference->GetPlayLength();
			}
		}
	}
}

FAnimSegment* FAnimTrack::GetSegmentAtTime(float InTime)
{
	const int32 SegmentIndex = GetSegmentIndexAtTime(InTime);
	return (SegmentIndex != INDEX_NONE) ? &(AnimSegments[SegmentIndex]) : nullptr;
}

const FAnimSegment* FAnimTrack::GetSegmentAtTime(float InTime) const
{
	const int32 SegmentIndex = GetSegmentIndexAtTime(InTime);
	return (SegmentIndex != INDEX_NONE) ? &(AnimSegments[SegmentIndex]) : nullptr;
}

int32 FAnimTrack::GetSegmentIndexAtTime(float InTime) const
{
	// Montage Segments overlap on a single frame.
 // 蒙太奇片段重叠在单个帧上。
	// So last frame of Segment1 overlaps first frame of Segment2.
 // 因此 Segment1 的最后一帧与 Segment2 的第一帧重叠。
	// But in that case we want Segment2 to win.
 // 但在这种情况下，我们希望 Segment2 获胜。
	// So we iterate through these segments in reverse 
 // 所以我们反向迭代这些段
	// and return the first match with an inclusive range check.
 // 并返回第一个包含范围检查的匹配项。
	for(int32 Idx = AnimSegments.Num()-1; Idx >= 0; Idx--)
	{
		const FAnimSegment& Segment = AnimSegments[Idx];
		if (Segment.IsInRange(InTime))
		{
			return Idx;
		}
	}
	return INDEX_NONE;
}

#if WITH_EDITOR
bool FAnimTrack::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive/* = true*/) const
{
	for (const FAnimSegment& AnimSegment : AnimSegments)
	{
		UAnimSequenceBase* AnimSeqBase = AnimSegment.GetAnimReference();
		if ( AnimSegment.bValid && AnimSeqBase )
		{
			AnimSeqBase->HandleAnimReferenceCollection(AnimationAssets, bRecursive);
		}
	}

	return (AnimationAssets.Num() > 0 );
}

void FAnimTrack::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap)
{
	TArray<FAnimSegment> NewAnimSegments;
	for (FAnimSegment& AnimSegment : AnimSegments)
	{
		if (AnimSegment.IsValid())
		{
			if (UAnimSequenceBase* SequenceBase = AnimSegment.GetAnimReference())
			{
				if(UAnimationAsset* const* ReplacementAsset = ReplacementMap.Find(SequenceBase))
				{
					AnimSegment.SetAnimReference(Cast<UAnimSequenceBase>(*ReplacementAsset));
					NewAnimSegments.Add(AnimSegment);
				}

				SequenceBase->ReplaceReferredAnimations(ReplacementMap);
			}
		}
	}

	AnimSegments = NewAnimSegments;
}

void FAnimTrack::CollapseAnimSegments()
{
	if(AnimSegments.Num() > 0)
	{
		// Sort function
  // 排序功能
		struct FSortFloatInt
		{
			bool operator()( const TKeyValuePair<float, int32> &A, const TKeyValuePair<float, int32>&B ) const
			{
				return A.Key < B.Key;
			}
		};

		// Create sorted map of start time to segment
  // 创建分段开始时间的排序映射
		TArray<TKeyValuePair<float, int32>> m;
		for( int32 SegmentInd=0; SegmentInd < AnimSegments.Num(); ++SegmentInd )
		{
			m.Add(TKeyValuePair<float, int32>(AnimSegments[SegmentInd].StartPos, SegmentInd));
		}
		m.Sort(FSortFloatInt());

		//collapse all start times based on sorted map
  // 根据排序地图折叠所有开始时间
		FAnimSegment* PrevSegment = &AnimSegments[m[0].Value];
		PrevSegment->StartPos = 0.0f;

		for ( int32 SegmentInd=1; SegmentInd < m.Num(); ++SegmentInd )
		{
			FAnimSegment* CurrSegment = &AnimSegments[m[SegmentInd].Value];
			CurrSegment->StartPos = PrevSegment->StartPos + PrevSegment->GetLength();
			PrevSegment = CurrSegment;
		}
	}
}

void FAnimTrack::SortAnimSegments()
{
	if(AnimSegments.Num() > 0)
	{
		struct FCompareSegments
		{
			bool operator()( const FAnimSegment &A, const FAnimSegment&B ) const
			{
				return A.StartPos < B.StartPos;
			}
		};

		AnimSegments.Sort( FCompareSegments() );

		ValidateSegmentTimes();
	}
}
#endif

void FAnimTrack::GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const
{
	bool bExtractedPose = false;
	const float ClampedTime = FMath::Clamp(ExtractionContext.CurrentTime, 0.f, GetLength());

	if (const FAnimSegment* const AnimSegment = GetSegmentAtTime(ClampedTime))
	{
		if (AnimSegment->bValid)
		{
			// Copy passed in Extraction Context, but override position and root motion parameters.
   // 复制在提取上下文中传递，但覆盖位置和根运动参数。
			float PositionInAnim = 0.f;
			if (const UAnimSequenceBase* const AnimRef = AnimSegment->GetAnimationData(ClampedTime, PositionInAnim))
			{
				FAnimExtractContext SequenceExtractionContext(ExtractionContext);
				SequenceExtractionContext.CurrentTime = static_cast<double>(PositionInAnim);
				SequenceExtractionContext.DeltaTimeRecord.SetPrevious(
					SequenceExtractionContext.CurrentTime - SequenceExtractionContext.DeltaTimeRecord.Delta);
				SequenceExtractionContext.bExtractRootMotion &= AnimRef->HasRootMotion();
				SequenceExtractionContext.bLooping = AnimSegment->LoopingCount > 1;
				AnimRef->GetAnimationPose(OutAnimationPoseData, SequenceExtractionContext);
				bExtractedPose = true;
			}
		}
	}

	if (!bExtractedPose)
	{
		OutAnimationPoseData.GetPose().ResetToRefPose();
	}
}

void FAnimTrack::EnableRootMotionSettingFromMontage(bool bInEnableRootMotion, const ERootMotionRootLock::Type InRootMotionRootLock)
{
	for (const FAnimSegment& AnimSegment : AnimSegments)
	{
		if (UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference())
		{
			AnimReference->EnableRootMotionSettingFromMontage(bInEnableRootMotion, InRootMotionRootLock);
		}
	}
}

// this is to prevent anybody adding recursive asset to anim composite
// 这是为了防止任何人将递归资产添加到动画组合中
// as a result of anim composite being a part of anim sequence base
// 由于动画合成是动画序列基础的一部分
void FAnimTrack::InvalidateRecursiveAsset(class UAnimCompositeBase* CheckAsset)
{
	for (FAnimSegment& AnimSegment : AnimSegments)
	{
		UAnimSequenceBase* SequenceBase = AnimSegment.GetAnimReference();
		UAnimCompositeBase* CompositeBase = Cast<UAnimCompositeBase>(SequenceBase);
		if (CompositeBase)
		{
			// add owner
   // 添加所有者
			TArray<UAnimCompositeBase*> CompositeBaseRecurisve;
			CompositeBaseRecurisve.Add(CheckAsset);

			if (CompositeBase->ContainRecursive(CompositeBaseRecurisve))
			{
				AnimSegment.bValid = false;
			}
			else
			{
				AnimSegment.bValid = IsValidToAdd(CompositeBase);
			}
		}
		else
		{
			AnimSegment.bValid = IsValidToAdd(SequenceBase);
		}
	}
}

// this is recursive function that look thorough internal assets 
// 这是递归函数，可以彻底查看内部资产
// and return true if it finds nested same assets
// 如果找到嵌套的相同资产则返回 true
bool FAnimTrack::ContainRecursive(const TArray<UAnimCompositeBase*>& CurrentAccumulatedList)
{
	for (const FAnimSegment& AnimSegment : AnimSegments)
	{
		// we don't want to send this list broad widely (but in depth search)
  // 我们不想广泛发送此列表（但要进行深度搜索）
		// to do that, we copy the current accumulated list, and send that only, not the siblings
  // 为此，我们复制当前累积的列表，并仅发送该列表，而不发送兄弟姐妹
		TArray<UAnimCompositeBase*> LocalCurrentAccumulatedList = CurrentAccumulatedList;
		UAnimCompositeBase* CompositeBase = Cast<UAnimCompositeBase>(AnimSegment.GetAnimReference());
		if (CompositeBase && CompositeBase->ContainRecursive(LocalCurrentAccumulatedList))
		{
			return true;
		}
	}

	return false;
}

void FAnimTrack::GetAnimNotifiesFromTrackPositions(const float& PreviousTrackPosition, const float& CurrentTrackPosition, FAnimNotifyContext& NotifyContext) const
{
	for (int32 SegmentIndex = 0; SegmentIndex<AnimSegments.Num(); ++SegmentIndex)
	{
		if (AnimSegments[SegmentIndex].IsValid())
// UAnimCompositeBase
// UAnimCompositeBase
		{
			AnimSegments[SegmentIndex].GetAnimNotifiesFromTrackPositions(PreviousTrackPosition, CurrentTrackPosition, NotifyContext);
		}
	}
}

bool FAnimTrack::IsNotifyAvailable() const
{
	for (int32 SegmentIndex = 0; SegmentIndex < AnimSegments.Num(); ++SegmentIndex)
	{
		if (AnimSegments[SegmentIndex].IsNotifyAvailable())
		{
			return true;
		}
	}

	return false;
}

int32 FAnimTrack::GetTotalBytesUsed() const
{
	return AnimSegments.GetAllocatedSize();
}

bool FAnimTrack::IsValidToAdd(const UAnimSequenceBase* SequenceBase, FText* OutReason /*= nullptr*/) const
{
	bool bValid = false;
	// remove asset if invalid
 // 如果资产无效，则删除资产
	if (SequenceBase)
	{
		const float PlayLength = SequenceBase->GetPlayLength();
		if (PlayLength <= 0.f)
		{
			UE_LOG(LogAnimation, Warning, TEXT("Remove Empty Sequence (%s)"), *SequenceBase->GetFullName());

			if (OutReason)
			{
				*OutReason = FText::FromString(FString::Printf(TEXT("Animation Asset %s has invalid playable length of %f"), *SequenceBase->GetName(), PlayLength));
			}			
			
			return false;
		}

		if (!SequenceBase->CanBeUsedInComposition())
		{
			UE_LOG(LogAnimation, Warning, TEXT("Remove Invalid Sequence (%s)"), *SequenceBase->GetFullName());
			if (OutReason)
			{
				*OutReason = FText::FromString(FString::Printf(TEXT("Animation Asset %s cannot be used in an Animation Composite/Montage"), *SequenceBase->GetName()));
			}
			return false;
		}
		
		const int32 TrackType = GetTrackAdditiveType();
		const EAdditiveAnimationType AnimAdditiveType = SequenceBase->GetAdditiveAnimType();
		if (TrackType != AnimAdditiveType && TrackType != INDEX_NONE)
		{
			const UEnum* TypeEnum = FindObject<UEnum>(nullptr, TEXT("/Script/Engine.EAdditiveAnimationType"));	
			if (OutReason)
			{
				*OutReason = FText::FromString(FString::Printf(TEXT("Animation Asset %s has an additive type %s that does not match the target's %s"), *SequenceBase->GetName(), *TypeEnum->GetNameStringByValue(AnimAdditiveType), *TypeEnum->GetNameStringByValue(TrackType)));
			}
   // UAnimCompositeBase
   // UAnimCompositeBase
			return false;
		}
  // UAnimCompositeBase
  // UAnimCompositeBase
		
		return true;
	}
// UAnimCompositeBase
// UAnimCompositeBase

	return true;
}
///////////////////////////////////////////////////////
// UAnimCompositeBase
// UAnimCompositeBase
// UAnimCompositeBase
// UAnimCompositeBase
///////////////////////////////////////////////////////

UAnimCompositeBase::UAnimCompositeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAnimCompositeBase::ExtractRootMotionFromTrack(const FAnimTrack& SlotAnimTrack, float StartTrackPosition, float EndTrackPosition, const FAnimExtractContext& Context, FRootMotionMovementParams& RootMotion) const
{
	TArray<FRootMotionExtractionStep> RootMotionExtractionSteps;
	SlotAnimTrack.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, StartTrackPosition, EndTrackPosition);

	UE_LOG(LogRootMotion, Verbose, TEXT("\tUAnimCompositeBase::ExtractRootMotionFromTrack, NumSteps: %d, StartTrackPosition: %.3f, EndTrackPosition: %.3f"),
		RootMotionExtractionSteps.Num(), StartTrackPosition, EndTrackPosition);

	// Go through steps sequentially, extract root motion, and accumulate it.
 // 按顺序执行步骤，提取根运动并累积它。
	// This has to be done in order so root motion translation & rotation is applied properly (as translation is relative to rotation)
 // 这必须按顺序完成，以便正确应用根运动平移和旋转（因为平移是相对于旋转的）
	for (int32 StepIndex = 0; StepIndex < RootMotionExtractionSteps.Num(); StepIndex++)
	{
		const FRootMotionExtractionStep & CurrentStep = RootMotionExtractionSteps[StepIndex];
		if (CurrentStep.AnimSequence->bEnableRootMotion)
		{
			FTransform DeltaTransform = CurrentStep.AnimSequence->ExtractRootMotionFromRange(CurrentStep.StartPosition, CurrentStep.EndPosition, Context);
			RootMotion.Accumulate(DeltaTransform);
		
			UE_LOG(LogRootMotion, Log, TEXT("\t\tCurrentStep: %d, StartPos: %.3f, EndPos: %.3f, Anim: %s DeltaTransform Translation: %s, Rotation: %s"),
				StepIndex, CurrentStep.StartPosition, CurrentStep.EndPosition, *CurrentStep.AnimSequence->GetName(),
				*DeltaTransform.GetTranslation().ToCompactString(), *DeltaTransform.GetRotation().Rotator().ToCompactString());
		}
	}
}

FFrameRate UAnimCompositeBase::GetSamplingFrameRate() const
{
	// Allowing for 0.00001s precision in composite/montage length
 // 复合/蒙太奇长度允许 0.00001 秒的精度
	static const FFrameRate CompositeFrameRate(100000, 1);
	return CompositeFrameRate;
}

void UAnimCompositeBase::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	UpdateCommonTargetFrameRate();
#endif // WITH_EDITOR

	InvalidateRecursiveAsset();
}

void FAnimSegment::SetAnimReference(UAnimSequenceBase* InAnimReference, bool bInitialize /*= false*/)
{
	AnimReference = InAnimReference;

#if WITH_EDITOR
	UpdateCachedPlayLength();
#endif // WITH_EDITOR

	if (AnimReference && bInitialize)
	{		
		AnimStartTime = 0.f;
		AnimEndTime = AnimReference->GetPlayLength();
		AnimPlayRate = 1.f;
		LoopingCount = 1;
		StartPos = 0.f;
	}
}

#if WITH_EDITOR
bool FAnimSegment::IsPlayLengthOutOfDate() const
{
	if (AnimReference && !FMath::IsNearlyZero(CachedPlayLength))
	{
		// When the segment length is equal to _cached_ playlength and the current model playlength is different flag as out-of-date
  // 当片段长度等于 _cached_ playlength 且当前模型 playlength 不同时，标记为过时
		// this can happen when the sequence is reimported without updating the montage and thus ending up with 'invalid' playback range.
  // 当重新导入序列而不更新剪辑并最终导致“无效”播放范围时，可能会发生这种情况。
		const float PlayableLength = (AnimEndTime - AnimStartTime);
		return FMath::IsNearlyEqual(PlayableLength, CachedPlayLength, UE_KINDA_SMALL_NUMBER) && !FMath::IsNearlyEqual(AnimReference->GetPlayLength(), CachedPlayLength, UE_KINDA_SMALL_NUMBER);
	}
	
	return false;
}

void FAnimSegment::UpdateCachedPlayLength()
{
	CachedPlayLength = 0.f;
	const IAnimationDataModel* DataModel = AnimReference ? AnimReference->GetDataModel() : nullptr;	
	if(DataModel)
	{
		CachedPlayLength = DataModel->GetPlayLength();
	}
}

void UAnimCompositeBase::PopulateWithExistingModel(TScriptInterface<IAnimationDataModel> ExistingDataModel)
{
	Super::PopulateWithExistingModel(ExistingDataModel);
	Controller->SetFrameRate(GetSamplingFrameRate());
}
#endif // WITH_EDITOR