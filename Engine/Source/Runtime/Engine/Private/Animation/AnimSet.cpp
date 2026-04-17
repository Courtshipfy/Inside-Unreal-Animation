// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimSet.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/UObjectIterator.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimSet)

/////////////////////////////////////////////////////
// UAnimSet
// U动画集

UAnimSet::UAnimSet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UAnimSet::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	// Make sure that AnimSets (and sequences) within level packages are not marked as standalone.
	// 确保关卡包中的动画集（和序列）没有标记为独立的。
	if(GetOutermost()->ContainsMap() && HasAnyFlags(RF_Standalone))
	{
		ClearFlags(RF_Standalone);

		for(int32 i=0; i<Sequences.Num(); i++)
		{
			UAnimSequence* Seq = Sequences[i];
			if(Seq)
			{
				Seq->ClearFlags(RF_Standalone);
			}
		}
	}
#endif	//#if WITH_EDITORONLY_DATA
}	


bool UAnimSet::CanPlayOnSkeletalMesh(USkeletalMesh* SkelMesh) const
{
	// Temporarily allow any animation to play on any AnimSet. 
	// 暂时允许任何动画在任何 AnimSet 上播放。
	// We need a looser metric for matching animation to skeletons. Some 'overlap bone count'?
	// 我们需要一个更宽松的指标来将动画与骨骼相匹配。一些“重叠骨数”？
#if 0
	// This is broken and needs to be looked into.
	// 这已被破坏，需要进行调查。
	// we require at least 10% of tracks matched by skeletal mesh.
	// 我们需要至少 10% 的轨迹与骨架网格物体匹配。
	return GetSkeletalMeshMatchRatio(SkelMesh) > 0.1f;
#else
	return true;
#endif
}

float UAnimSet::GetSkeletalMeshMatchRatio(USkeletalMesh* SkelMesh) const
{
	// First see if there is a bone for all tracks
	// 首先查看是否有所有轨道的骨骼
	int32 TracksMatched = 0;
	for(int32 i=0; i<TrackBoneNames.Num() ; i++)
	{
		const int32 BoneIndex = SkelMesh->GetRefSkeleton().FindBoneIndex( TrackBoneNames[i] );
		if( BoneIndex != INDEX_NONE )
		{
			++TracksMatched;
		}
	}

	// If we can't match any bones, then this is definitely not compatible.
	// 如果我们无法匹配任何骨骼，那么这肯定是不兼容的。
	if( TrackBoneNames.Num() == 0 || TracksMatched == 0 )
	{
		return 0.f;
	}

	// return how many of the animation tracks were matched by that mesh
	// 返回该网格匹配的动画轨道数量
	return (float)TracksMatched / float(TrackBoneNames.Num());
}


UAnimSequence* UAnimSet::FindAnimSequence(FName SequenceName)
{
	UAnimSequence* AnimSequence = NULL;
#if WITH_EDITORONLY_DATA
	if( SequenceName != NAME_None )
	{
		for(int32 i=0; i<Sequences.Num(); i++)
		{
			if( Sequences[i]->GetFName() == SequenceName )
			{				
				AnimSequence = Sequences[i];
				break;
			}
		}
	}
#endif	//#if WITH_EDITORONLY_DATA

	return AnimSequence;
}


