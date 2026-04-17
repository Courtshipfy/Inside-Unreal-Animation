// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Skeleton.cpp: Skeleton features
=============================================================================*/ 

#include "Animation/Skeleton.h"

#include "AnimationSequenceCompiler.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Components/SkinnedMeshComponent.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/LinkerLoad.h"
#include "Engine/AssetUserData.h"
#include "Modules/ModuleManager.h"
#include "Engine/DataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/BlendProfile.h"
#include "Engine/SkinnedAsset.h"
#include "Logging/MessageLog.h"
#include "ComponentReregisterContext.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Misc/ScopeRWLock.h"
#include "Misc/ScopedSlowTask.h"
#include "Animation/AnimBlueprint.h"
#include "UObject/AnimObjectVersion.h"
#include "EngineUtils.h"
#include "Animation/SkeletonRemappingRegistry.h"
#include "UObject/UE5MainStreamObjectVersion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Skeleton)

#define LOCTEXT_NAMESPACE "Skeleton"
#define ROOT_BONE_PARENT	INDEX_NONE


#if WITH_EDITOR
const FName USkeleton::AnimNotifyTag = FName(TEXT("AnimNotifyList"));
const FString USkeleton::AnimNotifyTagDelimiter = TEXT(";");

const FName USkeleton::AnimSyncMarkerTag = FName(TEXT("AnimSyncMarkerList"));
const FString USkeleton::AnimSyncMarkerTagDelimiter = TEXT(";");

const FName USkeleton::CurveNameTag = FName(TEXT("CurveNameList"));
const FString USkeleton::CurveTagDelimiter = TEXT(";");

const FName USkeleton::AttributeTag = FName(TEXT("AttributeList"));

const FName USkeleton::CompatibleSkeletonsNameTag = FName(TEXT("CompatibleSkeletonList"));
const FString USkeleton::CompatibleSkeletonsTagDelimiter = TEXT(";");
#endif 

#if WITH_EDITORONLY_DATA
FAreAllSkeletonsCompatible USkeleton::AreAllSkeletonsCompatibleDelegate;
#endif

const FName FAnimSlotGroup::DefaultGroupName = FName(TEXT("DefaultGroup"));
const FName FAnimSlotGroup::DefaultSlotName = FName(TEXT("DefaultSlot"));

TAutoConsoleVariable<bool> CVarAllowIncompatibleSkeletalMeshMerge(TEXT("a.Skeleton.AllowIncompatibleSkeletalMeshMerge"), 0, TEXT("When importing or otherwise merging in skeletal mesh bones, allow 'incompatible' hierarchies with bone insertions."));

void SerializeReferencePose(FArchive& Ar, FReferencePose& P, UObject* Outer)
{
	Ar.UsingCustomVersion(FAnimPhysObjectVersion::GUID);

	Ar << P.PoseName;
	Ar << P.ReferencePose;
#if WITH_EDITORONLY_DATA
	//TODO: we should use strip flags but we need to rev the serialization version
	//TODO：我们应该使用剥离标志，但我们需要修改序列化版本
	if (!Ar.IsCooking() && (!Ar.IsLoading() || !Outer->GetOutermost()->bIsCookedForEditor))
	{
		if (Ar.IsLoading() && Ar.CustomVer(FAnimPhysObjectVersion::GUID) < FAnimPhysObjectVersion::ChangeRetargetSourceReferenceToSoftObjectPtr)
		{
			USkeletalMesh* SourceMesh = nullptr;
			Ar << SourceMesh;
			P.SourceReferenceMesh = SourceMesh;
		}
		else
		{
			// Scope the soft pointer serialization so we can tag it as editor only
			// 确定软指针序列化的范围，以便我们可以将其仅标记为编辑器
			FName PackageName;
			FName PropertyName;
			ESoftObjectPathCollectType CollectType = ESoftObjectPathCollectType::AlwaysCollect;
			ESoftObjectPathSerializeType SerializeType = ESoftObjectPathSerializeType::AlwaysSerialize;
			FSoftObjectPathThreadContext& ThreadContext = FSoftObjectPathThreadContext::Get();
			ThreadContext.GetSerializationOptions(PackageName, PropertyName, CollectType, SerializeType);
			FSoftObjectPathSerializationScope SerializationScope(PackageName, PropertyName, ESoftObjectPathCollectType::EditorOnlyCollect, SerializeType);
			Ar << P.SourceReferenceMesh;
		}
	}
#endif
}

const TCHAR* SkipPrefix(const FString& InName)
{
	const int32 PrefixLength = VirtualBoneNameHelpers::VirtualBonePrefix.Len();
	check(InName.Len() > PrefixLength);
	return &InName[PrefixLength];
}

namespace VirtualBoneNameHelpers
{
	const FString VirtualBonePrefix(TEXT("VB "));

	FString AddVirtualBonePrefix(const FString& InName)
	{
		return VirtualBonePrefix + InName;
	}

	FName RemoveVirtualBonePrefix(const FString& InName)
	{
		return FName(SkipPrefix(InName));
	}

	bool CheckVirtualBonePrefix(const FString& InName)
	{
		return InName.StartsWith(VirtualBonePrefix);
	}
}

#if WITH_EDITORONLY_DATA
bool USkeleton::IsCompatibleForEditor(const USkeleton* InSkeleton) const
{
	return IsCompatibleForEditor(FAssetData(InSkeleton));
}

bool USkeleton::IsCompatibleForEditor(const FAssetData& AssetData, const TCHAR* InTag) const
{
	if(AssetData.GetClass() == USkeleton::StaticClass())
	{
		return IsCompatibleForEditor(AssetData.GetExportTextName());
	}
	else
	{
		return IsCompatibleForEditor(AssetData.GetTagValueRef<FString>(InTag));
	}
}

bool USkeleton::IsCompatibleForEditor(const FString& SkeletonAssetString) const
{
	// First check against itself.
	// 首先检查自身。
	TStringBuilder<128> SkeletonStringBuilder;
	FAssetData(this, FAssetData::ECreationFlags::SkipAssetRegistryTagsGathering).GetExportTextName(SkeletonStringBuilder);
	const TCHAR* SkeletonString = SkeletonStringBuilder.ToString();
	if (SkeletonAssetString == SkeletonString)
	{
		return true;
	}

	// Let the global delegate override any per-skeleton settings
	// 让全局委托覆盖任何每个骨架设置
	if(AreAllSkeletonsCompatibleDelegate.IsBound() && AreAllSkeletonsCompatibleDelegate.Execute())
	{
		return true;
	}

	// Now check against the list of compatible skeletons and see if we're dealing with the same asset.
	// 现在检查兼容骨架列表，看看我们是否正在处理相同的资产。
	if(CompatibleSkeletons.Num() > 0)
	{
		const FSoftObjectPath InPath(SkeletonAssetString);
		for (const TSoftObjectPtr<USkeleton>& CompatibleSkeleton : CompatibleSkeletons)
		{
			if (CompatibleSkeleton.ToSoftObjectPath() == InPath)
			{
				return true;
			}
		}
	}

	// Check if the other skeleton is compatible with this via the asset registry
	// 通过资产注册表检查其他骨架是否与此兼容
	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	constexpr bool bIncludeOnlyOnDiskAssets = true;
	const FAssetData SkeletonAssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(*SkeletonAssetString), bIncludeOnlyOnDiskAssets);
	const FString TagValue = SkeletonAssetData.GetTagValueRef<FString>(USkeleton::CompatibleSkeletonsNameTag);
	if (!TagValue.IsEmpty())
	{
		TArray<FString> OtherCompatibleSkeletons;
		if (TagValue.ParseIntoArray(OtherCompatibleSkeletons, *USkeleton::CompatibleSkeletonsTagDelimiter, true) > 0)
		{
			for (const FString& OtherCompatibleSkeleton : OtherCompatibleSkeletons)
			{
				if (OtherCompatibleSkeleton == SkeletonString)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool USkeleton::ShouldFilterAsset(const FAssetData& InAssetData, const TCHAR* InTag) const
{
	return !IsCompatibleForEditor(InAssetData, InTag);
}

void USkeleton::GetCompatibleSkeletonAssets(TArray<FAssetData>& OutAssets) const
{
	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	// If we are compatible with all, then just add all skeletons
	// 如果我们兼容所有，那么只需添加所有骨架
	if(AreAllSkeletonsCompatibleDelegate.IsBound() && AreAllSkeletonsCompatibleDelegate.Execute())
	{
		TArray<FAssetData> AllSkeletons;
		FARFilter ARFilter;
		ARFilter.ClassPaths.Add(USkeleton::StaticClass()->GetClassPathName());
		AssetRegistry.GetAssets(ARFilter, AllSkeletons);

		OutAssets.Append(AllSkeletons);
	}
	else
	{
		// Always add 'this'
		// 始终添加“这个”
		FAssetData AssetDataThis(this);
		OutAssets.Add(AssetDataThis);
		
		// Add skeletons in our compatibility list
		// 在我们的兼容性列表中添加骨架
		for(TSoftObjectPtr<USkeleton> CompatibleSkeleton : CompatibleSkeletons)
		{
			const FAssetData SkeletonAssetData = AssetRegistry.GetAssetByObjectPath(CompatibleSkeleton.ToSoftObjectPath());
			if(SkeletonAssetData.IsValid())
			{
				OutAssets.Add(SkeletonAssetData);
			}
		}

		// Add skeletons where we are listed in their compatibility list
		// 添加我们在其兼容性列表中列出的骨架
		{
			TArray<FAssetData> SkeletonsCompatibleWithThis;
			FARFilter ARFilter;
			ARFilter.ClassPaths.Add(USkeleton::StaticClass()->GetClassPathName());
			ARFilter.TagsAndValues.Add(CompatibleSkeletonsNameTag, AssetDataThis.GetExportTextName());
			AssetRegistry.GetAssets(ARFilter, SkeletonsCompatibleWithThis);

			OutAssets.Append(SkeletonsCompatibleWithThis);
		}
	}
}

void USkeleton::GetCompatibleAssets(UClass* AssetClass, const TCHAR* InTag, TArray<FAssetData>& OutAssets) const
{
	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	
	if(AreAllSkeletonsCompatibleDelegate.IsBound() && AreAllSkeletonsCompatibleDelegate.Execute())
	{
		// Compatible with all, so return all assets of type
		// 兼容所有，因此返回该类型的所有资产
		FARFilter ARFilter;
		ARFilter.ClassPaths.Add(AssetClass->GetClassPathName());
		ARFilter.bRecursiveClasses = true;
		AssetRegistry.GetAssets(ARFilter, OutAssets);
	}
	else
	{
		FARFilter ARFilter;
		ARFilter.ClassPaths.Add(AssetClass->GetClassPathName());
		ARFilter.bRecursiveClasses = true;

		TArray<FAssetData> CompatibleSkeletonAssets;
		GetCompatibleSkeletonAssets(CompatibleSkeletonAssets);

		for (const FAssetData& CompatibleSkeleton : CompatibleSkeletonAssets)
		{
			ARFilter.TagsAndValues.Add(InTag, CompatibleSkeleton.GetExportTextName());
		}

		AssetRegistry.GetAssets(ARFilter, OutAssets);
	}
}

#endif

void USkeleton::AddCompatibleSkeleton(const USkeleton* SourceSkeleton)
{
	CompatibleSkeletons.AddUnique(const_cast<USkeleton*>(SourceSkeleton));
}

void USkeleton::AddCompatibleSkeletonSoft(const TSoftObjectPtr<USkeleton>& SourceSkeleton)
{
	CompatibleSkeletons.AddUnique(SourceSkeleton);
}

void USkeleton::RemoveCompatibleSkeleton(const USkeleton* SourceSkeleton)
{
	CompatibleSkeletons.Remove(const_cast<USkeleton*>(SourceSkeleton));
}

void USkeleton::RemoveCompatibleSkeleton(const TSoftObjectPtr<USkeleton>& SourceSkeleton)
{
	CompatibleSkeletons.Remove(SourceSkeleton);
}

USkeleton::USkeleton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CachedSoftObjectPtr = TSoftObjectPtr<USkeleton>(this);

#if WITH_EDITORONLY_DATA
	PreviewForwardAxis = EAxis::Y;
#endif

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FCoreUObjectDelegates::OnPackageReloaded.AddStatic(&USkeleton::HandlePackageReloaded);
	}
}

void USkeleton::BeginDestroy()
{
#if WITH_EDITOR
	UE::Anim::FAnimSequenceCompilingManager::Get().FinishCompilation({this});
#endif // WITH_EDITOR
	
	Super::BeginDestroy();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FCoreUObjectDelegates::OnPackageReloaded.RemoveAll(this);
	}
}

