// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Class of pose asset that can evaluate pose by weights
 *
 */

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/SmartName.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimCurveTypes.h"
#include "AnimData/IAnimationDataModel.h"
#include "PoseAsset.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;
struct FLiveLinkCurveElement;
struct FReferenceSkeleton;
class FPoseAssetDetails;

/** 
 * Pose data 
 * 
 * This is one pose data structure
 * This will let us blend poses quickly easily
 * All poses within this asset should contain same number of tracks, 
 * so that we can blend quickly
 */

USTRUCT()
struct FPoseData
{
	GENERATED_USTRUCT_BODY()

#if WITH_EDITORONLY_DATA
	// source local space pose, this pose is always full pose
	// 源局部空间姿势，该姿势始终是完整姿势
	// the size this array matches Tracks in the pose container
	// 该数组的大小与姿势容器中的轨迹相匹配
	UPROPERTY()
	TArray<FTransform>		SourceLocalSpacePose;

	// source curve data that is full value
	// 源曲线数据为全值
	UPROPERTY()
	TArray<float>			SourceCurveData;
#endif // WITH_EDITORONLY_DATA

	// local space pose, # of array match with # of TrackToBufferIndex
	// 局部空间姿态，数组的 # 与 TrackToBufferIndex 的 # 匹配
	// it only saves the one with delta as base pose or ref pose if full pose
	// 它仅保存具有 delta 的姿势作为基本姿势或参考姿势（如果是完整姿势）
	UPROPERTY()
	TArray<FTransform>		LocalSpacePose;

	// # of array match with # of Curves in PoseDataContainer
	// 数组数量与 PoseDataContainer 中的曲线数量匹配
	// curve data is not compressed
	// 曲线数据未压缩
 	UPROPERTY()
 	TArray<float>			CurveData;
};

USTRUCT()
struct FPoseAssetInfluence
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 PoseIndex = INDEX_NONE;

	UPROPERTY()
	int32 BoneTransformIndex = INDEX_NONE;
};

USTRUCT()
struct FPoseAssetInfluences
{
	GENERATED_USTRUCT_BODY()

    UPROPERTY()
	TArray<FPoseAssetInfluence> Influences;
};

/**
* Pose data container
* 
* Contains animation and curve for all poses
*/
USTRUCT()
struct FPoseDataContainer
{
	GENERATED_USTRUCT_BODY()

public:
	/** For StructOpsTypeTraits */
	/** 对于 StructOpsTypeTraits */
	ENGINE_API bool Serialize(FArchive& Ar);
	ENGINE_API void PostSerialize(const FArchive& Ar);
	
private:
#if WITH_EDITORONLY_DATA
	// pose names - horizontal data
	// 姿势名称 - 水平数据
	UPROPERTY()
	TArray<FSmartName> PoseNames_DEPRECATED;
#endif

	// pose names - horizontal data
	// 姿势名称 - 水平数据
	UPROPERTY()
	TArray<FName> PoseFNames;

	// Sorted curve name indices
	// 排序曲线名称索引
	TArray<int32> SortedCurveIndices;
	
	// this is list of tracks - vertical data
	// 这是曲目列表 - 垂直数据
	UPROPERTY()
	TArray<FName>							Tracks;

	// cache containting the skeleton indices for FName in Tracks array
	// 包含 Tracks 数组中 FName 骨架索引的缓存
	UPROPERTY(transient)
	TArray<int32>						TrackBoneIndices;

	UPROPERTY()
	TArray<FPoseAssetInfluences>		TrackPoseInfluenceIndices;
	
	// this is list of poses
	// 这是姿势列表
	UPROPERTY()
	TArray<FPoseData>						Poses;
	
	
	// curve meta data # of Curve UIDs should match with Poses.CurveValues.Num
	// 曲线 UID 的曲线元数据数应与 Poses.CurveValues.Num 匹配
	UPROPERTY()
	TArray<FAnimCurveBase>					Curves;

