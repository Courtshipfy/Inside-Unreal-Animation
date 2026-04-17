// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimMontage.cpp: Montage classes that contains slots
=============================================================================*/ 

#include "Animation/AnimMontage.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "UObject/LinkerLoad.h"
#include "Animation/AnimData/CurveIdentifier.h"
#include "UObject/ObjectSaveContext.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "UObject/UObjectThreadContext.h"
#include "Animation/AssetMappingTable.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendProfile.h"
#include "AnimationUtils.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimTrace.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimationSettings.h"
#include "Logging/StructuredLog.h"
#include "UObject/FortniteMainBranchObjectVersion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimMontage)

DEFINE_LOG_CATEGORY(LogAnimMontage);

DECLARE_CYCLE_STAT(TEXT("AnimMontageInstance_Advance"), STAT_AnimMontageInstance_Advance, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("AnimMontageInstance_TickBranchPoints"), STAT_AnimMontageInstance_TickBranchPoints, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("AnimMontageInstance_Advance_Iteration"), STAT_AnimMontageInstance_Advance_Iteration, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("AnimMontageInstance_Terminate"), STAT_AnimMontageInstance_Terminate, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("AnimMontageInstance_HandleEvents"), STAT_AnimMontageInstance_HandleEvents, STATGROUP_Anim);

// Pre-built FNames so we don't take the hit of constructing FNames at spawn time
// 预构建 FName，这样我们就不会在生成时构建 FName
namespace MontageFNames
{
	static FName TimeStretchCurveName(TEXT("MontageTimeStretchCurve"));
}

 // CVars
 // CVars
 // CVars
 // CVars
// CVars
// CVars
// CVars
// CVars
// CVars
// CVars
// CVars
// CVars
namespace MontageCVars
{
	static bool bEndSectionRequiresTimeRemaining = false;
	static FAutoConsoleVariableRef CVarMontageEndSectionRequiresTimeRemaining(
		TEXT("a.Montage.EndSectionRequiresTimeRemaining"),
		bEndSectionRequiresTimeRemaining,
		TEXT("Montage EndOfSection is only checked if there is remaining time (default false)."));
	extern bool bEarlyOutMontageWhenUninitialized;
} // end namespace MontageCVars

///////////////////////////////////////////////////////////////////////////
//
UAnimMontage::UAnimMontage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BlendModeIn = EMontageBlendMode::Standard;
	BlendModeOut = EMontageBlendMode::Standard;

	BlendIn.SetBlendTime(0.25f);
	BlendOut.SetBlendTime(0.25f);
	BlendOutTriggerTime = -1.f;
	bEnableAutoBlendOut = true;
	SyncSlotIndex = 0;

	BlendProfileIn = nullptr;
	BlendProfileOut = nullptr;

#if WITH_EDITORONLY_DATA
	BlendInTime_DEPRECATED = -1.f;
	BlendOutTime_DEPRECATED = -1.f;
#endif

	AddSlot(FAnimSlotGroup::DefaultSlotName);

	TimeStretchCurveName = MontageFNames::TimeStretchCurveName;
}

FSlotAnimationTrack& UAnimMontage::AddSlot(FName SlotName)
{
	int32 NewSlot = SlotAnimTracks.AddDefaulted(1);
	SlotAnimTracks[NewSlot].SlotName = SlotName;
	return SlotAnimTracks[NewSlot];
}

bool UAnimMontage::IsValidSlot(FName InSlotName) const
{
	for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
	{
		if ( SlotAnimTracks[I].SlotName == InSlotName )
		{
			// if data is there, return true. Otherwise, it doesn't matter
   // 如果有数据，则返回 true。否则，没关系
			return ( SlotAnimTracks[I].AnimTrack.AnimSegments.Num() >  0 );
		}
	}

	return false;
}

bool UAnimMontage::IsDynamicMontage() const
{
	return GetPackage() == GetTransientPackage();
}

UAnimSequenceBase* UAnimMontage::GetFirstAnimReference() const
{
	if(!SlotAnimTracks.IsEmpty() && !SlotAnimTracks[0].AnimTrack.AnimSegments.IsEmpty())
	{
		return SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference();
	}

	return nullptr;
}

#if WITH_EDITORONLY_DATA
void UAnimMontage::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FFortniteMainBranchObjectVersion::GUID);
	if (Ar.IsLoading())
	{
		const int32 CustomVersion = Ar.CustomVer(FFortniteMainBranchObjectVersion::GUID);
		if (CustomVersion < FFortniteMainBranchObjectVersion::ChangeDefaultAlphaBlendType)
		{
			// Switch the default back to Linear so old data remains the same
   // 将默认值切换回线性，以便旧数据保持不变
			// Note this happens before serialization
   // 请注意，这发生在序列化之前
			BlendIn.SetBlendOption(EAlphaBlendOption::Linear);
			BlendOut.SetBlendOption(EAlphaBlendOption::Linear);
		}
	}
	
	Super::Serialize(Ar);
}
#endif // WITH_EDITORONLY_DATA

const FAnimTrack* UAnimMontage::GetAnimationData(FName InSlotName) const
{
	for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
	{
		if ( SlotAnimTracks[I].SlotName == InSlotName )
		{
			// if data is there, return true. Otherwise, it doesn't matter
   // 如果有数据，则返回 true。否则，没关系
			return &( SlotAnimTracks[I].AnimTrack );
		}
	}

	return nullptr;
}

bool UAnimMontage::IsWithinPos(int32 FirstIndex, int32 SecondIndex, float CurrentTime) const
{
	float StartTime;
	float EndTime;
	if ( CompositeSections.IsValidIndex(FirstIndex) )
	{
		StartTime = CompositeSections[FirstIndex].GetTime();
	}
	else // if first index isn't valid, set to be 0.f, so it starts from reset
	{
		StartTime = 0.f;
	}

	if ( CompositeSections.IsValidIndex(SecondIndex) )
	{
		EndTime = CompositeSections[SecondIndex].GetTime();
	}
	else // if end index isn't valid, set to be BIG_NUMBER
	{
		// @todo anim, I don't know if using SequenceLength is better or BIG_NUMBER
  // @todo anim，我不知道使用 SequenceLength 更好还是 BIG_NUMBER
		// I don't think that'd matter. 
  // 我认为这并不重要。
		EndTime = GetPlayLength();
	}

	// since we do range of [StartTime, EndTime) (excluding EndTime) 
 // 因为我们的范围是 [StartTime, EndTime)（不包括 EndTime）
	// there is blindspot of when CurrentTime becomes >= SequenceLength
 // 当 CurrentTime 变为 >= SequenceLength 时存在盲点
	// include that frame if CurrentTime gets there. 
 // 如果 CurrentTime 到达那里，则包括该帧。
	// Otherwise, we continue to use [StartTime, EndTime)
 // 否则，我们继续使用[StartTime, EndTime)
	if (CurrentTime >= GetPlayLength())
	{
		return (StartTime <= CurrentTime && EndTime >= CurrentTime);
	}

	return (StartTime <= CurrentTime && EndTime > CurrentTime);
}

float UAnimMontage::CalculatePos(FCompositeSection &Section, float PosWithinCompositeSection) const
{
	float Offset = Section.GetTime();
	Offset += PosWithinCompositeSection;
	// @todo anim
 // @todo动画
	return Offset;
}

int32 UAnimMontage::GetSectionIndexFromPosition(float Position) const
{
	for (int32 I=0; I<CompositeSections.Num(); ++I)
	{
		// if within
  // 如果在
		if( IsWithinPos(I, I+1, Position) )
		{
			return I;
		}
	}

	return INDEX_NONE;
}

int32 UAnimMontage::GetAnimCompositeSectionIndexFromPos(float CurrentTime, float& PosWithinCompositeSection) const
{
	PosWithinCompositeSection = 0.f;

	for (int32 I=0; I<CompositeSections.Num(); ++I)
	{
		// if within
  // 如果在
		if (IsWithinPos(I, I+1, CurrentTime))
		{
			PosWithinCompositeSection = CurrentTime - CompositeSections[I].GetTime();
			return I;
		}
	}

	return INDEX_NONE;
}

float UAnimMontage::GetSectionTimeLeftFromPos(float Position)
{
	const int32 SectionID = GetSectionIndexFromPosition(Position);
	if( SectionID != INDEX_NONE )
	{
		if( IsValidSectionIndex(SectionID+1) )
		{
			return (GetAnimCompositeSection(SectionID+1).GetTime() - Position);
		}
		else
		{
			return (GetPlayLength() - Position);
		}
	}

	return -1.f;
}

const FCompositeSection& UAnimMontage::GetAnimCompositeSection(int32 SectionIndex) const
{
	check ( CompositeSections.IsValidIndex(SectionIndex) );
	return CompositeSections[SectionIndex];
}

FCompositeSection& UAnimMontage::GetAnimCompositeSection(int32 SectionIndex)
{
	check ( CompositeSections.IsValidIndex(SectionIndex) );
	return CompositeSections[SectionIndex];
}

int32 UAnimMontage::GetSectionIndex(FName InSectionName) const
{
	// I can have operator== to check SectionName, but then I have to construct
 // 我可以使用运算符==来检查SectionName，但随后我必须构造
	// empty FCompositeSection all the time whenever I search :(
 // 每当我搜索时，FCompositeSection 总是为空:(
	for (int32 I=0; I<CompositeSections.Num(); ++I)
	{
		if ( CompositeSections[I].SectionName == InSectionName ) 
		{
			return I;
		}
	}

	return INDEX_NONE;
}

FName UAnimMontage::GetSectionName(int32 SectionIndex) const
{
	if ( CompositeSections.IsValidIndex(SectionIndex) )
	{
		return CompositeSections[SectionIndex].SectionName;
	}

	return NAME_None;
}

bool UAnimMontage::IsValidSectionName(FName InSectionName) const
{
	return GetSectionIndex(InSectionName) != INDEX_NONE;
}

bool UAnimMontage::IsValidSectionIndex(int32 SectionIndex) const
{
	return (CompositeSections.IsValidIndex(SectionIndex));
}

void UAnimMontage::GetSectionStartAndEndTime(int32 SectionIndex, float& OutStartTime, float& OutEndTime) const
{
	OutStartTime = 0.f;
	OutEndTime = GetPlayLength();	
	if ( IsValidSectionIndex(SectionIndex) )
	{
		OutStartTime = GetAnimCompositeSection(SectionIndex).GetTime();		
	}

	if ( IsValidSectionIndex(SectionIndex + 1))
	{
		OutEndTime = GetAnimCompositeSection(SectionIndex + 1).GetTime();		
	}
}

float UAnimMontage::GetSectionLength(int32 SectionIndex) const
{
	float StartTime = 0.f;
	float EndTime = GetPlayLength();
	if ( IsValidSectionIndex(SectionIndex) )
	{
		StartTime = GetAnimCompositeSection(SectionIndex).GetTime();		
	}

	if ( IsValidSectionIndex(SectionIndex + 1))
	{
		EndTime = GetAnimCompositeSection(SectionIndex + 1).GetTime();		
	}

	return EndTime - StartTime;
}

#if WITH_EDITOR
int32 UAnimMontage::AddAnimCompositeSection(FName InSectionName, float StartTime)
{
	FCompositeSection NewSection;

	// make sure same name doesn't exists
 // 确保同名不存在
	if ( InSectionName != NAME_None )
	{
		NewSection.SectionName = InSectionName;
	}
	else
	{
		// just give default name
  // 只需给出默认名称
		NewSection.SectionName = FName(*FString::Printf(TEXT("Section%d"), CompositeSections.Num()+1));
	}

	// we already have that name
 // 我们已经有了这个名字
	if ( GetSectionIndex(InSectionName)!=INDEX_NONE )
	{
		UE_LOG(LogAnimMontage, Warning, TEXT("AnimCompositeSection : %s(%s) already exists. Choose different name."), 
			*NewSection.SectionName.ToString(), *InSectionName.ToString());
		return INDEX_NONE;
	}

	NewSection.Link(this, StartTime);

	// we'd like to sort them in the order of time
 // 我们想按时间顺序对它们进行排序
	int32 NewSectionIndex = CompositeSections.Add(NewSection);

	// when first added, just make sure to link previous one to add me as next if previous one doesn't have any link
 // 第一次添加时，请确保链接前一个，以便将我添加为下一个（如果前一个没有任何链接）
	// it's confusing first time when you add this data
 // 第一次添加这些数据时会感到困惑
	int32 PrevSectionIndex = NewSectionIndex-1;
	if ( CompositeSections.IsValidIndex(PrevSectionIndex) )
	{
		if (CompositeSections[PrevSectionIndex].NextSectionName == NAME_None)
		{
			CompositeSections[PrevSectionIndex].NextSectionName = InSectionName;
		}
	}

	return NewSectionIndex;
}

bool UAnimMontage::DeleteAnimCompositeSection(int32 SectionIndex)
{
	if ( CompositeSections.IsValidIndex(SectionIndex) )
	{
		CompositeSections.RemoveAt(SectionIndex);
		return true;
	}

	return false;
}
void UAnimMontage::SortAnimCompositeSectionByPos()
{
	// sort them in the order of time
 // 按时间顺序对它们进行排序
	struct FCompareFCompositeSection
	{
		FORCEINLINE bool operator()( const FCompositeSection &A, const FCompositeSection &B ) const
		{
			return A.GetTime() < B.GetTime();
		}
	};
	CompositeSections.Sort( FCompareFCompositeSection() );
}

void UAnimMontage::RegisterOnMontageChanged(const FOnMontageChanged& Delegate)
{
	OnMontageChanged.Add(Delegate);
}
void UAnimMontage::UnregisterOnMontageChanged(FDelegateUserObject Unregister)
{
	OnMontageChanged.RemoveAll(Unregister);
}
#endif	//WITH_EDITOR

void UAnimMontage::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
#if WITH_EDITOR
	BakeTimeStretchCurve();
#endif // WITH_EDITOR
	Super::PreSave(ObjectSaveContext);
}

FFrameRate UAnimMontage::GetSamplingFrameRate() const
{	
	if (CommonTargetFrameRate.IsValid())
	{
		return CommonTargetFrameRate;
	}

	return Super::GetSamplingFrameRate();
}

void UAnimMontage::PostLoad()
{
	// Link notifies before we call the Super::PostLoad (and eventually RefreshCacheData). This is to ensure that branching points get correctly
 // Link 在我们调用 Super::PostLoad（以及最终的 RefreshCacheData）之前发出通知。这是为了确保分支点正确
	// picked up as FAnimNotifyEvent::IsBranchingPoint() relies on the LinkedMontage being valid, and due to an issue where (since deprecated)
 // 被拾取为 FAnimNotifyEvent::IsBranchingPoint() 依赖于 LinkedMontage 是否有效，并且由于存在以下问题（自已弃用）
	// LinkSequence() was called instead of LinkMontage() there exists content for which LinkedMontage is saved as a null reference.
 // 调用 LinkSequence() 而不是 LinkMontage() 时，存在将 LinkedMontage 保存为空引用的内容。
	for(FAnimNotifyEvent& Notify : Notifies)
	{
#if WITH_EDITORONLY_DATA
		if(Notify.DisplayTime_DEPRECATED != 0.0f)
		{
			Notify.Clear();
			Notify.Link(this, Notify.DisplayTime_DEPRECATED);
		}
		else
#endif
		{
			Notify.Link(this, Notify.GetTime());
		}

		if(Notify.Duration != 0.0f)
		{
			Notify.EndLink.Link(this, Notify.GetTime() + Notify.Duration);
		}
	}
	
	Super::PostLoad();

	// copy deprecated variable to new one, temporary code to keep data copied. Am deleting it right after this
 // 将已弃用的变量复制到新变量，这是用于保留数据复制的临时代码。我在此之后立即删除它
	for ( auto SlotIter = SlotAnimTracks.CreateIterator() ; SlotIter ; ++SlotIter)
	{
		FAnimTrack & Track = (*SlotIter).AnimTrack;
		Track.ValidateSegmentTimes();

		const float CurrentCalculatedLength = CalculateSequenceLength();
		if(!FMath::IsNearlyEqual(CurrentCalculatedLength, GetPlayLength(), UE_KINDA_SMALL_NUMBER))		
		{
			UE_LOGFMT_NSLOC(LogAnimMontage, Display, "AnimMontage", "PostLoad_SequenceLengthMismatch",
				"UAnimMontage::PostLoad: The actual sequence length for {Asset} does not match the length stored in the asset. Please resave the asset.",
				("Asset", UE::FAssetLog(this)));
			SetCompositeLength(CurrentCalculatedLength);
		}

#if WITH_EDITOR
		for (const FAnimSegment& AnimSegment : Track.AnimSegments)
		{
			if(AnimSegment.IsPlayLengthOutOfDate())
			{
				UE_LOGFMT_NSLOC(LogAnimation, Warning, "AnimMontage", "PostLoad_InvalidSectionLength",
					"AnimMontage ({MontageAsset}) contains a Segment for Slot ({SlotName}) for which the playable length {PlayableLength} "
					"is out-of-sync with the represented AnimationSequence its length {AnimSequenceLength} ({AnimSequenceAsset}). Please up-date the segment and resave.",
					("MontageAsset", UE::FAssetLog(this)),
					("SlotName", SlotIter->SlotName),
					("PlayableLength", AnimSegment.AnimEndTime - AnimSegment.AnimStartTime),
					("AnimSequenceLength", AnimSegment.GetAnimReference()->GetPlayLength()),
					("AnimSequenceAsset", UE::FAssetLog(AnimSegment.GetAnimReference())));
			}
		}
#endif
	}

	for(FCompositeSection& Composite : CompositeSections)
	{
#if WITH_EDITORONLY_DATA
		if(Composite.StartTime_DEPRECATED != 0.0f)
		{
			Composite.Clear();
			Composite.Link(this, Composite.StartTime_DEPRECATED);
		}
		else
#endif
		{
			Composite.RefreshSegmentOnLoad();
			Composite.Link(this, Composite.GetTime());
		}
	}

	bool bRootMotionEnabled = bEnableRootMotionTranslation || bEnableRootMotionRotation;

	if (bRootMotionEnabled)
	{
		for (FSlotAnimationTrack& Slot : SlotAnimTracks)
		{
			for (FAnimSegment& Segment : Slot.AnimTrack.AnimSegments)
			{
				if (UAnimSequenceBase* AnimReference = Segment.GetAnimReference())
				{
#if WITH_EDITOR
					if (!AnimReference->GetEnableRootMotionSettingFromMontage())
					{
						UE_LOG(LogAnimation, Warning, TEXT("[Montage %s] has RootMotionEnabled, but [AnimationSequence %s] has not been saved after setting the flag. Please open the Montage and the AnimationSequence and save the AnimationSequence as this will generate non determistic cooks."), *GetFullName(), *AnimReference->GetFullName());
					}
#endif // WITH_EDITOR
					AnimReference->EnableRootMotionSettingFromMontage(true, RootMotionRootLock);
				}
			}
		}
	}
	// find preview base pose if it can
 // 如果可以的话找到预览基本姿势
#if WITH_EDITORONLY_DATA
	if ( IsValidAdditive() && PreviewBasePose == nullptr )
	{
		for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
		{
			if ( SlotAnimTracks[I].AnimTrack.AnimSegments.Num() > 0 )
			{
				UAnimSequenceBase* SequenceBase = SlotAnimTracks[I].AnimTrack.AnimSegments[0].GetAnimReference();
				UAnimSequence* BaseAdditivePose = (SequenceBase) ? SequenceBase->GetAdditiveBasePose() : nullptr;
				if (BaseAdditivePose)
				{
					PreviewBasePose = BaseAdditivePose;
					MarkPackageDirty();
					break;
				}
			}
		}
	}

	// verify if skeleton is valid, otherwise clear it, this can happen if anim sequence has been modified when this hasn't been loaded. 
 // 验证骨架是否有效，否则清除它，如果在未加载动画序列时修改了动画序列，则可能会发生这种情况。
	for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
	{
		if ( SlotAnimTracks[I].AnimTrack.AnimSegments.Num() > 0 )
		{
			UAnimSequenceBase* SequenceBase = SlotAnimTracks[I].AnimTrack.AnimSegments[0].GetAnimReference();
			if (SequenceBase && SequenceBase->GetSkeleton() == nullptr)
			{
				SlotAnimTracks[I].AnimTrack.AnimSegments[0].SetAnimReference(nullptr);
				MarkPackageDirty();
				break;
			}
		}
	}
#endif // WITH_EDITORONLY_DATA

	// Register Slots w/ Skeleton - to aid deterministic cooking do not do this during cook! 
 // 注册带有骨架的槽位 - 为了帮助确定性烹饪，请勿在烹饪过程中执行此操作！
	if(!GIsCookerLoadingPackage)
	{
		USkeleton* MySkeleton = GetSkeleton();
		if (MySkeleton)
		{
			for (int32 SlotIndex = 0; SlotIndex < SlotAnimTracks.Num(); SlotIndex++)
			{
				FName SlotName = SlotAnimTracks[SlotIndex].SlotName;
				MySkeleton->RegisterSlotNode(SlotName);
			}
		}
	}

	// Convert BranchingPoints to AnimNotifies.
 // 将 BranchingPoints 转换为 AnimNotify。
	if (GetLinker() && (GetLinker()->UEVer() < VER_UE4_MONTAGE_BRANCHING_POINT_REMOVAL) )
	{
		ConvertBranchingPointsToAnimNotifies();
	}

#if WITH_EDITORONLY_DATA
	// fix up blending time deprecated variable
 // 修复混合时间已弃用的变量
	if (BlendInTime_DEPRECATED != -1.f)
	{
		BlendIn.SetBlendTime(BlendInTime_DEPRECATED);
		BlendInTime_DEPRECATED = -1.f;
	}

	if(BlendOutTime_DEPRECATED != -1.f)
	{
		BlendOut.SetBlendTime(BlendOutTime_DEPRECATED);
		BlendOutTime_DEPRECATED = -1.f;
	}
#endif

	// collect markers if it's valid
 // 收集标记（如果有效）
	CollectMarkers();
}