void USkeleton::PostInitProperties()
{
	Super::PostInitProperties();

	// this gets called after constructor, and this data can get
	// 这在构造函数之后被调用，并且该数据可以获得
	// serialized back if this already has Guid
	// 如果已经有 Guid 则序列化回来
	if (!IsTemplate())
	{
		RegenerateGuid();
	}
}

bool USkeleton::IsPostLoadThreadSafe() const
{
	return WITH_EDITORONLY_DATA == 0;
}

void USkeleton::PostLoad()
{
	Super::PostLoad();
	LLM_SCOPE(ELLMTag::Animation);

#if WITH_EDITORONLY_DATA
	if( GetLinker() && (GetLinker()->UEVer() < VER_UE4_REFERENCE_SKELETON_REFACTOR) )
	{
		// Convert RefLocalPoses & BoneTree to FReferenceSkeleton
		// 将 RefLocalPoses 和 BoneTree 转换为 FReferenceSkeleton
		ConvertToFReferenceSkeleton();
	}
#endif

	// catch any case if guid isn't valid
	// 如果 guid 无效，则捕获任何情况
	check(Guid.IsValid());

	// Cleanup CompatibleSkeletons for convenience. This basically removes any soft object pointers that has an invalid soft object name.
	// 为了方便起见，清理兼容的骨架。这基本上会删除任何具有无效软对象名称的软对象指针。
	CompatibleSkeletons = CompatibleSkeletons.FilterByPredicate([](const TSoftObjectPtr<USkeleton>& Skeleton)
	{
		return Skeleton.ToSoftObjectPath().IsValid();
	});

#if WITH_EDITORONLY_DATA
	if(GetLinkerCustomVersion(FUE5MainStreamObjectVersion::GUID) < FUE5MainStreamObjectVersion::AnimationRemoveSmartNames)
	{
		// Move curve metadata over to asset user data
		// 将曲线元数据移至资产用户数据
		UAnimCurveMetaData* AnimCurveMetaData = GetOrCreateCurveMetaDataObject();
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		for(const TPair<FName, FSmartNameMapping>& NameMappingPair : SmartNames_DEPRECATED.NameMappings)
		{
			for(const TPair<FName, FCurveMetaData>& NameMetaDataPair : NameMappingPair.Value.CurveMetaDataMap)
			{
				AnimCurveMetaData->CurveMetaData.Add(NameMetaDataPair.Key, NameMetaDataPair.Value);
			}
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
	else
	{
		// Ensure we have curve metadata, even if empty, so we can correctly pick up older objects in the asset registry
		// 确保我们有曲线元数据，即使是空的，这样我们就可以正确地拾取资产注册表中的旧对象
		GetOrCreateCurveMetaDataObject();
	}
#endif

	// refresh linked bone indices
	// [翻译失败: refresh linked bone indices]
	RefreshSkeletonMetaData();

}

void USkeleton::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		// regenerate Guid
		// [翻译失败: regenerate Guid]
		RegenerateGuid();
	}
}

void USkeleton::Serialize( FArchive& Ar )
{
	Ar.UsingCustomVersion(FAnimObjectVersion::GUID);
	Ar.UsingCustomVersion(FUE5MainStreamObjectVersion::GUID);

	DECLARE_SCOPE_CYCLE_COUNTER( TEXT("USkeleton::Serialize"), STAT_Skeleton_Serialize, STATGROUP_LoadTime );

	Super::Serialize(Ar);

	if( Ar.UEVer() >= VER_UE4_REFERENCE_SKELETON_REFACTOR )
	{
		Ar << ReferenceSkeleton;
	}

	if (Ar.UEVer() >= VER_UE4_FIX_ANIMATIONBASEPOSE_SERIALIZATION)
	{
		// Load Animation RetargetSources
		// 加载动画重定位源
		if (Ar.IsLoading())
		{
			int32 NumOfRetargetSources;
			Ar << NumOfRetargetSources;

			FName RetargetSourceName;
			FReferencePose RetargetSource;
			AnimRetargetSources.Empty(NumOfRetargetSources);
			for (int32 Index=0; Index<NumOfRetargetSources; ++Index)
			{
				Ar << RetargetSourceName;
				SerializeReferencePose(Ar, RetargetSource, this);

				AnimRetargetSources.Add(RetargetSourceName, RetargetSource);
			}
		}
		else 
		{
			int32 NumOfRetargetSources = AnimRetargetSources.Num();
			Ar << NumOfRetargetSources;

			for (auto Iter = AnimRetargetSources.CreateIterator(); Iter; ++Iter)
			{
				Ar << Iter.Key();
				SerializeReferencePose(Ar, Iter.Value(), this);
			}
		}
	}
	else
	{
		// this is broken, but we have to keep it to not corrupt content. 
		// [翻译失败: this is broken, but we have to keep it to not corrupt content.]
		for (auto Iter = AnimRetargetSources.CreateIterator(); Iter; ++Iter)
		{
			Ar << Iter.Key();
			SerializeReferencePose(Ar, Iter.Value(), this);
		}
	}

	if (Ar.UEVer() < VER_UE4_SKELETON_GUID_SERIALIZATION)
	{
		UE_LOG(LogAnimation, Warning, TEXT("Skeleton '%s' has not been saved since version 'VER_UE4_SKELETON_GUID_SERIALIZATION' This asset will not cook deterministically until it is resaved."), *GetPathName());
		RegenerateGuid();
	}
	else
	{
		Ar << Guid;
	}

	// If we should be using smartnames, serialize the mappings
	// [翻译失败: If we should be using smartnames, serialize the mappings]
	if(Ar.UEVer() >= VER_UE4_SKELETON_ADD_SMARTNAMES)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		SmartNames_DEPRECATED.Serialize(Ar, IsTemplate());
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	// Build look up table between Slot nodes and their Group.
	// 在插槽节点及其组之间建立查找表。
	if(Ar.UEVer() < VER_UE4_FIX_SLOT_NAME_DUPLICATION)
	{
		// In older assets we may have duplicates, remove these while building the map.
		// 在较旧的资产中，我们可能有重复项，请在构建地图时删除它们。
		BuildSlotToGroupMap(true);
	}
	else
	{
		BuildSlotToGroupMap();
	}

#if WITH_EDITORONLY_DATA
	if (Ar.UEVer() < VER_UE4_SKELETON_ASSET_PROPERTY_TYPE_CHANGE)
	{
		PreviewAttachedAssetContainer.SaveAttachedObjectsFromDeprecatedProperties();
	}
#endif

	if (Ar.CustomVer(FAnimObjectVersion::GUID) >= FAnimObjectVersion::StoreMarkerNamesOnSkeleton)
	{
		FStripDataFlags StripFlags(Ar);
		if (!StripFlags.IsEditorDataStripped())
		{
			Ar << ExistingMarkerNames;
		}
	}

	if (Ar.IsLoading())
	{
		const bool bRebuildNameMap = false;
		ReferenceSkeleton.RebuildRefSkeleton(this, bRebuildNameMap);
	}
}

#if WITH_EDITOR
void USkeleton::PreEditUndo()
{
	// Undoing so clear cached data as it will now be stale
	// 撤消如此清晰的缓存数据，因为它现在已经过时了
	ClearCacheData();
}

void USkeleton::PostEditUndo()
{
	Super::PostEditUndo();

	//If we were undoing virtual bone changes then we need to handle stale cache data
	//[翻译失败: If we were undoing virtual bone changes then we need to handle stale cache data]
	// Cached data is cleared in PreEditUndo to make sure it is done before any object hits their PostEditUndo
	// [翻译失败: Cached data is cleared in PreEditUndo to make sure it is done before any object hits their PostEditUndo]
	HandleVirtualBoneChanges();
}
#endif // WITH_EDITOR

/** Remove this function when VER_UE4_REFERENCE_SKELETON_REFACTOR is removed. */
/** [翻译失败: Remove this function when VER_UE4_REFERENCE_SKELETON_REFACTOR is removed.] */
void USkeleton::ConvertToFReferenceSkeleton()
{
#if WITH_EDITORONLY_DATA
	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

	check( BoneTree.Num() == RefLocalPoses_DEPRECATED.Num() );

	const int32 NumRefBones = RefLocalPoses_DEPRECATED.Num();
	ReferenceSkeleton.Empty();

	for(int32 BoneIndex=0; BoneIndex<NumRefBones; BoneIndex++)
	{
		const FBoneNode& BoneNode = BoneTree[BoneIndex];
		FMeshBoneInfo BoneInfo(BoneNode.Name_DEPRECATED, BoneNode.Name_DEPRECATED.ToString(), BoneNode.ParentIndex_DEPRECATED);
		const FTransform& BoneTransform = RefLocalPoses_DEPRECATED[BoneIndex];

		// All should be good. Parents before children, no duplicate bones?
		// 一切都应该很好。父母先于孩子，没有重复的骨头吗？
		RefSkelModifier.Add(BoneInfo, BoneTransform);
	}

	// technically here we should call 	RefershAllRetargetSources(); but this is added after 
	// 从技术上讲，我们应该调用 RefershAllRetargetSources();但这是在之后添加的
	// VER_UE4_REFERENCE_SKELETON_REFACTOR, this shouldn't be needed. It shouldn't have any 
	// VER_UE4_REFERENCE_SKELETON_REFACTOR，不需要这个。不应该有任何
	// AnimatedRetargetSources
	// 动画重定位源
	ensure (AnimRetargetSources.Num() == 0);
#endif
}

const FSkeletonRemapping* USkeleton::GetSkeletonRemapping(const USkeleton* SourceSkeleton) const
{
	return &UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(SourceSkeleton, this);
}

#if WITH_EDITOR

bool USkeleton::RemoveMarkerName(FName MarkerName)
{
	if(ExistingMarkerNames.Contains(MarkerName))
	{
		Modify();
	}

	return ExistingMarkerNames.Remove(MarkerName) != 0;
}

