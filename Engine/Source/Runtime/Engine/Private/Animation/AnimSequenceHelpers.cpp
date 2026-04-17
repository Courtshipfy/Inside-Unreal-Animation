// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimSequenceHelpers.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"


#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimComposite.h"
#include "Animation/MirrorDataTable.h"
#include "BonePose.h"

#include "AnimationRuntime.h"
#include "AnimEncoding.h"
#include "AnimationUtils.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Misc/MessageDialog.h"
#include "Animation/AnimSequenceHelpers.h"
#include "Animation/SkeletonRemapping.h"
#include "Animation/SkeletonRemappingRegistry.h"
#include "Animation/AttributesRuntime.h"
#include "Animation/AnimCurveUtils.h"

#define LOCTEXT_NAMESPACE "AnimSequenceHelpers"

namespace UE
{

namespace Anim
{

#if WITH_EDITOR
void BuildPoseFromModel(const IAnimationDataModel* Model, FCompactPose& OutPose, const double Time, const EAnimInterpolationType& InterpolationType, const FName& RetargetSource, const TArray<FTransform>& RetargetTransforms)
{
	FBlendedCurve TempCurve;
	UE::Anim::FStackAttributeContainer TempAttributes;
	TempCurve.InitFrom(OutPose.GetBoneContainer());

	FAnimationPoseData PoseData(OutPose, TempCurve, TempAttributes);
	BuildPoseFromModel(Model, PoseData, Time, InterpolationType, RetargetSource, RetargetTransforms);
}
				
void BuildPoseFromModel(const IAnimationDataModel* Model, FAnimationPoseData& OutPoseData, const double Time, const EAnimInterpolationType& InterpolationType, const FName& RetargetSource, const TArray<FTransform>& RetargetTransforms)
{
	FCompactPose& OutPose = OutPoseData.GetPose();
	check(Model);

	OutPose.ResetToRefPose();

	const UE::Anim::DataModel::FEvaluationContext EvaluationContext(Time, Model->GetFrameRate(), RetargetSource, RetargetTransforms, InterpolationType);
	Model->Evaluate(OutPoseData, EvaluationContext);
}

void EvaluateFloatCurvesFromModel(const IAnimationDataModel* Model, FBlendedCurve& OutCurves, double Time)
{
	check(Model);

	// Evaluate into a temporary curve, then filter by enabled curves
	// 评估临时曲线，然后按启用的曲线进行过滤
	const TArray<FFloatCurve>& ModelCurves = Model->GetFloatCurves();
	const int32 NumCurves = ModelCurves.Num();

	auto GetNameFromIndex = [&ModelCurves](int32 InCurveIndex)
	{
		return ModelCurves[InCurveIndex].GetName();
	};

	auto GetValueFromIndex = [&ModelCurves, Time](int32 InCurveIndex)
	{
		return ModelCurves[InCurveIndex].Evaluate(Time);
	};

	UE::Anim::FCurveUtils::BuildUnsorted(OutCurves, NumCurves, GetNameFromIndex, GetValueFromIndex, OutCurves.GetFilter());
}

void EvaluateTransformCurvesFromModel(const IAnimationDataModel* Model, TMap<FName, FTransform>& OutCurves, double Time, float BlendWeight)
{
	if (Model)
	{
		for (const FTransformCurve& Curve : Model->GetTransformCurves())
		{
			// if disabled, do not handle
			// 如果禁用，则不处理
			if (Curve.GetCurveTypeFlag(AACF_Disabled))
			{
				continue;
			}

			// Add or retrieve curve
			// 添加或检索曲线
			// note we're not checking Curve.GetCurveTypeFlags() yet
			// 请注意，我们还没有检查 Curve.GetCurveTypeFlags()
			FTransform& Value = OutCurves.FindOrAdd(Curve.GetName());
			Value = Curve.Evaluate(Time, BlendWeight);
		}
	}
}

void GetBoneTransformFromModel(const IAnimationDataModel* Model, FTransform& OutTransform, int32 TrackIndex, double Time, const EAnimInterpolationType& Interpolation)
{
	TArray<FName> TrackNames;
	Model->GetBoneTrackNames(TrackNames);
	OutTransform = Model->EvaluateBoneTrackTransform(TrackNames[TrackIndex], Model->GetFrameRate().AsFrameTime(Time), Interpolation);
}

void GetBoneTransformFromModel(const IAnimationDataModel* Model, FTransform& OutTransform, int32 TrackIndex, int32 KeyIndex)
{
	TArray<FName> TrackNames;
	Model->GetBoneTrackNames(TrackNames);
	OutTransform = Model->GetBoneTrackTransform(TrackNames[TrackIndex], KeyIndex);

	for (const FTransformCurve& AdditiveTransformCurve : Model->GetTransformCurves())
	{
		if (AdditiveTransformCurve.GetName() == TrackNames[TrackIndex])
		{
			const float TimeInterval = Model->GetFrameRate().AsSeconds(KeyIndex);
			const FTransform AdditiveTransform = AdditiveTransformCurve.Evaluate(TimeInterval, 1.f);
			const FTransform LocalTransform = OutTransform;
			OutTransform.SetRotation(LocalTransform.GetRotation() * AdditiveTransform.GetRotation());
			OutTransform.SetTranslation(LocalTransform.TransformPosition(AdditiveTransform.GetTranslation()));
			OutTransform.SetScale3D(LocalTransform.GetScale3D() * AdditiveTransform.GetScale3D());			
			break;
		}
	}
}

void CopyCurveDataToModel(const FRawCurveTracks& CurveData, const USkeleton* Skeleton, IAnimationDataController& Controller)
{
	// Populate float curve data
	// 填充浮点曲线数据
	for (const FFloatCurve& FloatCurve : CurveData.FloatCurves)
	{
		const FAnimationCurveIdentifier CurveId = UAnimationCurveIdentifierExtensions::FindCurveIdentifier(Skeleton, FloatCurve.GetName(), ERawCurveTrackTypes::RCT_Float);
		if (CurveId.IsValid())
		{
			Controller.AddCurve(CurveId, FloatCurve.GetCurveTypeFlags());
			FCurveAttributes Attributes;
			Attributes.SetPreExtrapolation(FloatCurve.FloatCurve.PreInfinityExtrap);
			Attributes.SetPostExtrapolation(FloatCurve.FloatCurve.PostInfinityExtrap);					
			Controller.SetCurveColor(CurveId, FloatCurve.GetColor());
			Controller.SetCurveAttributes(CurveId, Attributes);
			Controller.SetCurveColor(CurveId, FloatCurve.GetColor());
			Controller.SetCurveKeys(CurveId, FloatCurve.FloatCurve.GetConstRefOfKeys());
		}
	}

	// Populate transform curve data
	// 填充变换曲线数据
	for (const FTransformCurve& TransformCurve : CurveData.TransformCurves)
	{
		const FAnimationCurveIdentifier CurveId = UAnimationCurveIdentifierExtensions::FindCurveIdentifier(Skeleton, TransformCurve.GetName(), ERawCurveTrackTypes::RCT_Transform);
		if (CurveId.IsValid())
		{
			Controller.AddCurve(CurveId, TransformCurve.GetCurveTypeFlags());

			// Set each individual channel rich curve keys, to account for any custom tangents etc.
			// 设置每个单独通道的丰富曲线键，以考虑任何自定义切线等。
			for (int32 SubCurveIndex = 0; SubCurveIndex < 3; ++SubCurveIndex)
			{
				const ETransformCurveChannel Channel = (ETransformCurveChannel)SubCurveIndex;
				const FVectorCurve* VectorCurve = TransformCurve.GetVectorCurveByIndex(SubCurveIndex);
				for (int32 ChannelIndex = 0; ChannelIndex < 3; ++ChannelIndex)
				{
					const EVectorCurveChannel Axis = (EVectorCurveChannel)ChannelIndex;
					FAnimationCurveIdentifier TargetCurveIdentifier = CurveId;
					UAnimationCurveIdentifierExtensions::GetTransformChildCurveIdentifier(TargetCurveIdentifier, Channel, Axis);
					Controller.SetCurveKeys(TargetCurveIdentifier, VectorCurve->FloatCurves[ChannelIndex].GetConstRefOfKeys());
				}
			}
		}
	}
}
#endif // WITH_EDITOR
 
 
 void ExtractBoneTransform(const FRawAnimSequenceTrack& RawTrack, FTransform& OutTransform, int32 KeyIndex)
{
	// Bail out (with rather wacky data) if data is empty for some reason.
	// 如果由于某种原因数据为空，则退出（使用相当古怪的数据）。
	if (RawTrack.PosKeys.Num() == 0 || RawTrack.RotKeys.Num() == 0)
	{
		UE_LOG(LogAnimation, Log, TEXT("GetBoneTransform : No anim data in track!"));
		OutTransform.SetIdentity();
		return;
	}

	const int32 PosKeyIndex = FMath::Min(KeyIndex, RawTrack.PosKeys.Num() - 1);
	const int32 RotKeyIndex = FMath::Min(KeyIndex, RawTrack.RotKeys.Num() - 1);
	static const FVector DefaultScale3D = FVector(1.f);

	OutTransform.SetTranslation(FVector(RawTrack.PosKeys[PosKeyIndex]));
	OutTransform.SetRotation(FQuat(RawTrack.RotKeys[RotKeyIndex]));
	if (RawTrack.ScaleKeys.Num() > 0)
	{
		const int32 ScaleKeyIndex = FMath::Min(KeyIndex, RawTrack.ScaleKeys.Num() - 1);
		OutTransform.SetScale3D(FVector(RawTrack.ScaleKeys[ScaleKeyIndex]));
	}
	else
	{
		OutTransform.SetScale3D(DefaultScale3D);
	}	
}

FTransform MirrorTransform(const FTransform& Transform, const UMirrorDataTable& MirrorDataTable)
{
	FVector T = Transform.GetTranslation();
	T = FAnimationRuntime::MirrorVector(T, MirrorDataTable.MirrorAxis);

	FQuat Q = Transform.GetRotation();
	Q = FAnimationRuntime::MirrorQuat(Q, MirrorDataTable.MirrorAxis);

	const FVector S = Transform.GetScale3D();
	return FTransform(Q, T, S);
}

FTransform ExtractRootMotionFromAnimationAsset(const UAnimationAsset* Animation, const UMirrorDataTable* MirrorDataTable, float StartTime, float DeltaTime, bool bAllowLooping)
{
	if (!Animation)
	{
		return FTransform::Identity;
	}

	// Note: Adapted from 'UAnimSequence::ExtractRootMotion' - since anim sequence doesn't know about MirrorDataTable
	// 注意：改编自 'UAnimSequence::ExtractRootMotion' - 因为动画序列不知道 MirrorDataTable
	FRootMotionMovementParams RootMotionParams;

	if (DeltaTime != 0.f)
	{
		bool const bPlayingBackwards = (DeltaTime < 0.f);

		float PreviousPosition = StartTime;
		float CurrentPosition = StartTime;
		float DesiredDeltaMove = DeltaTime;

		do
		{
			// Disable looping here. Advance to desired position, or beginning / end of animation 
			// 此处禁用循环。前进到所需位置，或动画的开始/结束
			const ETypeAdvanceAnim AdvanceType = FAnimationRuntime::AdvanceTime(false, DesiredDeltaMove, CurrentPosition, Animation->GetPlayLength());

			// Verify position assumptions
			// 验证位置假设
			ensureMsgf(bPlayingBackwards ? (CurrentPosition <= PreviousPosition) : (CurrentPosition >= PreviousPosition), TEXT("in Animation %s(Skeleton %s) : bPlayingBackwards(%d), PreviousPosition(%0.2f), Current Position(%0.2f)"),
				*Animation->GetName(), *GetNameSafe(Animation->GetSkeleton()), bPlayingBackwards, PreviousPosition, CurrentPosition);
			
			RootMotionParams.Accumulate(UE::Anim::ExtractRootMotionFromAnimationAsset(Animation, MirrorDataTable, PreviousPosition, CurrentPosition));

			// If we've hit the end of the animation, and we're allowed to loop, keep going.
			// 如果我们已经到达动画的结尾，并且允许循环，请继续。
			if ((AdvanceType == ETAA_Finished) && bAllowLooping)
			{
				const double ActualDeltaMove = (CurrentPosition - PreviousPosition);
				DesiredDeltaMove -= ActualDeltaMove;

				PreviousPosition = bPlayingBackwards ? Animation->GetPlayLength() : 0.f;
				CurrentPosition = PreviousPosition;
			}
			else
			{
				break;
			}
		} while (true);
	}

	return RootMotionParams.GetRootMotionTransform();
}

FTransform ExtractRootMotionFromAnimationAsset(const UAnimationAsset* Animation, const UMirrorDataTable* MirrorDataTable, float StartPosition, float EndPosition)
{
	FTransform Result = FTransform::Identity;
	
	if (const UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		Result = Montage->ExtractRootMotionFromTrackRange(StartPosition, EndPosition, FAnimExtractContext());
	}
	else if (const UAnimComposite* AnimComposite = Cast<UAnimComposite>(Animation))
	{
		FRootMotionMovementParams RootMotion;
		AnimComposite->ExtractRootMotionFromTrack(AnimComposite->AnimationTrack, StartPosition, EndPosition, FAnimExtractContext(), RootMotion);
		Result = RootMotion.GetRootMotionTransform();
	}
	else if (const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Animation))
	{
		Result = AnimSequence->ExtractRootMotionFromRange(StartPosition, EndPosition, FAnimExtractContext());
	}