void UAnimMontage::ConvertBranchingPointsToAnimNotifies()
{
#if WITH_EDITORONLY_DATA
	if (BranchingPoints_DEPRECATED.Num() > 0)
	{
		// Handle deprecated DisplayTime first
  // 首先处理已弃用的 DisplayTime
		for (auto& BranchingPoint : BranchingPoints_DEPRECATED)
		{
			if (BranchingPoint.DisplayTime_DEPRECATED != 0.0f)
			{
				BranchingPoint.Clear();
				BranchingPoint.Link(this, BranchingPoint.DisplayTime_DEPRECATED);
			}
			else
			{
				BranchingPoint.Link(this, BranchingPoint.GetTime());
			}
		}

		// Then convert to AnimNotifies
  // 然后转换为AnimNotify
		USkeleton * MySkeleton = GetSkeleton();

		// Add a new AnimNotifyTrack, and place all branching points in there.
  // 添加一个新的 AnimNotifyTrack，并将所有分支点放置在其中。
		int32 TrackIndex = AnimNotifyTracks.Num();

		FAnimNotifyTrack NewItem;
		NewItem.TrackName = *FString::FromInt(TrackIndex + 1);
		NewItem.TrackColor = FLinearColor::White;
		AnimNotifyTracks.Add(NewItem);

		for (auto BranchingPoint : BranchingPoints_DEPRECATED)
		{
			int32 NewNotifyIndex = Notifies.Add(FAnimNotifyEvent());
			FAnimNotifyEvent& NewEvent = Notifies[NewNotifyIndex];
			NewEvent.NotifyName = BranchingPoint.EventName;

			float TriggerTime = BranchingPoint.GetTriggerTime();
			NewEvent.Link(this, TriggerTime);
#if WITH_EDITOR
			NewEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(CalculateOffsetForNotify(TriggerTime));
#endif
			NewEvent.TrackIndex = TrackIndex;
			NewEvent.Notify = nullptr;
			NewEvent.NotifyStateClass = nullptr;
			NewEvent.bConvertedFromBranchingPoint = true;
			NewEvent.MontageTickType = EMontageNotifyTickType::BranchingPoint;

			// Add as a custom AnimNotify event to Skeleton.
   // 作为自定义 AnimNotify 事件添加到 Skeleton。
			if (MySkeleton)
			{
				MySkeleton->AnimationNotifies.AddUnique(NewEvent.NotifyName);
			}
		}

		BranchingPoints_DEPRECATED.Empty();
		RefreshBranchingPointMarkers();
	}
#endif
}

void UAnimMontage::RefreshBranchingPointMarkers()
{
	BranchingPointMarkers.Empty();
	BranchingPointStateNotifyIndices.Empty();

	// Verify that we have no overlapping trigger times, this is not supported, and markers would not be triggered then.
 // 验证我们没有重叠的触发时间，这是不支持的，并且标记不会被触发。
	TMap<float, FAnimNotifyEvent*> TriggerTimes;

	int32 NumNotifies = Notifies.Num();
	for (int32 NotifyIndex = 0; NotifyIndex < NumNotifies; NotifyIndex++)
	{
		FAnimNotifyEvent& NotifyEvent = Notifies[NotifyIndex];

		if (NotifyEvent.IsBranchingPoint())
		{
			AddBranchingPointMarker(FBranchingPointMarker(NotifyIndex, NotifyEvent.GetTriggerTime(), EAnimNotifyEventType::Begin), TriggerTimes);

			if (NotifyEvent.NotifyStateClass)
			{
				// Track end point of AnimNotifyStates.
    // 跟踪 AnimNotifyStates 的终点。
				AddBranchingPointMarker(FBranchingPointMarker(NotifyIndex, NotifyEvent.GetEndTriggerTime(), EAnimNotifyEventType::End), TriggerTimes);

				// Also track AnimNotifyStates separately, so we can tick them between their Begin and End points.
    // 还要单独跟踪 AnimNotifyStates，这样我们就可以在它们的开始点和结束点之间勾选它们。
				BranchingPointStateNotifyIndices.Add(NotifyIndex);
			}
		}
	}
	
	if (BranchingPointMarkers.Num() > 0)
	{
		// Sort markers
  // 对标记进行排序
		struct FCompareNotifyTickMarkersTime
		{
			FORCEINLINE bool operator()(const FBranchingPointMarker &A, const FBranchingPointMarker &B) const
			{
				return A.TriggerTime < B.TriggerTime;
			}
		};

		BranchingPointMarkers.Sort(FCompareNotifyTickMarkersTime());
	}
}

void UAnimMontage::RefreshCacheData()
{
	Super::RefreshCacheData();

	// This gets called whenever notifies are modified in the editor, so refresh our branch list
 // 每当在编辑器中修改通知时都会调用此函数，因此请刷新我们的分支列表
	RefreshBranchingPointMarkers();
#if WITH_EDITOR
	if (!FUObjectThreadContext::Get().IsRoutingPostLoad)
	{
		// This is not needed during post load (as the child montages themselves will handle
  // 在后加载期间不需要这样做（因为子蒙太奇本身将处理
		// updating and calling it can cause deterministic cooking issues depending on load order
  // 更新和调用它可能会导致确定性烹饪问题，具体取决于加载顺序
		PropagateChanges();
	}
#endif // WITH_EDITOR
}

void UAnimMontage::AddBranchingPointMarker(FBranchingPointMarker TickMarker, TMap<float, FAnimNotifyEvent*>& TriggerTimes)
{
	// Add Marker
 // 添加标记
	BranchingPointMarkers.Add(TickMarker);

	// Check that there is no overlapping marker, as we don't support this.
 // 检查是否有重叠标记，因为我们不支持这一点。
	// This would mean one of them is not getting triggered!
 // 这意味着其中之一没有被触发！
	FAnimNotifyEvent** FoundNotifyEventPtr = TriggerTimes.Find(TickMarker.TriggerTime);
	if (FoundNotifyEventPtr)
	{
		UE_ASSET_LOG(LogAnimMontage, Warning, this, TEXT("Branching Point '%s' overlaps with '%s' at time: %f. One of them will not get triggered!"),
			*Notifies[TickMarker.NotifyIndex].NotifyName.ToString(), *(*FoundNotifyEventPtr)->NotifyName.ToString(), TickMarker.TriggerTime);
	}
	else
	{
		TriggerTimes.Add(TickMarker.TriggerTime, &Notifies[TickMarker.NotifyIndex]);
	}
}

const FBranchingPointMarker* UAnimMontage::FindFirstBranchingPointMarker(float StartTrackPos, float EndTrackPos) const
{
	if (BranchingPointMarkers.Num() > 0)
	{
		const bool bSearchBackwards = (EndTrackPos < StartTrackPos);
		if (!bSearchBackwards)
		{
			for (int32 Index = 0; Index < BranchingPointMarkers.Num(); Index++)
			{
				const FBranchingPointMarker& Marker = BranchingPointMarkers[Index];
				if (Marker.TriggerTime <= StartTrackPos)
				{
					continue;
				}
				if (Marker.TriggerTime > EndTrackPos)
				{
					break;
				}
				return &Marker;
			}
		}
		else
		{
			for (int32 Index = BranchingPointMarkers.Num() - 1; Index >= 0; Index--)
			{
				const FBranchingPointMarker& Marker = BranchingPointMarkers[Index];
				if (Marker.TriggerTime >= StartTrackPos)
				{
					continue;
				}
				if (Marker.TriggerTime < EndTrackPos)
				{
					break;
				}
				return &Marker;
			}
		}
	}
	return nullptr;
}

void UAnimMontage::FilterOutNotifyBranchingPoints(TArray<const FAnimNotifyEvent*>& InAnimNotifies)
{
	for (int32 Index = InAnimNotifies.Num()-1; Index >= 0; Index--)
	{
		if (InAnimNotifies[Index]->IsBranchingPoint())
		{
			InAnimNotifies.RemoveAt(Index, 1);
		}
	}
}

void UAnimMontage::FilterOutNotifyBranchingPoints(TArray<FAnimNotifyEventReference>& InAnimNotifies)
{
	for (int32 Index = InAnimNotifies.Num() - 1; Index >= 0; Index--)
	{
		if(const FAnimNotifyEvent* Notify = InAnimNotifies[Index].GetNotify())
		if (!Notify || Notify->IsBranchingPoint())
		{
			InAnimNotifies.RemoveAt(Index, 1);
		}
	}
}

#if WITH_EDITOR
void UAnimMontage::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// It is unclear if CollectMarkers should be here or in RefreshCacheData
 // 目前尚不清楚 CollectMarkers 应该位于此处还是位于 RefreshCacheData 中
	if (SyncGroup != NAME_None)
	{
		CollectMarkers();
	}

	UpdateCommonTargetFrameRate();

	PropagateChanges();
}

void UAnimMontage::PropagateChanges()
{
	// @note propagate to children
 // @note传播给孩子们
	// this isn't that slow yet, but if this gets slow, we'll have to do guid method
 // 这还没有那么慢，但是如果这变得很慢，我们就必须使用 guid 方法
	if (ChildrenAssets.Num() > 0)
	{
		for (UAnimationAsset* Child : ChildrenAssets)
		{
			if (Child)
			{
				Child->UpdateParentAsset();
			}
		}
	}
}
#endif // WITH_EDITOR

void UAnimMontage::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);

	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(CompositeSections.GetAllocatedSize());
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(SlotAnimTracks.GetAllocatedSize());
	for (FSlotAnimationTrack& Slot : SlotAnimTracks)
	{
		CumulativeResourceSize.AddDedicatedSystemMemoryBytes(Slot.AnimTrack.GetTotalBytesUsed());
	}
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(BranchingPointMarkers.GetAllocatedSize());
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(BranchingPointStateNotifyIndices.GetAllocatedSize());
}