bool USkeleton::RenameMarkerName(FName InOldName, FName InNewName)
{
	if(ExistingMarkerNames.Contains(InOldName))
	{
		Modify();
	}

	if(ExistingMarkerNames.Contains(InNewName))
	{
		return ExistingMarkerNames.Remove(InOldName) != 0;
	}

	for(FName& MarkerName : ExistingMarkerNames)
	{
		if(MarkerName == InOldName)
		{
			MarkerName = InNewName;
			return true;
		}
	}

	return false;
}

#endif

bool USkeleton::DoesParentChainMatch(int32 StartBoneIndex, const USkinnedAsset* InSkinnedAsset) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USkeleton::DoesParentChainMatch);
	const FReferenceSkeleton& SkeletonRefSkel = ReferenceSkeleton;
	const FReferenceSkeleton& MeshRefSkel = InSkinnedAsset->GetRefSkeleton();

	// if start is root bone
	// 如果开始是根骨骼
	if ( StartBoneIndex == 0 )
	{
		// verify name of root bone matches
		// 验证根骨骼的名称是否匹配
		return (SkeletonRefSkel.GetBoneName(0) == MeshRefSkel.GetBoneName(0));
	}

	int32 SkeletonBoneIndex = StartBoneIndex;
	// If skeleton bone is not found in mesh, fail.
	// 如果网格中没有找到骨架，则失败。
	int32 MeshBoneIndex = MeshRefSkel.FindBoneIndex( SkeletonRefSkel.GetBoneName(SkeletonBoneIndex) );
	if( MeshBoneIndex == INDEX_NONE )
	{
		return false;
	}
	do
	{
		// verify if parent name matches
		// 验证父母姓名是否匹配
		int32 ParentSkeletonBoneIndex = SkeletonRefSkel.GetParentIndex(SkeletonBoneIndex);
		int32 ParentMeshBoneIndex = MeshRefSkel.GetParentIndex(MeshBoneIndex);

		// if one of the parents doesn't exist, make sure both end. Otherwise fail.
		// 如果父母之一不存在，请确保两者都结束。否则失败。
		if( (ParentSkeletonBoneIndex == INDEX_NONE) || (ParentMeshBoneIndex == INDEX_NONE) )
		{
			return (ParentSkeletonBoneIndex == ParentMeshBoneIndex);
		}

		// If parents are not named the same, fail.
		// 如果父母的名字不同，则失败。
		if( SkeletonRefSkel.GetBoneName(ParentSkeletonBoneIndex) != MeshRefSkel.GetBoneName(ParentMeshBoneIndex) )
		{
			return false;
		}

		// move up
		// 向上移动
		SkeletonBoneIndex = ParentSkeletonBoneIndex;
		MeshBoneIndex = ParentMeshBoneIndex;
	} while ( true );
}

bool USkeleton::IsCompatibleMesh(const USkinnedAsset* InSkinnedAsset, bool bDoParentChainCheck) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USkeleton::IsCompatibleMesh);
	// at least % of bone should match 
	// 至少 % 的骨骼应该匹配
	int32 NumOfBoneMatches = 0;

	const FReferenceSkeleton& SkeletonRefSkel = ReferenceSkeleton;
	const FReferenceSkeleton& MeshRefSkel = InSkinnedAsset->GetRefSkeleton();
	const int32 NumBones = MeshRefSkel.GetRawBoneNum();

	// first ensure the parent exists for each bone
	// 首先确保每个骨骼都存在父级
	for (int32 MeshBoneIndex=0; MeshBoneIndex<NumBones; MeshBoneIndex++)
	{
		FName MeshBoneName = MeshRefSkel.GetBoneName(MeshBoneIndex);
		// See if Mesh bone exists in Skeleton.
		// 查看 Skeleton 中是否存在 Mesh 骨骼。
		int32 SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex( MeshBoneName );

		// if found, increase num of bone matches count
		// 如果找到，则增加骨骼匹配数
		if( SkeletonBoneIndex != INDEX_NONE )
		{
			++NumOfBoneMatches;

			// follow the parent chain to verify the chain is same
			// 跟踪父链以验证链是否相同
			if(bDoParentChainCheck && !DoesParentChainMatch(SkeletonBoneIndex, InSkinnedAsset))
			{
				UE_LOG(LogAnimation, Verbose, TEXT("%s : Hierarchy does not match."), *MeshBoneName.ToString());
				return false;
			}
		}
		else
		{
			int32 CurrentBoneId = MeshBoneIndex;
			// if not look for parents that matches
			// 如果不寻找匹配的父母
			while (SkeletonBoneIndex == INDEX_NONE && CurrentBoneId != INDEX_NONE)
			{
				// find Parent one see exists
				// 找到父级一看到存在
				const int32 ParentMeshBoneIndex = MeshRefSkel.GetParentIndex(CurrentBoneId);
				if ( ParentMeshBoneIndex != INDEX_NONE )
				{
					// @TODO: make sure RefSkeleton's root ParentIndex < 0 if not, I'll need to fix this by checking TreeBoneIdx
					// @TODO：确保 RefSkeleton 的根 ParentIndex < 0 如果不是，我需要通过检查 TreeBoneIdx 来修复此问题
					FName ParentBoneName = MeshRefSkel.GetBoneName(ParentMeshBoneIndex);
					SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex(ParentBoneName);
				}

				// root is reached
				// 已到达根
				if( ParentMeshBoneIndex == 0 )
				{
					break;
				}
				else
				{
					CurrentBoneId = ParentMeshBoneIndex;
				}
			}

			// still no match, return false, no parent to look for
			// 仍然没有匹配，返回 false，没有父级可供查找
			if( SkeletonBoneIndex == INDEX_NONE )
			{
				UE_LOG(LogAnimation, Verbose, TEXT("%s : Missing joint on skeleton.  Make sure to assign to the skeleton."), *MeshBoneName.ToString());
				return false;
			}

			// second follow the parent chain to verify the chain is same
			// 第二步跟随父链验证链是否相同
			if (bDoParentChainCheck && !DoesParentChainMatch(SkeletonBoneIndex, InSkinnedAsset))
			{
				UE_LOG(LogAnimation, Verbose, TEXT("%s : Hierarchy does not match."), *MeshBoneName.ToString());
				return false;
			}
		}
	}

	// originally we made sure at least matches more than 50% 
	// 最初我们确保至少匹配超过 50%
	// but then follower components can't play since they're only partial
	// 但跟随者组件无法播放，因为它们只是部分的
	// if the hierarchy matches, and if it's more then 1 bone, we allow
	// 如果层次结构匹配，并且超过 1 个骨骼，我们允许
	return (NumOfBoneMatches > 0);
}

void USkeleton::ClearCacheData()
{
	{
		UE::TWriteScopeLock Lock(SkinnedAssetLinkupCacheLock);
		SkinnedAssetLinkupCache.Empty();
	}
}

const FSkeletonToMeshLinkup& USkeleton::FindOrAddMeshLinkupData(const USkinnedAsset* InSkinnedAsset)
{
	const TUniquePtr<FSkeletonToMeshLinkup>* SkeletonToMeshLinkupPtr = nullptr;

	{
		UE::TReadScopeLock Lock(SkinnedAssetLinkupCacheLock);
		SkeletonToMeshLinkupPtr = SkinnedAssetLinkupCache.Find(InSkinnedAsset);
	}

	return (SkeletonToMeshLinkupPtr != nullptr)
		? *SkeletonToMeshLinkupPtr->Get()
		: AddMeshLinkupData(InSkinnedAsset);
}

const FSkeletonToMeshLinkup& USkeleton::AddMeshLinkupData(const USkinnedAsset* InSkinnedAsset)
{
	TUniquePtr<FSkeletonToMeshLinkup> TmpLinkup = MakeUnique<FSkeletonToMeshLinkup>();
	BuildLinkupData(InSkinnedAsset, *TmpLinkup.Get());

	const TObjectKey<USkinnedAsset> SkinnedAssetKey(InSkinnedAsset);
	const uint32 SkinnedAssetHash = GetTypeHash(SkinnedAssetKey);

	{
		UE::TWriteScopeLock Lock(SkinnedAssetLinkupCacheLock);
		if (const TUniquePtr<FSkeletonToMeshLinkup>* SkeletonToMeshLinkupPtr = SkinnedAssetLinkupCache.FindByHash(SkinnedAssetHash, SkinnedAssetKey))
		{
			// While we were building the linkup data, another thread beat us to it
			// [翻译失败: While we were building the linkup data, another thread beat us to it]
			// Discard the work we did to avoid freeing memory other threads might now be using
			// [翻译失败: Discard the work we did to avoid freeing memory other threads might now be using]
			return *SkeletonToMeshLinkupPtr->Get();
		}

		return *SkinnedAssetLinkupCache.AddByHash(SkinnedAssetHash, SkinnedAssetKey, MoveTemp(TmpLinkup)).Get();
	}
}

void USkeleton::RemoveLinkup(const USkinnedAsset* InSkinnedAsset)
{
	{
		UE::TWriteScopeLock Lock(SkinnedAssetLinkupCacheLock);
		SkinnedAssetLinkupCache.Remove(InSkinnedAsset);
	}
}

