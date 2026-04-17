// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Abstract base class of animation made of multiple sequences.
 *
 */

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Animation/AnimLinkableElement.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimationAsset.h"
#include "AlphaBlend.h"
#include "Animation/AnimCompositeBase.h"
#include "Animation/TimeStretchCurve.h"
#include "AnimMontage.generated.h"

class UAnimInstance;
class UAnimMontage;
class UAnimNotifyState;
class UAnimSequence;
class UBlendProfile;
class USkeletalMeshComponent;

enum class EBlendProfileMode : uint8;


/**
 * Section data for each track. Reference of data will be stored in the child class for the way they want
 * AnimComposite vs AnimMontage have different requirement for the actual data reference
 * This only contains composite section information. (vertical sequences)
 */
USTRUCT()
struct FCompositeSection : public FAnimLinkableElement
{
	GENERATED_USTRUCT_BODY()

	/** Section Name */
	/** 部分名称 */
	UPROPERTY(EditAnywhere, Category=Section)
	FName SectionName;

#if WITH_EDITORONLY_DATA
	/** Start Time **/
	/** 开始时间 **/
	UPROPERTY()
	float StartTime_DEPRECATED;
#endif

	/** Should this animation loop. */
	/** [翻译失败: Should this animation loop.] */
	UPROPERTY(VisibleAnywhere, Category=Section)
	FName NextSectionName;

	/** Meta data that can be saved with the asset 
	 * 
	 * You can query by GetMetaData function
	 */
	UPROPERTY(Category=Section, Instanced, EditAnywhere)
	TArray<TObjectPtr<class UAnimMetaData>> MetaData;

public:
	FCompositeSection()
		: FAnimLinkableElement()
		, SectionName(NAME_None)
#if WITH_EDITORONLY_DATA
		, StartTime_DEPRECATED(0.0f)
#endif
		, NextSectionName(NAME_None)
	{
	}

	/** Get available Metadata for this section
	 */
	const TArray<class UAnimMetaData*>& GetMetaData() const { return MetaData; }
};

/**
 * Each slot data referenced by Animation Slot 
 * contains slot name, and animation data 
 */
USTRUCT()
struct FSlotAnimationTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Slot)
	FName SlotName;

	UPROPERTY(EditAnywhere, Category=Slot)
	FAnimTrack AnimTrack;

	ENGINE_API FSlotAnimationTrack();
};

/** 
 * Remove FBranchingPoint when VER_UE4_MONTAGE_BRANCHING_POINT_REMOVAL is removed.
 */
USTRUCT()
struct FBranchingPoint : public FAnimLinkableElement
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=BranchingPoint)
	FName EventName;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	float DisplayTime_DEPRECATED = 0.f;
#endif

	/** An offset from the DisplayTime to the actual time we will trigger the notify, as we cannot always trigger it exactly at the time the user wants */
	/** [翻译失败: An offset from the DisplayTime to the actual time we will trigger the notify, as we cannot always trigger it exactly at the time the user wants] */
	UPROPERTY()
	float TriggerTimeOffset = 0.f;

	/** Returns the time this branching point should be triggered */
	/** 返回应触发该分支点的时间 */
	float GetTriggerTime() const { return GetTime() + TriggerTimeOffset; }
};

/**  */
UENUM()
namespace EAnimNotifyEventType
{
	enum Type : int
	{
		/** */
		Begin,
		/** */
		End,
	};
}

UENUM()
enum class EMontageBlendMode : uint8
{
	//Uses standard weight based blend
	//使用基于标准重量的混合物
	Standard,
	//Uses inertialization. Requires an inertialization node somewhere in the graph after any slot node used by this montage.
	//使用惯性化。需要在图中某处此蒙太奇使用的任何槽节点之后有一个惯性化节点。
	Inertialization,
};

/** AnimNotifies marked as BranchingPoints will create these markers on their Begin/End times.
	They create stopping points when the Montage is being ticked to dispatch events. */
USTRUCT()
struct FBranchingPointMarker
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 NotifyIndex;
	
	UPROPERTY()
	float TriggerTime;

	UPROPERTY()
	TEnumAsByte<EAnimNotifyEventType::Type> NotifyEventType;

	FBranchingPointMarker()
		: NotifyIndex(INDEX_NONE)
		, TriggerTime(0.f)
		, NotifyEventType(EAnimNotifyEventType::Begin)
	{
	}

	FBranchingPointMarker(int32 InNotifyIndex, float InTriggerTime, EAnimNotifyEventType::Type InNotifyEventType)
		: NotifyIndex(InNotifyIndex)
		, TriggerTime(InTriggerTime)
		, NotifyEventType(InNotifyEventType)
	{
	}
};

UENUM()
enum class EMontageSubStepResult : uint8
{
	Moved,
	NotMoved,
	InvalidSection,
	InvalidMontage,
};

/**
 * Delegate for when Montage is completed, whether interrupted or finished
 * Weight of this montage is 0.f, so it stops contributing to output pose
 *
 * bInterrupted = true if it was not property finished
 */
DECLARE_DELEGATE_TwoParams( FOnMontageEnded, class UAnimMontage*, bool /*bInterrupted*/) 
/**
 * Delegate for when Montage started to blend out, whether interrupted or finished
 * DesiredWeight of this montage becomes 0.f, but this still contributes to the output pose
 * 
 * bInterrupted = true if it was not property finished
 */
DECLARE_DELEGATE_TwoParams( FOnMontageBlendingOutStarted, class UAnimMontage*, bool /*bInterrupted*/) 

DECLARE_DELEGATE_OneParam(FOnMontageBlendedInEnded, class UAnimMontage*)

/**
 * Delegate for when Montage changes section.
 *
 * bLooped = true if a requested to change to the current section.
 */
DECLARE_DELEGATE_ThreeParams( FOnMontageSectionChanged, class UAnimMontage*, FName /*SectionName*/ , bool /*bInterrupted*/)

/**
	Helper struct to sub step through Montages when advancing time.
	These require stopping at sections and branching points to potential jumps and loops.
	And also stepping through TimeStretchMarkers to adjust play rate based on TimeStretchCurve.
 */
struct FMontageSubStepper
{
private:
	const struct FAnimMontageInstance* MontageInstance;
	const class UAnimMontage* Montage;

