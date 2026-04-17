// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/PoseAsset.h"

#include "BonePose.h"
#include "UObject/FortniteMainBranchObjectVersion.h"
#include "UObject/FrameworkObjectVersion.h"
#include "UObject/UE5MainStreamObjectVersion.h"
#include "UObject/UE5ReleaseStreamObjectVersion.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimCurveUtils.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimStats.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimSequenceHelpers.h"
#include "Animation/SkeletonRemapping.h"
#include "Animation/SkeletonRemappingRegistry.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/LinkerLoad.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectAnnotation.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoseAsset)

#define LOCTEXT_NAMESPACE "PoseAsset"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FPoseDataContainer
// FPose数据容器
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#if WITH_EDITOR
namespace UE { namespace Anim { FUObjectAnnotationSparseBool GRegeneratedPoseAssets; } }
#endif // WITH_EDITOR

bool FPoseDataContainer::Serialize(FArchive& Ar)
{
#if WITH_EDITORONLY_DATA
	Ar.UsingCustomVersion(FUE5MainStreamObjectVersion::GUID);
#endif

	return false;
}

void FPoseDataContainer::PostSerialize(const FArchive& Ar)
{
#if WITH_EDITORONLY_DATA
	if(Ar.CustomVer(FUE5MainStreamObjectVersion::GUID) < FUE5MainStreamObjectVersion::AnimationRemoveSmartNames)
	{
		for(const FSmartName& PoseName : PoseNames_DEPRECATED)
		{
			PoseFNames.Add(PoseName.DisplayName);
		}
	}
#endif

	RebuildCurveIndexTable();
}

void FPoseDataContainer::Reset()
{
	// clear everything
 // 清除一切
	PoseFNames.Reset();
	SortedCurveIndices.Reset();
	Poses.Reset();
	Tracks.Reset();
	TrackBoneIndices.Reset();
	Curves.Reset();
}

void FPoseDataContainer::GetPoseCurve(const FPoseData* PoseData, FBlendedCurve& OutCurve) const
{
	if (PoseData)
	{
		const TArray<float>& CurveValues = PoseData->CurveData;
		checkSlow(CurveValues.Num() == Curves.Num());

		auto GetNameFromIndex = [this](int32 InCurveIndex)
		{
			return Curves[SortedCurveIndices[InCurveIndex]].GetName();
		};

		auto GetValueFromIndex = [this, &CurveValues](int32 InCurveIndex)
		{
			return CurveValues[SortedCurveIndices[InCurveIndex]];
		};

		UE::Anim::FCurveUtils::BuildSorted(OutCurve, CurveValues.Num(), GetNameFromIndex, GetValueFromIndex, OutCurve.GetFilter());
	}
}

void FPoseDataContainer::BlendPoseCurve(const FPoseData* PoseData, FBlendedCurve& InOutCurve, float Weight) const
{
	if (PoseData)
	{
		const TArray<float>& CurveValues = PoseData->CurveData;
		checkSlow(CurveValues.Num() == Curves.Num());

		FBlendedCurve Curve;
		Curve.SetFilter(InOutCurve.GetFilter());

		GetPoseCurve(PoseData, Curve);

		UE::Anim::FNamedValueArrayUtils::Union(InOutCurve, Curve,
			[Weight](UE::Anim::FCurveElement& InOutElement, const UE::Anim::FCurveElement& InElement, UE::Anim::ENamedValueUnionFlags InFlags)
			{
				InOutElement.Value = InElement.Value * Weight + InOutElement.Value;
				InOutElement.Flags |= InElement.Flags;
			});
	}
}

FPoseData* FPoseDataContainer::FindPoseData(FName PoseName)
{
	int32 PoseIndex = PoseFNames.Find(PoseName);
	if (PoseIndex != INDEX_NONE)
	{
		return &Poses[PoseIndex];
	}

	return nullptr;
}

FPoseData* FPoseDataContainer::FindOrAddPoseData(FName PoseName)
{
	int32 PoseIndex = PoseFNames.Find(PoseName);
	if (PoseIndex == INDEX_NONE)
	{
		PoseIndex = PoseFNames.Add(PoseName);
		check(PoseIndex == Poses.AddZeroed(1));
	}

	return &Poses[PoseIndex];
}

FTransform FPoseDataContainer::GetDefaultTransform(const FName& InTrackName, USkeleton* InSkeleton, const TArray<FTransform>& RefPose) const
{
	if (InSkeleton)
	{
		int32 SkeletonIndex = InSkeleton->GetReferenceSkeleton().FindBoneIndex(InTrackName);
		return GetDefaultTransform(SkeletonIndex, RefPose);
	}

	return FTransform::Identity;
}

FTransform FPoseDataContainer::GetDefaultTransform(int32 SkeletonIndex, const TArray<FTransform>& RefPose) const
{
	if (RefPose.IsValidIndex(SkeletonIndex))
	{
		return RefPose[SkeletonIndex];
	}

	return FTransform::Identity;
}


#if WITH_EDITOR
void FPoseDataContainer::AddOrUpdatePose(const FName& InPoseName, const TArray<FTransform>& InLocalSpacePose, const TArray<float>& InCurveData)
{
	// make sure the transforms and curves are the correct size
 // 确保变换和曲线的大小正确
	if (ensureAlways(InLocalSpacePose.Num() == Tracks.Num()) && ensureAlways(InCurveData.Num() == Curves.Num()))
	{
		// find or add pose data
  // 查找或添加姿势数据
		FPoseData* PoseDataPtr = FindOrAddPoseData(InPoseName);
		// now add pose
  // 现在添加姿势
		PoseDataPtr->SourceLocalSpacePose = InLocalSpacePose;
		PoseDataPtr->SourceCurveData = InCurveData;
	}

	// for now we only supports same tracks
 // 目前我们只支持相同的曲目
}

bool FPoseDataContainer::InsertTrack(const FName& InTrackName, USkeleton* InSkeleton, const TArray<FTransform>& RefPose)
{
	check(InSkeleton);

	// make sure the transform is correct size
 // 确保变换尺寸正确
	if (Tracks.Contains(InTrackName) == false)
	{
		int32 SkeletonIndex = InSkeleton->GetReferenceSkeleton().FindBoneIndex(InTrackName);
		int32 TrackIndex = INDEX_NONE;
		if (SkeletonIndex != INDEX_NONE)
		{
			Tracks.Add(InTrackName);
			TrackIndex = Tracks.Num() - 1;

			// now insert default refpose
   // 现在插入默认引用
			const FTransform DefaultPose = GetDefaultTransform(SkeletonIndex, RefPose);

			for (FPoseData& PoseData : Poses)
			{
				ensureAlways(PoseData.SourceLocalSpacePose.Num() == TrackIndex);

				PoseData.SourceLocalSpacePose.Add(DefaultPose);

				// make sure they always match
    // 确保它们始终匹配
				ensureAlways(PoseData.SourceLocalSpacePose.Num() == Tracks.Num());
			}

			return true;
		}

		return false;
	}

	return false;
}

bool FPoseDataContainer::FillUpSkeletonPose(FPoseData* PoseData, const USkeleton* InSkeleton)
{
	if (PoseData)
	{
		int32 TrackIndex = 0;
		const TArray<FTransform>& RefPose = InSkeleton->GetRefLocalPoses();
		for (const int32& SkeletonIndex : TrackBoneIndices)
		{
			PoseData->SourceLocalSpacePose[TrackIndex] = RefPose[SkeletonIndex];
			++TrackIndex;
		}

		return true;
	}

	return false;
}

void FPoseDataContainer::RenamePose(FName OldPoseName, FName NewPoseName)
{
	int32 PoseIndex = PoseFNames.Find(OldPoseName);
	if (PoseIndex != INDEX_NONE)
	{
		PoseFNames[PoseIndex] = NewPoseName;
	}
}

int32 FPoseDataContainer::DeletePose(FName PoseName)
{
	int32 PoseIndex = PoseFNames.Find(PoseName);
	if (PoseIndex != INDEX_NONE)
	{
		PoseFNames.RemoveAt(PoseIndex);
		Poses.RemoveAt(PoseIndex);
		return PoseIndex;
	}

	return INDEX_NONE;
}

bool FPoseDataContainer::DeleteCurve(FName CurveName)
{
	for (int32 CurveIndex = 0; CurveIndex < Curves.Num(); ++CurveIndex)
	{
		if (Curves[CurveIndex].GetName() == CurveName)
		{
			Curves.RemoveAt(CurveIndex);

			// delete this index from all poses
   // 从所有姿势中删除该索引
			for (int32 PoseIndex = 0; PoseIndex < Poses.Num(); ++PoseIndex)
			{
				Poses[PoseIndex].CurveData.RemoveAt(CurveIndex);
				Poses[PoseIndex].SourceCurveData.RemoveAt(CurveIndex);
			}

			return true;
		}
	}

	return false;
}

void FPoseDataContainer::RetrieveSourcePoseFromExistingPose(bool bAdditive, int32 InBasePoseIndex, const TArray<FTransform>& InBasePose, const TArray<float>& InBaseCurve)
{
	for (int32 PoseIndex = 0; PoseIndex < Poses.Num(); ++PoseIndex)
	{
		FPoseData& PoseData = Poses[PoseIndex];

		// if this pose is not base pose
  // 如果这个姿势不是基本姿势
		if (bAdditive && PoseIndex != InBasePoseIndex)
		{
			PoseData.SourceLocalSpacePose.Reset(InBasePose.Num());
			PoseData.SourceLocalSpacePose.AddUninitialized(InBasePose.Num());

			PoseData.SourceCurveData.Reset(InBaseCurve.Num());
			PoseData.SourceCurveData.AddUninitialized(InBaseCurve.Num());

			// should it be move? Why? I need that buffer still
   // 应该移动吗？为什么？我仍然需要那个缓冲区
			TArray<FTransform> AdditivePose = PoseData.LocalSpacePose;
			const ScalarRegister AdditiveWeight(1.f);

			check(AdditivePose.Num() == InBasePose.Num());
			for (int32 BoneIndex = 0; BoneIndex < AdditivePose.Num(); ++BoneIndex)
			{
				PoseData.SourceLocalSpacePose[BoneIndex] = InBasePose[BoneIndex];
				PoseData.SourceLocalSpacePose[BoneIndex].AccumulateWithAdditiveScale(AdditivePose[BoneIndex], AdditiveWeight);
			}

			int32 CurveNum = Curves.Num();
			checkSlow(CurveNum == PoseData.CurveData.Num());
			for (int32 CurveIndex = 0; CurveIndex < CurveNum; ++CurveIndex)
			{
				PoseData.SourceCurveData[CurveIndex] = InBaseCurve[CurveIndex] + PoseData.CurveData[CurveIndex];
			}
		}
		else
		{
			// otherwise, the base pose is the one
   // 否则，基本姿势就是那个
			PoseData.SourceLocalSpacePose = PoseData.LocalSpacePose;
			PoseData.SourceCurveData = PoseData.CurveData;
		}
	}
}

