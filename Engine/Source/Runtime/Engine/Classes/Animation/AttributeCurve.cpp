// Copyright Epic Games, Inc. All Rights Reserved.

#include "AttributeCurve.h"

#include "Animation/AttributeTypes.h"
#include "Animation/IAttributeBlendOperator.h"
#include "Templates/Casts.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AttributeCurve)

FAttributeCurve::FAttributeCurve(const FAttributeCurve& OtherCurve)
{
	Keys = OtherCurve.Keys;
	ScriptStructPath = OtherCurve.ScriptStructPath;
	ScriptStruct = OtherCurve.ScriptStruct;
	Operator = OtherCurve.Operator;
	bShouldInterpolate = OtherCurve.bShouldInterpolate;
}

void FAttributeCurve::SetScriptStruct(UScriptStruct* InScriptStruct)
{
	if (InScriptStruct && ScriptStruct != InScriptStruct && (Keys.Num() == 0))
	{
		ScriptStruct = InScriptStruct;
		ScriptStructPath = InScriptStruct;

		Operator = UE::Anim::AttributeTypes::GetTypeOperator(ScriptStruct);
		ensure(Operator);
		bShouldInterpolate = UE::Anim::AttributeTypes::CanInterpolateType(ScriptStruct);
	}
}

bool FAttributeCurve::CanEvaluate() const
{
	return ScriptStruct != nullptr && Keys.Num() > 0;
}

void FAttributeCurve::SetKeyTime(FKeyHandle KeyHandle, float NewTime)
{
	if (IsKeyHandleValid(KeyHandle))
	{
		const FAttributeKey OldKey = GetKey(KeyHandle);

		DeleteKey(KeyHandle);
		AddKey(NewTime, (void*)OldKey.Value.GetPtr<void>(), KeyHandle);
		
		// Copy all properties from old key, but then fix time to be the new time
  // 复制旧密钥中的所有属性，然后将时间修复为新时间
		FAttributeKey& NewKey = GetKey(KeyHandle);
		NewKey = OldKey;
		NewKey.Time = NewTime;
	}
}

float FAttributeCurve::GetKeyTime(FKeyHandle KeyHandle) const
{
	if (!IsKeyHandleValid(KeyHandle))
	{
		return 0.f;
	}

	return GetKey(KeyHandle).Time;
}