bool UAnimMontage::IsValidAdditive() const
{
	// if first one is additive, this is additive
 // 如果第一个是可加的，则这是可加的
	if ( SlotAnimTracks.Num() > 0 )
	{
		for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
		{
			if (!SlotAnimTracks[I].AnimTrack.IsAdditive())
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

bool UAnimMontage::IsValidAdditiveSlot(const FName& SlotNodeName) const
{
	// if first one is additive, this is additive
 // 如果第一个是可加的，则这是可加的
	if ( SlotAnimTracks.Num() > 0 )
	{
		for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
		{
			if (SlotAnimTracks[I].SlotName == SlotNodeName)
			{
				return SlotAnimTracks[I].AnimTrack.IsAdditive();
			}
		}
	}

	return false;
}

EAnimEventTriggerOffsets::Type UAnimMontage::CalculateOffsetFromSections(float Time) const
{
	for(auto Iter = CompositeSections.CreateConstIterator(); Iter; ++Iter)
	{
		float SectionTime = Iter->GetTime();
		if(FMath::IsNearlyEqual(SectionTime,Time))
		{
			return EAnimEventTriggerOffsets::OffsetBefore;
		}
	}
	return EAnimEventTriggerOffsets::NoOffset;
}

bool FAnimMontageInstance::ValidateInstanceAfterNotifyState(const TWeakObjectPtr<UAnimInstance>& InAnimInstance, const UAnimNotifyState* InNotifyStateClass)
{
	// An owning instance should never be invalid after a notify call, since it's where the montage instance lives
 // 拥有实例在通知调用后永远不应该无效，因为它是蒙太奇实例所在的位置
	if (!InAnimInstance.IsValid())
	{
		ensureMsgf(false, TEXT("Invalid anim instance after triggering notify: %s"), *GetNameSafe(InNotifyStateClass));
		return false;
	}

	// Montage instances array should never be empty after a notify state
 // Montage 实例数组在通知状态后永远不应该为空
	if (InAnimInstance->MontageInstances.Num() == 0)
	{
		ensureMsgf(false, TEXT("Montage instances empty on AnimInstance(%s) after calling notify:  %s"), *GetNameSafe(InAnimInstance.Get()), *GetNameSafe(InNotifyStateClass));
		return false;
	}

	return true;
}

#if WITH_EDITOR
EAnimEventTriggerOffsets::Type UAnimMontage::CalculateOffsetForNotify(float NotifyDisplayTime) const
{
	EAnimEventTriggerOffsets::Type Offset = Super::CalculateOffsetForNotify(NotifyDisplayTime);
/** 获取 Montage 的组名 */
	if(Offset == EAnimEventTriggerOffsets::NoOffset)
	{
		Offset = CalculateOffsetFromSections(NotifyDisplayTime);
	}
	return Offset;
}
#endif

bool UAnimMontage::HasRootMotion() const
{
	for (const FSlotAnimationTrack& Track : SlotAnimTracks)
	{
		if (Track.AnimTrack.HasRootMotion())
		{
			return true;
		}
	}
	return false;
}

/** Extract RootMotion Transform from a contiguous Track position range.
 * *CONTIGUOUS* means that if playing forward StartTractPosition < EndTrackPosition.
 * No wrapping over if looping. No jumping across different sections.
 * So the AnimMontage has to break the update into contiguous pieces to handle those cases.
 *
 * This does handle Montage playing backwards (StartTrackPosition > EndTrackPosition).
 *
 * It will break down the range into steps if needed to handle looping animations, or different animations.
 * These steps will be processed sequentially, and output the RootMotion transform in component space.
 */
FTransform UAnimMontage::ExtractRootMotionFromTrackRange(float StartTrackPosition, float EndTrackPosition, const FAnimExtractContext& Context) const
{
	FRootMotionMovementParams RootMotion;

	// For now assume Root Motion only comes from first track.
 // 现在假设根运动仅来自第一条轨道。
	if( SlotAnimTracks.Num() > 0 )
	{
		const FAnimTrack& SlotAnimTrack = SlotAnimTracks[0].AnimTrack;

		// Get RootMotion pieces from this track.
  // 从此轨道获取 RootMotion 片段。
		// We can deal with looping animations, or multiple animations. So we break those up into sequential operations.
  // 我们可以处理循环动画或多个动画。因此，我们将它们分解为顺序操作。
		// (Animation, StartFrame, EndFrame) so we can then extract root motion sequentially.
  // （动画、StartFrame、EndFrame），这样我们就可以顺序提取根运动。
		ExtractRootMotionFromTrack(SlotAnimTrack, StartTrackPosition, EndTrackPosition, Context, RootMotion);

	}

	UE_LOG(LogRootMotion, Log,  TEXT("\tUAnimMontage::ExtractRootMotionForTrackRange RootMotionTransform: Translation: %s, Rotation: %s")
		, *RootMotion.GetRootMotionTransform().GetTranslation().ToCompactString()
		, *RootMotion.GetRootMotionTransform().GetRotation().Rotator().ToCompactString()
		);

	return RootMotion.GetRootMotionTransform();
/** 获取 Montage 的组名 */
}

/** Get Montage's Group Name */
/** 获取 Montage 的组名 */
FName UAnimMontage::GetGroupName() const
{
	USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton && (SlotAnimTracks.Num() > 0))
	{
		return MySkeleton->GetSlotGroupName(SlotAnimTracks[0].SlotName);
	}

	return FAnimSlotGroup::DefaultGroupName;
}

bool UAnimMontage::HasValidSlotSetup() const
{
	// We only need to worry about this if we have multiple tracks.
 // 如果我们有多个轨道，我们只需要担心这一点。
	// Montages with a single track will always have a valid slot setup.
 // 具有单个轨道的蒙太奇将始终具有有效的插槽设置。
	int32 NumAnimTracks = SlotAnimTracks.Num();
	if (NumAnimTracks > 1)
	{
		USkeleton* MySkeleton = GetSkeleton();
		if (MySkeleton)
		{
			FName MontageGroupName = GetGroupName();
			TArray<FName> UniqueSlotNameList;
			UniqueSlotNameList.Add(SlotAnimTracks[0].SlotName);

			for (int32 TrackIndex = 1; TrackIndex < NumAnimTracks; TrackIndex++)
			{
				// Verify that slot names are unique.
    // 验证插槽名称是否唯一。
				FName CurrentSlotName = SlotAnimTracks[TrackIndex].SlotName;
				bool bSlotNameAlreadyInUse = UniqueSlotNameList.Contains(CurrentSlotName);
				if (!bSlotNameAlreadyInUse)
				{
					UniqueSlotNameList.Add(CurrentSlotName);
				}
				else
				{
					UE_LOG(LogAnimMontage, Warning, TEXT("Montage '%s' not properly setup. Slot named '%s' is already used in this Montage. All slots must be unique"),
						*GetFullName(), *CurrentSlotName.ToString());
					return false;
				}

				// Verify that all slots belong to the same group.
    // 验证所有插槽是否属于同一组。
				FName CurrentSlotGroupName = MySkeleton->GetSlotGroupName(CurrentSlotName);
				bool bDifferentGroupName = (CurrentSlotGroupName != MontageGroupName);
				if (bDifferentGroupName)
				{
					UE_LOG(LogAnimMontage, Warning, TEXT("Montage '%s' not properly setup. Slot's group '%s' is different than the Montage's group '%s'. All slots must belong to the same group."),
						*GetFullName(), *CurrentSlotGroupName.ToString(), *MontageGroupName.ToString());
					return false;
				}
			}
		}
	}

	return true;
}

float UAnimMontage::CalculateSequenceLength()
{
	float CalculatedSequenceLength = 0.f;
	for (auto Iter = SlotAnimTracks.CreateIterator(); Iter; ++Iter)
	{
		FSlotAnimationTrack& SlotAnimTrack = (*Iter);
		if (SlotAnimTrack.AnimTrack.AnimSegments.Num() > 0)
		{
			CalculatedSequenceLength = FMath::Max(CalculatedSequenceLength, SlotAnimTrack.AnimTrack.GetLength());
		}
	}
	return CalculatedSequenceLength;
}

const TArray<class UAnimMetaData*> UAnimMontage::GetSectionMetaData(FName SectionName, bool bIncludeSequence/*=true*/, FName SlotName /*= NAME_None*/)
{
	TArray<class UAnimMetaData*> MetadataList;
	bool bShouldIIncludeSequence = bIncludeSequence;

	for (int32 SectionIndex = 0; SectionIndex < CompositeSections.Num(); ++SectionIndex)
	{
		const auto& CurSection = CompositeSections[SectionIndex];
		if (SectionName == NAME_None || CurSection.SectionName == SectionName)
		{
			// add to the list
   // 添加到列表
			MetadataList.Append(CurSection.GetMetaData());

			if (bShouldIIncludeSequence)
			{
				if (SectionName == NAME_None)
				{
					for (auto& SlotIter : SlotAnimTracks)
					{
						if (SlotName == NAME_None || SlotIter.SlotName == SlotName)
						{
							// now add the animations within this section
       // 现在在此部分中添加动画
							for (auto& SegmentIter : SlotIter.AnimTrack.AnimSegments)
							{
								if (UAnimSequenceBase* AnimReference = SegmentIter.GetAnimReference())
								{
									// only add unique here
         // 此处仅添加唯一的
									TArray<UAnimMetaData*> RefMetadata = AnimReference->GetMetaData();

									for (auto& RefData : RefMetadata)
									{
										MetadataList.AddUnique(RefData);
									}
								}
							}
						}
					}

					// if section name == None, we only grab slots once
     // 如果节名称 == None，我们只抢槽一次
					// otherwise, it will grab multiple times
     // 否则会多次抓取
					bShouldIIncludeSequence = false;
				}
				else
				{
					float SectionStartTime = 0.f, SectionEndTime = 0.f;
					GetSectionStartAndEndTime(SectionIndex, SectionStartTime, SectionEndTime);
					for (auto& SlotIter : SlotAnimTracks)
					{
						if (SlotName == NAME_None || SlotIter.SlotName == SlotName)
						{
							// now add the animations within this section
       // 现在在此部分中添加动画
							for (auto& SegmentIter : SlotIter.AnimTrack.AnimSegments)
							{
								if (SegmentIter.IsIncluded(SectionStartTime, SectionEndTime))
								{
									if (UAnimSequenceBase* AnimReference = SegmentIter.GetAnimReference())
									{
										// only add unique here
          // 此处仅添加唯一的
										TArray<UAnimMetaData*> RefMetadata = AnimReference->GetMetaData();

										for (auto& RefData : RefMetadata)
										{
											MetadataList.AddUnique(RefData);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return MetadataList;
}

#if WITH_EDITOR
bool UAnimMontage::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive /*= true*/)
{
	Super::GetAllAnimationSequencesReferred(AnimationAssets, bRecursive);

	for (auto Iter = SlotAnimTracks.CreateConstIterator(); Iter; ++Iter)
	{
		const FSlotAnimationTrack& Track = (*Iter);
		Track.AnimTrack.GetAllAnimationSequencesReferred(AnimationAssets, bRecursive);
	}

	if (PreviewBasePose)
	{
		PreviewBasePose->HandleAnimReferenceCollection(AnimationAssets, bRecursive);
	}

	return (AnimationAssets.Num() > 0);
}

void UAnimMontage::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap)
{
	Super::ReplaceReferredAnimations(ReplacementMap);

	for (auto Iter = SlotAnimTracks.CreateIterator(); Iter; ++Iter)
	{
		FSlotAnimationTrack& Track = (*Iter);
		Track.AnimTrack.ReplaceReferredAnimations(ReplacementMap);
	}

	if (PreviewBasePose)
	{
		UAnimSequence* const* ReplacementAsset = (UAnimSequence*const*)ReplacementMap.Find(PreviewBasePose);
		if (ReplacementAsset)
		{
			PreviewBasePose = *ReplacementAsset;
			PreviewBasePose->ReplaceReferredAnimations(ReplacementMap);
		}
	}
}

void UAnimMontage::UpdateLinkableElements()
{
	// Update all linkable elements
 // 更新所有可链接元素
	for (FCompositeSection& Section : CompositeSections)
	{
		Section.Update();
	}

	for (FAnimNotifyEvent& Notify : Notifies)
	{
		Notify.Update();
		Notify.RefreshTriggerOffset(CalculateOffsetForNotify(Notify.GetTime()));

		Notify.EndLink.Update();
		Notify.RefreshEndTriggerOffset(CalculateOffsetForNotify(Notify.EndLink.GetTime()));
	}
}

void UAnimMontage::UpdateLinkableElements(int32 SlotIdx, int32 SegmentIdx)
{
	for (FCompositeSection& Section : CompositeSections)
	{
		if (Section.GetSlotIndex() == SlotIdx && Section.GetSegmentIndex() == SegmentIdx)
		{
			// Update the link
   // 更新链接
			Section.Update();
		}
	}

	for (FAnimNotifyEvent& Notify : Notifies)
	{
		if (Notify.GetSlotIndex() == SlotIdx && Notify.GetSegmentIndex() == SegmentIdx)
		{
			Notify.Update();
			Notify.RefreshTriggerOffset(CalculateOffsetForNotify(Notify.GetTime()));
		}

		if (Notify.EndLink.GetSlotIndex() == SlotIdx && Notify.EndLink.GetSegmentIndex() == SegmentIdx)
		{
			Notify.EndLink.Update();
			Notify.RefreshEndTriggerOffset(CalculateOffsetForNotify(Notify.EndLink.GetTime()));
		}
	}
}

void UAnimMontage::RefreshParentAssetData()
{
	Super::RefreshParentAssetData();

	UAnimMontage* ParentMontage = CastChecked<UAnimMontage>(ParentAsset);

	BlendIn = ParentMontage->BlendIn;
	BlendOut = ParentMontage->BlendOut;
	BlendOutTriggerTime = ParentMontage->BlendOutTriggerTime;
	SyncGroup = ParentMontage->SyncGroup;
	SyncSlotIndex = ParentMontage->SyncSlotIndex;

	MarkerData = ParentMontage->MarkerData;
	CompositeSections = ParentMontage->CompositeSections;
	SlotAnimTracks = ParentMontage->SlotAnimTracks;

	PreviewBasePose = ParentMontage->PreviewBasePose;
	BranchingPointMarkers = ParentMontage->BranchingPointMarkers;
	BranchingPointStateNotifyIndices = ParentMontage->BranchingPointStateNotifyIndices;

	for (int32 SlotIdx = 0; SlotIdx < SlotAnimTracks.Num(); ++SlotIdx)
	{
		FSlotAnimationTrack& SlotTrack = SlotAnimTracks[SlotIdx];
		
		for (int32 SegmentIdx = 0; SegmentIdx < SlotTrack.AnimTrack.AnimSegments.Num(); ++SegmentIdx)
		{
			FAnimSegment& Segment = SlotTrack.AnimTrack.AnimSegments[SegmentIdx];
			FAnimSegment& ParentSegment = ParentMontage->SlotAnimTracks[SlotIdx].AnimTrack.AnimSegments[SegmentIdx];
			UAnimSequenceBase* SourceReference = Segment.GetAnimReference();
			UAnimSequenceBase* TargetReference = Cast<UAnimSequenceBase>(AssetMappingTable->GetMappedAsset(SourceReference));
			Segment.SetAnimReference(TargetReference);

			float LengthChange = FMath::IsNearlyZero(SourceReference->GetPlayLength()) ? 0.f : TargetReference->GetPlayLength() / SourceReference->GetPlayLength();
			float RateChange = FMath::IsNearlyZero(SourceReference->RateScale) ? 0.f : FMath::Abs(TargetReference->RateScale / SourceReference->RateScale);
			float TotalRateChange = FMath::IsNearlyZero(RateChange)? 0.f : (LengthChange / RateChange);
			Segment.AnimPlayRate *= TotalRateChange;
			Segment.AnimStartTime *= LengthChange;
			Segment.AnimEndTime *= LengthChange;
		}
	}

	UpdateCommonTargetFrameRate();

	OnMontageChanged.Broadcast();
}

#endif

FString MakePositionMessage(const FMarkerSyncAnimPosition& Position)
{
	return FString::Printf(TEXT("Names(PrevName: %s | NextName: %s) PosBetweenMarkers: %.2f"), *Position.PreviousMarkerName.ToString(), *Position.NextMarkerName.ToString(), Position.PositionBetweenMarkers);
}

void UAnimMontage::TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const
{
	bool bRecordNeedsResetting = true;

	// nothing has to happen here
 // 这里不必发生任何事情
	// we just have to make sure we set Context data correct
 // 我们只需要确保我们设置的上下文数据正确
	//if (ensure (Context.IsLeader()))
 // 如果（确保（Context.IsLeader()））
	if (Context.IsLeader())
	{
		check(Instance.DeltaTimeRecord);
		const float CurrentTime = Instance.Montage.CurrentPosition;
		const float PreviousTime = Instance.DeltaTimeRecord->GetPrevious();
		const float MoveDelta = Instance.DeltaTimeRecord->Delta;

		// Update context's data for followers to use.
  // 更新上下文的数据以供关注者使用。
		Context.SetLeaderDelta(MoveDelta);
		Context.SetPreviousAnimationPositionRatio(PreviousTime / GetPlayLength());

		if (MoveDelta != 0.f)
		{
			if (Instance.bCanUseMarkerSync && Instance.MarkerTickRecord && Context.CanUseMarkerPosition())
			{
				FMarkerTickRecord* MarkerTickRecord = Instance.MarkerTickRecord;
				FMarkerTickContext& MarkerTickContext = Context.MarkerTickContext;
				const bool bIsMarkerTickRecordValid = MarkerTickRecord->IsValid(Instance.bLooping);
				
				// Store the sync anim position BEFORE the asset has being ticked.
    // 在勾选资源之前存储同步动画位置。
				if (bIsMarkerTickRecordValid)
				{
					MarkerTickContext.SetMarkerSyncStartPosition(GetMarkerSyncPositionFromMarkerIndicies(MarkerTickRecord->PreviousMarker.MarkerIndex, MarkerTickRecord->NextMarker.MarkerIndex, PreviousTime, nullptr));
				}
				else
				{
					// only thing is that passed markers won't work in this frame. To do that, I have to figure out how it jumped from where to where, 
     // 唯一的问题是传递的标记在此框架中不起作用。为此，我必须弄清楚它是如何从哪里跳到哪里的，
					FMarkerPair PreviousMarker;
					FMarkerPair NextMarker;
					GetMarkerIndicesForTime(PreviousTime, false, MarkerTickContext.GetValidMarkerNames(), PreviousMarker, NextMarker);
					MarkerTickContext.SetMarkerSyncStartPosition(GetMarkerSyncPositionFromMarkerIndicies(PreviousMarker.MarkerIndex, NextMarker.MarkerIndex, PreviousTime, nullptr));
				}

				// Advance as leader.
    // 作为领导者前进。
				// @todo this won't work well once we start jumping
    // @todo 一旦我们开始跳跃，这将无法正常工作
				// only thing is that passed markers won't work in this frame. To do that, I have to figure out how it jumped from where to where, 
    // 唯一的问题是传递的标记在此框架中不起作用。为此，我必须弄清楚它是如何从哪里跳到哪里的，
				GetMarkerIndicesForTime(CurrentTime, false, MarkerTickContext.GetValidMarkerNames(), MarkerTickRecord->PreviousMarker, MarkerTickRecord->NextMarker);
				bRecordNeedsResetting = false; // we have updated it now, no need to reset.

				// Store the sync anim position AFTER the asset has being ticked.
    // 勾选资产后存储同步动画位置。
				MarkerTickContext.SetMarkerSyncEndPosition(GetMarkerSyncPositionFromMarkerIndicies(MarkerTickRecord->PreviousMarker.MarkerIndex, MarkerTickRecord->NextMarker.MarkerIndex, CurrentTime, nullptr));

				MarkerTickContext.MarkersPassedThisTick = *Instance.Montage.MarkersPassedThisTick;

#if DO_CHECK
				// The marker tick record gets invalidated when the montage position is set externally and due to this change we cannot assume
    // 当外部设置蒙太奇位置时，标记刻度记录将失效，并且由于此更改，我们无法假设
				// its sync positions will be the same as the previous tick. 
    // 其同步位置将与前一个刻度相同。
				if (MarkerTickContext.MarkersPassedThisTick.Num() == 0 && bIsMarkerTickRecordValid)
				{
					const FMarkerSyncAnimPosition& StartPosition = MarkerTickContext.GetMarkerSyncStartPosition();
					const FMarkerSyncAnimPosition& EndPosition = MarkerTickContext.GetMarkerSyncEndPosition();
					checkf(StartPosition.NextMarkerName == EndPosition.NextMarkerName, TEXT("StartPosition %s\nEndPosition %s\nPrevTime to CurrentTimeAsset: %.3f - %.3f Delta: %.3f\nAsset = %s"), *MakePositionMessage(StartPosition), *MakePositionMessage(EndPosition), PreviousTime, CurrentTime, MoveDelta, *Instance.SourceAsset->GetFullName());
					checkf(StartPosition.PreviousMarkerName == EndPosition.PreviousMarkerName, TEXT("StartPosition %s\nEndPosition %s\nPrevTime - CurrentTimeAsset: %.3f - %.3f Delta: %.3f\nAsset = %s"), *MakePositionMessage(StartPosition), *MakePositionMessage(EndPosition), PreviousTime, CurrentTime, MoveDelta, *Instance.SourceAsset->GetFullName());
				}
#endif

				UE_LOG(LogAnimMarkerSync, Log, TEXT("Montage Leading SyncGroup: %s(%s) Start [%s], End [%s]"), *GetNameSafe(this), *SyncGroup.ToString(), *MarkerTickContext.GetMarkerSyncStartPosition().ToString(), *MarkerTickContext.GetMarkerSyncEndPosition().ToString());
			}
		}
		
		// Update context's position for followers to use.
  // 更新上下文的位置以供关注者使用。
		Context.SetAnimationPositionRatio(CurrentTime / GetPlayLength());
	}

	// Reset record if needed.
 // 如果需要，重置记录。
	if (bRecordNeedsResetting && Instance.MarkerTickRecord)
	{
		Instance.MarkerTickRecord->Reset();
	}
}

void UAnimMontage::CollectMarkers()
{
	MarkerData.AuthoredSyncMarkers.Reset();

	// We want to make sure anim reference actually contains markers
 // 我们要确保动画参考实际上包含标记
	if (SyncGroup != NAME_None)
	{
		if (SlotAnimTracks.IsValidIndex(SyncSlotIndex))
		{
			const FAnimTrack& AnimTrack = SlotAnimTracks[SyncSlotIndex].AnimTrack;
			for (const auto& Seg : AnimTrack.AnimSegments)
			{
				const UAnimSequence* Sequence = Cast<UAnimSequence>(Seg.GetAnimReference());
				if (Sequence && Sequence->AuthoredSyncMarkers.Num() > 0)
				{
					// @todo this won't work well if you have starttime < end time and it does have negative playrate
     // @todo 如果你的开始时间<结束时间并且它确实具有负播放率，那么这将无法正常工作
					for (const auto& Marker : Sequence->AuthoredSyncMarkers)
					{
						if (Marker.Time >= Seg.AnimStartTime && Marker.Time <= Seg.AnimEndTime)
						{
							const float TotalSegmentLength = (Seg.AnimEndTime - Seg.AnimStartTime)*Seg.AnimPlayRate;
							// i don't think we can do negative in this case
       // 我认为在这种情况下我们不能做消极的事情
							ensure(TotalSegmentLength >= 0.f);

							// now add to the list
       // 现在添加到列表中
							for (int32 LoopCount = 0; LoopCount < Seg.LoopingCount; ++LoopCount)
							{
								FAnimSyncMarker NewMarker;

								NewMarker.Time = Seg.StartPos + (Marker.Time - Seg.AnimStartTime)*Seg.AnimPlayRate + TotalSegmentLength*LoopCount;
								NewMarker.MarkerName = Marker.MarkerName;
								MarkerData.AuthoredSyncMarkers.Add(NewMarker);
							}
						}
					}
				}
			}

			MarkerData.CollectUniqueNames();
		}
		else
		{
			UE_LOG(LogAnimMontage, Warning, TEXT("Montage's sync slot track index is invalid. Make sure to use a valid slot track index, otherwise this Montage will use old sync markers or not use marker-based syncing at all."))
		}
	}
}

void UAnimMontage::GetMarkerIndicesForTime(float CurrentTime, bool bLooping, const TArray<FName>& ValidMarkerNames, FMarkerPair& OutPrevMarker, FMarkerPair& OutNextMarker) const
{
	MarkerData.GetMarkerIndicesForTime(CurrentTime, bLooping, ValidMarkerNames, OutPrevMarker, OutNextMarker, GetPlayLength());
}

FMarkerSyncAnimPosition UAnimMontage::GetMarkerSyncPositionFromMarkerIndicies(int32 PrevMarker, int32 NextMarker, float CurrentTime, const UMirrorDataTable* MirrorTable) const
{
	return MarkerData.GetMarkerSyncPositionFromMarkerIndicies(PrevMarker, NextMarker, CurrentTime, GetPlayLength(), MirrorTable);
}

void UAnimMontage::InvalidateRecursiveAsset()
{
	for (FSlotAnimationTrack& SlotTrack : SlotAnimTracks)
	{
		SlotTrack.AnimTrack.InvalidateRecursiveAsset(this);
	}
}

bool UAnimMontage::ContainRecursive(TArray<UAnimCompositeBase*>& CurrentAccumulatedList) 
{
	// am I included already?
 // 我已经包括在内了吗？
	if (CurrentAccumulatedList.Contains(this))
	{
		return true;
	}

	// otherwise, add myself to it
 // 否则，将我自己添加到其中
	CurrentAccumulatedList.Add(this);

	for (FSlotAnimationTrack& SlotTrack : SlotAnimTracks)
	{
		// otherwise send to animation track
  // 否则发送到动画轨道
		if (SlotTrack.AnimTrack.ContainRecursive(CurrentAccumulatedList))
		{
			return true;
		}
	}

	return false;
}

void UAnimMontage::SetCompositeLength(float InLength)
{
#if WITH_EDITOR
	const FFrameTime LengthInFrameTime = DataModelInterface->GetFrameRate().AsFrameTime(InLength);
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	Controller->SetNumberOfFrames(LengthInFrameTime.RoundToFrame());
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#else
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	SequenceLength = InLength;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif	
}

//////////////////////////////////////////////////////////////////////////////////////////////
// MontageInstance
// 蒙太奇实例
/////////////////////////////////////////////////////////////////////////////////////////////

FAnimMontageInstance::FAnimMontageInstance()
	: Montage(nullptr)
	, bPlaying(false)
	, DefaultBlendTimeMultiplier(1.0f)
	, bDidUseMarkerSyncThisTick(false)
	, AnimInstance(nullptr)
	, InstanceID(INDEX_NONE)
	, Position(0.f)
	, PlayRate(1.f)
	, bInterrupted(false)
	, PreviousWeight(0.f)
	, NotifyWeight(0.f)
	, BlendStartAlpha(0.0f)
	, SyncGroupName(NAME_None)
	, ActiveBlendProfile(nullptr)
	, ActiveBlendProfileMode(EBlendProfileMode::TimeFactor)
	, DisableRootMotionCount(0)
	, MontageSyncLeader(nullptr)
	, MontageSyncUpdateFrameCounter(INDEX_NONE)
{
}

FAnimMontageInstance::FAnimMontageInstance(UAnimInstance * InAnimInstance)
	: Montage(nullptr)
	, bPlaying(false)
	, DefaultBlendTimeMultiplier(1.0f)
	, bDidUseMarkerSyncThisTick(false)
	, bEnableAutoBlendOut(true)
	, AnimInstance(InAnimInstance)
	, InstanceID(INDEX_NONE)
	, Position(0.f)
	, PlayRate(1.f)
	, bInterrupted(false)
	, PreviousWeight(0.f)
	, NotifyWeight(0.f)
	, BlendStartAlpha(0.0f)
	, SyncGroupName(NAME_None)
	, ActiveBlendProfile(nullptr)
	, ActiveBlendProfileMode(EBlendProfileMode::TimeFactor)
	, DisableRootMotionCount(0)
	, MontageSyncLeader(nullptr)
	, MontageSyncUpdateFrameCounter(INDEX_NONE)
{
}

void FAnimMontageInstance::Play(float InPlayRate)
{
	FMontageBlendSettings BlendInSettings;

	// Fill settings from our Montage asset
 // 从我们的蒙太奇资源填充设置
	if (Montage)
	{
		BlendInSettings.Blend = Montage->BlendIn;
		BlendInSettings.BlendMode = Montage->BlendModeIn;
		BlendInSettings.BlendProfile = Montage->BlendProfileIn;
	}

	Play(InPlayRate, BlendInSettings);
}

void FAnimMontageInstance::Play(float InPlayRate, const FMontageBlendSettings& BlendInSettings)
{
	bPlaying = true;
	PlayRate = InPlayRate;

	// if this doesn't exist, nothing works
 // 如果这不存在，则没有任何作用
	check(Montage);
	
	// Inertialization
 // 惯性化
	FAlphaBlendArgs BlendInArgs = BlendInSettings.Blend;
	if (AnimInstance.IsValid() && BlendInSettings.BlendMode == EMontageBlendMode::Inertialization)
	{
		FInertializationRequest Request;
		Request.Duration = BlendInArgs.BlendTime;
		Request.BlendMode = BlendInArgs.BlendOption;
		Request.CustomBlendCurve = BlendInArgs.CustomCurve;
		Request.BlendProfile = BlendInSettings.BlendProfile;
		Request.bUseBlendMode = (BlendInArgs.BlendOption != EAlphaBlendOption::Linear) || (BlendInArgs.CustomCurve != nullptr);

		// Request new inertialization for new montage's group name
  // 为新蒙太奇的组名称请求新的惯性化
		// If there is an existing inertialization request, we overwrite that here.
  // 如果存在现有的惯性化请求，我们将在此处覆盖该请求。
		AnimInstance->RequestMontageInertialization(Montage, Request);

		// When using inertialization, we need to instantly blend in.
  // 当使用惯性时，我们需要立即融入。
		BlendInArgs.BlendTime = 0.0f;
	}

	// set blend option
 // 设置混合选项
	float CurrentWeight = Blend.GetBlendedValue();
	InitializeBlend(FAlphaBlend(BlendInArgs));
	BlendStartAlpha = Blend.GetAlpha();
	Blend.SetBlendTime(BlendInArgs.BlendTime * DefaultBlendTimeMultiplier);
	Blend.SetValueRange(CurrentWeight, 1.f);
	bEnableAutoBlendOut = Montage->bEnableAutoBlendOut;

	ActiveBlendProfile = BlendInSettings.BlendProfile;
}

void FAnimMontageInstance::InitializeBlend(const FAlphaBlend& InAlphaBlend)
{
	Blend.SetBlendOption(InAlphaBlend.GetBlendOption());
	Blend.SetCustomCurve(InAlphaBlend.GetCustomCurve());
	Blend.SetBlendTime(InAlphaBlend.GetBlendTime());
}

void FAnimMontageInstance::Stop(const FMontageBlendSettings& InBlendOutSettings, bool bInterrupt)
{
	if (Montage)
	{
		UE_LOG(LogAnimMontage, Verbose, TEXT("Montage.Stop Before: AnimMontage: %s,  (DesiredWeight:%0.2f, Weight:%0.2f)"),
			*Montage->GetName(), GetDesiredWeight(), GetWeight());
	}

	// overwrite bInterrupted if it hasn't already interrupted
 // 如果 bInterrupted 尚未中断，则覆盖它
	// once interrupted, you don't go back to non-interrupted
 // 一旦被打断，你就不会再回到未被打断的状态
	if (!bInterrupted && bInterrupt)
	{
		bInterrupted = bInterrupt;
	}

	// if it hasn't stopped, stop now
 // 如果还没有停止，现在就停止
	if (IsStopped() == false)
	{
		// If we are using Inertial Blend, blend time should be 0 to instantly stop the montage.
  // 如果我们使用惯性混合，混合时间应为 0 以立即停止蒙太奇。
		FAlphaBlendArgs BlendOutArgs = InBlendOutSettings.Blend;
		const bool bShouldInertialize = InBlendOutSettings.BlendMode == EMontageBlendMode::Inertialization;
		BlendOutArgs.BlendTime = bShouldInertialize ? 0.0f : BlendOutArgs.BlendTime;

		// do not use default Montage->BlendOut 
  // 不要使用默认的 Montage->BlendOut
		// depending on situation, the BlendOut time can change 
  // 根据情况，BlendOut 时间可能会改变
		InitializeBlend(FAlphaBlend(BlendOutArgs));
		BlendStartAlpha = Blend.GetAlpha();
		Blend.SetDesiredValue(0.f);
		Blend.Update(0.0f);

		// Only change the active blend profile if the montage isn't stopped. This is to prevent pops on a sudden blend profile switch
  // 仅当蒙太奇未停止时才更改活动混合配置文件。这是为了防止突然切换混合配置文件时出现爆裂声
		ActiveBlendProfile = InBlendOutSettings.BlendProfile;

		if(Montage)
		{
			if (UAnimInstance* Inst = AnimInstance.Get())
			{
				FAnimMontageInstance* SelfPtr = this;
				uint32 LastMontageFlushFrame = Inst->GetLastMontageFlushFrame();
				
				// Let AnimInstance know we are being stopped.
    // 让 AnimInstance 知道我们被阻止了。
				Inst->OnMontageInstanceStopped(*this);
				Inst->QueueMontageBlendingOutEvent(FQueuedMontageBlendingOutEvent(Montage, bInterrupted, OnMontageBlendingOutStarted));
				
				// UninitializeAnimation() was called for the our animation instance when executing the blending out event(s).
    // 在执行混合事件时，为我们的动画实例调用 UninitializeAnimation()。
				if (!Inst->IsInitialized() && MontageCVars::bEarlyOutMontageWhenUninitialized)
				{
					return;
				}

				// Prone to have being free self from trigger montage events.
    // 容易将自己从触发蒙太奇事件中解放出来。
				if (LastMontageFlushFrame != Inst->GetLastMontageFlushFrame() && Inst->MontageInstances.Find(SelfPtr) == INDEX_NONE && MontageCVars::bEarlyOutMontageWhenUninitialized)
				{
					return;
				}
				
				if (bShouldInertialize)
				{
					FInertializationRequest Request;
					Request.Duration = InBlendOutSettings.Blend.BlendTime;
					Request.BlendMode = InBlendOutSettings.Blend.BlendOption;
					Request.CustomBlendCurve = InBlendOutSettings.Blend.CustomCurve;
					Request.BlendProfile = InBlendOutSettings.BlendProfile;
					Request.bUseBlendMode = (InBlendOutSettings.Blend.BlendOption != EAlphaBlendOption::Linear) || (InBlendOutSettings.Blend.CustomCurve != nullptr);

					// Send the inertial blend request to the anim instance
     // 将惯性混合请求发送到动画实例
					Inst->RequestMontageInertialization(Montage, Request);
				}
			}
		}
	}
	else
	{
		// it is already stopped, but new montage blendtime is shorter than what 
  // 它已经停止了，但是新的蒙太奇混合时间比原来的要短
		// I'm blending out, that means this needs to readjust blendtime
  // 我正在混合，这意味着需要重新调整混合时间
		// that way we don't accumulate old longer blendtime for newer montage to play
  // 这样我们就不会为新的蒙太奇播放积累旧的较长混合时间
		if (InBlendOutSettings.Blend.BlendTime < Blend.GetBlendTime())
		{
			// I don't know if also using inBlendOut is better than
   // 我不知道同时使用 inBlendOut 是否比
			// currently set up blend option, but it might be worse to switch between 
   // 当前设置了混合选项，但在两者之间切换可能会更糟
			// blending out, but it is possible options in the future
   // 混合，但未来可能有选择
			Blend.SetBlendTime(InBlendOutSettings.Blend.BlendTime);
			BlendStartAlpha = Blend.GetAlpha();
			// have to call this again to restart blending with new blend time
   // 必须再次调用它才能以新的混合时间重新开始混合
			// we don't change blend options
   // 我们不改变混合选项
			Blend.SetDesiredValue(0.f);
		}
	}

	// if blending time < 0.f
 // 如果混合时间 < 0.f
	// set the playing to be false
 // 设置播放为 false
	// @todo is this better to be IsComplete? 
 // @todo IsComplete 更好吗？
	// or maybe we need this for if somebody sets blend time to be 0.f
 // 或者如果有人将混合时间设置为 0.f，我们可能需要这个
	if (Blend.GetBlendTime() <= 0.0f)
	{
		bPlaying = false;
	}

	if (Montage != nullptr)
	{
		UE_LOG(LogAnimMontage, Verbose, TEXT("Montage.Stop After: AnimMontage: %s,  (DesiredWeight:%0.2f, Weight:%0.2f)"),
			*Montage->GetName(), GetDesiredWeight(), GetWeight());
	}
}

void FAnimMontageInstance::Stop(const FAlphaBlend& InBlendOut, bool bInterrupt/*=true*/)
{
	FMontageBlendSettings BlendOutSettings;
	BlendOutSettings.Blend = InBlendOut;

	// Fill our other settings from the montage asset
 // 从蒙太奇资源中填写我们的其他设置
	if (Montage)
	{
		BlendOutSettings.BlendMode = Montage->BlendModeOut;
		BlendOutSettings.BlendProfile = Montage->BlendProfileOut;
	}

	Stop(BlendOutSettings, bInterrupt);
}

void FAnimMontageInstance::Pause()
{
	bPlaying = false;
}

void FAnimMontageInstance::Initialize(class UAnimMontage * InMontage)
{
	// Generate unique ID for this instance
 // 为该实例生成唯一ID
	static int32 IncrementInstanceID = 0;
	InstanceID = IncrementInstanceID++;

	if (InMontage)
	{
		Montage = InMontage;
		SetPosition(0.f);
		BlendStartAlpha = 0.0f;
		// initialize Blend
  // 初始化混合
		Blend.SetValueRange(0.f, 1.0f);
		RefreshNextPrevSections();

		if (AnimInstance.IsValid() && Montage->CanUseMarkerSync())
		{
			SyncGroupName = Montage->SyncGroup;
		}

		MontageSubStepper.Initialize(*this);
	}
}

void FAnimMontageInstance::RefreshNextPrevSections()
{
	// initialize next section
 // 初始化下一节
	if ( Montage->CompositeSections.Num() > 0 )
	{
		NextSections.Empty(Montage->CompositeSections.Num());
		NextSections.AddUninitialized(Montage->CompositeSections.Num());
		PrevSections.Empty(Montage->CompositeSections.Num());
		PrevSections.AddUninitialized(Montage->CompositeSections.Num());

		for (int32 I=0; I<Montage->CompositeSections.Num(); ++I)
		{
			PrevSections[I] = INDEX_NONE;
		}

		for (int32 I=0; I<Montage->CompositeSections.Num(); ++I)
		{
			FCompositeSection & Section = Montage->CompositeSections[I];
			int32 NextSectionIdx = Montage->GetSectionIndex(Section.NextSectionName);
			NextSections[I] = NextSectionIdx;
			if (NextSections.IsValidIndex(NextSectionIdx))
			{
				PrevSections[NextSectionIdx] = I;
			}
		}
	}
}

void FAnimMontageInstance::AddReferencedObjects( FReferenceCollector& Collector )
{
	if (Montage)
	{
		Collector.AddReferencedObject(Montage);
	}
}

void FAnimMontageInstance::Terminate()
{
	SCOPE_CYCLE_COUNTER(STAT_AnimMontageInstance_Terminate);

	if (Montage == nullptr)
	{
		return;
	}

	UAnimMontage* OldMontage = Montage;
	
	if (AnimInstance.IsValid())
	{
		// Must grab a reference on the stack in case "this" is deleted during iteration
  // 必须获取堆栈上的引用，以防“this”在迭代过程中被删除
		TWeakObjectPtr<UAnimInstance> AnimInstanceLocal = AnimInstance;

		// End all active State BranchingPoints
  // 结束所有活动的 State BranchingPoints
		for (int32 Index = ActiveStateBranchingPoints.Num() - 1; Index >= 0; Index--)
		{
			FAnimNotifyEvent& NotifyEvent = ActiveStateBranchingPoints[Index];

			if (NotifyEvent.NotifyStateClass)
			{
				FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, &NotifyEvent, InstanceID, false);
				TRACE_ANIM_NOTIFY(AnimInstance.Get(), NotifyEvent, End);
				NotifyEvent.NotifyStateClass->BranchingPointNotifyEnd(BranchingPointNotifyPayload);

				if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal, NotifyEvent.NotifyStateClass))
				{
					return;
				}
			}
		}
		ActiveStateBranchingPoints.Empty();

		FAnimMontageInstance* SelfPtr = this;
		UAnimInstance* Inst = AnimInstance.Get();
		uint32 LastMontageFlushFrame = Inst->GetLastMontageFlushFrame();
		
		// terminating, trigger end
  // 终止，触发结束
		AnimInstance->QueueMontageEndedEvent(FQueuedMontageEndedEvent(OldMontage, InstanceID, bInterrupted, OnMontageEnded));

		// UninitializeAnimation() empties the MontagesInstances array thus we need to exit early to not use freed memory.
  // UninitializeAnimation() 清空 MontagesInstances 数组，因此我们需要提前退出以不使用释放的内存。
		if (!AnimInstance->IsInitialized() && MontageCVars::bEarlyOutMontageWhenUninitialized)
		{
			return;
		}

		// Prone to have being free self from trigger montage events.
  // 容易将自己从触发蒙太奇事件中解放出来。
		if (LastMontageFlushFrame != Inst->GetLastMontageFlushFrame() && Inst->MontageInstances.Find(SelfPtr) == INDEX_NONE && MontageCVars::bEarlyOutMontageWhenUninitialized)
		{
			return;
		}
		
		// Clear references to this MontageInstance. Needs to happen before Montage is cleared to nullptr, as TMaps can use that as a key.
  // 清除对此 MontageInstance 的引用。需要在 Montage 被清除为 nullptr 之前发生，因为 TMap 可以使用它作为键。
		AnimInstance->ClearMontageInstanceReferences(*this);
	}

	// clear Blend curve
 // 清晰的混合曲线
	Blend.SetCustomCurve(nullptr);
	Blend.SetBlendOption(EAlphaBlendOption::Linear);

	ActiveBlendProfile = nullptr;
	Montage = nullptr;

	UE_LOG(LogAnimMontage, Verbose, TEXT("Terminating: AnimMontage: %s"), *GetNameSafe(OldMontage));
}

bool FAnimMontageInstance::JumpToSectionName(FName const & SectionName, bool bEndOfSection)
{
	const int32 SectionID = Montage->GetSectionIndex(SectionName);

	if (Montage->IsValidSectionIndex(SectionID))
	{
		FCompositeSection & CurSection = Montage->GetAnimCompositeSection(SectionID);
		const float NewPosition = Montage->CalculatePos(CurSection, bEndOfSection ? Montage->GetSectionLength(SectionID) - UE_KINDA_SMALL_NUMBER : 0.0f);
		SetPosition(NewPosition);
		OnMontagePositionChanged(SectionName);
		
		// changed sections, trigger event
  // 更改部分，触发事件
		const bool bLooped = Montage->IsValidSectionIndex(MontageSubStepper.GetCurrentSectionIndex()) ? MontageSubStepper.GetCurrentSectionIndex() == SectionID : false;
		AnimInstance->QueueMontageSectionChangedEvent(FQueuedMontageSectionChangedEvent(Montage, InstanceID, SectionName, bLooped, OnMontageSectionChanged));
		
		return true;
	}

	UE_LOG(LogAnimMontage, Warning, TEXT("JumpToSectionName %s bEndOfSection: %d failed for Montage %s"),
		*SectionName.ToString(), bEndOfSection, *GetNameSafe(Montage));
	return false;
}

bool FAnimMontageInstance::SetNextSectionName(FName const & SectionName, FName const & NewNextSectionName)
{
	int32 const SectionID = Montage->GetSectionIndex(SectionName);
	int32 const NewNextSectionID = Montage->GetSectionIndex(NewNextSectionName);

	return SetNextSectionID(SectionID, NewNextSectionID);
}

bool FAnimMontageInstance::SetNextSectionID(int32 const & SectionID, int32 const & NewNextSectionID)
{
	bool const bHasValidNextSection = NextSections.IsValidIndex(SectionID);

	// disconnect prev section
 // 断开上一节
	if (bHasValidNextSection && (NextSections[SectionID] != INDEX_NONE) && PrevSections.IsValidIndex(NextSections[SectionID]))
	{
		PrevSections[NextSections[SectionID]] = INDEX_NONE;
	}

	// update in-reverse next section
 // 反向更新下一节
	if (PrevSections.IsValidIndex(NewNextSectionID))
	{
		PrevSections[NewNextSectionID] = SectionID;
	}

	// update next section for the SectionID
 // 更新下一节的SectionID
	// NextSection can be invalid
 // NextSection 可能无效
	if (bHasValidNextSection)
	{
		NextSections[SectionID] = NewNextSectionID;
		OnMontagePositionChanged(GetSectionNameFromID(NewNextSectionID));
		return true;
	}

	UE_LOG(LogAnimMontage, Warning, TEXT("SetNextSectionName %s to %s failed for Montage %s"),
		*GetSectionNameFromID(SectionID).ToString(), *GetSectionNameFromID(NewNextSectionID).ToString(), *GetNameSafe(Montage));

	return false;
}

void FAnimMontageInstance::OnMontagePositionChanged(FName const & ToSectionName) 
{
	if (bPlaying && IsStopped())
	{
		UE_LOG(LogAnimMontage, Warning, TEXT("Changing section on Montage (%s) to '%s' during blend out. This can cause incorrect visuals!"),
			*GetNameSafe(Montage), *ToSectionName.ToString());

		Play(PlayRate);
	}
}

FName FAnimMontageInstance::GetCurrentSection() const
{
	if ( Montage )
	{
		float CurrentPosition;
		const int32 CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(Position, CurrentPosition);
		if ( Montage->IsValidSectionIndex(CurrentSectionIndex) )
		{
			FCompositeSection& CurrentSection = Montage->GetAnimCompositeSection(CurrentSectionIndex);
			return CurrentSection.SectionName;
		}
	}

	return NAME_None;
}

FName FAnimMontageInstance::GetNextSection() const
{
	if (Montage)
	{
		float CurrentPosition;
		const int32 CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(Position, CurrentPosition);
		if (Montage->IsValidSectionIndex(CurrentSectionIndex))
		{
			const int32 NextSectionIndex = GetNextSectionID(CurrentSectionIndex);
			if (Montage->IsValidSectionIndex(NextSectionIndex))
			{
				return GetSectionNameFromID(NextSectionIndex);
			}
		}
	}

	return NAME_None;
}

int32 FAnimMontageInstance::GetNextSectionID(int32 const & CurrentSectionID) const
{
	return NextSections.IsValidIndex(CurrentSectionID) ? NextSections[CurrentSectionID] : INDEX_NONE;
}

FName FAnimMontageInstance::GetSectionNameFromID(int32 const & SectionID) const
{
	if (Montage && Montage->IsValidSectionIndex(SectionID))
	{
		FCompositeSection const & CurrentSection = Montage->GetAnimCompositeSection(SectionID);
		return CurrentSection.SectionName;
	}

	return NAME_None;
}

void FAnimMontageInstance::MontageSync_Follow(struct FAnimMontageInstance* NewLeaderMontageInstance)
{
	// Stop following previous leader if any.
 // 如果有的话，请停止跟随以前的领导者。
	MontageSync_StopFollowing();

	// Follow new leader
 // 跟随新领导
	// Note: we don't really care about detecting loops there, there's no real harm in doing so.
 // 注意：我们并不真正关心检测那里的循环，这样做并没有真正的危害。
	if (NewLeaderMontageInstance && (NewLeaderMontageInstance != this))
	{
		NewLeaderMontageInstance->MontageSyncFollowers.AddUnique(this);
		MontageSyncLeader = NewLeaderMontageInstance;
	}
}

void FAnimMontageInstance::MontageSync_StopLeading()
{
	for (auto MontageSyncFollower : MontageSyncFollowers)
	{
		if (MontageSyncFollower)
		{
			ensure(MontageSyncFollower->MontageSyncLeader == this);
			MontageSyncFollower->MontageSyncLeader = nullptr;
		}
	}
	MontageSyncFollowers.Empty();
}

void FAnimMontageInstance::MontageSync_StopFollowing()
{
	if (MontageSyncLeader)
	{
		MontageSyncLeader->MontageSyncFollowers.RemoveSingleSwap(this);
		MontageSyncLeader = nullptr;
	}
}

uint32 FAnimMontageInstance::MontageSync_GetFrameCounter() const
{
	return (GFrameCounter % MAX_uint32);
}

bool FAnimMontageInstance::MontageSync_HasBeenUpdatedThisFrame() const
{
	return (MontageSyncUpdateFrameCounter == MontageSync_GetFrameCounter());
}

void FAnimMontageInstance::MontageSync_PreUpdate()
{
	// If we are being synchronized to a leader
 // 如果我们正在同步到领导者
	// And our leader HASN'T been updated yet, then we need to synchronize ourselves now.
 // 而我们的leader还没有更新，那么我们现在需要同步自己。
	// We're basically synchronizing to last frame's values.
 // 我们基本上同步到最后一帧的值。
	// If we want to avoid that frame of lag, a tick prerequisite should be put between the follower and the leader.
 // 如果我们想避免这种滞后框架，就应该在追随者和领导者之间放置一个勾选先决条件。
	if (MontageSyncLeader && !MontageSyncLeader->MontageSync_HasBeenUpdatedThisFrame())
	{
		MontageSync_PerformSyncToLeader();
	}
}

void FAnimMontageInstance::MontageSync_PostUpdate()
{
	// Tag ourselves as updated this frame.
 // 将我们自己标记为已更新此框架。
	MontageSyncUpdateFrameCounter = MontageSync_GetFrameCounter();

	// If we are being synchronized to a leader
 // 如果我们正在同步到领导者
	// And our leader HAS already been updated, then we can synchronize ourselves now.
 // 我们的领导者已经更新了，那么我们现在可以同步了。
	// To make sure we are in sync before rendering.
 // 确保我们在渲染之前保持同步。
	if (MontageSyncLeader && MontageSyncLeader->MontageSync_HasBeenUpdatedThisFrame())
	{
		MontageSync_PerformSyncToLeader();
	}
}

void FAnimMontageInstance::MontageSync_PerformSyncToLeader()
{
	if (MontageSyncLeader)
	{
		// Sync follower position only if significant error.
  // 仅在出现重大错误时同步从动位置。
		// We don't want continually 'teleport' it, which could have side-effects and skip AnimNotifies.
  // 我们不希望不断地“传送”它，这可能会产生副作用并跳过 AnimNotify。
		const float LeaderPosition = MontageSyncLeader->GetPosition();
		const float FollowerPosition = GetPosition();
		if (FMath::Abs(FollowerPosition - LeaderPosition) > UE_KINDA_SMALL_NUMBER)
		{
			SetPosition(LeaderPosition);
		}

		SetPlayRate(MontageSyncLeader->GetPlayRate());

		// If source and target share same section names, keep them in sync as well. So we properly handle jumps and loops.
  // 如果源和目标共享相同的部分名称，也请保持它们同步。所以我们要正确处理跳转和循环。
		const FName LeaderCurrentSectionName = MontageSyncLeader->GetCurrentSection();
		if ((LeaderCurrentSectionName != NAME_None) && (GetCurrentSection() == LeaderCurrentSectionName))
		{
			const FName LeaderNextSectionName = MontageSyncLeader->GetNextSection();
			SetNextSectionName(LeaderCurrentSectionName, LeaderNextSectionName);
		}
	}
}


void FAnimMontageInstance::UpdateWeight(float DeltaTime)
{
	if ( IsValid() )
	{
		PreviousWeight = Blend.GetBlendedValue();
		const bool bWasComplete = Blend.IsComplete();

		// update weight
  // 更新权重
		Blend.Update(DeltaTime);

		if (Blend.GetBlendTimeRemaining() < 0.0001f)
		{
			ActiveBlendProfile = nullptr;
		}

		if (!IsStopped() && !bWasComplete && Blend.IsComplete())
		{
			if (UAnimInstance* Inst = AnimInstance.Get())
			{
				Inst->QueueMontageBlendedInEvent(FQueuedMontageBlendedInEvent(Montage, OnMontageBlendedInEnded));
			}
		}

		// Notify weight is max of previous and current as notify could have come
  // 通知权重是先前和当前的最大值，因为通知可能已经到来
		// from any point between now and last tick
  // 从现在到最后一个报价之间的任何点
		NotifyWeight = FMath::Max(PreviousWeight, Blend.GetBlendedValue());

		UE_LOG(LogAnimMontage, Verbose, TEXT("UpdateWeight: AnimMontage: %s,  (DesiredWeight:%0.2f, Weight:%0.2f, PreviousWeight: %0.2f)"),
			*Montage->GetName(), GetDesiredWeight(), GetWeight(), PreviousWeight);
		UE_LOG(LogAnimMontage, Verbose, TEXT("Blending Info: BlendOption : %d, AlphaLerp : %0.2f, BlendTime: %0.2f"),
			(int32)Blend.GetBlendOption(), Blend.GetAlpha(), Blend.GetBlendTime());
	}
}

bool FAnimMontageInstance::SimulateAdvance(float DeltaTime, float& InOutPosition, FRootMotionMovementParams & OutRootMotionParams) const
{
	if (!IsValid())
	{
		return false;
	}

	const bool bExtractRootMotion = Montage->HasRootMotion() && !IsRootMotionDisabled();

	FMontageSubStepper SimulateMontageSubStepper;
	SimulateMontageSubStepper.Initialize(*this);
	SimulateMontageSubStepper.AddEvaluationTime(DeltaTime);
	while (SimulateMontageSubStepper.HasTimeRemaining())
	{
		const float PreviousSubStepPosition = InOutPosition;
		EMontageSubStepResult SubStepResult = SimulateMontageSubStepper.Advance(InOutPosition, nullptr);

		if (SubStepResult != EMontageSubStepResult::Moved)
		{
			// stop and leave this loop
   // 停止并离开这个循环
			break;
		}

		// Extract Root Motion for this time slice, and accumulate it.
  // 提取该时间片的根运动并累加。
		if (bExtractRootMotion)
		{
			OutRootMotionParams.Accumulate(Montage->ExtractRootMotionFromTrackRange(PreviousSubStepPosition, InOutPosition, FAnimExtractContext()));
		}

		// if we reached end of section, and we were not processing a branching point, and no events has messed with out current position..
  // 如果我们到达部分末尾，并且我们没有处理分支点，并且没有事件扰乱当前位置..
		// .. Move to next section.
  // .. 移至下一节。
		// (this also handles looping, the same as jumping to a different section).
  // （这也处理循环，与跳转到不同的部分相同）。
		if (SimulateMontageSubStepper.HasReachedEndOfSection())
		{
			const int32 CurrentSectionIndex = SimulateMontageSubStepper.GetCurrentSectionIndex();
			const bool bPlayingForward = SimulateMontageSubStepper.GetbPlayingForward();

			// Get recent NextSectionIndex in case it's been changed by previous events.
   // 获取最近的 NextSectionIndex，以防它被之前的事件更改。
			const int32 RecentNextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];
			if (RecentNextSectionIndex != INDEX_NONE)
			{
				float LatestNextSectionStartTime;
				float LatestNextSectionEndTime;
				Montage->GetSectionStartAndEndTime(RecentNextSectionIndex, LatestNextSectionStartTime, LatestNextSectionEndTime);

				// Jump to next section's appropriate starting point (start or end).
    // 跳转到下一部分的适当起点（开始或结束）。
				InOutPosition = bPlayingForward ? LatestNextSectionStartTime : (LatestNextSectionEndTime - UE_KINDA_SMALL_NUMBER); // remain within section
			}
			else
			{
				// Clamp position to prevent playing past the end of the current section
    // 限制位置以防止播放超过当前部分的结尾
				float CurrentSectionStartTime, CurrentSectionEndTime;
				Montage->GetSectionStartAndEndTime(CurrentSectionIndex, CurrentSectionStartTime, CurrentSectionEndTime);

				InOutPosition = bPlayingForward ? (CurrentSectionEndTime - UE_KINDA_SMALL_NUMBER) : CurrentSectionStartTime;

				// Reached end of last section. Exit.
    // 到达最后一节的结尾。出口。
				break;
			}
		}
	}

	return true;
}

FSlotAnimationTrack::FSlotAnimationTrack()
	: SlotName(FAnimSlotGroup::DefaultSlotName)
{}

void FMontageSubStepper::Initialize(const struct FAnimMontageInstance& InAnimInstance)
{
	MontageInstance = &InAnimInstance;
	Montage = MontageInstance->Montage;
}

EMontageSubStepResult FMontageSubStepper::Advance(float& InOut_P_Original, const FBranchingPointMarker** OutBranchingPointMarkerPtr)
{
	DeltaMove = 0.f;

	if (MontageInstance == nullptr || (Montage == nullptr))
	{
		return EMontageSubStepResult::InvalidMontage;
	}

	bReachedEndOfSection = false;

	// Update Current Section info in case it's needed by the montage's update loop.
 // 更新当前部分信息，以防蒙太奇更新循环需要。
	// We need to do this even if we're not going to move this frame.
 // 即使我们不打算移动这个框架，我们也需要这样做。
	// We could have been moved externally via a SetPosition() call.
 // 我们可以通过 SetPosition() 调用从外部移动。
	float PositionInSection;
	CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(InOut_P_Original, PositionInSection);
	if (!Montage->IsValidSectionIndex(CurrentSectionIndex))
	{
		return EMontageSubStepResult::InvalidSection;
	}

	const FCompositeSection& CurrentSection = Montage->GetAnimCompositeSection(CurrentSectionIndex);
	CurrentSectionStartTime = CurrentSection.GetTime();

	// Find end of current section. We only update one section at a time.
 // 查找当前部分的结尾。我们一次只更新一个部分。
	CurrentSectionLength = Montage->GetSectionLength(CurrentSectionIndex);

	if (!MontageInstance->bPlaying || FMath::IsNearlyZero(TimeRemaining))
	{
		return EMontageSubStepResult::NotMoved;
	}

	// If we're forcing next position, this is our DeltaMove.
 // 如果我们强制下一个位置，这就是我们的 DeltaMove。
	// We don't use play rate and delta time to move.
 // 我们不使用播放速率和增量时间来移动。
	if (MontageInstance->ForcedNextToPosition.IsSet())
	{
		const float NewPosition = MontageInstance->ForcedNextToPosition.GetValue();
		if (MontageInstance->ForcedNextFromPosition.IsSet())
		{
			// We are modifying the current position so we also need to update the section and pos in section
   // 我们正在修改当前位置，因此我们还需要更新部分和部分中的位置
			InOut_P_Original = MontageInstance->ForcedNextFromPosition.GetValue();
			CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(InOut_P_Original, PositionInSection);
			
			if (!Montage->IsValidSectionIndex(CurrentSectionIndex))
			{
				return EMontageSubStepResult::InvalidSection;
			}
			CurrentSectionStartTime = Montage->GetAnimCompositeSection(CurrentSectionIndex).GetTime();
			CurrentSectionLength = Montage->GetSectionLength(CurrentSectionIndex);
		}
		DeltaMove = NewPosition - InOut_P_Original;
		PlayRate = DeltaMove / TimeRemaining;
		bPlayingForward = (DeltaMove >= 0.f);
		TimeStretchMarkerIndex = INDEX_NONE;
	}
	else 
	{
		PlayRate = MontageInstance->PlayRate * Montage->RateScale;

		if (FMath::IsNearlyZero(PlayRate))
		{
			return EMontageSubStepResult::NotMoved;
		}

		// See if we can attempt to use a TimeStretchCurve.
  // 看看我们是否可以尝试使用 TimeStretchCurve。
		const bool bAttemptTimeStretchCurve = Montage->TimeStretchCurve.IsValid() && !FMath::IsNearlyEqual(PlayRate, 1.f);
		if (bAttemptTimeStretchCurve)
		{
			// First we need to see if we have valid cached data and if it is up to date.
   // 首先，我们需要查看是否有有效的缓存数据以及是否是最新的。
			ConditionallyUpdateTimeStretchCurveCachedData();
		}

		// If we're not using a TimeStretchCurve, play rate is constant.
  // 如果我们不使用 TimeStretchCurve，播放速率是恒定的。
		if (!bAttemptTimeStretchCurve || !bHasValidTimeStretchCurveData)
		{
			bPlayingForward = (PlayRate > 0.f);
			DeltaMove = TimeRemaining * PlayRate;
			TimeStretchMarkerIndex = INDEX_NONE;
		}
		else
		{
			// We're using a TimeStretchCurve.
   // 我们正在使用 TimeStretchCurve。

			// Find P_Target for current InOut_P_Original.
   // 查找当前 InOut_P_Original 的 P_Target。
			// Not that something external could have modified the montage's position.
   // 并不是说外部因素可以改变蒙太奇的位置。
			// So we need to refresh our P_Target.
   // 所以我们需要刷新我们的P_Target。
			float P_Target = FindMontagePosition_Target(InOut_P_Original);

			// With P_Target, we're in 'play back time' space. 
   // 有了 P_Target，我们就进入了“回放时间”空间。
			// So we can add our delta time there directly.
   // 所以我们可以直接在那里添加增量时间。
			P_Target += bPlayingForward ? TimeRemaining : -TimeRemaining;
			// Make sure we don't exceed our boundaries.
   // 确保我们不超出我们的界限。
			P_Target = TimeStretchCurveInstance.Clamp_P_Target(P_Target);

			// Now we can map this back into 'original' space and find which frame of animation we should play.
   // 现在我们可以将其映射回“原始”空间并找到我们应该播放的动画帧。
			const float NewP_Original = FindMontagePosition_Original(P_Target);

			// And from there, derive our DeltaMove and actual PlayRate for this substep.
   // 并从那里导出此子步骤的 DeltaMove 和实际 PlayRate。
			DeltaMove = NewP_Original - InOut_P_Original;
			PlayRate = DeltaMove / TimeRemaining;
		}
	}

	// Now look for a branching point. If we have one, stop there first to handle it.
 // 现在寻找一个分支点。如果我们有的话，就先停下来处理它。
	// We need to stop at branching points, because they can trigger events that can cause side effects
 // 我们需要在分支点停止，因为它们可能会触发导致副作用的事件
	// (jumping to a new position, changing sections, changing play rate, etc).
 // （跳到新位置、更改部分、更改播放速率等）。
	if (OutBranchingPointMarkerPtr)
	{
		*OutBranchingPointMarkerPtr = Montage->FindFirstBranchingPointMarker(InOut_P_Original, InOut_P_Original + DeltaMove);
		if (*OutBranchingPointMarkerPtr)
		{
			// If we have a branching point, adjust DeltaMove so we stop there.
   // 如果我们有一个分支点，请调整 DeltaMove，以便我们在那里停止。
			DeltaMove = (*OutBranchingPointMarkerPtr)->TriggerTime - InOut_P_Original;
		}
	}

	// Finally clamp DeltaMove by section markers.
 // 最后用截面标记夹住 DeltaMove。
	{
		const float OldDeltaMove = DeltaMove;

		// Clamp DeltaMove based on move allowed within current section
  // 根据当前部分允许的移动来钳制 DeltaMove
		// We stop at each section marker to evaluate whether we should jump to another section marker or not.
  // 我们在每个节标记处停下来评估是否应该跳转到另一个节标记。
		// Test is inclusive, so we know if we've reached marker or not.
  // 测试具有包容性，因此我们知道是否达到了目标。
		if (bPlayingForward)
		{
			const float MaxSectionMove = CurrentSectionLength - PositionInSection;
			if (DeltaMove >= MaxSectionMove)
			{
				DeltaMove = MaxSectionMove;
				bReachedEndOfSection = true;
			}
		}
		else
		{
			const float MinSectionMove = /* 0.f */ - PositionInSection;
			if (DeltaMove <= MinSectionMove)
			{
				DeltaMove = MinSectionMove;
				bReachedEndOfSection = true;
			}
		}

		if (OutBranchingPointMarkerPtr && *OutBranchingPointMarkerPtr && (OldDeltaMove != DeltaMove))
		{
			// Clean up the marker since we hit end of a section and overrode the delta move.
   // 清理标记，因为我们到达了一个部分的末尾并覆盖了增量移动。
			*OutBranchingPointMarkerPtr = nullptr;
		}
	}

	// DeltaMove is now final, see if it has any effect on our position.
 // DeltaMove 现已最终确定，看看它对我们的位置是否有任何影响。
	if (FMath::Abs(DeltaMove) > 0.f)
	{
		// Note that we don't worry about looping and wrapping around here.
  // 请注意，我们不担心这里的循环和环绕。
		// We step per section to simplify code to extract notifies/root motion/etc.
  // 我们按节逐步简化代码以提取通知/根运动/等。
		InOut_P_Original += DeltaMove;

		// Decrease RemainingTime with actual time elapsed 
  // 随着实际时间的流逝而减少 RemainingTime
		// So we can take more substeps as needed.
  // 因此我们可以根据需要采取更多的子步骤。
		const float TimeStep = DeltaMove / PlayRate;
		ensure(TimeStep >= 0.f);
		TimeRemaining = FMath::Max(TimeRemaining - TimeStep, 0.f);

		return EMontageSubStepResult::Moved;
	}
	else
	{
		return EMontageSubStepResult::NotMoved;
	}
}

void FMontageSubStepper::ConditionallyUpdateTimeStretchCurveCachedData()
{
	// CombinedPlayRate defines our overall desired play back time, aka T_Target.
 // CombinedPlayRate 定义了我们所需的总体播放时间，也称为 T_Target。
	// When using a TimeStretchCurve, this also defines S and U.
 // 使用 TimeStretchCurve 时，这还定义了 S 和 U。
	// Only update these if CombinedPlayRate has changed.
 // 仅当 CombinedPlayRate 发生更改时才更新这些内容。
	const float CombinedPlayRate = MontageInstance->PlayRate * Montage->RateScale;
	if (CombinedPlayRate == Cached_CombinedPlayRate)
	{
		return;
	}
	Cached_CombinedPlayRate = CombinedPlayRate;
	
	// We'll set this to true at the end, if we succeed with valid data.
 // 如果我们成功获得有效数据，我们将在最后将其设置为 true。
	bHasValidTimeStretchCurveData = false;

	// We should not be using this code path with a 0 play rate
 // 我们不应该以 0 播放率使用此代码路径
	// or a 1 play rate. we can use traditional cheaper update without curve.
 // 或 1 播放率。我们可以使用传统的更便宜的更新，无需曲线。
	ensure(!FMath::IsNearlyZero(CombinedPlayRate));
	ensure(!FMath::IsNearlyEqual(CombinedPlayRate, 1.f));

	bPlayingForward = (CombinedPlayRate > 0.f);
	TimeStretchCurveInstance.InitializeFromPlayRate(CombinedPlayRate, Montage->TimeStretchCurve);

	/*
		Section Segment Positions in Target space will have to be re-cached, as needed.
		This is to determine 'remaining time until end' to trigger blend outs.
		But most montages don't use sections.
		So this is optional and done on demand.
	*/
	{
		const int32 NumSections = Montage->CompositeSections.Num();
		SectionStartPositions_Target.Reset(NumSections);
		SectionStartPositions_Target.Init(-1.f, NumSections);
		SectionEndPositions_Target.Reset(NumSections);
		SectionEndPositions_Target.Init(-1.f, NumSections);
	}

	bHasValidTimeStretchCurveData = TimeStretchCurveInstance.HasValidData();
}

float FMontageSubStepper::FindMontagePosition_Target(float In_P_Original)
{
	check(bHasValidTimeStretchCurveData);

	// See if our cached version is not up to date.
 // 查看我们的缓存版本是否是最新的。
	// Then we need to update it.
 // 然后我们需要更新它。
	if (In_P_Original != Cached_P_Original)
	{
		// Update cached value.
  // 更新缓存值。
		Cached_P_Original = In_P_Original;

		// Update TimeStretchMarkerIndex if needed.
  // 如果需要，更新 TimeStretchMarkerIndex。
		// This would happen if we jumped position due to sections or external input.
  // 如果我们由于部分或外部输入而跳跃位置，就会发生这种情况。
		TimeStretchCurveInstance.UpdateMarkerIndexForPosition(TimeStretchMarkerIndex, Cached_P_Original, TimeStretchCurveInstance.GetMarkers_Original());

		// With an accurate TimeStretchMarkerIndex, we can map P_Original to P_Target
  // 有了准确的 TimeStretchMarkerIndex，我们就可以将 P_Original 映射到 P_Target
		Cached_P_Target = TimeStretchCurveInstance.Convert_P_Original_To_Target(TimeStretchMarkerIndex, Cached_P_Original);
	}

	return Cached_P_Target;
}

float FMontageSubStepper::FindMontagePosition_Original(float In_P_Target)
{
	check(bHasValidTimeStretchCurveData);

	// See if our cached version is not up to date.
 // 查看我们的缓存版本是否是最新的。
	// Then we need to update it.
 // 然后我们需要更新它。
	if (In_P_Target != Cached_P_Target)
	{
		// Update cached value.
  // 更新缓存值。
		Cached_P_Target = In_P_Target;

		// Update TimeStretchMarkerIndex if needed.
  // 如果需要，更新 TimeStretchMarkerIndex。
		// This would happen if we jumped position due to sections or external input.
  // 如果我们由于部分或外部输入而跳跃位置，就会发生这种情况。
		TimeStretchCurveInstance.UpdateMarkerIndexForPosition(TimeStretchMarkerIndex, Cached_P_Target, TimeStretchCurveInstance.GetMarkers_Target());

		// With an accurate TimeStretchMarkerIndex, we can map P_Original to P_Target
  // 有了准确的 TimeStretchMarkerIndex，我们就可以将 P_Original 映射到 P_Target
		Cached_P_Original = TimeStretchCurveInstance.Convert_P_Target_To_Original(TimeStretchMarkerIndex, Cached_P_Target);
	}

	return Cached_P_Original;
}

float FMontageSubStepper::GetCurrSectionStartPosition_Target() const
{
	check(bHasValidTimeStretchCurveData);

	const float CachedSectionStartPosition_Target = SectionStartPositions_Target[CurrentSectionIndex];
	if (CachedSectionStartPosition_Target >= 0.f)
	{
		return CachedSectionStartPosition_Target;
	}

	const int32 SectionStartMarkerIndex = TimeStretchCurveInstance.BinarySearchMarkerIndex(CurrentSectionStartTime, TimeStretchCurveInstance.GetMarkers_Original());
	const float SectionStart_Target = TimeStretchCurveInstance.Convert_P_Original_To_Target(SectionStartMarkerIndex, CurrentSectionStartTime);

	SectionStartPositions_Target[CurrentSectionIndex] = SectionStart_Target;

	return SectionStart_Target;
}

float FMontageSubStepper::GetCurrSectionEndPosition_Target() const
{
	check(bHasValidTimeStretchCurveData);

	const float CachedSectionEndPosition_Target = SectionEndPositions_Target[CurrentSectionIndex];
	if (CachedSectionEndPosition_Target >= 0.f)
	{
		return CachedSectionEndPosition_Target;
	}

	const float SectionEnd_Original = CurrentSectionStartTime + CurrentSectionLength;
	const int32 SectionEndMarkerIndex = TimeStretchCurveInstance.BinarySearchMarkerIndex(SectionEnd_Original, TimeStretchCurveInstance.GetMarkers_Original());
	const float SectionEnd_Target = TimeStretchCurveInstance.Convert_P_Original_To_Target(SectionEndMarkerIndex, SectionEnd_Original);

	SectionEndPositions_Target[CurrentSectionIndex] = SectionEnd_Target;

	return SectionEnd_Target;
}

float FMontageSubStepper::GetRemainingPlayTimeToSectionEnd(const float In_P_Original)
{
	// If our current play rate is zero, we can't predict our remaining play time.
 // 如果我们当前的游戏率为零，我们就无法预测剩余的游戏时间。
	if (FMath::IsNearlyZero(PlayRate))
	{
		return UE_BIG_NUMBER;
	}

	// Find position in montage where current section ends.
 // 查找蒙太奇中当前部分结束的位置。
	const float CurrSectionEnd_Original = bPlayingForward
		? (CurrentSectionStartTime + CurrentSectionLength)
		: CurrentSectionStartTime;

	// If we have no TimeStretchCurve, it's pretty straight forward.
 // 如果我们没有 TimeStretchCurve，那就非常简单了。
	// Assume constant play rate.
 // 假设播放速率恒定。
	if (TimeStretchMarkerIndex == INDEX_NONE)
	{
		const float DeltaPositionToEnd = CurrSectionEnd_Original - In_P_Original;
		const float RemainingPlayTime = FMath::Abs(DeltaPositionToEnd / PlayRate);
		return RemainingPlayTime;
	}

	// We're using a TimeStretchCurve.
 // 我们正在使用 TimeStretchCurve。
	check(bHasValidTimeStretchCurveData);

	// Find our position in 'target' space. This is in play back time.
 // 找到我们在“目标”空间中的位置。这是回放时间。
	const float P_Target = FindMontagePosition_Target(In_P_Original);
	if (bPlayingForward)
	{
		// Find CurrSectionEnd_Target.
  // 找到 CurrSectionEnd_Target。
		if (FMath::IsNearlyEqual(CurrSectionEnd_Original, TimeStretchCurveInstance.Get_T_Original()))
		{
			const float RemainingPlayTime = (TimeStretchCurveInstance.Get_T_Target() - P_Target);
			return RemainingPlayTime;
		}
		else
		{
			const float CurrSectionEnd_Target = GetCurrSectionEndPosition_Target();
			const float RemainingPlayTime = (CurrSectionEnd_Target - P_Target);
			return RemainingPlayTime;
		}
	}
	// Playing Backwards
 // 向后播放
	else
	{
		// Find CurrSectionEnd_Target.
  // 找到 CurrSectionEnd_Target。
		if (FMath::IsNearlyEqual(CurrSectionEnd_Original, 0.f))
		{
			const float RemainingPlayTime = P_Target;
			return RemainingPlayTime;
		}
		else
		{
			const float CurrSectionStart_Target = GetCurrSectionStartPosition_Target();
			const float RemainingPlayTime = (P_Target - CurrSectionStart_Target);
			return RemainingPlayTime;
		}
	}
}

#if WITH_EDITOR
void FAnimMontageInstance::EditorOnly_PreAdvance()
{
	// this is necessary and it is not easy to do outside of here
 // 这是必要的，而且在外面不容易做到
	// since undo also can change composite sections
 // 因为撤消也可以更改复合部分
	if ((Montage->CompositeSections.Num() != NextSections.Num()) || (Montage->CompositeSections.Num() != PrevSections.Num()))
	{
		RefreshNextPrevSections();
	}

	// Auto refresh this in editor to catch changes being made to AnimNotifies.
 // 在编辑器中自动刷新以捕获对 AnimNotify 所做的更改。
	// RefreshCacheData should handle this but I'm not 100% sure it will cover all existing cases
 // RefreshCacheData 应该处理这个问题，但我不能 100% 确定它会涵盖所有现有情况
	Montage->RefreshBranchingPointMarkers();

	// Bake TimeStretchCurve in editor to catch any edits made to source curve.
 // 在编辑器中烘焙 TimeStretchCurve 以捕获对源曲线所做的任何编辑。
	Montage->BakeTimeStretchCurve();
	// Clear cached data, so it can be recached from updated time stretch curve.
 // 清除缓存数据，以便可以从更新的时间拉伸曲线重新缓存数据。
	MontageSubStepper.ClearCachedData();
}
#endif

void FAnimMontageInstance::Advance(float DeltaTime, struct FRootMotionMovementParams* OutRootMotionParams, bool bBlendRootMotion)
{
	SCOPE_CYCLE_COUNTER(STAT_AnimMontageInstance_Advance);
	FScopeCycleCounterUObject MontageScope(Montage);

	if (IsValid())
	{
		// with custom curves, we can't just filter by weight
  // 使用自定义曲线，我们不能只按重量过滤
		// also if you have custom curve with longer 0, you'll likely to pause montage during that blending time
  // 另外，如果您有更长 0 的自定义曲线，您可能会在混合时间内暂停蒙太奇
		// I think that is a bug. It still should move, the weight might come back later. 
  // 我认为这是一个错误。它仍然应该移动，重量可能会稍后恢复。
		if (bPlaying)
		{
			const bool bExtractRootMotion = (OutRootMotionParams != nullptr) && Montage->HasRootMotion();
			
			DeltaTimeRecord.Set(Position, 0.f);

			bDidUseMarkerSyncThisTick = CanUseMarkerSync();
			if (bDidUseMarkerSyncThisTick)
			{
				MarkersPassedThisTick.Reset();
			}
			
			/** 
				Limit number of iterations for performance.
				This can get out of control if PlayRate is set really high, or there is a hitch, and Montage is looping for example.
			*/
			const int32 MaxIterations = 10;
			int32 NumIterations = 0;

			/** 
				If we're hitting our max number of iterations for whatever reason,
				make sure we're not accumulating too much time, and go out of range.
			*/
			if (MontageSubStepper.GetRemainingTime() < 10.f)
			{
				MontageSubStepper.AddEvaluationTime(DeltaTime);
			}

			// Gather active anim state notifies if DeltaTime == 0 (happens when TimeDilation is 0.f), so these are not prematurely ended
   // 收集活动动画状态会通知 DeltaTime == 0（当 TimeDilation 为 0.f 时发生），因此这些不会提前结束
			if (DeltaTime == 0.f)
			{
				HandleEvents(Position, Position, nullptr);
			}

			while (bPlaying && MontageSubStepper.HasTimeRemaining() && (++NumIterations < MaxIterations))
			{
				SCOPE_CYCLE_COUNTER(STAT_AnimMontageInstance_Advance_Iteration);

				const float PreviousSubStepPosition = Position;
				const FBranchingPointMarker* BranchingPointMarker = nullptr;
				EMontageSubStepResult SubStepResult = MontageSubStepper.Advance(Position, &BranchingPointMarker);

				if (SubStepResult == EMontageSubStepResult::InvalidSection
					|| SubStepResult == EMontageSubStepResult::InvalidMontage)
				{
					// stop and leave this loop
     // 停止并离开这个循环
					Stop(FAlphaBlend(Montage->BlendOut, Montage->BlendOut.GetBlendTime() * DefaultBlendTimeMultiplier), false);
					break;
				}

				const float SubStepDeltaMove = MontageSubStepper.GetDeltaMove();
				DeltaTimeRecord.Delta += SubStepDeltaMove;
				const bool bPlayingForward = MontageSubStepper.GetbPlayingForward();

				// If current section is last one, check to trigger a blend out and if it hasn't stopped yet, see if we should stop
    // 如果当前部分是最后一个，请检查是否触发混合，如果尚未停止，看看我们是否应该停止
				// We check this even if we haven't moved, in case our position was different from last frame.
    // 即使我们没有移动，我们也会检查这一点，以防我们的位置与上一帧不同。
				// (Code triggered a position jump).
    // （代码触发了位置跳转）。
				if (!IsStopped() && bEnableAutoBlendOut)
				{
					const int32 CurrentSectionIndex = MontageSubStepper.GetCurrentSectionIndex();
					check(NextSections.IsValidIndex(CurrentSectionIndex));
					const int32 NextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];
					if (NextSectionIndex == INDEX_NONE)
					{
						const float PlayTimeToEnd = MontageSubStepper.GetRemainingPlayTimeToSectionEnd(Position);

						const bool bCustomBlendOutTriggerTime = (Montage->BlendOutTriggerTime >= 0);
						const float DefaultBlendOutTime = Montage->BlendOut.GetBlendTime() * DefaultBlendTimeMultiplier;
						const float BlendOutTriggerTime = bCustomBlendOutTriggerTime ? Montage->BlendOutTriggerTime : DefaultBlendOutTime;

						// ... trigger blend out if within blend out time window.
      // ...如果在混合时间窗口内，则触发混合。
						if (PlayTimeToEnd <= FMath::Max<float>(BlendOutTriggerTime, UE_KINDA_SMALL_NUMBER))
						{
							const float BlendOutTime = bCustomBlendOutTriggerTime ? DefaultBlendOutTime : PlayTimeToEnd;
							Stop(FAlphaBlend(Montage->BlendOut, BlendOutTime), false);
						}
					}
				}

				const bool bHaveMoved = (SubStepResult == EMontageSubStepResult::Moved);
				if (bHaveMoved)
				{
					if (bDidUseMarkerSyncThisTick)
					{
						Montage->MarkerData.CollectMarkersInRange(PreviousSubStepPosition, Position, MarkersPassedThisTick, SubStepDeltaMove);
					}

					// Extract Root Motion for this time slice, and accumulate it.
     // 提取该时间片的根运动并累加。
					// IsRootMotionDisabled() can be changed by AnimNotifyState BranchingPoints while advancing, so it needs to be checked here.
     // IsRootMotionDisabled()在前进时可以被AnimNotifyState BranchingPoints改变，所以需要在这里检查。
					if (bExtractRootMotion && AnimInstance.IsValid() && !IsRootMotionDisabled())
					{
						const FTransform RootMotion = Montage->ExtractRootMotionFromTrackRange(PreviousSubStepPosition, Position, FAnimExtractContext());
						if (bBlendRootMotion)
						{
							// Defer blending in our root motion until after we get our slot weight updated
       // 推迟根运动的混合，直到我们更新插槽权重之后
							const float Weight = Blend.GetBlendedValue();
							AnimInstance.Get()->QueueRootMotionBlend(RootMotion, Montage->SlotAnimTracks[0].SlotName, Weight);
						}
						else
						{
							OutRootMotionParams->Accumulate(RootMotion);
						}

						UE_LOG(LogRootMotion, Log, TEXT("\tFAnimMontageInstance::Advance ExtractedRootMotion: %s, AccumulatedRootMotion: %s, bBlendRootMotion: %d")
							, *RootMotion.GetTranslation().ToCompactString()
							, *OutRootMotionParams->GetRootMotionTransform().GetTranslation().ToCompactString()
							, bBlendRootMotion
						);
					}
				}

				// Delegate has to be called last in this loop
    // 必须在此循环中最后调用委托
				// so that if this changes position, the new position will be applied in the next loop
    // 这样，如果位置发生变化，新位置将应用于下一个循环
				// first need to have event handler to handle it
    // 首先需要有事件处理程序来处理它
				// Save off position before triggering events, in case they cause a jump to another position
    // 在触发事件之前保存关闭位置，以防它们导致跳转到另一个位置
				const float PositionBeforeFiringEvents = Position;

				if(bHaveMoved)
				{
					// Save position before firing events.
     // 在触发事件之前保存位置。
					if (!bInterrupted)
					{
						// Must grab a reference on the stack in case "this" is deleted during iteration
      // 必须获取堆栈上的引用，以防“this”在迭代过程中被删除
						TWeakObjectPtr<UAnimInstance> AnimInstanceLocal = AnimInstance;

						HandleEvents(PreviousSubStepPosition, Position, BranchingPointMarker);

						// Break out if we no longer have active montage instances. This may happen when we call UninitializeAnimation from a notify
      // 如果我们不再有活动的蒙太奇实例，请中断。当我们从通知中调用 UninitializeAnimation 时可能会发生这种情况
						if (AnimInstanceLocal.IsValid() && AnimInstanceLocal->MontageInstances.Num() == 0)
						{
							return;
						}
					}
				}

				// Note that we have to check this even if there is no time remaining, in order to correctly handle loops
    // 请注意，即使没有剩余时间，我们也必须检查这一点，以便正确处理循环
				// CVar allows reverting to old behavior, in case a project relies on it
    // CVar 允许恢复到旧的行为，以防项目依赖它
				if (MontageCVars::bEndSectionRequiresTimeRemaining == false || MontageSubStepper.HasTimeRemaining())
				{
					// if we reached end of section, and we were not processing a branching point, and no events has messed with out current position..
     // 如果我们到达部分末尾，并且我们没有处理分支点，并且没有事件扰乱当前位置..
					// .. Move to next section.
     // .. 移至下一节。
					// (this also handles looping, the same as jumping to a different section).
     // （这也处理循环，与跳转到不同的部分相同）。
					if (MontageSubStepper.HasReachedEndOfSection() && !BranchingPointMarker && (PositionBeforeFiringEvents == Position))
					{
						// Get recent NextSectionIndex in case it's been changed by previous events.
      // 获取最近的 NextSectionIndex，以防它被之前的事件更改。
						const int32 CurrentSectionIndex = MontageSubStepper.GetCurrentSectionIndex();
						const int32 RecentNextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];
						const float EndOffset = UE_KINDA_SMALL_NUMBER / 2.f; //KINDA_SMALL_NUMBER/2 because we use KINDA_SMALL_NUMBER to offset notifies for triggering and SMALL_NUMBER is too small

						if (RecentNextSectionIndex != INDEX_NONE)
						{
							float LatestNextSectionStartTime, LatestNextSectionEndTime;
							Montage->GetSectionStartAndEndTime(RecentNextSectionIndex, LatestNextSectionStartTime, LatestNextSectionEndTime);

							// Jump to next section's appropriate starting point (start or end).
       // 跳转到下一部分的适当起点（开始或结束）。
							Position = bPlayingForward ? LatestNextSectionStartTime : (LatestNextSectionEndTime - EndOffset);
							SubStepResult = EMontageSubStepResult::Moved;
							
							const bool bLooped = CurrentSectionIndex == RecentNextSectionIndex;
							AnimInstance->QueueMontageSectionChangedEvent(FQueuedMontageSectionChangedEvent(Montage, InstanceID, Montage->GetSectionName(RecentNextSectionIndex), bLooped, OnMontageSectionChanged));
						}
						else
						{
							// If there is no next section and we've reached the end of this one, exit
       // 如果没有下一节并且我们已到达本节的末尾，请退出

							// Stop playing and clamp position to prevent playing animation data past the end of the current section
       // 停止播放并锁定位置以防止播放动画数据超过当前部分的末尾
							// We already called Stop above if needed, like if bEnableAutoBlendOut is true
       // 如果需要，我们已经在上面调用了 Stop，例如 bEnableAutoBlendOut 为 true
							bPlaying = false;

							float CurrentSectionStartTime, CurrentSectionEndTime;
							Montage->GetSectionStartAndEndTime(CurrentSectionIndex, CurrentSectionStartTime, CurrentSectionEndTime);

							Position = bPlayingForward ? (CurrentSectionEndTime - EndOffset) : CurrentSectionStartTime;
							SubStepResult = EMontageSubStepResult::Moved;

							break;
						}
					}
				}

				if (SubStepResult == EMontageSubStepResult::NotMoved)
				{
					// If it hasn't moved, there is nothing much to do but weight update
     // 如果它没有移动，除了权重更新之外没有什么可做的
					break;
				}
			}
		
			// if we had a ForcedNextPosition set, reset it.
   // 如果我们设置了 ForcedNextPosition，请重置它。
			ForcedNextToPosition.Reset();
			ForcedNextFromPosition.Reset();
		}
	}

#if ANIM_TRACE_ENABLED
	for(const FPassedMarker& PassedMarker : MarkersPassedThisTick)
	{
		TRACE_ANIM_SYNC_MARKER(AnimInstance.Get(), PassedMarker);
	}
#endif

	// If this Montage has no weight, it should be terminated.
 // 如果这个蒙太奇没有重量，它应该被终止。
	if (IsStopped() && (Blend.IsComplete()))
	{
		// nothing else to do
  // 没有别的事可做
		Terminate();
		return;
	}

	if (!bInterrupted && AnimInstance.IsValid())
	{
		SCOPE_CYCLE_COUNTER(STAT_AnimMontageInstance_TickBranchPoints);

		// Must grab a reference on the stack in case "this" is deleted during iteration
  // 必须获取堆栈上的引用，以防“this”在迭代过程中被删除
		TWeakObjectPtr<UAnimInstance> AnimInstanceLocal = AnimInstance;

		// Tick all active state branching points
  // 勾选所有活动状态分支点
		for (int32 Index = 0; Index < ActiveStateBranchingPoints.Num(); Index++)
		{
			FAnimNotifyEvent& NotifyEvent = ActiveStateBranchingPoints[Index];
			if (NotifyEvent.NotifyStateClass)
			{
				FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, &NotifyEvent, InstanceID);
				NotifyEvent.NotifyStateClass->BranchingPointNotifyTick(BranchingPointNotifyPayload, DeltaTime);

				// Break out if we no longer have active montage instances. This may happen when we call UninitializeAnimation from a notify
    // 如果我们不再有活动的蒙太奇实例，请中断。当我们从通知中调用 UninitializeAnimation 时可能会发生这种情况
				if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal, NotifyEvent.NotifyStateClass))
				{
					return;
				}
			}
		}
	}
}

void FAnimMontageInstance::HandleEvents(float PreviousTrackPos, float CurrentTrackPos, const FBranchingPointMarker* BranchingPointMarker)
{
	SCOPE_CYCLE_COUNTER(STAT_AnimMontageInstance_HandleEvents);

	// Skip notifies and branching points if montage has been interrupted.
 // 如果蒙太奇被中断，则跳过通知和分支点。
	if (bInterrupted)
	{
		return;
	}

	// Now get active Notifies based on how it advanced
 // 现在根据进展情况激活通知
	if (AnimInstance.IsValid())
	{
		FAnimTickRecord TickRecord;

		// Used to ensure all gathered notifies know the current montage time at the point they were queued.
  // 用于确保所有收集的通知都知道它们排队时的当前剪辑时间。
		TickRecord.TimeAccumulator = &CurrentTrackPos;
		
		// Add instance ID to context to differentiate notifies between different instances of the same montage
  // 将实例 ID 添加到上下文以区分同一蒙太奇的不同实例之间的通知
		TickRecord.MakeContextData<UE::Anim::FAnimNotifyMontageInstanceContext>(InstanceID);

		FAnimNotifyContext NotifyContext(TickRecord);

		// Queue all notifies fired from the AnimMontage's Notify Track.
  // 对从 AnimMontage 的通知轨道发出的所有通知进行排队。
		{
			// We already break up AnimMontage update to handle looping, so we guarantee that PreviousPos and CurrentPos are contiguous.
   // 我们已经分解了 AnimMontage 更新来处理循环，因此我们保证 PreviousPos 和 CurrentPos 是连续的。
			Montage->GetAnimNotifiesFromDeltaPositions(PreviousTrackPos, CurrentTrackPos, NotifyContext);

			// For Montage only, remove notifies marked as 'branching points'. They are not queued and are handled separately.
   // 仅对于蒙太奇，删除标记为“分支点”的通知。它们不排队并单独处理。
			Montage->FilterOutNotifyBranchingPoints(NotifyContext.ActiveNotifies);

			// Queue active non-'branching point' notifies.
   // 队列活动非“分支点”通知。
			AnimInstance->NotifyQueue.AddAnimNotifies(NotifyContext.ActiveNotifies, NotifyWeight);
		}

		// Queue all notifies fired by all the animations within the AnimMontage. We'll do this for all slot tracks.
  // 将 AnimMontage 中所有动画触发的所有通知排队。我们将对所有老虎机轨道执行此操作。
		{
			TMap<FName, TArray<FAnimNotifyEventReference>> NotifyMap;
			
			for (auto SlotTrack = Montage->SlotAnimTracks.CreateIterator(); SlotTrack; ++SlotTrack)
			{
				TArray<FAnimNotifyEventReference>& CurrentSlotNotifies = NotifyMap.FindOrAdd(SlotTrack->SlotName);

				// Queue active notifies from current slot.
    // 来自当前槽的活动通知队列。
				{
					NotifyContext.ActiveNotifies.Reset();
					SlotTrack->AnimTrack.GetAnimNotifiesFromTrackPositions(PreviousTrackPos, CurrentTrackPos, NotifyContext);
					Swap(CurrentSlotNotifies, NotifyContext.ActiveNotifies);
				}
			}

			// Queue active unfiltered notifies from slot tracks.
   // 将来自插槽轨道的活动未过滤通知排队。
			AnimInstance->NotifyQueue.AddAnimNotifies(NotifyMap, NotifyWeight);	
		}
	}

	// Update active state branching points, before we handle the immediate tick marker.
 // 在我们处理即时刻度标记之前，更新活动状态分支点。
	// In case our position jumped on the timeline, we need to begin/end state branching points accordingly.
 // 如果我们的位置在时间线上跳跃，我们需要相应地开始/结束状态分支点。
	// If this fails, this montage instance is no longer valid. Return to avoid crash.
 // 如果失败，则该蒙太奇实例不再有效。返回以避免崩溃。
	if (!UpdateActiveStateBranchingPoints(CurrentTrackPos))
	{
		return;
	}

	// Trigger ImmediateTickMarker event if we have one
 // 如果有一个，则触发 ImmediateTickMarker 事件
	if (BranchingPointMarker)
	{
		BranchingPointEventHandler(BranchingPointMarker);
	}
}

bool FAnimMontageInstance::UpdateActiveStateBranchingPoints(float CurrentTrackPosition)
{
	int32 NumStateBranchingPoints = Montage->BranchingPointStateNotifyIndices.Num();

	if (AnimInstance.IsValid() && NumStateBranchingPoints > 0)
	{
		// Must grab a reference on the stack in case "this" is deleted during iteration
  // 必须获取堆栈上的引用，以防“this”在迭代过程中被删除
		TWeakObjectPtr<UAnimInstance> AnimInstanceLocal = AnimInstance;

		// End no longer active events first. We want this to happen before we trigger NotifyBegin on newly active events.
  // 首先结束不再活动的事件。我们希望在对新活动事件触发 NotifyBegin 之前发生这种情况。
		for (int32 Index = ActiveStateBranchingPoints.Num() - 1; Index >= 0; Index--)
		{
			FAnimNotifyEvent& NotifyEvent = ActiveStateBranchingPoints[Index];

			if (NotifyEvent.NotifyStateClass)
			{
				const float NotifyStartTime = NotifyEvent.GetTriggerTime();
				const float NotifyEndTime = NotifyEvent.GetEndTriggerTime();
				bool bNotifyIsActive = (CurrentTrackPosition > NotifyStartTime) && (CurrentTrackPosition <= NotifyEndTime);

				if (!bNotifyIsActive)
				{
					FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, &NotifyEvent, InstanceID, true);
					TRACE_ANIM_NOTIFY(AnimInstance.Get(), NotifyEvent, End);
					NotifyEvent.NotifyStateClass->BranchingPointNotifyEnd(BranchingPointNotifyPayload);

					// Break out if we no longer have active montage instances. This may happen when we call UninitializeAnimation from a notify
     // 如果我们不再有活动的蒙太奇实例，请中断。当我们从通知中调用 UninitializeAnimation 时可能会发生这种情况
					if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal, NotifyEvent.NotifyStateClass))
					{
						return false;
					}

					ActiveStateBranchingPoints.RemoveAt(Index, 1);
				}
			}
		}

		// Then, begin newly active notifies
  // 然后，开始新的活动通知
		for (int32 Index = 0; Index < NumStateBranchingPoints; Index++)
		{
			const int32 NotifyIndex = Montage->BranchingPointStateNotifyIndices[Index];
			FAnimNotifyEvent& NotifyEvent = Montage->Notifies[NotifyIndex];

			if (NotifyEvent.NotifyStateClass)
			{
				const float NotifyStartTime = NotifyEvent.GetTriggerTime();
				const float NotifyEndTime = NotifyEvent.GetEndTriggerTime();

				bool bNotifyIsActive = (CurrentTrackPosition > NotifyStartTime) && (CurrentTrackPosition <= NotifyEndTime);
				if (bNotifyIsActive && !ActiveStateBranchingPoints.Contains(NotifyEvent))
				{
					FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, &NotifyEvent, InstanceID);
					TRACE_ANIM_NOTIFY(AnimInstance.Get(), NotifyEvent, Begin);
					NotifyEvent.NotifyStateClass->BranchingPointNotifyBegin(BranchingPointNotifyPayload);

					// Break out if we no longer have active montage instances. This may happen when we call UninitializeAnimation from a notify
     // 如果我们不再有活动的蒙太奇实例，请中断。当我们从通知中调用 UninitializeAnimation 时可能会发生这种情况
					if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal, NotifyEvent.NotifyStateClass))
					{
						return false;
					}

					ActiveStateBranchingPoints.Add(NotifyEvent);
				}
			}
		}
	}

	return true;
}

