// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimLinkableElement.h"
#include "Animation/AnimMontage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimLinkableElement)

void FAnimLinkableElement::LinkMontage(UAnimMontage* Montage, float AbsMontageTime, int32 InSlotIndex)
{
	Link(Montage, AbsMontageTime, InSlotIndex);
}

void FAnimLinkableElement::LinkSequence(UAnimSequenceBase* Sequence, float AbsSequenceTime)
{
	Link(Sequence, AbsSequenceTime);
}

void FAnimLinkableElement::Clear()
{
	ChangeLinkMethod(EAnimLinkMethod::Absolute);
	LinkedSequence = nullptr;
	SegmentBeginTime = -1.0f;
	SegmentLength = -1.0f;
	SegmentIndex = INDEX_NONE;
}

void FAnimLinkableElement::Update()
{
	if(LinkedMontage && LinkedMontage->SlotAnimTracks.IsValidIndex(SlotIndex))
	{
		FSlotAnimationTrack& Slot = LinkedMontage->SlotAnimTracks[SlotIndex];
		float CurrentTime = GetTime();

		// If we don't have a segment, check to see if one has been added.
  // 如果我们没有分段，请检查是否已添加分段。
		if(SegmentIndex == INDEX_NONE || !Slot.AnimTrack.AnimSegments.IsValidIndex(SegmentIndex))
		{
			SegmentIndex = Slot.AnimTrack.GetSegmentIndexAtTime(CurrentTime);
		}

		if(SegmentIndex != INDEX_NONE)
		{
			// Update timing info from current segment
   // 更新当前段的计时信息
			FAnimSegment& Segment = Slot.AnimTrack.AnimSegments[SegmentIndex];
			LinkedSequence = Segment.GetAnimReference();
			SegmentBeginTime = Segment.StartPos;
			SegmentLength = Segment.GetLength();

			// Handle Relative link mode, make sure to stay within the linked segment
   // 处理相对链接模式，确保保持在链接段内
			if(CachedLinkMethod == EAnimLinkMethod::Relative)
			{
				float SegmentEnd = SegmentBeginTime + SegmentLength;
				if(GetTime() > SegmentEnd)
				{
					SetTime(SegmentEnd);
				}
			}

			// Relink if necessary
   // 必要时重新链接
			ConditionalRelink();
		}
	}
}

void FAnimLinkableElement::OnChanged(float NewMontageTime)
{
	// Only update linkage in a montage.
 // 仅更新蒙太奇中的链接。
	if(!LinkedMontage)
	{
		return;
	}

	SlotIndex = FMath::Clamp(SlotIndex, 0, LinkedMontage->SlotAnimTracks.Num()-1);

	// If the link method changed, transform the link value
 // 如果链接方法改变，则转换链接值
	if(CachedLinkMethod != LinkMethod)
	{
		float AbsTime = -1.0f;
		switch(CachedLinkMethod)
		{
			case EAnimLinkMethod::Absolute:
				AbsTime = LinkValue;
				break;
			case EAnimLinkMethod::Relative:
				AbsTime = GetTimeFromRelative(EAnimLinkMethod::Absolute);
				break;
			case EAnimLinkMethod::Proportional:
				AbsTime = GetTimeFromProportional(EAnimLinkMethod::Absolute);
				break;
		}
		check(AbsTime != -1.0f);
		CachedLinkMethod = LinkMethod;

		// We aren't changing the time, just transforming it so use internal settime
  // 我们不会改变时间，只是改变它，所以使用内部设置时间
		SetTime_Internal(AbsTime);
		NewMontageTime = AbsTime;
	}

	FSlotAnimationTrack& Slot = LinkedMontage->SlotAnimTracks[SlotIndex];

	SegmentIndex = Slot.AnimTrack.GetSegmentIndexAtTime(NewMontageTime);
	if(SegmentIndex != INDEX_NONE)
	{
		// Update to the detected segment
  // 更新检测到的段
		FAnimSegment& Segment = Slot.AnimTrack.AnimSegments[SegmentIndex];
		LinkedSequence = Segment.GetAnimReference();
		SegmentBeginTime = Segment.StartPos;
		SegmentLength = Segment.GetLength();

		SetTime(NewMontageTime);
	}
	else if(!LinkedSequence)
	{
		// We have no segment to link to, we need to clear our the segment data
  // 我们没有要链接的段，我们需要清除段数据
		// and give ourselves an absolute time
  // 并给自己一个绝对的时间
		LinkValue = NewMontageTime;
		Clear();
	}
}

FAnimSegment* FAnimLinkableElement::GetSegmentAtCurrentTime()
{
	FAnimSegment* Result = nullptr;
	if(LinkedMontage)
	{
		Result = LinkedMontage->SlotAnimTracks[SlotIndex].AnimTrack.GetSegmentAtTime(GetTime());
	}
	return Result;
}