// this marks dirty tracks for each pose 
// 这标记了每个姿势的脏轨迹
void FPoseDataContainer::ConvertToFullPose(USkeleton* InSkeleton, const TArray<FTransform>& RefPose)
{
	TrackPoseInfluenceIndices.Reset();
	TrackPoseInfluenceIndices.SetNum(Tracks.Num());
	
	// first create pose buffer that only has valid data
 // 首先创建仅包含有效数据的姿势缓冲区
	for (int32 PoseIndex = 0; PoseIndex < Poses.Num(); ++PoseIndex)
	{
		FPoseData& Pose = Poses[PoseIndex];
		check(Pose.SourceLocalSpacePose.Num() == Tracks.Num());
		Pose.LocalSpacePose.Reset();
		if (InSkeleton)
		{
			for (int32 TrackIndex = 0; TrackIndex < Tracks.Num(); ++TrackIndex)
			{
				// we only add to local space poses if it's not same as default pose
    // 如果局部空间姿势与默认姿势不同，我们只添加它
				FTransform DefaultTransform = GetDefaultTransform(Tracks[TrackIndex], InSkeleton, RefPose);
				if (!Pose.SourceLocalSpacePose[TrackIndex].Equals(DefaultTransform, UE_KINDA_SMALL_NUMBER))
				{
					int32 NewIndex = Pose.LocalSpacePose.Add(Pose.SourceLocalSpacePose[TrackIndex]);
					TrackPoseInfluenceIndices[TrackIndex].Influences.Emplace(FPoseAssetInfluence{PoseIndex, NewIndex});
				}
			}
		}

		// for now we just copy curve directly
  // 现在我们直接复制曲线
		Pose.CurveData = Pose.SourceCurveData;
	}
}

void FPoseDataContainer::ConvertToAdditivePose(const TArray<FTransform>& InBasePose, const TArray<float>& InBaseCurve)
{
	check(InBaseCurve.Num() == Curves.Num());
	const FTransform AdditiveIdentity(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);

	TrackPoseInfluenceIndices.Reset();
	TrackPoseInfluenceIndices.SetNum(Tracks.Num());
	
	for (int32 PoseIndex = 0; PoseIndex < Poses.Num(); ++PoseIndex)
	{
		FPoseData& PoseData = Poses[PoseIndex];
		// set up buffer
  // 设置缓冲区
		PoseData.LocalSpacePose.Reset();
		PoseData.CurveData.Reset(PoseData.SourceCurveData.Num());
		PoseData.CurveData.AddUninitialized(PoseData.SourceCurveData.Num());

		check(PoseData.SourceLocalSpacePose.Num() == InBasePose.Num());
		for (int32 BoneIndex = 0; BoneIndex < InBasePose.Num(); ++BoneIndex)
		{
			// we only add to local space poses if it has any changes in additive
   // 如果附加的有任何变化，我们只添加到局部空间姿势
			FTransform NewTransform = PoseData.SourceLocalSpacePose[BoneIndex];
			FAnimationRuntime::ConvertTransformToAdditive(NewTransform, InBasePose[BoneIndex]);
			if (!NewTransform.Equals(AdditiveIdentity))
			{
				const int32 Index = PoseData.LocalSpacePose.Add(NewTransform);
				TrackPoseInfluenceIndices[BoneIndex].Influences.Emplace(FPoseAssetInfluence{PoseIndex, Index});
			}
		}

		int32 CurveNum = Curves.Num();
		checkSlow(CurveNum == PoseData.CurveData.Num());
		for (int32 CurveIndex = 0; CurveIndex < CurveNum; ++CurveIndex)
		{
			PoseData.CurveData[CurveIndex] = PoseData.SourceCurveData[CurveIndex] - InBaseCurve[CurveIndex];
		}
	}
}
#endif // WITH_EDITOR

void FPoseDataContainer::RebuildCurveIndexTable()
{
	// Recreate sorted curve index table
 // 重新创建排序曲线索引表
	SortedCurveIndices.SetNumUninitialized(Curves.Num());
	for(int32 NameIndex = 0; NameIndex < SortedCurveIndices.Num(); ++NameIndex)
	{
		SortedCurveIndices[NameIndex] = NameIndex;
	}

	SortedCurveIndices.Sort([&Curves = Curves](int32 LHS, int32 RHS)
	{
		return Curves[LHS].GetName().FastLess(Curves[RHS].GetName());
	});
}
/////////////////////////////////////////////////////
// UPoseAsset
// U姿势资产
/////////////////////////////////////////////////////
UPoseAsset::UPoseAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bAdditivePose(false)
	, BasePoseIndex(-1)
{
}

/**
 * Local utility struct that keeps skeleton bone index and compact bone index together for retargeting
 */
struct FBoneIndices
{
	int32 SkeletonBoneIndex;
	FCompactPoseBoneIndex CompactBoneIndex;

	FBoneIndices(int32 InSkeletonBoneIndex, FCompactPoseBoneIndex InCompactBoneIndex)
		: SkeletonBoneIndex(InSkeletonBoneIndex)
		, CompactBoneIndex(InCompactBoneIndex)
	{}
};

struct FPoseAssetEvalData : public TThreadSingleton<FPoseAssetEvalData>
{
	TArray<FBoneIndices> BoneIndices;
	TArray<int32> PoseWeightedIndices;
	TArray<float> PoseWeights;
	TArray<bool> WeightedPoses;
};

void UPoseAsset::GetBaseAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& OutCurve) const
{
	UE::Anim::FStackAttributeContainer TempAttributes;
	FAnimationPoseData OutPoseData(OutPose, OutCurve, TempAttributes);
	GetBaseAnimationPose(OutPoseData);
}

void UPoseAsset::GetBaseAnimationPose(FAnimationPoseData& OutAnimationPoseData) const
{
	FCompactPose& OutPose = OutAnimationPoseData.GetPose();
	if (bAdditivePose && PoseContainer.Poses.IsValidIndex(BasePoseIndex))
	{
		FBlendedCurve& OutCurve = OutAnimationPoseData.GetCurve();

		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
		USkeleton* MySkeleton = GetSkeleton();

		OutPose.ResetToRefPose();

		// this contains compact bone pose list that this pose cares
  // 这包含该姿势关心的紧凑骨骼姿势列表
		FPoseAssetEvalData& EvalData = FPoseAssetEvalData::Get();
		TArray<FBoneIndices>& BoneIndices = EvalData.BoneIndices;
        const int32 TrackNum = PoseContainer.Tracks.Num();
		BoneIndices.SetNumUninitialized(TrackNum, EAllowShrinking::No);

		const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(GetSkeleton(), RequiredBones.GetSkeletonAsset());
		for(int32 TrackIndex = 0; TrackIndex < TrackNum; ++TrackIndex)
		{
			const int32 SkeletonBoneIndex = SkeletonRemapping.IsValid() ? SkeletonRemapping.GetTargetSkeletonBoneIndex(PoseContainer.TrackBoneIndices[TrackIndex]) : PoseContainer.TrackBoneIndices[TrackIndex];
			const FCompactPoseBoneIndex PoseBoneIndex = RequiredBones.GetCompactPoseIndexFromSkeletonIndex(SkeletonBoneIndex);
			// we add even if it's invalid because we want it to match with track index
   // 即使它无效我们也会添加，因为我们希望它与轨道索引匹配
			BoneIndices[TrackIndex].SkeletonBoneIndex = SkeletonBoneIndex;
			BoneIndices[TrackIndex].CompactBoneIndex = PoseBoneIndex;
		}

		const TArray<FTransform>& PoseTransform = PoseContainer.Poses[BasePoseIndex].LocalSpacePose;

		for (int32 TrackIndex = 0; TrackIndex < TrackNum; ++TrackIndex)
		{
			const FBoneIndices& LocalBoneIndices = BoneIndices[TrackIndex];

			if (LocalBoneIndices.CompactBoneIndex != INDEX_NONE)
			{
				FTransform& OutTransform = OutPose[LocalBoneIndices.CompactBoneIndex];
				OutTransform = PoseTransform[TrackIndex];
				FAnimationRuntime::RetargetBoneTransform(MySkeleton, GetRetargetTransformsSourceName(), GetRetargetTransforms(), OutTransform, LocalBoneIndices.SkeletonBoneIndex, LocalBoneIndices.CompactBoneIndex, RequiredBones, false);
			}
		}

		PoseContainer.GetPoseCurve(&PoseContainer.Poses[BasePoseIndex], OutCurve);
	}
	else
	{
		OutPose.ResetToRefPose();
	}
}

/*
 * The difference between BlendFromIdentityAndAccumulcate is scale
 * This ADDS scales to the FinalAtom. We use additive identity as final atom, so can't use
 */