	ENGINE_API void Reset();

	ENGINE_API FPoseData* FindPoseData(FName PoseName);
	ENGINE_API FPoseData* FindOrAddPoseData(FName PoseName);

	int32 GetNumPoses() const { return Poses.Num();  }
	bool Contains(FName PoseName) const { return PoseFNames.Contains(PoseName); }

	bool IsValid() const { return PoseFNames.Num() == Poses.Num() && Tracks.Num() == TrackBoneIndices.Num(); }
	ENGINE_API void GetPoseCurve(const FPoseData* PoseData, FBlendedCurve& OutCurve) const;
	ENGINE_API void BlendPoseCurve(const FPoseData* PoseData, FBlendedCurve& OutCurve, float Weight) const;

	// we have to delete tracks if skeleton has modified
	// 如果骨架已修改，我们必须删除轨迹
	// usually this may not be issue since once cooked, it should match
	// 通常这可能不是问题，因为一旦煮熟，它应该匹配
	ENGINE_API void DeleteTrack(int32 TrackIndex);
	
	// get default transform - it considers for retarget source if exists
	// 获取默认转换 - 如果存在，它会考虑重定向源
	ENGINE_API FTransform GetDefaultTransform(const FName& InTrackName, USkeleton* InSkeleton, const TArray<FTransform>& RefPose) const;
	ENGINE_API FTransform GetDefaultTransform(int32 SkeletonIndex, const TArray<FTransform>& RefPose) const;

#if WITH_EDITOR
	ENGINE_API void AddOrUpdatePose(const FName& InPoseName, const TArray<FTransform>& InlocalSpacePose, const TArray<float>& InCurveData);
	ENGINE_API void RenamePose(FName OldPoseName, FName NewPoseName);
	ENGINE_API int32 DeletePose(FName PoseName);
	ENGINE_API bool DeleteCurve(FName CurveName);
	ENGINE_API bool InsertTrack(const FName& InTrackName, USkeleton* InSkeleton, const TArray<FTransform>& RefPose);
	
	ENGINE_API bool FillUpSkeletonPose(FPoseData* PoseData, const USkeleton* InSkeleton);
	ENGINE_API void RetrieveSourcePoseFromExistingPose(bool bAdditive, int32 InBasePoseIndex, const TArray<FTransform>& InBasePose, const TArray<float>& InBaseCurve);

	// editor features for full pose <-> additive pose
	// 完整姿势 <-> 附加姿势的编辑器功能
	ENGINE_API void ConvertToFullPose(USkeleton* InSkeleton, const TArray<FTransform>& RefPose);
	ENGINE_API void ConvertToAdditivePose(const TArray<FTransform>& InBasePose, const TArray<float>& InBaseCurve);
#endif // WITH_EDITOR

	ENGINE_API void RebuildCurveIndexTable();
	
	friend class UPoseAsset;
};

template<>
struct TStructOpsTypeTraits<FPoseDataContainer> : public TStructOpsTypeTraitsBase2<FPoseDataContainer>
{
	enum
	{
		WithSerializer = true,
		WithPostSerialize = true,
	};
};


/**
 * Pose Asset that can be blended by weight of curves 
 */
UCLASS(MinimalAPI, BlueprintType)
class UPoseAsset : public UAnimationAsset
{
	GENERATED_UCLASS_BODY()

private:
	/** Animation Pose Data*/
	/** 动画姿势数据*/
	UPROPERTY()
	struct FPoseDataContainer PoseContainer;

	/** Whether or not Additive Pose or not - these are property that needs post process, so */
	/** 无论是否是 Additive Pose - 这些都是需要后期处理的属性，所以 */
	UPROPERTY(Category = Additive, EditAnywhere)
	bool bAdditivePose;

