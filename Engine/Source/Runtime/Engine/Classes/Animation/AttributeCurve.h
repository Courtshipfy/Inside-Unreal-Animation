// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/KeyHandle.h"
#include "Curves/IndexedCurve.h"
#include "Serialization/Archive.h"

#include "Animation/WrappedAttribute.h"
#include "Animation/IAttributeBlendOperator.h"

#include "AttributeCurve.generated.h"

namespace UE { namespace Anim { class IAttributeBlendOperator; struct Attributes; } }
namespace UE::UAF { class FDecompressionTools; }

typedef UE::Anim::TWrappedAttribute<FDefaultAllocator> FWrappedAttribute;

USTRUCT()
struct FAttributeKey
{
	GENERATED_USTRUCT_BODY()
public:
	 
	FAttributeKey(float InTime = 0.f) : Time(InTime) {}

	/** The keyed time */
	/** 键控时间 */
	UPROPERTY(EditAnywhere, Category = Key)
	float Time;

	template<typename AttributeType>
	const AttributeType& GetValue() const
	{
		return Value.GetRef<AttributeType>();
	}

	template<typename AttributeType>
	const AttributeType* GetValuePtr() const
	{
		return Value.GetPtr<AttributeType>();
	}

	
	friend FArchive& operator<<(FArchive& Ar, FAttributeKey& P)
	{
		Ar << P.Time;
		return Ar;
	}

protected:
	/** Value for this key, populated by FAttributeCurve during serialization */
	/** 该键的值，在序列化期间由 FAttributeCurve 填充 */
	FWrappedAttribute Value;

	friend struct FAttributeCurve;
};

USTRUCT(BlueprintType)
struct FAttributeCurve : public FIndexedCurve
{
	GENERATED_USTRUCT_BODY()
public:
	FAttributeCurve() : ScriptStruct(nullptr), bShouldInterpolate(false), Operator(nullptr) {}
	FAttributeCurve(UScriptStruct* InScriptStruct) : ScriptStructPath(InScriptStruct), ScriptStruct(InScriptStruct), bShouldInterpolate(false), Operator(nullptr) {}

	ENGINE_API FAttributeCurve(const FAttributeCurve& OtherCurve);

	/** Virtual destructor. */
	/** 虚拟析构函数。 */
	virtual ~FAttributeCurve() { }

	ENGINE_API bool Serialize(FArchive& Ar);

	/** Begin FIndexedCurve overrides */
	/** 开始 FIndexedCurve 覆盖 */
	virtual int32 GetNumKeys() const override final { return Keys.Num(); }
	virtual FAttributeCurve* Duplicate() const final { return new FAttributeCurve(*this); }
	ENGINE_API virtual void SetKeyTime(FKeyHandle KeyHandle, float NewTime) override final;
	ENGINE_API virtual float GetKeyTime(FKeyHandle KeyHandle) const override final;
	/** End FIndexedCurve overrides */
	/** 结束 FIndexedCurve 覆盖 */
	
	/** Sets the underlying type for the curve, only possible when not containing any keys (see ::Reset) */
	/** 设置曲线的基础类型，仅在不包含任何关键点时才可能（请参阅::Reset） */
	ENGINE_API void SetScriptStruct(UScriptStruct* InScriptStruct);
	const UScriptStruct* GetScriptStruct() const { return ScriptStruct; }

	/** Whether or not the curve can be evaluated, based upon having a valid type and any keys */
	/** 是否可以根据有效类型和任何键来评估曲线 */
	ENGINE_API bool CanEvaluate() const;

	/** Evaluate the curve keys into a temporary value container */
	/** 将曲线关键点计算到临时值容器中 */
	template<typename AttributeType>
	AttributeType Evaluate(float Time) const
	{
		AttributeType EvaluatedValue;
		EvaluateToPtr(AttributeType::StaticStruct(), Time, (uint8*)&EvaluatedValue);
		return EvaluatedValue;
	}

	/** Check whether this curve has any data or not */
	/** 检查这条曲线是否有数据 */
	ENGINE_API bool HasAnyData() const;

	/** Removes all key data */
	/** 删除所有关键数据 */
	ENGINE_API void Reset();

	/** Const iterator for the keys, so the indices and handles stay valid */
	/** 键的常量迭代器，因此索引和句柄保持有效 */
	ENGINE_API TArray<FAttributeKey>::TConstIterator GetKeyIterator() const;

	/** Add a new typed key to the curve with the supplied Time and Value. */
	/** 使用提供的时间和值将新键入的键添加到曲线。 */
	template<typename AttributeType>
	FKeyHandle AddTypedKey(float InTime, const AttributeType& InValue, FKeyHandle InKeyHandle = FKeyHandle())
	{
		check(AttributeType::StaticStruct() == ScriptStruct); 
		return AddKey(InTime, &InValue, InKeyHandle);
	}

	/** Remove the specified key from the curve.*/
	/** 从曲线中删除指定的关键点。*/
	ENGINE_API void DeleteKey(FKeyHandle KeyHandle);

	/** Finds the key at InTime, and updates its typed value. If it can't find the key within the KeyTimeTolerance, it adds one at that time */
	/** 在 InTime 中查找键，并更新其键入的值。如果在 KeyTimeTolerance 范围内找不到该密钥，则会在那时添加一个 */
	template<typename AttributeType>
	FKeyHandle UpdateOrAddTypedKey(float InTime, const AttributeType& InValue, float KeyTimeTolerance = UE_KINDA_SMALL_NUMBER)
	{
		check(AttributeType::StaticStruct() == ScriptStruct);
		return UpdateOrAddKey(InTime, &InValue, KeyTimeTolerance);
	}