FORCEINLINE void BlendFromIdentityAndAccumulateAdditively_Custom(FTransform& FinalAtom, const FTransform& SourceAtom, float BlendWeight)
{
	const  FTransform AdditiveIdentity(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);

	FTransform Delta = SourceAtom;
	// Scale delta by weight
 // 按重量缩放增量
	if (BlendWeight < (1.f - ZERO_ANIMWEIGHT_THRESH))
	{
		Delta.Blend(AdditiveIdentity, Delta, BlendWeight);
	}

	FinalAtom.SetRotation(Delta.GetRotation() * FinalAtom.GetRotation());
	FinalAtom.SetTranslation(FinalAtom.GetTranslation() + Delta.GetTranslation());
	// this ADDS scale
 // 这个 ADDS 量表
	FinalAtom.SetScale3D(FinalAtom.GetScale3D() + Delta.GetScale3D());

	FinalAtom.DiagnosticCheckNaN_All();

	FinalAtom.NormalizeRotation();
}

void UPoseAsset::GetAnimationCurveOnly(TArray<FName>& InCurveNames, TArray<float>& InCurveValues, TArray<FName>& OutCurveNames, TArray<float>& OutCurveValues) const
{
	// if we have any pose curve
 // 如果我们有任何姿态曲线
	if (ensure(InCurveNames.Num() == InCurveValues.Num()) && InCurveNames.Num() > 0)
	{
		USkeleton* MySkeleton = GetSkeleton();
		check(MySkeleton);

		FPoseAssetEvalData& EvalData = FPoseAssetEvalData::Get();
		const int32 NumPoses = PoseContainer.Poses.Num();
		TArray<float>& PoseWeights = EvalData.PoseWeights;		
		PoseWeights.Reset();
		PoseWeights.SetNumZeroed(NumPoses, EAllowShrinking::No);

		TArray<int32>& WeightedPoseIndices = EvalData.PoseWeightedIndices;	
		WeightedPoseIndices.Reset();

		bool bNormalizeWeight = bAdditivePose == false;
		float TotalWeight = 0.f;
		// we iterate through to see if we have that corresponding pose
  // 我们迭代看看是否有相应的姿势
		for (int32 CurveIndex = 0; CurveIndex < InCurveNames.Num(); ++CurveIndex)
		{
			int32 PoseIndex = PoseContainer.PoseFNames.Find(InCurveNames[CurveIndex]);
			if (ensure(PoseIndex != INDEX_NONE))
			{
				const FPoseData& PoseData = PoseContainer.Poses[PoseIndex];
				const float Value = InCurveValues[CurveIndex];

				// we only add to the list if it's not additive Or if it's additive, we don't want to add base pose index
    // 如果它不是可加的，我们只添加到列表中或者如果它是可加的，我们不想添加基本姿势索引
				// and has weight
    // 并且有重量
				if ((!bAdditivePose || PoseIndex != BasePoseIndex) && FAnimationRuntime::HasWeight(Value))
				{
					TotalWeight += Value;												
					PoseWeights[PoseIndex] = Value;
					WeightedPoseIndices.Add(PoseIndex);
				}
			}
		}

		const int32 TotalNumberOfValidPoses = WeightedPoseIndices.Num();
		if (TotalNumberOfValidPoses > 0)
		{
			// blend curves
   // 混合曲线
			FBlendedCurve BlendedCurve;

			//if full pose, we'll have to normalize by weight
   // 如果是完整姿势，我们必须按体重标准化
			if (bNormalizeWeight && TotalWeight > 1.f)
			{
				for (const int32& WeightedPoseIndex : WeightedPoseIndices)
				{
					float& PoseWeight = PoseWeights[WeightedPoseIndex];
					PoseWeight /= TotalWeight;

					const FPoseData& Pose = PoseContainer.Poses[WeightedPoseIndex];
					PoseContainer.BlendPoseCurve(&Pose, BlendedCurve, PoseWeight);
				}
			}
			else
			{
				for (const int32& WeightedPoseIndex : WeightedPoseIndices)
				{				
					const FPoseData& Pose = PoseContainer.Poses[WeightedPoseIndex];
					const float& PoseWeight = PoseWeights[WeightedPoseIndex];

					PoseContainer.BlendPoseCurve(&Pose, BlendedCurve, PoseWeight);
				}
			}

			OutCurveNames.Reset();
			OutCurveValues.Reset();

			BlendedCurve.ForEachElement([&OutCurveNames, &OutCurveValues](const UE::Anim::FCurveElement& InElement)
			{
				OutCurveNames.Add(InElement.Name);
				OutCurveValues.Add(InElement.Value);
			});
		}
	}
}

bool UPoseAsset::GetAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const
{
	UE::Anim::FStackAttributeContainer TempAttributes;
	FAnimationPoseData OutPoseData(OutPose, OutCurve, TempAttributes);
	return GetAnimationPose(OutPoseData, ExtractionContext);
}

