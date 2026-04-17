// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * One animation sequence of keyframes. Contains a number of tracks of data.
 *
 */

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimCompressionTypes.h"
#include "CustomAttributes.h"
#include "Containers/ArrayView.h"
#include "Animation/CustomAttributes.h"
#include "Animation/AnimData/AnimDataNotifications.h"
#include "Animation/AttributeCurve.h"
#include "Async/SharedRecursiveMutex.h"
#include "UObject/PerPlatformProperties.h"
#include "IO/IoHash.h"

#if WITH_EDITOR
#include "AnimData/IAnimationDataModel.h"
#endif // WITH_EDITOR

#include "AnimSequence.generated.h"

typedef TArray<FTransform> FTransformArrayA2;

class USkeletalMesh;
class FQueuedThreadPool;
class ITargetPlatform;
enum class EQueuedWorkPriority : uint8;
struct FAnimCompressContext;
struct FAnimSequenceDecompressionContext;
struct FCompactPose;

namespace UE { namespace Anim { class FAnimSequenceCompilingManager; namespace Compression { struct FScopedCompressionGuard; } class FAnimationSequenceAsyncCacheTask; } }
namespace UE::UAF { class FDecompressionTools; }

extern ENGINE_API int32 GPerformFrameStripping;

// These two always should go together, but it is not right now. 
// 这两个人总是应该在一起的，但现在不是时候。
// I wonder in the future, we change all compressed to be inside as well, so they all stay together
// 我想知道将来我们也将所有压缩都改为内部，这样它们就都在一起了
// When remove tracks, it should be handled together 
// 拆除轨道时应一并处理
USTRUCT()
struct FAnimSequenceTrackContainer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<struct FRawAnimSequenceTrack> AnimationTracks;

	UPROPERTY()
	TArray<FName>						TrackNames;

	// @todo expand this struct to work better and assign data better
 // @todo 扩展此结构以更好地工作并更好地分配数据
	void Initialize(int32 NumNode)
	{
		AnimationTracks.Empty(NumNode);
		AnimationTracks.AddZeroed(NumNode);
		TrackNames.Empty(NumNode);
		TrackNames.AddZeroed(NumNode);
	}

	void Initialize(TArray<FName> InTrackNames)
	{
		TrackNames = MoveTemp(InTrackNames);
		const int32 NumNode = TrackNames.Num();
		AnimationTracks.Empty(NumNode);
		AnimationTracks.AddZeroed(NumNode);
	}

	int32 GetNum() const
	{
		check (TrackNames.Num() == AnimationTracks.Num());
		return (AnimationTracks.Num());
	}
};

/**
 * Keyframe position data for one track.  Pos(i) occurs at Time(i).  Pos.Num() always equals Time.Num().
 */
USTRUCT()
struct FTranslationTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FVector3f> PosKeys;

	UPROPERTY()
	TArray<float> Times;
};

/**
 * Keyframe rotation data for one track.  Rot(i) occurs at Time(i).  Rot.Num() always equals Time.Num().
 */
USTRUCT()
struct FRotationTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FQuat4f> RotKeys;

	UPROPERTY()
	TArray<float> Times;
};

/**
 * Keyframe scale data for one track.  Scale(i) occurs at Time(i).  Rot.Num() always equals Time.Num().
 */
USTRUCT()
struct FScaleTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FVector3f> ScaleKeys;

	UPROPERTY()
	TArray<float> Times;
};


/**
 * Key frame curve data for one track
 * CurveName: Morph Target Name
 * CurveWeights: List of weights for each frame
 */
USTRUCT()
struct FCurveTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName CurveName;

	/** 如果数组中存在有效的曲线权重，则返回 true*/
	UPROPERTY()
	TArray<float> CurveWeights;

	/** 如果数组中存在有效的曲线权重，则返回 true*/
	/** Returns true if valid curve weight exists in the array*/
	/** 如果数组中存在有效的曲线权重，则返回 true*/
	ENGINE_API bool IsValidCurveTrack();
	
	/** This is very simple cut to 1 key method if all is same since I see so many redundant same value in every frame 
	 *  Eventually this can get more complicated 
	 *  Will return true if compressed to 1. Return false otherwise **/
	ENGINE_API bool CompressCurveWeights();
};

USTRUCT()
struct FCompressedTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<uint8> ByteStream;

	UPROPERTY()
	TArray<float> Times;

	UPROPERTY()
	float Mins[3];

	UPROPERTY()
	float Ranges[3];


	FCompressedTrack()
	{
		for (int32 ElementIndex = 0; ElementIndex < 3; ElementIndex++)
		{
			Mins[ElementIndex] = 0;
		}
		for (int32 ElementIndex = 0; ElementIndex < 3; ElementIndex++)
		{
			Ranges[ElementIndex] = 0;
		}
/** 枚举用于决定是否应该在专用服务器上剥离动画数据 */
	}

/** 枚举用于决定是否应该在专用服务器上剥离动画数据 */
};

	/** 如果项目设置中的“在专用服务器上剥离动画数据”选项为 true 并且 EnableRootMotion 为 false，则在专用服务器上剥离轨道数据 */
/** Enum used to decide whether we should strip animation data on dedicated server */
	/** 如果项目设置中的“在专用服务器上剥离动画数据”选项为 true 并且 EnableRootMotion 为 false，则在专用服务器上剥离轨道数据 */
/** 枚举用于决定是否应该在专用服务器上剥离动画数据 */
	/** 只要 EnableRootMotion 为 false，无论项目设置中“在专用服务器上剥离动画数据”选项的值如何，都会在专用服务器上剥离轨道数据  */
	/** 只要 EnableRootMotion 为 false，无论项目设置中“在专用服务器上剥离动画数据”选项的值如何，都会在专用服务器上剥离轨道数据  */
UENUM()
enum class EStripAnimDataOnDedicatedServerSettings : uint8
	/** 无论项目设置中“在专用服务器上剥离动画数据”选项的值如何，都不要在专用服务器上剥离轨道数据  */
	/** 无论项目设置中“在专用服务器上剥离动画数据”选项的值如何，都不要在专用服务器上剥离轨道数据  */
{
	/** Strip track data on dedicated server if 'Strip Animation Data on Dedicated Server' option in Project Settings is true and EnableRootMotion is false */
	/** 如果项目设置中的“在专用服务器上剥离动画数据”选项为 true 并且 EnableRootMotion 为 false，则在专用服务器上剥离轨道数据 */
	UseProjectSetting,
	/** Strip track data on dedicated server regardless of the value of 'Strip Animation Data on Dedicated Server' option in Project Settings as long as EnableRootMotion is false  */
	/** 只要 EnableRootMotion 为 false，无论项目设置中“在专用服务器上剥离动画数据”选项的值如何，都会在专用服务器上剥离轨道数据  */
	StripAnimDataOnDedicatedServer,
	/** Do not strip track data on dedicated server regardless of the value of 'Strip Animation Data on Dedicated Server' option in Project Settings  */
	/** 无论项目设置中“在专用服务器上剥离动画数据”选项的值如何，都不要在专用服务器上剥离轨道数据  */
	/** 导入文件的 DCC 帧速率。仅 UI 信息，单位为 Hz */
	DoNotStripAnimDataOnDedicatedServer
	/** 导入文件的 DCC 帧速率。仅 UI 信息，单位为 Hz */
};

	/** 导入期间计算的重采样帧速率。仅 UI 信息，单位为 Hz */
