// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/ObjectMacros.h"
#include "UObject/NameTypes.h"
#include "Misc/Variant.h"
#include "Animation/AnimTypes.h"
#include "BoneIndices.h"
#include "Templates/Tuple.h"
#include "Curves/StringCurve.h"
#include "Curves/IntegralCurve.h"
#include "Curves/SimpleCurve.h"

#include "CustomAttributes.generated.h"

UENUM(Experimental)
enum class ECustomAttributeBlendType : uint8
{
	/** Overrides Custom attributes according to highest weighted pose */
	/** 根据最高权重姿势覆盖自定义属性 */
	Override,
	/** Blends Custom attributes according to weights per pose */
	/** 根据每个姿势的权重混合自定义属性 */
	Blend
};

USTRUCT()
struct FCustomAttributeSetting
{
	GENERATED_BODY()

	/** Name of the custom attribute */
	/** 自定义属性的名称 */
	UPROPERTY(EditAnywhere, Category = CustomAttributeSetting)
	FString Name;

	/** Optional property describing the meaning (or role) of the custom attribute, allowing to add context to an attribute */
	/** 可选属性描述自定义属性的含义（或角色），允许向属性添加上下文 */
	UPROPERTY(EditAnywhere, Category = CustomAttributeSetting)
	FString Meaning;
};

/**
 * Settings that identify the names of custom attributes that represent the individual components of a timecode and a subframe along with a take name.
 */
USTRUCT()
struct FTimecodeCustomAttributeNameSettings
{
	GENERATED_BODY()

	/** Name of the custom attribute representing the hour component of a timecode. */
	/** 表示时间码的小时部分的自定义属性的名称。 */
	UPROPERTY(EditAnywhere, Category = TimecodeCustomAttributeNameSettings)
	FName HourAttributeName;

	/** Name of the custom attribute representing the minute component of a timecode. */
	/** 表示时间码分钟部分的自定义属性的名称。 */
	UPROPERTY(EditAnywhere, Category = TimecodeCustomAttributeNameSettings)
	FName MinuteAttributeName;

	/** Name of the custom attribute representing the second component of a timecode. */
	/** 表示时间码第二个组成部分的自定义属性的名称。 */
	UPROPERTY(EditAnywhere, Category = TimecodeCustomAttributeNameSettings)
	FName SecondAttributeName;

	/** Name of the custom attribute representing the frame component of a timecode. */
	/** 表示时间码的帧组件的自定义属性的名称。 */
	UPROPERTY(EditAnywhere, Category = TimecodeCustomAttributeNameSettings)
	FName FrameAttributeName;

	/** Name of the custom attribute representing a subframe value. Though not strictly a component
		of a timecode, this attribute can be authored to identify samples in between timecodes. */
	UPROPERTY(EditAnywhere, Category = TimecodeCustomAttributeNameSettings)
	FName SubframeAttributeName;

	/** Name of the custom attribute representing the timecode rate. This may be different from
	    the animation or capture frame rate, for example when capturing "high" frame rate data
		at 120 frames per second but recording SMPTE timecode at 30 frames per second. */
	UPROPERTY(EditAnywhere, Category = TimecodeCustomAttributeNameSettings)
	FName RateAttributeName;

	/** Name of the custom attribute representing the name of a take. */
	/** 代表镜头名称的自定义属性的名称。 */
	UPROPERTY(EditAnywhere, Category = TimecodeCustomAttributeNameSettings)
	FName TakenameAttributeName;
};

struct UE_DEPRECATED(5.0, "FCustomAttribute has been deprecated") FCustomAttribute;
USTRUCT(Experimental)
struct FCustomAttribute
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	/** Name of this attribute */
	/** 该属性的名称 */
	UPROPERTY(VisibleAnywhere, Category = CustomAttribute)
	FName Name;

	/** (FVariant) type contained by Values array */
	/** Values 数组包含的 (FVariant) 类型 */
	UPROPERTY(VisibleAnywhere, Category = CustomAttribute)
	int32 VariantType = 0;

	/** Time keys (should match number of Value entries) */
	/** 时间键（应与值条目的数量匹配） */
	UPROPERTY(VisibleAnywhere, Category = CustomAttributeBoneData)
	TArray<float> Times;
		
	/** Value keys (should match number of Times entries) */
	/** 值键（应与 Times 条目的数量匹配） */
	TArray<FVariant> Values;

	bool Serialize(FArchive& Ar)
	{
		Ar << Name;
		Ar << VariantType;
		Ar << Times;
		Ar << Values;

		return true;
	}