bool UPoseAsset::GetAnimationPose(struct FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const
{
	ANIM_MT_SCOPE_CYCLE_COUNTER(PoseAssetGetAnimationPose, !IsInGameThread());

	// if we have any pose curve
 // 如果我们有任何姿态曲线
	if (ExtractionContext.PoseCurves.Num() > 0)
	{
		FCompactPose& OutPose = OutAnimationPoseData.GetPose();
		FBlendedCurve& OutCurve = OutAnimationPoseData.GetCurve();

		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
		USkeleton* MySkeleton = GetSkeleton();
		
		check(PoseContainer.IsValid());

		if (bAdditivePose)
		{
			OutPose.ResetToAdditiveIdentity();
		}
		else
		{
			OutPose.ResetToRefPose();
		}

		const int32 TrackNum = PoseContainer.Tracks.Num();

		const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(GetSkeleton(), RequiredBones.GetSkeletonAsset());
		// Single pose optimized evaluation path - explicitly used by PoseByName Animation Node
  // 单姿势优化评估路径 - 由 PoseByName 动画节点显式使用
		if (ExtractionContext.PoseCurves.Num() == 1)
		{
			const FPoseCurve& Curve = ExtractionContext.PoseCurves[0];
			const int32 PoseIndex = Curve.PoseIndex; 
			if (ensure(PoseIndex != INDEX_NONE))
			{
				const FPoseData& Pose = PoseContainer.Poses[PoseIndex];
				// Clamp weight for non-additive pose assets rather than normalizing the weight
    // 钳制非附加姿势资产的权重而不是标准化权重
				const float Weight = bAdditivePose ? Curve.Value : FMath::Clamp(Curve.Value, 0.f, 1.f);

				// Only generate pose if the single weight is actually relevant
    // 仅当单个权重实际相关时才生成姿势
				if(FAnimWeight::IsRelevant(Weight))
				{
					// Blend curve
     // 混合曲线
					PoseContainer.BlendPoseCurve(&Pose, OutCurve, Weight);

					// Per-track (bone) transform
     // 每轨（骨骼）变换
					for (int32 TrackIndex = 0; TrackIndex < TrackNum; ++TrackIndex)
					{
						const FSkeletonPoseBoneIndex SkeletonBoneIndex = FSkeletonPoseBoneIndex(SkeletonRemapping.IsValid() ? SkeletonRemapping.GetTargetSkeletonBoneIndex(PoseContainer.TrackBoneIndices[TrackIndex]) : PoseContainer.TrackBoneIndices[TrackIndex]);
						const FCompactPoseBoneIndex CompactIndex = RequiredBones.GetCompactPoseIndexFromSkeletonPoseIndex(SkeletonBoneIndex);
						
						// If bone index is invalid, or not required for the pose - skip
      // 如果骨骼索引无效，或者姿势不需要 - 跳过
						if (!CompactIndex.IsValid() || !ExtractionContext.IsBoneRequired(CompactIndex.GetInt()))
						{
							continue;
						}
					
						// Check if this track is part of the pose
      // 检查该轨迹是否是姿势的一部分
						const TArray<FPoseAssetInfluence>& PoseInfluences = PoseContainer.TrackPoseInfluenceIndices[TrackIndex].Influences;
						const int32 InfluenceIndex = PoseInfluences.IndexOfByPredicate([PoseIndex](const FPoseAssetInfluence& Influence) -> bool
						{
							return Influence.PoseIndex == PoseIndex;
						});

						if (InfluenceIndex != INDEX_NONE)
						{
							FTransform& OutBoneTransform =  OutPose[CompactIndex];
							const FPoseAssetInfluence& Influence = PoseInfluences[InfluenceIndex];
							const int32& BonePoseIndex = Influence.BoneTransformIndex;

							const FPoseData& PoseData = PoseContainer.Poses[PoseIndex];
							const FTransform& BonePose = PoseData.LocalSpacePose[BonePoseIndex];

							// Apply additive, overriede or blend using pose weight
       // 使用姿势权重应用添加剂、覆盖或混合
							if (bAdditivePose)
							{
								BlendFromIdentityAndAccumulateAdditively_Custom(OutBoneTransform, BonePose, Weight);
							}
							else if (FAnimWeight::IsFullWeight(Weight))
							{
								OutBoneTransform = BonePose;
							}
							else
							{
								OutBoneTransform = OutBoneTransform * ScalarRegister( 1 - Weight);
								OutBoneTransform.AccumulateWithShortestRotation(BonePose, ScalarRegister(Weight));
							}

							// Retarget the bone transform
       // 重新定位骨骼变换
							FAnimationRuntime::RetargetBoneTransform(MySkeleton, GetRetargetTransformsSourceName(), GetRetargetTransforms(), OutBoneTransform, SkeletonBoneIndex.GetInt(), CompactIndex, RequiredBones, bAdditivePose);
							OutBoneTransform.NormalizeRotation();
						}
					}					
					
					return true;
				}
			}
		}
		else
		{
			// TLS storage for working data
   // 工作数据的 TLS 存储
			FPoseAssetEvalData& EvalData = FPoseAssetEvalData::Get();

			// this contains compact bone pose list that this pose cares
   // 这包含该姿势关心的紧凑骨骼姿势列表
			TArray<FBoneIndices>& BoneIndices = EvalData.BoneIndices;
			BoneIndices.SetNumUninitialized(TrackNum, EAllowShrinking::No);

			for(int32 TrackIndex = 0; TrackIndex < TrackNum; ++TrackIndex)
			{
				const FSkeletonPoseBoneIndex SkeletonBoneIndex = FSkeletonPoseBoneIndex(SkeletonRemapping.IsValid() ? SkeletonRemapping.GetTargetSkeletonBoneIndex(PoseContainer.TrackBoneIndices[TrackIndex]) : PoseContainer.TrackBoneIndices[TrackIndex]);
				const FCompactPoseBoneIndex CompactIndex = RequiredBones.GetCompactPoseIndexFromSkeletonPoseIndex(SkeletonBoneIndex);

				// we add even if it's invalid because we want it to match with track index
    // 即使它无效我们也会添加，因为我们希望它与轨道索引匹配
				BoneIndices[TrackIndex].SkeletonBoneIndex = SkeletonBoneIndex.GetInt();
				BoneIndices[TrackIndex].CompactBoneIndex = CompactIndex;
			}

			// you could only have morphtargets
   // 你只能有变形目标
			// so can't return here yet when bone indices is empty
   // 所以当骨骼索引为空时还无法返回这里
			const bool bNormalizeWeight = bAdditivePose == false;
			const int32 NumPoses = PoseContainer.Poses.Num();
			TArray<float>& PoseWeights = EvalData.PoseWeights;
			PoseWeights.Reset();
			PoseWeights.SetNumZeroed(NumPoses, EAllowShrinking::No);

			TArray<int32>& WeightedPoseIndices = EvalData.PoseWeightedIndices;
			WeightedPoseIndices.Reset();

			TArray<bool>& WeightedPoses = EvalData.WeightedPoses;
			WeightedPoses.Reset();
			WeightedPoses.SetNumZeroed(NumPoses, EAllowShrinking::No);

			float TotalWeight = 0.f;
			// we iterate through to see if we have that corresponding pose
   // 我们迭代看看是否有相应的姿势

			const int32 NumPoseCurves = ExtractionContext.PoseCurves.Num();
			for (int32 CurveIndex = 0; CurveIndex < NumPoseCurves; ++CurveIndex)
			{
				const FPoseCurve& Curve = ExtractionContext.PoseCurves[CurveIndex];
				const int32 PoseIndex = Curve.PoseIndex; 
				if (ensure(PoseIndex != INDEX_NONE))
				{
					const FPoseData& PoseData = PoseContainer.Poses[PoseIndex];
					const float Value = Curve.Value;

					// we only add to the list if it's not additive Or if it's additive, we don't want to add base pose index
     // 如果它不是可加的，我们只添加到列表中或者如果它是可加的，我们不想添加基本姿势索引
					// and has weight
     // 并且有重量
					if ((!bAdditivePose || PoseIndex != BasePoseIndex) && FAnimationRuntime::HasWeight(Value))
					{
						TotalWeight += Value;

						// Set pose weight and bit, and add weighted pose index
      // 设置姿势权重和位，并添加加权姿势索引
						PoseWeights[PoseIndex] = Value;
						WeightedPoseIndices.Add(PoseIndex);
						WeightedPoses[PoseIndex] = true;
					}
				}
			}

			const int32 TotalNumberOfValidPoses = WeightedPoseIndices.Num();
			if (TotalNumberOfValidPoses > 0)
			{
				//if full pose, we'll have to normalize by weight
    // 如果是完整姿势，我们必须按体重标准化
				if (bNormalizeWeight && TotalWeight > 1.f)
				{
					for (const int32& WeightedPoseIndex : WeightedPoseIndices)
					{
						float& PoseWeight = PoseWeights[WeightedPoseIndex];
						PoseWeight /= TotalWeight;

						// Do curve blending inline as we are looping over weights anyway
      // 无论如何，当我们循环权重时，进行内联曲线混合
						const FPoseData& Pose = PoseContainer.Poses[WeightedPoseIndex];
						PoseContainer.BlendPoseCurve(&Pose, OutCurve, PoseWeight);
					}
				}
				else
				{
					// Take the matching curve weights from the selected poses, and blend them using the
     // 从选定的姿势中获取匹配的曲线权重，并使用
					// weighting that we need from each pose. This is much faster than grabbing each
     // 我们需要从每个姿势中获得权重。这比抓取每个要快得多
					// blend curve and blending them in their entirety, especially when there are very
     // 混合曲线并将它们完全混合，特别是当有很多
					// few active curves for each pose and many curves for the entire pose asset.
     // 每个姿势很少有活动曲线，整个姿势资源有很多曲线。
					for (const int32& WeightedPoseIndex : WeightedPoseIndices)
					{
						const FPoseData& Pose = PoseContainer.Poses[WeightedPoseIndex];
						const float& Weight = PoseWeights[WeightedPoseIndex];
						PoseContainer.BlendPoseCurve(&Pose, OutCurve, Weight);
					}
				}

				// Final per-track (bone) transform
    // 最终每轨（骨骼）变换
				FTransform OutBoneTransform;
				for (int32 TrackIndex = 0; TrackIndex < TrackNum; ++TrackIndex)
				{
					const FCompactPoseBoneIndex& CompactIndex = BoneIndices[TrackIndex].CompactBoneIndex;

					// If bone index is invalid, or not required for the pose - skip
     // 如果骨骼索引无效，或者姿势不需要 - 跳过
					if (CompactIndex == INDEX_NONE || !ExtractionContext.IsBoneRequired(CompactIndex.GetInt()))
					{
						continue;
					}
					
					const TArray<FPoseAssetInfluence>& PoseInfluences = PoseContainer.TrackPoseInfluenceIndices[TrackIndex].Influences;

					// When additive, or for any bone that has no pose influences. Set transform to input.
     // 当添加时，或对于任何没有姿势影响的骨骼。将变换设置为输入。
					if (bAdditivePose || PoseInfluences.Num() == 0)
					{
						OutBoneTransform = OutPose[CompactIndex];
					}

					const int32 NumInfluences = PoseInfluences.Num();
					if (NumInfluences)
					{
						float TotalLocalWeight = 0.f;
						bool bSet = false;
						// Only loop over poses known to influence this track its final transform
      // 仅循环已知会影响该轨道最终变换的姿势
						for (int32 Index = 0; Index < NumInfluences; ++Index)
						{
							const FPoseAssetInfluence& Influence = PoseInfluences[Index];
							const int32& PoseIndex = Influence.PoseIndex;
							const int32& BonePoseIndex = Influence.BoneTransformIndex;
							
							// Only processs pose if its weighted
       // 只有加权的过程才会构成
							if(WeightedPoses[PoseIndex])
							{
								const float& Weight = PoseWeights[PoseIndex];
								TotalLocalWeight += Weight;
							
								const FPoseData& PoseData = PoseContainer.Poses[PoseIndex];
								const FTransform& BonePose = PoseData.LocalSpacePose[BonePoseIndex];

								// Set weighted value For first pose, if applicable
        // 设置第一个姿势的权重值（如果适用）
								if(!bSet && !bAdditivePose)
								{
									OutBoneTransform = BonePose * ScalarRegister(Weight);
									bSet = true;
								}
								else
								{
									if (bAdditivePose)
									{
										BlendFromIdentityAndAccumulateAdditively_Custom(OutBoneTransform, BonePose, Weight);
									}
									else
									{
										OutBoneTransform.AccumulateWithShortestRotation(BonePose, ScalarRegister(Weight));
									}
								}
							}
						}

						// In case no influencing poses had any weight, set transform to input
      // 如果没有影响姿势有任何权重，请将变换设置为输入
						if(!FAnimWeight::IsRelevant(TotalLocalWeight))
						{
							OutBoneTransform = OutPose[CompactIndex];
						}
						else if (!FAnimWeight::IsFullWeight(TotalLocalWeight) && !bAdditivePose)
						{
							OutBoneTransform.AccumulateWithShortestRotation(OutPose[CompactIndex], ScalarRegister(1.f - TotalLocalWeight));
						}
					}

					// Retarget the blended transform, and copy to output pose
     // 重新定位混合变换，并复制到输出姿势
					FAnimationRuntime::RetargetBoneTransform(MySkeleton, GetRetargetTransformsSourceName(), GetRetargetTransforms(), OutBoneTransform,  BoneIndices[TrackIndex].SkeletonBoneIndex, CompactIndex, RequiredBones, bAdditivePose);

					OutPose[CompactIndex] = OutBoneTransform;
					OutPose[CompactIndex].NormalizeRotation();
				}

				return true;
			}
		}		
	}

	return false;
}

bool UPoseAsset::IsPostLoadThreadSafe() const
{
	return WITH_EDITORONLY_DATA == 0;	// Not thread safe in editor as the skeleton can be modified on version upgrade
}

void UPoseAsset::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	// moved to PostLoad because Skeleton is not completely loaded when we do this in Serialize
 // 移至 PostLoad，因为当我们在 Serialize 中执行此操作时，Skeleton 尚未完全加载
	// and we need Skeleton
 // 我们需要骷髅
	if (GetLinkerCustomVersion(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::PoseAssetSupportPerBoneMask && GetLinkerCustomVersion(FAnimPhysObjectVersion::GUID) >= FAnimPhysObjectVersion::SaveEditorOnlyFullPoseForPoseAsset)
	{
		// fix curve names
  // 修复曲线名称
		// copy to source local data FIRST
  // 首先复制到源本地数据
		for (FPoseData& Pose : PoseContainer.Poses)
		{
			Pose.SourceCurveData = Pose.CurveData;
			Pose.SourceLocalSpacePose = Pose.LocalSpacePose;
		}

		PostProcessData();
	}

	if (GetLinkerCustomVersion(FAnimPhysObjectVersion::GUID) < FAnimPhysObjectVersion::SaveEditorOnlyFullPoseForPoseAsset)
	{
		TArray<FTransform>	BasePose;
		TArray<float>		BaseCurves;
		// since the code change, the LocalSpacePose will have to be copied here manually
  // 由于代码更改，必须手动将 LocalSpacePose 复制到此处
		// RemoveUnnecessaryTracksFromPose removes LocalSpacePose data, so we're not using it for getting base pose
  // RemoveUnnecessaryTracksFromPose 删除 LocalSpacePose 数据，因此我们不使用它来获取基本姿势
		if (PoseContainer.Poses.IsValidIndex(BasePoseIndex))
		{
			BasePose = PoseContainer.Poses[BasePoseIndex].LocalSpacePose;
			BaseCurves = PoseContainer.Poses[BasePoseIndex].CurveData;
			check(BasePose.Num() == PoseContainer.Tracks.Num());
		}
		else
		{
			GetBasePoseTransform(BasePose, BaseCurves);
		}

		PoseContainer.RetrieveSourcePoseFromExistingPose(bAdditivePose, GetBasePoseIndex(), BasePose, BaseCurves);
	}

	if (GetLinkerCustomVersion(FFrameworkObjectVersion::GUID) >= FFrameworkObjectVersion::PoseAssetSupportPerBoneMask &&
		GetLinkerCustomVersion(FFortniteMainBranchObjectVersion::GUID) < FFortniteMainBranchObjectVersion::RemoveUnnecessaryTracksFromPose)
	{
		// fix curve names
  // 修复曲线名称
		PostProcessData();
	}

  	if(GetLinkerCustomVersion(FUE5ReleaseStreamObjectVersion::GUID) < FUE5ReleaseStreamObjectVersion::PoseAssetRuntimeRefactor)
    {
		PostProcessData();
    }

	if (GetLinkerCustomVersion(FUE5MainStreamObjectVersion::GUID) >= FUE5MainStreamObjectVersion::PoseAssetRawDataGUIDUpdate)
	{
		if (SourceAnimation)
		{
			// Fully load the source animation to ensure its RawDataGUID is populated
   // 完全加载源动画以确保填充其 RawDataGUID
			SourceAnimation->ConditionalPreload();
			SourceAnimation->ConditionalPostLoad();

			const FGuid DataModelGuid = GetSourceAnimationGuid();
			if (SourceAnimationRawDataGUID.IsValid() && SourceAnimationRawDataGUID != DataModelGuid)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("AssetName"), FText::FromString(GetPathName()));
				Args.Add(TEXT("SourceAsset"), FText::FromString(SourceAnimation->GetPathName()));

				Args.Add(TEXT("Stored"), FText::FromString(SourceAnimationRawDataGUID.ToString()));
				Args.Add(TEXT("Found"), FText::FromString(DataModelGuid.ToString()));
				
				const FText ResultText = FText::Format(LOCTEXT("PoseAssetSourceOutOfDate", "PoseAsset {AssetName} is out-of-date with its source animation {SourceAsset} {Stored} vs {Found}"), Args);
				UE_LOG(LogAnimation, Warning,TEXT("%s"), *ResultText.ToString());
			}
		}	
	}	

	// fix curve names
 // 修复曲线名称
	USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton)
	{
		// double loop but this check only should happen once per asset
  // 双循环，但每个资产只应进行一次此检查
		// this should continue to add if skeleton hasn't been saved either 
  // 如果骨架也没有被保存，这应该继续添加
		if (GetLinkerCustomVersion(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::MoveCurveTypesToSkeleton 
			|| MySkeleton->GetLinkerCustomVersion(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::MoveCurveTypesToSkeleton)
		{
			// fix up curve flags to skeleton
   // 将曲线标志修复到骨架
			for (FAnimCurveBase& Curve : PoseContainer.Curves)
			{
				bool bMorphtargetSet = Curve.GetCurveTypeFlag(AACF_DriveMorphTarget_DEPRECATED);
				bool bMaterialSet = Curve.GetCurveTypeFlag(AACF_DriveMaterial_DEPRECATED);

				// only add this if that has to 
    // 仅在必须时添加此内容
				if (bMorphtargetSet || bMaterialSet)
				{
					MySkeleton->AccumulateCurveMetaData(Curve.GetName(), bMaterialSet, bMorphtargetSet);
				}
			}
		}
	}

	// I have to fix pose names
 // 我必须修复姿势名称
	if(RemoveInvalidTracks())
	{
		PostProcessData();
	}
	else
#endif // WITH_EDITORONLY_DATA
	{
		UpdateTrackBoneIndices();
	}
}

void UPoseAsset::Serialize(FArchive& Ar)
{
 	Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);
	Ar.UsingCustomVersion(FAnimPhysObjectVersion::GUID);
	Ar.UsingCustomVersion(FFortniteMainBranchObjectVersion::GUID);
	Ar.UsingCustomVersion(FUE5ReleaseStreamObjectVersion::GUID);

	Super::Serialize(Ar);
}