	if (MirrorDataTable)
	{
		Result =  MirrorTransform(Result, *MirrorDataTable);
	}

	return Result;
}

FTransform ExtractRootTransformFromAnimationAsset(const UAnimationAsset* Animation, float Time)
{
	const FVector DefaultScale(1.f);
	bool bUseNormalizedRootMotionScale = false;

	FTransform RootTransformRefPose = FTransform::Identity;
	if (Animation)
	{
		if (const USkeleton* MySkeleton = Animation->GetSkeleton())
		{
			const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
			if (RefSkeleton.GetNum() > 0)
			{
				RootTransformRefPose = RefSkeleton.GetRefBonePose()[0];
			}
		}
	}

	FTransform Result = FTransform::Identity;
	if (const UAnimMontage* AnimMontage = Cast<UAnimMontage>(Animation))
	{
		if (const FAnimSegment* Segment = AnimMontage->SlotAnimTracks[0].AnimTrack.GetSegmentAtTime(Time))
		{
			if (const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Segment->GetAnimReference()))
			{
				const float AnimSequenceTime = Segment->ConvertTrackPosToAnimPos(Time);
				Result = AnimSequence->ExtractRootTrackTransform(FAnimExtractContext(static_cast<double>(AnimSequenceTime)), nullptr);
				bUseNormalizedRootMotionScale = AnimSequence->bUseNormalizedRootMotionScale;
			}	
		}
	}
	else if (const UAnimComposite* AnimComposite = Cast<UAnimComposite>(Animation))
	{
		Result = AnimComposite->ExtractRootTrackTransform(FAnimExtractContext(static_cast<double>(Time)), nullptr);
	}
	else if (const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Animation))
	{
		Result = AnimSequence->ExtractRootTrackTransform(FAnimExtractContext(static_cast<double>(Time)), nullptr);
		bUseNormalizedRootMotionScale = AnimSequence->bUseNormalizedRootMotionScale;
	}

	// Use old calculation if needed.
	// 如果需要，请使用旧的计算。
	if (bUseNormalizedRootMotionScale)
	{
		//Clear scale as it will muck up GetRelativeTransform
		//清除比例，因为它会弄乱 GetRelativeTransform
		Result.SetScale3D(FVector(1.f));
	}
	else if (Animation)
	{
		if (Animation->IsValidAdditive())
		{
			Result.SetScale3D(Result.GetScale3D() + DefaultScale);
		}
	}

	// Transform to Component Space
	// 变换到组件空间
	const FTransform RootToComponent = RootTransformRefPose.Inverse();
	Result = RootToComponent * Result;

	return Result;
}