void USkeleton::BuildLinkupData(const USkinnedAsset* InSkinnedAsset, FSkeletonToMeshLinkup& NewMeshLinkup)
{
	const FReferenceSkeleton& SkeletonRefSkel = ReferenceSkeleton;
	const FReferenceSkeleton& MeshRefSkel = InSkinnedAsset->GetRefSkeleton();

	// First, make sure the Skeleton has all the bones the SkeletalMesh possesses.
	// [翻译失败: First, make sure the Skeleton has all the bones the SkeletalMesh possesses.]
	// This can get out of sync if a mesh was imported on that Skeleton, but the Skeleton was not saved.
	// 如果在该骨架上导入了网格，但未保存该骨架，则这可能会不同步。

	const int32 NumMeshBones = MeshRefSkel.GetNum();
	NewMeshLinkup.MeshToSkeletonTable.Empty(NumMeshBones);
	NewMeshLinkup.MeshToSkeletonTable.AddUninitialized(NumMeshBones);

	for (int32 MeshBoneIndex = 0; MeshBoneIndex < NumMeshBones; MeshBoneIndex++)
	{
		const FName MeshBoneName = MeshRefSkel.GetBoneName(MeshBoneIndex);
		int32 SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex(MeshBoneName);

		if (SkeletonBoneIndex == INDEX_NONE)
		{
			bool bSpawnedErrorMessage = false;

#if WITH_EDITOR
			// If we're in editor, and skeleton is missing a bone, fix it.
			// [翻译失败: If we're in editor, and skeleton is missing a bone, fix it.]
			// not currently supported in-game.
			// [翻译失败: not currently supported in-game.]

			static FName NAME_LoadErrors("LoadErrors");

			if (IsInGameThread() || IsInSlateThread())
			{
				FMessageLog LoadErrors(NAME_LoadErrors);

				TSharedRef<FTokenizedMessage> Message = LoadErrors.Info();
				Message->AddToken(FTextToken::Create(LOCTEXT("SkeletonBuildLinkupMissingBones1", "The Skeleton ")));
				Message->AddToken(FAssetNameToken::Create(GetPathName(), FText::FromString(GetNameSafe(this))));
				Message->AddToken(FTextToken::Create(LOCTEXT("SkeletonBuildLinkupMissingBones2", " is missing bones that SkeletalMesh ")));
				Message->AddToken(FAssetNameToken::Create(InSkinnedAsset->GetPathName(), FText::FromString(GetNameSafe(InSkinnedAsset))));
				Message->AddToken(FTextToken::Create(LOCTEXT("SkeletonBuildLinkupMissingBones3", "  needs. They will be added now. Please save the Skeleton!")));
				LoadErrors.Open();

				bSpawnedErrorMessage = true;
			}

			// Re-add all SkelMesh bones to the Skeleton.
			// 将所有 SkelMesh 骨骼重新添加到骨架中。
			MergeAllBonesToBoneTree(InSkinnedAsset);

			// Fix missing bone.
			// 修复缺失的骨头。
			SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex(MeshBoneName);
#endif // WITH_EDITOR

			if (!bSpawnedErrorMessage)
			{
				// If we're not in editor, we still want to know which skeleton is missing a bone.
				// 如果我们不在编辑器中，我们仍然想知道哪个骨架缺少一根骨头。
				UE_LOG(LogAnimation, Error, TEXT("USkeleton::BuildLinkup: The Skeleton %s, is missing bones that SkeletalMesh %s needs. MeshBoneName %s"),
					*GetNameSafe(this), *GetNameSafe(InSkinnedAsset), *MeshBoneName.ToString());
			}
		}

		NewMeshLinkup.MeshToSkeletonTable[MeshBoneIndex] = SkeletonBoneIndex;
	}

	const int32 NumSkeletonBones = SkeletonRefSkel.GetNum();
	NewMeshLinkup.SkeletonToMeshTable.Empty(NumSkeletonBones);
	NewMeshLinkup.SkeletonToMeshTable.AddUninitialized(NumSkeletonBones);
	
	for (int32 SkeletonBoneIndex=0; SkeletonBoneIndex<NumSkeletonBones; SkeletonBoneIndex++)
	{
		const int32 MeshBoneIndex = MeshRefSkel.FindBoneIndex( SkeletonRefSkel.GetBoneName(SkeletonBoneIndex) );
		NewMeshLinkup.SkeletonToMeshTable[SkeletonBoneIndex] = MeshBoneIndex;
	}
}


void USkeleton::RebuildLinkup(const USkinnedAsset* InSkinnedAsset)
{
	// remove the key
	// 拔掉钥匙
	RemoveLinkup(InSkinnedAsset);
	// build new one
	// 建造新的
	AddMeshLinkupData(InSkinnedAsset);
}

void USkeleton::UpdateReferencePoseFromMesh(const USkinnedAsset* InSkinnedAsset)
{
	check(InSkinnedAsset);
	
	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

	for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetRawBoneNum(); BoneIndex++)
	{
		// find index from ref pose array
		// 从参考位姿数组中查找索引
		const int32 MeshBoneIndex = InSkinnedAsset->GetRefSkeleton().FindRawBoneIndex(ReferenceSkeleton.GetBoneName(BoneIndex));
		if (MeshBoneIndex != INDEX_NONE)
		{
			RefSkelModifier.UpdateRefPoseTransform(BoneIndex, InSkinnedAsset->GetRefSkeleton().GetRefBonePose()[MeshBoneIndex]);
		}
	}

	MarkPackageDirty();
}

bool USkeleton::RecreateBoneTree(USkinnedAsset* InSkinnedAsset)
{
	if( InSkinnedAsset )
	{
		// regenerate Guid
		// [翻译失败: regenerate Guid]
		RegenerateGuid();	
		BoneTree.Empty();
		ReferenceSkeleton.Empty();

		return MergeAllBonesToBoneTree(InSkinnedAsset);
	}

	return false;
}

bool USkeleton::MergeAllBonesToBoneTree(const USkinnedAsset* InSkinnedAsset, bool bShowProgress /*= true*/)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USkeleton::MergeAllBonesToBoneTree);
	if( InSkinnedAsset )
	{
		TArray<int32> RequiredBoneIndices;

		// for now add all in this case. 
		// [翻译失败: for now add all in this case.]
		RequiredBoneIndices.AddUninitialized(InSkinnedAsset->GetRefSkeleton().GetRawBoneNum());
		// gather bone list
		// [翻译失败: gather bone list]
		for (int32 I=0; I<InSkinnedAsset->GetRefSkeleton().GetRawBoneNum(); ++I)
		{
			RequiredBoneIndices[I] = I;
		}

		if( RequiredBoneIndices.Num() > 0 )
		{
			// merge bones to the selected skeleton
			// [翻译失败: merge bones to the selected skeleton]
			return MergeBonesToBoneTree( InSkinnedAsset, RequiredBoneIndices, bShowProgress);
		}
	}

	return false;
}

bool USkeleton::CreateReferenceSkeletonFromMesh(const USkinnedAsset* InSkinnedAsset, const TArray<int32> & RequiredRefBones)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USkeleton::CreateReferenceSkeletonFromMesh);
	// Filter list, we only want bones that have their parents present in this array.
	// 过滤器列表，我们只想要其父级存在于该数组中的骨骼。
	TArray<int32> FilteredRequiredBones; 
	FAnimationRuntime::ExcludeBonesWithNoParents(RequiredRefBones, InSkinnedAsset->GetRefSkeleton(), FilteredRequiredBones);

	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

	if( FilteredRequiredBones.Num() > 0 )
	{
		const int32 NumBones = FilteredRequiredBones.Num();
		ReferenceSkeleton.Empty(NumBones);
		BoneTree.Empty(NumBones);
		BoneTree.AddZeroed(NumBones);

		for (int32 Index=0; Index<FilteredRequiredBones.Num(); Index++)
		{
			const int32& BoneIndex = FilteredRequiredBones[Index];

			FMeshBoneInfo NewMeshBoneInfo = InSkinnedAsset->GetRefSkeleton().GetRefBoneInfo()[BoneIndex];
			// Fix up ParentIndex for our new Skeleton.
			// 修复我们的新骨架的 ParentIndex。
			if( BoneIndex == 0 )
			{
				NewMeshBoneInfo.ParentIndex = INDEX_NONE; // root
			}
			else
			{
				const int32 ParentIndex = InSkinnedAsset->GetRefSkeleton().GetParentIndex(BoneIndex);
				const FName ParentName = InSkinnedAsset->GetRefSkeleton().GetBoneName(ParentIndex);
				NewMeshBoneInfo.ParentIndex = ReferenceSkeleton.FindRawBoneIndex(ParentName);
			}
			RefSkelModifier.Add(NewMeshBoneInfo, InSkinnedAsset->GetRefSkeleton().GetRefBonePose()[BoneIndex]);
		}

		return true;
	}

	return false;
}


bool USkeleton::MergeBonesToBoneTree(const USkinnedAsset* InSkinnedAsset, const TArray<int32> & RequiredRefBones, bool bShowProgress /*= true*/)
{
	// see if it needs all animation data to remap - only happens when bone structure CHANGED - added
	// 看看是否需要所有动画数据重新映射 - 仅在骨骼结构更改时发生 - 添加
	bool bSuccess = false;
	bool bShouldHandleHierarchyChange = false;

	// clear cache data since it won't work anymore once this is done
	// 清除缓存数据，因为完成此操作后它将不再起作用
	ClearCacheData();

	// if it's first time
	// 如果这是第一次
	if( BoneTree.Num() == 0 )
	{
		bSuccess = CreateReferenceSkeletonFromMesh(InSkinnedAsset, RequiredRefBones);
		bShouldHandleHierarchyChange = true;
	}
	else
	{
		// Check if we can merge in the bones
		// 检查我们是否可以合并骨骼
		const bool bDoParentChainCheck = !CVarAllowIncompatibleSkeletalMeshMerge.GetValueOnGameThread();
		if( IsCompatibleMesh(InSkinnedAsset, bDoParentChainCheck) )
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(USkeleton::MergeBonesToBoneTree::CompatibleBranch);
			// Exclude bones who do not have a parent.
			// 排除没有父母的骨骼。
			TArray<int32> FilteredRequiredBones;
			FAnimationRuntime::ExcludeBonesWithNoParents(RequiredRefBones, InSkinnedAsset->GetRefSkeleton(), FilteredRequiredBones);

			// Two modifier passes: add bones that dont already exist, then set bone parents that have changed
			// 两个修改器通道：添加尚不存在的骨骼，然后设置已更改的骨骼父级
			{
				FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

				// Check for bone's existence
				// [翻译失败: Check for bone's existence]
				for (const int32 MeshBoneIndex : FilteredRequiredBones)
				{
					const int32 SkeletonBoneIndex = ReferenceSkeleton.FindRawBoneIndex(InSkinnedAsset->GetRefSkeleton().GetBoneName(MeshBoneIndex));
					
					// Bone doesn't already exist. Add it.
					// 骨头还不存在。添加它。
					if( SkeletonBoneIndex == INDEX_NONE )
					{
						FMeshBoneInfo NewMeshBoneInfo = InSkinnedAsset->GetRefSkeleton().GetRefBoneInfo()[MeshBoneIndex];
						// Fix up ParentIndex for our new Skeleton.
						// 修复我们的新骨架的 ParentIndex。
						if( ReferenceSkeleton.GetRawBoneNum() == 0 )
						{
							NewMeshBoneInfo.ParentIndex = INDEX_NONE; // root
						}
						else
						{
							NewMeshBoneInfo.ParentIndex = ReferenceSkeleton.FindRawBoneIndex(InSkinnedAsset->GetRefSkeleton().GetBoneName(InSkinnedAsset->GetRefSkeleton().GetParentIndex(MeshBoneIndex)));
						}

						RefSkelModifier.Add(NewMeshBoneInfo, InSkinnedAsset->GetRefSkeleton().GetRefBonePose()[MeshBoneIndex]);
						BoneTree.AddDefaulted(1);
						bShouldHandleHierarchyChange = true;
					}
				}
			}

			{
				FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, this);

				TMap<FName, FBoneNode> NameToBoneNode;
				const int32 NumBones = BoneTree.Num();
				check(NumBones == ReferenceSkeleton.GetRawRefBoneInfo().Num());

				// Check for different parents
				// [翻译失败: Check for different parents]
				for (const int32 MeshBoneIndex : FilteredRequiredBones)
				{
					const FName BoneName = InSkinnedAsset->GetRefSkeleton().GetBoneName(MeshBoneIndex);
					const int32 SkeletonBoneIndex = ReferenceSkeleton.FindRawBoneIndex(BoneName);

					if(SkeletonBoneIndex != INDEX_NONE)
					{
						// Bone exists, check if it is in the same place in the hierarchy
						// [翻译失败: Bone exists, check if it is in the same place in the hierarchy]
						const int32 MeshParentIndex = InSkinnedAsset->GetRefSkeleton().GetParentIndex(MeshBoneIndex);
						const int32 SkeletonParentIndex = ReferenceSkeleton.GetRawParentIndex(SkeletonBoneIndex);
						const FName MeshParentName = MeshParentIndex != INDEX_NONE ? InSkinnedAsset->GetRefSkeleton().GetBoneName(MeshParentIndex) : NAME_None;
						const FName SkeletonParentName = SkeletonParentIndex != INDEX_NONE ? ReferenceSkeleton.GetRawRefBoneInfo()[SkeletonParentIndex].Name : NAME_None;

						if(MeshParentName != SkeletonParentName)
						{
							// Cache the bone tree if we are going to change the structure
							// [翻译失败: Cache the bone tree if we are going to change the structure]
							if(NameToBoneNode.Num() == 0)
							{
								NameToBoneNode.Reserve(NumBones);
								for(int32 BoneTreeIndex = 0; BoneTreeIndex < NumBones; ++BoneTreeIndex)
								{
									NameToBoneNode.Add(ReferenceSkeleton.GetRawRefBoneInfo()[BoneTreeIndex].Name, BoneTree[BoneTreeIndex]);
								}
							}

							RefSkelModifier.SetParent(BoneName, MeshParentName);
							bShouldHandleHierarchyChange = true;
						}
					}
				}

				if(NameToBoneNode.Num() > 0)
				{
					// Setting parent can re-order bones, so the skeleton's BoneTree needs to update to reflect that
					// 设置parent可以重新排序骨骼，因此骨架的BoneTree需要更新以反映这一点
					BoneTree.Reset();
					for(int32 BoneTreeIndex = 0; BoneTreeIndex < NumBones; ++BoneTreeIndex)
					{
						FName BoneName = ReferenceSkeleton.GetRawRefBoneInfo()[BoneTreeIndex].Name;
						BoneTree.Add(NameToBoneNode.FindChecked(BoneName));
					}
				}
			}

			bSuccess = true;
		}
	}

	// if succeed
	// 如果成功
	if (bShouldHandleHierarchyChange)
	{
#if WITH_EDITOR
		HandleSkeletonHierarchyChange(bShowProgress);
#endif
	}

	return bSuccess;
}