void UPoseAsset::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
#if WITH_EDITOR
	if (!ObjectSaveContext.IsProceduralSave())
	{
		UpdateRetargetSourceAssetData();
	}
#endif // WITH_EDITOR
	Super::PreSave(ObjectSaveContext);
}

void UPoseAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS;
	Super::GetAssetRegistryTags(OutTags);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS;
}

void UPoseAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);

	// Number of poses
 // 姿势数
	Context.AddTag(FAssetRegistryTag("Poses", FString::FromInt(GetNumPoses()), FAssetRegistryTag::TT_Numerical));
#if WITH_EDITOR
	TArray<FName> Names;
	Names.Reserve(PoseContainer.PoseFNames.Num() + PoseContainer.Curves.Num());

	for (const FName& PoseName : PoseContainer.PoseFNames)
	{
		Names.Add(PoseName);
	}

	for (const FAnimCurveBase& Curve : PoseContainer.Curves)
	{
		Names.AddUnique(Curve.GetName());
	}
	
	// Add curve IDs to a tag list, or a delimiter if we have no curves.
 // 将曲线 ID 添加到标签列表，如果没有曲线，则添加分隔符。
	// The delimiter is necessary so we can distinguish between data with no curves and old data, as the asset registry
 // 分隔符是必要的，这样我们就可以区分没有曲线的数据和旧数据，就像资产注册表一样
	// strips tags that have empty values 
 // 删除具有空值的标签
	FString PoseNameList = USkeleton::CurveTagDelimiter;
	for(const FName& Name : Names)
	{
		PoseNameList += FString::Printf(TEXT("%s%s"), *Name.ToString(), *USkeleton::CurveTagDelimiter);
	}
	Context.AddTag(FAssetRegistryTag(USkeleton::CurveNameTag, PoseNameList, FAssetRegistryTag::TT_Hidden)); //write pose names as curve tag as they use 
#endif
}

int32 UPoseAsset::GetNumPoses() const
{ 
	return PoseContainer.GetNumPoses();
}

int32 UPoseAsset::GetNumCurves() const
{
	return PoseContainer.Curves.Num();
}

int32 UPoseAsset::GetNumTracks() const
{
	return PoseContainer.Tracks.Num();
}

const TArray<FName>& UPoseAsset::GetPoseFNames() const
{
	return PoseContainer.PoseFNames;
}

const TArray<FSmartName> UPoseAsset::GetPoseNames() const
{
	return TArray<FSmartName>();
}

const TArray<FName>& UPoseAsset::GetTrackNames() const
{
	return PoseContainer.Tracks;
}

const TArray<FSmartName> UPoseAsset::GetCurveNames() const
{
	return TArray<FSmartName>();
}

const TArray<FName> UPoseAsset::GetCurveFNames() const
{
	TArray<FName> CurveNames;
	for (int32 CurveIndex = 0; CurveIndex < PoseContainer.Curves.Num(); ++CurveIndex)
	{
		CurveNames.Add(PoseContainer.Curves[CurveIndex].GetName());
	}

	return CurveNames;
}

const TArray<FAnimCurveBase>& UPoseAsset::GetCurveData() const
{
	return PoseContainer.Curves;
}

const TArray<float> UPoseAsset::GetCurveValues(const int32 PoseIndex) const
{
	TArray<float> ResultCurveData;

	if (PoseContainer.Poses.IsValidIndex(PoseIndex))
	{
		ResultCurveData = PoseContainer.Poses[PoseIndex].CurveData;
	}

	return ResultCurveData;
}

bool UPoseAsset::GetCurveValue(const int32 PoseIndex, const int32 CurveIndex, float& OutValue) const
{
	bool bSuccess = false;

	if (PoseContainer.Poses.IsValidIndex(PoseIndex))
	{
		const FPoseData& PoseData = PoseContainer.Poses[PoseIndex];
		if (PoseData.CurveData.IsValidIndex(CurveIndex))
		{
			OutValue = PoseData.CurveData[CurveIndex];
			bSuccess = true;
		}
	}

	return bSuccess;
}

const int32 UPoseAsset::GetTrackIndexByName(const FName& InTrackName) const
{
	int32 ResultTrackIndex = INDEX_NONE;

	// Only search if valid name passed in
 // 仅在传入有效名称时搜索
	if (InTrackName != NAME_None)
	{
		ResultTrackIndex = PoseContainer.Tracks.Find(InTrackName);
	}

	return ResultTrackIndex;
}


