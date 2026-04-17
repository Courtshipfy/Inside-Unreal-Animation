// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Abstract base class of animation sequence that can be played and evaluated to produce a pose.
 *
 */

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimNotifyQueue.h"
#include "Animation/AnimData/AnimDataModelNotifyCollector.h"
#include "Animation/AnimData/IAnimationDataController.h"

#include "AnimSequenceBase.generated.h"

UENUM()
enum ETypeAdvanceAnim : int
{
	ETAA_Default,
	ETAA_Finished,
	ETAA_Looped
};

struct FAnimationPoseData;
struct FAnimDataModelNotifPayload;
class UAnimDataModel;
class IAnimationDataModel;
enum class EAnimDataModelNotifyType : uint8;

UCLASS(abstract, BlueprintType, MinimalAPI)
class UAnimSequenceBase : public UAnimationAsset
{
	GENERATED_UCLASS_BODY()

public:
	/** Animation notifies, sorted by time (earliest notification first). */
	/** 动画通知，按时间排序（最早的通知在前）。 */
	/** 动画通知，按时间排序（最早的通知在前）。 */
	/** 动画通知，按时间排序（最早的通知在前）。 */
	UPROPERTY()
	TArray<struct FAnimNotifyEvent> Notifies;

	/** 如果以 1.0 的速度播放，则此 AnimSequence 的长度（以秒为单位）。 */
protected:
	/** 如果以 1.0 的速度播放，则此 AnimSequence 的长度（以秒为单位）。 */
	/** Length (in seconds) of this AnimSequence if played back with a speed of 1.0. */
	/** 如果以 1.0 的速度播放，则此 AnimSequence 的长度（以秒为单位）。 */
	UE_DEPRECATED(5.0, "Public access to SequenceLength is deprecated, use GetPlayLength or UAnimDataController::SetPlayLength instead")
	UPROPERTY(Category=Length, AssetRegistrySearchable, VisibleAnywhere, BlueprintReadOnly)
	float SequenceLength;

	/**
	 * Raw uncompressed float curve data
	 */
	UE_DEPRECATED(5.0, "Public access to RawCurveData is deprecated, see UAnimDataModel for source data or use GetCurveData for runtime instead")
	UPROPERTY()
	/** 全局调整该动画播放速率的编号。 */
	struct FRawCurveTracks RawCurveData;
	
	/** 全局调整该动画播放速率的编号。 */
public:
	/** Number for tweaking playback rate of this animation globally. */
	/** 全局调整该动画播放速率的编号。 */
	UPROPERTY(EditAnywhere, Category=Animation)
	float RateScale;
	
	/** 
	 * The default looping behavior of this animation.
	 * Asset players can override this
	 */
	UPROPERTY(EditAnywhere, Category=Animation)
	bool bLoop;
#if WITH_EDITORONLY_DATA
	// if you change Notifies array, this will need to be rebuilt
 // 如果更改通知数组，则需要重新构建
	UPROPERTY()
	TArray<FAnimNotifyTrack> AnimNotifyTracks;
#endif // WITH_EDITORONLY_DATA

	//~ Begin UObject Interface
 // ~ 开始 UObject 接口
	ENGINE_API virtual void PostLoad() override;
	ENGINE_API virtual bool IsPostLoadThreadSafe() const override;
	ENGINE_API virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#if WITH_EDITORONLY_DATA
	/** 如果以 1.0 的速度播放，则返回蒙太奇的总播放长度。 */
	static ENGINE_API void DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass);
	/** 如果以 1.0 的速度播放，则返回蒙太奇的总播放长度。 */