	float TimeRemaining;
	float Cached_CombinedPlayRate;
	float PlayRate;
	float DeltaMove;
	bool bPlayingForward;

	int32 CurrentSectionIndex;
	float CurrentSectionStartTime;
	float CurrentSectionLength;
	bool bReachedEndOfSection;
	bool bHasValidTimeStretchCurveData;

	int32 TimeStretchMarkerIndex;

	mutable TArray<float> SectionStartPositions_Target;
	mutable TArray<float> SectionEndPositions_Target;

	float Cached_P_Target;
	float Cached_P_Original;

	FTimeStretchCurveInstance TimeStretchCurveInstance;

public:
	FMontageSubStepper()
		: MontageInstance(nullptr)
		, Montage(nullptr)
		, TimeRemaining(0.f)
		, Cached_CombinedPlayRate(0.f)
		, PlayRate(0.f)
		, DeltaMove(0.f)
		, bPlayingForward(true)
		, CurrentSectionIndex(INDEX_NONE)
		, CurrentSectionStartTime(0.f)
		, CurrentSectionLength(0.f)
		, bReachedEndOfSection(false)
		, bHasValidTimeStretchCurveData(false)
		, TimeStretchMarkerIndex(INDEX_NONE)
		, Cached_P_Target(FLT_MAX)
		, Cached_P_Original(FLT_MAX)
	{}

	void Initialize(const struct FAnimMontageInstance& InAnimInstance);

	void AddEvaluationTime(float InDeltaTime) { TimeRemaining += InDeltaTime; }
	bool HasTimeRemaining() const { return (TimeRemaining > UE_SMALL_NUMBER); }
	float GetRemainingTime() const { return TimeRemaining; }
	EMontageSubStepResult Advance(float& InOut_P_Original, const FBranchingPointMarker** OutBranchingPointMarkerPtr);
	bool HasReachedEndOfSection() const { return bReachedEndOfSection; }
	float GetRemainingPlayTimeToSectionEnd(const float In_P_Original);

	bool GetbPlayingForward() const { return bPlayingForward; }
	float GetDeltaMove() const { return DeltaMove; }
	int32 GetCurrentSectionIndex() const { return CurrentSectionIndex; }

	/** Invalidate Cached_CombinedPlayRate to force data to be recached in 'ConditionallyUpdateCachedData' */
	/** 使 Cached_CombinedPlayRate 无效以强制在“ConditionallyUpdateCachedData”中重新缓存数据 */
	void ClearCachedData() { Cached_CombinedPlayRate = FLT_MAX; }

private:
	/** 
		Updates markers P_Marker_Original and P_Marker_Target
		*only* if T_Target has changed. 
	*/
	void ConditionallyUpdateTimeStretchCurveCachedData();

	/** 
		Finds montage position in 'target' space, given current position in 'original' space.
		This means given a montage position, we find its playback time.
		This should only be used for montage position, as we cache results and lazily update it for performance.
	*/
	float FindMontagePosition_Target(float In_P_Original);

	/**
		Finds montage position in 'original' space, given current position in 'target' space.
		This means given a montage playback time, we find its actual position.
		This should only be used for montage position, as we cache results and lazily update it for performance.
	*/
	float FindMontagePosition_Original(float In_P_Target);

	/** 
		Gets current section end position in target space. 
		this is lazily cached, as it can be called every frame to test when to blend out. 
	*/
	float GetCurrSectionEndPosition_Target() const;

	/** 
		Gets current section start position in target space.
		this is lazily cached, as it can be called every frame to test when to blend out. 
	*/
	float GetCurrSectionStartPosition_Target() const;
};

/**
* Montage blend settings. Can be used to overwrite default Montage settings on Play/Stop
*/
USTRUCT(BlueprintType)
struct FMontageBlendSettings
{
	GENERATED_BODY()

	ENGINE_API FMontageBlendSettings();
	ENGINE_API FMontageBlendSettings(float BlendTime);
	ENGINE_API FMontageBlendSettings(const FAlphaBlendArgs& BlendArgs);

	/** Blend Profile to use for this blend */
	/** 用于此混合的混合配置文件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (DisplayAfter = "Blend"))
	TObjectPtr<UBlendProfile> BlendProfile;

	/** AlphaBlend options (time, curve, etc.) */
	/** AlphaBlend 选项（时间、曲线等） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (DisplayAfter = "BlendMode"))
	FAlphaBlendArgs Blend;

	/** Type of blend mode (Standard vs Inertial) */
	/** 混合模式类型（标准与惯性） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
	EMontageBlendMode BlendMode;
};

USTRUCT()
struct FAnimMontageInstance
{
	GENERATED_USTRUCT_BODY()

	friend struct FMontageSubStepper;

	// Montage reference
	// 蒙太奇参考
	UPROPERTY()
	TObjectPtr<class UAnimMontage> Montage;

	// delegates
	// 代表们
	FOnMontageEnded OnMontageEnded;
	FOnMontageBlendingOutStarted OnMontageBlendingOutStarted;
	FOnMontageBlendedInEnded OnMontageBlendedInEnded;
	FOnMontageSectionChanged OnMontageSectionChanged;
	
	UPROPERTY()
	bool bPlaying;

	// Blend Time multiplier to allow extending and narrowing blendtimes
	// 混合时间倍增器可延长和缩小混合时间
	UPROPERTY(transient)
	float DefaultBlendTimeMultiplier;

	// transient value of time position and delta in the last frame known
	// 已知最后一帧的时间位置和增量的瞬态值
	FDeltaTimeRecord DeltaTimeRecord;

	// marker tick record
	// 标记刻度记录
	FMarkerTickRecord MarkerTickRecord;

	// markers that passed in this tick
	// 在本次蜱虫中传递的标记
	TArray<FPassedMarker> MarkersPassedThisTick;

	// Whether this in this tick's call to Advance we used marker based sync
	// 无论是在本次对 Advance 的调用中，我们都使用了基于标记的同步
	bool bDidUseMarkerSyncThisTick;

	// enable auto blend out. This is instance set up. You can override
	// 启用自动混合。这是实例设置。您可以覆盖
	bool bEnableAutoBlendOut;

private:
	struct FMontageSubStepper MontageSubStepper;

	// list of next sections per section - index of array is section id
	// 每个部分的下一个部分的列表 - 数组的索引是部分 id
	UPROPERTY()
	TArray<int32> NextSections;

	// list of prev sections per section - index of array is section id
	// 每个部分的上一个部分的列表 - 数组的索引是部分 id
	UPROPERTY()
	TArray<int32> PrevSections;

	// reference to AnimInstance
	// 对 AnimInstance 的引用
	TWeakObjectPtr<UAnimInstance> AnimInstance;

	// Unique ID for this instance
	// 该实例的唯一ID
	int32 InstanceID;

	/** Currently Active AnimNotifyState, stored as a copy of the event as we need to
		call NotifyEnd on the event after a deletion in the editor. After this the event
		is removed correctly. */
	UPROPERTY(Transient)
	TArray<FAnimNotifyEvent> ActiveStateBranchingPoints;