bool UPoseAsset::ContainsPose(const FName& InPoseName) const
{
	for (const FName& PoseName : PoseContainer.PoseFNames)
	{
		if (PoseName == InPoseName)
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
void UPoseAsset::RenamePose(const FName& OriginalPoseName, const FName& NewPoseName)
{
	ModifyPoseName(OriginalPoseName, NewPoseName);
}

FName UPoseAsset::GetBasePoseName() const
{
	return PoseContainer.PoseFNames.IsValidIndex(BasePoseIndex) ? PoseContainer.PoseFNames[BasePoseIndex] : NAME_None;
}

bool UPoseAsset::SetBasePoseName(const FName& NewBasePoseName)
{
	if (NewBasePoseName != NAME_None)
	{
		const int32 NewIndex = PoseContainer.PoseFNames.IndexOfByKey(NewBasePoseName);
		if (NewIndex != INDEX_NONE)
		{
			BasePoseIndex = NewIndex;
			return true;
		}
		else
		{
			return false;
		}
	}
	
	BasePoseIndex = INDEX_NONE;
	return true;
}

void UPoseAsset::GetPoseNames(TArray<FName>& PoseNames) const
{	
	const int32 NumPoses = GetNumPoses();
	for (int32 PoseIndex = 0; PoseIndex < NumPoses; ++PoseIndex)
	{
		PoseNames.Add(GetPoseNameByIndex(PoseIndex));
	}
}
// whenever you change SourceLocalPoses, or SourceCurves, we should call this to update runtime dataa
// 每当您更改 SourceLocalPoses 或 SourceCurves 时，我们都应该调用它来更新运行时数据a
void UPoseAsset::PostProcessData()
{
	RemoveInvalidTracks();
	
	// convert back to additive if it was that way
 // 如果是这样的话，转换回加法
	if (bAdditivePose)
	{
		ConvertToAdditivePose(GetBasePoseIndex());
	}
	else
	{
		ConvertToFullPose();
	}

	UpdateTrackBoneIndices();

	PoseContainer.RebuildCurveIndexTable();
}

void UPoseAsset::BreakAnimationSequenceGUIDComparison()
{
	SourceAnimationRawDataGUID.Invalidate();
}

FName UPoseAsset::AddPoseWithUniqueName(const USkeletalMeshComponent* MeshComponent)
{
	const FName NewPoseName = GetUniquePoseName(this);
	AddOrUpdatePose(NewPoseName, MeshComponent);

	PostProcessData();

	OnPoseListChanged.Broadcast();

	return NewPoseName;
}

void UPoseAsset::AddReferencePose(const FName& PoseName, const FReferenceSkeleton& RefSkeleton)
{
	TArray<FTransform> BoneTransforms;
	TArray<FName> TrackNames;

	const TArray<FTransform>& ReferencePose = RefSkeleton.GetRefBonePose();
	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		TrackNames.Add(RefSkeleton.GetBoneName(BoneIndex));
		BoneTransforms.Add(ReferencePose[BoneIndex]);
		//const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
  // const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
	}
			
	TArray<float> NewCurveValues;
	NewCurveValues.AddZeroed(PoseContainer.Curves.Num());

			//int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
   // int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
	AddOrUpdatePose(PoseName, TrackNames, BoneTransforms, NewCurveValues);
	PostProcessData();
}

void UPoseAsset::AddOrUpdatePose(const FName& PoseName, const USkeletalMeshComponent* MeshComponent, bool bUpdateCurves)
{
	const USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton && MeshComponent && MeshComponent->GetSkeletalMeshAsset())
	{
		TArray<FName> TrackNames;
		// note this ignores root motion
  // 请注意，这会忽略根运动
		TArray<FTransform> BoneTransform = MeshComponent->GetComponentSpaceTransforms();
		const FReferenceSkeleton& RefSkeleton = MeshComponent->GetSkeletalMeshAsset()->GetRefSkeleton();
		for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
		{
			TrackNames.Add(RefSkeleton.GetBoneName(BoneIndex));
		}

		// convert to local space
  // 转换为本地空间
		for (int32 BoneIndex = BoneTransform.Num() - 1; BoneIndex >= 0; --BoneIndex)
		{
			const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
			if (ParentIndex != INDEX_NONE)
			{
				BoneTransform[BoneIndex] = BoneTransform[BoneIndex].GetRelativeTransform(BoneTransform[ParentIndex]);
			}
		}
		
		TArray<float> NewCurveValues;
		NewCurveValues.AddZeroed(PoseContainer.Curves.Num());

		const FBlendedHeapCurve& MeshCurves = MeshComponent->GetAnimationCurves();

		for (int32 NewCurveIndex = 0; NewCurveIndex < NewCurveValues.Num(); ++NewCurveIndex)
		{
			const FAnimCurveBase& Curve = PoseContainer.Curves[NewCurveIndex];
			const float MeshCurveValue = MeshCurves.Get(Curve.GetName());
			NewCurveValues[NewCurveIndex] = MeshCurveValue;
		}
		
		BreakAnimationSequenceGUIDComparison();

		// Only update curves if user has requested so - or when setting up a new pose
  // 仅在用户请求时或设置新姿势时更新曲线
		const FPoseData* PoseData = PoseContainer.FindPoseData(PoseName);
		AddOrUpdatePose(PoseName, TrackNames, BoneTransform, (PoseData && !bUpdateCurves) ? PoseData->CurveData : NewCurveValues);
		PostProcessData();
	}
}

void UPoseAsset::AddOrUpdatePose(const FName& PoseName, const TArray<FName>& TrackNames, const TArray<FTransform>& LocalTransform, const TArray<float>& CurveValues)
{
	const USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton)
	{
		// first combine track, we want to make sure all poses contains tracks with this
  // 首先组合轨迹，我们要确保所有姿势都包含这样的轨迹
		CombineTracks(TrackNames);

		const bool bNewPose = PoseContainer.FindPoseData(PoseName) == nullptr;
		FPoseData* PoseData = PoseContainer.FindOrAddPoseData(PoseName);
		// now copy all transform back to it. 
  // 现在将所有变换复制回其中。
		check(PoseData);
		// Make sure this is whole tracks, not tracknames
  // 确保这是整个曲目，而不是曲目名称
		// TrackNames are what this pose contains
  // TrackNames 是这个姿势包含的内容
		// but We have to add all tracks to match poses container
  // 但我们必须添加所有轨道以匹配姿势容器
		// TrackNames.Num() is subset of PoseContainer.Tracks.Num()
  // TrackNames.Num() 是 PoseContainer.Tracks.Num() 的子集
		// Above CombineTracks will combine both
  // 上面的CombineTracks将结合两者
		const int32 TotalTracks = PoseContainer.Tracks.Num();
		PoseData->SourceLocalSpacePose.Reset(TotalTracks);
		PoseData->SourceLocalSpacePose.AddUninitialized(TotalTracks);
		PoseData->SourceLocalSpacePose.SetNumZeroed(TotalTracks, EAllowShrinking::Yes);

		// just fill up skeleton pose
  // 只需填充骨架姿势
		// the reason we use skeleton pose, is that retarget source can change, and 
  // 我们使用骨架姿势的原因是重定向源可以改变，并且
		// it can miss the tracks. 
  // 它可能会错过轨道。
		PoseContainer.FillUpSkeletonPose(PoseData, MySkeleton);
		check(CurveValues.Num() == PoseContainer.Curves.Num());
		PoseData->SourceCurveData = CurveValues;
  // const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
  // const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
  // const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
  // const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();

		//const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
  // const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
		// why do we need skeleton index
  // 为什么我们需要骨架索引
		//const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
  // const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
  // int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
  // int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
		for (int32 Index = 0; Index < TrackNames.Num(); ++Index)
  // int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
  // int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
		{
			// now get poseData track index
   // 现在获取poseData轨迹索引
			//int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
   // int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
			const FName& TrackName = TrackNames[Index];
			//int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
   // int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
			//int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
   // int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
			const int32 InternalTrackIndex = PoseContainer.Tracks.Find(TrackName);
			// copy to the internal track index
   // 复制到内部轨道索引
			PoseData->SourceLocalSpacePose[InternalTrackIndex] = LocalTransform[Index];
		}
				
		BreakAnimationSequenceGUIDComparison();

		if (bNewPose)
		{
			OnPoseListChanged.Broadcast();
		}
	}
}
void UPoseAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		bool bConvertToAdditivePose = false;
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPoseAsset, RetargetSourceAsset))
		{
			bConvertToAdditivePose = true;
			UpdateRetargetSourceAssetData();
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPoseAsset, RetargetSource))
		{
			bConvertToAdditivePose = true;
		}

		if (bConvertToAdditivePose)
		{
			USkeleton* MySkeleton = GetSkeleton();
			if (MySkeleton)
			{
				// Convert to additive again since retarget source changed
    // 由于重定向源更改，再次转换为加法
				ConvertToAdditivePose(GetBasePoseIndex());
			}
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPoseAsset, SourceAnimation))
		{
			BreakAnimationSequenceGUIDComparison();
		}
	}
}

void UPoseAsset::CombineTracks(const TArray<FName>& NewTracks)
{
	USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton)
	{
		for (const FName& NewTrack : NewTracks)
		{
			if (PoseContainer.Tracks.Contains(NewTrack) == false)
			{
				// if we don't have it, then we'll have to add this track and then 
    // 如果我们没有，那么我们必须添加此曲目，然后
				// right now it doesn't have to be in the hierarchy
    // 现在它不必位于层次结构中
				// @todo: it is probably best to keep the hierarchy of the skeleton, so in the future, we might like to sort this by track after
    // @todo：最好保留骨架的层次结构，因此将来，我们可能希望按轨道排序
				PoseContainer.InsertTrack(NewTrack, MySkeleton, GetRetargetTransforms());
				UpdateTrackBoneIndices();
			}
		}
	}
}

void UPoseAsset::Reinitialize()
{
	PoseContainer.Reset();

	bAdditivePose = false;
	BasePoseIndex = INDEX_NONE;
}

void UPoseAsset::RenameSmartName(const FName& InOriginalName, const FName& InNewName)
{
	RenamePoseOrCurveName(InOriginalName, InNewName);
}

void UPoseAsset::RenamePoseOrCurveName(const FName& InOriginalName, const FName& InNewName)
{
	if(PoseContainer.PoseFNames.Contains(InNewName) || PoseContainer.Curves.ContainsByPredicate([InNewName](const FAnimCurveBase& InCurve){ return InCurve.GetName() == InNewName; }))
	{
		// Cant rename on top of something we already have - this will create duplicates
  // 无法在我们已有的内容之上重命名 - 这会创建重复项
		return;
	}

	for (FName Name : PoseContainer.PoseFNames)
	{
		if (Name == InOriginalName)
		{
			Name = InNewName;
			break;
		}
	}

	for (FAnimCurveBase& Curve : PoseContainer.Curves)
	{
		if (Curve.GetName() == InOriginalName)
		{
			Curve.SetName(InNewName);
			break;
		}
	}

	PoseContainer.RebuildCurveIndexTable();
}

void UPoseAsset::RemoveSmartNames(const TArray<FName>& InNamesToRemove)
{
	RemovePoseOrCurveNames(InNamesToRemove);
}

void UPoseAsset::RemovePoseOrCurveNames(const TArray<FName>& InNamesToRemove)
{
	DeletePoses(InNamesToRemove);
	DeleteCurves(InNamesToRemove);
}

