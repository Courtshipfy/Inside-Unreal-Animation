// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Animation that can be streamed instead of being loaded completely
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "Serialization/BulkData.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimCompress.h"
#include "Animation/AnimCompressionTypes.h"
#include "Animation/AnimSequenceBase.h"
#include "AnimStreamable.generated.h"

class UAnimSequence;
class UAnimCompress;
struct FCompactPose;

class FAnimStreamableChunk
{
public:
	FAnimStreamableChunk() : StartTime(0.f), SequenceLength(0.f), CompressedAnimSequence(nullptr) {}
	~FAnimStreamableChunk()
	{
		if (CompressedAnimSequence)
		{
			delete CompressedAnimSequence;
			CompressedAnimSequence = nullptr;
		}
	}

	float StartTime;

	float SequenceLength;
	int32 NumFrames;

	// Compressed Data for this chunk (if nullptr then data needs to be loaded via BulkData)
	// 该块的压缩数据（如果为 nullptr，则需要通过 BulkData 加载数据）
	FCompressedAnimSequence* CompressedAnimSequence;

	// Bulk data if stored in the package.
	// 批量数据（如果存储在包中）。
	FByteBulkData BulkData;

	SIZE_T GetMemorySize() const
	{
		static const SIZE_T ClassSize = sizeof(FAnimStreamableChunk);
		SIZE_T CurrentSize = ClassSize;

		if (CompressedAnimSequence)
		{
			CurrentSize += CompressedAnimSequence->GetMemorySize();
		}
		return CurrentSize;
	}

	/** Serialization. */
	/** 序列化。 */
	void Serialize(FArchive& Ar, UAnimStreamable* Owner, int32 ChunkIndex);
};

class FStreamableAnimPlatformData
{
public:
	TArray<FAnimStreamableChunk> Chunks;

	void Serialize(FArchive& Ar, class UAnimStreamable* Owner);

	void Reset()
	{
		Chunks.Reset();
	}

	SIZE_T GetMemorySize() const
	{
		SIZE_T ChunkSize = 0;
		for (const FAnimStreamableChunk& Chunk : Chunks)
		{
			ChunkSize += Chunk.GetMemorySize();
		}
		return sizeof(FStreamableAnimPlatformData) + ChunkSize;
	}
};


UCLASS(config=Engine, hidecategories=UObject, MinimalAPI, BlueprintType)
class UAnimStreamable : public UAnimSequenceBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The number of keys expected within the individual animation tracks. */
	/** 各个动画轨道中预期的关键点数量。 */
	UPROPERTY(AssetRegistrySearchable)
	int32 NumberOfKeys;

	/** This defines how values between keys are calculated **/
	/** 这定义了如何计算键之间的值 **/
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = Animation)
	EAnimInterpolationType Interpolation;

	/** Base pose to use when retargeting */
	/** 重定位时使用的基本姿势 */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = Animation)
	FName RetargetSource;

	UPROPERTY()
	FFrameRate SamplingFrameRate;

#if WITH_EDITORONLY_DATA

	// Sequence the streamable was created from (used for reflecting changes to the source in editor)
	// 创建流的序列（用于反映编辑器中源的更改）
	UPROPERTY()
	TObjectPtr<const UAnimSequence> SourceSequence;

	UPROPERTY()
	FGuid RawDataGuid;

	/** Number of raw frames in this sequence (not used by engine - just for informational purposes). */
	/** 此序列中的原始帧数（引擎不使用 - 仅用于提供信息）。 */
	UE_DEPRECATED(5.0, "Num Frames is deprecated use NumberOfKeys instead")
	UPROPERTY()
	int32 NumFrames;

	/**
	 * Raw uncompressed keyframe data.
	 */
	UPROPERTY()
	TArray<struct FRawAnimSequenceTrack> RawAnimationData;

	/**
	 * In the future, maybe keeping RawAnimSequenceTrack + TrackMap as one would be good idea to avoid inconsistent array size
	 * TrackToSkeletonMapTable(i) should contains  track mapping data for RawAnimationData(i).
	 */
	UPROPERTY()
	TArray<struct FTrackToSkeletonMap> TrackToSkeletonMapTable;

	/**
	 * This is name of RawAnimationData tracks for editoronly - if we lose skeleton, we'll need relink them
	 */
	UPROPERTY()
	TArray<FName> AnimationTrackNames;

	// Editor can have multiple platforms loaded at once
	// 编辑器可以同时加载多个平台
	TMap<const ITargetPlatform*, FStreamableAnimPlatformData*> StreamableAnimPlatformData;

	FStreamableAnimPlatformData* RunningAnimPlatformData;
