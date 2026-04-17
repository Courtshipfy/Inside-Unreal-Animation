// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Abstract base class of animation assets that can be played back and evaluated to produce a pose.
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "Templates/SubclassOf.h"
#include "Interfaces/Interface_AssetUserData.h"
#include "AnimInterpFilter.h"
#include "AnimEnums.h"
#include "Interfaces/Interface_PreviewMeshProvider.h"
#if WITH_EDITOR
#include "IO/IoHash.h"
#endif //  WITH_EDITOR
#include "AnimationAsset.generated.h"

class UAnimMetaData;
class UAnimMontage;
class UAssetMappingTable;
class UAssetUserData;
class USkeleton;
class UAnimSequenceBase;
class UBlendSpace;
class UPoseAsset;
class UMirrorDataTable;
class USkeletalMesh;
class ITargetPlatform;
struct FAnimationUpdateContext;
namespace SmartName
{
typedef uint16 UID_Type;
}

namespace UE { namespace Anim {
class IAnimNotifyEventContextDataInterface;
}}

namespace MarkerIndexSpecialValues
{
	enum Type
	{
		Uninitialized = -2,
		AnimationBoundary = -1,
	};
};

struct FMarkerPair
{
	int32 MarkerIndex;
	float TimeToMarker;

	FMarkerPair() : MarkerIndex(MarkerIndexSpecialValues::Uninitialized) {}
	FMarkerPair(int32 InMarkerIndex, float InTimeToMarker) : MarkerIndex(InMarkerIndex), TimeToMarker(InTimeToMarker) {}

	void Reset() { MarkerIndex = MarkerIndexSpecialValues::Uninitialized; }
};

struct FMarkerTickRecord
{
	//Current Position in marker space, equivalent to TimeAccumulator
 // 标记空间中的当前位置，相当于 TimeAccumulator
	FMarkerPair PreviousMarker;
	FMarkerPair NextMarker;

	bool IsValid(bool bLooping) const
	{
		int32 Threshold = bLooping ? MarkerIndexSpecialValues::AnimationBoundary : MarkerIndexSpecialValues::Uninitialized;
		return PreviousMarker.MarkerIndex > Threshold && NextMarker.MarkerIndex > Threshold;
	}

	void Reset() { PreviousMarker.Reset(); NextMarker.Reset(); }

	/** 调试输出功能*/
	/** 调试输出功能*/
	/** Debug output function*/
	/** 调试输出功能*/
	FString ToString() const
	{
		return FString::Printf(TEXT("[PreviousMarker Index/Time %i/%.2f, NextMarker Index/Time %i/%.2f]"), PreviousMarker.MarkerIndex, PreviousMarker.TimeToMarker, NextMarker.MarkerIndex, NextMarker.TimeToMarker);
	}
};

/**
 * Used when sampling a given animation asset, this structure will contain the previous frame's
 * internal sample time alongside the 'effective' delta time leading into the current frame.
 * 
 * An 'effective' delta time represents a value that has undergone all side effects present in the
 * corresponding asset's TickAssetPlayer call including but not limited to syncing, play rate 
 * adjustment, looping, etc.
 * 
 * For montages Delta isn't always abs(CurrentPosition-PreviousPosition) because a Montage can jump or repeat or loop
 */
struct FDeltaTimeRecord
{
public:
	void Set(float InPrevious, float InDelta)
	{
		Previous = InPrevious;
		Delta = InDelta;
		bPreviousIsValid = true;
	}
	void SetPrevious(float InPrevious) { Previous = InPrevious; bPreviousIsValid = true; }
	float GetPrevious() const { return Previous; }
	bool IsPreviousValid() const { return bPreviousIsValid; }

	float Delta = 0.f;
	
	FDeltaTimeRecord() = default;
	explicit FDeltaTimeRecord(float InDeltaTime) : Delta(InDeltaTime), Previous(0.f) {}
private:
	float Previous = 0.f;
	bool  bPreviousIsValid = false; // Will be set to true when Previous has been set
};
/** 变换定义 */
/** 变换定义 */

/** Transform definition */
/** 变换定义 */
USTRUCT(BlueprintType)
struct FBlendSampleData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 SampleDataIndex;

	UPROPERTY()
	TObjectPtr<class UAnimSequence> Animation;

	UPROPERTY()
	float TotalWeight;

	// Rate of change of the Weight - used in smoothed BlendSpace blends
 // 权重变化率 - 用于平滑的 BlendSpace 混合
	UPROPERTY()
	float WeightRate;

	UPROPERTY()
	float Time;

	UPROPERTY()
	float PreviousTime;

	// We may merge multiple samples if they use the same animation
 // 如果多个样本使用相同的动画，我们可能会合并它们
	// Calculate the combined sample play rate here
 // 在此计算组合样本播放率
	UPROPERTY()
	float SamplePlayRate;

	FDeltaTimeRecord DeltaTimeRecord;

	FMarkerTickRecord MarkerTickRecord;

	// transient per-bone interpolation data
 // 瞬态每骨插值数据
	TArray<float> PerBoneBlendData;

	// transient per-bone weight rate - only allocated when used
 // 瞬时每骨重量率 - 仅在使用时分配
	TArray<float> PerBoneWeightRate;

	FBlendSampleData()
		: SampleDataIndex(0)
		, Animation(nullptr)
		, TotalWeight(0.f)
		, WeightRate(0.f)
		, Time(0.f)
		, PreviousTime(0.f)
		, SamplePlayRate(0.0f)
		, DeltaTimeRecord()
	{
	}

	FBlendSampleData(int32 Index)
		: SampleDataIndex(Index)
		, Animation(nullptr)
		, TotalWeight(0.f)
		, WeightRate(0.f)
		, Time(0.f)
		, PreviousTime(0.f)
		, SamplePlayRate(0.0f)
		, DeltaTimeRecord()
	{
	}

	bool operator==( const FBlendSampleData& Other ) const 
	{
		// if same position, it's same point
  // 如果位置相同，则为同一点
		return (Other.SampleDataIndex== SampleDataIndex);
	}
	void AddWeight(float Weight)
	{
		TotalWeight += Weight;
	}

	UE_DEPRECATED(5.0, "GetWeight() was renamed to GetClampedWeight()")
	float GetWeight() const
	{
		return GetClampedWeight();
	}

	float GetClampedWeight() const
	{
		return FMath::Clamp<float>(TotalWeight, 0.f, 1.f);
	}

	static void ENGINE_API NormalizeDataWeight(TArray<FBlendSampleData>& SampleDataList);
};

USTRUCT()
struct FBlendFilter
{
	GENERATED_USTRUCT_BODY()

	TArray<FFIRFilterTimeBased> FilterPerAxis;

	FBlendFilter()
	{
	}
	