void USkeleton::SetBoneTranslationRetargetingMode(const int32 BoneIndex, EBoneTranslationRetargetingMode::Type NewRetargetingMode, bool bChildrenToo)
{
	BoneTree[BoneIndex].TranslationRetargetingMode = NewRetargetingMode;

	if( bChildrenToo )
	{
		// Bones are guaranteed to be sorted in increasing order. So children will be after this bone.
		// 骨头保证按升序排序。所以孩子们会追寻这块骨头。
		const int32 NumBones = ReferenceSkeleton.GetRawBoneNum();
		for(int32 ChildIndex=BoneIndex+1; ChildIndex<NumBones; ChildIndex++)
		{
			if( ReferenceSkeleton.BoneIsChildOf(ChildIndex, BoneIndex) )
			{
				BoneTree[ChildIndex].TranslationRetargetingMode = NewRetargetingMode;
			}
		}
	}
}

const TArray<FTransform>& USkeleton::GetRefLocalPoses( FName RetargetSource ) const 
{
	if ( RetargetSource != NAME_None ) 
	{
		if (const FReferencePose* FoundRetargetSource = AnimRetargetSources.Find(RetargetSource))
		{
			return FoundRetargetSource->ReferencePose;
		}
	}
	return ReferenceSkeleton.GetRefBonePose();
}

#if WITH_EDITORONLY_DATA

FName USkeleton::GetRetargetSourceForMesh(USkinnedAsset* InSkinnedAsset) const
{
	FSoftObjectPath MeshPath(InSkinnedAsset);
	for(const TPair<FName, FReferencePose>& AnimRetargetSource : AnimRetargetSources)
	{
		if(AnimRetargetSource.Value.SourceReferenceMesh.ToSoftObjectPath() == MeshPath)
		{
			return AnimRetargetSource.Key;
		}
	}

	return NAME_None;
}

void USkeleton::GetRetargetSources(TArray<FName>& OutRetargetSources) const
{
	for(const TPair<FName, FReferencePose>& AnimRetargetSource : AnimRetargetSources)
	{
		OutRetargetSources.Add(AnimRetargetSource.Key);
	}
}

#endif

int32 USkeleton::GetRawAnimationTrackIndex(const int32 InSkeletonBoneIndex, const UAnimSequence* InAnimSeq)
{
	if (InSkeletonBoneIndex != INDEX_NONE)
	{
#if WITH_EDITOR
		TArray<FName> BoneTrackNames;
		InAnimSeq->GetDataModel()->GetBoneTrackNames(BoneTrackNames);
		const FName BoneName = ReferenceSkeleton.GetBoneName(InSkeletonBoneIndex);
		
		return BoneTrackNames.IndexOfByKey(BoneName);
#else
		UAnimSequence::FScopedCompressedAnimSequence CompressedAnimSequence = InAnimSeq->GetCompressedData();
		const TArray<FTrackToSkeletonMap>& BoneMappings = CompressedAnimSequence.Get().CompressedTrackToSkeletonMapTable;
		return BoneMappings.IndexOfByPredicate([InSkeletonBoneIndex](const FTrackToSkeletonMap& Mapping)
			{
				return Mapping.BoneTreeIndex == InSkeletonBoneIndex;
			});
#endif
	}

	return INDEX_NONE;
}


int32 USkeleton::GetSkeletonBoneIndexFromMeshBoneIndex(const USkinnedAsset* InSkinnedAsset, const int32 MeshBoneIndex)
{
	check(MeshBoneIndex != INDEX_NONE);
	const FSkeletonToMeshLinkup& LinkupTable = FindOrAddMeshLinkupData(InSkinnedAsset);
	return LinkupTable.MeshToSkeletonTable[MeshBoneIndex];
}

int32 USkeleton::GetMeshBoneIndexFromSkeletonBoneIndex(const USkinnedAsset* InSkinnedAsset, const int32 SkeletonBoneIndex)
{
	check(SkeletonBoneIndex != INDEX_NONE);
	const FSkeletonToMeshLinkup& LinkupTable = FindOrAddMeshLinkupData(InSkinnedAsset);
	return LinkupTable.SkeletonToMeshTable[SkeletonBoneIndex];
}


USkeletalMesh* USkeleton::GetPreviewMesh(bool bFindIfNotSet/*=false*/)
{
#if WITH_EDITORONLY_DATA
	USkeletalMesh* PreviewMesh = PreviewSkeletalMesh.LoadSynchronous();

	if(PreviewMesh && !IsCompatibleForEditor(PreviewMesh->GetSkeleton())) // fix mismatched skeleton
	{
		PreviewSkeletalMesh.Reset();
		PreviewMesh = nullptr;
	}

	// if not existing, and if bFindIfNotExisting is true, then try find one
	// 如果不存在，并且 bFindIfNotExisting 为 true，则尝试查找一个
	if(!PreviewMesh && bFindIfNotSet)
	{
		USkeletalMesh* CompatibleSkeletalMesh = FindCompatibleMesh();
		if(CompatibleSkeletalMesh)
		{
			SetPreviewMesh(CompatibleSkeletalMesh, false);
			// update PreviewMesh
			// 更新预览网格
			PreviewMesh = PreviewSkeletalMesh.Get();
		}
	}

	return PreviewMesh;
#else
	return nullptr;
#endif
}

USkeletalMesh* USkeleton::GetPreviewMesh() const
{
#if WITH_EDITORONLY_DATA
	if (!PreviewSkeletalMesh.IsValid())
	{
		PreviewSkeletalMesh.LoadSynchronous();
	}
	return PreviewSkeletalMesh.Get();
#else
	return nullptr;
#endif
}

void USkeleton::SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty/*=true*/)
{
#if WITH_EDITORONLY_DATA
	if (bMarkAsDirty)
	{
		Modify();
	}

	PreviewSkeletalMesh = PreviewMesh;
#endif
}

#if WITH_EDITORONLY_DATA
void USkeleton::UpdateRetargetSource( const FName Name )
{
	FReferencePose * PoseFound = AnimRetargetSources.Find(Name);

	if (PoseFound)
	{
		USkeletalMesh* ReferenceMesh;
		
		if (PoseFound->SourceReferenceMesh.IsValid())
		{
			ReferenceMesh = PoseFound->SourceReferenceMesh.Get();
		}
		else
		{
			PoseFound->SourceReferenceMesh.LoadSynchronous();
			ReferenceMesh = PoseFound->SourceReferenceMesh.Get();
		}

		if (ReferenceMesh)
		{
			FAnimationRuntime::MakeSkeletonRefPoseFromMesh(ReferenceMesh, this, PoseFound->ReferencePose);
		}
		else
		{
			UE_LOG(LogAnimation, Warning, TEXT("Reference Mesh for Retarget Source %s has been removed."), *GetName());
		}
	}
}

void USkeleton::RefreshAllRetargetSources()
{
	for (auto Iter = AnimRetargetSources.CreateConstIterator(); Iter; ++Iter)
	{
		UpdateRetargetSource(Iter.Key());
	}
}

int32 USkeleton::GetChildBones(int32 ParentBoneIndex, TArray<int32> & Children) const
{
	return ReferenceSkeleton.GetDirectChildBones(ParentBoneIndex, Children);
}

void USkeleton::CollectAnimationNotifies()
{
	CollectAnimationNotifies(AnimationNotifies);
}

