// Copyright Epic Games, Inc. All Rights Reserved.

/** 
 * This is a set of AnimSequences
 * All sequence have the same number of tracks, and they relate to the same bone names.
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "AnimSet.generated.h"

class UAnimSequence;
class USkeletalMesh;

/** This is a mapping table between each bone in a particular skeletal mesh and the tracks of this animation set. */
/** 这是特定骨架网格物体中的每个骨骼与该动画集的轨迹之间的映射表。 */
/** 这是特定骨架网格物体中的每个骨骼与该动画集的轨迹之间的映射表。 */
/** 这是特定骨架网格物体中的每个骨骼与该动画集的轨迹之间的映射表。 */
USTRUCT()
struct FAnimSetMeshLinkup
{
	GENERATED_USTRUCT_BODY()

	/** 
	 * Mapping table. Size must be same as size of SkelMesh reference skeleton. 
	 * No index should be more than the number of tracks in this AnimSet.
	 * -1 indicates no track for this bone - will use reference pose instead.
	 */
	UPROPERTY()
	TArray<int32> BoneToTrackTable;


		/** 重置此链接并在提供的骨架网格物体和动画集之间重新创建。 */

		/** 重置此链接并在提供的骨架网格物体和动画集之间重新创建。 */
		/** Reset this linkup and re-create between the provided skeletal mesh and anim set. */
		/** 重置此链接并在提供的骨架网格物体和动画集之间重新创建。 */
		void BuildLinkup(USkeletalMesh* InSkelMesh, class UAnimSet* InAnimSet);
	
};

UCLASS(hidecategories=Object, MinimalAPI)
class UAnimSet : public UObject
{
	GENERATED_UCLASS_BODY()

private:
	/** 
	 *	Indicates that only the rotation should be taken from the animation sequence and the translation should come from the USkeletalMesh ref pose. 
	 *	Note that the root bone always takes translation from the animation, even if this flag is set.
	 *	You can use the UseTranslationBoneNames array to specify other bones that should use translation with this flag set.
	 */
	UPROPERTY(EditAnywhere, Category=AnimSet)
	/** 每个轨道相关的骨骼名称。 TrackBoneName.Num() == 轨道数。 */
	uint32 bAnimRotationOnly:1;

	/** 每个轨道相关的骨骼名称。 TrackBoneName.Num() == 轨道数。 */
public:
	/** Bone name that each track relates to. TrackBoneName.Num() == Number of tracks. */
	/** 实际的动画序列信息。 */
	/** 每个轨道相关的骨骼名称。 TrackBoneName.Num() == 轨道数。 */
	UPROPERTY()
	TArray<FName> TrackBoneNames;
	/** 实际的动画序列信息。 */

#if WITH_EDITORONLY_DATA
	/** 不同骨架网格物体和此 AnimSet 之间链接的非序列化缓存。 */
	/** Actual animation sequence information. */
	/** 实际的动画序列信息。 */
	UPROPERTY()
	TArray<TObjectPtr<class UAnimSequence>> Sequences;
	/** 不同骨架网格物体和此 AnimSet 之间链接的非序列化缓存。 */

#endif // WITH_EDITORONLY_DATA
private:
	/** Non-serialised cache of linkups between different skeletal meshes and this AnimSet. */
	/** 不同骨架网格物体和此 AnimSet 之间链接的非序列化缓存。 */
	// Do not change private - they will go away
 // 不要改变私人 - 他们会消失
	UPROPERTY(transient)
	TArray<struct FAnimSetMeshLinkup> LinkupCache;

	/** ForceMeshTranslationBoneNames 的简化版本 */
	/** 
	 *	Array of booleans that indicate whether or not to read the translation of a bone from animation or ref skeleton.
	 *	This is basically a cooked down version of UseTranslationBoneNames for speed.
	 *	Size matches the number of tracks.
	/** ForceMeshTranslationBoneNames 的简化版本 */
	/** 如果设置了 bAnimRotationOnly，则应使用动画翻译的骨骼名称。 */
	 */
	// Do not change private - they will go away
 // 不要改变私人 - 他们会消失
	UPROPERTY(transient)
	TArray<uint8> BoneUseAnimTranslation;
	/** 如果设置了 bAnimRotationOnly，则应使用动画翻译的骨骼名称。 */
	/** 总是使用网格物体而不是动画的翻译的骨骼列表。 */

	/** Cooked down version of ForceMeshTranslationBoneNames */
	/** ForceMeshTranslationBoneNames 的简化版本 */
	// Do not change private - they will go away
 // 不要改变私人 - 他们会消失
	/** 总是使用网格物体而不是动画的翻译的骨骼列表。 */
	UPROPERTY(transient)
	/** 在 AnimSetEditor 中，当您切换到此 AnimSet 时，它会查看此骨架网格物体是否已加载，如果已加载，则切换到它。 */
	TArray<uint8> ForceUseMeshTranslation;