void UPoseAsset::CreatePoseFromAnimation(class UAnimSequence* AnimSequence, const TArray<FName>* InPoseNames/*== nullptr*/)
{
	if (AnimSequence)
	{
		USkeleton* TargetSkeleton = AnimSequence->GetSkeleton();
		if (TargetSkeleton)
		{
			SetSkeleton(TargetSkeleton);
			SourceAnimation = AnimSequence;

			// reinitialize, now we're making new pose from this animation
   // 重新初始化，现在我们正在根据这个动画制作新的姿势
			Reinitialize();

			int32 NumPoses = AnimSequence->GetNumberOfSampledKeys();
			if(InPoseNames && InPoseNames->Num() > NumPoses)
			{
				NumPoses=InPoseNames->Num();
			}
			// make sure we have more than one pose
   // 确保我们有不止一个姿势
			if (NumPoses > 0)
			{
				// stack allocator for extracting curve
    // 用于提取曲线的堆栈分配器
				FMemMark Mark(FMemStack::Get());

				// set up track data - @todo: add revaliation code when checked
    // 设置轨迹数据 - @todo：选中时添加重新验证代码
				IAnimationDataModel* DataModel = AnimSequence->GetDataModel();

				TArray<FName> TrackNames;
				DataModel->GetBoneTrackNames(TrackNames);

				for (const FName& TrackName : TrackNames)
				{
					PoseContainer.Tracks.Add(TrackName);
				}

				// now create pose transform
    // 现在创建姿势变换
				TArray<FTransform> NewPose;
				
				const int32 NumTracks = TrackNames.Num();
				NewPose.Reset(NumTracks);
				NewPose.AddUninitialized(NumTracks);
				
				const double IntervalBetweenKeys = (NumPoses > 1)? AnimSequence->GetPlayLength() / (NumPoses -1 ) : 0.f;

				// add curves - only float curves
    // 添加曲线 - 仅浮动曲线
				const FAnimationCurveData& AnimationCurveData = DataModel->GetCurveData();
				const int32 TotalFloatCurveCount = AnimationCurveData.FloatCurves.Num();

				if (TotalFloatCurveCount > 0)
				{
					for (const FFloatCurve& Curve : AnimationCurveData.FloatCurves)
					{
						PoseContainer.Curves.Add(FAnimCurveBase(Curve.GetName(), Curve.GetCurveTypeFlags()));
					}
				}

				// add to skeleton UID, so that it knows the curve data
    // 添加骨架UID，使其知道曲线数据
				for (int32 PoseIndex = 0; PoseIndex < NumPoses; ++PoseIndex)
				{
					TArray<float> CurveData;
					CurveData.Reserve(TotalFloatCurveCount);
					
					FName NewPoseName = (InPoseNames && InPoseNames->IsValidIndex(PoseIndex))? (*InPoseNames)[PoseIndex] : GetUniquePoseName(this);
					// now get rawanimationdata, and each key is converted to new pose
     // 现在获取原始动画数据，并将每个关键点转换为新的姿势
					for (int32 TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex)
					{
						NewPose[TrackIndex] = AnimSequence->GetDataModel()->GetBoneTrackTransform(TrackNames[TrackIndex], FFrameNumber(PoseIndex));
					}

					if (TotalFloatCurveCount > 0)
					{
						// get curve data
      // 获取曲线数据
						// have to do iterate over time
      // 必须随着时间的推移进行迭代
						// support curve
      // 支持曲线
						FBlendedCurve SourceCurve;
						AnimSequence->EvaluateCurveData(SourceCurve, FAnimExtractContext(PoseIndex*IntervalBetweenKeys), true);
						
						SourceCurve.ForEachElement([&CurveData](const UE::Anim::FCurveElement& InElement)
						{
							CurveData.Add(InElement.Value);
						});
						check(CurveData.Num() == PoseContainer.Curves.Num());
					}
				
					// add new pose
     // 添加新姿势
					PoseContainer.AddOrUpdatePose(NewPoseName, NewPose, CurveData);
				}

				PostProcessData();
			}

			// Mark PoseAsset as (re-)generated
   // 将 PoseAsset 标记为（重新）生成
			UE::Anim::GRegeneratedPoseAssets.Set(this);
			
			SourceAnimationRawDataGUID = GetSourceAnimationGuid();
		}
	}
}

void UPoseAsset::UpdatePoseFromAnimation(class UAnimSequence* AnimSequence)
{
	if (AnimSequence)
	{
		// when you update pose, right now, it just only keeps pose names
  // 当你更新姿势时，现在它只保留姿势名称
		// in the future we might want to make it more flexible
  // 将来我们可能想让它更加灵活
		// back up old pose names
  // 备份旧的姿势名称
		const TArray<FName> OldPoseNames = PoseContainer.PoseFNames;
		const bool bOldAdditive = bAdditivePose;
		int32 OldBasePoseIndex = BasePoseIndex;
		CreatePoseFromAnimation(AnimSequence, &OldPoseNames);

		// fix up additive info if it's additive
  // 修复附加信息（如果它是附加信息）
		if (bOldAdditive)
		{
			if (PoseContainer.Poses.IsValidIndex(OldBasePoseIndex) == false)
			{
				// if it's pointing at invalid index, just reset to ref pose
    // 如果它指向无效索引，只需重置为参考姿势
				OldBasePoseIndex = INDEX_NONE;
			}

			// Convert to additive again
   // 再次转换为加法
			ConvertToAdditivePose(OldBasePoseIndex);
		}

		PoseContainer.RebuildCurveIndexTable();

		OnPoseListChanged.Broadcast();
	}
}

bool UPoseAsset::ModifyPoseName(FName OldPoseName, FName NewPoseName)
{
	if (ContainsPose(NewPoseName))
	{
		// already exists, return 
  // 已存在，返回
		return false;
	}

	if (FPoseData* PoseData = PoseContainer.FindPoseData(OldPoseName))
	{
		PoseContainer.RenamePose(OldPoseName, NewPoseName);
		OnPoseListChanged.Broadcast();

		return true;
	}

	return false;
}

int32 UPoseAsset::DeletePoses(TArray<FName> PoseNamesToDelete)
{
	int32 ItemsDeleted = 0;

	for (const FName& PoseName : PoseNamesToDelete)
	{
		int32 PoseIndexDeleted = PoseContainer.DeletePose(PoseName);
		if (PoseIndexDeleted != INDEX_NONE)
		{
			++ItemsDeleted;
			// if base pose index is same as pose index deleted
   // 如果基本姿势索引与删除的姿势索引相同
			if (BasePoseIndex == PoseIndexDeleted)
			{
				BasePoseIndex = INDEX_NONE;
			}
			// if base pose index is after this, we reduce the number
   // 如果基本姿势索引在此之后，我们减少数量
			else if (BasePoseIndex > PoseIndexDeleted)
			{
				--BasePoseIndex;
			}
		}
	}
	
	if (ItemsDeleted)
	{
		BreakAnimationSequenceGUIDComparison();
	}

	PostProcessData();
	OnPoseListChanged.Broadcast();

	return ItemsDeleted;
}

int32 UPoseAsset::DeleteCurves(TArray<FName> CurveNamesToDelete)
{
	int32 ItemsDeleted = 0;

	for (const FName& CurveName : CurveNamesToDelete)
	{
		if(PoseContainer.DeleteCurve(CurveName))
		{
			++ItemsDeleted;
		}
	}

	OnPoseListChanged.Broadcast();

	return ItemsDeleted;
}

void UPoseAsset::ConvertToFullPose()
{
	PoseContainer.ConvertToFullPose(GetSkeleton(), GetRetargetTransforms());
	bAdditivePose = false;
}

void UPoseAsset::ConvertToAdditivePose(int32 NewBasePoseIndex)
{
	// make sure it's valid
 // 确保它有效
	check(NewBasePoseIndex == -1 || PoseContainer.Poses.IsValidIndex(NewBasePoseIndex));

	BasePoseIndex = NewBasePoseIndex;

	TArray<FTransform> BasePose;
	TArray<float>		BaseCurves;
	GetBasePoseTransform(BasePose, BaseCurves);

	PoseContainer.ConvertToAdditivePose(BasePose, BaseCurves);

	bAdditivePose = true;
}

bool UPoseAsset::GetFullPose(int32 PoseIndex, TArray<FTransform>& OutTransforms) const
{
	if (!PoseContainer.Poses.IsValidIndex(PoseIndex))
	{
		return false;
	}

	// just return source data
 // 只返回源数据
	OutTransforms = PoseContainer.Poses[PoseIndex].SourceLocalSpacePose;
	return true;
}

FTransform UPoseAsset::GetComponentSpaceTransform(FName BoneName, const TArray<FTransform>& LocalTransforms) const
{
	const FReferenceSkeleton& RefSkel = GetSkeleton()->GetReferenceSkeleton();

	// Init component space transform with identity
 // 用恒等式初始化组件空间变换
	FTransform ComponentSpaceTransform = FTransform::Identity;

	// Start to walk up parent chain until we reach root (ParentIndex == INDEX_NONE)
 // 开始沿着父链向上走，直到到达根（ParentIndex == INDEX_NONE）
	int32 BoneIndex = RefSkel.FindBoneIndex(BoneName);
	while (BoneIndex != INDEX_NONE)
	{
		BoneName = RefSkel.GetBoneName(BoneIndex);
		int32 TrackIndex = GetTrackIndexByName(BoneName);

		// If a track for parent, get local space transform from that
  // 如果是父轨道，则从中获取局部空间变换
		// If not, get from ref pose
  // 如果没有，则从参考姿势获取
		FTransform BoneLocalTM = (TrackIndex != INDEX_NONE) ? LocalTransforms[TrackIndex] : RefSkel.GetRefBonePose()[BoneIndex];

		// Continue to build component space transform
  // 继续构建组件空间变换
		ComponentSpaceTransform = ComponentSpaceTransform * BoneLocalTM;

		// Now move up to parent
  // 现在向上移动到父级
		BoneIndex = RefSkel.GetParentIndex(BoneIndex);
	}

	return ComponentSpaceTransform;
}