UCLASS(config=Engine, hidecategories=(UObject, Length), BlueprintType, MinimalAPI)
class UAnimSequence : public UAnimSequenceBase
	/** 导入期间计算的重采样帧速率。仅 UI 信息，单位为 Hz */
{
	GENERATED_UCLASS_BODY()
	/** 包含各个动画轨道中预期的关键点数量。 */

#if WITH_EDITORONLY_DATA
	/** The DCC framerate of the imported file. UI information only, unit are Hz */
	/** 包含各个动画轨道中预期的关键点数量。 */
	/** 导入文件的 DCC 帧速率。仅 UI 信息，单位为 Hz */
	/** 各个（非均匀）动画轨道中预期的关键点数量。 */
	UPROPERTY(AssetRegistrySearchable, meta = (DisplayName = "Import File Framerate"))
	float ImportFileFramerate;

	/** The resample framerate that was computed during import. UI information only, unit are Hz */
	/** 各个（非均匀）动画轨道中预期的关键点数量。 */
	/** 对源动画进行采样的帧速率。 */
	/** 导入期间计算的重采样帧速率。仅 UI 信息，单位为 Hz */
	UPROPERTY(AssetRegistrySearchable, meta = (DisplayName = "Import Resample Framerate"))
	int32 ImportResampleFramerate;

protected:
	/** 对源动画进行采样的帧速率。 */
	/** Contains the number of keys expected within the individual animation tracks. */
	/** 包含各个动画轨道中预期的关键点数量。 */
	UE_DEPRECATED(5.0, "NumFrames is deprecated see UAnimDataModel::GetNumberOfFrames for the number of source data frames, or GetNumberOfSampledKeys for the target keys")
	UPROPERTY()
	int32 NumFrames;

	/** The number of keys expected within the individual (non-uniform) animation tracks. */
	/** 各个（非均匀）动画轨道中预期的关键点数量。 */
	UE_DEPRECATED(5.0, "NumberOfKeys is deprecated see UAnimDataModel::GetNumberOfKeys for the number of source data keys, or GetNumberOfSampledKeys for the target keys")
	UPROPERTY()
	int32 NumberOfKeys;

	/** The frame rate at which the source animation is sampled. */
	/** 对源动画进行采样的帧速率。 */
	UE_DEPRECATED(5.0, "SamplingFrameRate is deprecated see UAnimDataModel::GetFrameRate for the source frame rate, or GetSamplingFrameRate for the target frame rate instead")
	UPROPERTY()
	FFrameRate SamplingFrameRate;

	UE_DEPRECATED(5.0, "RawAnimationData has been deprecated see FBoneAnimationTrack::InternalTrackData")
	TArray<struct FRawAnimSequenceTrack> RawAnimationData;

	// Update this if the contents of RawAnimationData changes
 // 如果 RawAnimationData 的内容发生更改，请更新此内容
	UE_DEPRECATED(5.1, "RawDataGuid has been deprecated see GenerateGuidFromModel instead")
	UPROPERTY()
	FGuid RawDataGuid;

	/**
	 * This is name of RawAnimationData tracks for editoronly - if we lose skeleton, we'll need relink them
	 */
	UE_DEPRECATED(5.0, "Animation track names has been deprecated see FBoneAnimationTrack::Name")
	UPROPERTY(VisibleAnywhere, DuplicateTransient, Category="Animation")
	TArray<FName> AnimationTrackNames;

	/**
	 * Source RawAnimationData. Only can be overridden by when transform curves are added first time OR imported
	 */
	/** 用于按此顺序压缩骨骼的骨骼压缩设置。 */
	TArray<struct FRawAnimSequenceTrack> SourceRawAnimationData_DEPRECATED;
public:

	/**
	/** 用于压缩此序列中的曲线的曲线压缩设置。 */
	 * Allow frame stripping to be performed on this animation if the platform requests it
	/** 用于按此顺序压缩骨骼的骨骼压缩设置。 */
	 * Can be disabled if animation has high frequency movements that are being lost.
	 */
	UPROPERTY(Category = Compression, EditAnywhere)
	bool bAllowFrameStripping;
	/** 用于压缩此序列中的曲线的曲线压缩设置。 */

	/**
	 * Set a scale for error threshold on compression. This is useful if the animation will 
	 * be played back at a different scale (e.g. if you know the animation will be played
	/** 附加动画类型。 **/
	 * on an actor/component that is scaled up by a factor of 10, set this value to 10)
	 */
	UPROPERTY(Category = Compression, EditAnywhere)
	float CompressionErrorThresholdScale;
	/* 附加参考姿势类型。请参阅上面的枚举类型 */
#endif

	/** The bone compression settings used to compress bones in this sequence. */
	/** 附加动画类型。 **/
	/* 如果 RefPoseType == AnimFrame，则添加参考帧 **/
	/** 用于按此顺序压缩骨骼的骨骼压缩设置。 */
	UPROPERTY(Category = Compression, EditAnywhere, meta = (ForceShowEngineContent))
	TObjectPtr<class UAnimBoneCompressionSettings> BoneCompressionSettings;

	/* 附加参考动画（如果相关） - 即 AnimScaled 或 AnimFrame **/
	/* 附加参考姿势类型。请参阅上面的枚举类型 */
	/** The curve compression settings used to compress curves in this sequence. */
	/** 用于压缩此序列中的曲线的曲线压缩设置。 */
	UPROPERTY(Category = Compression, EditAnywhere, meta = (ForceShowEngineContent))
	/** 重定位时使用的基本姿势 */
	TObjectPtr<class UAnimCurveCompressionSettings> CurveCompressionSettings;
	/* 如果 RefPoseType == AnimFrame，则添加参考帧 **/

	// CompressedData is only valid in cook/non-editor runtime, see DataByPlatformKeyHash for editor-runtime data
 // 压缩数据仅在厨师/非编辑器运行时有效，请参阅 DataByPlatformKeyHash 了解编辑器运行时数据
	UE_DEPRECATED(5.6, "CompressedData public access will be removed")
	/** 如果 RetargetSource 设置为默认（无），则这是重定目标时要使用的基本姿势的资源。变换数据将保存在 RetargetSourceAssetReferencePose 中。 */
	/* 附加参考动画（如果相关） - 即 AnimScaled 或 AnimFrame **/
	FCompressedAnimSequence CompressedData;

	UPROPERTY(Category = Compression, EditAnywhere, meta = (ForceShowEngineContent))
	TObjectPtr<class UVariableFrameStrippingSettings> VariableFrameStrippingSettings;
	/** 重定位时使用的基本姿势 */
	/** 使用 RetargetSourceAsset 时，请使用此处存储的帖子 */

	/** Additive animation type. **/
	/** 附加动画类型。 **/
	UPROPERTY(EditAnywhere, Category=AdditiveSettings, AssetRegistrySearchable)
	/** 这定义了如何计算键之间的值 **/
	TEnumAsByte<enum EAdditiveAnimationType> AdditiveAnimType;
	/** 如果 RetargetSource 设置为默认（无），则这是重定目标时要使用的基本姿势的资源。变换数据将保存在 RetargetSourceAssetReferencePose 中。 */

	/* Additive refrerence pose type. Refer above enum type */
	/** 如果打开，它将允许提取根运动 **/
	/* 附加参考姿势类型。请参阅上面的枚举类型 */
	UPROPERTY(EditAnywhere, Category=AdditiveSettings, meta=(DisplayName = "Base Pose Type"))
	TEnumAsByte<enum EAdditiveBasePoseType> RefPoseType;

	/** 提取根运动时，根骨骼将被锁定到该位置。**/
	/** 使用 RetargetSourceAsset 时，请使用此处存储的帖子 */
	/* Additve reference frame if RefPoseType == AnimFrame **/
	/* 如果 RefPoseType == AnimFrame，则添加参考帧 **/
	UPROPERTY(EditAnywhere, Category = AdditiveSettings)
	/** 即使未启用根运动，也强制根骨锁定 */
	int32 RefFrameIndex;
	/** 这定义了如何计算键之间的值 **/
	
	/* Additive reference animation if it's relevant - i.e. AnimScaled or AnimFrame **/
	/** 如果启用此选项，它将使用标准化比例值来提取提取的根运动：FVector(1.0, 1.0, 1.0) **/
	/* 附加参考动画（如果相关） - 即 AnimScaled 或 AnimFrame **/
	UPROPERTY(EditAnywhere, Category=AdditiveSettings, meta=(DisplayName = "Base Pose Animation"))
	/** 如果打开，它将允许提取根运动 **/
	TObjectPtr<class UAnimSequence> RefPoseSeq;
	/** 我们是否从拥有的蒙太奇复制了根运动设置 */

	/** Base pose to use when retargeting */
	/** 重定位时使用的基本姿势 */
	/** 提取根运动时，根骨骼将被锁定到该位置。**/
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category=Animation)
	/** 使用 CompressAnimations commandlet 保存的版本号。帮助多遍完成。 */
	FName RetargetSource;