void FAnimMontageInstance::BranchingPointEventHandler(const FBranchingPointMarker* BranchingPointMarker)
{
	if (AnimInstance.IsValid() && Montage && BranchingPointMarker)
	{
		// Must grab a reference on the stack in case "this" is deleted during iteration
  // 必须获取堆栈上的引用，以防“this”在迭代过程中被删除
		TWeakObjectPtr<UAnimInstance> AnimInstanceLocal = AnimInstance;

		FAnimNotifyEvent* NotifyEvent = (BranchingPointMarker->NotifyIndex < Montage->Notifies.Num()) ? &Montage->Notifies[BranchingPointMarker->NotifyIndex] : nullptr;
		if (NotifyEvent)
		{
			// Handle backwards compatibility with older BranchingPoints.
   // 处理与旧分支点的向后兼容性。
			if (NotifyEvent->bConvertedFromBranchingPoint && (NotifyEvent->NotifyName != NAME_None))
			{
				FString FuncName = FString::Printf(TEXT("MontageBranchingPoint_%s"), *NotifyEvent->NotifyName.ToString());
				FName FuncFName = FName(*FuncName);

				UFunction* Function = AnimInstance.Get()->FindFunction(FuncFName);
				if (Function)
				{
					AnimInstance.Get()->ProcessEvent(Function, nullptr);
				}
				// In case older BranchingPoint has been re-implemented as a new Custom Notify, this is if BranchingPoint function hasn't been found.
    // 如果旧的 BranchingPoint 已被重新实现为新的自定义通知，则表示尚未找到 BranchingPoint 函数。
				else
				{
					AnimInstance.Get()->TriggerSingleAnimNotify(NotifyEvent);
				}
			}
			else if (NotifyEvent->NotifyStateClass != nullptr)
			{
				if (BranchingPointMarker->NotifyEventType == EAnimNotifyEventType::Begin)
				{
					FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, NotifyEvent, InstanceID);
					TRACE_ANIM_NOTIFY(AnimInstance.Get(), *NotifyEvent, Begin);
					NotifyEvent->NotifyStateClass->BranchingPointNotifyBegin(BranchingPointNotifyPayload);

					if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal, NotifyEvent->NotifyStateClass))
					{
						return;
					}

					ActiveStateBranchingPoints.Add(*NotifyEvent);
				}
				else
				{
					FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, NotifyEvent, InstanceID, true);
					TRACE_ANIM_NOTIFY(AnimInstance.Get(), *NotifyEvent, End);
					NotifyEvent->NotifyStateClass->BranchingPointNotifyEnd(BranchingPointNotifyPayload);

					if (!ValidateInstanceAfterNotifyState(AnimInstanceLocal, NotifyEvent->NotifyStateClass))
					{
						return;
					}

					ActiveStateBranchingPoints.RemoveSingleSwap(*NotifyEvent);
				}
			}
			// Non state notify with a native notify class
   // 使用本机通知类进行非状态通知
			else if	(NotifyEvent->Notify != nullptr)
			{
				// Implemented notify: just call Notify. UAnimNotify will forward this to the event which will do the work.
    // 实现通知：只需调用Notify即可。 UAnimNotify 会将其转发给执行该工作的事件。
				FBranchingPointNotifyPayload BranchingPointNotifyPayload(AnimInstance->GetSkelMeshComponent(), Montage, NotifyEvent, InstanceID);
				TRACE_ANIM_NOTIFY(AnimInstance.Get(), *NotifyEvent, Event);
				NotifyEvent->Notify->BranchingPointNotify(BranchingPointNotifyPayload);
			}
			// Try to match a notify function by name.
   // 尝试按名称匹配通知函数。
			else
			{
				AnimInstance.Get()->TriggerSingleAnimNotify(NotifyEvent);
			}
		}
	}
}