	UPROPERTY()
	float Position;

	UPROPERTY()
	float PlayRate;

	UPROPERTY(transient)
	FAlphaBlend Blend;

	// need to save if it's interrupted or not
	// 是否中断需要保存
	// this information is crucial for gameplay
	// 这些信息对于游戏玩法至关重要
	bool bInterrupted;

	// transient PreviousWeight - Weight of previous tick
	// 瞬态 PreviousWeight - 前一个报价的权重
	float PreviousWeight;

	// transient NotifyWeight   - Weight for spawned notifies, modified slightly to make sure
	// 瞬态 NotifyWeight - 生成通知的权重，稍作修改以确保
	//                          - we spawn all notifies
	//                          - 我们生成所有通知
	float NotifyWeight;

	// The current start linear alpha value of the blend. This is not stored inside the FAlphaBlend struct.
	// 混合的当前起始线性 alpha 值。它不存储在 FAlphaBlend 结构中。
	float BlendStartAlpha;

	// sync group name
	// 同步组名称
	FName SyncGroupName;

	// Active blend profile.
	// 活跃的混合轮廓。
	UBlendProfile* ActiveBlendProfile;
	EBlendProfileMode ActiveBlendProfileMode;

	/**
	 * Optional evaluation range to use next update (ignoring the real delta time).
	 * Used by external systems that are setting animation times directly. Will fire off notifies and other events provided the animation system is ticking.
	 */
	TOptional<float> ForcedNextFromPosition;
	TOptional<float> ForcedNextToPosition;

	UPROPERTY(Transient)
	int32 DisableRootMotionCount;

public:
	/** Montage to Montage Synchronization.
	 *
	 * A montage can only have a single leader. A leader can have multiple followers.
	 * Loops cause no harm.
	 * If Follower gets ticked before Leader, then synchronization will be performed with a frame of lag.
	 *		Essentially correcting the previous frame. Which is enough for simple cases (i.e. no timeline jumps from notifies).
	 * If Follower gets ticked after Leader, then synchronization will be exact and support more complex cases (i.e. timeline jumps).
	 *		This can be enforced by setting up tick pre-requisites if desired.
	 */
	ENGINE_API void MontageSync_Follow(struct FAnimMontageInstance* NewLeaderMontageInstance);
	/** Stop leading, release all followers. */
	/** 停止领导，释放所有追随者。 */
	ENGINE_API void MontageSync_StopLeading();
	/** Stop following our leader */
	/** 别再追随我们的领袖了 */
	ENGINE_API void MontageSync_StopFollowing();
	/** PreUpdate - Sync if updated before Leader. */
	/** [翻译失败: PreUpdate - Sync if updated before Leader.] */
	ENGINE_API void MontageSync_PreUpdate();
	/** PostUpdate - Sync if updated after Leader. */
	/** [翻译失败: PostUpdate - Sync if updated after Leader.] */
	ENGINE_API void MontageSync_PostUpdate();

	/** Get Weight */
	/** [翻译失败: Get Weight] */
	float GetWeight() const { return Blend.GetBlendedValue(); }
	float GetDesiredWeight() const { return Blend.GetDesiredValue(); }
	float GetBlendTime() const { return Blend.GetBlendTime(); }
	UE_DEPRECATED(4.26, "Please use GetSyncGroupName")
	int32 GetSyncGroupIndex() const { return INDEX_NONE;  }
	FName GetSyncGroupName() const { return SyncGroupName;  }

	/** Set the weight */
	/** [翻译失败: Set the weight] */
	void SetWeight(float InValue) { Blend.SetAlpha(InValue); }
	/** Set the Desired Weight */
	/** [翻译失败: Set the Desired Weight] */
	void SetDesiredWeight(float InValue) { Blend.SetDesiredValue(InValue); }

	/** Get the current blend info. */
	/** [翻译失败: Get the current blend info.] */
	const FAlphaBlend& GetBlend() const { return Blend; }

private:
	/** Followers this Montage will synchronize */
	/** 此蒙太奇将同步关注者 */
	TArray<struct FAnimMontageInstance*> MontageSyncFollowers;
	/** Leader this Montage will follow */
	/** 这个蒙太奇将跟随领导者 */
	struct FAnimMontageInstance* MontageSyncLeader;
	/** Frame counter to sync montages once per frame */
	/** 帧计数器每帧同步一次蒙太奇 */
	uint32 MontageSyncUpdateFrameCounter;

	/** true if montage has been updated this frame */
	/** true 如果蒙太奇已更新此帧 */
	ENGINE_API bool MontageSync_HasBeenUpdatedThisFrame() const;
	/** This frame's counter, to track which Montages have been updated */
	/** 该帧的计数器，用于跟踪哪些蒙太奇已更新 */
	ENGINE_API uint32 MontageSync_GetFrameCounter() const;
	/** Synchronize ourselves to our leader */
	/** 让我们自己与我们的领导同步 */
	ENGINE_API void MontageSync_PerformSyncToLeader();

	/** Initialize Blend Setup from Montage */
	/** 从蒙太奇初始化混合设置 */
	ENGINE_API void InitializeBlend(const FAlphaBlend& InAlphaBlend);

	/**  Notify may invalidate current montage instance. Inputs should be memory not belonging to calling FAnimMontageInstance.*/
	/**  通知可能会使当前蒙太奇实例无效。输入应该是不属于调用 FAnimMontageInstance 的内存。*/
	static ENGINE_API bool ValidateInstanceAfterNotifyState(const TWeakObjectPtr<UAnimInstance>& InAnimInstance, const UAnimNotifyState* InNotifyStateClass);

public:
	ENGINE_API FAnimMontageInstance();