	FVector GetFilterLastOutput() const
	{
		return FVector(
			FilterPerAxis.Num() > 0 ? FilterPerAxis[0].LastOutput : 0.0f,
			FilterPerAxis.Num() > 1 ? FilterPerAxis[1].LastOutput : 0.0f,
			FilterPerAxis.Num() > 2 ? FilterPerAxis[2].LastOutput : 0.0f);
	}
};

/*
 * Pose Curve container for extraction
 * This is used by pose anim node
 * Saves Name/PoseIndex/Value of the curve
 */
struct FPoseCurve
{
	UE_DEPRECATED(5.3, "UID is no longer used.")
	static SmartName::UID_Type	UID;

	// The name of the curve
 // 曲线名称
	FName				Name;
	// PoseIndex of pose asset it's dealing with
 // 正在处理的姿势资产的 PoseIndex
	// used to extract pose value fast
 // 用于快速提取姿态值
	int32				PoseIndex;
	// Curve Value
 // 曲线值
	float				Value;

	FPoseCurve()
		: Name(NAME_None)
		, PoseIndex(INDEX_NONE)
		, Value(0.f)
	{}

	FPoseCurve(int32 InPoseIndex, FName InName, float InValue)
		: Name(InName)
/** 动画提取上下文 */
		, PoseIndex(InPoseIndex)
		, Value(InValue)
	{}
	/** 动画中的位置以从中提取姿势 */

	UE_DEPRECATED(5.3, "Please use the constructor that takes an FName.")
	FPoseCurve(int32 InPoseIndex, SmartName::UID_Type	InUID, float  InValue )
	/** 是否提取根运动？ */
/** 动画提取上下文 */
		: Name(NAME_None)
		, PoseIndex(InPoseIndex)
		, Value(InValue)
	/** 动画中的位置以从中提取姿势 */
	/** 根运动提取所需的增量时间范围 **/
	{}
};
	/** 是否提取根运动？ */

	/** 当前动画资源是否标记为循环？ **/
/** Animation Extraction Context */
	/** 根运动提取所需的增量时间范围 **/
/** [翻译失败: Animation Extraction Context] */
struct FAnimExtractContext
{
	/** 当前动画资源是否标记为循环？ **/
	/** Position in animation to extract pose from */
	/** 动画中的位置以从中提取姿势 */
	double CurrentTime;

	/** Is root motion being extracted? */
	/** 是否提取根运动？ */
	bool bExtractRootMotion;

	/** Delta time range required for root motion extraction **/
	/** 根运动提取所需的增量时间范围 **/
	FDeltaTimeRecord DeltaTimeRecord;

	/** Is the current animation asset marked as looping? **/
	/** 当前动画资源是否标记为循环？ **/
	bool bLooping;

	/** 
	 * Pose Curve Values to extract pose from pose assets. 
	 * This is used by pose asset extraction 
	 */
	TArray<FPoseCurve> PoseCurves;

	/**
	 * The BonesRequired array is a list of bool flags to determine
	 * if a bone is required to be retrieved. This is currently used
	 * by several animation nodes to optimize evaluation time.
	 */
	TArray<bool> BonesRequired;

	/** 
	 * The optional interpolation mode override.
	 * If not set, it will simply use the interpolation mode provided by the asset.
	 * One example where this could be used is if you want to force sampling the animation with Step interpolation
	 * even when the animation sequence asset is set to Linear interpolation.
	 */
	TOptional<EAnimInterpolationType> InterpolationOverride;

#if WITH_EDITOR
	bool bIgnoreRootLock;

	// Experimental
 // 实验性的
	bool bExtractWithRootMotionProvider;

	// Flag when set will enforce evaluation/sampling of, specific, compressed animation data, will assert if it is missing (user is responsible for ensuring the data was previously requested/resident)
 // 设置时的标志将强制对特定的压缩动画数据进行评估/采样，如果丢失则将断言（用户负责确保数据先前已请求/驻留）
	bool bEnforceCompressedDataSampling = false;
	// Used in combination with bForceSampleCompressedData, forcing a specific target platform hash with corresponding compressed data to sample (retrieved using UAnimSequence::GetDerivedDataKeyHash) 
 // 与 bForceSampleCompressedData 结合使用，强制使用特定的目标平台哈希和相应的压缩数据进行采样（使用 UAnimSequence::GetDerivedDataKeyHash 检索）
	FIoHash TargetPlatformHash = FIoHash::Zero;
	// Used in combination with bForceSampleCompressedData, forcing a specific target platform its compressed data to sample
 // 与bForceSampleCompressedData结合使用，强制特定目标平台对其压缩数据进行采样
	const ITargetPlatform* TargetPlatform = nullptr;
#endif 
	
	UE_DEPRECATED(5.1, "FAnimExtractContext construct with float-based time value is deprecated, use other signature")
	FAnimExtractContext(float InCurrentTime, bool InbExtractRootMotion = false, FDeltaTimeRecord InDeltaTimeRecord = {}, bool InbLooping = false)
		: CurrentTime((double)InCurrentTime)
		, bExtractRootMotion(InbExtractRootMotion)
		, DeltaTimeRecord(InDeltaTimeRecord)
		, bLooping(InbLooping)
		, PoseCurves()
		, BonesRequired()
		, InterpolationOverride()
#if WITH_EDITOR
		, bIgnoreRootLock(false)
		, bExtractWithRootMotionProvider(true)
#endif 
	{
	}

	FAnimExtractContext(double InCurrentTime = 0.0, bool InbExtractRootMotion = false, FDeltaTimeRecord InDeltaTimeRecord = {}, bool InbLooping = false)
		: CurrentTime(InCurrentTime)
		, bExtractRootMotion(InbExtractRootMotion)
		, DeltaTimeRecord(InDeltaTimeRecord)
		, bLooping(InbLooping)
		, PoseCurves()
		, BonesRequired()
		, InterpolationOverride()
#if WITH_EDITOR
		, bIgnoreRootLock(false)
		, bExtractWithRootMotionProvider(true)
#endif 
	/** 我们经过的标记*/
	{
	}

	bool IsBoneRequired(int32 BoneIndex) const
	/** 我们正前往的标记 */
	{
		if (BoneIndex >= BonesRequired.Num())
	/** 我们经过的标记*/
		{
			return true;
		}

	/** 我们正前往的标记 */
		return BonesRequired[BoneIndex];
	}
};

//Represent a current play position in an animation
// 表示动画中的当前播放位置
	/** 这是有效的标记同步位置吗 */
//based on sync markers
// 基于同步标记
USTRUCT(BlueprintType)
struct FMarkerSyncAnimPosition
{
	GENERATED_USTRUCT_BODY()
	/** 这是有效的标记同步位置吗 */