void FAttributeCurve::EvaluateToPtr(const UScriptStruct* InScriptStruct, float Time, uint8* InOutDataPtr) const
{
	if (CanEvaluate() && InScriptStruct == ScriptStruct)
	{
		const void* DataPtr = Keys[0].Value.GetPtr<void>();

		if (bShouldInterpolate)
		{	
			const int32 NumKeys = Keys.Num();
			if (NumKeys == 0)
			{
				ensure(false);
				// If no keys in curve, return the Default value.
    // 如果曲线中没有关键点，则返回默认值。
			}
			else if (NumKeys < 2 || (Time <= Keys[0].Time))
			{
				DataPtr = Keys[0].Value.GetPtr<void>();
			}
			else if (Time < Keys[NumKeys - 1].Time)
			{
				// perform a lower bound to get the second of the interpolation nodes
    // 执行下界以获得第二个插值节点
				int32 first = 1;
				int32 last = NumKeys - 1;
				int32 count = last - first;

				while (count > 0)
				{
					int32 step = count / 2;
					int32 middle = first + step;

					if (Time >= Keys[middle].Time)
					{
						first = middle + 1;
						count -= step + 1;
					}
					else
					{
						count = step;
					}
				}

				const FAttributeKey& Key = Keys[first - 1];
				const FAttributeKey& Key1 = Keys[first];

				const float Diff = Key1.Time - Key.Time;
				if (Diff > 0.f)
				{
					const float Alpha = (Time - Key.Time) / Diff;
					Operator->Interpolate(Key.Value.GetPtr<void>(), Key1.Value.GetPtr<void>(), Alpha, InOutDataPtr);
					return;
				}
				else
				{
					DataPtr = Key.Value.GetPtr<void>();
				}
			}
			else
			{
				// Key is beyon the last point in the curve.  Return it's value
    // 关键点超出了曲线的最后一个点。  返回它的值
				DataPtr = Keys[Keys.Num() - 1].Value.GetPtr<void>();
			}
		}
		else
		{
			if (Keys.Num() == 0 || (Time < Keys[0].Time))
			{
				// If no keys in curve, or bUseDefaultValueBeforeFirstKey is set and the time is before the first key, return the Default value.
    // 如果曲线中没有关键点，或者设置了 bUseDefaultValueBeforeFirstKey 并且时间在第一个关键点之前，则返回默认值。
			}
			else if (Keys.Num() < 2 || Time < Keys[0].Time)
			{
				// There is only one key or the time is before the first value. Return the first value
    // 只有一个键或者时间早于第一个值。返回第一个值
				DataPtr = Keys[0].Value.GetPtr<void>();
			}
			else if (Time < Keys[Keys.Num() - 1].Time)
			{
				// The key is in the range of Key[0] to Keys[Keys.Num()-1].  Find it by searching
    // 密钥的范围是Key[0]到Keys[Keys.Num()-1]。  通过搜索找到它
				for (int32 i = 0; i < Keys.Num(); ++i)
				{
					if (Time < Keys[i].Time)
					{
						DataPtr = Keys[FMath::Max(0, i - 1)].Value.GetPtr<void>();
						break;
					}
				}
			}
			else
			{
				// Key is beyon the last point in the curve.  Return it's value
    // 关键点超出了曲线的最后一个点。  返回它的值
				DataPtr = Keys[Keys.Num() - 1].Value.GetPtr<void>();
			}
		}

		ScriptStruct->CopyScriptStruct(InOutDataPtr, DataPtr, 1);
	}
}

bool FAttributeCurve::HasAnyData() const
{
	return Keys.Num() != 0;
}

TArray<FAttributeKey>::TConstIterator FAttributeCurve::GetKeyIterator() const
{
	return Keys.CreateConstIterator();
}

FKeyHandle FAttributeCurve::AddKey(float InTime, const void* InValue, FKeyHandle InKeyHandle)
{
	int32 Index = 0;
	for (; Index < Keys.Num() && Keys[Index].Time < InTime; ++Index);

	FAttributeKey& NewKey = Keys.Insert_GetRef(FAttributeKey(InTime), Index);
	NewKey.Value.Allocate(ScriptStruct);
	ScriptStruct->CopyScriptStruct(NewKey.Value.GetPtr<void>(), InValue);
	

	KeyHandlesToIndices.Add(InKeyHandle, Index);

	return GetKeyHandle(Index);
}

void FAttributeCurve::DeleteKey(FKeyHandle InKeyHandle)
{
	int32 Index = GetIndex(InKeyHandle);

	Keys.RemoveAt(Index);

	KeyHandlesToIndices.Remove(InKeyHandle);
}

FKeyHandle FAttributeCurve::UpdateOrAddKey(float InTime, const void* InValue, float KeyTimeTolerance)
{
	for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
	{
		float KeyTime = Keys[KeyIndex].Time;

		if (FMath::IsNearlyEqual(KeyTime, InTime, KeyTimeTolerance))
		{
			ScriptStruct->CopyScriptStruct(Keys[KeyIndex].Value.GetPtr<void>(), InValue, 1);
			return GetKeyHandle(KeyIndex);
		}

		if (KeyTime > InTime)
		{
			// All the rest of the keys exist after the key we want to add
   // 所有其余的键都存在于我们要添加的键之后
			// so there is no point in searching
   // 所以没有必要去寻找
			break;
		}
	}

	// A key wasnt found, add it now
 // 未找到密钥，请立即添加
	return AddKey(InTime, InValue);
}

FAttributeKey& FAttributeCurve::GetKey(FKeyHandle KeyHandle)
{
	EnsureAllIndicesHaveHandles();
	return Keys[GetIndex(KeyHandle)];
}