	ENGINE_API FAnimMontageInstance(UAnimInstance * InAnimInstance);

	//~ Begin montage instance Interfaces
	//~ 开始蒙太奇实例界面

	// Blend in with the supplied play rate. Other blend settings will come from the Montage asset.
	// 与提供的播放速率混合。其他混合设置将来自蒙太奇资源。
	ENGINE_API void Play(float InPlayRate = 1.f);
	// Blend in with the supplied blend settings
	// 使用提供的混合设置进行混合
	ENGINE_API void Play(float InPlayRate, const FMontageBlendSettings& BlendInSettings);

	// Blend out with the supplied FAlphaBlend. Other blend settings will come from the Montage asset.
	// 使用提供的 FAlphaBlend 进行混合。其他混合设置将来自蒙太奇资源。
	ENGINE_API void Stop(const FAlphaBlend& InBlendOut, bool bInterrupt=true);
	// Blend out with the supplied blend settings
	// 使用提供的混合设置进行混合
	ENGINE_API void Stop(const FMontageBlendSettings& InBlendOutSettings, bool bInterrupt=true);

	ENGINE_API void Pause();
	ENGINE_API void Initialize(class UAnimMontage * InMontage);

	ENGINE_API bool JumpToSectionName(FName const & SectionName, bool bEndOfSection = false);
	ENGINE_API bool SetNextSectionName(FName const & SectionName, FName const & NewNextSectionName);
	ENGINE_API bool SetNextSectionID(int32 const & SectionID, int32 const & NewNextSectionID);

	bool IsValid() const { return (Montage!=nullptr); }
	bool IsPlaying() const { return IsValid() && bPlaying; }
	void SetPlaying(bool bInPlaying) { bPlaying = bInPlaying; }
	bool IsStopped() const { return Blend.GetDesiredValue() == 0.f; }

	/** Returns true if this montage is active (valid and not blending out) */
	/** 如果此蒙太奇处于活动状态（有效且未混合），则返回 true */
	bool IsActive() const { return (IsValid() && !IsStopped()); }

	ENGINE_API void Terminate();

	/** return true if it can use marker sync */
	/** 如果可以使用标记同步则返回 true */
	ENGINE_API bool CanUseMarkerSync() const;

	/**
	 *  Getters
	 */
	int32 GetInstanceID() const { return InstanceID; }
	float GetPosition() const { return Position; };
	float GetPlayRate() const { return PlayRate; }
	float GetDeltaMoved() const { return DeltaTimeRecord.Delta; }
	float GetPreviousPosition() const { return DeltaTimeRecord.GetPrevious();  }
	float GetBlendStartAlpha() const { return BlendStartAlpha; }
	const FAnimMontageInstance* GetMontageSyncLeader() const { return MontageSyncLeader; } 
	const UBlendProfile* GetActiveBlendProfile() const { return ActiveBlendProfile; }
	const EBlendProfileMode GetActiveBlendProfileMode() const { return ActiveBlendProfileMode; }

	/** 
	 * Setters
	 */
	void SetPosition(float const & InPosition) { Position = InPosition; MarkerTickRecord.Reset(); }
	void SetPlayRate(float const & InPlayRate) { PlayRate = InPlayRate; }

	// Disable RootMotion
	// 禁用 RootMotion
	void PushDisableRootMotion() { DisableRootMotionCount++; }
	void PopDisableRootMotion() { DisableRootMotionCount--; }
	bool IsRootMotionDisabled() const {	return DisableRootMotionCount > 0; }

	/** Set the next position of this animation for the next animation update tick. Will trigger events and notifies since last position. */
	/** 设置该动画的下一个位置作为下一个动画更新标记。自上次位置以来将触发事件和通知。 */
	void SetNextPositionWithEvents(float InPosition) { ForcedNextFromPosition.Reset();  ForcedNextToPosition = InPosition; }
	/** Set the evaluation range of this animation for the next animation update tick. Will trigger events and notifies for that range. */
	/** 设置下一个动画更新tick的该动画的评估范围。将触发该范围的事件和通知。 */
	void SetNextPositionWithEvents(float InFromPosition, float InToPosition) { ForcedNextFromPosition = InFromPosition; ForcedNextToPosition = InToPosition; }

	/**
	 * Montage Tick happens in 2 phases
	 *
	 * first is to update weight of current montage only
	 * this will make sure that all nodes will get up-to-date weight information
	 * when update comes in for them
	 *
	 * second is normal tick. This tick has to happen later when all node ticks
	 * to accumulate and update curve data/notifies/branching points
	 */
	ENGINE_API void UpdateWeight(float DeltaTime);

#if WITH_EDITOR	
	ENGINE_API void EditorOnly_PreAdvance();
#endif

	/** Simulate is same as Advance, but without calling any events or touching any of the instance data. So it performs a simulation of advancing the timeline. */
	/** 模拟与高级相同，但不调用任何事件或接触任何实例数据。因此它执行了推进时间线的模拟。 */
	ENGINE_API bool SimulateAdvance(float DeltaTime, float& InOutPosition, struct FRootMotionMovementParams & OutRootMotionParams) const;
	ENGINE_API void Advance(float DeltaTime, struct FRootMotionMovementParams * OutRootMotionParams, bool bBlendRootMotion);

	ENGINE_API FName GetCurrentSection() const;
	ENGINE_API FName GetNextSection() const;
	ENGINE_API int32 GetNextSectionID(int32 const & CurrentSectionID) const;
	ENGINE_API FName GetSectionNameFromID(int32 const & SectionID) const;

	// reference has to be managed manually
	// 必须手动管理引用
	ENGINE_API void AddReferencedObjects( FReferenceCollector& Collector );

	/** Delegate function handlers
	 */
	ENGINE_API void HandleEvents(float PreviousTrackPos, float CurrentTrackPos, const FBranchingPointMarker* BranchingPointMarker);

private:
	/** Called by blueprint functions that modify the montages current position. */
	/** 由修改蒙太奇当前位置的蓝图函数调用。 */
	ENGINE_API void OnMontagePositionChanged(FName const & ToSectionName);
	