UAnimMontage* FAnimMontageInstance::PreviewSequencerMontagePosition(FName SlotName, USkeletalMeshComponent* SkeletalMeshComponent, int32& InOutInstanceId, UAnimSequenceBase* InAnimSequence, float InFromPosition, float InToPosition, float Weight, bool bLooping, bool bFireNotifies, bool bPlaying)
{
	if (SkeletalMeshComponent)
	{
		return PreviewSequencerMontagePosition(SlotName, SkeletalMeshComponent, SkeletalMeshComponent->GetAnimInstance(), InOutInstanceId, InAnimSequence, InFromPosition, InToPosition, Weight, bLooping, bFireNotifies, bPlaying);
	}

	return nullptr;
}

UAnimMontage* FAnimMontageInstance::SetSequencerMontagePosition(FName SlotName, USkeletalMeshComponent* SkeletalMeshComponent, int32& InOutInstanceId, UAnimSequenceBase* InAnimSequence, float InFromPosition, float InToPosition, float Weight, bool bLooping, bool bPlaying)
{
	if (SkeletalMeshComponent)
	{
		return SetSequencerMontagePosition(SlotName, SkeletalMeshComponent->GetAnimInstance(), InOutInstanceId, InAnimSequence, InFromPosition, InToPosition, Weight, bLooping, bPlaying);
	}

	return nullptr;
}