const FAttributeKey& FAttributeCurve::GetKey(FKeyHandle KeyHandle) const
{
	EnsureAllIndicesHaveHandles();
	return Keys[GetIndex(KeyHandle)];
}

FKeyHandle FAttributeCurve::FindKey(float KeyTime, float KeyTimeTolerance) const
{
	int32 Start = 0;
	int32 End = Keys.Num() - 1;

	// Binary search since the keys are in sorted order
 // 由于键是按排序顺序进行二分搜索
	while (Start <= End)
	{
		int32 TestPos = Start + (End - Start) / 2;
		float TestKeyTime = Keys[TestPos].Time;

		if (FMath::IsNearlyEqual(TestKeyTime, KeyTime, KeyTimeTolerance))
		{
			return GetKeyHandle(TestPos);
		}
		else if (TestKeyTime < KeyTime)
		{
			Start = TestPos + 1;
		}
		else
		{
			End = TestPos - 1;
		}
	}

	return FKeyHandle::Invalid();
}

FKeyHandle FAttributeCurve::FindKeyBeforeOrAt(float KeyTime) const
{
	// If there are no keys or the time is before the first key return an invalid handle.
 // 如果没有键或者时间早于第一个键，则返回无效句柄。
	if (Keys.Num() == 0 || KeyTime < Keys[0].Time)
	{
		return FKeyHandle();
	}

	// If the time is after or at the last key return the last key.
 // 如果时间在最后一个键之后或之前，则返回最后一个键。
	if (KeyTime >= Keys[Keys.Num() - 1].Time)
	{
		return GetKeyHandle(Keys.Num() - 1);
	}

	// Otherwise binary search to find the handle of the nearest key at or before the time.
 // 否则，二分查找查找该时间或之前最近的键的句柄。
	int32 Start = 0;
	int32 End = Keys.Num() - 1;
	int32 FoundIndex = -1;
	while (FoundIndex < 0)
	{
		int32 TestPos = (Start + End) / 2;
		float TestKeyTime = Keys[TestPos].Time;
		float NextTestKeyTime = Keys[TestPos + 1].Time;
		if (TestKeyTime <= KeyTime)
		{
			if (NextTestKeyTime > KeyTime)
			{
				FoundIndex = TestPos;
			}
			else
			{
				Start = TestPos + 1;
			}
		}
		else
		{
			End = TestPos;
		}
	}
	return GetKeyHandle(FoundIndex);
}

void FAttributeCurve::RemoveRedundantKeys()
{
	TSet<int32> KeyIndicesToRemove;
	for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
	{
		if (KeyIndex + 2 < Keys.Num())
		{
			const FAttributeKey& CurrentKey = Keys[KeyIndex];
			const FAttributeKey& NextKeyOne = Keys[KeyIndex + 1];
			const FAttributeKey& NextKeyTwo = Keys[KeyIndex + 2];

			if (ScriptStruct->CompareScriptStruct(CurrentKey.Value.GetPtr<void>(), NextKeyOne.Value.GetPtr<void>(), 0)
				&& ScriptStruct->CompareScriptStruct(NextKeyOne.Value.GetPtr<void>(), NextKeyTwo.Value.GetPtr<void>(), 0))
			{
				KeyIndicesToRemove.Add(KeyIndex + 1);
			}
		}
	}

	if (KeyIndicesToRemove.Num())
	{
	    TArray<FAttributeKey> NewKeys;
		NewKeys.Reserve(Keys.Num() - KeyIndicesToRemove.Num());
	    for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
	    {
		    if (!KeyIndicesToRemove.Contains(KeyIndex))
		    {
			    NewKeys.Add(Keys[KeyIndex]);
		    }
	    }
    
	    Swap(Keys, NewKeys);
	    KeyHandlesToIndices.Empty(Keys.Num());
	    KeyHandlesToIndices.SetKeyHandles(Keys.Num());
	}

	// If only two keys left and they are identical as well, remove the 2nd one.
 // 如果只剩下两把钥匙并且它们也相同，则删除第二把。
	if (Keys.Num() == 2 && ScriptStruct->CompareScriptStruct(Keys[0].Value.GetPtr<void>(), Keys[1].Value.GetPtr<void>(), 0))
	{
		DeleteKey(GetKeyHandle(1));
	}
}