	/** if -1, use ref pose */
	/** 如果-1，则使用参考姿势 */
	UPROPERTY()
	int32 BasePoseIndex;

public: 
	/** Base pose to use when retargeting */
	/** 重定位时使用的基本姿势 */
	UPROPERTY(Category=Animation, EditAnywhere)
	FName RetargetSource;

#if WITH_EDITORONLY_DATA
	/** If RetargetSource is set to Default (None), this is asset for the base pose to use when retargeting. Transform data will be saved in RetargetSourceAssetReferencePose. */
	/** 如果 RetargetSource 设置为默认（无），则这是重定目标时要使用的基本姿势的资源。变换数据将保存在 RetargetSourceAssetReferencePose 中。 */
	UE_DEPRECATED(5.5, "Direct access to RetargetSourceAsset has been deprecated. Please use members GetRetargetSourceAsset & SetRetargetSourceAsset instead.")
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category=Animation, meta = (DisallowedClasses = "/Script/ApexDestruction.DestructibleMesh"))
	TSoftObjectPtr<USkeletalMesh> RetargetSourceAsset;
#endif

	/** When using RetargetSourceAsset, use the post stored here */
	/** 使用 RetargetSourceAsset 时，请使用此处存储的帖子 */
	UPROPERTY()
	TArray<FTransform> RetargetSourceAssetReferencePose;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Category=Source, EditAnywhere)
	TObjectPtr<UAnimSequence> SourceAnimation;

	/** GUID cached when the contained poses were last updated according to SourceAnimation - used to keep track of out-of-date/sync data*/ 
	/** 根据 SourceAnimation 最后更新所包含的姿势时缓存的 GUID - 用于跟踪过时/同步数据*/
	UPROPERTY()	
	FGuid SourceAnimationRawDataGUID;
#endif // WITH_EDITORONLY_DATA

	/**
	* Get Animation Pose from one pose of PoseIndex and with PoseWeight
	* This returns OutPose and OutCurve of one pose of PoseIndex with PoseWeight
	*
	* @param	OutPose				Pose object to fill
	* @param	InOutCurve			Curves to fill
	* @param	PoseIndex			Index of Pose
	* @param	PoseWeight			Weight of pose
	*/
	UE_DEPRECATED(4.26, "Use GetAnimationPose with other signature")
	ENGINE_API bool GetAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const;
	ENGINE_API bool GetAnimationPose(struct FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;

	UE_DEPRECATED(4.26, "Use GetBaseAnimationPose with other signature")
	ENGINE_API void GetBaseAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& OutCurve) const;
	ENGINE_API void GetBaseAnimationPose(struct FAnimationPoseData& OutAnimationPoseData) const;

	virtual bool HasRootMotion() const { return false; }
	virtual bool IsValidAdditive() const { return bAdditivePose; }

	// this is utility function that just cares by names to be used by live link
	// 这是实用程序函数，只关心实时链接使用的名称
	// this isn't fast. Use it at your caution
	// 这并不快。请谨慎使用
	ENGINE_API void GetAnimationCurveOnly(TArray<FName>& InCurveNames, TArray<float>& InCurveValues, TArray<FName>& OutCurveNames, TArray<float>& OutCurveValues) const;

	//Begin UObject Interface
	//开始UObject接口
	virtual void PostLoad() override;
	virtual bool IsPostLoadThreadSafe() const override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
	UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
//End UObject Interface
//结束UObject接口

