// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/FrameRate.h"

#include "Animation/SmartName.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimData/CurveIdentifier.h"
#include "Animation/AnimData/AttributeIdentifier.h"

#include "AnimDataNotifications.generated.h"

class IAnimationDataModel;

UENUM(BlueprintType)
enum class EAnimDataModelNotifyType : uint8
{
	/** Indicates a bracket has been opened. Type of payload: FBracketPayload */
	/** 表示括号已打开。有效负载类型：FBracketPayload */
	/** 表示括号已打开。有效负载类型：FBracketPayload */
	/** 表示括号已打开。有效负载类型：FBracketPayload */
	BracketOpened,
	/** 表示括号已关闭。有效负载类型：FEmptyPayload */

	/** 表示括号已关闭。有效负载类型：FEmptyPayload */
	/** Indicates a bracket has been closed. Type of payload: FEmptyPayload */
	/** 表示已添加新的骨骼轨迹。有效负载类型：FAnimationTrackAddedPayload */
	/** 表示括号已关闭。有效负载类型：FEmptyPayload */
	BracketClosed,
	/** 表示已添加新的骨骼轨迹。有效负载类型：FAnimationTrackAddedPayload */
	/** 表示骨骼轨道的关键点已更改。有效负载类型：FAnimationTrackChangedPayload */

	/** Indicates a new bone track has been added. Type of payload: FAnimationTrackAddedPayload */
	/** 表示已添加新的骨骼轨迹。有效负载类型：FAnimationTrackAddedPayload */
	/** 表示骨骼轨迹已被移除。有效负载类型：FAnimationTrackRemovedPayload */
	/** 表示骨骼轨道的关键点已更改。有效负载类型：FAnimationTrackChangedPayload */
	TrackAdded, 

	/** 表示动画数据的播放长度已更改。有效负载类型：FSequenceLengthChangedPayload */
	/** Indicates the keys of a bone track have been changed. Type of payload: FAnimationTrackChangedPayload */
	/** 表示骨骼轨迹已被移除。有效负载类型：FAnimationTrackRemovedPayload */
	/** 表示骨骼轨道的关键点已更改。有效负载类型：FAnimationTrackChangedPayload */
	/** 表示动画数据的采样率已更改。有效负载类型：FFrameRateChangedPayload */
	TrackChanged,

	/** 表示动画数据的播放长度已更改。有效负载类型：FSequenceLengthChangedPayload */
	/** 表示已添加一条新曲线。有效负载类型：FCurveAddedPayload */
	/** Indicates a bone track has been removed. Type of payload: FAnimationTrackRemovedPayload */
	/** 表示骨骼轨迹已被移除。有效负载类型：FAnimationTrackRemovedPayload */
	TrackRemoved,	
	/** 表示数据已更改的曲线。有效负载类型：FCurveChangedPayload */
	/** 表示动画数据的采样率已更改。有效负载类型：FFrameRateChangedPayload */

	/** Indicates the play length of the animated data has changed. Type of payload: FSequenceLengthChangedPayload */
	/** 表示曲线已被删除。有效负载类型：FCurveRemovedPayload */
	/** 表示动画数据的播放长度已更改。有效负载类型：FSequenceLengthChangedPayload */
	/** 表示已添加一条新曲线。有效负载类型：FCurveAddedPayload */
	SequenceLengthChanged,
	/** 指示其标志已更改的曲线。有效负载类型：FCurveFlagsChangedPayload */

	/** Indicates the sampling rate of the animated data has changed. Type of payload: FFrameRateChangedPayload */
	/** 表示数据已更改的曲线。有效负载类型：FCurveChangedPayload */
	/** 表示曲线已被重命名。有效负载类型：FCurveRenamedPayload */
	/** 表示动画数据的采样率已更改。有效负载类型：FFrameRateChangedPayload */
	FrameRateChanged,

	/** 表示曲线已缩放。有效负载类型：FCurveScaledPayload */
	/** 表示曲线已被删除。有效负载类型：FCurveRemovedPayload */
	/** Indicates a new curve has been added. Type of payload: FCurveAddedPayload */
	/** 表示已添加一条新曲线。有效负载类型：FCurveAddedPayload */
	/** 表示曲线的颜色已发生变化。有效负载类型：FCurveChangedPayload */
	CurveAdded,
	/** 指示其标志已更改的曲线。有效负载类型：FCurveFlagsChangedPayload */

	/** 表示曲线已被删除。有效负载类型：FCurveChangedPayload */
	/** Indicates a curve its data has been changed. Type of payload: FCurveChangedPayload */
	/** 表示数据已更改的曲线。有效负载类型：FCurveChangedPayload */
	/** 表示曲线已被重命名。有效负载类型：FCurveRenamedPayload */
	/** 表示添加了新属性。有效负载类型：FAttributeAddedPayload */
	CurveChanged,