	/** The marker we have passed*/
	/** 我们经过的标记*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sync)
	FName PreviousMarkerName;

	/** 调试输出功能*/
	/** The marker we are heading towards */
	/** 我们正前往的标记 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sync)
	FName NextMarkerName;

	/** Value between 0 and 1 representing where we are:
	/** 调试输出功能*/
	0   we are at PreviousMarker
	1   we are at NextMarker
	0.5 we are half way between the two */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sync)
	float PositionBetweenMarkers;

	/** Is this a valid Marker Sync Position */
	/** 这是有效的标记同步位置吗 */
	bool IsValid() const { return (PreviousMarkerName != NAME_None && NextMarkerName != NAME_None); }

	FMarkerSyncAnimPosition()
		: PositionBetweenMarkers(0.0f)
	{}

	FMarkerSyncAnimPosition(const FName& InPrevMarkerName, const FName& InNextMarkerName, const float& InAlpha)
		: PreviousMarkerName(InPrevMarkerName)
		, NextMarkerName(InNextMarkerName)
		, PositionBetweenMarkers(InAlpha)
	{}

	/** Debug output function*/
	/** 调试输出功能*/
	FString ToString() const
	{
		return FString::Printf(TEXT("[PreviousMarker %s, NextMarker %s] : %0.2f "), *PreviousMarkerName.ToString(), *NextMarkerName.ToString(), PositionBetweenMarkers);
	}
};

struct FPassedMarker
{
	FName PassedMarkerName;

	float DeltaTimeWhenPassed;
};

/**
* Information about an animation asset that needs to be ticked
*/
USTRUCT()
struct FAnimTickRecord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TObjectPtr<class UAnimationAsset> SourceAsset = nullptr;

	float* TimeAccumulator = nullptr;
	float PlayRateMultiplier = 1.0f;
	float EffectiveBlendWeight = 0.0f;
	float RootMotionWeightModifier = 1.0f;

	bool bLooping = false;
	bool bIsEvaluator = false;
	bool bRequestedInertialization = false;
	bool bOverridePositionWhenJoiningSyncGroupAsLeader = false;
	bool bIsExclusiveLeader = false;
	bool bActiveContext = true;

	const UMirrorDataTable* MirrorDataTable = nullptr;

	TSharedPtr<TArray<TUniquePtr<const UE::Anim::IAnimNotifyEventContextDataInterface>>> ContextData;

	union
	{
		struct
		{
			FBlendFilter* BlendFilter;
			TArray<FBlendSampleData>* BlendSampleDataCache;
			int32  TriangulationIndex;
			float  BlendSpacePositionX;
			float  BlendSpacePositionY;
			bool   bTeleportToTime;
		} BlendSpace;

		struct
		{
			float CurrentPosition;  // montage doesn't use accumulator, but this
			TArray<FPassedMarker>* MarkersPassedThisTick;
		} Montage;
	};

	// Asset players (and other nodes) have ownership of their respective DeltaTimeRecord value/state,
 // 资产玩家（和其他节点）拥有各自 DeltaTimeRecord 值/状态的所有权，
	// while an asset's tick update will forward the time-line through the tick record
 // 而资产的tick更新将通过tick记录转发时间线
	FDeltaTimeRecord* DeltaTimeRecord = nullptr;

	// marker sync related data
 // 标记同步相关数据
	FMarkerTickRecord* MarkerTickRecord = nullptr;
	bool bCanUseMarkerSync = false;
	float LeaderScore = 0.0f;

	// Return the root motion weight for this tick record
 // 返回此刻度记录的根运动权重
	float GetRootMotionWeight() const { return EffectiveBlendWeight * RootMotionWeightModifier; }

private:
	void AllocateContextDataContainer();

public:
	FAnimTickRecord() = default;

	// Create a tick record for an anim sequence
 // 为动画序列创建刻度记录
	UE_DEPRECATED(5.2, "Please use the anim sequence FAnimTickRecord constructor which adds bInIsEvaluator (defaulted to false)")
	ENGINE_API FAnimTickRecord(UAnimSequenceBase* InSequence, bool bInLooping, float InPlayRate, float InFinalBlendWeight, float& InCurrentTime, FMarkerTickRecord& InMarkerTickRecord);

	// Create a tick record for an anim sequence
 // 为动画序列创建刻度记录
	/** 这可以与 FAnimTickRecord 的 TArray 上的 Sort() 函数一起使用，以从较高的领导者分数进行排序 */
	ENGINE_API FAnimTickRecord(UAnimSequenceBase* InSequence, bool bInLooping, float InPlayRate, bool bInIsEvaluator, float InFinalBlendWeight, float& InCurrentTime, FMarkerTickRecord& InMarkerTickRecord);

	// Create a tick record for a blendspace
 // 为混合空间创建刻度记录
	ENGINE_API FAnimTickRecord(
		UBlendSpace* InBlendSpace, const FVector& InBlendInput, TArray<FBlendSampleData>& InBlendSampleDataCache, FBlendFilter& InBlendFilter, bool bInLooping, 
		float InPlayRate, bool bShouldTeleportToTime, bool bInIsEvaluator, float InFinalBlendWeight, float& InCurrentTime, FMarkerTickRecord& InMarkerTickRecord);

	// Create a tick record for a montage
 // 为蒙太奇创建刻度记录
	UE_DEPRECATED(5.0, "Please use the montage FAnimTickRecord constructor which removes InPreviousPosition and InMoveDelta")
	ENGINE_API FAnimTickRecord(UAnimMontage* InMontage, float InCurrentPosition, float InPreviousPosition, float InMoveDelta, float InWeight, TArray<FPassedMarker>& InMarkersPassedThisTick, FMarkerTickRecord& InMarkerTickRecord);

	/** 这可以与 FAnimTickRecord 的 TArray 上的 Sort() 函数一起使用，以从较高的领导者分数进行排序 */
	// Create a tick record for a montage
 // 为蒙太奇创建刻度记录
	ENGINE_API FAnimTickRecord(UAnimMontage* InMontage, float InCurrentPosition, float InWeight, TArray<FPassedMarker>& InMarkersPassedThisTick, FMarkerTickRecord& InMarkerTickRecord);

	// Create a tick record for a pose asset
 // 为姿势资产创建刻度记录
	ENGINE_API FAnimTickRecord(UPoseAsset* InPoseAsset, float InFinalBlendWeight);

	// Gather any data from the current update context
 // 从当前更新上下文收集任何数据
	ENGINE_API void GatherContextData(const FAnimationUpdateContext& InContext);
	
	// Explicitly add typed context data to the tick record
 // 将类型化上下文数据显式添加到刻度记录中
	template<typename Type, typename... TArgs>
	void MakeContextData(TArgs&&... Args)
	{
		static_assert(TPointerIsConvertibleFromTo<Type, const UE::Anim::IAnimNotifyEventContextDataInterface>::Value, "'Type' template parameter to MakeContextData must be derived from IAnimNotifyEventContextDataInterface");
		if (!ContextData.IsValid())
		{
			AllocateContextDataContainer();
		}

		ContextData->Add(MakeUnique<Type>(Forward<TArgs>(Args)...));
	}

	/** This can be used with the Sort() function on a TArray of FAnimTickRecord to sort from higher leader score */
	/** 这可以与 FAnimTickRecord 的 TArray 上的 Sort() 函数一起使用，以从较高的领导者分数进行排序 */
	bool operator <(const FAnimTickRecord& Other) const { return LeaderScore > Other.LeaderScore; }
};