float FAnimLinkableElement::GetTime(EAnimLinkMethod::Type ReferenceFrame /*= EMontageLinkMethod::Absolute*/) const
{
	if(ReferenceFrame != CachedLinkMethod)
	{
		switch(CachedLinkMethod)
		{
			case EAnimLinkMethod::Absolute:
			{
				return GetTimeFromAbsolute(ReferenceFrame);
			}
			case EAnimLinkMethod::Relative:
			{
				return GetTimeFromRelative(ReferenceFrame);
			}
			case EAnimLinkMethod::Proportional:
			{
				return GetTimeFromProportional(ReferenceFrame);
			}
		}
	}
	return LinkValue;
}

void FAnimLinkableElement::SetTime(float NewTime, EAnimLinkMethod::Type ReferenceFrame /*= EMontageLinkMethod::Absolute*/)
{
	SetTime_Internal(NewTime, ReferenceFrame);
}

float FAnimLinkableElement::GetTimeFromAbsolute(EAnimLinkMethod::Type ReferenceFrame) const
{
	switch(ReferenceFrame)
	{
		case EAnimLinkMethod::Relative:
		{
			return LinkValue - SegmentBeginTime;
		}
		case EAnimLinkMethod::Proportional:
		{
			return (LinkValue - SegmentBeginTime) / SegmentLength;
		}
	}
	return -1.0f;
}

float FAnimLinkableElement::GetTimeFromRelative(EAnimLinkMethod::Type ReferenceFrame) const
{
	switch(ReferenceFrame)
	{
		case EAnimLinkMethod::Absolute:
		{
			return SegmentBeginTime + LinkValue;
		}
		case EAnimLinkMethod::Proportional:
		{
			return LinkValue / SegmentLength;
		}
	}
	return -1.0f;
}

float FAnimLinkableElement::GetTimeFromProportional(EAnimLinkMethod::Type ReferenceFrame) const
{
	switch(ReferenceFrame)
	{
		case EAnimLinkMethod::Absolute:
		{
			return SegmentBeginTime + LinkValue * SegmentLength;
		}
		case EAnimLinkMethod::Relative:
		{
			return LinkValue * SegmentLength;
		}
	}
	return -1.0f;
}

void FAnimLinkableElement::SetTimeFromAbsolute(float NewTime, EAnimLinkMethod::Type ReferenceFrame)
{
	switch(ReferenceFrame)
	{
		case EAnimLinkMethod::Relative:
		{
			LinkValue = SegmentBeginTime + NewTime;
		}
		case EAnimLinkMethod::Proportional:
		{
			LinkValue = SegmentBeginTime + SegmentLength * NewTime;
		}
	}
}

void FAnimLinkableElement::SetTimeFromRelative(float NewTime, EAnimLinkMethod::Type ReferenceFrame)
{
	switch(ReferenceFrame)
	{
		case EAnimLinkMethod::Absolute:
		{
			LinkValue = NewTime - SegmentBeginTime;
		}
			break;
		case EAnimLinkMethod::Proportional:
		{
			LinkValue = NewTime * SegmentLength;
		}
			break;
	}
}

void FAnimLinkableElement::SetTimeFromProportional(float NewTime, EAnimLinkMethod::Type ReferenceFrame)
{
	switch(ReferenceFrame)
	{
		case EAnimLinkMethod::Absolute:
		{
			if(SegmentLength != 0.0f)
			{
				LinkValue = (NewTime - SegmentBeginTime) / SegmentLength;
			}
			else
			{
				// if segment length is 0, we can't set anywhere else but 0.f
    // 如果段长度为0，我们不能设置除了0.f之外的任何其他地方
				LinkValue = 0.f;
			}
		}
			break;
		case EAnimLinkMethod::Relative:
		{
			if(SegmentLength != 0.0f)
			{
				LinkValue = NewTime / SegmentLength;
			}
			else
			{
				// if segment length is 0, we can't set anywhere else but 0.f
    // 如果段长度为0，我们不能设置除了0.f之外的任何其他地方
				LinkValue = 0.f;			
			}
		}
			break;
	}
}

void FAnimLinkableElement::ChangeLinkMethod(EAnimLinkMethod::Type NewLinkMethod)
{
	if(NewLinkMethod != LinkMethod)
	{
		// Switch to the new link method and resolve it
  // 切换到新的链接方式并解决
		LinkMethod = NewLinkMethod;
		OnChanged(GetTime());
	}
}

void FAnimLinkableElement::ChangeSlotIndex(int32 NewSlotIndex)
{
	if(LinkedMontage)
	{
		Link(LinkedMontage, GetTime(), NewSlotIndex);
	}
}