#endif
	/** 按时间对通知数组进行排序，最早的在前。 */
	ENGINE_API virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~ End UObject Interface
 // ~ 结束 UObject 接口
	/** 按时间对通知数组进行排序，最早的在前。 */
	/** 删除指定的通知 */

	/** Returns the total play length of the montage, if played back with a speed of 1.0. */
	/** 删除指定的通知 */
	/** 删除所有通知 */
	/** 如果以 1.0 的速度播放，则返回蒙太奇的总播放长度。 */
	ENGINE_API virtual float GetPlayLength() const override;

	/** 删除所有通知 */
	/** 将所有具有 InOldName 的命名通知重命名为 InNewName */
	/** Sort the Notifies array by time, earliest first. */
	/** 按时间对通知数组进行排序，最早的在前。 */
	ENGINE_API void SortNotifies();	

	/** 将所有具有 InOldName 的命名通知重命名为 InNewName */
	/** Remove the notifies specified */
	/** 删除指定的通知 */
	ENGINE_API bool RemoveNotifies(const TArray<FName>& NotifiesToRemove);
	
	/** Remove all notifies */
	/** 删除所有通知 */
	ENGINE_API void RemoveNotifies();

#if WITH_EDITOR
	/** Renames all named notifies with InOldName to InNewName */
	/** 将所有具有 InOldName 的命名通知重命名为 InNewName */
	ENGINE_API void RenameNotifies(FName InOldName, FName InNewName);
#endif
	
	/**
	/** 将 CurrentTime 时的曲线数据评估到 Instance **/
	* Retrieves AnimNotifies given a StartTime and a DeltaTime.
	* Time will be advanced and support looping if bAllowLooping is true.
	* Supports playing backwards (DeltaTime<0).
	* Returns notifies between StartTime (exclusive) and StartTime+DeltaTime (inclusive)
	*/
	/** 将 CurrentTime 时的曲线数据评估到 Instance **/
	ENGINE_API void GetAnimNotifies(const float& StartTime, const float& DeltaTime, FAnimNotifyContext& NotifyContext) const;

	/**
	* Retrieves AnimNotifies between two time positions. ]PreviousPosition, CurrentPosition]
	* Between PreviousPosition (exclusive) and CurrentPosition (inclusive).
	* Supports playing backwards (CurrentPosition<PreviousPosition).
	* Only supports contiguous range, does NOT support looping and wrapping over.
	/** 返回为此动画采样的按键总数，包括 T0 键 **/
	*/
	ENGINE_API virtual void GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float & CurrentPosition, FAnimNotifyContext& NotifyContext) const;

	/** 动画采样的返回率 **/
	/** Evaluate curve data to Instance at the time of CurrentTime **/
	/** 将 CurrentTime 时的曲线数据评估到 Instance **/
	UE_DEPRECATED(5.6, "Please use EvaluateCurveData with FAnimExtractContext")
	/** 获取给定帧的时间 */
	/** 返回为此动画采样的按键总数，包括 T0 键 **/
	virtual void EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime, bool bForceUseRawData = false) const { const FAnimExtractContext Context(static_cast<double>(CurrentTime)); EvaluateCurveData(OutCurve, Context, bForceUseRawData); }
	ENGINE_API virtual void EvaluateCurveData(FBlendedCurve& OutCurve, const FAnimExtractContext& AnimExtractContext, bool bForceUseRawData = false) const;
	
	/** 获取指定时间的帧数 */
	/** 动画采样的返回率 **/
	UE_DEPRECATED(5.6, "Please use EvaluateCurveData with FAnimExtractContext")
	virtual float EvaluateCurveData(FName CurveName, float CurrentTime, bool bForceUseRawData = false) const { const FAnimExtractContext Context(static_cast<double>(CurrentTime)); return EvaluateCurveData(CurveName, Context, bForceUseRawData); }
	ENGINE_API virtual float EvaluateCurveData(FName CurveName, const FAnimExtractContext& AnimExtractContext, bool bForceUseRawData = false) const;
	/** 获取给定帧的时间 */
	
	/** 修复任何位于序列末尾之外的通知 */
	ENGINE_API virtual const FRawCurveTracks& GetCurveData() const;
		
	ENGINE_API virtual bool HasCurveData(FName CurveName, bool bForceUseRawData = false) const;
	/** 计算在给定显示时间的情况下应将什么偏移量（如果有）应用于通知的触发时间 */
	/** 获取指定时间的帧数 */

	/** Return the total number of keys sampled for this animation, including the T0 key **/
	/** 返回为此动画采样的按键总数，包括 T0 键 **/
	ENGINE_API virtual int32 GetNumberOfSampledKeys() const;

	/** Return rate at which the animation is sampled **/
	/** 修复任何位于序列末尾之外的通知 */
	/** 动画采样的返回率 **/
	ENGINE_API virtual FFrameRate GetSamplingFrameRate() const;

	/** 计算在给定显示时间的情况下应将什么偏移量（如果有）应用于通知的触发时间 */
	/** Get the time at the given frame */
	/** 获取给定帧的时间 */
	ENGINE_API virtual float GetTimeAtFrame(const int32 Frame) const;