class FMarkerTickContext
{
public:

	static const TArray<FName> DefaultMarkerNames;

	FMarkerTickContext(const TArray<FName>& ValidMarkerNames) 
		: ValidMarkers(&ValidMarkerNames) 
	{}

	FMarkerTickContext() 
		: ValidMarkers(&DefaultMarkerNames) 
	/** 调试输出功能 */
	{}

	void SetMarkerSyncStartPosition(const FMarkerSyncAnimPosition& SyncPosition)
	{
		MarkerSyncStartPostion = SyncPosition;
	}

	void SetMarkerSyncEndPosition(const FMarkerSyncAnimPosition& SyncPosition)
	{
		MarkerSyncEndPostion = SyncPosition;
	}

	const FMarkerSyncAnimPosition& GetMarkerSyncStartPosition() const
	{
		return MarkerSyncStartPostion;
	}
	/** 调试输出功能 */

	const FMarkerSyncAnimPosition& GetMarkerSyncEndPosition() const
	{
		return MarkerSyncEndPostion;
	}

	const TArray<FName>& GetValidMarkerNames() const
	{
		return *ValidMarkers;
	}

	bool IsMarkerSyncStartValid() const
	{
		return MarkerSyncStartPostion.IsValid();
	}
		/** 该节点可以成为领导者，只要它具有比先前最佳领导者更高的混合权重。 */

	bool IsMarkerSyncEndValid() const
	{
		/** 该节点将始终是追随者（除非只有追随者，在这种情况下，第一个勾选的节点获胜）。 */
		// does it have proper end position
  // 它有正确的结束位置吗
		return MarkerSyncEndPostion.IsValid();
		/** 该节点将始终是领导者（如果多个节点是 AlwaysLeader，则最后一个勾选的节点获胜）。 */
	}

	TArray<FPassedMarker> MarkersPassedThisTick;
		/** 混合时，该节点将被排除在同步组之外。混合后，它将成为同步组领导者，直到混合为止*/

	/** Debug output function */
	/** 调试输出功能 */
		/** 该节点在混合时将被排除在同步组之外。一旦混合，它将成为跟随者，直到混合为止*/
		/** 该节点可以成为领导者，只要它具有比先前最佳领导者更高的混合权重。 */
	FString  ToString() const
	{
		FString MarkerString;
		/** 该节点将始终是追随者（除非只有追随者，在这种情况下，第一个勾选的节点获胜）。 */
		/** 该节点将永远是领导者。如果它未能被标记为领导者，它将作为未分组的资源播放器运行 (EAnimSyncMethod::DoNotSync) 。*/

		for (const auto& ValidMarker : *ValidMarkers)
		/** 该节点将始终是领导者（如果多个节点是 AlwaysLeader，则最后一个勾选的节点获胜）。 */
		{
			MarkerString.Append(FString::Printf(TEXT("%s,"), *ValidMarker.ToString()));
		}
		/** 混合时，该节点将被排除在同步组之外。混合后，它将成为同步组领导者，直到混合为止*/

		return FString::Printf(TEXT(" - Sync Start Position : %s\n - Sync End Position : %s\n - Markers : %s"),
			*MarkerSyncStartPostion.ToString(), *MarkerSyncEndPostion.ToString(), *MarkerString);
		/** 该节点在混合时将被排除在同步组之外。一旦混合，它将成为跟随者，直到混合为止*/
	}
private:
	// Structure representing our sync position based on markers before tick
 // 基于刻度之前的标记表示我们的同步位置的结构
		/** 该节点将永远是领导者。如果它未能被标记为领导者，它将作为未分组的资源播放器运行 (EAnimSyncMethod::DoNotSync) 。*/
	// This is used to allow new animations to play from the right marker position
 // 这用于允许从正确的标记位置播放新动画
	FMarkerSyncAnimPosition MarkerSyncStartPostion;

	// Structure representing our sync position based on markers after tick
 // 基于刻度后的标记表示我们的同步位置的结构
	FMarkerSyncAnimPosition MarkerSyncEndPostion;

	// Valid marker names for this sync group
 // 该同步组的有效标记名称
	const TArray<FName>* ValidMarkers;
};


UENUM()
namespace EAnimGroupRole
{
	enum Type : int
	{
		/** This node can be the leader, as long as it has a higher blend weight than the previous best leader. */
		/** [翻译失败: This node can be the leader, as long as it has a higher blend weight than the previous best leader.] */
		CanBeLeader,
		
		/** This node will always be a follower (unless there are only followers, in which case the first one ticked wins). */
		/** [翻译失败: This node will always be a follower (unless there are only followers, in which case the first one ticked wins).] */
		AlwaysFollower,

		/** This node will always be a leader (if more than one node is AlwaysLeader, the last one ticked wins). */
		/** [翻译失败: This node will always be a leader (if more than one node is AlwaysLeader, the last one ticked wins).] */
		AlwaysLeader,

		/** This node will be excluded from the sync group while blending in. Once blended in it will be the sync group leader until blended out*/
		/** 混合时，该节点将被排除在同步组之外。混合后，它将成为同步组领导者，直到混合为止*/
		TransitionLeader,

		/** This node will be excluded from the sync group while blending in. Once blended in it will be a follower until blended out*/
		/** 该节点在混合时将被排除在同步组之外。一旦混合，它将成为跟随者，直到混合为止*/
		TransitionFollower,

		/** This node will always be a leader. If it fails to be ticked as a leader it will be run as ungrouped asset player (EAnimSyncMethod::DoNotSync) .*/
		/** 该节点将永远是领导者。如果它未能被标记为领导者，它将作为未分组的资源播放器运行 (EAnimSyncMethod::DoNotSync) 。*/
		ExclusiveAlwaysLeader,
	};
}

// Deprecated - do not use
// 已弃用 - 请勿使用
UENUM()
enum class EAnimSyncGroupScope : uint8
{
	// Sync only with animations in the current instance (either main or linked instance)
 // 仅与当前实例（主实例或链接实例）中的动画同步
	Local,

	// Sync with all animations in the main and linked instances of this skeletal mesh component
 // 与该骨架网格物体组件的主实例和链接实例中的所有动画同步
	Component,
};

// How an asset will synchronize with other assets
// 资产如何与其他资产同步
UENUM()
enum class EAnimSyncMethod : uint8
{
	// Don't sync ever
 // 永远不要同步
	DoNotSync,

