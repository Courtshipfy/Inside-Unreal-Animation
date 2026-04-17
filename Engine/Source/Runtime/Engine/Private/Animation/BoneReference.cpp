// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "Animation/Skeleton.h"
#include "EngineLogs.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BoneReference)

bool FBoneReference::Initialize(const FBoneContainer& RequiredBones)
{
#if WITH_EDITOR
	BoneName = *BoneName.ToString().TrimStartAndEnd();
#endif
	BoneIndex = RequiredBones.GetPoseBoneIndexForBoneName(BoneName);

	bUseSkeletonIndex = false;
	// If bone name is not found, look into the leader skeleton to see if it's found there.
 // 如果未找到骨骼名称，请查看领导者骨骼以查看是否在那里找到它。
	// SkeletalMeshes can exclude bones from the leader skeleton, and that's OK.
 // SkeletalMeshes 可以从领导者骨架中排除骨骼，这没问题。
	// If it's not found in the leader skeleton, the bone does not exist at all! so we should log it.
 // 如果在首领骨骼中没有找到，那么这根骨头根本不存在！所以我们应该记录它。
	if (BoneIndex == INDEX_NONE && BoneName != NAME_None)
	{
		if (USkeleton* SkeletonAsset = RequiredBones.GetSkeletonAsset())
		{
			if (SkeletonAsset->GetReferenceSkeleton().FindBoneIndex(BoneName) == INDEX_NONE)
			{
				UE_LOG(LogAnimation, Log, TEXT("FBoneReference::Initialize BoneIndex for Bone '%s' does not exist in Skeleton '%s'"),
					*BoneName.ToString(), *GetNameSafe(SkeletonAsset));
			}
		}
	}

	CachedCompactPoseIndex = RequiredBones.MakeCompactPoseIndex(GetMeshPoseIndex(RequiredBones));

	return (BoneIndex != INDEX_NONE);
}

bool FBoneReference::Initialize(const USkeleton* Skeleton)
{
	if (Skeleton && (BoneName != NAME_None))
	{
#if WITH_EDITOR
		BoneName = *BoneName.ToString().TrimStartAndEnd();
#endif
		BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(BoneName);
		bUseSkeletonIndex = true;
	}
	else
	{
		BoneIndex = INDEX_NONE;
	}

	CachedCompactPoseIndex = FCompactPoseBoneIndex(INDEX_NONE);

	return (BoneIndex != INDEX_NONE);
}

bool FBoneReference::IsValidToEvaluate(const FBoneContainer& RequiredBones) const
{
	return (BoneIndex != INDEX_NONE && RequiredBones.IsValid() && RequiredBones.Contains(BoneIndex));
}

FSkeletonPoseBoneIndex FBoneReference::GetSkeletonPoseIndex(const FBoneContainer& RequiredBones) const
{ 
	// accessing array with invalid index would cause crash, so we have to check here
 // 访问具有无效索引的数组会导致崩溃，所以我们必须在这里检查
	if (BoneIndex != INDEX_NONE)
	{
		if (bUseSkeletonIndex)
		{
			return FSkeletonPoseBoneIndex(BoneIndex);
		}
		else
		{
			return RequiredBones.GetSkeletonPoseIndexFromMeshPoseIndex(FMeshPoseBoneIndex(BoneIndex));
		}
	}

	return FSkeletonPoseBoneIndex(INDEX_NONE);
}
	
FMeshPoseBoneIndex FBoneReference::GetMeshPoseIndex(const FBoneContainer& RequiredBones) const
{ 
	// accessing array with invalid index would cause crash, so we have to check here
 // 访问具有无效索引的数组会导致崩溃，所以我们必须在这里检查
	if (BoneIndex != INDEX_NONE)
	{
		if (bUseSkeletonIndex)
		{
			return RequiredBones.GetMeshPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(BoneIndex));
		}
		else
		{
			return FMeshPoseBoneIndex(BoneIndex);
		}
	}

	return FMeshPoseBoneIndex(INDEX_NONE);
}

FCompactPoseBoneIndex FBoneReference::GetCompactPoseIndex(const FBoneContainer& RequiredBones) const 
{ 
	if (bUseSkeletonIndex)
	{
		//If we were initialized with a skeleton we wont have a cached index.
  // 如果我们用骨架初始化，我们将不会有缓存索引。
		if (BoneIndex != INDEX_NONE)
		{
			// accessing array with invalid index would cause crash, so we have to check here
   // 访问具有无效索引的数组会导致崩溃，所以我们必须在这里检查
			return RequiredBones.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(BoneIndex));
		}
		return FCompactPoseBoneIndex(INDEX_NONE);
	}
		
	return CachedCompactPoseIndex;
}

// need this because of BoneReference being used in CurveMetaData and that is in SmartName
// 需要这个，因为 CurveMetaData 中使用了 BoneReference，而 SmartName 中也使用了 BoneReference
FArchive& operator<<(FArchive& Ar, FBoneReference& B)
{
	Ar << B.BoneName;
	return Ar;
}

bool FBoneReference::Serialize(FArchive& Ar)
{
	Ar << *this;
	return true;
}