#if WITH_EDITORONLY_DATA
	/** 即使未启用根运动，也强制根骨锁定 */
	/** If RetargetSource is set to Default (None), this is asset for the base pose to use when retargeting. Transform data will be saved in RetargetSourceAssetReferencePose. */
	/** 如果 RetargetSource 设置为默认（无），则这是重定目标时要使用的基本姿势的资源。变换数据将保存在 RetargetSourceAssetReferencePose 中。 */
	UE_DEPRECATED(5.5, "Direct access to RetargetSourceAsset has been deprecated. Please use members GetRetargetSourceAsset & SetRetargetSourceAsset instead.")
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category=Animation, meta = (DisallowedClasses = "/Script/ApexDestruction.DestructibleMesh"))
	/** 如果启用此选项，它将使用标准化比例值来提取提取的根运动：FVector(1.0, 1.0, 1.0) **/
	TSoftObjectPtr<USkeletalMesh> RetargetSourceAsset;
#endif
	/** 导入用于该网格的数据和选项 */

	/** When using RetargetSourceAsset, use the post stored here */
	/** 我们是否从拥有的蒙太奇复制了根运动设置 */
	/** 使用 RetargetSourceAsset 时，请使用此处存储的帖子 */
	/***  用于重新进口 **/
	UPROPERTY()
	/** 用于构造此骨架网格物体的资源的路径 */
	TArray<FTransform> RetargetSourceAssetReferencePose;

	/** This defines how values between keys are calculated **/
	/** 使用 CompressAnimations commandlet 保存的版本号。帮助多遍完成。 */
	/** 上次导入的文件的日期/时间戳 */
	/** 这定义了如何计算键之间的值 **/
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = Animation)
	EAnimInterpolationType Interpolation;
	
	/** If this is on, it will allow extracting of root motion **/
	/** 如果打开，它将允许提取根运动 **/
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = RootMotion)
	bool bEnableRootMotion;
	/** 枚举用于决定是否应该在专用服务器上剥离动画数据 */

	/** Root Bone will be locked to that position when extracting root motion.**/
	/** 提取根运动时，根骨骼将被锁定到该位置。**/
	/** 导入用于该网格的数据和选项 */
	UPROPERTY(EditAnywhere, Category = RootMotion)
	TEnumAsByte<ERootMotionRootLock::Type> RootMotionRootLock;
	
	/** Force Root Bone Lock even if Root Motion is not enabled */
	/***  用于重新进口 **/
	/** 即使未启用根运动，也强制根骨锁定 */
	/** 用于构造此骨架网格物体的资源的路径 */
	UPROPERTY(EditAnywhere, Category = RootMotion)
	bool bForceRootLock;

	/** If this is on, it will use a normalized scale value for the root motion extracted: FVector(1.0, 1.0, 1.0) **/
	/** 上次导入的文件的日期/时间戳 */
	/** 如果启用此选项，它将使用标准化比例值来提取提取的根运动：FVector(1.0, 1.0, 1.0) **/
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = RootMotion)
	bool bUseNormalizedRootMotionScale;

	/** Have we copied root motion settings from an owning montage */
	/** 我们是否从拥有的蒙太奇复制了根运动设置 */
	UPROPERTY()
	bool bRootMotionSettingsCopiedFromMontage;
	/** 枚举用于决定是否应该在专用服务器上剥离动画数据 */

#if WITH_EDITORONLY_DATA
	/** Saved version number with CompressAnimations commandlet. To help with doing it in multiple passes. */
	/** 使用 CompressAnimations commandlet 保存的版本号。帮助多遍完成。 */
	UPROPERTY()
	int32 CompressCommandletVersion;

	/**
	 * Do not attempt to override compression scheme when running CompressAnimations commandlet.
	 * Some high frequency animations are too sensitive and shouldn't be changed.
	 */
	UPROPERTY(EditAnywhere, Category=Compression)
	uint32 bDoNotOverrideCompression:1;

	/** Importing data and options used for this mesh */
	/** 导入用于该网格的数据和选项 */
	UPROPERTY(VisibleAnywhere, Instanced, Category=ImportSettings)
	TObjectPtr<class UAssetImportData> AssetImportData;

	/***  for Reimport **/
	/***  用于重新进口 **/
	/** Path to the resource used to construct this skeletal mesh */
	/** 用于构造此骨架网格物体的资源的路径 */
	UPROPERTY()
	FString SourceFilePath_DEPRECATED;

	/** Date/Time-stamp of the file from the last import */
	/** 上次导入的文件的日期/时间戳 */
	UPROPERTY()
	FString SourceFileTimestamp_DEPRECATED;

	// Track whether we have updated markers so cached data can be updated
 // 跟踪我们是否更新了标记，以便可以更新缓存的数据
	int32 MarkerDataUpdateCounter;
#endif // WITH_EDITORONLY_DATA

	/** Enum used to decide whether we should strip animation data on dedicated server */
	/** 枚举用于决定是否应该在专用服务器上剥离动画数据 */
	UPROPERTY(EditAnywhere, Category = Compression)
	EStripAnimDataOnDedicatedServerSettings StripAnimDataOnDedicatedServer = EStripAnimDataOnDedicatedServerSettings::UseProjectSetting;

public:
	//~ Begin UObject Interface
 // ~ 开始 UObject 接口
	ENGINE_API virtual void Serialize(FArchive& Ar) override;
	ENGINE_API virtual void PostInitProperties() override;
	ENGINE_API virtual void PostLoad() override;
	ENGINE_API virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	ENGINE_API virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;
#if WITH_EDITOR
	ENGINE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	ENGINE_API virtual void BeginCacheForCookedPlatformData(const ITargetPlatform* TargetPlatform) override;
	ENGINE_API virtual bool IsCachedCookedPlatformDataLoaded(const ITargetPlatform* TargetPlatform) override;
	ENGINE_API virtual void WillNeverCacheCookedPlatformDataAgain() override;
	ENGINE_API virtual void ClearAllCachedCookedPlatformData() override;
	ENGINE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif // WITH_EDITOR
	ENGINE_API virtual void BeginDestroy() override;
	ENGINE_API virtual bool IsReadyForFinishDestroy() override;
	ENGINE_API virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
	UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
	ENGINE_API virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	static ENGINE_API void AddReferencedObjects(UObject* This, FReferenceCollector& Collector);
	//~ End UObject Interface
 // ~ 结束 UObject 接口

	//~ Begin UAnimationAsset Interface
 // ~ 开始 UAnimationAsset 接口
	ENGINE_API virtual bool IsValidAdditive() const override;
	virtual TArray<FName>* GetUniqueMarkerNames() { return &UniqueMarkerNames; }