void USkeleton::CollectAnimationNotifies(TArray<FName>& OutNotifies) const
{
	// first merge in AnimationNotifies
	// 首先合并到AnimationNotify中
	if(&AnimationNotifies != &OutNotifies)
	{
		for(const FName& NotifyName : AnimationNotifies)
		{
			OutNotifies.AddUnique(NotifyName);
		}
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// @Todo : remove it when we know the asset registry is updated
	// @Todo：当我们知道资产注册表已更新时将其删除
	// meanwhile if you remove this, this will miss the links
	// 同时如果你删除它，这将错过链接
	//AnimationNotifies.Empty();
	//AnimationNotify.Empty();
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssetsByClass(UAnimSequenceBase::StaticClass()->GetClassPathName(), AssetList, true);

	// do not clear AnimationNotifies. We can't remove old ones yet. 
	// 不要清除AnimationNotify。我们还不能删除旧的。
	FString CurrentSkeletonName = FAssetData(this).GetExportTextName();
	for (auto Iter = AssetList.CreateConstIterator(); Iter; ++Iter)
	{
		const FAssetData& Asset = *Iter;
		const FString SkeletonValue = Asset.GetTagValueRef<FString>(TEXT("Skeleton"));
		if (SkeletonValue == CurrentSkeletonName)
		{
			FString Value;
			if (Asset.GetTagValue(USkeleton::AnimNotifyTag, Value))
			{
				TArray<FString> NotifyList;
				Value.ParseIntoArray(NotifyList, *USkeleton::AnimNotifyTagDelimiter, true);
				for (auto NotifyIter = NotifyList.CreateConstIterator(); NotifyIter; ++NotifyIter)
				{
					FString NotifyName = *NotifyIter;
					OutNotifies.AddUnique(FName(*NotifyName));
				}
			}
		}
	}
}

void USkeleton::AddNewAnimationNotify(FName NewAnimNotifyName)
{
	if (NewAnimNotifyName!=NAME_None)
	{
		AnimationNotifies.AddUnique( NewAnimNotifyName);
	}
}

void USkeleton::RemoveAnimationNotify(FName AnimNotifyName)
{
	if (AnimNotifyName != NAME_None)
	{
		AnimationNotifies.Remove(AnimNotifyName);
	}
}

void USkeleton::RenameAnimationNotify(FName OldAnimNotifyName, FName NewAnimNotifyName)
{
	if(!AnimationNotifies.Contains(NewAnimNotifyName))
	{
		for(FName& NotifyName : AnimationNotifies)
		{
			if(NotifyName == OldAnimNotifyName)
			{
				NotifyName = NewAnimNotifyName;
				break;
			}
		}
	}
}

USkeletalMesh* USkeleton::FindCompatibleMesh() const
{
	FARFilter Filter;
	Filter.ClassPaths.Add(USkeletalMesh::StaticClass()->GetClassPathName());

	FString SkeletonString = FAssetData(this).GetExportTextName();
	
	Filter.TagsAndValues.Add(USkeletalMesh::GetSkeletonMemberName(), SkeletonString);

	TArray<FAssetData> AssetList;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	if (AssetList.Num() > 0)
	{
		return Cast<USkeletalMesh>(AssetList[0].GetAsset());
	}

	return nullptr;
}

USkeletalMesh* USkeleton::GetAssetPreviewMesh(UObject* InAsset) 
{
	USkeletalMesh* PreviewMesh = nullptr;

	// return asset preview asset
	// 返回资产预览资产
	// if nothing is assigned, return skeleton asset
	// 如果没有分配任何内容，则返回骨架资产
	if (UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(InAsset))
	{
		PreviewMesh = AnimAsset->GetPreviewMesh();
	}
	else if (UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(InAsset))
	{
		PreviewMesh = AnimBlueprint->GetPreviewMesh();
	}

	if (!PreviewMesh)
	{
		//The const version avoid verifying the skeleton compatibility, which can stall the thread
		//const 版本避免验证骨架兼容性，这可能会导致线程停顿
		const USkeleton* ThisSkeleton = this;
		PreviewMesh = ThisSkeleton->GetPreviewMesh();
		if (PreviewMesh && !PreviewMesh->IsCompiling())
		{
			//Verify the compatibility only if we are not building
			//仅当我们不构建时才验证兼容性
			PreviewMesh = GetPreviewMesh(false);
		}
	}

	return PreviewMesh;
}

void USkeleton::LoadAdditionalPreviewSkeletalMeshes()
{
	AdditionalPreviewSkeletalMeshes.LoadSynchronous();
}

UDataAsset* USkeleton::GetAdditionalPreviewSkeletalMeshes() const
{
	return AdditionalPreviewSkeletalMeshes.Get();
}

void USkeleton::SetAdditionalPreviewSkeletalMeshes(UDataAsset* InPreviewCollectionAsset)
{
	Modify();

	AdditionalPreviewSkeletalMeshes = InPreviewCollectionAsset;
}

int32 USkeleton::ValidatePreviewAttachedObjects()
{
	int32 NumBrokenAssets = PreviewAttachedAssetContainer.ValidatePreviewAttachedObjects();

	if(NumBrokenAssets > 0)
	{
		MarkPackageDirty();
	}
	return NumBrokenAssets;
}

#if WITH_EDITOR

void USkeleton::RemoveBonesFromSkeleton( const TArray<FName>& BonesToRemove, bool bRemoveChildBones )
{
	TArray<int32> BonesRemoved = ReferenceSkeleton.RemoveBonesByName(this, BonesToRemove);
	if(BonesRemoved.Num() > 0)
	{
		BonesRemoved.Sort();
		for(int32 Index = BonesRemoved.Num()-1; Index >=0; --Index)
		{
			BoneTree.RemoveAt(BonesRemoved[Index]);
		}
		HandleSkeletonHierarchyChange();
	}
}

void USkeleton::HandleSkeletonHierarchyChange(bool bShowProgress /*= true*/)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USkeleton::HandleSkeletonHierarchyChange);
	MarkPackageDirty();

	RegenerateGuid();

	// Clear exiting MeshLinkUp tables.
	// 清除现有的 MeshLinkUp 表。
	ClearCacheData();

	for (int i = VirtualBones.Num() - 1; i >= 0; --i)
	{
		FVirtualBone& VB = VirtualBones[i];

		// Note: here virtual bones can have source bound to other virtual bones
		// 注意：这里虚拟骨骼可以将源绑定到其他虚拟骨骼
		if (ReferenceSkeleton.FindBoneIndex(VB.SourceBoneName) == INDEX_NONE ||
			ReferenceSkeleton.FindBoneIndex(VB.TargetBoneName) == INDEX_NONE)
		{
			//Virtual Bone no longer valid
			//虚拟骨骼不再有效
			VirtualBones.RemoveAt(i);
		}
	}

	// Full rebuild of all compatible with this and with ones we are compatible with.
	// 完全重建所有与此兼容的以及与我们兼容的。
	UE::Anim::FSkeletonRemappingRegistry::Get().RefreshMappings(this);

	// Fix up loaded animations (any animations that aren't loaded will be fixed on load)
	// 修复加载的动画（任何未加载的动画将在加载时修复）
	int32 NumLoadedAssets = 0;
	for (TObjectIterator<UAnimationAsset> It; It; ++It)
	{
		UAnimationAsset* CurrentAnimation = *It;
		if (CurrentAnimation->GetSkeleton() == this)
		{
			NumLoadedAssets++;
		}
	}

	FScopedSlowTask SlowTask((float)NumLoadedAssets, LOCTEXT("HandleSkeletonHierarchyChange", "Rebuilding animations..."), bShowProgress);
	if (bShowProgress)
	{
		SlowTask.MakeDialog();
	}

	for (TObjectIterator<UAnimationAsset> It; It; ++It)
	{
		UAnimationAsset* CurrentAnimation = *It;
		if (CurrentAnimation->GetSkeleton() == this)
		{
			if (bShowProgress)
			{
				SlowTask.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("HandleSkeletonHierarchyChange_Format", "Rebuilding Animation: {0}"), FText::FromString(CurrentAnimation->GetName())));
			}

			CurrentAnimation->ValidateSkeleton();
		}
	}

#if WITH_EDITORONLY_DATA
	RefreshAllRetargetSources();
#endif

	RefreshSkeletonMetaData();

	// Remove entries from Blend Profiles for bones that no longer exists
	// 从混合配置文件中删除不再存在的骨骼条目
	for (UBlendProfile* Profile : BlendProfiles)
	{
		Profile->RefreshBoneEntriesFromName();
		Profile->CleanupBoneEntries();
	}

	OnSkeletonHierarchyChanged.Broadcast();
}

void USkeleton::RegisterOnSkeletonHierarchyChanged(const FOnSkeletonHierarchyChanged& Delegate)
{
	OnSkeletonHierarchyChanged.Add(Delegate);
}

void USkeleton::UnregisterOnSkeletonHierarchyChanged(FDelegateUserObject Unregister)
{
	OnSkeletonHierarchyChanged.RemoveAll(Unregister);
}

#endif

#endif // WITH_EDITORONLY_DATA

const TArray<FAnimSlotGroup>& USkeleton::GetSlotGroups() const
{
	return SlotGroups;
}

void USkeleton::BuildSlotToGroupMap(bool bInRemoveDuplicates)
{
	SlotToGroupNameMap.Empty();

	for (FAnimSlotGroup& SlotGroup : SlotGroups)
	{
		for (const FName& SlotName : SlotGroup.SlotNames)
		{
			SlotToGroupNameMap.Add(SlotName, SlotGroup.GroupName);
		}
	}

	// Use the map we've just build to rebuild the slot groups
	// 使用我们刚刚构建的地图来重建插槽组
	if(bInRemoveDuplicates)
	{
		for(FAnimSlotGroup& SlotGroup : SlotGroups)
		{
			SlotGroup.SlotNames.Empty(SlotGroup.SlotNames.Num());

			for(TPair<FName, FName>& SlotToGroupPair : SlotToGroupNameMap)
			{
				if(SlotToGroupPair.Value == SlotGroup.GroupName)
				{
					SlotGroup.SlotNames.Add(SlotToGroupPair.Key);
				}
			}
		}

	}
}

FAnimSlotGroup* USkeleton::FindAnimSlotGroup(const FName& InGroupName)
{
	return SlotGroups.FindByPredicate([&InGroupName](const FAnimSlotGroup& Item)
	{
		return Item.GroupName == InGroupName;
	});
}

const FAnimSlotGroup* USkeleton::FindAnimSlotGroup(const FName& InGroupName) const
{
	return SlotGroups.FindByPredicate([&InGroupName](const FAnimSlotGroup& Item)
	{
		return Item.GroupName == InGroupName;
	});
}

bool USkeleton::ContainsSlotName(const FName& InSlotName) const
{
	return SlotToGroupNameMap.Contains(InSlotName);
}

bool USkeleton::RegisterSlotNode(const FName& InSlotName)
{
	// verify the slot name exists, if not create it in the default group.
	// 验证插槽名称是否存在，如果不存在，请在默认组中创建它。
	if (!ContainsSlotName(InSlotName))
	{
		SetSlotGroupName(InSlotName, FAnimSlotGroup::DefaultGroupName);
		return true;
	}

	return false;
}

void USkeleton::SetSlotGroupName(const FName& InSlotName, const FName& InGroupName)
{
// See if Slot already exists and belongs to a group.
// 查看插槽是否已存在并且属于某个组。
	const FName* FoundGroupNamePtr = SlotToGroupNameMap.Find(InSlotName);

	// If slot exists, but is not in the right group, remove it from there
	// 如果插槽存在，但不在正确的组中，请将其从那里删除
	if (FoundGroupNamePtr && ((*FoundGroupNamePtr) != InGroupName))
	{
		FAnimSlotGroup* OldSlotGroupPtr = FindAnimSlotGroup(*FoundGroupNamePtr);
		if (OldSlotGroupPtr)
		{
			OldSlotGroupPtr->SlotNames.RemoveSingleSwap(InSlotName);
		}
	}

	// Add the slot to the right group if it's not
	// 如果不是，请将插槽添加到右侧组
	if ((FoundGroupNamePtr == NULL) || (*FoundGroupNamePtr != InGroupName))
	{
		// If the SlotGroup does not exist, create it.
		// 如果 SlotGroup 不存在，则创建它。
		FAnimSlotGroup* SlotGroupPtr = FindAnimSlotGroup(InGroupName);
		if (SlotGroupPtr == NULL)
		{
			SlotGroups.AddZeroed(1);
			SlotGroupPtr = &SlotGroups.Last();
			SlotGroupPtr->GroupName = InGroupName;
		}
		// Add Slot to group.
		// 将插槽添加到组中。
		SlotGroupPtr->SlotNames.Add(InSlotName);
		// Keep our TMap up to date.
		// 让我们的 TMap 保持最新状态。
		SlotToGroupNameMap.Add(InSlotName, InGroupName);
	}
}

bool USkeleton::AddSlotGroupName(const FName& InNewGroupName)
{
	FAnimSlotGroup* ExistingSlotGroupPtr = FindAnimSlotGroup(InNewGroupName);
	if (ExistingSlotGroupPtr == NULL)
	{
		// if not found, create a new one.
		// 如果没有找到，则创建一个新的。
		SlotGroups.AddZeroed(1);
		ExistingSlotGroupPtr = &SlotGroups.Last();
		ExistingSlotGroupPtr->GroupName = InNewGroupName;
		return true;
	}

	return false;
}