	/** Updates ActiveStateBranchingPoints array and triggers Begin/End notifications based on CurrentTrackPosition */
	/** [翻译失败: Updates ActiveStateBranchingPoints array and triggers Begin/End notifications based on CurrentTrackPosition] */
	/** Returns false if montage instance was destroyed during branching point update*/
	/** [翻译失败: Returns false if montage instance was destroyed during branching point update]*/
	ENGINE_API bool UpdateActiveStateBranchingPoints(float CurrentTrackPosition);

	/** Trigger associated events when Montage ticking reaches given FBranchingPointMarker */
	/** [翻译失败: Trigger associated events when Montage ticking reaches given FBranchingPointMarker] */
	ENGINE_API void BranchingPointEventHandler(const FBranchingPointMarker* BranchingPointMarker);
	ENGINE_API void RefreshNextPrevSections();

	ENGINE_API float GetRemainingPlayTimeToSectionEnd(const FMontageSubStepper& MontageSubStepper) const;

public:
	/** static functions that are used by sequencer montage support*/
	/** 音序器蒙太奇支持使用的静态函数*/
	static ENGINE_API UAnimMontage* SetSequencerMontagePosition(FName SlotName, USkeletalMeshComponent* SkeletalMeshComponent, int32& InOutInstanceId, UAnimSequenceBase* InAnimSequence, float InFromPosition, float InToPosition, float Weight, bool bLooping, bool bPlaying);
	static ENGINE_API UAnimMontage* PreviewSequencerMontagePosition(FName SlotName, USkeletalMeshComponent* SkeletalMeshComponent, int32& InOutInstanceId, UAnimSequenceBase* InAnimSequence, float InFromPosition, float InToPosition, float Weight, bool bLooping, bool bFireNotifies, bool bPlaying);
	static ENGINE_API UAnimMontage* SetSequencerMontagePosition(FName SlotName, UAnimInstance* AnimInstance, int32& InOutInstanceId, UAnimSequenceBase* InAnimSequence, float InFromPosition, float InToPosition, float Weight, bool bLooping, bool bPlaying);
	static ENGINE_API UAnimMontage* PreviewSequencerMontagePosition(FName SlotName, USkeletalMeshComponent* SkeletalMeshComponent, UAnimInstance* AnimInstance, int32& InOutInstanceId, UAnimSequenceBase* InAnimSequence, float InFromPosition, float InToPosition, float Weight, bool bLooping, bool bFireNotifies, bool bPlaying);
};

/**
 * Any property you're adding to AnimMontage and parent class has to be considered for Child Asset
 *
 * Child Asset is considered to be only asset mapping feature using everything else in the class
 * For example, you can just use all parent's setting  for the montage, but only remap assets
 * This isn't magic bullet unfortunately and it is consistent effort of keeping the data synced with parent
 * If you add new property, please make sure those property has to be copied for children.
 * If it does, please add the copy in the function RefreshParentAssetData
 */
UCLASS(config=Engine, hidecategories=(UObject, Length), MinimalAPI, BlueprintType, meta= (LoadBehavior = "LazyOnDemand"))
class UAnimMontage : public UAnimCompositeBase
{
	GENERATED_UCLASS_BODY()

	friend struct FAnimMontageInstance;
	friend class UAnimMontageFactory;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BlendOption)
	EMontageBlendMode BlendModeIn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BlendOption)
	EMontageBlendMode BlendModeOut;

	/** Blend in option. */
	/** 混合选项。 */
	UPROPERTY(EditAnywhere, Category=BlendOption)
	FAlphaBlend BlendIn;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	float BlendInTime_DEPRECATED;
#endif

	/** Blend out option. This is only used when it blends out itself. If it's interrupted by other montages, it will use new montage's BlendIn option to blend out. */
	/** 混合选项。仅当其自身混合时才使用。如果它被其他蒙太奇打断，它将使用新蒙太奇的 BlendIn 选项来混合。 */
	UPROPERTY(EditAnywhere, Category=BlendOption)
	FAlphaBlend BlendOut;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	float BlendOutTime_DEPRECATED;
#endif

	/** Time from Sequence End to trigger blend out.
	 * <0 means using BlendOutTime, so BlendOut finishes as Montage ends.
	 * >=0 means using 'SequenceEnd - BlendOutTriggerTime' to trigger blend out. */
	UPROPERTY(EditAnywhere, Category = BlendOption)
	float BlendOutTriggerTime;

	UFUNCTION(BlueprintPure, Category = "Montage")
	FAlphaBlendArgs GetBlendInArgs() const { return FAlphaBlendArgs(BlendIn); }

	UFUNCTION(BlueprintPure, Category = "Montage")
	FAlphaBlendArgs GetBlendOutArgs() const { return FAlphaBlendArgs(BlendOut); }

	UFUNCTION(BlueprintPure, Category = "Montage")
	float GetDefaultBlendInTime() const { return BlendIn.GetBlendTime(); }

	UFUNCTION(BlueprintCallable, Category = "Montage")
	float GetDefaultBlendOutTime() const { return BlendOut.GetBlendTime(); }

	/** If you're using marker based sync for this montage, make sure to add sync group name. For now we only support one group */
	/** 如果您对此剪辑使用基于标记的同步，请确保添加同步组名称。目前我们只支持一组 */
	UPROPERTY(EditAnywhere, Category = SyncGroup)
	FName SyncGroup;

	/** Index of the slot track used for collecting sync markers */
	/** [翻译失败: Index of the slot track used for collecting sync markers] */
	UPROPERTY(EditAnywhere, Category = SyncGroup)
	int32 SyncSlotIndex;

	UPROPERTY()
	struct FMarkerSyncData	MarkerData;

	// composite section. 
	// [翻译失败: composite section.]
	UPROPERTY()
	TArray<FCompositeSection> CompositeSections;
	
	// slot data, each slot contains anim track
	// [翻译失败: slot data, each slot contains anim track]
	UPROPERTY()
	TArray<struct FSlotAnimationTrack> SlotAnimTracks;

#if WITH_EDITORONLY_DATA
	// Remove this when VER_UE4_MONTAGE_BRANCHING_POINT_REMOVAL is removed.
	// [翻译失败: Remove this when VER_UE4_MONTAGE_BRANCHING_POINT_REMOVAL is removed.]
	UPROPERTY()
	TArray<struct FBranchingPoint> BranchingPoints_DEPRECATED;