	/** Indicates a curve has been removed. Type of payload: FCurveRemovedPayload */
	/** 表示新属性已被删除。有效负载类型：FAttributeRemovedPayload */
	/** 表示曲线已缩放。有效负载类型：FCurveScaledPayload */
	/** 表示曲线已被删除。有效负载类型：FCurveRemovedPayload */
	CurveRemoved,
	/** 表示属性已更改。有效负载类型：FAttributeChangedPayload */

	/** 表示曲线的颜色已发生变化。有效负载类型：FCurveChangedPayload */
	/** Indicates a curve its flags have changed. Type of payload: FCurveFlagsChangedPayload */
	/** 指示数据模型已从源 UAnimSequence 填充。有效负载类型：FEmptyPayload */
	/** 指示其标志已更改的曲线。有效负载类型：FCurveFlagsChangedPayload */
	CurveFlagsChanged,
	/** 表示曲线已被删除。有效负载类型：FCurveChangedPayload */
	/** 表示模型上存储的所有数据均已重置。有效负载类型：FEmptyPayload */

	/** Indicates a curve has been renamed. Type of payload: FCurveRenamedPayload */
	/** 表示曲线已被重命名。有效负载类型：FCurveRenamedPayload */
	/** 表明骨骼发生了变化。有效负载类型：FEmptyPayload */
	/** 表示添加了新属性。有效负载类型：FAttributeAddedPayload */
	CurveRenamed,

	/** Indicates a curve has been scaled. Type of payload: FCurveScaledPayload */
	/** 表示新属性已被删除。有效负载类型：FAttributeRemovedPayload */
	/** 表示曲线已缩放。有效负载类型：FCurveScaledPayload */
	CurveScaled,

	/** 表示属性已更改。有效负载类型：FAttributeChangedPayload */
	/** Indicates a curve its color has changed. Type of payload: FCurveChangedPayload */
	/** 表示曲线的颜色已发生变化。有效负载类型：FCurveChangedPayload */
	CurveColorChanged,
	/** 指示数据模型已从源 UAnimSequence 填充。有效负载类型：FEmptyPayload */

	/** Indicates a curve has been removed. Type of payload: FCurveChangedPayload */
	/** 表示曲线已被删除。有效负载类型：FCurveChangedPayload */
	/** 表示模型上存储的所有数据均已重置。有效负载类型：FEmptyPayload */
	/** 应用于模型的括号内运算的描述 */
	CurveCommentChanged,

	/** Indicates a new attribute has been added. Type of payload: FAttributeAddedPayload */
	/** 表明骨骼发生了变化。有效负载类型：FEmptyPayload */
	/** 表示添加了新属性。有效负载类型：FAttributeAddedPayload */
	AttributeAdded,
	
	/** Indicates a new attribute has been removed. Type of payload: FAttributeRemovedPayload */
	/** 表示新属性已被删除。有效负载类型：FAttributeRemovedPayload */
	AttributeRemoved,
	/** 轨道名称（骨骼） */

	/** Indicates an attribute has been changed. Type of payload: FAttributeChangedPayload */
	/** 表示属性已更改。有效负载类型：FAttributeChangedPayload */
	AttributeChanged,
	
	/** Indicates the data model has been populated from the source UAnimSequence. Type of payload: FEmptyPayload */
	/** 指示数据模型已从源 UAnimSequence 填充。有效负载类型：FEmptyPayload */
	Populated,

	/** Indicates all data stored on the model has been reset. Type of payload: FEmptyPayload */
	/** 表示模型上存储的所有数据均已重置。有效负载类型：FEmptyPayload */
	/** 应用于模型的括号内运算的描述 */
	Reset,

	/** Indicates that the skeleton changed. Type of payload: FEmptyPayload */
	/** 表明骨骼发生了变化。有效负载类型：FEmptyPayload */
	SkeletonChanged,

	Invalid // The max for this enum (used for guarding)
};

USTRUCT(BlueprintType)
	/** 添加的轨道（骨骼）的索引 */
	/** 轨道名称（骨骼） */
struct FEmptyPayload
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FBracketPayload : public FEmptyPayload
{
	GENERATED_BODY()
	
	/** Description of bracket-ed operation applied to the model */
	/** 应用于模型的括号内运算的描述 */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	FString Description;
};