FName USkeleton::GetSlotGroupName(const FName& InSlotName) const
{
	const FName* FoundGroupNamePtr = SlotToGroupNameMap.Find(InSlotName);
	if (FoundGroupNamePtr)
	{
		return *FoundGroupNamePtr;
	}

	// If Group name cannot be found, use DefaultSlotGroupName.
	// 如果找不到组名称，请使用 DefaultSlotGroupName。
	return FAnimSlotGroup::DefaultGroupName;
}

void USkeleton::RemoveSlotName(const FName& InSlotName)
{
	FName GroupName = GetSlotGroupName(InSlotName);
	
	if(SlotToGroupNameMap.Remove(InSlotName) > 0)
	{
		FAnimSlotGroup* SlotGroup = FindAnimSlotGroup(GroupName);
		SlotGroup->SlotNames.Remove(InSlotName);
	}
}

void USkeleton::RemoveSlotGroup(const FName& InSlotGroupName)
{
	FAnimSlotGroup* SlotGroup = FindAnimSlotGroup(InSlotGroupName);
	// Remove slot mappings
	// 删除插槽映射
	for(const FName& SlotName : SlotGroup->SlotNames)
	{
		SlotToGroupNameMap.Remove(SlotName);
	}

	// Remove group
	// 删除组
	SlotGroups.RemoveAll([&InSlotGroupName](const FAnimSlotGroup& Item)
	{
		return Item.GroupName == InSlotGroupName;
	});
}

void USkeleton::RenameSlotName(const FName& OldName, const FName& NewName)
{
	// Can't rename a name that doesn't exist
	// 无法重命名不存在的名称
	check(ContainsSlotName(OldName))

	FName GroupName = GetSlotGroupName(OldName);
	RemoveSlotName(OldName);
	SetSlotGroupName(NewName, GroupName);
}

bool USkeleton::AddCurveMetaData(FName CurveName, bool bTransact)
{
	UAnimCurveMetaData* AnimCurveMetaData = GetOrCreateCurveMetaDataObject();
	return AnimCurveMetaData->AddCurveMetaData(CurveName, bTransact);
}

#if WITH_EDITOR

bool USkeleton::RenameCurveMetaData(FName OldName, FName NewName)
{
	if(UAnimCurveMetaData* AnimCurveMetaData = GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->RenameCurveMetaData(OldName, NewName);
	}
	return false;
}

bool USkeleton::RemoveCurveMetaData(FName CurveName)
{
	if(UAnimCurveMetaData* AnimCurveMetaData = GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->RemoveCurveMetaData(CurveName);
	}
	return false;
}

bool USkeleton::RemoveCurveMetaData(TArrayView<FName> CurveNames)
{
	if(UAnimCurveMetaData* AnimCurveMetaData = GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->RemoveCurveMetaData(CurveNames);
	}
	return false;
}

bool USkeleton::GetCurveMetaDataMaterial(FName CurveName) const
{
	if (const UAnimCurveMetaData* AnimCurveMetaData = const_cast<USkeleton*>(this)->GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->GetCurveMetaDataMaterial(CurveName);
	}

	return false;
}

bool USkeleton::GetCurveMetaDataMorphTarget(FName CurveName) const
{
	if (const UAnimCurveMetaData* AnimCurveMetaData = const_cast<USkeleton*>(this)->GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->GetCurveMetaDataMorphTarget(CurveName);
	}

	return false;
}

void USkeleton::SetCurveMetaDataMaterial(FName CurveName, bool bOverrideMaterial)
{
	if (UAnimCurveMetaData* AnimCurveMetaData = GetAssetUserData<UAnimCurveMetaData>())
	{
		AnimCurveMetaData->SetCurveMetaDataMaterial(CurveName, bOverrideMaterial);
	}
}

void USkeleton::SetCurveMetaDataMorphTarget(FName CurveName, bool bOverrideMorphTarget)
{
	if (UAnimCurveMetaData* AnimCurveMetaData = GetAssetUserData<UAnimCurveMetaData>())
	{
		AnimCurveMetaData->SetCurveMetaDataMorphTarget(CurveName, bOverrideMorphTarget);
	}
}

#endif // WITH_EDITOR

uint16 USkeleton::GetAnimCurveUidVersion() const
{
	if(UAnimCurveMetaData* AnimCurveMetaData = const_cast<USkeleton*>(this)->GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->GetVersionNumber();
	}
	return 0;
}

void USkeleton::GetCurveMetaDataNames(TArray<FName>& OutNames) const
{
	if(const UAnimCurveMetaData* AnimCurveMetaData = const_cast<USkeleton*>(this)->GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->GetCurveMetaDataNames(OutNames);
	}
}

void USkeleton::RegenerateGuid()
{
	Guid = FGuid::NewGuid();
	check(Guid.IsValid());
}

void USkeleton::RegenerateVirtualBoneGuid()
{
	VirtualBoneGuid = FGuid::NewGuid();
	check(VirtualBoneGuid.IsValid());
}

FCurveMetaData* USkeleton::GetCurveMetaData(FName CurveName)
{
	if(UAnimCurveMetaData* AnimCurveMetaData = GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->GetCurveMetaData(CurveName);
	}
	return nullptr;
}

const FCurveMetaData* USkeleton::GetCurveMetaData(FName CurveName) const
{
	if(const UAnimCurveMetaData* AnimCurveMetaData = const_cast<USkeleton*>(this)->GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->GetCurveMetaData(CurveName);
	}
	return nullptr;
}

void USkeleton::ForEachCurveMetaData(TFunctionRef<void(FName, const FCurveMetaData&)> InFunction) const
{
	if(const UAnimCurveMetaData* AnimCurveMetaData = const_cast<USkeleton*>(this)->GetAssetUserData<UAnimCurveMetaData>())
	{
		AnimCurveMetaData->ForEachCurveMetaData(InFunction);
	}
}

int32 USkeleton::GetNumCurveMetaData() const
{
	if(const UAnimCurveMetaData* AnimCurveMetaData = const_cast<USkeleton*>(this)->GetAssetUserData<UAnimCurveMetaData>())
	{
		return AnimCurveMetaData->GetNumCurveMetaData();
	}
	return 0;
}

void USkeleton::AccumulateCurveMetaData(FName CurveName, bool bMaterialSet, bool bMorphtargetSet)
{
	if (bMaterialSet || bMorphtargetSet)
	{
		// Add curve if not already present
		// 添加曲线（如果尚不存在）
		AddCurveMetaData(CurveName);

		FCurveMetaData* FoundCurveMetaData = GetCurveMetaData(CurveName);
		check(FoundCurveMetaData);

		bool bOldMaterial = FoundCurveMetaData->Type.bMaterial;
		bool bOldMorphtarget = FoundCurveMetaData->Type.bMorphtarget;
		// we don't want to undo previous flags, if it was true, we just allow more to it. 
		// 我们不想撤消以前的标志，如果这是真的，我们只是允许更多。
		FoundCurveMetaData->Type.bMaterial |= bMaterialSet;
		FoundCurveMetaData->Type.bMorphtarget |= bMorphtargetSet;

		if (bOldMaterial != FoundCurveMetaData->Type.bMaterial 
			|| bOldMorphtarget != FoundCurveMetaData->Type.bMorphtarget)
		{
			MarkPackageDirty();
		}
	}
}

bool USkeleton::AddNewVirtualBone(const FName SourceBoneName, const FName TargetBoneName)
{
	FName Dummy;
	return AddNewVirtualBone(SourceBoneName, TargetBoneName, Dummy);
}

bool USkeleton::AddNewVirtualBone(const FName SourceBoneName, const FName TargetBoneName, FName& NewVirtualBoneName)
{
	for (const FVirtualBone& SSBone : VirtualBones)
	{
		if (SSBone.SourceBoneName == SourceBoneName &&
			SSBone.TargetBoneName == TargetBoneName)
		{
			return false;
		}
	}
	Modify();
	VirtualBones.Add(FVirtualBone(SourceBoneName, TargetBoneName));
	NewVirtualBoneName = VirtualBones.Last().VirtualBoneName;

	RegenerateVirtualBoneGuid();
	HandleVirtualBoneChanges();


	return true;
}

bool USkeleton::AddNewNamedVirtualBone(const FName SourceBoneName, const FName TargetBoneName, const FName VirtualBoneName)
{
	if (!VirtualBoneNameHelpers::CheckVirtualBonePrefix(VirtualBoneName.ToString()))
	{
		return false;
	}

	for (const FVirtualBone& SSBone : VirtualBones)
	{
		if ((SSBone.SourceBoneName == SourceBoneName && SSBone.TargetBoneName == TargetBoneName) ||
			SSBone.VirtualBoneName == VirtualBoneName)
		{
			return false;
		}
	}

	Modify();

	VirtualBones.Emplace(SourceBoneName, TargetBoneName, VirtualBoneName);

	RegenerateVirtualBoneGuid();
	HandleVirtualBoneChanges();

	return true;
}

int32 FindBoneByName(const FName& BoneName, TArray<FVirtualBone>& Bones)
{
	for (int32 Idx = 0; Idx < Bones.Num(); ++Idx)
	{
		if (Bones[Idx].VirtualBoneName == BoneName)
		{
			return Idx;
		}
	}
	return INDEX_NONE;
}

void USkeleton::RemoveVirtualBones(const TArray<FName>& BonesToRemove)
{
	Modify();
	for (const FName& BoneName : BonesToRemove)
	{
		int32 Idx = FindBoneByName(BoneName, VirtualBones);
		if (Idx != INDEX_NONE)
		{
			FName Parent = VirtualBones[Idx].SourceBoneName;
			for (FVirtualBone& VB : VirtualBones)
			{
				if (VB.SourceBoneName == BoneName)
				{
					VB.SourceBoneName = Parent;
				}
			}
			VirtualBones.RemoveAt(Idx,EAllowShrinking::No);

			// @todo: This might be a slow operation if there's a large amount of blend profiles and entries
			// @todo：如果有大量混合配置文件和条目，这可能是一个缓慢的操作
			int32 BoneIdx = GetReferenceSkeleton().FindBoneIndex(BoneName);
			if(BoneIdx != INDEX_NONE)
			{
				for (UBlendProfile* Profile : BlendProfiles)
				{
					Profile->RemoveEntry(BoneIdx);
				}
			}
		}
	}

	RegenerateVirtualBoneGuid();
	HandleVirtualBoneChanges();

	// Blend profiles cache bone names and indices, make sure they remain in sync when the indices change
	// 混合配置文件缓存骨骼名称和索引，确保它们在索引更改时保持同步
	for (UBlendProfile* Profile : BlendProfiles)
	{
		Profile->RefreshBoneEntriesFromName();
	}
}