int32 UAnimSet::GetMeshLinkupIndex(USkeletalMesh* SkelMesh)
{
	// First, see if we have a cached link-up between this animation and the given skeletal mesh.
	// [翻译失败: First, see if we have a cached link-up between this animation and the given skeletal mesh.]
	check(SkelMesh);

	// Get SkeletalMesh path name
	// [翻译失败: Get SkeletalMesh path name]
	FName SkelMeshName = FName( *SkelMesh->GetPathName() );

	// See if we have already cached this Skeletal Mesh.
	// 看看我们是否已经缓存了这个骨架网格体。
	const int32* IndexPtr = SkelMesh2LinkupCache.Find( SkelMeshName );

	// If not found, create a new entry
	// 如果没有找到，则创建一个新条目
	if( IndexPtr == NULL )
	{
		// No linkup found - so create one here and add to cache.
		// 未找到链接 - 因此请在此处创建一个链接并将其添加到缓存中。
		const int32 NewLinkupIndex = LinkupCache.AddZeroed();

		// Add it to our cache
		// 将其添加到我们的缓存中
		SkelMesh2LinkupCache.Add( SkelMeshName, NewLinkupIndex );
		
		// Fill it up
		// 填满它
		FAnimSetMeshLinkup* NewLinkup = &LinkupCache[NewLinkupIndex];
		NewLinkup->BuildLinkup(SkelMesh, this);

		return NewLinkupIndex;
	}

	return (*IndexPtr);
}

void UAnimSet::ResetAnimSet()
{
#if WITH_EDITORONLY_DATA
	// Make sure we handle AnimSequence references properly before emptying the array.
	// 确保在清空数组之前正确处理 AnimSequence 引用。
	for(int32 i=0; i<Sequences.Num(); i++)
	{	
		UAnimSequence* AnimSeq = Sequences[i];
		if( AnimSeq )
		{
			AnimSeq->ResetAnimation();
		}
	}
	Sequences.Empty();
	TrackBoneNames.Empty();
	LinkupCache.Empty();
	SkelMesh2LinkupCache.Empty();

	// We need to re-init any skeleltal mesh components now, because they might still have references to linkups in this set.
	// 我们现在需要重新初始化所有骨架网格物体组件，因为它们可能仍然引用该集合中的链接。
	for(TObjectIterator<USkeletalMeshComponent> It;It;++It)
	{
		USkeletalMeshComponent* SkelComp = *It;
		if(IsValid(SkelComp) && !SkelComp->IsTemplate())
		{
			SkelComp->InitAnim(true);
		}
	}
#endif // WITH_EDITORONLY_DATA
}


bool UAnimSet::RemoveAnimSequenceFromAnimSet(UAnimSequence* AnimSeq)
{
#if WITH_EDITORONLY_DATA
	int32 SequenceIndex = Sequences.Find(AnimSeq);
	if( SequenceIndex != INDEX_NONE )
	{
		// Handle reference clean up properly
		// 正确处理引用清理
		AnimSeq->ResetAnimation();
		// Remove from array
		// 从数组中删除
		Sequences.RemoveAt(SequenceIndex, 1);
		if( GIsEditor )
		{
			MarkPackageDirty();
		}
		return true;
	}
#endif // WITH_EDITORONLY_DATA

	return false;
}

void UAnimSet::ClearAllAnimSetLinkupCaches()
{
	double Start = FPlatformTime::Seconds();

	TArray<UAnimSet*> AnimSets;
	TArray<USkeletalMeshComponent*> SkelComps;
	// Find all AnimSets and SkeletalMeshComponents (just do one iterator)
	// 查找所有 AnimSet 和 SkeletalMeshComponents（只需执行一个迭代器）
	for(TObjectIterator<UObject> It;It;++It)
	{
		UAnimSet* AnimSet = Cast<UAnimSet>(*It);
		if(IsValid(AnimSet) && !AnimSet->IsTemplate())
		{
			AnimSets.Add(AnimSet);
		}

		USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(*It);
		if(IsValid(SkelComp) && !SkelComp->IsTemplate())
		{
			SkelComps.Add(SkelComp);
		}
	}

	// For all AnimSets, empty their linkup cache
	// 对于所有 AnimSet，清空其链接缓存
	for(int32 i=0; i<AnimSets.Num(); i++)
	{
		AnimSets[i]->LinkupCache.Empty();
		AnimSets[i]->SkelMesh2LinkupCache.Empty();
	}

	UE_LOG(LogAnimation, Log, TEXT("ClearAllAnimSetLinkupCaches - Took %3.2fms"), (FPlatformTime::Seconds() - Start)*1000.f);
}


/////////////////////////////////////////////////////
// FAnimSetMeshLinkup
// FAnimSetMeshLinkup