UAnimMontage* FAnimMontageInstance::SetSequencerMontagePosition(FName SlotName, UAnimInstance* AnimInstance, int32& InOutInstanceId, UAnimSequenceBase* InAnimSequence, float InFromPosition, float InToPosition, float Weight, bool bLooping, bool bInPlaying)
{
	UAnimInstance* AnimInst = AnimInstance;
	if (AnimInst)
	{
		UAnimMontage* PlayingMontage = nullptr;
		FAnimMontageInstance* MontageInstanceToUpdate = AnimInst->GetMontageInstanceForID(InOutInstanceId);

		if (!MontageInstanceToUpdate)
		{
			PlayingMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(InAnimSequence, SlotName, 0.0f, 0.0f, 0.f, 1);
			if (PlayingMontage)
			{
				AnimInst->Montage_Play(PlayingMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false);
				MontageInstanceToUpdate = AnimInst->GetActiveInstanceForMontage(PlayingMontage);
				// this is sequencer set up, we disable auto blend out
    // 这是音序器设置，我们禁用自动混合
				if (MontageInstanceToUpdate)
				{
					MontageInstanceToUpdate->bEnableAutoBlendOut = false;
				}
			}
		}

		if (MontageInstanceToUpdate)
		{
			PlayingMontage = MontageInstanceToUpdate->Montage;
			InOutInstanceId = MontageInstanceToUpdate->GetInstanceID();

			// ensure full weighting to this instance
   // 确保对该实例进行充分加权
			MontageInstanceToUpdate->Blend.SetDesiredValue(Weight);
			MontageInstanceToUpdate->Blend.SetAlpha(Weight);
			MontageInstanceToUpdate->BlendStartAlpha = MontageInstanceToUpdate->Blend.GetAlpha();
			
			if (bInPlaying)
			{
				MontageInstanceToUpdate->SetNextPositionWithEvents(InFromPosition, InToPosition);
			}
			else
			{
				MontageInstanceToUpdate->SetPosition(InToPosition);
			}

			MontageInstanceToUpdate->bPlaying = bInPlaying;
			return PlayingMontage;
		}
	}
	else
	{
		UE_LOG(LogSkeletalMesh, Warning, TEXT("Invalid animation configuration when attempting to set animation possition with : %s"), *InAnimSequence->GetName());
	}

	return nullptr;
}