const FTransform& UPoseAsset::GetLocalSpaceTransform(FName BoneName, int32 PoseIndex) const
{
	const int32 TrackIdx = GetTrackIndexByName(BoneName);
	if (!PoseContainer.Poses.IsValidIndex(PoseIndex) || TrackIdx == INDEX_NONE)
	{
		UE_LOG(LogAnimation, Error, TEXT("Can't find bone with name %s when trying to get local space transform"), *BoneName.ToString());
		return FTransform::Identity;
	}
	return PoseContainer.Poses[PoseIndex].SourceLocalSpacePose[TrackIdx];
}

bool UPoseAsset::ConvertSpace(bool bNewAdditivePose, int32 NewBasePoseIndex)
{
	// first convert to full pose first
 // 首先转换为完整姿势
	bAdditivePose = bNewAdditivePose;
	BasePoseIndex = NewBasePoseIndex;
	PostProcessData();

	return true;
}
#endif // WITH_EDITOR

const int32 UPoseAsset::GetPoseIndexByName(const FName& InBasePoseName) const
{
	for (int32 PoseIndex = 0; PoseIndex < PoseContainer.PoseFNames.Num(); ++PoseIndex)
	{
		if (PoseContainer.PoseFNames[PoseIndex] == InBasePoseName)
		{
			return PoseIndex;
		}
	}

	return INDEX_NONE;
}

const int32 UPoseAsset::GetCurveIndexByName(const FName& InCurveName) const
{
	for (int32 TestIdx = 0; TestIdx < PoseContainer.Curves.Num(); TestIdx++)
	{
		const FAnimCurveBase& Curve = PoseContainer.Curves[TestIdx];
		if (Curve.GetName() == InCurveName)
		{
			return TestIdx;
		}
	}
	return INDEX_NONE;
}


void UPoseAsset::UpdateTrackBoneIndices()
{
	USkeleton* MySkeleton = GetSkeleton();
	PoseContainer.TrackBoneIndices.Reset();
	if (MySkeleton)
	{
		const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();

		PoseContainer.TrackBoneIndices.SetNumZeroed(PoseContainer.Tracks.Num());
		for (int32 TrackIndex = 0; TrackIndex < PoseContainer.Tracks.Num(); ++TrackIndex)
		{
			const FName& TrackName = PoseContainer.Tracks[TrackIndex];
			PoseContainer.TrackBoneIndices[TrackIndex] = RefSkeleton.FindBoneIndex(TrackName);
		}
	}
}

bool UPoseAsset::RemoveInvalidTracks()
{
	const USkeleton* MySkeleton = GetSkeleton();
	const int32 InitialNumTracks = PoseContainer.Tracks.Num();

	if (MySkeleton)
	{
		const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();

		// set up track data 
  // 设置轨迹数据
		for (int32 TrackIndex = 0; TrackIndex < PoseContainer.Tracks.Num(); ++TrackIndex)
		{
			const FName& TrackName = PoseContainer.Tracks[TrackIndex];
			const int32 SkeletonTrackIndex = RefSkeleton.FindBoneIndex(TrackName);
			if (SkeletonTrackIndex == INDEX_NONE)
			{
				// delete this track. It's missing now
    // 删除该曲目。现在不见了
				PoseContainer.DeleteTrack(TrackIndex);
				--TrackIndex;
			}
		}
	}

	return InitialNumTracks != PoseContainer.Tracks.Num();
}

#if WITH_EDITOR
void UPoseAsset::SetRetargetSourceAsset(USkeletalMesh* InRetargetSourceAsset)
{
	if (InRetargetSourceAsset != nullptr && InRetargetSourceAsset->HasAnyFlags(RF_Transient))
	{
		UE_LOG(LogAnimation, Error, TEXT("Error, Transient asset [%s] can not be assigned as Retarget Source for Pose Asset [%s]. Please, use a non transient asset as retarget surce.")
			, *(InRetargetSourceAsset->GetFullName())
			, *GetFullName());
		ensure(false);
		return;
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	RetargetSourceAsset = InRetargetSourceAsset;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UPoseAsset::ClearRetargetSourceAsset()
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	RetargetSourceAsset.Reset();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

const TSoftObjectPtr<USkeletalMesh>& UPoseAsset::GetRetargetSourceAsset() const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return RetargetSourceAsset;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UPoseAsset::UpdateRetargetSourceAssetData()
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	USkeletalMesh* SourceReferenceMesh = RetargetSourceAsset.LoadSynchronous();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	
	const USkeleton* MySkeleton = GetSkeleton();
	if (SourceReferenceMesh && MySkeleton)
	{
		FAnimationRuntime::MakeSkeletonRefPoseFromMesh(SourceReferenceMesh, MySkeleton, RetargetSourceAssetReferencePose);
	}
	else
	{
		RetargetSourceAssetReferencePose.Empty();
	}
}
#endif // WITH_EDITORONLY_DATA

const TArray<FTransform>& UPoseAsset::GetRetargetTransforms() const
{
	if (RetargetSource.IsNone() && RetargetSourceAssetReferencePose.Num() > 0)
	{
		return RetargetSourceAssetReferencePose;
	}
	else
	{
		const USkeleton* MySkeleton = GetSkeleton();
		if (MySkeleton)
		{
			return MySkeleton->GetRefLocalPoses(RetargetSource);
		}
		else
		{
			static TArray<FTransform> EmptyTransformArray;
			return EmptyTransformArray;
		}
	}
}

FName UPoseAsset::GetRetargetTransformsSourceName() const
{
	if (RetargetSource.IsNone() && RetargetSourceAssetReferencePose.Num() > 0)
	{
		return GetOutermost()->GetFName();
	}
	else
	{
		return RetargetSource;
	}
}

void FPoseDataContainer::DeleteTrack(int32 TrackIndex)
{
	Tracks.RemoveAt(TrackIndex);
	for (FPoseData& Pose : Poses)
	{
#if WITH_EDITOR
		// if not editor, they can't save this data, so it will run again when editor runs
  // 如果不是编辑器，他们无法保存此数据，因此当编辑器运行时它将再次运行
		Pose.SourceLocalSpacePose.RemoveAt(TrackIndex);
#endif // WITH_EDITOR
	}
}

#if WITH_EDITOR
FName UPoseAsset::GetUniquePoseName(const USkeleton* Skeleton)
{
	return NAME_None;
}

FSmartName UPoseAsset::GetUniquePoseSmartName(USkeleton* Skeleton)
{
	return FSmartName();
}

FName UPoseAsset::GetUniquePoseName(UPoseAsset* PoseAsset)
{
	check(PoseAsset);
	int32 NameIndex = 0;
	FName NewName;

	do
	{
		NewName = FName(*FString::Printf(TEXT("Pose_%d"), NameIndex++));
	}
	while(PoseAsset->ContainsPose(NewName));
	
	return NewName;
}

FGuid UPoseAsset::GetSourceAnimationGuid() const
{	
	if (SourceAnimation)
	{
		IAnimationDataModel::FGuidGenerationSettings Settings;
		if (GetLinkerCustomVersion(FUE5ReleaseStreamObjectVersion::GUID) < FUE5ReleaseStreamObjectVersion::AnimModelGuidGenerationSettings)
		{
			Settings.bIncludeTimingData = false;
		}		

		// If pose asset was re-generated during editor runtime, use the new GUID format
  // 如果在编辑器运行时重新生成姿势资源，请使用新的 GUID 格式
		if(UE::Anim::GRegeneratedPoseAssets.Get(this))
		{
			Settings.bIncludeTimingData = true;	
		}
		
		return SourceAnimation->GetDataModel()->GenerateGuid(Settings);
	}

	return FGuid();
}

void UPoseAsset::RemapTracksToNewSkeleton(USkeleton* NewSkeleton, bool bConvertSpaces)
{
	Super::RemapTracksToNewSkeleton(NewSkeleton, bConvertSpaces);

	PostProcessData();
}

bool UPoseAsset::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive /*= true*/)
{
	Super::GetAllAnimationSequencesReferred(AnimationAssets, bRecursive);
	if (SourceAnimation)
	{
		SourceAnimation->HandleAnimReferenceCollection(AnimationAssets, bRecursive);
	}

	return AnimationAssets.Num() > 0;
}

void UPoseAsset::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap)
{
	Super::ReplaceReferredAnimations(ReplacementMap);
	if (SourceAnimation)
	{
		UAnimSequence* const* ReplacementAsset = (UAnimSequence*const*)ReplacementMap.Find(SourceAnimation);
		if (ReplacementAsset)
		{
			SourceAnimation = *ReplacementAsset;
		}
	}
}

bool UPoseAsset::GetBasePoseTransform(TArray<FTransform>& OutBasePose, TArray<float>& OutCurve) const
{
	int32 TotalNumTrack = PoseContainer.Tracks.Num();
	OutBasePose.Reset(TotalNumTrack);

	if (BasePoseIndex == -1)
	{
		OutBasePose.AddUninitialized(TotalNumTrack);

		USkeleton* MySkeleton = GetSkeleton();
		if (MySkeleton)
		{
			for (int32 TrackIndex = 0; TrackIndex < PoseContainer.Tracks.Num(); ++TrackIndex)
			{
				const FName& TrackName = PoseContainer.Tracks[TrackIndex];
				OutBasePose[TrackIndex] = PoseContainer.GetDefaultTransform(TrackName, MySkeleton, GetRetargetTransforms());
			}
		}
		else
		{
			for (int32 TrackIndex = 0; TrackIndex < PoseContainer.Tracks.Num(); ++TrackIndex)
			{
				OutBasePose[TrackIndex].SetIdentity();
			}
		}

		// add zero curves
  // 添加零曲线
		OutCurve.AddZeroed(PoseContainer.Curves.Num());
		check(OutBasePose.Num() == TotalNumTrack);
		return true;
	}
	else if (PoseContainer.Poses.IsValidIndex(BasePoseIndex))
	{
		OutBasePose = PoseContainer.Poses[BasePoseIndex].SourceLocalSpacePose;
		OutCurve = PoseContainer.Poses[BasePoseIndex].SourceCurveData;
		check(OutBasePose.Num() == TotalNumTrack);
		return true;
	}

	return false;
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE 