	// Use a named sync group
 // 使用指定的同步组
	SyncGroup,

	// Use the graph structure to provide a sync group to apply
 // 使用图结构提供要应用的同步组
	Graph
};

USTRUCT()
struct FAnimGroupInstance
{
	GENERATED_USTRUCT_BODY()

public:
	// The list of animation players in this group which are going to be evaluated this frame
 // 该组中将在本帧进行评估的动画播放器列表
	TArray<FAnimTickRecord> ActivePlayers;
/** 累积根运动的实用结构。 */

	// The current group leader
 // 现任课题组组长
	// @note : before ticking, this is invalid
 // @note：勾选之前，这是无效的
	// after ticking, this should contain the real leader
 // 勾选后，这应该包含真正的领导者
	// during ticket, this list gets sorted by LeaderScore of AnimTickRecord,
 // 在票务期间，此列表按 AnimTickRecord 的 LeaderScore 排序，
	// and it starts from 0 index, but if that fails due to invalid position, it will go to the next available leader
 // 它从 0 索引开始，但如果由于位置无效而失败，它将转到下一个可用的领导者
	int32 GroupLeaderIndex;

	// Valid marker names for this sync group
 // 该同步组的有效标记名称
	TArray<FName> ValidMarkers;

	// Can we use sync markers for ticking this sync group
 // 我们可以使用同步标记来勾选此同步组吗
	bool bCanUseMarkerSync;

	// This has latest Montage Leader Weight
 // 这是最新的 Montage Leader 重量
	float MontageLeaderWeight;

	FMarkerTickContext MarkerTickContext;

	// Float in 0 - 1 range representing how far through an animation we were before ticking
 // 在 0 - 1 范围内浮动，表示我们在滴答之前动画的进度
	float PreviousAnimLengthRatio;

/** 累积根运动的实用结构。 */
	// Float in 0 - 1 range representing how far through an animation we are
 // 在 0 - 1 范围内浮动，表示动画的进度
	float AnimLengthRatio;

public:
	FAnimGroupInstance()
		: GroupLeaderIndex(INDEX_NONE)
		, bCanUseMarkerSync(false)
		, MontageLeaderWeight(0.f)
		, PreviousAnimLengthRatio(0.f)
		, AnimLengthRatio(0.f)
	{
	}

	void Reset()
	{
		GroupLeaderIndex = INDEX_NONE;
		ActivePlayers.Reset();
		bCanUseMarkerSync = false;
		MontageLeaderWeight = 0.f;
		MarkerTickContext = FMarkerTickContext();
		PreviousAnimLengthRatio = 0.f;
		AnimLengthRatio = 0.f;
	}

	// Checks the last tick record in the ActivePlayers array to see if it's a better leader than the current candidate.
 // 检查 ActivePlayers 数组中的最后一个刻度记录，看看它是否是比当前候选者更好的领导者。
	// This should be called once for each record added to ActivePlayers, after the record is setup.
 // 设置记录后，应为添加到 ActivePlayers 的每个记录调用一次此函数。
	ENGINE_API void TestTickRecordForLeadership(EAnimGroupRole::Type MembershipType);

	UE_DEPRECATED(5.0, "Use TestTickRecordForLeadership, as it now internally supports montages")
	void TestMontageTickRecordForLeadership() { TestTickRecordForLeadership(EAnimGroupRole::CanBeLeader); }

	// Called after leader has been ticked and decided
 // 在领导者被勾选并决定后调用
	ENGINE_API void Finalize(const FAnimGroupInstance* PreviousGroup);

	// Called after all tick records have been added but before assets are actually ticked
 // 在添加所有刻度记录之后但在资产实际刻度之前调用
	ENGINE_API void Prepare(const FAnimGroupInstance* PreviousGroup);
};

/** Utility struct to accumulate root motion. */
/** 累积根运动的实用结构。 */
USTRUCT()
struct FRootMotionMovementParams
{
	GENERATED_USTRUCT_BODY()

private:
	ENGINE_API static FVector RootMotionScale;

public:
	
	UPROPERTY()
	bool bHasRootMotion;

	UPROPERTY()
	float BlendWeight;

private:
	UPROPERTY()
	FTransform RootMotionTransform;

public:
	FRootMotionMovementParams()
		: bHasRootMotion(false)
		, BlendWeight(0.f)
	{
	}

	// Copy/Move constructors and assignment operator added for deprecation support
 // 添加了复制/移动构造函数和赋值运算符以支持弃用
	// Could be removed once RootMotionTransform is made private
 // 一旦 RootMotionTransform 设为私有，就可以删除
	FRootMotionMovementParams(const FRootMotionMovementParams& Other)
		: bHasRootMotion(Other.bHasRootMotion)
		, BlendWeight(Other.BlendWeight)
	{
		RootMotionTransform = Other.RootMotionTransform;
	}

	FRootMotionMovementParams(const FRootMotionMovementParams&& Other)
		: bHasRootMotion(Other.bHasRootMotion)
		, BlendWeight(Other.BlendWeight)
	{
		RootMotionTransform = Other.RootMotionTransform;
	}

	FRootMotionMovementParams& operator=(const FRootMotionMovementParams& Other)
	{
		bHasRootMotion = Other.bHasRootMotion;
		BlendWeight = Other.BlendWeight;
		RootMotionTransform = Other.RootMotionTransform;
		return *this;
	}

	void Set(const FTransform& InTransform)
	{
		bHasRootMotion = true;
		RootMotionTransform = InTransform;
		RootMotionTransform.SetScale3D(RootMotionScale);
		BlendWeight = 1.f;
	}

	void Accumulate(const FTransform& InTransform)
	{
		if (!bHasRootMotion)
		{
			Set(InTransform);
		}
		else
		{
			RootMotionTransform = InTransform * RootMotionTransform;
			RootMotionTransform.SetScale3D(RootMotionScale);
		}
	}

	void Accumulate(const FRootMotionMovementParams& MovementParams)
	{
		if (MovementParams.bHasRootMotion)
		{
			Accumulate(MovementParams.RootMotionTransform);
		}
	}

	void AccumulateWithBlend(const FTransform& InTransform, float InBlendWeight)
	{
		const ScalarRegister VBlendWeight(InBlendWeight);
		if (bHasRootMotion)
		{
			RootMotionTransform.AccumulateWithShortestRotation(InTransform, VBlendWeight);
			RootMotionTransform.SetScale3D(RootMotionScale);
			BlendWeight += InBlendWeight;
		}
		else
		{
			Set(InTransform * VBlendWeight);
			BlendWeight = InBlendWeight;
		}
	}

	void AccumulateWithBlend(const FRootMotionMovementParams & MovementParams, float InBlendWeight)
	{
		if (MovementParams.bHasRootMotion)
		{
			AccumulateWithBlend(MovementParams.RootMotionTransform, InBlendWeight);
		}
	}