bool FAttributeCurve::Serialize(FArchive& Ar)
{
	Ar << Keys;
	Ar << ScriptStructPath;

	if (!ScriptStructPath.IsNull())
	{
		if (Ar.IsSaving())
		{
			ensure(ScriptStruct);
			for (FAttributeKey& Key : Keys)
			{
				ScriptStruct->SerializeItem(Ar, Key.Value.GetPtr<void>(), nullptr);
			}
		}
		else if (Ar.IsLoading())
		{
			ScriptStruct = Cast<UScriptStruct>(ScriptStructPath.ResolveObject());
			ensure(ScriptStruct);

			for (FAttributeKey& Key : Keys)
			{
				Key.Value.Allocate(ScriptStruct);
				ScriptStruct->SerializeItem(Ar, Key.Value.GetPtr<void>(), nullptr);
			}

			Operator = UE::Anim::AttributeTypes::GetTypeOperator(ScriptStruct);
			ensure(Operator);

			bShouldInterpolate = UE::Anim::AttributeTypes::CanInterpolateType(ScriptStruct);
		}
	}	

	return true;
}

void FAttributeCurve::Reset()
{
	Keys.Empty();
	KeyHandlesToIndices.Empty();
}

void FAttributeCurve::SetKeys(TArrayView<const float> InTimes, TArrayView<const void*> InValues)
{
	check(InTimes.Num() == InValues.Num());

	Reset();

	Keys.SetNum(InTimes.Num());
	KeyHandlesToIndices.SetKeyHandles(InTimes.Num());
	for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
	{
		Keys[KeyIndex].Time = InTimes[KeyIndex];
		Keys[KeyIndex].Value.Allocate(ScriptStruct);

		ScriptStruct->CopyScriptStruct(Keys[KeyIndex].Value.GetPtr<void>(), InValues[KeyIndex]);
	}
}