public:
	ENGINE_API int32 GetNumPoses() const;
	ENGINE_API int32 GetNumCurves() const;
	ENGINE_API int32 GetNumTracks() const;

	UE_DEPRECATED(5.3, "Please use GetPoseFNames.")
	ENGINE_API const TArray<FSmartName> GetPoseNames() const;
	
	ENGINE_API const TArray<FName>& GetPoseFNames() const;
	ENGINE_API const TArray<FName>& GetTrackNames() const;

	UE_DEPRECATED(5.3, "Please use GetCurveFNames.")
	ENGINE_API const TArray<FSmartName> GetCurveNames() const;
	
	ENGINE_API const TArray<FName> GetCurveFNames() const;
	ENGINE_API const TArray<FAnimCurveBase>& GetCurveData() const;
	ENGINE_API const TArray<float> GetCurveValues(const int32 PoseIndex) const;

	/** Find index of a track with a given bone name. Returns INDEX_NONE if not found. */
	/** 查找具有给定骨骼名称的轨道的索引。如果未找到，则返回 INDEX_NONE。 */
	ENGINE_API const int32 GetTrackIndexByName(const FName& InTrackName) const;

	/** 
	 *	Return value of a curve for a particular pose 
	 *	@return	Returns true if OutValue is valid, false if not
	 */
	ENGINE_API bool GetCurveValue(const int32 PoseIndex, const int32 CurveIndex, float& OutValue) const;

	UE_DEPRECATED(5.3, "Please use ContainsPose that takes a FName.")
	bool ContainsPose(const FSmartName& InPoseName) const { return PoseContainer.Contains(InPoseName.DisplayName); }
	ENGINE_API bool ContainsPose(const FName& InPoseName) const;