	void Clear()
	{
		bHasRootMotion = false;
		BlendWeight = 0.f;
	}

	void MakeUpToFullWeight()
	{
		float WeightLeft = FMath::Max(1.f - BlendWeight, 0.f);
		if (WeightLeft > UE_KINDA_SMALL_NUMBER)
		{
			AccumulateWithBlend(FTransform(), WeightLeft);
		}
		RootMotionTransform.NormalizeRotation();
	}

	FRootMotionMovementParams ConsumeRootMotion(float Alpha)
	{
		FTransform PartialRootMotion(FQuat::Slerp(FQuat::Identity, RootMotionTransform.GetRotation(), Alpha), RootMotionTransform.GetTranslation()*Alpha, RootMotionScale);

		// remove the part of the root motion we are applying now and leave the remainder in RootMotionTransform
  // 删除我们现在应用的根运动部分，并将其余部分保留在 RootMotionTransform 中
		RootMotionTransform = RootMotionTransform.GetRelativeTransform(PartialRootMotion);

		FRootMotionMovementParams ReturnParams;
		ReturnParams.Set(PartialRootMotion);

		check(PartialRootMotion.IsRotationNormalized());
		check(RootMotionTransform.IsRotationNormalized());
		return ReturnParams;
	}

	const FTransform& GetRootMotionTransform() const { return RootMotionTransform; }
	void ScaleRootMotionTranslation(float TranslationScale) { RootMotionTransform.ScaleTranslation(TranslationScale); }
};

// This structure is used to either advance or synchronize animation players
// 该结构用于推进或同步动画播放器
struct FAnimAssetTickContext
{
public:
	FAnimAssetTickContext(float InDeltaTime, ERootMotionMode::Type InRootMotionMode, bool bInOnlyOneAnimationInGroup, const TArray<FName>& ValidMarkerNames)
		: RootMotionMode(InRootMotionMode)
		, MarkerTickContext(ValidMarkerNames)
		, DeltaTime(InDeltaTime)
		, LeaderDelta(0.f)
		, PreviousAnimLengthRatio(0.0f)
		, AnimLengthRatio(0.0f)
		, bIsMarkerPositionValid(ValidMarkerNames.Num() > 0)
		, bIsLeader(true)
		, bOnlyOneAnimationInGroup(bInOnlyOneAnimationInGroup)
		, bResyncToSyncGroup(false)
	{
	}

	FAnimAssetTickContext(float InDeltaTime, ERootMotionMode::Type InRootMotionMode, bool bInOnlyOneAnimationInGroup)
		: RootMotionMode(InRootMotionMode)
		, DeltaTime(InDeltaTime)
		, LeaderDelta(0.f)
		, PreviousAnimLengthRatio(0.0f)
		, AnimLengthRatio(0.0f)
		, bIsMarkerPositionValid(false)
		, bIsLeader(true)
		, bOnlyOneAnimationInGroup(bInOnlyOneAnimationInGroup)
		, bResyncToSyncGroup(false)
	{
	}

	// Are we the leader of our sync group (or ungrouped)?
 // 我们是同步组（或未分组）的领导者吗？
	bool IsLeader() const
	{
		return bIsLeader;
	}

	bool IsFollower() const
	{
		return !bIsLeader;
	}

	// Return the delta time of the tick
 // 返回刻度的增量时间
	float GetDeltaTime() const
	{
		return DeltaTime;
	}

	void SetLeaderDelta(float InLeaderDelta)
	{
		LeaderDelta = InLeaderDelta;
	}

	float GetLeaderDelta() const
	{
		return LeaderDelta;
	}

	void SetPreviousAnimationPositionRatio(float NormalizedTime)
	{
		PreviousAnimLengthRatio = NormalizedTime;
	}

	void SetAnimationPositionRatio(float NormalizedTime)
	{
		AnimLengthRatio = NormalizedTime;
	}

	// Returns the previous synchronization point (normalized time)
 // 返回前一个同步点（标准化时间）
	float GetPreviousAnimationPositionRatio() const
	{
		return PreviousAnimLengthRatio;
	}

	// Returns the synchronization point (normalized time)
 // 返回同步点（标准化时间）
	float GetAnimationPositionRatio() const
	{
		return AnimLengthRatio;
	}

	void InvalidateMarkerSync()
	{
		bIsMarkerPositionValid = false;
	}

	bool CanUseMarkerPosition() const
	{
		return bIsMarkerPositionValid;
	}

	void ConvertToFollower()
	{
		bIsLeader = false;
	/** 指向可以播放该资源的骨架的指针。	*/
	}

	bool ShouldGenerateNotifies() const
	{
	/** 骷髅引导。如果发生变化，您需要重新映射信息*/
		return IsLeader();
	}

	/** 允许动画跟踪虚拟骨骼信息 */
	bool IsSingleAnimationContext() const
	{
		return bOnlyOneAnimationInGroup;
	}

	void SetResyncToSyncGroup(bool bInResyncToSyncGroup)
	{
		bResyncToSyncGroup = bInResyncToSyncGroup;
	}

	// Should we resync to the sync group this tick (eg: when initializing or resuming from zero weight)?
 // 我们是否应该在这个时间点重新同步到同步组（例如：初始化或从零权重恢复时）？
	bool ShouldResyncToSyncGroup() const
	{
		return bResyncToSyncGroup;
	}

	//Root Motion accumulated from this tick context
 // 从此刻度上下文中累积的根运动
	FRootMotionMovementParams RootMotionMovementParams;

	// The root motion mode of the owning AnimInstance
 // 所属 AnimInstance 的根运动模式
	ERootMotionMode::Type RootMotionMode;

	FMarkerTickContext MarkerTickContext;

private:
	float DeltaTime;

	float LeaderDelta;

	// Float in 0 - 1 range representing how far through an animation we were before ticking
 // 在 0 - 1 范围内浮动，表示我们在滴答之前动画的进度
	float PreviousAnimLengthRatio;

	// Float in 0 - 1 range representing how far through an animation we are
 // 在 0 - 1 范围内浮动，表示动画的进度
	float AnimLengthRatio;
	/** 指向可以播放该资源的骨架的指针。	*/

	bool bIsMarkerPositionValid;

	/** 设置ParentAsset时的资源映射表 */
	/** 骷髅引导。如果发生变化，您需要重新映射信息*/
	bool bIsLeader;

	bool bOnlyOneAnimationInGroup;
	/** 允许动画跟踪虚拟骨骼信息 */

	// True if the asset player being ticked should (re)synchronize to the sync group's time (eg: it was inactive and has now reactivated)
 // 如果被勾选的资产播放器应该（重新）同步到同步组的时间（例如：它之前处于非活动状态，现在已重新激活），则为 true
	/** 与资产一起存储的用户数据数组 */
	bool bResyncToSyncGroup;
};

USTRUCT()
struct FAnimationGroupReference
{
	GENERATED_USTRUCT_BODY()
	