void FAnimSetMeshLinkup::BuildLinkup(USkeletalMesh* InSkelMesh, UAnimSet* InAnimSet)
{
	int32 const NumBones = InSkelMesh->GetRefSkeleton().GetNum();

	// Bone to Track mapping.
	// 骨骼到轨迹映射。
	BoneToTrackTable.Empty(NumBones);
	BoneToTrackTable.AddUninitialized(NumBones);

	// For each bone in skeletal mesh, find which track to pull from in the AnimSet.
	// 对于骨架网格物体中的每个骨骼，找到要从 AnimSet 中拉出的轨道。
	for (int32 i=0; i<NumBones; i++)
	{
		FName const BoneName = InSkelMesh->GetRefSkeleton().GetBoneName(i);

		// FindTrackWithName will return INDEX_NONE if no track exists.
		// 如果不存在曲目，FindTrackWithName 将返回 INDEX_NONE。
		BoneToTrackTable[i] = InAnimSet->FindTrackWithName(BoneName);
	}

	// Check here if we've properly cached those arrays.
	// 在此处检查我们是否已正确缓存这些数组。
	if( InAnimSet->BoneUseAnimTranslation.Num() != InAnimSet->TrackBoneNames.Num() )
	{
		int32 const NumTracks = InAnimSet->TrackBoneNames.Num();

		InAnimSet->BoneUseAnimTranslation.Empty(NumTracks);
		InAnimSet->BoneUseAnimTranslation.AddUninitialized(NumTracks);

		InAnimSet->ForceUseMeshTranslation.Empty(NumTracks);
		InAnimSet->ForceUseMeshTranslation.AddUninitialized(NumTracks);

		for(int32 TrackIndex = 0; TrackIndex<NumTracks; TrackIndex++)
		{
			FName const TrackBoneName = InAnimSet->TrackBoneNames[TrackIndex];

			// Cache whether to use the translation from this bone or from ref pose.
			// 缓存是否使用来自该骨骼的平移或来自参考姿势的平移。
			InAnimSet->BoneUseAnimTranslation[TrackIndex] = InAnimSet->UseTranslationBoneNames.Contains(TrackBoneName);
			InAnimSet->ForceUseMeshTranslation[TrackIndex] = InAnimSet->ForceMeshTranslationBoneNames.Contains(TrackBoneName);
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TArray<bool> TrackUsed;
	TrackUsed.AddZeroed(InAnimSet->TrackBoneNames.Num());
	const int32 AnimLinkupIndex = InAnimSet->GetMeshLinkupIndex( InSkelMesh );
	const FAnimSetMeshLinkup& AnimLinkup = InAnimSet->LinkupCache[ AnimLinkupIndex ];
	for(int32 BoneIndex=0; BoneIndex<NumBones; BoneIndex++)
	{
		const int32 TrackIndex = AnimLinkup.BoneToTrackTable[BoneIndex];

		if( TrackIndex == INDEX_NONE )
		{
			continue;
		}

		if( TrackUsed[TrackIndex] )
		{
			UE_LOG(LogAnimation, Warning, TEXT("%s has multiple bones sharing the same track index!!!"), *InAnimSet->GetFullName());	
			for(int32 DupeBoneIndex=0; DupeBoneIndex<NumBones; DupeBoneIndex++)
			{
				const int32 DupeTrackIndex = AnimLinkup.BoneToTrackTable[DupeBoneIndex];
				if( DupeTrackIndex == TrackIndex )
				{
					UE_LOG(LogAnimation, Warning, TEXT(" BoneIndex: %i, BoneName: %s, TrackIndex: %i, TrackBoneName: %s"), DupeBoneIndex, *InSkelMesh->GetRefSkeleton().GetBoneName(DupeBoneIndex).ToString(), DupeTrackIndex, *InAnimSet->TrackBoneNames[DupeTrackIndex].ToString());
				}
			}
		}

		TrackUsed[TrackIndex] = true;
	}
#endif
}