#endif // WITH_EDITORONLY_DATA
};

#if WITH_EDITORONLY_DATA
PRAGMA_DISABLE_DEPRECATION_WARNINGS
// Custom serializer required for FVariant array
// FVariant 数组需要自定义序列化器
template<>
struct TStructOpsTypeTraits<FCustomAttribute> : public TStructOpsTypeTraitsBase2<FCustomAttribute>
{
	enum
	{
		WithSerializer = true
	};
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif // WITH_EDITORONLY_DATA

/** Structure describing custom attributes for a single bone (index) */
/** 描述单个骨骼的自定义属性的结构（索引） */
struct UE_DEPRECATED(5.0, "FCustomAttributePerBoneData has been deprecated") FCustomAttributePerBoneData;

USTRUCT(Experimental)
struct FCustomAttributePerBoneData
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = CustomAttributeBoneData)
	int32 BoneTreeIndex = 0;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UPROPERTY(VisibleAnywhere, EditFixedSize, Category = CustomAttributeBoneData)
	TArray<FCustomAttribute> Attributes;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif // WITH_EDITORONLY_DATA
};

/** (Baked) string custom attribute, uses FStringCurve for evaluation instead of FVariant array */
/** （烘焙）字符串自定义属性，使用 FStringCurve 代替 FVariant 数组进行评估 */
struct UE_DEPRECATED(5.0, "FBakedStringCustomAttribute has been deprecated") FBakedStringCustomAttribute;

USTRUCT(Experimental)
struct FBakedStringCustomAttribute
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = CustomAttributeBoneData)
	FName AttributeName;

	UPROPERTY(VisibleAnywhere, Category = CustomAttributeBoneData)
	FStringCurve StringCurve;
#endif // WITH_EDITORONLY_DATA
};

/** (Baked) int32 custom attribute, uses FIntegralCurve for evaluation instead of FVariant array */
/** （烘焙）int32 自定义属性，使用 FIntegralCurve 代替 FVariant 数组进行评估 */
struct UE_DEPRECATED(5.0, "FBakedIntegerCustomAttribute has been deprecated") FBakedIntegerCustomAttribute;

USTRUCT(Experimental)
struct FBakedIntegerCustomAttribute
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = CustomAttributeBoneData)
	FName AttributeName;

	UPROPERTY(VisibleAnywhere, Category = CustomAttributeBoneData)
	FIntegralCurve IntCurve;
#endif // WITH_EDITORONLY_DATA
};

/** (Baked) float custom attribute, uses FSimpleCurve for evaluation instead of FVariant array */
/** （烘焙）浮动自定义属性，使用 FSimpleCurve 进行评估而不是 FVariant 数组 */
struct UE_DEPRECATED(5.0, "FBakedFloatCustomAttribute has been deprecated") FBakedFloatCustomAttribute;

USTRUCT(Experimental)
struct FBakedFloatCustomAttribute
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = CustomAttributeBoneData)
	FName AttributeName;

	UPROPERTY(VisibleAnywhere, Category = CustomAttributeBoneData)
	FSimpleCurve FloatCurve;
#endif // WITH_EDITORONLY_DATA
};

/** Structure describing baked custom attributes for a single bone (index) */
/** 描述单个骨骼的烘焙自定义属性的结构（索引） */
struct UE_DEPRECATED(5.0, "FBakedCustomAttributePerBoneData has been deprecated") FBakedCustomAttributePerBoneData;

USTRUCT(Experimental)
struct FBakedCustomAttributePerBoneData
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	int32 BoneTreeIndex = 0;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UPROPERTY(VisibleAnywhere, EditFixedSize, Category = CustomAttributeBoneData)
	TArray<FBakedStringCustomAttribute> StringAttributes;

	UPROPERTY(VisibleAnywhere, EditFixedSize, Category = CustomAttributeBoneData)
	TArray<FBakedIntegerCustomAttribute> IntAttributes;

	UPROPERTY(VisibleAnywhere, EditFixedSize, Category = CustomAttributeBoneData)
	TArray<FBakedFloatCustomAttribute> FloatAttributes;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif // WITH_EDITORONLY_DATA
};