#if WITH_EDITOR
	/** Get the frame number for the provided time */
	/** 获取指定时间的帧数 */
	ENGINE_API virtual int32 GetFrameAtTime(const float Time) const;

	// @todo document
 // @todo文档
	ENGINE_API void InitializeNotifyTrack();

	/** Fix up any notifies that are positioned beyond the end of the sequence */
	/** 修复任何位于序列末尾之外的通知 */
	ENGINE_API void ClampNotifiesAtEndOfSequence();

	/** Calculates what (if any) offset should be applied to the trigger time of a notify given its display time */ 
	/** 计算在给定显示时间的情况下应将什么偏移量（如果有）应用于通知的触发时间 */
	ENGINE_API virtual EAnimEventTriggerOffsets::Type CalculateOffsetForNotify(float NotifyDisplayTime) const;

	ENGINE_API virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
	UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
	ENGINE_API virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;

	// Get a pointer to the data for a given Anim Notify
 // 获取指向给定动画通知数据的指针
	ENGINE_API uint8* FindNotifyPropertyData(int32 NotifyIndex, FArrayProperty*& ArrayProperty);

	// Get a pointer to the data for a given array property item
 // 获取指向给定数组属性项的数据的指针
	ENGINE_API uint8* FindArrayProperty(const TCHAR* PropName, FArrayProperty*& ArrayProperty, int32 ArrayIndex);

protected:
	ENGINE_API virtual void RefreshParentAssetData() override;