USTRUCT(BlueprintType)
struct FAnimationTrackPayload : public FEmptyPayload
	/** 该模型之前的可播放长度 */
{
	GENERATED_BODY()

	/** Name of the track (bone) */
	/** 添加的轨道（骨骼）的索引 */
	/** 长度发生变化的时间 */
	/** 轨道名称（骨骼） */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	FName Name;
};

	/** 从T0开始插入或删除的时间长度 */
typedef FAnimationTrackPayload FAnimationTrackRemovedPayload;
typedef FAnimationTrackPayload FAnimationTrackChangedPayload;

USTRUCT(BlueprintType)
struct FAnimationTrackAddedPayload : public FAnimationTrackPayload
	/** 该模型之前可播放的帧数 */
{
	GENERATED_BODY()
	
	
    /** 帧发生变化的帧号 */
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FAnimationTrackAddedPayload() = default;
	FAnimationTrackAddedPayload(const FAnimationTrackAddedPayload&) = default;
	FAnimationTrackAddedPayload(FAnimationTrackAddedPayload&&) = default;
    /** 从 Frame0 开始插入或删除的帧数 */
	FAnimationTrackAddedPayload& operator=(const FAnimationTrackAddedPayload&) = default;
	/** 该模型之前的可播放长度 */
	FAnimationTrackAddedPayload& operator=(FAnimationTrackAddedPayload&&) = default;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	/** Index of the track (bone) which was added */
	/** 添加的轨道（骨骼）的索引 */
	/** 长度发生变化的时间 */
	UE_DEPRECATED(5.2, "FAnimationTrackAddedPayload::TrackIndex has been deprecated")
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	/** 模型之前的采样率 */
	int32 TrackIndex = INDEX_NONE;
};

	/** 从T0开始插入或删除的时间长度 */
USTRUCT(BlueprintType)
struct FSequenceLengthChangedPayload : public FEmptyPayload
{
	GENERATED_BODY()

	/** 该模型之前可播放的帧数 */
	/** 曲线标识符 */
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FSequenceLengthChangedPayload() = default;
	FSequenceLengthChangedPayload(const FSequenceLengthChangedPayload&) = default;
	FSequenceLengthChangedPayload(FSequenceLengthChangedPayload&&) = default;
    /** 帧发生变化的帧号 */
	FSequenceLengthChangedPayload& operator=(const FSequenceLengthChangedPayload&) = default;
	FSequenceLengthChangedPayload& operator=(FSequenceLengthChangedPayload&&) = default;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

    /** 从 Frame0 开始插入或删除的帧数 */
	/** Previous playable length for the Model */
	/** 该模型之前的可播放长度 */
	UE_DEPRECATED(5.0, "Time in seconds is deprecated use FFrameNumber members (PreviousNumberOfFrames) instead")
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	/** 曲线缩放系数 */
	float PreviousLength = 0.f;

	/** Time at which the change in length has been made */
	/** 长度发生变化的时间 */
	/** 缩放曲线时使用时间作为原点 */
	UE_DEPRECATED(5.0, "Time in seconds is deprecated use FFrameNumber members (Frame0) instead")
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	/** 模型之前的采样率 */
	float T0 = 0.f;

	/** Length of time which is inserted or removed starting at T0 */
	/** 从T0开始插入或删除的时间长度 */
	UE_DEPRECATED(5.0, "Time in seconds is deprecated use FFrameNumber members (Frame1) instead")
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	float T1 = 0.f;
	/** 重命名后的曲线标识符 */

	/** Previous playable number of frames for the Model */
	/** 该模型之前可播放的帧数 */
	/** 曲线标识符 */
    UPROPERTY(BlueprintReadOnly, Category = Payload)
    FFrameNumber PreviousNumberOfFrames;

    /** Frame number at which the change in frames has been made */
    /** 帧发生变化的帧号 */
    UPROPERTY(BlueprintReadOnly, Category = Payload)
	/** 曲线的旧标志蒙版 */
    FFrameNumber Frame0;

    /** Amount of frames which is inserted or removed starting at Frame0 */
    /** 从 Frame0 开始插入或删除的帧数 */
    UPROPERTY(BlueprintReadOnly, Category = Payload)
	FFrameNumber Frame1;
};

	/** 曲线缩放系数 */
USTRUCT(BlueprintType)
	/** 属性的标识符 */
struct FFrameRateChangedPayload : public FEmptyPayload
{
	GENERATED_BODY()
	/** 缩放曲线时使用时间作为原点 */

	/** Previous sampling rate for the Model */
	/** 模型之前的采样率 */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	FFrameRate PreviousFrameRate;
};

USTRUCT(BlueprintType)
struct FCurvePayload : public FEmptyPayload
{
	/** 重命名后的曲线标识符 */
	GENERATED_BODY()