	/** Finds the key at InTime, and updates its typed value. If it can't find the key within the KeyTimeTolerance, it adds one at that time */
	/** 在 InTime 中查找键，并更新其键入的值。如果在 KeyTimeTolerance 范围内找不到该密钥，则会在那时添加一个 */
	FKeyHandle UpdateOrAddTypedKey(float InTime, const void* InValue, const UScriptStruct* ValueType, float KeyTimeTolerance = UE_KINDA_SMALL_NUMBER)
	{
		check(ValueType == ScriptStruct);
		return UpdateOrAddKey(InTime, InValue, KeyTimeTolerance);
	}
			
	/** Functions for getting keys based on handles */
	/** 根据句柄获取键的函数 */
	ENGINE_API FAttributeKey& GetKey(FKeyHandle KeyHandle);
	ENGINE_API const FAttributeKey& GetKey(FKeyHandle KeyHandle) const;

	/** Finds the key at KeyTime and returns its handle. If it can't find the key within the KeyTimeTolerance, it will return an invalid handle */
	/** 在 KeyTime 处查找密钥并返回其句柄。如果在 KeyTimeTolerance 范围内找不到密钥，它将返回无效句柄 */
	ENGINE_API FKeyHandle FindKey(float KeyTime, float KeyTimeTolerance = UE_KINDA_SMALL_NUMBER) const;

	/** Gets the handle for the last key which is at or before the time requested.  If there are no keys at or before the requested time, an invalid handle is returned. */
	/** 获取在请求时间或之前的最后一个键的句柄。  如果在请求时间或之前没有键，则返回无效句柄。 */
	ENGINE_API FKeyHandle FindKeyBeforeOrAt(float KeyTime) const;

	/** Tries to reduce the number of keys required for accurate evaluation (zero error threshold) */
	/** 尝试减少准确评估所需的密钥数量（零错误阈值） */
	ENGINE_API void RemoveRedundantKeys();
	ENGINE_API void SetKeys(TArrayView<const float> InTimes, TArrayView<const void*> InValues);

	/** Populates OutKeys with typed value-ptrs */
	/** 使用类型化值指针填充 OutKeys */
	template<typename AttributeType>
	void GetTypedKeys(TArray<const AttributeType*>& OutKeys) const
	{
		for (const FAttributeKey& Key : Keys)
		{
			OutKeys.Add(Key.Value.GetPtr<AttributeType>());
		}
	}

	/** Return copy of contained key-data */
	/** 返回包含的关键数据的副本 */
	ENGINE_API TArray<FAttributeKey> GetCopyOfKeys() const;
	ENGINE_API const TArray<FAttributeKey>& GetConstRefOfKeys() const;

	/** Used for adjusting the internal key-data when owning object its playlength changes */
	/** 用于当拥有对象的播放长度发生变化时调整内部关键数据 */
	ENGINE_API void ReadjustTimeRange(float NewMinTimeRange, float NewMaxTimeRange, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime);

protected:
	/** Evaluate the curve keys into the provided memory (should be appropriatedly sized) */
	/** 将曲线键评估到提供的内存中（大小应适当） */
	ENGINE_API void EvaluateToPtr(const UScriptStruct* InScriptStruct, float Time, uint8* InOutDataPtr) const;

	/** Finds the key at InTime, and updates its typed value. If it can't find the key within the KeyTimeTolerance, it adds one at that time */
	/** 在 InTime 中查找键，并更新其键入的值。如果在 KeyTimeTolerance 范围内找不到该密钥，则会在那时添加一个 */
	ENGINE_API FKeyHandle UpdateOrAddKey(float InTime, const void* InValue, float KeyTimeTolerance = UE_KINDA_SMALL_NUMBER);

	/** Add a new raw memory key (should be appropriately sized) to the curve with the supplied Time and Value. */
	/** 使用提供的时间和值将新的原始内存键（大小应适当）添加到曲线中。 */
	ENGINE_API FKeyHandle AddKey(float InTime, const void* InValue, FKeyHandle InKeyHandle = FKeyHandle());
protected:
	/** The keys, ordered by time */
	/** 按键，按时间排序 */
	UPROPERTY(EditAnywhere, Category = "Custom Attributes")
	TArray<FAttributeKey> Keys;	

	/* Path to UScriptStruct to be loaded */
	/* 要加载的 UScriptStruct 的路径 */
	UPROPERTY(VisibleAnywhere, Category = "Custom Attributes")
	FSoftObjectPath ScriptStructPath;

	/* Transient UScriptStruct instance representing the underlying value type for the curve */
	/* 表示曲线基础值类型的瞬态 UScriptStruct 实例 */
	UPROPERTY(EditAnywhere, Transient, Category = "Custom Attributes")
	TObjectPtr<UScriptStruct> ScriptStruct;

	/** Whether or not to interpolate between keys of ScripStruct type */
	/** 是否在 ScripStruct 类型的键之间进行插值 */
	UPROPERTY(EditAnywhere, Transient, Category = "Custom Attributes")
	bool bShouldInterpolate;

	/** Operator instanced used for interpolating between keys */
	/** 用于在键之间进行插值的实例化运算符 */
	const UE::Anim::IAttributeBlendOperator* Operator;

	friend class UAnimSequence;
	friend struct UE::Anim::Attributes;
	friend UE::UAF::FDecompressionTools;
};

template<>
struct TStructOpsTypeTraits<FAttributeCurve> : public TStructOpsTypeTraitsBase2<FAttributeCurve>
{
	enum
	{
		WithSerializer = true,
	};
};