#endif	//WITH_EDITORONLY_DATA
public: 
	// update cache data (notify tracks, sync markers)
 // 更新缓存数据（通知曲目、同步标记）
	ENGINE_API virtual void RefreshCacheData();

	//~ Begin UAnimationAsset Interface
 // ~ 开始 UAnimationAsset 接口
	ENGINE_API virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const override;
	//~ End UAnimationAsset Interface
 // ~ 结束 UAnimationAsset 接口
	
	ENGINE_API void TickByMarkerAsFollower(FMarkerTickRecord &Instance, FMarkerTickContext &MarkerContext, float& CurrentTime, float& OutPreviousTime, const float MoveDelta, const bool bLooping, const UMirrorDataTable* MirrorTable = nullptr) const;
	ENGINE_API void TickByMarkerAsLeader(FMarkerTickRecord& Instance, FMarkerTickContext& MarkerContext, float& CurrentTime, float& OutPreviousTime, const float MoveDelta, const bool bLooping, const UMirrorDataTable* MirrorTable = nullptr) const;

	/**
	* Get Bone Transform of the Time given, relative to Parent for all RequiredBones
	* This returns different transform based on additive or not. Or what kind of additive.
	*
	* @param	OutPose				Pose object to fill
	* @param	OutCurve			Curves to fill
	* @param	ExtractionContext	Extraction Context (position, looping, root motion, etc.)
	*/	
	virtual void GetAnimationPose(FAnimationPoseData& OutPoseData, const FAnimExtractContext& ExtractionContext) const
		PURE_VIRTUAL(UAnimSequenceBase::GetAnimationPose, );
	
	ENGINE_API virtual void HandleAssetPlayerTickedInternal(FAnimAssetTickContext &Context, const float PreviousTime, const float MoveDelta, const FAnimTickRecord &Instance, struct FAnimNotifyQueue& NotifyQueue) const;

	virtual bool HasRootMotion() const { return false; }

	UE_DEPRECATED(5.6, "Please use ExtractRootMotion with FAnimExtractContext")
	virtual FTransform ExtractRootMotion(float StartTime, float DeltaTime, bool bAllowLooping) const { const FAnimExtractContext Context(static_cast<double>(StartTime), true, FDeltaTimeRecord(DeltaTime), bAllowLooping); return ExtractRootMotion(Context); }

	// Extract Root Motion transform from the animation
 // 从动画中提取根运动变换
	virtual FTransform ExtractRootMotion(const FAnimExtractContext& ExtractionContext) const { return {}; }

	UE_DEPRECATED(5.6, "Please use ExtractRootMotionFromRange with FAnimExtractContext")
	virtual FTransform ExtractRootMotionFromRange(float StartTrackPosition, float EndTrackPosition) const { const FAnimExtractContext Context; return ExtractRootMotionFromRange(StartTrackPosition, EndTrackPosition, Context); }

	// Extract Root Motion transform from a contiguous position range (no looping)
 // 从连续位置范围提取根运动变换（无循环）
	virtual FTransform ExtractRootMotionFromRange(double StartTime, double EndTime, const FAnimExtractContext& ExtractionContext) const { return {}; }

	UE_DEPRECATED(5.6, "Please use ExtractRootTrackTransform with FAnimExtractContext")
	virtual FTransform ExtractRootTrackTransform(float Time, const FBoneContainer* RequiredBones) const { const FAnimExtractContext Context(static_cast<double>(Time)); return ExtractRootTrackTransform(Context, RequiredBones); }

	// Extract the transform from the root track for the given animation position
 // 从给定动画位置的根轨道中提取变换
	virtual FTransform ExtractRootTrackTransform(const FAnimExtractContext& ExtractionContext, const FBoneContainer* RequiredBones) const { return {}; }

	ENGINE_API virtual void Serialize(FArchive& Ar) override;

	virtual void AdvanceMarkerPhaseAsLeader(bool bLooping, float MoveDelta, const TArray<FName>& ValidMarkerNames, float& CurrentTime, FMarkerPair& PrevMarker, FMarkerPair& NextMarker, TArray<FPassedMarker>& MarkersPassed, const UMirrorDataTable* MirrorTable) const { check(false); /*Should never call this (either missing override or calling on unsupported asset */ }
	/** 注册一个委托，以便在通知更改后调用*/

	virtual void AdvanceMarkerPhaseAsFollower(const FMarkerTickContext& Context, float DeltaRemaining, bool bLooping, float& CurrentTime, FMarkerPair& PreviousMarker, FMarkerPair& NextMarker, const UMirrorDataTable* MirrorTable) const { check(false); /*Should never call this (either missing override or calling on unsupported asset */ }
	
	/** 注册一个委托，以便在通知更改后调用*/
	virtual void GetMarkerIndicesForTime(float CurrentTime, bool bLooping, const TArray<FName>& ValidMarkerNames, FMarkerPair& OutPrevMarker, FMarkerPair& OutNextMarker) const { check(false); /*Should never call this (either missing override or calling on unsupported asset */ }

	virtual FMarkerSyncAnimPosition GetMarkerSyncPositionFromMarkerIndicies(int32 PrevMarker, int32 NextMarker, float CurrentTime, const UMirrorDataTable* MirrorTable) const { check(false); return FMarkerSyncAnimPosition(); /*Should never call this (either missing override or calling on unsupported asset */ }

	virtual void GetMarkerIndicesForPosition(const FMarkerSyncAnimPosition& SyncPosition, bool bLooping, FMarkerPair& OutPrevMarker, FMarkerPair& OutNextMarker, float& CurrentTime, const UMirrorDataTable* MirrorTable ) const { check(false); /*Should never call this (either missing override or calling on unsupported asset */ }
	
	virtual float GetFirstMatchingPosFromMarkerSyncPos(const FMarkerSyncAnimPosition& InMarkerSyncGroupPosition) const { return 0.f; }
	virtual float GetNextMatchingPosFromMarkerSyncPos(const FMarkerSyncAnimPosition& InMarkerSyncGroupPosition, const float& StartingPosition) const { return 0.f; }
	virtual float GetPrevMatchingPosFromMarkerSyncPos(const FMarkerSyncAnimPosition& InMarkerSyncGroupPosition, const float& StartingPosition) const { return 0.f; }

	// default implementation, no additive
 // 默认实现，无添加
	/** 返回嵌入此 UAnimSequenceBase 中的 IAnimationDataModel 对象 */
	virtual EAdditiveAnimationType GetAdditiveAnimType() const { return AAT_None; }
	virtual bool CanBeUsedInComposition() const { return true;  }

	/** 返回 IAnimationDataModel 作为脚本接口，提供对 UObject 和 Interface 的访问 */
	/** 返回嵌入此 UAnimSequenceBase 中的 IAnimationDataModel 对象 */
	// to support anim sequence base to montage
 // 支持动画序列基础到蒙太奇
	virtual void EnableRootMotionSettingFromMontage(bool bInEnableRootMotion, const ERootMotionRootLock::Type InRootMotionRootLock) {};
	/** 返回设置为对 DataModel 进行操作的瞬态 UAnimDataController */
	/** 返回 IAnimationDataModel 作为脚本接口，提供对 UObject 和 Interface 的访问 */
	virtual bool GetEnableRootMotionSettingFromMontage() const { return false; }

	/** 根据任何预先存在的数据填充 UAnimDataModel 对象。 （覆盖期望根据其数据填充模型） */