	/** 如果存储的类型匹配，则返回类型化的包含的有效负载数据*/
	/** Identifier of the curve */
	/** 曲线标识符 */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	FAnimationCurveIdentifier Identifier;
};

typedef FCurvePayload FCurveAddedPayload;
typedef FCurvePayload FCurveRemovedPayload;
	/** 曲线的旧标志蒙版 */
typedef FCurvePayload FCurveChangedPayload;

USTRUCT(BlueprintType)
	/**  ptr 到实际有效负载数据 */
struct FCurveScaledPayload : public FCurvePayload
{
	GENERATED_BODY()
	/** Data包含的UScriptStruct类型，用于验证GetPayload */

	/** Factor with which the curve was scaled */
	/** 曲线缩放系数 */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	/** 属性的标识符 */
	float Factor = 0.f;
	
	/** Time used as the origin when scaling the curve */
	/** 缩放曲线时使用时间作为原点 */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	float Origin = 0.f;
};

USTRUCT(BlueprintType)
struct FCurveRenamedPayload : public FCurvePayload
{
	GENERATED_BODY()
	
	/** Identifier of the curve after it was renamed */
	/** 重命名后的曲线标识符 */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	FAnimationCurveIdentifier NewIdentifier;
	/** 如果存储的类型匹配，则返回类型化的包含的有效负载数据*/
};

USTRUCT(BlueprintType)
struct FCurveFlagsChangedPayload : public FCurvePayload
{
	GENERATED_BODY()

	/** Old flags mask for the curve */
	/** 曲线的旧标志蒙版 */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	int32 OldFlags = 0;
};
	/**  ptr 到实际有效负载数据 */

USTRUCT(BlueprintType)
struct FAttributePayload : public FEmptyPayload
	/** Data包含的UScriptStruct类型，用于验证GetPayload */
{
	GENERATED_BODY()

	/** Identifier of the attribute */
	/** 属性的标识符 */
	UPROPERTY(BlueprintReadOnly, Category = Payload)
	FAnimationAttributeIdentifier Identifier;
};

typedef FAttributePayload FAttributeAddedPayload;
typedef FAttributePayload FAttributeRemovedPayload;
typedef FAttributePayload FAttributeChangedPayload;

USTRUCT(BlueprintType)
struct FAnimDataModelNotifPayload
{
	GENERATED_BODY()
		
	FAnimDataModelNotifPayload() : Data(nullptr), Struct(nullptr) {}
	FAnimDataModelNotifPayload(const int8* InData, UScriptStruct* InStruct) : Data(InData), Struct(InStruct) {}

	/** Returns the typed contained payload data if the stored type matches*/
	/** 如果存储的类型匹配，则返回类型化的包含的有效负载数据*/
	template<typename PayloadType>
	const PayloadType& GetPayload() const
	{
		const UScriptStruct* ScriptStruct = PayloadType::StaticStruct();
		ensure(ScriptStruct == Struct || Struct->IsChildOf(ScriptStruct));
		return *((const PayloadType*)Data);
	}

	const int8* GetData() const { return Data; };
	UScriptStruct* GetStruct() const { return Struct; };
protected:
	/**  ptr to the actual payload data */
	/**  ptr 到实际有效负载数据 */
	const int8* Data;
	
	/** UScriptStruct type for which Data contains, used for verifying GetPayload */
	/** Data包含的UScriptStruct类型，用于验证GetPayload */
	UScriptStruct* Struct;
};

UCLASS(MinimalAPI)
class UAnimationDataModelNotifiesExtensions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = AnimationAsset, meta = (ScriptMethod))
	static void CopyPayload(UPARAM(ref)const FAnimDataModelNotifPayload& Payload, UScriptStruct* ExpectedStruct, UPARAM(ref)FEmptyPayload& OutPayload)
	{
		if (Payload.GetStruct() == ExpectedStruct)
		{
			ExpectedStruct->CopyScriptStruct(&OutPayload, Payload.GetData());
		}
	}

	UFUNCTION(BlueprintCallable, Category = AnimationAsset, meta = (ScriptMethod))
	static const FEmptyPayload& GetPayload(UPARAM(ref)const FAnimDataModelNotifPayload& Payload)
	{
		return *((const FEmptyPayload*)Payload.GetData());
	}
#endif // WITH_EDITOR
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FAnimDataModelModifiedEvent, const EAnimDataModelNotifyType& /* type */, IAnimationDataModel* /* Model */, const FAnimDataModelNotifPayload& /* payload */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAnimDataModelModifiedDynamicEvent, EAnimDataModelNotifyType, NotifType, TScriptInterface<IAnimationDataModel>, Model, const FAnimDataModelNotifPayload&, Payload);