	// How this animation will synchronize with other animations. 
 // 该动画如何与其他动画同步。
	UPROPERTY(EditAnywhere, Category=Settings)
	EAnimSyncMethod Method;

	// The group name that we synchronize with (NAME_None if it is not part of any group). 
 // 我们与之同步的组名称（如果不属于任何组，则为 NAME_None）。
	UPROPERTY(EditAnywhere, Category=Settings, meta = (EditCondition = "Method == EAnimSyncMethod::SyncGroup"))
	FName GroupName;

	// The role this animation can assume within the group (ignored if GroupName is not set)
 // 该动画在组内可以扮演的角色（如果未设置 GroupName，则忽略）
	UPROPERTY(EditAnywhere, Category=Settings, meta = (EditCondition = "Method == EAnimSyncMethod::SyncGroup"))
	TEnumAsByte<EAnimGroupRole::Type> GroupRole;

	FAnimationGroupReference()
		: Method(EAnimSyncMethod::DoNotSync)
		, GroupName(NAME_None)
		, GroupRole(EAnimGroupRole::CanBeLeader)
	/** 根据我们的骨架验证我们存储的数据并进行相应更新 */
	{
	}
};

UCLASS(abstract, BlueprintType, MinimalAPI)
class UAnimationAsset : public UObject, public IInterface_AssetUserData, public IInterface_PreviewMeshProvider
{
	GENERATED_BODY()

	/** 返回指定类的第一个元数据 */
private:
	/** Pointer to the Skeleton this asset can be played on .	*/
	/** [翻译失败: Pointer to the Skeleton this asset can be played on .]	*/
	UPROPERTY(AssetRegistrySearchable, Category=Animation, VisibleAnywhere)
	/** 设置ParentAsset时的资源映射表 */
	/** FindMetaDataByClass 的模板化版本，可为您处理转换 */
	TObjectPtr<class USkeleton> Skeleton;

	/** Skeleton guid. If changes, you need to remap info*/
	/** [翻译失败: Skeleton guid. If changes, you need to remap info]*/
	FGuid SkeletonGuid;
	/** 与资产一起存储的用户数据数组 */

	/** Allow animations to track virtual bone info */
	/** [翻译失败: Allow animations to track virtual bone info] */
	FGuid SkeletonVirtualBoneGuid; 

	/** Meta data that can be saved with the asset 
	 * 
	 * You can query by GetMetaData function
	 */
	/** IInterface_PreviewMeshProvider接口 */
	UPROPERTY(Category=MetaData, instanced, EditAnywhere)
	TArray<TObjectPtr<UAnimMetaData>> MetaData;

public:
	/* 
	 * Parent asset is used for AnimMontage when it derives all settings but remap animation asset. 
	/** 设置或更新预览骨架网格物体 */
	 * For example, you can just use all parent's setting  for the montage, but only remap assets
	 * This isn't magic bullet unfortunately and it is consistent effort of keeping the data synced with parent
	 * If you add new property, please make sure those property has to be copied for children. 
	 * If it does, please add the copy in the function RefreshParentAssetData
	 * We'd like to extend this feature to BlendSpace in the future
	 */
#if WITH_EDITORONLY_DATA
	/** Parent Asset, if set, you won't be able to edit any data in here but just mapping table
	 * 
	 * During cooking, this data will be used to bake out to normal asset */
	/** 根据我们的骨架验证我们存储的数据并进行相应更新 */
	UPROPERTY(AssetRegistrySearchable, Category=Animation, VisibleAnywhere)
	TObjectPtr<class UAnimationAsset> ParentAsset;

	/** 
	 * @todo : comment
	 */
	ENGINE_API void ValidateParentAsset();

	/**
	/** 返回指定类的第一个元数据 */
	 * note this is transient as they're added as they're loaded
	 */
	UPROPERTY(transient)
	TArray<TObjectPtr<class UAnimationAsset>> ChildrenAssets;
	/** FindMetaDataByClass 的模板化版本，可为您处理转换 */

	const UAssetMappingTable* GetAssetMappingTable() const
	{
		return AssetMappingTable;
	}
protected:
	/** Asset mapping table when ParentAsset is set */
	/** [翻译失败: Asset mapping table when ParentAsset is set] */
	UPROPERTY(Category=Animation, VisibleAnywhere)
	TObjectPtr<class UAssetMappingTable> AssetMappingTable;
#endif // WITH_EDITORONLY_DATA

protected:
	/** Array of user data stored with the asset */
	/** IInterface_PreviewMeshProvider接口 */
	/** 与资产一起存储的用户数据数组 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Instanced, Category = Animation)
	TArray<TObjectPtr<UAssetUserData>> AssetUserData;

public:
	/** Advances the asset player instance 
	/** 设置或更新预览骨架网格物体 */
	/** 返回用于混合兼容性的唯一标记名称列表 */
	 * 
	 * @param Instance		AnimationTickRecord Instance - saves data to evaluate
	 * @param NotifyQueue	Queue for any notifies we create
	 * @param Context		The tick context (leader/follower, delta time, sync point, etc...)
	 */
	virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const {}

	// this is used in editor only when used for transition getter
 // 仅当用于转换 getter 时才在编辑器中使用
	// this doesn't mean max time. In Sequence, this is SequenceLength,
 // 这并不意味着最长时间。在序列中，这是 SequenceLength，
	// but for BlendSpace CurrentTime is normalized [0,1], so this is 1
 // 但对于 BlendSpace CurrentTime 是标准化的 [0,1]，所以这是 1
	UE_DEPRECATED(5.0, "Use GetPlayLength instead")
	virtual float GetMaxCurrentTime() { return GetPlayLength(); }

	UFUNCTION(BlueprintPure, Category = "Animation", meta=(BlueprintThreadSafe))
	virtual float GetPlayLength() const { return 0.f; };

	ENGINE_API void SetSkeleton(USkeleton* NewSkeleton);
	UE_DEPRECATED(5.2, "ResetSkeleton has been deprecated, use ReplaceSkeleton or SetSkeleton instead")
	ENGINE_API void ResetSkeleton(USkeleton* NewSkeleton);
	ENGINE_API virtual void PostLoad() override;

	/** Validate our stored data against our skeleton and update accordingly */
	/** 根据我们的骨架验证我们存储的数据并进行相应更新 */
	/** 缩略图渲染信息 */
	ENGINE_API void ValidateSkeleton();

	ENGINE_API virtual void Serialize(FArchive& Ar) override;

	/** 预览此资源时使用的默认骨架网格物体 - 仅当您使用此资源打开角色时才适用*/
	/** Get available Metadata within the animation asset
	 */
	const TArray<UAnimMetaData*>& GetMetaData() const { return MetaData; }
	