void USkeleton::RenameVirtualBone(const FName OriginalBoneName, const FName NewBoneName)
{
	bool bModified = false;

	for (FVirtualBone& VB : VirtualBones)
	{
		if (VB.VirtualBoneName == OriginalBoneName)
		{
			if (!bModified)
			{
				bModified = true;
				Modify();
			}

			VB.VirtualBoneName = NewBoneName;
		}

		if (VB.SourceBoneName == OriginalBoneName)
		{
			if (!bModified)
			{
				bModified = true;
				Modify();
			}
			VB.SourceBoneName = NewBoneName;
		}
	}

	if (bModified)
	{
		RegenerateVirtualBoneGuid();
		HandleVirtualBoneChanges();

		// @todo: This might be a slow operation if there's a large amount of blend profiles and entries
		// @todo：如果有大量混合配置文件和条目，这可能是一个缓慢的操作
		int32 BoneIdx = GetReferenceSkeleton().FindBoneIndex(NewBoneName);
		if (BoneIdx != INDEX_NONE)
		{
			for (UBlendProfile* Profile : BlendProfiles)
			{
				Profile->RefreshBoneEntry(BoneIdx);
			}
		}
	}
}

void USkeleton::HandleVirtualBoneChanges()
{
	constexpr bool bRebuildNameMap = false;
	ReferenceSkeleton.RebuildRefSkeleton(this, bRebuildNameMap);

	UE::Anim::FSkeletonRemappingRegistry::Get().RefreshMappings(this);

	// store skeletal meshes that are also transacting to avoid re-registering the component here
	// 存储也在进行事务处理的骨架网格物体，以避免在此处重新注册组件
	// as it will be done later in USkeletalMesh::PostEditUndo()
	// [翻译失败: as it will be done later in USkeletalMesh::PostEditUndo()]
	TArray<USkeletalMesh*> SkeletalMeshTransacting;
	
	for (TObjectIterator<USkeletalMesh> ItMesh; ItMesh; ++ItMesh)
	{
		USkeletalMesh* SkelMesh = *ItMesh;
		if (SkelMesh->GetSkeleton() == this)
		{
			// also have to update retarget base pose
			// [翻译失败: also have to update retarget base pose]
			SkelMesh->GetRefSkeleton().RebuildRefSkeleton(this, bRebuildNameMap);
			RebuildLinkup(SkelMesh);

#if WITH_EDITOR
			if (SkelMesh->IsTransacting())
			{
				SkeletalMeshTransacting.Add(SkelMesh);
			}
#endif
		}
	}

	// refresh curve meta data that contains joint info
	// [翻译失败: refresh curve meta data that contains joint info]
	RefreshSkeletonMetaData();

	auto NeedsReRegistration = [this, &SkeletalMeshTransacting](const USkinnedMeshComponent* InMeshComponent)
	{
		if (!InMeshComponent || InMeshComponent->IsTemplate())
		{
			return false;
		}

		USkinnedAsset* SkinnedAsset = InMeshComponent->GetSkinnedAsset();
		if (!SkinnedAsset || SkinnedAsset->GetSkeleton() != this)
		{
			return false;
		}

#if WITH_EDITOR
		return !SkeletalMeshTransacting.Contains(SkinnedAsset);
#else
		return true;
#endif
	};
	
	for (TObjectIterator<USkinnedMeshComponent> It; It; ++It)
	{
		USkinnedMeshComponent* MeshComponent = *It;
		if (NeedsReRegistration(MeshComponent))
		{
			FComponentReregisterContext Context(MeshComponent);
		}
	}

#if WITH_EDITOR
	OnSkeletonHierarchyChanged.Broadcast();
#endif
}

#if WITH_EDITOR

void USkeleton::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS;
	Super::GetAssetRegistryTags(OutTags);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS;
}

void USkeleton::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);

	TStringBuilder<256> CompatibleSkeletonsBuilder;
	
	for(const TSoftObjectPtr<USkeleton>& CompatibleSkeleton : CompatibleSkeletons)
	{
		const FString ExportPath = FObjectPropertyBase::GetExportPath(USkeleton::StaticClass()->GetClassPathName(), CompatibleSkeleton.ToSoftObjectPath().GetAssetPathString());
		CompatibleSkeletonsBuilder.Append(ExportPath);
		CompatibleSkeletonsBuilder.Append(USkeleton::CompatibleSkeletonsTagDelimiter);
	}

	Context.AddTag(FAssetRegistryTag(USkeleton::CompatibleSkeletonsNameTag, CompatibleSkeletonsBuilder.ToString(), FAssetRegistryTag::TT_Hidden));

	// Output sync notify names we use
	// [翻译失败: Output sync notify names we use]
	TStringBuilder<256> NotifiesBuilder;
	NotifiesBuilder.Append(USkeleton::AnimNotifyTagDelimiter);

	for(FName NotifyName : AnimationNotifies)
	{
		NotifiesBuilder.Append(NotifyName.ToString());
		NotifiesBuilder.Append(USkeleton::AnimNotifyTagDelimiter);
	}

	Context.AddTag(FAssetRegistryTag(USkeleton::AnimNotifyTag, NotifiesBuilder.ToString(), FAssetRegistryTag::TT_Hidden));
	
	// Output sync marker names we use
	// 我们使用的输出同步标记名称
	TStringBuilder<256> SyncMarkersBuilder;
	SyncMarkersBuilder.Append(USkeleton::AnimSyncMarkerTagDelimiter);

	for(FName SyncMarker : ExistingMarkerNames)
	{
		SyncMarkersBuilder.Append(SyncMarker.ToString());
		SyncMarkersBuilder.Append(USkeleton::AnimSyncMarkerTagDelimiter);
	}

	Context.AddTag(FAssetRegistryTag(USkeleton::AnimSyncMarkerTag, SyncMarkersBuilder.ToString(), FAssetRegistryTag::TT_Hidden));
	
	// Allow asset user data to output tags
	// 允许资产用户数据输出标签
	for(const UAssetUserData* AssetUserDataItem : *GetAssetUserDataArray())
	{
		if (AssetUserDataItem)
		{
			AssetUserDataItem->GetAssetRegistryTags(Context);
		}
	}
}

#endif //WITH_EDITOR

UBlendProfile* USkeleton::GetBlendProfile(const FName& InProfileName)
{
	for (const TObjectPtr<UBlendProfile>& Profile : BlendProfiles)
	{
		if (Profile->GetFName() == InProfileName)
		{
			return Profile;
		}
	}
	return nullptr;
}

UBlendProfile* USkeleton::CreateNewBlendProfile(const FName& InProfileName)
{
	Modify();
	UBlendProfile* NewProfile = NewObject<UBlendProfile>(this, InProfileName, RF_Public | RF_Transactional);
	BlendProfiles.Add(NewProfile);

	return NewProfile;
}

UBlendProfile* USkeleton::RenameBlendProfile(const FName& InProfileName, const FName& InNewProfileName)
{
	if (!GetBlendProfile(InNewProfileName)) // we can not rename if the new name already exists
	{
		UBlendProfile* BlendProfile = GetBlendProfile(InProfileName);
		if (BlendProfile)
		{
			Modify();
			if (BlendProfile->Rename(*InNewProfileName.ToString(), this, REN_DontCreateRedirectors))
			{
				return BlendProfile;
			}
		}
	}

	return nullptr;
}

USkeletalMeshSocket* USkeleton::FindSocket(FName InSocketName) const
{
	int32 DummyIndex;
	return FindSocketAndIndex(InSocketName, DummyIndex);
}

USkeletalMeshSocket* USkeleton::FindSocketAndIndex(FName InSocketName, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;
	if (InSocketName == NAME_None)
	{
		return nullptr;
	}

	for (int32 i = 0; i < Sockets.Num(); ++i)
	{
		USkeletalMeshSocket* Socket = Sockets[i];
		if (Socket && Socket->SocketName == InSocketName)
		{
			OutIndex = i;
			return Socket;
		}
	}

	return nullptr;
}


void USkeleton::AddAssetUserData( UAssetUserData* InUserData)
{
	if (InUserData != NULL)
	{
		RemoveUserDataOfClass(InUserData->GetClass());
		AssetUserData.Add(InUserData);
	}
}

UAssetUserData* USkeleton::GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass)
{
	const TArray<UAssetUserData*>* ArrayPtr = GetAssetUserDataArray();
	for (int32 DataIdx = 0; DataIdx < ArrayPtr->Num(); DataIdx++)
	{
		UAssetUserData* Datum = (*ArrayPtr)[DataIdx];
		if (Datum != NULL && Datum->IsA(InUserDataClass))
		{
			return Datum;
		}
	}
	return NULL;
}

void USkeleton::RemoveUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass)
{
	for (int32 DataIdx = 0; DataIdx < AssetUserData.Num(); DataIdx++)
	{
		UAssetUserData* Datum = AssetUserData[DataIdx];
		if (Datum != NULL && Datum->IsA(InUserDataClass))
		{
			AssetUserData.RemoveAt(DataIdx);
			return;
		}
	}

#if WITH_EDITOR
	for (int32 DataIdx = 0; DataIdx < AssetUserDataEditorOnly.Num(); DataIdx++)
	{
		UAssetUserData* Datum = AssetUserDataEditorOnly[DataIdx];
		if (Datum != NULL && Datum->IsA(InUserDataClass))
		{
			AssetUserDataEditorOnly.RemoveAt(DataIdx);
			return;
		}
	}
#endif
}

const TArray<UAssetUserData*>* USkeleton::GetAssetUserDataArray() const
{
#if WITH_EDITOR
	if (IsRunningCookCommandlet())
	{
		return &ToRawPtrTArrayUnsafe(AssetUserData);
	}
	else
	{
		static thread_local TArray<TObjectPtr<UAssetUserData>> CachedAssetUserData;
		CachedAssetUserData.Reset();
		CachedAssetUserData.Append(AssetUserData);
		CachedAssetUserData.Append(AssetUserDataEditorOnly);
		return &ToRawPtrTArrayUnsafe(CachedAssetUserData);
	}
#else
	return &ToRawPtrTArrayUnsafe(AssetUserData);
#endif
}

void USkeleton::HandlePackageReloaded(const EPackageReloadPhase InPackageReloadPhase, FPackageReloadedEvent* InPackageReloadedEvent)
{
	if (InPackageReloadPhase == EPackageReloadPhase::PostPackageFixup)
	{
		for (const auto& RepointedObjectPair : InPackageReloadedEvent->GetRepointedObjects())
		{
			if (USkeleton* NewObject = Cast<USkeleton>(RepointedObjectPair.Value))
			{
				NewObject->HandleVirtualBoneChanges(); // Reloading Skeletons can invalidate virtual bones so refresh
			}
		}
	}
}

void USkeleton::RefreshSkeletonMetaData()
{
	if(UAnimCurveMetaData* AnimCurveMetaData = GetAssetUserData<UAnimCurveMetaData>())
	{
		AnimCurveMetaData->RefreshBoneIndices(this);
	}
}

UAnimCurveMetaData* USkeleton::GetOrCreateCurveMetaDataObject()
{
	UAnimCurveMetaData* AnimCurveMetaData = GetAssetUserData<UAnimCurveMetaData>();
	if (AnimCurveMetaData == nullptr)
	{
		AnimCurveMetaData = NewObject<UAnimCurveMetaData>(this, NAME_None, RF_Transactional);
		AddAssetUserData(AnimCurveMetaData);
	}

	return AnimCurveMetaData;
}

bool USkeleton::GetUseRetargetModesFromCompatibleSkeleton() const
{
	return bUseRetargetModesFromCompatibleSkeleton;
}

void USkeleton::SetUseRetargetModesFromCompatibleSkeleton(bool bUse)
{
	bUseRetargetModesFromCompatibleSkeleton = bUse;
}

#undef LOCTEXT_NAMESPACE 