#if WITH_EDITOR
	ENGINE_API virtual bool GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive = true) override;
	ENGINE_API virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap) override;
	ENGINE_API virtual void OnSetSkeleton(USkeleton* NewSkeleton) override;
#endif
	//~ End UAnimationAsset Interface
 // ~ 结束 UAnimationAsset 接口

	//~ Begin UAnimSequenceBase Interface
 // ~ 开始 UAnimSequenceBase 接口
	ENGINE_API virtual void HandleAssetPlayerTickedInternal(FAnimAssetTickContext &Context, const float PreviousTime, const float MoveDelta, const FAnimTickRecord &Instance, struct FAnimNotifyQueue& NotifyQueue) const override;
	virtual bool HasRootMotion() const override { return bEnableRootMotion; }
	ENGINE_API virtual void RefreshCacheData() override;
	virtual EAdditiveAnimationType GetAdditiveAnimType() const override { return AdditiveAnimType; }
	ENGINE_API virtual int32 GetNumberOfSampledKeys() const override;
	virtual FFrameRate GetSamplingFrameRate() const override { return PlatformTargetFrameRate.Default; }
	ENGINE_API virtual void EvaluateCurveData(FBlendedCurve& OutCurve, const FAnimExtractContext& AnimExtractContext, bool bForceUseRawData = false) const override;
	ENGINE_API virtual float EvaluateCurveData(FName CurveName, const FAnimExtractContext& AnimExtractContext, bool bForceUseRawData = false) const override;
	// This will only check the current platform its compressed data (if valid)
 // 这只会检查当前平台的压缩数据（如果有效）
	ENGINE_API virtual bool HasCurveData(FName CurveName, bool bForceUseRawData) const override;

	//~ End UAnimSequenceBase Interface
 // ~ 结束 UAnimSequenceBase 接口

	UE_DEPRECATED(5.6, "Please use ExtractRootMotion with FAnimExtractContext")
	virtual FTransform ExtractRootMotion(float StartTime, float DeltaTime, bool bAllowLooping) const override final { PRAGMA_DISABLE_DEPRECATION_WARNINGS return UAnimSequenceBase::ExtractRootMotion(StartTime, DeltaTime, bAllowLooping); PRAGMA_ENABLE_DEPRECATION_WARNINGS }
	UE_DEPRECATED(5.6, "Please use ExtractRootMotionFromRange with FAnimExtractContext")
	virtual FTransform ExtractRootMotionFromRange(float StartTrackPosition, float EndTrackPosition) const override final { PRAGMA_DISABLE_DEPRECATION_WARNINGS return UAnimSequenceBase::ExtractRootMotionFromRange(StartTrackPosition, EndTrackPosition); PRAGMA_ENABLE_DEPRECATION_WARNINGS }
	UE_DEPRECATED(5.6, "Please use ExtractRootTrackTransform with FAnimExtractContext")
	virtual FTransform ExtractRootTrackTransform(float Time, const FBoneContainer* RequiredBones) const override final { PRAGMA_DISABLE_DEPRECATION_WARNINGS return UAnimSequenceBase::ExtractRootTrackTransform(Time, RequiredBones); PRAGMA_ENABLE_DEPRECATION_WARNINGS }
	
	/** 使用序列长度和采样键数量更新存储的采样帧率 */
	// Extract Root Motion transform from the animation
 // 从动画中提取根运动变换
	ENGINE_API virtual FTransform ExtractRootMotion(const FAnimExtractContext& ExtractionContext) const override final;
	// Extract Root Motion transform from a contiguous position range (no looping)
 // 从连续位置范围提取根运动变换（无循环）
	ENGINE_API virtual FTransform ExtractRootMotionFromRange(double StartTime, double EndTime, const FAnimExtractContext& ExtractionContext) const override final;
	// Extract the transform from the root track for the given animation position
 // 从给定动画位置的根轨道中提取变换
	ENGINE_API virtual FTransform ExtractRootTrackTransform(const FAnimExtractContext& ExtractionContext, const FBoneContainer* RequiredBones) const override final;

	// Begin Transform related functions 
 // 开始Transform相关函数
	ENGINE_API virtual void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const override;
	
	/**
	* Get Bone Transform of the Time given, relative to Parent for all RequiredBones
	* This returns different transform based on additive or not. Or what kind of additive.
	/** 使用序列长度和采样键数量更新存储的采样帧率 */
	*
	* @param	OutAnimationPoseData  [out] Animation Pose related data to populate
	* @param	ExtractionContext	  Extraction Context (position, looping, root motion, etc.)
	* @param	bForceUseRawData	  Whether or not to forcefully try to sample the animation data model	
	*/
	ENGINE_API void GetBonePose(struct FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext, bool bForceUseRawData = false) const;

	UE_DEPRECATED(5.6, "GetCompressedTrackToSkeletonMapTable has been deprecated. Use GetCompressedData().CompressedTrackToSkeletonMapTable instead")
	const TArray<FTrackToSkeletonMap>& GetCompressedTrackToSkeletonMapTable() const
	{
		static TArray<FTrackToSkeletonMap> TempArray;
		return TempArray;
	}
	
	UE_DEPRECATED(5.6, "GetCompressedCurveIndexedNames has been deprecated. Use GetCompressedData().IndexedCurveNames instead")
	const TArray<FAnimCompressedCurveIndexedName>& GetCompressedCurveIndexedNames() const
	{
		static TArray<FAnimCompressedCurveIndexedName> TempArray;
		return TempArray;
	}

#if WITH_EDITORONLY_DATA
protected:
	ENGINE_API void UpdateCompressedCurveName(const FName& OldCurveName, const FName& NewCurveName);
#endif

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
#if WITH_EDITORONLY_DATA
	/** Updates the stored sampling frame-rate using the sequence length and number of sampling keys */
	/** 使用序列长度和采样键数量更新存储的采样帧率 */
	UE_DEPRECATED(5.0, "UpdateFrameRate has been deprecated see UAnimDataController::SetFrameRate")
	ENGINE_API void UpdateFrameRate();
#endif
	