UAnimMontage* FAnimMontageInstance::PreviewSequencerMontagePosition(FName SlotName, USkeletalMeshComponent* SkeletalMeshComponent, UAnimInstance* AnimInstance, int32& InOutInstanceId, UAnimSequenceBase* InAnimSequence, float InFromPosition, float InToPosition, float Weight, bool bLooping, bool bFireNotifies, bool bInPlaying)
{
	UAnimInstance* AnimInst = AnimInstance;
	if (AnimInst)
	{
		FAnimMontageInstance* MontageInstanceToUpdate = AnimInst->GetMontageInstanceForID(InOutInstanceId);

		UAnimMontage* PlayingMontage = SetSequencerMontagePosition(SlotName, AnimInst, InOutInstanceId, InAnimSequence, InFromPosition, InToPosition, Weight, bLooping, bInPlaying);
		if (PlayingMontage)
		{
			// we have to get it again in case if this is new
   // 如果这是新的，我们必须再次获取它
			MontageInstanceToUpdate = AnimInst->GetMontageInstanceForID(InOutInstanceId);
			// since we don't advance montage in the tick, we manually have to handle notifies
   // 由于我们不在勾选中提前蒙太奇，因此我们必须手动处理通知
			MontageInstanceToUpdate->HandleEvents(InFromPosition, InToPosition, nullptr);
			if (!bFireNotifies)
			{
				AnimInst->NotifyQueue.Reset(SkeletalMeshComponent);
			}

			return PlayingMontage;
		}
	}

	return nullptr;
}