#if WITH_EDITOR
	/** Renames a specific pose */
	/** 重命名特定姿势 */
	UFUNCTION(BlueprintCallable, Category=PoseAsset)
	void RenamePose(const FName& OriginalPoseName, const FName& NewPoseName);
	
	/** Returns the name of all contained poses */
	/** 返回所有包含的姿势的名称 */
	UFUNCTION(BlueprintPure, Category=PoseAsset)
	void GetPoseNames(TArray<FName>& PoseNames) const;

	/** Returns base pose name, only valid when additive, NAME_None indicates reference pose */
	/** 返回基础姿势名称，仅在累加时有效，NAME_None表示参考姿势 */
	UFUNCTION(BlueprintPure, Category=PoseAsset)
	FName GetBasePoseName() const;

	/** Set base pose index by name, NAME_None indicates reference pose - returns true if set successfully */
	/** 按名称设置基本姿势索引，NAME_None 表示参考姿势 - 如果设置成功则返回 true */
	UFUNCTION(BlueprintCallable, Category=PoseAsset)
    bool SetBasePoseName(const FName& NewBasePoseName);

	UE_DEPRECATED(5.3, "Please use AddPoseWithUniqueName.")
	bool AddOrUpdatePoseWithUniqueName(const USkeletalMeshComponent* MeshComponent, FSmartName* OutPoseName = nullptr) { return false; }
	
	ENGINE_API FName AddPoseWithUniqueName(const USkeletalMeshComponent* MeshComponent);
	
	UE_DEPRECATED(5.3, "Please use AddOrUpdatePose that takes a FName.")
	void AddOrUpdatePose(const FSmartName& PoseName, const USkeletalMeshComponent* MeshComponent, bool bUpdateCurves = true) { AddOrUpdatePose(PoseName.DisplayName, MeshComponent, bUpdateCurves); }

	ENGINE_API void AddOrUpdatePose(const FName& PoseName, const USkeletalMeshComponent* MeshComponent, bool bUpdateCurves = true);

	UE_DEPRECATED(5.3, "Please use AddReferencePose that takes a FName.")
	ENGINE_API void AddReferencePose(const FSmartName& PoseName, const FReferenceSkeleton& ReferenceSkeleton);

	ENGINE_API void AddReferencePose(const FName& PoseName, const FReferenceSkeleton& ReferenceSkeleton);

	UE_DEPRECATED(5.3, "Please use CreatePoseFromAnimation that takes a ptr to an array of FNames.")
	void CreatePoseFromAnimation(class UAnimSequence* AnimSequence, const TArray<FSmartName>* InPoseNames) {}
	
	ENGINE_API void CreatePoseFromAnimation(class UAnimSequence* AnimSequence, const TArray<FName>* InPoseNames = nullptr);

	/** Contained poses are re-generated from the provided Animation Sequence*/
	/** 包含的姿势是从提供的动画序列重新生成的*/
	UFUNCTION(BlueprintCallable, Category=PoseAsset)
	ENGINE_API void UpdatePoseFromAnimation(class UAnimSequence* AnimSequence);

	// Begin AnimationAsset interface
	// 开始AnimationAsset接口
	virtual bool GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive = true) override;
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap) override;
	// End AnimationAsset interface
	// 结束AnimationAsset接口

	UE_DEPRECATED(5.3, "Please use ModifyPoseName that does not take a UID.")
	bool ModifyPoseName(FName OldPoseName, FName NewPoseName, const SmartName::UID_Type* NewUID) { return ModifyPoseName(OldPoseName, NewPoseName); }

	ENGINE_API bool ModifyPoseName(FName OldPoseName, FName NewPoseName);

	
	UE_DEPRECATED(5.3, "Please use RenamePoseOrCurveName.")
	ENGINE_API void RenameSmartName(const FName& InOriginalName, const FName& InNewName);

	// Rename poses or curves using the names supplied
	// 使用提供的名称重命名姿势或曲线
	ENGINE_API void RenamePoseOrCurveName(const FName& InOriginalName, const FName& InNewName);

	UE_DEPRECATED(5.3, "Please use RemovePoseOrCurveNames.")
	ENGINE_API void RemoveSmartNames(const TArray<FName>& InNamesToRemove);

	// Remove poses or curves using the names supplied
	// 使用提供的名称删除姿势或曲线
	ENGINE_API void RemovePoseOrCurveNames(const TArray<FName>& InNamesToRemove);

	// editor interface
	// 编辑器界面
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// Return full (local space, non additive) pose. Will do conversion if PoseAsset is Additive. 
	// 返回完整（局部空间，非附加）姿势。如果 PoseAsset 是 Additive，则会进行转换。
	ENGINE_API bool GetFullPose(int32 PoseIndex, TArray<FTransform>& OutTransforms) const;
	
	// util to return transform of a bone from the pose asset in component space, by walking up tracks in pose asset */
	// util 通过沿着姿势资产中的轨迹向上行走，从组件空间中的姿势资产返回骨骼的变换 */
	ENGINE_API FTransform GetComponentSpaceTransform(FName BoneName, const TArray<FTransform>& LocalTransforms) const;

	// util to return transform of a bone from the pose asset in local space */
	// util 从本地空间中的姿势资源返回骨骼的变换 */
	ENGINE_API const FTransform& GetLocalSpaceTransform(FName BoneName, int32 PoseIndex = 0) const;

	ENGINE_API int32 DeletePoses(TArray<FName> PoseNamesToDelete);
	ENGINE_API int32 DeleteCurves(TArray<FName> CurveNamesToDelete);
	ENGINE_API bool ConvertSpace(bool bNewAdditivePose, int32 NewBasePoseInde);
	const FName GetPoseNameByIndex(int32 InBasePoseIndex) const { return PoseContainer.PoseFNames.IsValidIndex(InBasePoseIndex) ? PoseContainer.PoseFNames[InBasePoseIndex] : NAME_None; }
#endif // WITH_EDITOR

	int32 GetBasePoseIndex() const { return BasePoseIndex;  }
	ENGINE_API const int32 GetPoseIndexByName(const FName& InBasePoseName) const;
	ENGINE_API const int32 GetCurveIndexByName(const FName& InCurveName) const;

#if WITH_EDITOR
private: 
	DECLARE_MULTICAST_DELEGATE(FOnPoseListChangedMulticaster)
	FOnPoseListChangedMulticaster OnPoseListChanged;