public:
	ENGINE_API const TArray<FTransform>& GetRetargetTransforms() const;
	ENGINE_API FName GetRetargetTransformsSourceName() const;

	/**
	* Retarget a single bone transform, to apply right after extraction.
	*
	* @param	BoneTransform		BoneTransform to read/write from.
	* @param	SkeletonBoneIndex	Bone Index in USkeleton.
	* @param	BoneIndex			Bone Index in Bone Transform array.
	* @param	RequiredBones		BoneContainer
	*/
	ENGINE_API void RetargetBoneTransform(FTransform& BoneTransform, const int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex, const FBoneContainer& RequiredBones, const bool bIsBakedAdditive) const;

	/**
	* Get Bone Transform of the additive animation for the Time given, relative to Parent for all RequiredBones
	*
	* @param	OutAnimationPoseData	[out] Output pose data
	* @param	ExtractionContext		Extraction Context (position, looping, root motion, etc.)
	*/
	ENGINE_API void GetBonePose_Additive(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;

	/**
	* Get Bone Transform of the base (reference) pose of the additive animation for the Time given, relative to Parent for all RequiredBones
	*
	* @param	OutAnimationPoseData	[out] Output pose data
	* @param	ExtractionContext	Extraction Context (position, looping, root motion, etc.)
	*/
	ENGINE_API void GetAdditiveBasePose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;

	UE_DEPRECATED(5.6, "Please use GetBoneTransform with FAnimExtractContext")
	void GetBoneTransform(FTransform& OutAtom, FSkeletonPoseBoneIndex BoneIndex, double Time, bool bUseRawData, TOptional<EAnimInterpolationType> InterpolationOverride=TOptional<EAnimInterpolationType>()) const { FAnimExtractContext ExtractionContext(Time); ExtractionContext.InterpolationOverride = InterpolationOverride; return GetBoneTransform(OutAtom, BoneIndex, ExtractionContext, bUseRawData); }

	/**
	 * Get Bone Transform of the Time given, relative to Parent for the Track Given
	 *
	 * @param	OutAtom					[out] Output bone transform.
	 * @param	BoneIndex				Index of bone to evaluate.
	 * @param	ExtractionContext		Extraction context containing time and interpolation info.
	 * @param	bUseRawData		If true, use raw animation data instead of compressed data.
	 */
	ENGINE_API void GetBoneTransform(FTransform& OutAtom, FSkeletonPoseBoneIndex BoneIndex, const FAnimExtractContext& ExtractionContext, bool bUseRawData) const;

	/**
	 * Get Bone Transform of the Time given, relative to Parent for the Track Given. This will sample the compressed data for the current platform only. (if available)
	 *
	 * @param	OutAtom			[out] Output bone transform.
	 * @param	BoneIndex		Index of bone to evaluate.
	 * @param	DecompContext	Decompression context to use.
	 * @param	bUseRawData		If true, use raw animation data instead of compressed data.
	 */
	UE_DEPRECATED(5.6, "Please use GetBoneTransform with FAnimExtractContext")
	ENGINE_API void GetBoneTransform(FTransform& OutAtom, FSkeletonPoseBoneIndex BoneIndex, FAnimSequenceDecompressionContext& DecompContext, bool bUseRawData) const;
	// End Transform related functions 
 // 结束Transform相关函数

	// Begin Memory related functions
 // 开始内存相关功能

	/** @return	estimate uncompressed raw size. This is *not* the real raw size. 
				Here we estimate what it would be with no trivial compression. */
#if WITH_EDITOR
	ENGINE_API int64 GetUncompressedRawSize() const;

	/**
	 * @return		The approximate size of raw animation data.
	 */
	ENGINE_API int64 GetApproxRawSize() const;

	/**
	 * @return		The approximate size of raw bone animation data.
	 */
	ENGINE_API int64 GetApproxBoneRawSize() const;

	/**
	 * @return		The approximate size of raw curve animation data.
	 */
	ENGINE_API int64 GetApproxCurveRawSize() const;
#endif // WITH_EDITOR

	/**
	 * @return		The approximate size of compressed animation data for only bones.
	 */
	ENGINE_API int32 GetApproxBoneCompressedSize() const;
	
	/**
	 * @return		The approximate size of compressed animation data.
	 */
	ENGINE_API int32 GetApproxCompressedSize() const;
protected:
	/**
	 * @return		The approximate size of compressed animation data for only bones.
	 */
	ENGINE_API int32 GetApproxBoneCompressedSize_Lockless() const;
	
	/**
	 * @return		The approximate size of compressed animation data.
	 */
	ENGINE_API int32 GetApproxCompressedSize_Lockless() const;

	void EvaluateCurveData_Lockless(FBlendedCurve& OutCurve, const FAnimExtractContext& AnimExtractContext, bool bForceUseRawData = false) const;	
	float EvaluateCurveData_Lockless(FName CurveName, const FAnimExtractContext& AnimExtractContext, bool bForceUseRawData = false) const;	
	/** 按时间对同步标记数组进行排序，最早的在前。 */
	FTransform ExtractRootTrackTransform_Lockless(const FAnimExtractContext& ExtractionContext, const FBoneContainer* RequiredBones) const;

	ENGINE_API void GetBoneTransform_Lockless(FTransform& OutAtom, FSkeletonPoseBoneIndex BoneIndex, const FAnimExtractContext& ExtractionContext, bool bForceUseRawData) const;	

	/** 删除所有具有指定名称的标记 */
#if WITH_EDITOR
	ENGINE_API bool ShouldPerformStripping(const bool bPerformFrameStripping, const bool bPerformStrippingOnOddFramedAnims) const;
#endif // WITH_EDITOR
	/** 使用指定名称重命名标记 */
	/** 按时间对同步标记数组进行排序，最早的在前。 */


protected:
	UE_DEPRECATED(5.6, "ClearCompressedBoneData will be removed")
	/** 删除所有具有指定名称的标记 */
    void ClearCompressedBoneData() {}
	UE_DEPRECATED(5.6, "ClearCompressedCurveData will be removed")
    void ClearCompressedCurveData() {}
	/** 使用指定名称重命名标记 */
    // Write the compressed data to the supplied FArchive
    // 将压缩数据写入提供的 FArchive
    ENGINE_API void SerializeCompressedData(FArchive& Ar, bool bDDCData);
	ENGINE_API void SerializeCompressedData(FArchive& Ar, FCompressedAnimSequence& CompressedData);
#if WITH_EDITOR
	ENGINE_API virtual void OnAnimModelLoaded() override;
#endif
public:
	ENGINE_API bool IsCompressedDataValid() const;
	ENGINE_API bool IsBoneCompressedDataValid() const;	
	ENGINE_API bool IsCurveCompressedDataValid() const;
	// End Memory related functions
 // 结束内存相关函数
#if WITH_EDITOR
	/**
	 * Add Key to Transform Curves
	 */
	ENGINE_API void AddKeyToSequence(float Time, const FName& BoneName, const FTransform& AdditiveTransform);

	UE_DEPRECATED(5.6, "DoesNeedRecompress has been renamed to IsCompressedDataOutOfDate")
	bool DoesNeedRecompress() const { return IsCompressedDataOutOfDate(); }

	/**
	* Return true if compressed data is invalid or if it is not in sync with the skeleton
	*/
	ENGINE_API bool IsCompressedDataOutOfDate() const;

	/**
	 * Create Animation Sequence from Reference Pose of the Mesh
	 */
	ENGINE_API bool CreateAnimation(class USkeletalMesh* Mesh);
	/**
	 * Create Animation Sequence from the Mesh Component's current bone transform
	 */
	ENGINE_API bool CreateAnimation(class USkeletalMeshComponent* MeshComponent);
	/**
	/** 重置骨骼动画、曲线数据和通知轨迹 **/
	 * Create Animation Sequence from the given animation
	 */
	ENGINE_API bool CreateAnimation(class UAnimSequence* Sequence);

	/** 
	 * Add validation check to see if it's being ready to play or not
	 */
	ENGINE_API virtual bool IsValidToPlay() const override;
	/** 重置骨骼动画、曲线数据和通知轨迹 **/

	// Get a pointer to the data for a given Anim Notify
 // 获取指向给定动画通知数据的指针
	ENGINE_API uint8* FindSyncMarkerPropertyData(int32 SyncMarkerIndex, FArrayProperty*& ArrayProperty);

	virtual int32 GetMarkerUpdateCounter() const { return MarkerDataUpdateCounter; }
#endif
	/** 根据（仅限编辑器）数据是否已被剥离，返回是否可以评估原始（源）动画数据 */

	/** Sort the sync markers array by time, earliest first. */
	/** 按时间对同步标记数组进行排序，最早的在前。 */
	ENGINE_API void SortSyncMarkers();

#if WITH_EDITOR
	/** Remove all markers with the specified names */
	/** 删除所有具有指定名称的标记 */
	/** 根据（仅限编辑器）数据是否已被剥离，返回是否可以评估原始（源）动画数据 */
	ENGINE_API bool RemoveSyncMarkers(const TArray<FName>& MarkersToRemove);
	/** 重定向功能 */

	/** Rename the markers with the specified name */
	/** 使用指定名称重命名标记 */
	ENGINE_API bool RenameSyncMarkers(FName InOldName, FName InNewName);
#endif
	/** 刷新同步标记数据*/

	// Advancing based on markers
 // 根据标记前进
	ENGINE_API float GetCurrentTimeFromMarkers(FMarkerPair& PrevMarker, FMarkerPair& NextMarker, float PositionBetweenMarkers) const;
	/** 获取一组标记位置并根据请求的起始位置验证它们，根据需要更新它们 */
	/** 重定向功能 */
	ENGINE_API virtual void AdvanceMarkerPhaseAsLeader(bool bLooping, float MoveDelta, const TArray<FName>& ValidMarkerNames, float& CurrentTime, FMarkerPair& PrevMarker, FMarkerPair& NextMarker, TArray<FPassedMarker>& MarkersPassed, const UMirrorDataTable* MirrorTable) const;
	ENGINE_API virtual void AdvanceMarkerPhaseAsFollower(const FMarkerTickContext& Context, float DeltaRemaining, bool bLooping, float& CurrentTime, FMarkerPair& PreviousMarker, FMarkerPair& NextMarker, const UMirrorDataTable* MirrorTable) const;
	ENGINE_API virtual void GetMarkerIndicesForTime(float CurrentTime, bool bLooping, const TArray<FName>& ValidMarkerNames, FMarkerPair& OutPrevMarker, FMarkerPair& OutNextMarker) const;

	ENGINE_API virtual FMarkerSyncAnimPosition GetMarkerSyncPositionFromMarkerIndicies(int32 PrevMarker, int32 NextMarker, float CurrentTime, const UMirrorDataTable* MirrorTable) const;
	/** 刷新同步标记数据*/
	/** 创作的同步标记 */
	ENGINE_API virtual void GetMarkerIndicesForPosition(const FMarkerSyncAnimPosition& SyncPosition, bool bLooping, FMarkerPair& OutPrevMarker, FMarkerPair& OutNextMarker, float& CurrentTime, const UMirrorDataTable* MirrorTable) const;
	
	ENGINE_API virtual float GetFirstMatchingPosFromMarkerSyncPos(const FMarkerSyncAnimPosition& InMarkerSyncGroupPosition) const override;
	/** 获取一组标记位置并根据请求的起始位置验证它们，根据需要更新它们 */
	/** 该动画序列中唯一标记名称的列表 */
	ENGINE_API virtual float GetNextMatchingPosFromMarkerSyncPos(const FMarkerSyncAnimPosition& InMarkerSyncGroupPosition, const float& StartingPosition) const override;
	ENGINE_API virtual float GetPrevMatchingPosFromMarkerSyncPos(const FMarkerSyncAnimPosition& InMarkerSyncGroupPosition, const float& StartingPosition) const override;

	// to support anim sequence base to all montages
 // 支持所有蒙太奇的动画序列基础
	ENGINE_API virtual void EnableRootMotionSettingFromMontage(bool bInEnableRootMotion, const ERootMotionRootLock::Type InRootMotionRootLock) override;
	ENGINE_API virtual bool GetEnableRootMotionSettingFromMontage() const override;
	/** 创作的同步标记 */

#if WITH_EDITOR
	virtual class UAnimSequence* GetAdditiveBasePose() const override 
	{ 
	/** 该动画序列中唯一标记名称的列表 */
		if (IsValidAdditive())
		{
			return RefPoseSeq;
		}

		return nullptr;
	}

	// Is this animation valid for baking into additive
 // 该动画对于烘焙成添加剂是否有效
	ENGINE_API bool CanBakeAdditive() const;

	// Test whether at any point we will scale a bone to 0 (needed for validating additive anims)
 // 测试是否在任何时候我们都会将骨骼缩放到 0（验证附加动画所需）
	ENGINE_API bool DoesSequenceContainZeroScale() const;

	// Helper function to allow us to notify animations that depend on us that they need to update
 // 帮助函数允许我们通知依赖于我们的动画需要更新
	ENGINE_API void FlagDependentAnimationsAsRawDataOnly() const;

	// Helper function to allow us to update streaming animations that depend on us with our data when we are updated
 // 帮助函数允许我们在更新时更新依赖于我们数据的流动画
	ENGINE_API void UpdateDependentStreamingAnimations() const;

	/** Resets Bone Animation, Curve data and Notify tracks **/
	/** 重置骨骼动画、曲线数据和通知轨迹 **/
	ENGINE_API void ResetAnimation();
#endif

private:
	/**
	* Get Bone Transform of the animation for the Time given, relative to Parent for all RequiredBones
	* This return mesh rotation only additive pose
	*
	* @param	OutAnimationPoseData	[out] Output pose data
	* @param	ExtractionContext		Extraction Context (position, looping, root motion, etc.)
	*/
	ENGINE_API void GetBonePose_AdditiveMeshRotationOnly(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const;

protected:
	/** Returns whether or not evaluation of the raw (source) animation data is possible according to whether or not the (editor only) data has been stripped */
	/** 根据（仅限编辑器）数据是否已被剥离，返回是否可以评估原始（源）动画数据 */
	ENGINE_API virtual bool CanEvaluateRawAnimationData() const;

private:
#if WITH_EDITOR
	/**
	 * Remap Tracks to New Skeleton
	 */ 
	ENGINE_API virtual void RemapTracksToNewSkeleton( USkeleton* NewSkeleton, bool bConvertSpaces ) override;

	/** Retargeting functions */
	/** 重定向功能 */
	ENGINE_API int32 GetSpaceBasedAnimationData(TArray< TArray<FTransform> > & AnimationDataInComponentSpace) const;
#endif

public:
	/** Refresh sync marker data*/
	/** 刷新同步标记数据*/
	ENGINE_API void RefreshSyncMarkerDataFromAuthored();

	/** Take a set of marker positions and validates them against a requested start position, updating them as desired */
	/** 获取一组标记位置并根据请求的起始位置验证它们，根据需要更新它们 */
	ENGINE_API void ValidateCurrentPosition(const FMarkerSyncAnimPosition& Position, bool bPlayingForwards, bool bLooping, float&CurrentTime, FMarkerPair& PreviousMarker, FMarkerPair& NextMarker, const UMirrorDataTable* MirrorTable = nullptr) const;

	UE_DEPRECATED(5.6, "Public access to UseRawDataForPoseExtraction has been deprecated")
	ENGINE_API bool UseRawDataForPoseExtraction(const FBoneContainer& RequiredBones) const;

public:
	/** Authored Sync markers */
	/** 创作的同步标记 */
	UPROPERTY()
	TArray<FAnimSyncMarker>		AuthoredSyncMarkers;

	/** List of Unique marker names in this animation sequence */
	/** 该动画序列中唯一标记名称的列表 */
	TArray<FName>				UniqueMarkerNames;

public:
	ENGINE_API void EvaluateAttributes(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext, bool bUseRawData) const;	
protected:
#if WITH_EDITOR
	ENGINE_API void SynchronousAnimatedBoneAttributesCompression();
	ENGINE_API void MoveAttributesToModel();

	// Begin UAnimSequenceBase virtual overrides
 // 开始 UAnimSequenceBase 虚拟覆盖
	ENGINE_API virtual void OnModelModified(const EAnimDataModelNotifyType& NotifyType, IAnimationDataModel* Model, const FAnimDataModelNotifPayload& Payload) override;
	ENGINE_API virtual void PopulateModel() override;
	// End UAnimSequenceBase virtual overrides
 // 结束 UAnimSequenceBase 虚拟覆盖

	ENGINE_API void EnsureValidRawDataGuid();
	ENGINE_API void CalculateNumberOfSampledKeys();
	ENGINE_API void DeleteBoneAnimationData();
	ENGINE_API void DeleteDeprecatedRawAnimationData();
public:
	ENGINE_API void DeleteNotifyTrackData();
	// Resets the bone and curve compression settings to the project default 
 // 将骨骼和曲线压缩设置重置为项目默认值
	ENGINE_API void ResetCompressionSettings();
#endif // WITH_EDITOR

protected:
	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "Animation")
	FPerPlatformFrameRate PlatformTargetFrameRate;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FFrameRate TargetFrameRate;	

	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "Animation", Transient, DuplicateTransient)
	int32 NumberOfSampledKeys;

	UPROPERTY(VisibleAnywhere, Category = "Animation", Transient, DuplicateTransient)
	int32 NumberOfSampledFrames;

	bool bBlockCompressionRequests;
