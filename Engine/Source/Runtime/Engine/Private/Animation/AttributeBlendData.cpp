// Copyright Epic Games, Inc. All Rights Reserved.


#include "Animation/AttributeBlendData.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AttributesRuntime.h"

namespace UE { namespace Anim {

	FAttributeBlendData::FAttributeBlendData() : UniformWeight(0.f), AdditiveType(AAT_None) {}

	FAttributeBlendData::FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const float> SourceWeights, const UScriptStruct* AttributeScriptStruct) : UniformWeight(0.f), AdditiveType(AAT_None)
	{
		Weights = SourceWeights;
		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex, AttributeScriptStruct);
		}
	}

	FAttributeBlendData::FAttributeBlendData(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const TArrayView<const float> SourceWeights, const UScriptStruct* AttributeScriptStruct) : UniformWeight(0.f), AdditiveType(AAT_None)
	{
		Weights = SourceWeights;
		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = *SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex, AttributeScriptStruct);
		}
	}

	FAttributeBlendData::FAttributeBlendData(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const float InUniformWeight, const UScriptStruct* AttributeScriptStruct) : UniformWeight(InUniformWeight), AdditiveType(AAT_None)
	{
		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = *SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex, AttributeScriptStruct);
		}
	}

	FAttributeBlendData::FAttributeBlendData(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const float InUniformWeight, EAdditiveAnimationType InAdditiveType, const UScriptStruct* AttributeScriptStruct) : UniformWeight(InUniformWeight), AdditiveType(InAdditiveType)
	{
		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = *SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex, AttributeScriptStruct);
		}
	}

	FAttributeBlendData::FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const FPerBoneBlendWeight> InPerBoneBlendWeights, const UScriptStruct* AttributeScriptStruct) : UniformWeight(0.f), AdditiveType(AAT_None)
	{
		PerBoneWeights = InPerBoneBlendWeights;

		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex, AttributeScriptStruct);
		}

		HighestBoneWeightedIndices.SetNum(InPerBoneBlendWeights.Num());
		for (int32 BoneIndex = 0; BoneIndex < HighestBoneWeightedIndices.Num(); ++BoneIndex)
		{
			const FPerBoneBlendWeight& Weight = InPerBoneBlendWeights[BoneIndex];
			if (Weight.BlendWeight > .5f)
			{
				HighestBoneWeightedIndices[BoneIndex] = Weight.SourceIndex;
			}
			else
			{
				HighestBoneWeightedIndices[BoneIndex] = 0;
			}
		}
	}

	FAttributeBlendData::FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, TArrayView<const int32> InPerBoneInterpolationIndices, const TArrayView<const FBlendSampleData> InBlendSampleDataCache, const UScriptStruct* AttributeScriptStruct) : UniformWeight(0.f), AdditiveType(AAT_None)
	{
		PerBoneInterpolationIndices = InPerBoneInterpolationIndices;
		BlendSampleDataCache = InBlendSampleDataCache;

		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex, AttributeScriptStruct);
		}
	
		HighestBoneWeightedIndices.Reserve(InPerBoneInterpolationIndices.Num());
		for (int32 Index = 0; Index < InPerBoneInterpolationIndices.Num(); ++Index)
		{
			float Weight = -1.f;
			int32 HighestIndex = INDEX_NONE;
			for (int32 EntryIndex = 0; EntryIndex < SourceAttributes.Num(); ++EntryIndex)
			{
				const int32 BoneIndex = HighestBoneWeightedIndices.Num();
				const float BoneWeight = GetBoneWeight(EntryIndex, BoneIndex);
				if (BoneWeight > Weight)
				{
					Weight = BoneWeight;
					HighestIndex = EntryIndex;
				}
			}

			HighestBoneWeightedIndices.Add(HighestIndex);
		}
	}

	FAttributeBlendData::FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, TArrayView<const int32> InPerBoneInterpolationIndices, const TArrayView<const FBlendSampleData> InBlendSampleDataCache, TArrayView<const int32> InBlendSampleDataCacheIndices, const UScriptStruct* AttributeScriptStruct) : UniformWeight(0.f), AdditiveType(AAT_None)
	{
		PerBoneInterpolationIndices = InPerBoneInterpolationIndices;
		BlendSampleDataCache = InBlendSampleDataCache;
		WeightIndices = InBlendSampleDataCacheIndices;

		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex, AttributeScriptStruct);
		}

		HighestBoneWeightedIndices.Reserve(InPerBoneInterpolationIndices.Num());
		for (int32 Index = 0; Index < InPerBoneInterpolationIndices.Num(); ++Index)
		{
			float Weight = -1.f;
			int32 HighestIndex = INDEX_NONE;
			for (int32 EntryIndex = 0; EntryIndex < SourceAttributes.Num(); ++EntryIndex)
			{
				const int32 BoneIndex = HighestBoneWeightedIndices.Num();
				const float BoneWeight = GetBoneWeight(EntryIndex, BoneIndex);
				if (BoneWeight > Weight)
				{
					Weight = BoneWeight;
					HighestIndex = EntryIndex;
				}
			}

			HighestBoneWeightedIndices.Add(HighestIndex);
		}
	}

	FAttributeBlendData::FAttributeBlendData(const FStackAttributeContainer& BaseAttributes, const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const FPerBoneBlendWeight> InPerBoneBlendWeights, const UScriptStruct* AttributeScriptStruct) : UniformWeight(0.f), AdditiveType(AAT_None), bPerBoneFilter(true)
	{
		PerBoneWeights = InPerBoneBlendWeights;
		ProcessAttributes(BaseAttributes, 0, AttributeScriptStruct);

		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex + 1, AttributeScriptStruct);
		}

		HighestBoneWeightedIndices.Reserve(InPerBoneBlendWeights.Num());
		for (const FPerBoneBlendWeight& BoneWeight : InPerBoneBlendWeights)
		{
			// First input takes precedence with equal weighting
   // 第一个输入优先且权重相等
			if (BoneWeight.BlendWeight > 0.5f)
			{
				HighestBoneWeightedIndices.Add(BoneWeight.SourceIndex + 1);
			}
			else
			{
				HighestBoneWeightedIndices.Add(0);
			}
		}
	}

	FAttributeBlendData::FAttributeBlendData(const FStackAttributeContainer& SourceAttributes1, const FStackAttributeContainer& SourceAttributes2, const TArrayView<const float> WeightsOfSource2, const UScriptStruct* AttributeScriptStruct) : UniformWeight(0.f), AdditiveType(AAT_None)
	{
		Weights = WeightsOfSource2;

		HighestBoneWeightedIndices.Reserve(WeightsOfSource2.Num());
		for (const float& BoneWeight : WeightsOfSource2)
		{
			// First input takes precedence with equal weighting
   // 第一个输入优先且权重相等
			if (BoneWeight > 0.5f)
			{
				HighestBoneWeightedIndices.Add(1);
			}
			else
			{
				HighestBoneWeightedIndices.Add(0);
			}
		}

		ProcessAttributes(SourceAttributes1, 0, AttributeScriptStruct);
		ProcessAttributes(SourceAttributes2, 1, AttributeScriptStruct);
	}

	FAttributeBlendData::FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const float> SourceWeights, const TArrayView<const int32> SourceWeightsIndices, const UScriptStruct* AttributeScriptStruct) : UniformWeight(0.f), AdditiveType(AAT_None)
	{
		Weights = SourceWeights;
		WeightIndices = SourceWeightsIndices;

		for (int32 SourceAttributesIndex = 0; SourceAttributesIndex < SourceAttributes.Num(); ++SourceAttributesIndex)
		{
			const FStackAttributeContainer& CustomAttributes = SourceAttributes[SourceAttributesIndex];
			ProcessAttributes(CustomAttributes, SourceAttributesIndex, AttributeScriptStruct);
		}
	}

	void FAttributeBlendData::ProcessAttributes(const FStackAttributeContainer& AttributeContainers, int32 SourceAttributesIndex, const UScriptStruct* AttributeScriptStruct)
	{
		auto FindExistingUniqueAttribute = [this](const FAttributeId& Identifier)
		{
			return UniqueAttributes.IndexOfByPredicate([Identifier](const FUniqueAttribute& Attribute)
			{
				return *Attribute.Identifier == Identifier;
			});
		};

		auto FindExistingAttributeSet = [this](const FAttributeId& Identifier)
		{
			return AttributeSets.IndexOfByPredicate([Identifier](const FAttributeSet& AttributeSet)
			{
				return *AttributeSet.Identifier == Identifier;
			});
		};

		auto AddAttributeToSet = [this](FAttributeSet& AttributeSet, const uint8* DataPtr, int32 WeightIndex)
		{
			AttributeSet.DataPtrs.Add(DataPtr);
			AttributeSet.WeightIndices.Add(WeightIndex);

			const float Weight = GetContainerWeight(WeightIndex);
			if (Weight > AttributeSet.HighestWeight)
			{
				AttributeSet.HighestWeight = Weight;
				AttributeSet.HighestWeightedIndex = AttributeSet.DataPtrs.Num() - 1;
			}
		};

		const int32 WeightIndex = SourceAttributesIndex;
		const int32 TypeIndex = AttributeContainers.FindTypeIndex(AttributeScriptStruct);
		if (TypeIndex != INDEX_NONE)
		{
			const TConstArrayView<TWrappedAttribute<FAnimStackAllocator>> ValuesArray = AttributeContainers.GetValues(TypeIndex);
			const TConstArrayView<FAttributeId> AttributeIdentifiers = AttributeContainers.GetKeys(TypeIndex);
			
			for (int32 AttributeIndex = 0; AttributeIndex < AttributeIdentifiers.Num(); ++AttributeIndex)
			{
				const FAttributeId& AttributeIdentifier = AttributeIdentifiers[AttributeIndex];
				const int32 ExistingAttributeSetIndex = FindExistingAttributeSet(AttributeIdentifier);

				if (ExistingAttributeSetIndex != INDEX_NONE)
				{
					// Add entry to the set
     // 将条目添加到集合中
					FAttributeSet& ExistingSet = AttributeSets[ExistingAttributeSetIndex];
					AddAttributeToSet(ExistingSet, ValuesArray[AttributeIndex].GetPtr<uint8>(), WeightIndex);
				}
				else
				{
					const int32 ExistingUniqueAttributeIndex = FindExistingUniqueAttribute(AttributeIdentifier);
					if (ExistingUniqueAttributeIndex != INDEX_NONE)
					{
						// Need to create a set
      // 需要创建一个集合
						const FUniqueAttribute& ExistingUniqueAttribute = UniqueAttributes[ExistingUniqueAttributeIndex];

						FAttributeSet& AttributeSet = AttributeSets.AddZeroed_GetRef();
						AttributeSet.Identifier = &AttributeIdentifier;
						AttributeSet.HighestWeight = -1.f;
						AttributeSet.HighestWeightedIndex = -1;

						// Add existing data
      // 添加现有数据
						AddAttributeToSet(AttributeSet, ExistingUniqueAttribute.DataPtr, ExistingUniqueAttribute.WeightIndex);

						// Add new data
      // 添加新数据
						AddAttributeToSet(AttributeSet, ValuesArray[AttributeIndex].GetPtr<uint8>(), WeightIndex);

						// Remove as a unique attribute
      // 作为唯一属性删除
						UniqueAttributes.RemoveAtSwap(ExistingUniqueAttributeIndex);
					}
					else
					{
						// Create a unique attribute
      // 创建独特的属性
						FUniqueAttribute& NewUniqueAttribute = UniqueAttributes.AddZeroed_GetRef();
						NewUniqueAttribute.Identifier = &AttributeIdentifier;
						NewUniqueAttribute.WeightIndex = WeightIndex;
						NewUniqueAttribute.DataPtr = ValuesArray[AttributeIndex].GetPtr<uint8>();
					}
				}
			}
		}
	}

	float FAttributeBlendData::GetContainerWeight(int32 ContainerIndex) const
	{
		// Check for float weights
  // 检查浮子重量
		if (Weights.Num())
		{
			int32 WeightIndex = ContainerIndex;

			// Remap weight index if necessary
   // 如有必要，重新映射权重指数
			if (WeightIndices.Num())
			{
				check(WeightIndices.IsValidIndex(ContainerIndex));
				WeightIndex = WeightIndices[ContainerIndex];
			}

			return Weights[WeightIndex];
		}

		return UniformWeight;
	}

	float FAttributeBlendData::GetBoneWeight(int32 ContainerIndex, int32 BoneIndex) const
	{
		// Check for FPerBoneBlendWeight data
  // 检查 FPerBoneBlendWeight 数据
		if (PerBoneWeights.Num())
		{
			ensure(PerBoneWeights.IsValidIndex(BoneIndex));

			// The ContainerIndex is offset by one when doing a filtered bone blend, as the Base Attributes take the 0 index
   // 进行过滤骨骼混合时，ContainerIndex 会偏移 1，因为基本属性采用 0 索引
			if ((PerBoneWeights[BoneIndex].SourceIndex + 1) == ContainerIndex)
			{
				return PerBoneWeights[BoneIndex].BlendWeight;
			}
			// The base attributes weighting is the inverse of the filtered bone weight
   // 基本属性权重是过滤后骨骼权重的倒数
			else if (ContainerIndex == 0)
			{
				return 1.0f - PerBoneWeights[BoneIndex].BlendWeight;
			}
		}

		// Check for float bone weights
  // 检查浮骨重量
		if (GetBoneWeights().Num())
		{
			ensure(Weights.IsValidIndex(BoneIndex));

			float BoneWeight = GetBoneWeights()[BoneIndex];
			// First attribute containers weighting is the inverse of the second container its weights
   // 第一个属性容器权重是第二个容器权重的倒数
			BoneWeight = ContainerIndex == 0 ? 1.f - BoneWeight : BoneWeight;
			return BoneWeight;
		}

		// Check for FBlendSampleData data 
  // 检查 FBlendSampleData 数据
		if (BlendSampleDataCache.Num())
		{
			// Remap index if necessary
   // 如有必要，重新映射索引
			const int32 SampleDataIndex = GetBlendSampleDataCacheIndices().Num() ? GetBlendSampleDataCacheIndices()[ContainerIndex] : ContainerIndex;
						
			const FBlendSampleData& BlendSampleData = BlendSampleDataCache[SampleDataIndex];
			const int32 PerBoneIndex = PerBoneInterpolationIndices[BoneIndex];

			// Blend-sample blending is only performed when they contain per-bone weights, if INDEX_NONE or out of range use the total weight instead
   // 混合样本混合仅在包含每个骨骼权重时执行，如果 INDEX_NONE 或超出范围，则使用总权重
			if (PerBoneIndex != INDEX_NONE && BlendSampleData.PerBoneBlendData.IsValidIndex(PerBoneIndex))
			{
				return BlendSampleData.PerBoneBlendData[PerBoneIndex];
			}
			else
			{
				return BlendSampleData.GetClampedWeight();
			}
		}

		return 0.f;
	}

	bool FAttributeBlendData::HasBoneWeights() const
	{
		return HighestBoneWeightedIndices.Num() > 0;
	}
	bool FAttributeBlendData::HasContainerWeights() const
	{
		return HighestBoneWeightedIndices.Num() == 0;
	}
}}