void FAttributeCurve::ReadjustTimeRange(float NewMinTimeRange, float NewMaxTimeRange, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime)
{
	EnsureAllIndicesHaveHandles();

	// first readjust modified time keys
 // 首先重新调整修改时间键
	float ModifiedDuration = OldEndTime - OldStartTime;

	if (bInsert)
	{
		for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
		{
			float& CurrentTime = Keys[KeyIndex].Time;
			if (CurrentTime >= OldStartTime)
			{
				CurrentTime += ModifiedDuration;
			}
		}
	}
	else
	{
		// since we only allow one key at a given time, we will just cache the value that needs to be saved
  // 由于我们在给定时间只允许一个键，因此我们将只缓存需要保存的值
		// this is the key to be replaced when this section is gone
  // 这是当该部分消失时要替换的关键
		bool bAddNewKey = false;
		FWrappedAttribute NewValue;
		NewValue.Allocate(ScriptStruct);

		TArray<int32> KeysToDelete;

		for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
		{
			float& CurrentTime = Keys[KeyIndex].Time;
			// if this key exists between range of deleted
   // 如果该键存在于删除范围之间
			// we'll evaluate the value at the "OldStartTime"
   // 我们将评估“OldStartTime”的值
			// and re-add key, so that it keeps the previous value at the
   // 并重新添加密钥，以便将之前的值保留在
			// start time
   // 开始时间
			// But that means if there are multiple keys, 
   // 但这意味着如果有多个键，
			// since we don't want multiple values in the same time
   // 因为我们不希望同时有多个值
			// the last one will override the value
   // 最后一个将覆盖该值
			if (CurrentTime >= OldStartTime && CurrentTime <= OldEndTime)
			{
				// get new value and add new key on one of OldStartTime, OldEndTime;
    // 获取新值并在 OldStartTime、OldEndTime 之一上添加新键；
				// this is a bit complicated problem since we don't know if OldStartTime or OldEndTime is preferred. 
    // 这是一个有点复杂的问题，因为我们不知道 OldStartTime 还是 OldEndTime 是首选。
				// generall we use OldEndTime unless OldStartTime == 0.f
    // 一般我们使用 OldEndTime 除非 OldStartTime == 0.f
				// which means it's cut in the beginning. Otherwise it will always use the end time. 
    // 这意味着它从一开始就被削减了。否则它将始终使用结束时间。
				bAddNewKey = true;
				if (OldStartTime != 0.f)
				{	
					ScriptStruct->InitializeDefaultValue(NewValue.GetPtr<uint8>());
					EvaluateToPtr(ScriptStruct, OldStartTime, NewValue.GetPtr<uint8>());
				}
				else
				{
					ScriptStruct->InitializeDefaultValue(NewValue.GetPtr<uint8>());
					EvaluateToPtr(ScriptStruct, OldEndTime, NewValue.GetPtr<uint8>());
				}
				// remove this key, but later because it might change eval result
    // 删除此键，但稍后删除，因为它可能会更改评估结果
				KeysToDelete.Add(KeyIndex);
			}
			else if (CurrentTime > OldEndTime)
			{
				CurrentTime -= ModifiedDuration;
			}
		}

		if (bAddNewKey)
		{
			for (int32 KeyIndex = KeysToDelete.Num() - 1; KeyIndex >= 0; --KeyIndex)
			{
				const FKeyHandle* KeyHandle = KeyHandlesToIndices.FindKey(KeysToDelete[KeyIndex]);
				if (KeyHandle)
				{
					DeleteKey(*KeyHandle);
				}

			}

			UpdateOrAddKey(OldStartTime, (void*)NewValue.GetPtr<void>());
		}
	}

	// now remove all redundant key
 // 现在删除所有多余的键
	RemoveRedundantKeys();

	// now cull out all out of range 
 // 现在剔除所有超出范围的
	float MinTime, MaxTime;

	if (Keys.Num() == 0)
	{
		MinTime = 0.f;
		MaxTime = 0.f;
	}
	else
	{
		MinTime = Keys[0].Time;
		MaxTime = Keys[Keys.Num() - 1].Time;
	}

	bool bNeedToDeleteKey = false;

	// if there is key below min time, just add key at new min range, 
 // 如果有低于最小时间的密钥，只需在新的最小范围内添加密钥，
	if (MinTime < NewMinTimeRange)
	{
		FWrappedAttribute NewValue;
		NewValue.Allocate(ScriptStruct);
		EvaluateToPtr(ScriptStruct, NewMinTimeRange, NewValue.GetPtr<uint8>());

		UpdateOrAddKey(NewMinTimeRange, (void*)NewValue.GetPtr<void>());

		bNeedToDeleteKey = true;
	}

	// if there is key after max time, just add key at new max range, 
 // 如果在最大时间之后还有密钥，只需在新的最大范围内添加密钥，
	if (MaxTime > NewMaxTimeRange)
	{
		FWrappedAttribute NewValue;
		NewValue.Allocate(ScriptStruct);
		EvaluateToPtr(ScriptStruct, NewMaxTimeRange, NewValue.GetPtr<uint8>());

		UpdateOrAddKey(NewMaxTimeRange, (void*)NewValue.GetPtr<void>());

		bNeedToDeleteKey = true;
	}

	// delete the keys outside of range
 // 删除超出范围的键
	if (bNeedToDeleteKey)
	{
		for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
		{
			if (Keys[KeyIndex].Time < NewMinTimeRange || Keys[KeyIndex].Time > NewMaxTimeRange)
			{
				const FKeyHandle* KeyHandle = KeyHandlesToIndices.FindKey(KeyIndex);
				if (KeyHandle)
				{
					DeleteKey(*KeyHandle);
					--KeyIndex;
				}
			}
		}
	}
}

TArray<FAttributeKey> FAttributeCurve::GetCopyOfKeys() const
{
	return Keys;
}

const TArray<FAttributeKey>& FAttributeCurve::GetConstRefOfKeys() const
{
	return Keys;
}