#endif

	/** If this is on, it will allow extracting root motion translation. DEPRECATED in 4.5 root motion is controlled by anim sequences **/
	/** 如果打开，它将允许提取根运动平移。 4.5 中已弃用 根运动由动画序列控制 **/
	UPROPERTY()
	bool bEnableRootMotionTranslation;

	/** If this is on, it will allow extracting root motion rotation. DEPRECATED in 4.5 root motion is controlled by anim sequences **/
	/** 如果打开，它将允许提取根运动旋转。 4.5 中已弃用 根运动由动画序列控制 **/
	UPROPERTY()
	bool bEnableRootMotionRotation;

	/** When it hits end, it automatically blends out. If this is false, it won't blend out but keep the last pose until stopped explicitly */
	/** 当它结束时，它会自动混合出来。如果这是假的，它不会混合，但会保留最后一个姿势，直到明确停止 */
	UPROPERTY(EditAnywhere, Category = BlendOption)
	bool bEnableAutoBlendOut;

	/** The blend profile to use. */
	/** 要使用的混合配置文件。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BlendOption, meta = (UseAsBlendProfile = true))
	TObjectPtr<UBlendProfile> BlendProfileIn;

	/** The blend profile to use. */
	/** 要使用的混合配置文件。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BlendOption, meta = (UseAsBlendProfile = true))
	TObjectPtr<UBlendProfile> BlendProfileOut;

	/** Root Bone will be locked to that position when extracting root motion. DEPRECATED in 4.5 root motion is controlled by anim sequences **/
	/** 提取根运动时，根骨骼将被锁定到该位置。 4.5 中已弃用 根运动由动画序列控制 **/
	UPROPERTY()
	TEnumAsByte<ERootMotionRootLock::Type> RootMotionRootLock;

#if WITH_EDITORONLY_DATA
	/** Preview Base pose for additive BlendSpace **/
	/** 预览附加 BlendSpace 的基本姿势 **/
	UPROPERTY(EditAnywhere, Category = AdditiveSettings)
	TObjectPtr<UAnimSequence> PreviewBasePose;
#endif // WITH_EDITORONLY_DATA

	// Add new slot track to this montage
	// 向此蒙太奇添加新的老虎机轨道
	ENGINE_API FSlotAnimationTrack& AddSlot(FName SlotName);

	/** return true if valid slot */
	/** 如果有效槽则返回 true */
	ENGINE_API bool IsValidSlot(FName InSlotName) const;

	UFUNCTION(BlueprintPure, Category = "Montage")
	ENGINE_API bool IsDynamicMontage() const;

	UFUNCTION(BlueprintPure, Category = "Montage")
	ENGINE_API UAnimSequenceBase* GetFirstAnimReference() const;

public:
	//~ Begin UObject Interface
	//~ 开始 UObject 接口
#if WITH_EDITORONLY_DATA
	virtual void Serialize(FArchive& Ar) override;
#endif // WITH_EDITORONLY_DATA
	virtual void PostLoad() override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;

	virtual FFrameRate GetSamplingFrameRate() const override;

	// Gets the sequence length of the montage by calculating it from the lengths of the segments in the montage
	// 通过根据蒙太奇中片段的长度计算来获取蒙太奇的序列长度
	ENGINE_API float CalculateSequenceLength();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~ End UObject Interface
	//[翻译失败: ~ End UObject Interface]

	//~ Begin AnimSequenceBase Interface
	//[翻译失败: ~ Begin AnimSequenceBase Interface]
	virtual bool IsValidAdditive() const override;
#if WITH_EDITOR
	virtual EAnimEventTriggerOffsets::Type CalculateOffsetForNotify(float NotifyDisplayTime) const override;
#endif // WITH_EDITOR
	virtual void GetMarkerIndicesForTime(float CurrentTime, bool bLooping, const TArray<FName>& ValidMarkerNames, FMarkerPair& OutPrevMarker, FMarkerPair& OutNextMarker) const override;
	UE_DEPRECATED(5.0, "Use other GetMarkerSyncPositionfromMarkerIndicies signature")
    virtual FMarkerSyncAnimPosition GetMarkerSyncPositionfromMarkerIndicies(int32 PrevMarker, int32 NextMarker, float CurrentTime) const { return UAnimMontage::GetMarkerSyncPositionFromMarkerIndicies(PrevMarker, NextMarker, CurrentTime, nullptr); }
	virtual FMarkerSyncAnimPosition GetMarkerSyncPositionFromMarkerIndicies(int32 PrevMarker, int32 NextMarker, float CurrentTime, const UMirrorDataTable* MirrorTable) const override;
	virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const override;
	virtual TArray<FName>* GetUniqueMarkerNames() override { return &MarkerData.UniqueMarkerNames; }
	virtual void RefreshCacheData() override;
	virtual bool CanBeUsedInComposition() const { return false; }
	virtual void GetAnimationPose(FAnimationPoseData& OutPoseData, const FAnimExtractContext& ExtractionContext) const override { check(false); /* Should never be called, montages dont use this API */ }
	//~ End AnimSequenceBase Interface
	//[翻译失败: ~ End AnimSequenceBase Interface]

#if WITH_EDITOR
	//~ Begin UAnimationAsset Interface
	//~ 开始 UAnimationAsset 接口
	virtual bool GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive = true) override;
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap) override;
	//~ End UAnimationAsset Interface
	//~ 结束 UAnimationAsset 接口

	/** Update all linkable elements contained in the montage */
	/** 更新蒙太奇中包含的所有可链接元素 */
	ENGINE_API void UpdateLinkableElements();

	/** Update linkable elements that rely on a specific segment. This will update linkable elements for the segment specified
	 *	and elements linked to segments after the segment specified
	 *	@param SlotIdx The slot that the segment is contained in
	 *	@param SegmentIdx The index of the segment within the specified slot
	 */
	ENGINE_API void UpdateLinkableElements(int32 SlotIdx, int32 SegmentIdx);