#if WITH_EDITOR
	/** 返回设置为对 DataModel 进行操作的瞬态 UAnimDataController */
private:
	DECLARE_MULTICAST_DELEGATE( FOnNotifyChangedMulticaster );
	/** 为嵌入对象注册到 UAnimDatModel::GetModifiedEvent 的回调 */
	FOnNotifyChangedMulticaster OnNotifyChanged;
	/** 根据任何预先存在的数据填充 UAnimDataModel 对象。 （覆盖期望根据其数据填充模型） */

	/** 验证 DataModel 包含有效的 UAnimDataModel 对象 */
public:
	typedef FOnNotifyChangedMulticaster::FDelegate FOnNotifyChanged;

	/** 绑定到 DataModel 其修改委托 */
	/** 为嵌入对象注册到 UAnimDatModel::GetModifiedEvent 的回调 */
	/** Registers a delegate to be called after notification has changed*/
	/** 注册一个委托，以便在通知更改后调用*/
	/** 将当前的 DataModel（如果有）替换为提供的 DataModel */
	ENGINE_API void RegisterOnNotifyChanged(const FOnNotifyChanged& Delegate);
	/** 验证 DataModel 包含有效的 UAnimDataModel 对象 */
	ENGINE_API void UnregisterOnNotifyChanged(FDelegateUserObject Unregister);
	/** 创建一个新的 UAnimDataModel 实例并相应地设置 DataModel */
	virtual bool IsValidToPlay() const { return true; }
	// ideally this would be animsequcnebase, but we might have some issue with that. For now, just allow AnimSequence
 // 理想情况下，这将是 animsequcnebase，但我们可能会遇到一些问题。现在，只允许 AnimSequence
	/** 绑定到 DataModel 其修改委托 */
	virtual class UAnimSequence* GetAdditiveBasePose() const { return nullptr; }
#endif
	/** 将当前的 DataModel（如果有）替换为提供的 DataModel */

	// return true if anim notify is available 
 // 如果动画通知可用，则返回 true
	/** 创建一个新的 UAnimDataModel 实例并相应地设置 DataModel */
	ENGINE_API virtual bool IsNotifyAvailable() const;

#if WITH_EDITOR
	ENGINE_API void OnEndLoadPackage(const FEndLoadPackageContext& Context);
	ENGINE_API virtual void OnAnimModelLoaded();