void FAnimLinkableElement::SetTime_Internal(float NewTime, EAnimLinkMethod::Type ReferenceFrame)
{
	if(ReferenceFrame != CachedLinkMethod)
	{
		switch(CachedLinkMethod)
		{
			case EAnimLinkMethod::Absolute:
			{
				SetTimeFromAbsolute(NewTime, ReferenceFrame);
			}
				break;
			case EAnimLinkMethod::Relative:
			{
				SetTimeFromRelative(NewTime, ReferenceFrame);
			}
				break;
			case EAnimLinkMethod::Proportional:
			{
				SetTimeFromProportional(NewTime, ReferenceFrame);
			}
				break;
		}
	}
	else
	{
		LinkValue = NewTime;
	}
}

bool FAnimLinkableElement::ConditionalRelink()
{
	// Check slot index if we're in a montage
 // 如果我们处于蒙太奇中，请检查插槽索引
	bool bRequiresRelink = false;
	
	if(LinkedMontage)
	{
		if(!LinkedMontage->SlotAnimTracks.IsValidIndex(SlotIndex))
		{
			bRequiresRelink = true;
			SlotIndex = 0;
		}
	}

	// Check to see if we've moved to a new segment
 // 检查我们是否已移至新航段
	float CurrentAbsTime = GetTime();
	if(CurrentAbsTime < SegmentBeginTime || CurrentAbsTime > SegmentBeginTime + SegmentLength)
	{
		bRequiresRelink = true;
	}

	if(bRequiresRelink)
	{
		if(LinkedMontage)
		{
			Link(LinkedMontage, CurrentAbsTime, SlotIndex);
		}
		else if(LinkedSequence)
		{
			Link(LinkedSequence, CurrentAbsTime);
		}
	}

	return bRequiresRelink;
}

void FAnimLinkableElement::Link(UAnimSequenceBase* AnimSequenceBase, float AbsTime, int32 InSlotIndex /*= 0*/)
{
	if(UAnimMontage* Montage = Cast<UAnimMontage>(AnimSequenceBase))
	{
		if(Montage->SlotAnimTracks.Num() > 0)
		{
			LinkedMontage = Montage;

			SlotIndex = InSlotIndex;
			FSlotAnimationTrack& Slot = Montage->SlotAnimTracks[SlotIndex];

			SegmentIndex = Slot.AnimTrack.GetSegmentIndexAtTime(AbsTime);
			if(SegmentIndex != INDEX_NONE)
			{
				const FAnimSegment& Segment = Slot.AnimTrack.AnimSegments[SegmentIndex];
				LinkedSequence = Segment.GetAnimReference();
				SegmentBeginTime = Segment.StartPos;
				SegmentLength = Segment.GetLength();

				SetTime_Internal(AbsTime);
			}
			else
			{
				// Nothing to link to
    // 没有可链接的内容
				// We have no segment to link to, we need to clear our the segment data
    // 我们没有要链接的段，我们需要清除段数据
				// and give ourselves an absolute time
    // 并给自己一个绝对的时间
				LinkValue = AbsTime;
				LinkedSequence = nullptr;
				SegmentBeginTime = -1.0f;
				SegmentLength = -1.0f;
				LinkMethod = EAnimLinkMethod::Absolute;
				CachedLinkMethod = LinkMethod;
			}
		}
	}
	else if (AnimSequenceBase && AnimSequenceBase->GetPlayLength() > 0)
	{		
		LinkedMontage = nullptr;
		LinkedSequence = AnimSequenceBase;
		SegmentIndex = 0;

		SegmentBeginTime = 0.0f;
		SegmentLength = AnimSequenceBase->GetPlayLength();

		SetTime(AbsTime);
	}
}

void FAnimLinkableElement::RefreshSegmentOnLoad()
{
	// We only perform this step if we have valid data from a previous link
 // 仅当我们从先前的链接获得有效数据时，我们才执行此步骤
	if(LinkedMontage && SegmentIndex != INDEX_NONE && SlotIndex != INDEX_NONE)
	{
		if(LinkedMontage->SlotAnimTracks.IsValidIndex(SlotIndex))
		{
			FSlotAnimationTrack& Slot = LinkedMontage->SlotAnimTracks[SlotIndex];
			if(Slot.AnimTrack.AnimSegments.IsValidIndex(SegmentIndex))
			{
				FAnimSegment& Segment = Slot.AnimTrack.AnimSegments[SegmentIndex];

				if(Segment.GetAnimReference() == LinkedSequence)
				{
					if(CachedLinkMethod == EAnimLinkMethod::Relative)
					{
						LinkValue = FMath::Clamp<float>(LinkValue, 0.0f, Segment.GetLength());
					}

					// Update timing
     // 更新时间
					SegmentBeginTime = Segment.StartPos;
					SegmentLength = Segment.GetLength();
				}
			}
		}
	}
}