Retargeting::FRetargetingScope::FRetargetingScope(const USkeleton* InSourceSkeleton, FCompactPose& ToRetargetPose, const DataModel::FEvaluationContext& InEvaluationContext)
	: SourceSkeleton(InSourceSkeleton),
	RetargetPose(ToRetargetPose),
	EvaluationContext(InEvaluationContext),
	RetargetTracking(FBuildRawPoseScratchArea::Get().RetargetTracking),
	bShouldRetarget((!ToRetargetPose.GetBoneContainer().GetDisableRetargeting() && EvaluationContext.RetargetTransforms.Num()) || (SourceSkeleton != ToRetargetPose.GetBoneContainer().GetSkeletonAsset()))
{
	check(SourceSkeleton);
	RetargetTracking.Reset();
}

void Retargeting::FRetargetingScope::AddTrackedBone(FCompactPoseBoneIndex CompactBoneIndex, int32 SkeletonBoneIndex) const
{
	if (bShouldRetarget)
	{
		RetargetTracking.Add({CompactBoneIndex, SkeletonBoneIndex});
	}
}

Retargeting::FRetargetingScope::~FRetargetingScope()
{
	if (bShouldRetarget)
	{
		const FBoneContainer& RequiredBones = RetargetPose.GetBoneContainer();
		for (const FRetargetTracking& RT : RetargetTracking)
		{
			FAnimationRuntime::RetargetBoneTransform(SourceSkeleton, EvaluationContext.RetargetSource, EvaluationContext.RetargetTransforms, RetargetPose[RT.PoseBoneIndex], RT.SkeletonBoneIndex, RT.PoseBoneIndex, RequiredBones, false);
		}
	}
}

void Retargeting::RetargetPose(FCompactPose& InOutPose, const FName& RetargetSource, const TArray<FTransform>& RetargetTransforms)
{
	const FBoneContainer& RequiredBones = InOutPose.GetBoneContainer();
	const bool bDisableRetargeting = RequiredBones.GetDisableRetargeting();

	if (!bDisableRetargeting && RetargetTransforms.Num())
	{
		const TArray<UE::Anim::Retargeting::FRetargetTracking>& RetargetTracking = UE::Anim::FBuildRawPoseScratchArea::Get().RetargetTracking;

		USkeleton* Skeleton = RequiredBones.GetSkeletonAsset();

		for (const UE::Anim::Retargeting::FRetargetTracking& RT : RetargetTracking)
		{
			FAnimationRuntime::RetargetBoneTransform(Skeleton, RetargetSource, RetargetTransforms, InOutPose[RT.PoseBoneIndex], RT.SkeletonBoneIndex, RT.PoseBoneIndex, RequiredBones, false);
		}
	}
}