public:
	typedef FOnPoseListChangedMulticaster::FDelegate FOnPoseListChanged;

	/** Registers a delegate to be called after the preview animation has been changed */
	/** 注册一个委托，在预览动画更改后调用 */
	FDelegateHandle RegisterOnPoseListChanged(const FOnPoseListChanged& Delegate)
	{
		return OnPoseListChanged.Add(Delegate);
	}
	/** Unregisters a delegate to be called after the preview animation has been changed */
	/** 取消注册要在预览动画更改后调用的委托 */
	void UnregisterOnPoseListChanged(FDelegateHandle Handle)
	{
		OnPoseListChanged.Remove(Handle);
	}

	UE_DEPRECATED(5.3, "Please use GetUniquePoseName scoped to this pose asset.")
	ENGINE_API static FName GetUniquePoseName(const USkeleton* Skeleton);
	UE_DEPRECATED(5.3, "Please use GetUniquePoseName.")
	ENGINE_API static FSmartName GetUniquePoseSmartName(USkeleton* Skeleton);
	
	ENGINE_API static FName GetUniquePoseName(UPoseAsset* PoseAsset);
	ENGINE_API FGuid GetSourceAnimationGuid() const;

protected:
	virtual void RemapTracksToNewSkeleton(USkeleton* NewSkeleton, bool bConvertSpaces) override;
private: 
	// this will do multiple things, it will add tracks and make sure it fix up all poses with it
	// 这将做很多事情，它将添加轨道并确保它修复所有姿势
	// use same as retarget source system we have for animation
	// 使用与我们用于动画的重定向源系统相同的内容
	void CombineTracks(const TArray<FName>& NewTracks);

	void ConvertToFullPose();
	void ConvertToAdditivePose(int32 NewBasePoseIndex);
	bool GetBasePoseTransform(TArray<FTransform>& OutBasePose, TArray<float>& OutCurve) const;
	void Reinitialize();

	// After any update to SourceLocalPoses, this does update runtime data
	// 对 SourceLocalPoses 进行任何更新后，这会更新运行时数据
	void AddOrUpdatePose(const FName& PoseName, const TArray<FName>& TrackNames, const TArray<FTransform>& LocalTransform, const TArray<float>& CurveValues);
	void PostProcessData();
	void BreakAnimationSequenceGUIDComparison();
#endif // WITH_EDITOR	

private:
	void UpdateTrackBoneIndices();
	bool RemoveInvalidTracks();

public:
#if WITH_EDITOR
	// Assigns the passed skeletal mesh to the retarget source
	// 将传递的骨架网格物体分配给重定向源
	UFUNCTION(BlueprintCallable, Category = "Animation")
	ENGINE_API void SetRetargetSourceAsset(USkeletalMesh* InRetargetSourceAsset);

	// Resets the retarget source asset
	// 重置重定向源资源
	UFUNCTION(BlueprintCallable, Category = "Animation")
	ENGINE_API void ClearRetargetSourceAsset();

	// Returns the retarget source asset soft object pointer.
	// 返回重定向源资源软对象指针。
	UFUNCTION(BlueprintPure, Category = "Animation")
	ENGINE_API const TSoftObjectPtr<USkeletalMesh>& GetRetargetSourceAsset() const;

	// Update the retarget data pose from the source, if it exist, else clears the retarget data pose saved in RetargetSourceAssetReferencePose.
	// 从源更新重定位数据姿势（如果存在），否则清除 RetargetSourceAssetReferencePose 中保存的重定位数据姿势。
	// Warning : This function calls LoadSynchronous at the retarget source asset soft object pointer, so it can not be used at PostLoad
	// 警告：此函数在重定向源资源软对象指针处调用 LoadSynchronous，因此不能在 PostLoad 中使用
	UFUNCTION(BlueprintCallable, Category = "Animation")
	ENGINE_API void UpdateRetargetSourceAssetData();
#endif

private:
	const TArray<FTransform>& GetRetargetTransforms() const;
	FName GetRetargetTransformsSourceName() const;

	friend class FPoseAssetDetails;
};