#endif
	/** Check if this slot has a valid additive animation for the specified slot.
	 * The slot name should not include the group name.
	 * i.e. for "DefaultGroup.DefaultSlot", the slot name is "DefaultSlot".
	 */
	UFUNCTION(BlueprintPure, Category = "Montage")
	bool IsValidAdditiveSlot(const FName& SlotNodeName) const;

	/** Get FCompositeSection with InSectionName */
	/** 使用 InSectionName 获取 FCompositeSection */
	ENGINE_API FCompositeSection& GetAnimCompositeSection(int32 SectionIndex);
	ENGINE_API const FCompositeSection& GetAnimCompositeSection(int32 SectionIndex) const;

	// @todo document
	// @todo文档
	ENGINE_API void GetSectionStartAndEndTime(int32 SectionIndex, float& OutStartTime, float& OutEndTime) const;
	
	// @todo document
	// @todo文档
	ENGINE_API float GetSectionLength(int32 SectionIndex) const;
	
	/** Get SectionIndex from SectionName. Returns INDEX_None if not found */
	/** 从SectionName 获取SectionIndex。如果未找到则返回 INDEX_None */
	UFUNCTION(BlueprintPure, Category = "Montage")
	ENGINE_API int32 GetSectionIndex(FName InSectionName) const;
	
	/** Get SectionName from SectionIndex. Returns NAME_None if not found */
	/** 从SectionIndex 获取SectionName。如果未找到则返回 NAME_None */
	UFUNCTION(BlueprintPure, Category = "Montage")
	ENGINE_API FName GetSectionName(int32 SectionIndex) const;

	/** Returns the number of sections this montage has */
	/** 返回该蒙太奇的节数 */
	UFUNCTION(BlueprintPure, Category = "Montage")
	int32 GetNumSections() const { return CompositeSections.Num(); }

	/** @return true if valid section */
	/** @return true 如果有效部分 */
	UFUNCTION(BlueprintCallable, Category = "Montage")
	ENGINE_API bool IsValidSectionName(FName InSectionName) const;

	// @todo document
	// @todo文档
	ENGINE_API bool IsValidSectionIndex(int32 SectionIndex) const;

	/** Return Section Index from Position */
	/** 从位置返回部分索引 */
	ENGINE_API int32 GetSectionIndexFromPosition(float Position) const;
	
	/**
	 * Get Section Metadata for the montage including metadata belong to the anim reference
	 * This will remove redundant entry if found - i.e. multiple same anim reference is used
	 * 
	 * @param : SectionName - Name of section you'd like to get meta data for. 
	 *						- If SectionName == NONE, it will return all the section data
	 * @param : bIncludeSequence - if true, it returns all metadata of the animation within that section
	 *						- whether partial or full
	 * @param : SlotName - this only matters if bIncludeSequence is true.
	 *						- If true, and if SlotName is given, it will only look for SlotName.
	 *						- If true and if SlotName is none, then it will look for all slot nodes
	 ***/

	ENGINE_API const TArray<class UAnimMetaData*> GetSectionMetaData(FName SectionName, bool bIncludeSequence=true, FName SlotName = NAME_None);

	/** Get Section Index from CurrentTime with PosWithinCompositeSection */
	/** [翻译失败: Get Section Index from CurrentTime with PosWithinCompositeSection] */
	ENGINE_API int32 GetAnimCompositeSectionIndexFromPos(float CurrentTime, float& PosWithinCompositeSection) const;

	/** Return time left to end of section from given position. -1.f if not a valid position */
	/** [翻译失败: Return time left to end of section from given position. -1.f if not a valid position] */
	ENGINE_API float GetSectionTimeLeftFromPos(float Position);

	/** Utility function to calculate Animation Pos from Section, PosWithinCompositeSection */
	/** 用于从Section、PosWithinCompositeSection 计算动画位置的实用函数 */
	float CalculatePos(FCompositeSection &Section, float PosWithinCompositeSection) const;
	
	/** Prototype function to get animation data - this will need rework */
	/** 获取动画数据的原型函数 - 这需要返工 */
	ENGINE_API const FAnimTrack* GetAnimationData(FName SlotName) const;

	/** Returns whether the anim sequences this montage have root motion enabled */
	/** 返回此蒙太奇的动画序列是否启用了根运动 */
	virtual bool HasRootMotion() const override;

	/** Extract RootMotion Transform from a contiguous Track position range.
	 * *CONTIGUOUS* means that if playing forward StartTractPosition < EndTrackPosition.
	 * No wrapping over if looping. No jumping across different sections.
	 * So the AnimMontage has to break the update into contiguous pieces to handle those cases.
	 *
	 * This does handle Montage playing backwards (StartTrackPosition > EndTrackPosition).
	 *
	 * It will break down the range into steps if needed to handle looping animations, or different animations.
	 * These steps will be processed sequentially, and output the RootMotion transform in component space.
	 */
	ENGINE_API FTransform ExtractRootMotionFromTrackRange(float StartTrackPosition, float EndTrackPosition, const FAnimExtractContext& Context) const;

	UE_DEPRECATED(5.6, "Use ExtractRootMotionFromTrackRange with FAnimExtractContext")
	FTransform ExtractRootMotionFromTrackRange(float StartTrackPosition, float EndTrackPosition) const { const FAnimExtractContext Context; return ExtractRootMotionFromTrackRange(StartTrackPosition, EndTrackPosition, Context); }

	/** Get the Montage's Group Name. This is the group from the first slot.  */
	/** 获取蒙太奇的组名称。这是从第一个槽位开始的组。  */
	UFUNCTION(BlueprintPure, Category = "Montage")
	ENGINE_API FName GetGroupName() const;

	/** true if valid, false otherwise. Will log warning if not valid. */
	/** 如果有效则为 true，否则为 false。如果无效，将记录警告。 */
	bool HasValidSlotSetup() const;

private:
	/** 
	 * Utility function to check if CurrentTime is between FirstIndex and SecondIndex of CompositeSections
	 * return true if it is
	 */
	bool IsWithinPos(int32 FirstIndex, int32 SecondIndex, float CurrentTime) const;

	/** Calculates a trigger offset based on the supplied time taking into account only the montages sections */
	/** 根据提供的时间计算触发偏移，仅考虑蒙太奇部分 */
	EAnimEventTriggerOffsets::Type CalculateOffsetFromSections(float Time) const;


public:
#if WITH_EDITOR
	/**
	 * Add Composite section with InSectionName
	 * returns index of added item 
	 * returns INDEX_NONE if failed. - i.e. InSectionName is not unique
	 */
	ENGINE_API int32 AddAnimCompositeSection(FName InSectionName, float StartPos);
	
	/** 
	 * Delete Composite section with InSectionName
	 * return true if success, false otherwise
	 */
	ENGINE_API bool DeleteAnimCompositeSection(int32 SectionIndex);