#if WITH_EDITOR
bool CopyNotifies(const UAnimSequenceBase* SourceAnimSeq, UAnimSequenceBase* DestAnimSeq, bool bShowDialogs, bool bDeleteNotifies)
{
	// Abort if source == destination.
	// 如果源 == 目标则中止。
	if (SourceAnimSeq == DestAnimSeq)
	{
		return true;
	}

	// If the destination sequence is shorter than the source sequence, we'll be dropping notifies that
	// 如果目标序列比源序列短，我们将删除通知
	// occur at later times than the dest sequence is long.  Give the user a chance to abort if we
	// 发生的时间晚于 dest 序列的长度。  如果我们这样做的话，给用户一个中止的机会
	// find any notifies that won't be copied over.
	// 找到任何不会被复制的通知。
	if (DestAnimSeq->GetPlayLength() < SourceAnimSeq->GetPlayLength())
	{
		for (int32 NotifyIndex = 0; NotifyIndex < SourceAnimSeq->Notifies.Num(); ++NotifyIndex)
		{
			// If a notify is found which occurs off the end of the destination sequence, prompt the user to continue.
			// 如果发现目标序列末尾处出现通知，则提示用户继续。
			const FAnimNotifyEvent& SrcNotifyEvent = SourceAnimSeq->Notifies[NotifyIndex];
			if (SrcNotifyEvent.GetTriggerTime() > DestAnimSeq->GetPlayLength())
			{
				UE_LOG(LogAnimation, Warning, TEXT("Animation Notify trigger time %f falls outside of the destination animation sequence its length %f, notify will not be copied."), SrcNotifyEvent.GetTriggerTime(), DestAnimSeq->GetPlayLength());
				
				const bool bProceed = !bShowDialogs || EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "SomeNotifiesWillNotBeCopiedQ", "Some notifies will not be copied because the destination sequence is not long enough.  Proceed?"));
				if (!bProceed)
				{
					return false;
				}
				else
				{
					break;
				}
			}
		}
	}

	// If the destination sequence contains any notifies, ask the user if they'd like
	// 如果目标序列包含任何通知，请询问用户是否愿意
	// to delete the existing notifies before copying over from the source sequence.
	// 在从源序列复制之前删除现有通知。
	if (DestAnimSeq->Notifies.Num() > 0)
	{
		const bool bDeleteExistingNotifies = bDeleteNotifies || (bShowDialogs && EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(
			NSLOCTEXT("UnrealEd", "DestSeqAlreadyContainsNotifiesMergeQ", "The destination sequence already contains {0} notifies.  Delete these before copying?"), FText::AsNumber(DestAnimSeq->Notifies.Num()))));

		if (bDeleteExistingNotifies)
		{
			DestAnimSeq->Notifies.Empty();
			DestAnimSeq->MarkPackageDirty();
			DestAnimSeq->RefreshCacheData();
		}
	}

	// Do the copy.
	// [翻译失败: Do the copy.]
	int32 NumNotifiesThatWereNotCopied = 0;

	for (int32 NotifyIndex = 0; NotifyIndex < SourceAnimSeq->Notifies.Num(); ++NotifyIndex)
	{
		const FAnimNotifyEvent& SrcNotifyEvent = SourceAnimSeq->Notifies[NotifyIndex];

		// Skip notifies which occur at times later than the destination sequence is long.
		// [翻译失败: Skip notifies which occur at times later than the destination sequence is long.]
		if (SrcNotifyEvent.GetTriggerTime() > DestAnimSeq->GetPlayLength())
		{
			++NumNotifiesThatWereNotCopied;
			continue;
		}

		// Copy notify tracks from src to dest if they are missing
		// 如果通知曲目丢失，则将通知曲目从 src 复制到 dest
		if (SrcNotifyEvent.TrackIndex >= DestAnimSeq->AnimNotifyTracks.Num())
		{
			for (int32 TrackIndex = DestAnimSeq->AnimNotifyTracks.Num(); TrackIndex <= SrcNotifyEvent.TrackIndex; ++TrackIndex)
			{
				DestAnimSeq->AnimNotifyTracks.Add(FAnimNotifyTrack(SourceAnimSeq->AnimNotifyTracks[TrackIndex].TrackName, SourceAnimSeq->AnimNotifyTracks[TrackIndex].TrackColor));
			}
		}

		// Track the location of the new notify.
		// 跟踪新通知的位置。
		int32 NewNotifyIndex = DestAnimSeq->Notifies.AddDefaulted();
		FAnimNotifyEvent& NotifyEvent = DestAnimSeq->Notifies[NewNotifyIndex];

		// Copy properties of the NotifyEvent
		// 复制 NotifyEvent 的属性
		NotifyEvent.TrackIndex = SrcNotifyEvent.TrackIndex;
		NotifyEvent.NotifyName = SrcNotifyEvent.NotifyName;
		NotifyEvent.Duration = SrcNotifyEvent.Duration;

		// Copy the notify itself, and point the new one at it.
		// 复制通知本身，并将新通知指向它。
		if (SrcNotifyEvent.Notify)
		{
			DestAnimSeq->Notifies[NewNotifyIndex].Notify = static_cast<UAnimNotify*>(StaticDuplicateObject(SrcNotifyEvent.Notify, DestAnimSeq, NAME_None, RF_AllFlags, nullptr, EDuplicateMode::Normal, ~EInternalObjectFlags::RootSet));
		}
		else
		{
			DestAnimSeq->Notifies[NewNotifyIndex].Notify = nullptr;
		}

		if (SrcNotifyEvent.NotifyStateClass)
		{
			DestAnimSeq->Notifies[NewNotifyIndex].NotifyStateClass = static_cast<UAnimNotifyState*>(StaticDuplicateObject(SrcNotifyEvent.NotifyStateClass, DestAnimSeq, NAME_None, RF_AllFlags, nullptr, EDuplicateMode::Normal, ~EInternalObjectFlags::RootSet));
		}
		else
		{
			DestAnimSeq->Notifies[NewNotifyIndex].NotifyStateClass = nullptr;
		}

		// Copy notify timing
		// 复制通知时间
		NotifyEvent.Link(DestAnimSeq, SrcNotifyEvent.GetTriggerTime());
		NotifyEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(DestAnimSeq->CalculateOffsetForNotify(NotifyEvent.GetTriggerTime()));

		// Make sure editor knows we've changed something.
		// 确保编辑知道我们改变了一些东西。
		DestAnimSeq->MarkPackageDirty();
		DestAnimSeq->RefreshCacheData();
	}

	// Inform the user if some notifies weren't copied.
	// 如果某些通知未复制，则通知用户。
	if (bShowDialogs && NumNotifiesThatWereNotCopied > 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			NSLOCTEXT("UnrealEd", "SomeNotifiesWereNotCopiedF", "Because the destination sequence was shorter, {0} notifies were not copied."), FText::AsNumber(NumNotifiesThatWereNotCopied)));
	}

	return true;
}

#endif // WITH_EDITOR

bool Compression::CompressAnimationDataTracks(TArray<FRawAnimSequenceTrack>& RawAnimationData, int32 NumberOfKeys, FName ErrorName, float MaxPosDiff /*= 0.0001f*/, float MaxAngleDiff /*= 0.0003f*/, float MaxScaleDiff)
{
	bool bRemovedKeys = false;

#if WITH_EDITORONLY_DATA
	if (ensureMsgf(RawAnimationData.Num() > 0, TEXT("%s is trying to compress while raw animation is missing"), * ErrorName.ToString()))
	{
		// This removes trivial keys, and this has to happen before the removing tracks
		// 这会删除琐碎的键，并且必须在删除轨道之前发生
		for (int32 TrackIndex = 0; TrackIndex < RawAnimationData.Num(); TrackIndex++)
		{
			bRemovedKeys |= CompressRawAnimSequenceTrack(RawAnimationData[TrackIndex], NumberOfKeys, ErrorName, MaxPosDiff, MaxAngleDiff, MaxScaleDiff);
		}
	}
#endif
	return bRemovedKeys;
}