private:
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UE_DEPRECATED(5.0, "PerBoneCustomAttributeData has been deprecated see UAnimDataModel::AnimatedBoneAttributes")
	UPROPERTY(VisibleAnywhere, EditFixedSize, Category=CustomAttributes)
	TArray<FCustomAttributePerBoneData> PerBoneCustomAttributeData;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif // WITH_EDITORONLY_DATA

protected:
	UPROPERTY()
	TMap<FAnimationAttributeIdentifier, FAttributeCurve> AttributeCurves;

#if WITH_EDITOR
	ENGINE_API FIoHash CreateDerivedDataKeyHash(const ITargetPlatform* TargetPlatform) const;
	FString CreateDerivedDataKeyString(const ITargetPlatform* TargetPlatform) const;

	ENGINE_API FIoHash BeginCacheDerivedData(const ITargetPlatform* TargetPlatform);
	ENGINE_API bool PollCacheDerivedData(const FIoHash& KeyHash) const;
	ENGINE_API void EndCacheDerivedData(const FIoHash& KeyHash);

	mutable FCompressedAnimSequence* CurrentPlatformData = nullptr; 
	TMap<FIoHash, TUniquePtr<FCompressedAnimSequence>> DataByPlatformKeyHash;
	TMap<FIoHash, TPimplPtr<UE::Anim::FAnimationSequenceAsyncCacheTask>> CacheTasksByKeyHash;
	mutable UE::FSharedRecursiveMutex SharedCompressedDataMutex;

	// Cached FIoHash keys stored according to ITargetPlatform type-hash
 // 根据 ITargetPlatform 类型哈希存储的缓存 FIoHash 密钥
	mutable TMap<uint32, FIoHash> PlatformHashToKeyHash;
	mutable FRWLock HashCacheLock;
	// Whether or not compressed data should be cleared in case of residency releasing (cook-time behaviour only)
 // 驻留释放时是否应清除压缩数据（仅限烹饪时行为）
	TAtomic<bool> bShouldClearCompressedData = false;
	
	// Bi-directional multi-maps keeping track of required (target-platform specific) compressed data, stored by hash and provided identifier from API usage
 // 双向多映射跟踪所需的（特定于目标平台的）压缩数据，通过哈希存储并通过 API 使用提供标识符
	TMultiMap<uint32, FIoHash> ResidencyReferencerHashes;
	TMultiMap<FIoHash, uint32> PlatformHashToReferencers;

	mutable FRWLock ResidencyLock;

	ENGINE_API bool TryCancelAsyncTasks();
	ENGINE_API bool WaitForAsyncTasks(float TimeLimitSeconds);
	ENGINE_API void FinishAsyncTasks();
	ENGINE_API void Reschedule(FQueuedThreadPool* InThreadPool, EQueuedWorkPriority InPriority);
	ENGINE_API bool IsAsyncTaskComplete() const;
	ENGINE_API bool IsCompiling() const;
	ENGINE_API bool RequiresResidency(const FIoHash& InKeyHash) const;

	ENGINE_API void ValidateCompressionSettings();

	const FCompressedAnimSequence& GetPlatformCompressedData(const ITargetPlatform* InTargetPlatform) const;