	/** Names of bones that should use translation from the animation, if bAnimRotationOnly is set. */
	/** 如果设置了 bAnimRotationOnly，则应使用动画翻译的骨骼名称。 */
	/** 保存其参考骨架与 TrackBoneName 数组最匹配的骨架网格物体的名称。 */
	// Do not change private - they will go away
 // 不要改变私人 - 他们会消失
	/** 在 AnimSetEditor 中，当您切换到此 AnimSet 时，它会查看此骨架网格物体是否已加载，如果已加载，则切换到它。 */
	UPROPERTY(EditAnywhere, Category=AnimSet)
	TArray<FName> UseTranslationBoneNames;

	/** 保存其参考骨架与 TrackBoneName 数组最匹配的骨架网格物体的名称。 */
	/** List of bones which are ALWAYS going to use their translation from the mesh and not the animation. */
	/** 总是使用网格物体而不是动画的翻译的骨骼列表。 */
	// Do not change private - they will go away
 // 不要改变私人 - 他们会消失
	UPROPERTY(EditAnywhere, Category=AnimSet)
	TArray<FName> ForceMeshTranslationBoneNames;

public:
	/** In the AnimSetEditor, when you switch to this AnimSet, it sees if this skeletal mesh is loaded and if so switches to it. */
	/** 在 AnimSetEditor 中，当您切换到此 AnimSet 时，它会查看此骨架网格物体是否已加载，如果已加载，则切换到它。 */
	/** 运行时在 SkeletalMeshes 和 LinkupCache 数组索引之间构建映射表。 */
	UPROPERTY()
	FName PreviewSkelMeshName;

	/** Holds the name of the skeletal mesh whose reference skeleton best matches the TrackBoneName array. */
	/** 保存其参考骨架与 TrackBoneName 数组最匹配的骨架网格物体的名称。 */
	/** 运行时在 SkeletalMeshes 和 LinkupCache 数组索引之间构建映射表。 */
	UPROPERTY()
	FName BestRatioSkelMeshName;

	/**
	 * Find a mesh linkup table (mapping of sequence tracks to bone indices) for a particular SkeletalMesh
	 * If one does not already exist, create it now.
	 *
	 * @param SkelMesh SkeletalMesh to look for linkup with.
	 *
	 * @return Index of Linkup between mesh and animation set.
	 */
	virtual int32 GetMeshLinkupIndex(class USkeletalMesh* SkelMesh);

	/** 获取网格与动画集的拟合程度的比率 */
public:
	/** Runtime built mapping table between SkeletalMeshes, and LinkupCache array indices. */
	/** 运行时在 SkeletalMeshes 和 LinkupCache 数组索引之间构建映射表。 */
	// Do change private - they will go away
 // 一定要改变私人 - 他们会消失
	TMap<FName,int32> SkelMesh2LinkupCache;
	/** 获取网格与动画集的拟合程度的比率 */

	//~ Begin UObject Interface
 // ~ 开始 UObject 接口
	virtual void PostLoad() override;
	//~ End UObject Interface
 // ~ 结束 UObject 接口
	
	//~ Begin UAnimSet Interface
 // ~ 开始 UAnimSet 界面
	/**
	 * See if we can play sequences from this AnimSet on the provided USkeletalMesh.
	 * Returns true if there is a bone in SkelMesh for every track in the AnimSet,
	 * or there is a track of animation for every bone of the SkelMesh.
	 * 
	 * @param	SkelMesh	USkeletalMesh to compare the AnimSet against.
	 * @return				true if animation set can play on supplied USkeletalMesh, false if not.
	 */
	bool CanPlayOnSkeletalMesh(USkeletalMesh* SkelMesh) const;

	/** Get Ratio of how much that mesh fits that animation set */
	/** 获取网格与动画集的拟合程度的比率 */
	float GetSkeletalMeshMatchRatio(USkeletalMesh* SkelMesh) const;

	/**
	 * Returns the AnimSequence with the specified name in this set.
	 * 
	 * @param		SequenceName	Name of sequence to find.
	/** 找到所有 AnimSet 并刷新其 LinkupCache 的 Util，然后在所有 SkeletalMeshComponent 上调用 InitAnimTree。 */
	 * @return						Pointer to AnimSequence with desired name, or NULL if sequence was not found.
	 */
	UAnimSequence* FindAnimSequence(FName SequenceName);
	/** 找到所有 AnimSet 并刷新其 LinkupCache 的 Util，然后在所有 SkeletalMeshComponent 上调用 InitAnimTree。 */

	/**
	 * @return		The track index for the bone with the supplied name, or INDEX_NONE if no track exists for that bone.
	 */
	int32 FindTrackWithName(FName BoneName) const
	{
		return TrackBoneNames.Find( BoneName );
	}

	/**
	 * Clears all sequences and resets the TrackBoneNames table.
	 */
	void ResetAnimSet();

	/** 
	 * Properly remove an AnimSequence from an AnimSet, and updates references it might have.
	 * @return true if AnimSequence was properly removed, false if it wasn't found.
	 */
	bool RemoveAnimSequenceFromAnimSet(UAnimSequence* AnimSeq);

	/** Util that finds all AnimSets and flushes their LinkupCache, then calls InitAnimTree on all SkeletalMeshComponents. */
	/** 找到所有 AnimSet 并刷新其 LinkupCache 的 Util，然后在所有 SkeletalMeshComponent 上调用 InitAnimTree。 */
	static void ClearAllAnimSetLinkupCaches();

	friend struct FAnimSetMeshLinkup;
};