bool Compression::CompressAnimationDataTracks(const USkeleton* Skeleton, const TArray<FTrackToSkeletonMap>& TrackToSkeleton, TArray<FRawAnimSequenceTrack>& RawAnimationData, int32 NumberOfKeys, FName ErrorName, float MaxPosDiff, float MaxAngleDiff, float MaxScaleDiff)
{
	bool bRemovedKeys = false;

#if WITH_EDITORONLY_DATA
	if (ensureMsgf(RawAnimationData.Num() > 0, TEXT("%s is trying to compress while raw animation is missing"), * ErrorName.ToString()))
	{
		// This removes trivial keys, and this has to happen before the removing tracks
		// 这会删除琐碎的键，并且必须在删除轨道之前发生
		for (int32 TrackIndex = 0; TrackIndex < RawAnimationData.Num(); TrackIndex++)
		{
			bRemovedKeys |= CompressRawAnimSequenceTrack(RawAnimationData[TrackIndex], NumberOfKeys, ErrorName, MaxPosDiff, MaxAngleDiff, MaxScaleDiff);
		}
	}
#endif
	return bRemovedKeys;
}

bool Compression::CompressRawAnimSequenceTrack(FRawAnimSequenceTrack& RawTrack, int32 NumberOfKeys, FName ErrorName, float MaxPosDiff, float MaxAngleDiff)
{
	return CompressRawAnimSequenceTrack(RawTrack, NumberOfKeys, ErrorName, MaxPosDiff, MaxAngleDiff, 0.0001f);
}