#endif // WITH_EDITOR

	// Whether or not RawData should be sampled during pose extraction, either because compressed data is not available or through user directive
 // 是否应在姿势提取期间对 RawData 进行采样，因为压缩数据不可用或通过用户指令
	ENGINE_API bool ShouldUseRawDataForPoseExtraction(const FBoneContainer& RequiredBones, const FAnimExtractContext& ExtractionContext) const;
	ENGINE_API bool ShouldUseRawDataForPoseExtraction_Lockless(const FBoneContainer& RequiredBones, const FAnimExtractContext& ExtractionContext) const;
	ENGINE_API bool ShouldUseRawDataForPoseExtraction_Lockless(const FAnimExtractContext& ExtractionContext) const;

	// Clears all currently cached compressed data (all platform data)
 // 清除所有当前缓存的压缩数据（所有平台数据）
	ENGINE_API void ClearAllCompressionData();
	// Clears compressed data, if currently cached, for specified hash
 // 清除指定哈希的压缩数据（如果当前已缓存）
	ENGINE_API void ClearCompressionData(const FIoHash& InKeyHash);
	// Whether or not compressed data, for specified hash, is currently cached
 // 当前是否缓存指定哈希的压缩数据
	ENGINE_API bool HasCompressedDataForHash(const FIoHash& InKeyHash) const;

private:	
	/** 在读取模式下进入/离开压缩数据锁的帮助程序结构 */
	// Returns compressed animation data for specified Hash or TargetPlatform (not thread-safe see FCompressedAnimationDataReadScope and FCompressedAnimationResidencyScope)
 // 返回指定 Hash 或 TargetPlatform 的压缩动画数据（不是线程安全的，请参阅 FcompressedAnimationDataReadScope 和 FcompressedAnimationResidencyScope）
	/** 在读取模式下进入/离开压缩数据锁的帮助程序结构 */
	ENGINE_API const FCompressedAnimSequence& GetPlatformCompressedData(const FAnimExtractContext& AnimExtractContext) const;
	ENGINE_API FCompressedAnimSequence& GetPlatformCompressedData(const FAnimExtractContext& AnimExtractContext);
	ENGINE_API FCompressedAnimSequence& GetPlatformCompressedData(const ITargetPlatform* InTargetPlatform);
	
	// Returns compressed animation data for the current platform (not thread-safe in editor-builds see FCompressedAnimationDataReadScope and FCompressedAnimationResidencyScope)
 // 返回当前平台的压缩动画数据（在编辑器构建中不是线程安全的，请参阅 FcompressedAnimationDataReadScope 和 FcompressedAnimationResidencyScope）
	ENGINE_API const FCompressedAnimSequence& GetCompressedData_Internal() const;
	// Returns compressed animation data for the current platform (not thread-safe in editor-builds see FCompressedAnimationDataReadScope and FCompressedAnimationResidencyScope)
 // 返回当前平台的压缩动画数据（在编辑器构建中不是线程安全的，请参阅 FcompressedAnimationDataReadScope 和 FcompressedAnimationResidencyScope）
	/** 在写入模式下进入/离开压缩数据锁的帮助程序结构 */
	ENGINE_API FCompressedAnimSequence& GetCompressedData_Internal();

public:	
	/** 在写入模式下进入/离开压缩数据锁的帮助程序结构 */
	ENGINE_API void BeginCacheDerivedDataForCurrentPlatform();
	ENGINE_API void CacheDerivedDataForCurrentPlatform();