UAnimMontage* UAnimMontage::CreateSlotAnimationAsDynamicMontage(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime, float BlendOutTime, float InPlayRate, int32 LoopCount, float BlendOutTriggerTime, float InTimeToStartMontageAt)
{
	FMontageBlendSettings BlendInSettings(BlendInTime);
	FMontageBlendSettings BlendOutSettings(BlendOutTime);

	// InTimeToStartMontageAt is an unused argument. Keeping it to avoid changing public api.
 // InTimeToStartMontageAt 是一个未使用的参数。保留它以避免更改公共 api。
	return CreateSlotAnimationAsDynamicMontage_WithBlendSettings(Asset, SlotNodeName, BlendInSettings, BlendOutSettings, InPlayRate, LoopCount, BlendOutTriggerTime);
}

UAnimMontage* UAnimMontage::CreateSlotAnimationAsDynamicMontage_WithFractionalLoops(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime, float BlendOutTime, float LoopCount, float BlendOutTriggerTime)
{
	if (!ensure(LoopCount > 0.0))
	{
		return nullptr;
	}

	UAnimMontage* BaseMontage = CreateSlotAnimationAsDynamicMontage(Asset, SlotNodeName, BlendInTime, BlendOutTime, 1.0f, 1, BlendOutTriggerTime);
	if (BaseMontage == nullptr)
	{
		return nullptr;
	}

	if (LoopCount == 1.0)
	{
		return BaseMontage;
	}

	FSlotAnimationTrack& SlotTrack = BaseMontage->SlotAnimTracks[0];
	FAnimSegment& OriginalSegment = SlotTrack.AnimTrack.AnimSegments[0];

	FCompositeSection& Section = BaseMontage->CompositeSections[0];
	Section.Link(Asset, 0.0);

	if (LoopCount < 1.0)
	{
		// Need to shorten the existing segment
  // 需要缩短现有段
		OriginalSegment.AnimEndTime = Asset->GetPlayLength() * LoopCount;
	}
	else
	{
		int32 NumFullLoops = FMath::IsFinite(LoopCount) ? static_cast<int32>(LoopCount) : MAX_int32;
		float FractionalLoop = LoopCount - NumFullLoops;

		// Need to lengthen the existing segment
  // 需要延长现有段
		OriginalSegment.LoopingCount = NumFullLoops;

		// Need to add a new fractional segment onto the end of the montage
  // 需要在蒙太奇的末尾添加一个新的分数段
		FAnimSegment& NewSegment = SlotTrack.AnimTrack.AnimSegments.AddDefaulted_GetRef();
		NewSegment.SetAnimReference(Asset, true);

		NewSegment.StartPos = Asset->GetPlayLength() * NumFullLoops;
		NewSegment.AnimEndTime = Asset->GetPlayLength() * FractionalLoop;
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	BaseMontage->SequenceLength = Asset->GetPlayLength() * LoopCount;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	return BaseMontage;
}

UAnimMontage* UAnimMontage::CreateSlotAnimationAsDynamicMontage_WithBlendSettings(UAnimSequenceBase* Asset, FName SlotNodeName, const FMontageBlendSettings& BlendInSettings, const FMontageBlendSettings& BlendOutSettings, float InPlayRate, int32 LoopCount, float InBlendOutTriggerTime)
{
	// create temporary montage and play
 // 创建临时蒙太奇并播放
	bool bValidAsset = Asset && !Asset->IsA(UAnimMontage::StaticClass());
	if (!bValidAsset)
	{
		// user warning
  // 用户警告
		UE_LOG(LogAnimMontage, Warning, TEXT("PlaySlotAnimationAsDynamicMontage: Invalid input asset(%s). If Montage, please use Montage_Play"), *GetNameSafe(Asset));
		return nullptr;
	}

	if (SlotNodeName == NAME_None)
	{
		// user warning
  // 用户警告
		UE_LOG(LogAnimMontage, Warning, TEXT("SlotNode Name is required. Make sure to add Slot Node in your anim graph and name it."));
		return nullptr;
	}

	USkeleton* AssetSkeleton = Asset->GetSkeleton();
	if (!Asset->CanBeUsedInComposition())
	{
		UE_LOG(LogAnimMontage, Warning, TEXT("This animation isn't supported to play as montage"));
		return nullptr;
	}

	// now play
 // 现在玩
	UAnimMontage* NewMontage = NewObject<UAnimMontage>();
	NewMontage->SetSkeleton(AssetSkeleton);

	// add new track
 // 添加新曲目
	FSlotAnimationTrack& NewTrack = NewMontage->SlotAnimTracks[0];
	NewTrack.SlotName = SlotNodeName;
	FAnimSegment NewSegment;
	NewSegment.SetAnimReference(Asset, true);
	NewSegment.LoopingCount = LoopCount;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
    NewMontage->SequenceLength = NewSegment.GetLength();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	NewTrack.AnimTrack.AnimSegments.Add(NewSegment);

	FCompositeSection NewSection;
	NewSection.SectionName = TEXT("Default");
	NewSection.Link(Asset, Asset->GetPlayLength());
	NewSection.SetTime(0.0f);

	// add new section
 // 添加新部分
	NewMontage->CompositeSections.Add(NewSection);

	NewMontage->BlendIn = FAlphaBlend(BlendInSettings.Blend);
	NewMontage->BlendModeIn = BlendInSettings.BlendMode;
	NewMontage->BlendProfileIn = BlendInSettings.BlendProfile;

	NewMontage->BlendOut = FAlphaBlend(BlendOutSettings.Blend);
	NewMontage->BlendModeOut = BlendOutSettings.BlendMode;
	NewMontage->BlendProfileOut = BlendOutSettings.BlendProfile;

	NewMontage->BlendOutTriggerTime = InBlendOutTriggerTime;

	NewMontage->CommonTargetFrameRate = Asset->GetSamplingFrameRate();
	
	return NewMontage;
}

bool FAnimMontageInstance::CanUseMarkerSync() const
{
	// for now we only allow non-full weight and when blending out
 // 目前我们只允许非满重量并且混合时
	return SyncGroupName != NAME_None && IsStopped() && Blend.IsComplete() == false;
}

#if WITH_EDITOR
void UAnimMontage::BakeTimeStretchCurve()
{
	TimeStretchCurve.Reset();

	// See if Montage is hosting a curve named 'TimeStretchCurveName'
 // 查看 Montage 是否托管名为“TimeStretchCurveName”的曲线
	const FFloatCurve* TimeStretchFloatCurve = nullptr;
	if (ShouldDataModelBeValid())
	{		
		TimeStretchFloatCurve = GetDataModel()->FindFloatCurve(FAnimationCurveIdentifier(TimeStretchCurveName, ERawCurveTrackTypes::RCT_Float));
	}

	if (TimeStretchFloatCurve == nullptr)
	{
		return;
	}
	
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TimeStretchCurve.BakeFromFloatCurve(*TimeStretchFloatCurve, SequenceLength);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UAnimMontage::PopulateWithExistingModel(TScriptInterface<IAnimationDataModel> ExistingDataModel)
{
	Super::PopulateWithExistingModel(ExistingDataModel);
	
	// Set composite length while model is being populated
 // 填充模型时设置复合长度
	const float CurrentCalculatedLength = CalculateSequenceLength();
	SetCompositeLength(CurrentCalculatedLength);
}

void UAnimMontage::UpdateCommonTargetFrameRate()
{
	CommonTargetFrameRate = FFrameRate(0,0);
	FFrameRate TargetRate = UAnimationSettings::Get()->GetDefaultFrameRate();

	bool bValidFrameRate = true;
	bool bFirst = true;
	for (const FSlotAnimationTrack& Track : SlotAnimTracks)
	{
		for (const FAnimSegment& Segment : Track.AnimTrack.AnimSegments)
		{
			const UAnimSequenceBase* Base = Segment.GetAnimReference();
			if (Base && Base != this)
			{
				const FFrameRate BaseFrameRate = Base->GetSamplingFrameRate();
				if (bFirst)
				{
					TargetRate = BaseFrameRate;
					bFirst = false;
				}
				else
				{
					if (BaseFrameRate.IsValid())
					{
						if (TargetRate.IsMultipleOf(BaseFrameRate))
						{
							TargetRate = BaseFrameRate;
						}
						else if (TargetRate != BaseFrameRate && !BaseFrameRate.IsMultipleOf(TargetRate))
						{
							FString AssetString;
							TArray<UAnimationAsset*> Assets;
							if(GetAllAnimationSequencesReferred(Assets, false))
							{
								for (const UAnimationAsset* AnimAsset : Assets)
								{
									if (const UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(AnimAsset))
									{
										AssetString.Append(FString::Printf(TEXT("\n\t%s - %s"), *AnimSequenceBase->GetName(), *AnimSequenceBase->GetSamplingFrameRate().ToPrettyText().ToString()));
									}
								}
							}						

							if (UE::Anim::CVarOutputMontageFrameRateWarning.GetValueOnAnyThread() == true)
							{
								UE_LOG(LogAnimation, Warning, TEXT("Frame rate of animation %s (%s) is incompatible with other animations in Animation Montage %s - underlying frame-rate will be set to %s:%s"), *Base->GetName(), *BaseFrameRate.ToPrettyText().ToString(), *GetName(), *Super::GetSamplingFrameRate().ToPrettyText().ToString(), *AssetString);
							}
						
							bValidFrameRate = false;
							break;
						}
					}
					else
					{
						UE_LOG(LogAnimMontage, Warning, TEXT("Invalid frame rate %s for %s in %s"), *BaseFrameRate.ToPrettyText().ToString(), *Base->GetName(), *GetName());
					}
				}			
			}	
		}			
	}

	if (bValidFrameRate)
	{
		CommonTargetFrameRate = TargetRate;
	}
}

#endif // WITH_EDITOR

FMontageBlendSettings::FMontageBlendSettings()
	: BlendProfile(nullptr)
	, BlendMode(EMontageBlendMode::Standard)
{}

FMontageBlendSettings::FMontageBlendSettings(float BlendTime)
	: BlendProfile(nullptr)
	, Blend(BlendTime)
	, BlendMode(EMontageBlendMode::Standard)
{}

FMontageBlendSettings::FMontageBlendSettings(const FAlphaBlendArgs& BlendArgs)
	: BlendProfile(nullptr)
	, Blend(BlendArgs)
	, BlendMode(EMontageBlendMode::Standard)
{}