bool Compression::CompressRawAnimSequenceTrack(FRawAnimSequenceTrack& RawTrack, int32 NumberOfKeys, FName ErrorName, float MaxPosDiff, float MaxAngleDiff, float MaxScaleDiff)
{
	bool bRemovedKeys = false;

	// First part is to make sure we have valid input
	// 第一部分是确保我们有有效的输入
	bool const bPosTrackIsValid = (RawTrack.PosKeys.Num() == 1 || RawTrack.PosKeys.Num() == NumberOfKeys);
	if (!bPosTrackIsValid)
	{
		UE_LOG(LogAnimation, Warning, TEXT("Found non valid position track for %s, %d frames, instead of %d. Chopping!"), *ErrorName.ToString(), RawTrack.PosKeys.Num(), NumberOfKeys);
		bRemovedKeys = true;
		RawTrack.PosKeys.RemoveAt(1, RawTrack.PosKeys.Num() - 1);
		RawTrack.PosKeys.Shrink();
		check(RawTrack.PosKeys.Num() == 1);
	}

	bool const bRotTrackIsValid = (RawTrack.RotKeys.Num() == 1 || RawTrack.RotKeys.Num() == NumberOfKeys);
	if (!bRotTrackIsValid)
	{
		UE_LOG(LogAnimation, Warning, TEXT("Found non valid rotation track for %s, %d frames, instead of %d. Chopping!"), *ErrorName.ToString(), RawTrack.RotKeys.Num(), NumberOfKeys);
		bRemovedKeys = true;
		RawTrack.RotKeys.RemoveAt(1, RawTrack.RotKeys.Num() - 1);
		RawTrack.RotKeys.Shrink();
		check(RawTrack.RotKeys.Num() == 1);
	}

	// scale keys can be empty, and that is valid 
	// 比例键可以为空，且有效
	bool const bScaleTrackIsValid = (RawTrack.ScaleKeys.Num() == 0 || RawTrack.ScaleKeys.Num() == 1 || RawTrack.ScaleKeys.Num() == NumberOfKeys);
	if (!bScaleTrackIsValid)
	{
		UE_LOG(LogAnimation, Warning, TEXT("Found non valid scaling track for %s, %d frames, instead of %d. Chopping!"), *ErrorName.ToString(), RawTrack.ScaleKeys.Num(), NumberOfKeys);
		bRemovedKeys = true;
		RawTrack.ScaleKeys.RemoveAt(1, RawTrack.ScaleKeys.Num() - 1);
		RawTrack.ScaleKeys.Shrink();
		check(RawTrack.ScaleKeys.Num() == 1);
	}

	// Second part is actual compression.
	// 第二部分是实际压缩。

	// Check variation of position keys
	// 检查位置键的变化
	if ((RawTrack.PosKeys.Num() > 1) && (MaxPosDiff >= 0.0f))
	{
		FVector3f FirstPos = RawTrack.PosKeys[0];
		bool bFramesIdentical = true;
		for (int32 j = 1; j < RawTrack.PosKeys.Num() && bFramesIdentical; j++)
		{
			if ((FirstPos - RawTrack.PosKeys[j]).SizeSquared() > FMath::Square(MaxPosDiff))
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		// 如果所有关键帧都相同，则删除除第一帧之外的所有帧
		if (bFramesIdentical)
		{
			bRemovedKeys = true;
			RawTrack.PosKeys.RemoveAt(1, RawTrack.PosKeys.Num() - 1);
			RawTrack.PosKeys.Shrink();
			check(RawTrack.PosKeys.Num() == 1);
		}
	}

	// Check variation of rotational keys
	// 检查旋转钥匙的变化
	if ((RawTrack.RotKeys.Num() > 1) && (MaxAngleDiff >= 0.0f))
	{
		FQuat4f FirstRot = RawTrack.RotKeys[0];
		bool bFramesIdentical = true;
		for (int32 j = 1; j < RawTrack.RotKeys.Num() && bFramesIdentical; j++)
		{
			if (FQuat4f::Error(FirstRot, RawTrack.RotKeys[j]) > MaxAngleDiff)
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		// 如果所有关键帧都相同，则删除除第一帧之外的所有帧
		if (bFramesIdentical)
		{
			bRemovedKeys = true;
			RawTrack.RotKeys.RemoveAt(1, RawTrack.RotKeys.Num() - 1);
			RawTrack.RotKeys.Shrink();
			check(RawTrack.RotKeys.Num() == 1);
		}
	}

	// Check variation of Scaleition keys
	// 检查 Scaleition 键的变化
	if ((RawTrack.ScaleKeys.Num() > 1) && (MaxScaleDiff >= 0.0f))
	{
		FVector3f FirstScale = RawTrack.ScaleKeys[0];
		bool bFramesIdentical = true;
		for (int32 j = 1; j < RawTrack.ScaleKeys.Num() && bFramesIdentical; j++)
		{
			if ((FirstScale - RawTrack.ScaleKeys[j]).SizeSquared() > FMath::Square(MaxScaleDiff))
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		// 如果所有关键帧都相同，则删除除第一帧之外的所有帧
		if (bFramesIdentical)
		{
			bRemovedKeys = true;
			RawTrack.ScaleKeys.RemoveAt(1, RawTrack.ScaleKeys.Num() - 1);
			RawTrack.ScaleKeys.Shrink();
			check(RawTrack.ScaleKeys.Num() == 1);
		}
	}

	return bRemovedKeys;
}

void Compression::SanitizeRawAnimSequenceTrack(FRawAnimSequenceTrack& RawTrack)
{
	// if scale is too small, zero it out. Cause it hard to retarget when compress
	// 如果比例太小，请将其归零。导致压缩时很难重定向
	// inverse scale is applied to translation, and causing translation to be huge to retarget, but
	// 反比例应用于翻译，并导致翻译巨大以重新定位，但是
	// compression can't handle that much precision. 
	// 压缩无法处理那么高的精度。
	for (auto& Scale3D : RawTrack.ScaleKeys)
	{
		if (FMath::IsNearlyZero(Scale3D.X))
		{
			Scale3D.X = 0.f;
		}
		if (FMath::IsNearlyZero(Scale3D.Y))
		{
			Scale3D.Y = 0.f;
		}
		if (FMath::IsNearlyZero(Scale3D.Z))
		{
			Scale3D.Z = 0.f;
		}
	}
	
	for (auto& Position : RawTrack.PosKeys)
	{		
		if (FMath::IsNearlyZero(Position.X))
		{
			Position.X = 0.f;
		}
		if (FMath::IsNearlyZero(Position.Y))
		{
			Position.Y = 0.f;
		}
		if (FMath::IsNearlyZero(Position.Z))
		{
			Position.Z = 0.f;
		}
	}

	// make sure Rotation part is normalized before compress
	// 确保旋转部分在压缩前标准化
	for (auto& Rotation : RawTrack.RotKeys)
	{
		if (!Rotation.IsNormalized())
		{
			Rotation.Normalize();
		}

		if (FMath::IsNearlyZero(Rotation.X))
		{
			Rotation.X = 0.f;
		}
		if (FMath::IsNearlyZero(Rotation.Y))
		{
			Rotation.Y = 0.f;
		}
		if (FMath::IsNearlyZero(Rotation.Z))
		{
			Rotation.Z = 0.f;
		}
	}
}

#if WITH_EDITOR
Compression::FScopedCompressionGuard::FScopedCompressionGuard(UAnimSequence* InAnimSequence) : AnimSequence(InAnimSequence)
{
	if(AnimSequence)
	{
		AnimSequence->bBlockCompressionRequests = true;		
	}
}

Compression::FScopedCompressionGuard::~FScopedCompressionGuard()
{
	if(AnimSequence)
	{
		AnimSequence->bBlockCompressionRequests = false;		
	}
}

bool AnimationData::AddLoopingInterpolation(UAnimSequence* InSequence)
{
	const IAnimationDataModel* DataModel = InSequence->GetDataModel();
	IAnimationDataController& Controller = InSequence->GetController();

	const int32 NumTracks = DataModel->GetNumBoneTracks();
	const int32 NumKeys = DataModel->GetNumberOfKeys();

	if (NumTracks > 0 && NumKeys > 0)
	{
		// now I need to calculate back to new animation data
		// 现在我需要计算回新的动画数据
		auto LoopKeyData = [&](auto& KeyData)
		{
			// Need at least a single 
			// 至少需要一个
			if (KeyData.Num() > 1)
			{
				auto FirstKey = KeyData[0];
				KeyData.Add(FirstKey);
			}
		};
		
		IAnimationDataController::FScopedBracket ScopedBracket(Controller, LOCTEXT("AddLoopingInterpolation_Bracket", "Adding looping interpolation"));

		TArray<FTransform> BoneTransforms;
		TArray<FName> TrackNames;
		DataModel->GetBoneTrackNames(TrackNames);
 
		TArray<FVector3f> PositionalKeys, ScaleKeys;
		TArray<FQuat4f> RotationalKeys;
		for (const FName& TrackName : TrackNames)
		{
			BoneTransforms.Reset();
			PositionalKeys.Reset();
			RotationalKeys.Reset();
			ScaleKeys.Reset();
			DataModel->GetBoneTrackTransforms(TrackName, BoneTransforms);

			for (const FTransform& Transform : BoneTransforms)
			{
				PositionalKeys.Add(FVector3f(Transform.GetLocation()));
				RotationalKeys.Add(FQuat4f(Transform.GetRotation()));
				ScaleKeys.Add(FVector3f(Transform.GetScale3D()));
			}
		
			LoopKeyData(PositionalKeys);
			LoopKeyData(RotationalKeys);
			LoopKeyData(ScaleKeys);

			Controller.SetBoneTrackKeys(TrackName, PositionalKeys, RotationalKeys, ScaleKeys);
		}

		// New number of frames is equal to current number of keys, as we'll be adding one frame (Number of Frames + 1 == Number of Keys)
		// 新的帧数等于当前的键数，因为我们将添加一帧（帧数 + 1 == 键数）
		Controller.SetNumberOfFrames(NumKeys);

		return true;
	}

	return false;
}

bool AnimationData::Trim(UAnimSequence* InSequence, TRange<FFrameNumber> TrimRange)
{
	const IAnimationDataModel* DataModel = InSequence->GetDataModel();
	IAnimationDataController& Controller = InSequence->GetController();

	const int32 NumTracks = DataModel->GetNumBoneTracks();
	const int32 NumKeys = DataModel->GetNumberOfKeys();

	if (NumTracks > 0 && NumKeys > 0)
	{
		const FFrameRate& FrameRate = DataModel->GetFrameRate();
		// if there is only one key, there is nothing to trim away
		// 如果只有一把钥匙，就没有什么可以修剪的
		if (NumKeys <= 1)
		{
			return false;
		}

		// Convert frame range to key indices
		// 将帧范围转换为关键索引
		const int32 StartTrimFrameIndex = TrimRange.GetLowerBound().IsInclusive() ? TrimRange.GetLowerBound().GetValue().Value : TrimRange.GetLowerBound().GetValue().Value + 1;
		const int32 EndTrimFrameIndex = TrimRange.GetUpperBound().IsInclusive() ? TrimRange.GetUpperBound().GetValue().Value : TrimRange.GetUpperBound().GetValue().Value - 1;
		const int32 NumTrimmedFrames = (EndTrimFrameIndex - StartTrimFrameIndex) + 1;

		const int32 StartTrimKeyIndex = StartTrimFrameIndex;
		const int32 NumTrimmedKeys = NumTrimmedFrames;

		if (StartTrimKeyIndex == 0 && NumTrimmedFrames == NumKeys)
		{
			return false;
		}

		if (NumTrimmedFrames == 0)
		{
			return false;
		}
		
		IAnimationDataController::FScopedBracket ScopedBracket(Controller, LOCTEXT("TrimRawAnimation_Bracket", "Trimming Animation Track Data"));
		RemoveKeys(InSequence, StartTrimKeyIndex, NumTrimmedFrames);
		
		return true;
	}

	return false;
}

bool AnimationData::Trim(UAnimSequence* InSequence, float TrimStart, float TrimEnd, bool bInclusiveEnd /*=false*/ )
{
	const IAnimationDataModel* DataModel = InSequence->GetDataModel();
	const FFrameRate& FrameRate = DataModel->GetFrameRate();
	const FFrameNumber StartFrameTrim = FrameRate.AsFrameTime(TrimStart).RoundToFrame();
	const FFrameNumber EndFrameTrim = FrameRate.AsFrameTime(TrimEnd).RoundToFrame();

	const TRange<FFrameNumber> TrimRange(TRangeBound<FFrameNumber>::Inclusive(StartFrameTrim), bInclusiveEnd ? TRangeBound<FFrameNumber>::Inclusive(EndFrameTrim) : TRangeBound<FFrameNumber>::Exclusive(EndFrameTrim));
	return Trim(InSequence, TrimRange);
}

void AnimationData::DuplicateKeys(UAnimSequence* InSequence, int32 StartKeyIndex, int32 NumDuplicates, int32 SourceKeyIndex /*= INDEX_NONE */)
{
	const IAnimationDataModel* Model = InSequence->GetDataModel();
	IAnimationDataController& Controller = InSequence->GetController();

	const int32 NumberOfKeys = Model->GetNumberOfKeys();
	const FFrameRate& FrameRate = Model->GetFrameRate();

	// Ensure that the index at which keys will be inserted, and the source key index for the duplicates is valid as well
	// 确保将插入键的索引以及重复项的源键索引也有效
	if (StartKeyIndex >= 0 && StartKeyIndex <= NumberOfKeys && NumDuplicates >= 1)
	{
		const int32 CopyKeyIndex = SourceKeyIndex == INDEX_NONE ? StartKeyIndex : SourceKeyIndex;
		if (CopyKeyIndex >= 0 && CopyKeyIndex < NumberOfKeys)
		{
			IAnimationDataController::FScopedBracket ScopedBracket(Controller, LOCTEXT("DuplicateKeys_Bracket", "Inserting Duplicate Animation Track Keys"));

			const int32 NumFramesToInsert = NumDuplicates;
			const int32 EndFrameIndex = StartKeyIndex + NumDuplicates;

			auto InsertFrames = [&](auto& KeyData)
			{
				if (KeyData.Num() >= 1 && KeyData.IsValidIndex(CopyKeyIndex))
				{
					auto SourceKeyData = KeyData[CopyKeyIndex];
					KeyData.InsertZeroed(StartKeyIndex, NumFramesToInsert);

					for (int32 FrameIndex = StartKeyIndex; FrameIndex < EndFrameIndex; ++FrameIndex)
					{
						KeyData[FrameIndex] = SourceKeyData;
					}
				}
			};

			TArray<FName> TrackNames;
			Model->GetBoneTrackNames(TrackNames);
			
			TArray<FTransform> BoneTransforms;
			BoneTransforms.Reserve(Model->GetNumberOfKeys());
			
			for (const FName& TrackName : TrackNames)
			{
				BoneTransforms.Reset();
				Model->GetBoneTrackTransforms(TrackName, BoneTransforms);

				TArray<FVector3f> PosKeys;
				PosKeys.SetNum(BoneTransforms.Num() + NumDuplicates);
				TArray<FQuat4f> RotKeys;
				RotKeys.SetNum(BoneTransforms.Num() + NumDuplicates);
				TArray<FVector3f> ScaleKeys;
				ScaleKeys.SetNum(BoneTransforms.Num() + NumDuplicates);

				ensure(BoneTransforms.IsValidIndex(SourceKeyIndex));
				for (int32 TransformIndex = 0; TransformIndex < BoneTransforms.Num() + NumFramesToInsert; ++TransformIndex)
				{
					if (TransformIndex < StartKeyIndex)
					{
						PosKeys[TransformIndex] = FVector3f(BoneTransforms[TransformIndex].GetLocation());
						RotKeys[TransformIndex] = FQuat4f(BoneTransforms[TransformIndex].GetRotation());
						ScaleKeys[TransformIndex] = FVector3f(BoneTransforms[TransformIndex].GetScale3D());
					}
					if (TransformIndex >= StartKeyIndex && TransformIndex < StartKeyIndex + NumDuplicates)
					{
						PosKeys[TransformIndex] = FVector3f(BoneTransforms[SourceKeyIndex].GetLocation());
						RotKeys[TransformIndex] = FQuat4f(BoneTransforms[SourceKeyIndex].GetRotation());
						ScaleKeys[TransformIndex] = FVector3f(BoneTransforms[SourceKeyIndex].GetScale3D());
					}
					else if (TransformIndex >= StartKeyIndex + NumDuplicates)
					{
						PosKeys[TransformIndex] = FVector3f(BoneTransforms[TransformIndex - NumDuplicates].GetLocation());
						RotKeys[TransformIndex] = FQuat4f(BoneTransforms[TransformIndex - NumDuplicates].GetRotation());
						ScaleKeys[TransformIndex] = FVector3f(BoneTransforms[TransformIndex - NumDuplicates].GetScale3D());
					}
				}

				Controller.SetBoneTrackKeys(TrackName, PosKeys, RotKeys, ScaleKeys);
				
				BoneTransforms.Reset();
			}

			// The number of keys has changed, which means that the sequence length and number of frames should be updated as well
			// 键的数量发生了变化，这意味着序列长度和帧数也应该更新
			const int32 NewNumKeys = NumberOfKeys + NumDuplicates;
			const int32 NewNumFrames = NewNumKeys - 1;
			const float NewSequenceLength = FrameRate.AsSeconds(NewNumFrames);

			const float StartTime = FrameRate.AsSeconds(StartKeyIndex);
			const float InsertedTime = FrameRate.AsInterval() * NumDuplicates;

			// Notify will happen with time slice that was inserted
			// 通知将随着插入的时间片而发生
			Controller.ResizeInFrames(NewNumFrames, StartKeyIndex, StartKeyIndex + NumDuplicates);
		}
	}
}

void AnimationData::RemoveKeys(UAnimSequence* InSequence, int32 StartKeyIndex, int32 NumKeysToRemove)
{
	const IAnimationDataModel* Model = InSequence->GetDataModel();
	IAnimationDataController& Controller = InSequence->GetController();

	const int32 NumberOfKeys = Model->GetNumberOfKeys();
	const FFrameRate& FrameRate = Model->GetFrameRate();

	const int32 EndKeyIndex = StartKeyIndex + NumKeysToRemove;
	if (StartKeyIndex >= 0 && StartKeyIndex < NumberOfKeys && NumKeysToRemove > 0 && EndKeyIndex <= NumberOfKeys)
	{
		IAnimationDataController::FScopedBracket ScopedBracket(Controller, LOCTEXT("RemoveKeys_Bracket", "Removing Animation Track Keys"));

		auto ShrinkKeys = [&](auto& KeyData)
		{
			// Dont allow us to trim below 2 keys (1 frame)
			// 不允许我们修剪低于 2 个关键点（1 帧）
			int32 NumKeyDataToRemove = NumKeysToRemove;
			if((KeyData.Num() - NumKeysToRemove) < 2)
			{
				NumKeyDataToRemove = KeyData.Num() - 2;
			}

			if (KeyData.Num() >= (StartKeyIndex + NumKeyDataToRemove))
			{
				KeyData.RemoveAt(StartKeyIndex, NumKeyDataToRemove);
				check(KeyData.Num() > 0);
				KeyData.Shrink();
			}
		};

		const int32 NewNumberOfKeys = NumberOfKeys - NumKeysToRemove;

		TArray<FName> TrackNames;
		Model->GetBoneTrackNames(TrackNames);
			
		TArray<FTransform> BoneTransforms;
		BoneTransforms.Reserve(Model->GetNumberOfKeys());
			
		for (const FName& TrackName : TrackNames)
		{
			BoneTransforms.Reset();

			Model->GetBoneTrackTransforms(TrackName, BoneTransforms);

			TArray<FVector3f> PosKeys;
			PosKeys.SetNum(BoneTransforms.Num() - NumKeysToRemove);
			TArray<FQuat4f> RotKeys;
			RotKeys.SetNum(BoneTransforms.Num() - NumKeysToRemove);
			TArray<FVector3f> ScaleKeys;
			ScaleKeys.SetNum(BoneTransforms.Num() - NumKeysToRemove);

			for (int32 TransformIndex = 0; TransformIndex < BoneTransforms.Num() - NumKeysToRemove; ++TransformIndex)
			{
				if (TransformIndex < StartKeyIndex)
				{
					PosKeys[TransformIndex] = FVector3f(BoneTransforms[TransformIndex].GetLocation());
					RotKeys[TransformIndex] = FQuat4f(BoneTransforms[TransformIndex].GetRotation());
					ScaleKeys[TransformIndex] = FVector3f(BoneTransforms[TransformIndex].GetScale3D());
				}
				if (TransformIndex >= StartKeyIndex)
				{
					PosKeys[TransformIndex] = FVector3f(BoneTransforms[TransformIndex + NumKeysToRemove].GetLocation());
					RotKeys[TransformIndex] = FQuat4f(BoneTransforms[TransformIndex + NumKeysToRemove].GetRotation());
					ScaleKeys[TransformIndex] = FVector3f(BoneTransforms[TransformIndex + NumKeysToRemove].GetScale3D());		
				}
			}
			
			Controller.SetBoneTrackKeys(TrackName, PosKeys, RotKeys, ScaleKeys);
		}

		const int32 NewNumberOfFrames = FMath::Max(NewNumberOfKeys - 1, 1);

		const int32 StartFrame = FMath::Max(StartKeyIndex - 1, 0);
		// Notify will happen with time slice that was removed
		// 通知将在删除的时间片上发生
		Controller.ResizeInFrames(NewNumberOfFrames, StartFrame, StartFrame + NumKeysToRemove);
	}
}

FName AnimationData::FindFirstChildTrackName(const UAnimSequence* InSequence, const USkeleton* Skeleton, const FName& BoneName)
{
	const IAnimationDataModel* DataModel = InSequence->GetDataModel();
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

	const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		// get out, nothing to do
		// [翻译失败: get out, nothing to do]
		return NAME_None;
	}

	// find children
	// [翻译失败: find children]
	TArray<int32> ChildBoneIndices;
	if (Skeleton->GetChildBones(BoneIndex, ChildBoneIndices) > 0)
	{
		// first look for direct children
		// [翻译失败: first look for direct children]
		for (const int32 ChildBoneIndex : ChildBoneIndices)
		{
			const FName ChildBoneName = RefSkeleton.GetBoneName(ChildBoneIndex);
			if (DataModel->IsValidBoneTrackName(ChildBoneName))
			{
				// found the new track
				// [翻译失败: found the new track]
				return ChildBoneName;
			}
		}

		int32 BestGrandChildIndex = INDEX_NONE;
		FName BestGrandChildName = NAME_None;
		// if you didn't find yet, now you have to go through all children
		// 如果你还没有找到，现在你必须遍历所有的孩子
		for (const int32 ChildBoneIndex : ChildBoneIndices)
		{
			const FName ChildBoneName = RefSkeleton.GetBoneName(ChildBoneIndex);
			// now I have to go through all childrewn and find who is earliest since I don't know which one might be the closest one
			// 现在我必须遍历所有的孩子并找出谁是最早的，因为我不知道哪一个可能是最接近的
			const FName GrandChildName = FindFirstChildTrackName(InSequence, Skeleton, ChildBoneName);
			if (GrandChildName != NAME_None)
			{
				const int32 GrandChildIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(GrandChildName);
				if (BestGrandChildIndex == INDEX_NONE)
				{
					BestGrandChildIndex = GrandChildIndex;
					BestGrandChildName = GrandChildName;
				}
				else if (BestGrandChildIndex > GrandChildIndex)
				{
					// best should be earlier track index
					// 最好应该是更早的轨道索引
					BestGrandChildIndex = GrandChildIndex;
					BestGrandChildName = GrandChildName;
				}
			}
		}

		return BestGrandChildName;
	}

	return NAME_None;
}

#endif // WITH_EDITOR


} // Namespace Anim

} // Namespace UE

#undef LOCTEXT_NAMESPACE 