#else

	// Non editor only has one set of platform data
	// 非编辑者只有一套平台数据
	FStreamableAnimPlatformData RunningAnimPlatformData;
#endif

	bool HasRunningPlatformData() const
	{
#if WITH_EDITOR
		return RunningAnimPlatformData != nullptr;
#else
		return true;
#endif
	}

	FStreamableAnimPlatformData& GetRunningPlatformData()
	{
#if WITH_EDITOR
		check(RunningAnimPlatformData);
		return *RunningAnimPlatformData;
#else
		return RunningAnimPlatformData;
#endif
	}

	const FStreamableAnimPlatformData& GetRunningPlatformData() const
	{
#if WITH_EDITOR
		check(RunningAnimPlatformData);
		return *RunningAnimPlatformData;
#else
		return RunningAnimPlatformData;
#endif
	}

	/** The bone compression settings used to compress bones in this sequence. */
	/** 用于按此顺序压缩骨骼的骨骼压缩设置。 */
	UPROPERTY(Category = Compression, EditAnywhere)
	TObjectPtr<class UAnimBoneCompressionSettings> BoneCompressionSettings;

	/** The curve compression settings used to compress curves in this sequence. */
	/** 用于压缩此序列中的曲线的曲线压缩设置。 */
	UPROPERTY(Category = Compression, EditAnywhere)
	TObjectPtr<class UAnimCurveCompressionSettings> CurveCompressionSettings;

	/** The settings used to control whether or not to use variable frame stripping and its amount*/
	/** 用于控制是否使用可变帧剥离及其数量的设置*/
	UPROPERTY(Category = Compression, EditAnywhere)
	TObjectPtr<class UVariableFrameStrippingSettings> VariableFrameStrippingSettings;

	/** If this is on, it will allow extracting of root motion **/
	/** 如果打开，它将允许提取根运动 **/
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = RootMotion, meta = (DisplayName = "EnableRootMotion"))
	bool bEnableRootMotion;

	/** Root Bone will be locked to that position when extracting root motion.**/
	/** 提取根运动时，根骨骼将被锁定到该位置。**/
	UPROPERTY(EditAnywhere, Category = RootMotion)
	TEnumAsByte<ERootMotionRootLock::Type> RootMotionRootLock;

	/** Force Root Bone Lock even if Root Motion is not enabled */
	/** 即使未启用根运动，也强制根骨锁定 */
	UPROPERTY(EditAnywhere, Category = RootMotion)
	bool bForceRootLock;

	/** If this is on, it will use a normalized scale value for the root motion extracted: FVector(1.0, 1.0, 1.0) **/
	/** 如果启用此选项，它将使用标准化比例值来提取提取的根运动：FVector(1.0, 1.0, 1.0) **/
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = RootMotion, meta = (DisplayName = "Use Normalized Root Motion Scale"))
	bool bUseNormalizedRootMotionScale;

	//~ Begin UObject Interface
	//~ 开始 UObject 接口
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void FinishDestroy() override;
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~ End UObject Interface
	//~ 结束 UObject 接口

	//~ Begin UAnimSequenceBase Interface
	//~ 开始 UAnimSequenceBase 接口
	ENGINE_API virtual void HandleAssetPlayerTickedInternal(FAnimAssetTickContext &Context, const float PreviousTime, const float MoveDelta, const FAnimTickRecord &Instance, struct FAnimNotifyQueue& NotifyQueue) const override;
	virtual void GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const override;
	virtual int32 GetNumberOfSampledKeys() const override { return NumberOfKeys; }
	//~ End UAnimSequenceBase Interface
	//~ 结束 UAnimSequenceBase 接口

#if WITH_EDITOR
	ENGINE_API void InitFrom(const UAnimSequence* InSourceSequence);
#endif

	ENGINE_API FStreamableAnimPlatformData& GetStreamingAnimPlatformData(const ITargetPlatform* Platform);

	ENGINE_API float GetChunkSizeSeconds(const ITargetPlatform* Platform) const;

	private:

#if WITH_EDITOR
	ENGINE_API void RequestCompressedData(const  ITargetPlatform* Platform=nullptr);

	void UpdateRawData();

	FString GetBaseDDCKey(uint32 NumChunks, const ITargetPlatform* TargetPlatform) const;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	void RequestCompressedDataForChunk(const FString& ChunkDDCKey, FAnimStreamableChunk& Chunk, const int32 ChunkIndex, const uint32 FrameStart, const uint32 FrameEnd, TSharedRef<FAnimCompressContext> CompressContext, const ITargetPlatform* TargetPlatform);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif

	bool bUseRawDataOnly;
	int32 GetChunkIndexForTime(const TArray<FAnimStreamableChunk>& Chunks, const float CurrentTime) const;
};