#if WITH_EDITOR
	ENGINE_API void WaitOnExistingCompression(const bool bWantResults=true);
	
	// Returns hash identifying compressed animation data with the specified target platform its settings (these can overlap between different platforms due to the settings matching)
 // 返回标识压缩动画数据及其指定目标平台及其设置的哈希值（由于设置匹配，这些可能在不同平台之间重叠）
	ENGINE_API FIoHash GetDerivedDataKeyHash(const ITargetPlatform* TargetPlatform) const;
	
	// Whether or not compressed data, for specified platform, is currently cached
 // 当前是否缓存指定平台的压缩数据
	ENGINE_API bool HasCompressedDataForPlatform(const ITargetPlatform* InPlatform) const;	
	// Synchronous caching of compressed animation data for provided target platform
 // 为提供的目标平台同步缓存压缩动画数据
	ENGINE_API void CacheDerivedDataForPlatform(const ITargetPlatform* TargetPlatform);

	// Whether or not animation sequence can be compressed (dependent on loading state)
 // 动画序列是否可以压缩（取决于加载状态）
	ENGINE_API bool CanBeCompressed() const;
	
	// Whether or not compressed animation data has been requested to stay resident until released 
 // 压缩动画数据是否已被请求保留至释放
	ENGINE_API bool HasResidency(const ITargetPlatform* InPlatform) const;
	ENGINE_API bool HasResidency(uint32 InReferencerHash) const;
	// Fetches and keeps resident the compressed animation data for the provided target platform (until released)
 // 获取并保留所提供目标平台的压缩动画数据（直到发布）
	ENGINE_API void RequestResidency(const ITargetPlatform* InPlatform, uint32 InReferencerHash);
	// Releases previously requested residency of compressed animation data
 // 释放先前请求的压缩动画数据驻留
	ENGINE_API void ReleaseResidency(const ITargetPlatform* InPlatform, uint32 InReferencerHash);
	
	// Synchronous caching of compressed animation data for provided target platform
 // 为提供的目标平台同步缓存压缩动画数据
	UE_DEPRECATED(5.6, "CacheDerivedData returning FCompressedAnimSequence reference has been deprecated, use CacheDerivedDataForPlatform")
	ENGINE_API FCompressedAnimSequence& CacheDerivedData(const ITargetPlatform* TargetPlatform);

	ENGINE_API FFrameRate GetTargetSamplingFrameRate(const ITargetPlatform* InPlatform) const;
#endif

public:
	// Provides access to an instance of compressed animation data within the lifetime of FScopedCompressedAnimSequence, should _only_ be used on the stack due to risk of deadlocking
 // 提供对 FScopedCompressedAnimSequence 生命周期内压缩动画数据实例的访问，由于存在死锁风险，应该仅在堆栈上使用
	struct FScopedCompressedAnimSequence
	{
		FScopedCompressedAnimSequence() = delete;
		FScopedCompressedAnimSequence(const FScopedCompressedAnimSequence&) = delete;
		FScopedCompressedAnimSequence& operator=(const FScopedCompressedAnimSequence&) = delete;		
		FScopedCompressedAnimSequence(FScopedCompressedAnimSequence&&) = delete;
		FScopedCompressedAnimSequence& operator=(FScopedCompressedAnimSequence&&) = delete;
		
		const FCompressedAnimSequence& operator->() const { return *CompressedData; }		
		const FCompressedAnimSequence& Get() const { return *CompressedData; }
#if WITH_EDITOR
		~FScopedCompressedAnimSequence()
		{
			SharedLock.Reset();
		}
#endif // WITH_EDITOR
		
	protected:
		FScopedCompressedAnimSequence(const UAnimSequence* InAnimSequence, const FCompressedAnimSequence* InCompressedData) :
			CompressedData(InCompressedData)
		{
			check(CompressedData && InAnimSequence);			
#if WITH_EDITOR
			SharedLock = MakeUnique<UE::TSharedLock<UE::FSharedRecursiveMutex>>(InAnimSequence->SharedCompressedDataMutex);
#endif // WITH_EDITOR
		}

		const FCompressedAnimSequence* CompressedData = nullptr;
#if WITH_EDITOR
		TUniquePtr<UE::TSharedLock<UE::FSharedRecursiveMutex>> SharedLock;
#endif // WITH_EDITOR
		friend class UAnimSequence;
	};
	
	// Returns scoped version of the compressed AnimSequence, only valid to read when FScopedConstCompressedAnimSequence is in scope (this internally locks CompressedDataLock)
 // 返回压缩 AnimSequence 的作用域版本，仅当 FScopedConstCompressedAnimSequence 位于作用域内时才有效（这会在内部锁定 CompressedDataLock）
	ENGINE_API FScopedCompressedAnimSequence GetCompressedData() const;	
	ENGINE_API FScopedCompressedAnimSequence GetCompressedData(const FAnimExtractContext& AnimExtractContext) const;
	
#if WITH_EDITOR	
	ENGINE_API FScopedCompressedAnimSequence GetCompressedData(const ITargetPlatform* InTargetPlatform) const;	
protected:
	/** Helper struct to enter/leave compressed data lock in read-mode */
	/** 在读取模式下进入/离开压缩数据锁的帮助程序结构 */
	struct FCompressedAnimationDataReadScope
	{
		FCompressedAnimationDataReadScope(const UAnimSequence* InAnimSequence)
		{
			check(InAnimSequence);
			SharedLock = MakeUnique<UE::TSharedLock<UE::FSharedRecursiveMutex>>(InAnimSequence->SharedCompressedDataMutex);
		}
		TUniquePtr<UE::TSharedLock<UE::FSharedRecursiveMutex>> SharedLock;
	};
	
	/** Helper struct to enter/leave compressed data lock in write-mode */
	/** 在写入模式下进入/离开压缩数据锁的帮助程序结构 */
	struct FCompressedAnimationDataWriteScope
	{
		FCompressedAnimationDataWriteScope(const UAnimSequence* InAnimSequence) : AnimSequence(InAnimSequence)
		{
			check(InAnimSequence);
			InAnimSequence->SharedCompressedDataMutex.Lock();
		}
		~FCompressedAnimationDataWriteScope()
		{
			AnimSequence->SharedCompressedDataMutex.Unlock();
		}

		const UAnimSequence* AnimSequence = nullptr;
	};
#endif // WITH_EDITOR

public:
	friend class UAnimationAsset;
	friend struct FScopedAnimSequenceRawDataCache;
	friend class UAnimationBlueprintLibrary;
	friend class UAnimBoneCompressionSettings;
	friend class FCustomAttributeCustomization;
	friend class FAnimSequenceDeterminismHelper;
	friend class FAnimSequenceTestBase;
	friend struct UE::Anim::Compression::FScopedCompressionGuard;
	friend class FAnimDataControllerTestBase;
	friend class UE::Anim::FAnimSequenceCompilingManager;
	friend UE::UAF::FDecompressionTools;
	friend class FAnimSequenceDetails;
	friend class FCompressedAnimationDataNodeBuilder;
	friend class UAnimBank;
	friend class FAnimBankBuildAsyncCacheTask;
	friend struct FAnimCurveBufferAccess;
	friend class UE::Anim::FAnimationSequenceAsyncCacheTask;
};

#ifndef UE_COMPRESSED_DATA_WRITE_SCOPE
#if WITH_EDITOR
	#define UE_COMPRESSED_DATA_WRITE_SCOPE(sequence) \
	UAnimSequence::FCompressedAnimationDataWriteScope Scope(sequence);
#else
	#define UE_COMPRESSED_DATA_WRITE_SCOPE(sequence)
#endif
#endif

#ifndef UE_COMPRESSED_DATA_READ_SCOPE
#if WITH_EDITOR
	#define UE_COMPRESSED_DATA_READ_SCOPE(sequence) \
	UAnimSequence::FCompressedAnimationDataReadScope Scope(sequence);
#else
	#define UE_COMPRESSED_DATA_READ_SCOPE(sequence)
#endif
#endif