	/** Returns the first metadata of the specified class */
	/** 返回指定类的第一个元数据 */
	/** 预览此资源时使用的默认骨架网格物体 - 仅当您使用此资源打开角色时才适用*/
	UFUNCTION(BlueprintCallable, Category = "Animation")
	ENGINE_API UAnimMetaData* FindMetaDataByClass(const TSubclassOf<UAnimMetaData> MetaDataClass) const;

	/** Templatized version of FindMetaDataByClass that handles casting for you */
	/** FindMetaDataByClass 的模板化版本，可为您处理转换 */
	template<class T>
	/** 返回用于混合兼容性的唯一标记名称列表 */
	T* FindMetaDataByClass() const
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UAnimMetaData>::Value, "'T' template parameter to FindMetaDataByClass must be derived from UAnimMetaData");

		return (T*)FindMetaDataByClass(T::StaticClass());
	}
	
	ENGINE_API void AddMetaData(UAnimMetaData* MetaDataInstance); 
	void EmptyMetaData() { MetaData.Empty(); }	
	ENGINE_API void RemoveMetaData(UAnimMetaData* MetaDataInstance);
	ENGINE_API void RemoveMetaData(TArrayView<UAnimMetaData*> MetaDataInstances);

	/** IInterface_PreviewMeshProvider interface */
	/** [翻译失败: IInterface_PreviewMeshProvider interface] */
	ENGINE_API virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true) override;
	ENGINE_API virtual USkeletalMesh* GetPreviewMesh(bool bFindIfNotSet = false) override;
	ENGINE_API virtual USkeletalMesh* GetPreviewMesh() const override;

#if WITH_EDITOR
	/** Sets or updates the preview skeletal mesh */
	/** [翻译失败: Sets or updates the preview skeletal mesh] */
	UFUNCTION(BlueprintCallable, Category=Animation)
	void SetPreviewSkeletalMesh(USkeletalMesh* PreviewMesh) { SetPreviewMesh(PreviewMesh); }
	
	/** Replace Skeleton 
	 * 
	/** 缩略图渲染信息 */
	 * @param NewSkeleton	NewSkeleton to change to 
	 */
	ENGINE_API bool ReplaceSkeleton(USkeleton* NewSkeleton, bool bConvertSpaces=false);

	/** 预览此资源时使用的默认骨架网格物体 - 仅当您使用此资源打开角色时才适用*/
	virtual void OnSetSkeleton(USkeleton* NewSkeleton) {}

	// Helper function for GetAllAnimationSequencesReferred, it adds itself first and call GetAllAnimationSEquencesReferred
 // GetAllAnimationSequencesReferred 的辅助函数，它首先添加自身并调用 GetAllAnimationSequencesReferred
	ENGINE_API void HandleAnimReferenceCollection(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive);

	/** Retrieve all animations that are used by this asset 
	/** 预览此资源时使用的默认骨架网格物体 - 仅当您使用此资源打开角色时才适用*/
	 * 
	 * @param (out)		AnimationSequences 
	 **/
	ENGINE_API virtual bool GetAllAnimationSequencesReferred(TArray<class UAnimationAsset*>& AnimationSequences, bool bRecursive = true);

public:
	/** Replace this assets references to other animations based on ReplacementMap 
	 * 
	 * @param ReplacementMap	Mapping of original asset to new asset
	 **/
	ENGINE_API virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap);

	virtual int32 GetMarkerUpdateCounter() const { return 0; }

	/** 
	 * Parent Asset related function. Used by editor
	 */
	ENGINE_API void SetParentAsset(UAnimationAsset* InParentAsset);
	bool HasParentAsset() const { return ParentAsset != nullptr; }
	ENGINE_API bool RemapAsset(UAnimationAsset* SourceAsset, UAnimationAsset* TargetAsset);
	// we have to update whenever we have anything loaded
 // 每当我们加载任何东西时我们都必须更新
	ENGINE_API void UpdateParentAsset();
protected:
	ENGINE_API virtual void RefreshParentAssetData();
#endif //WITH_EDITOR

public:
	/** Return a list of unique marker names for blending compatibility */
	/** 返回用于混合兼容性的唯一标记名称列表 */
	virtual TArray<FName>* GetUniqueMarkerNames() { return nullptr; }

	//~ Begin IInterface_AssetUserData Interface
 // ~ 开始 IInterface_AssetUserData 接口
	ENGINE_API virtual void AddAssetUserData(UAssetUserData* InUserData) override;
	ENGINE_API virtual void RemoveUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass) override;
	ENGINE_API virtual UAssetUserData* GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass) override;
	ENGINE_API virtual const TArray<UAssetUserData*>* GetAssetUserDataArray() const override;
	//~ End IInterface_AssetUserData Interface
 // ~ 结束 IInterface_AssetUserData 接口

	//~ Begin UObject Interface.
 // ~ 开始 UObject 接口。
#if WITH_EDITOR
	ENGINE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	ENGINE_API virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
	UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
	ENGINE_API virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	ENGINE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif // WITH_EDITOR

	/**
	* return true if this is valid additive animation
	* false otherwise
	*/
	virtual bool IsValidAdditive() const { return false; }

#if WITH_EDITORONLY_DATA
	/** Information for thumbnail rendering */
	/** 缩略图渲染信息 */
	UPROPERTY(VisibleAnywhere, Instanced, Category = Thumbnail)
	TObjectPtr<class UThumbnailInfo> ThumbnailInfo;

	/** The default skeletal mesh to use when previewing this asset - this only applies when you open Persona using this asset*/
	/** 预览此资源时使用的默认骨架网格物体 - 仅当您使用此资源打开角色时才适用*/
	// @todo: note that this doesn't retarget right now
 // @todo：请注意，这现在不会重新定位
	UPROPERTY(duplicatetransient, EditAnywhere, Category = Animation)
	TObjectPtr<class UPoseAsset> PreviewPoseAsset;

private:
	/** The default skeletal mesh to use when previewing this asset - this only applies when you open Persona using this asset*/
	/** 预览此资源时使用的默认骨架网格物体 - 仅当您使用此资源打开角色时才适用*/
	UPROPERTY(duplicatetransient, AssetRegistrySearchable)
	TSoftObjectPtr<class USkeletalMesh> PreviewSkeletalMesh;
#endif //WITH_EDITORONLY_DATA

protected:
#if WITH_EDITOR
	ENGINE_API virtual void RemapTracksToNewSkeleton(USkeleton* NewSkeleton, bool bConvertSpaces);
#endif // WITH_EDITOR

public:
	class USkeleton* GetSkeleton() const { return Skeleton; }

	FGuid GetSkeletonVirtualBoneGuid() const { return SkeletonVirtualBoneGuid; }
	void SetSkeletonVirtualBoneGuid(FGuid Guid) { SkeletonVirtualBoneGuid = Guid; }
	FGuid GetSkeletonGuid() const { return SkeletonGuid; }
};