public:
	/** Returns the IAnimationDataModel object embedded in this UAnimSequenceBase */
	/** 返回嵌入此 UAnimSequenceBase 中的 IAnimationDataModel 对象 */
	ENGINE_API IAnimationDataModel* GetDataModel() const;

	/** Returns the IAnimationDataModel as a script-interface, provides access to UObject and Interface */
	/** 返回 IAnimationDataModel 作为脚本接口，提供对 UObject 和 Interface 的访问 */
	ENGINE_API TScriptInterface<IAnimationDataModel> GetDataModelInterface() const;
	/** 包含（源）动画数据的 IAnimationDataModel 实例 */

	/** Returns the transient UAnimDataController set to operate on DataModel */
	/** 返回设置为对 DataModel 进行操作的瞬态 UAnimDataController */
	ENGINE_API IAnimationDataController& GetController();
	/** 每当数据模型最初填充时（在升级路径期间）设置标志 */
protected:
	/** Populates the UAnimDataModel object according to any pre-existing data. (overrides expect to populate the model according to their data) */
	/** 根据任何预先存在的数据填充 UAnimDataModel 对象。 （覆盖期望根据其数据填充模型） */
	/** UAnimDataController 实例设置为在 DataModel 上操作 */
	ENGINE_API virtual void PopulateModel();
	ENGINE_API virtual void PopulateWithExistingModel(TScriptInterface<IAnimationDataModel> ExistingDataModel);
	/** 包含（源）动画数据的 IAnimationDataModel 实例 */

	/** 跟踪任何控制器括号以及在此期间广播的所有唯一通知类型的帮助程序对象 */
	/** Callback registered to UAnimDatModel::GetModifiedEvent for the embedded object */
	/** 为嵌入对象注册到 UAnimDatModel::GetModifiedEvent 的回调 */
	ENGINE_API virtual void OnModelModified(const EAnimDataModelNotifyType& NotifyType, IAnimationDataModel* Model, const FAnimDataModelNotifPayload& Payload);
	/** 每当数据模型最初填充时（在升级路径期间）设置标志 */

	/** Validates that DataModel contains a valid UAnimDataModel object */
	/** 验证 DataModel 包含有效的 UAnimDataModel 对象 */
	/** UAnimDataController 实例设置为在 DataModel 上操作 */
	ENGINE_API void ValidateModel() const;
	
	/** Binds to DataModel its modification delegate */
	/** 绑定到 DataModel 其修改委托 */
	/** 跟踪任何控制器括号以及在此期间广播的所有唯一通知类型的帮助程序对象 */
	ENGINE_API void BindToModelModificationEvent();

	/** Replaces the current DataModel, if any, with the provided one */
	/** 将当前的 DataModel（如果有）替换为提供的 DataModel */
	ENGINE_API void CopyDataModel(const TScriptInterface<IAnimationDataModel>& ModelToDuplicate);
private:
	/** Creates a new UAnimDataModel instance and sets DataModel accordingly */
	/** 创建一个新的 UAnimDataModel 实例并相应地设置 DataModel */
	ENGINE_API void CreateModel();

public:
	ENGINE_API bool ShouldDataModelBeValid() const;
	bool IsDataModelValid() const
	{
		if(ShouldDataModelBeValid())
		{
			return DataModelInterface != nullptr;
		}

		return false;
	}	
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
protected:
	UE_DEPRECATED(5.1, "DataModel has been converted to DataModelInterface")
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation Model")
	TObjectPtr<UAnimDataModel> DataModel;

	/** IAnimationDataModel instance containing (source) animation data */
	/** 包含（源）动画数据的 IAnimationDataModel 实例 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation Model")
	TScriptInterface<IAnimationDataModel> DataModelInterface;

	/** Flag set whenever the data-model is initially populated (during upgrade path) */
	/** 每当数据模型最初填充时（在升级路径期间）设置标志 */
	bool bPopulatingDataModel;

	/** UAnimDataController instance set to operate on DataModel */
	/** UAnimDataController 实例设置为在 DataModel 上操作 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "Animation Model")
	TScriptInterface<IAnimationDataController> Controller;
	
	/** Helper object that keeps track of any controller brackets, and all unique notify types that are broadcasted during it */
	/** 跟踪任何控制器括号以及在此期间广播的所有唯一通知类型的帮助程序对象 */
	UE::Anim::FAnimDataModelNotifyCollector NotifyCollector;
#endif // WITH_EDITORONLY_DATA

};