private:
	/** Sort CompositeSections in the order of StartPos */
	/** 按 StartPos 的顺序对 CompositeSections 进行排序 */
	void SortAnimCompositeSectionByPos();

	/** Refresh Parent Asset Data to the child */
	/** 将父级资产数据刷新到子级 */
	virtual void RefreshParentAssetData() override;
	
	/** Propagate the changes to children */
	/** 将改变传播给孩子 */
	void PropagateChanges();

private:
	DECLARE_MULTICAST_DELEGATE(FOnMontageChangedMulticaster);
	FOnMontageChangedMulticaster OnMontageChanged;

public:
	typedef FOnMontageChangedMulticaster::FDelegate FOnMontageChanged;

	/** Registers a delegate to be called after notification has changed*/
	/** 注册一个委托，以便在通知更改后调用*/
	ENGINE_API void RegisterOnMontageChanged(const FOnMontageChanged& Delegate);
	ENGINE_API void UnregisterOnMontageChanged(FDelegateUserObject Unregister);
#endif	//WITH_EDITOR

private:

	/** Convert all branching points to AnimNotifies */
	/** 将所有分支点转换为 AnimNotify */
	void ConvertBranchingPointsToAnimNotifies();
	/** Recreate BranchingPoint markers from AnimNotifies marked 'BranchingPoints' */
	/** 从 AnimNotify 中重新创建标记为“BranchingPoints”的 BranchingPoint 标记 */
	void RefreshBranchingPointMarkers();
	void AddBranchingPointMarker(FBranchingPointMarker Marker, TMap<float, FAnimNotifyEvent*>& TriggerTimes);
	
	/** Cached list of Branching Point markers */
	/** 分支点标记的缓存列表 */
	UPROPERTY()
	TArray<FBranchingPointMarker> BranchingPointMarkers;
public:

	/** Keep track of which AnimNotify_State are marked as BranchingPoints, so we can update their state when the Montage is ticked */
	/** 跟踪哪些 AnimNotify_State 被标记为 BranchingPoints，以便我们可以在勾选 Montage 时更新它们的状态 */
	UPROPERTY()
	TArray<int32> BranchingPointStateNotifyIndices;

	/** Find first branching point marker between track positions */
	/** 找到轨道位置之间的第一个分支点标记 */
	const FBranchingPointMarker* FindFirstBranchingPointMarker(float StartTrackPos, float EndTrackPos) const;
	
	/** Filter out notifies from array that are marked as 'BranchingPoints' */
	/** 从数组中过滤掉标记为“BranchingPoints”的通知 */
	UE_DEPRECATED(4.19, "Use the GetAnimNotifiesFromTrackPositions that takes FAnimNotifyEventReferences instead")
	void FilterOutNotifyBranchingPoints(TArray<const FAnimNotifyEvent*>& InAnimNotifies);

	/** Filter out notifies from array that are marked as 'BranchingPoints' */
	/** 从数组中过滤掉标记为“BranchingPoints”的通知 */
	void FilterOutNotifyBranchingPoints(TArray<FAnimNotifyEventReference>& InAnimNotifies);

	bool CanUseMarkerSync() const { return MarkerData.AuthoredSyncMarkers.Num() > 0; }

	// update markers
	// 更新标记
	void CollectMarkers();

	//~Begin UAnimCompositeBase Interface
	//~开始 UAnimCompositeBase 接口
	virtual void InvalidateRecursiveAsset() override;
	virtual bool ContainRecursive(TArray<UAnimCompositeBase*>& CurrentAccumulatedList) override;
	virtual void SetCompositeLength(float InLength) override;
#if WITH_EDITOR
	virtual void PopulateWithExistingModel(TScriptInterface<IAnimationDataModel> ExistingDataModel) override;
#endif // WITH_EDITOR
	//~End UAnimCompositeBase Interface
	//~UAnimCompositeBase 接口结束

	/** Utility function to create dynamic montage from AnimSequence */
	/** 从 AnimSequence 创建动态蒙太奇的实用函数 */
	ENGINE_API static UAnimMontage* CreateSlotAnimationAsDynamicMontage(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime = 0.25f, float BlendOutTime = 0.25f, float InPlayRate = 1.f, int32 LoopCount = 1, float BlendOutTriggerTime = -1.f, float InTimeToStartMontageAt = 0.f);

	/** Utility function to create dynamic montage from AnimSequence with blend in settings */
	/** 通过混合设置从 AnimSequence 创建动态蒙太奇的实用函数 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	static ENGINE_API UAnimMontage* CreateSlotAnimationAsDynamicMontage_WithBlendSettings(UAnimSequenceBase* Asset, FName SlotNodeName, const FMontageBlendSettings& BlendInSettings, const FMontageBlendSettings& BlendOutSettings, float InPlayRate = 1.f, int32 LoopCount = 1, float InBlendOutTriggerTime = -1.f);

	/** Utility function to create dynamic montage from AnimSequence */
	/** [翻译失败: Utility function to create dynamic montage from AnimSequence] */
	ENGINE_API static UAnimMontage* CreateSlotAnimationAsDynamicMontage_WithFractionalLoops(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime = 0.25f, float BlendOutTime = 0.25f, float LoopCount = 1.0f, float BlendOutTriggerTime = -1.f);

	//~Begin Time Stretch Curve
	//[翻译失败: ~Begin Time Stretch Curve]
public:

	/** Time stretch curve will only be used when the montage has a non-default play rate  */
	/** 仅当蒙太奇具有非默认播放速率时才会使用时间拉伸曲线  */
	UPROPERTY(EditAnywhere, Category = TimeStretchCurve)
	FTimeStretchCurve TimeStretchCurve;

	/** Name of optional TimeStretchCurveName to look for in Montage. Time stretch curve will only be used when the montage has a non-default play rate */
	/** 要在蒙太奇中查找的可选 TimeStretchCurveName 的名称。仅当蒙太奇具有非默认播放速率时才会使用时间拉伸曲线 */
	UPROPERTY(EditAnywhere, Category = TimeStretchCurve)
	FName TimeStretchCurveName;

private:
#if WITH_EDITOR
	void BakeTimeStretchCurve();
	//~End Time Stretch Curve
	//~结束时间拉伸曲线
	virtual void UpdateCommonTargetFrameRate() override;
#endif // WITH_EDITOR
};
