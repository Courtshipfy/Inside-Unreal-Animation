// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AttributesRuntime.h"


namespace UE
{
	namespace Anim
	{	
		struct FAttributeBlendData
		{
			friend struct Attributes;

		protected:
			static FAttributeBlendData PerContainerWeighted(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const float> SourceWeights, const UScriptStruct* AttributeScriptStruct)
			{
				return FAttributeBlendData(SourceAttributes, SourceWeights, AttributeScriptStruct);
			}

			static FAttributeBlendData PerContainerPtrWeighted(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const TArrayView<const float> SourceWeights, const UScriptStruct* AttributeScriptStruct)
			{
				return FAttributeBlendData(SourceAttributes, SourceWeights, AttributeScriptStruct);
			}

			static FAttributeBlendData PerContainerRemappedWeighted(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const float> SourceWeights, const TArrayView<const int32> SourceWeightsIndices, const UScriptStruct* AttributeScriptStruct)
			{
				return FAttributeBlendData(SourceAttributes, SourceWeights, SourceWeightsIndices, AttributeScriptStruct);
			}

			static FAttributeBlendData SingleContainerUniformWeighted(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const float InUniformWeight, const UScriptStruct* AttributeScriptStruct)
			{
				ensure(SourceAttributes.Num() == 1);
				return FAttributeBlendData(SourceAttributes, InUniformWeight, AttributeScriptStruct);
			}

			static FAttributeBlendData SingleAdditiveContainerUniformWeighted(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const float InUniformWeight, EAdditiveAnimationType AdditiveType, const UScriptStruct* AttributeScriptStruct)
			{
				ensure(SourceAttributes.Num() == 1);
				return FAttributeBlendData(SourceAttributes, InUniformWeight, AdditiveType, AttributeScriptStruct);
			}

			static FAttributeBlendData PerBoneWeighted(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const FPerBoneBlendWeight> InPerBoneBlendWeights, const UScriptStruct* AttributeScriptStruct)
			{
				return FAttributeBlendData(SourceAttributes, InPerBoneBlendWeights, AttributeScriptStruct);
			}

			static FAttributeBlendData PerBoneBlendSamples(const TArrayView<const FStackAttributeContainer> SourceAttributes, TArrayView<const int32> InPerBoneInterpolationIndices, const TArrayView<const FBlendSampleData> InBlendSampleDataCache, const UScriptStruct* AttributeScriptStruct)
			{
				return FAttributeBlendData(SourceAttributes, InPerBoneInterpolationIndices, InBlendSampleDataCache, AttributeScriptStruct);
			}

			static FAttributeBlendData PerBoneRemappedBlendSamples(const TArrayView<const FStackAttributeContainer> SourceAttributes, TArrayView<const int32> InPerBoneInterpolationIndices, const TArrayView<const FBlendSampleData> InBlendSampleDataCache, TArrayView<const int32> InBlendSampleDataCacheIndices, const UScriptStruct* AttributeScriptStruct)
			{
				return FAttributeBlendData(SourceAttributes, InPerBoneInterpolationIndices, InBlendSampleDataCache, InBlendSampleDataCacheIndices, AttributeScriptStruct);
			}

			static FAttributeBlendData PerBoneFilteredWeighted(const FStackAttributeContainer& BaseAttributes, const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const FPerBoneBlendWeight> InPerBoneBlendWeights, const UScriptStruct* AttributeScriptStruct)
			{
				return FAttributeBlendData(BaseAttributes, SourceAttributes, InPerBoneBlendWeights, AttributeScriptStruct);
			}

			static FAttributeBlendData PerBoneSingleContainerWeighted(const FStackAttributeContainer& SourceAttributes1, const FStackAttributeContainer& SourceAttributes2, const TArrayView<const float> WeightsOfSource2, const UScriptStruct* AttributeScriptStruct)
			{
				return FAttributeBlendData(SourceAttributes1, SourceAttributes2, WeightsOfSource2, AttributeScriptStruct);
			}
		private:
			// Blend constructor
			// 混合构造函数
			ENGINE_API FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const float> SourceWeights, const UScriptStruct* AttributeScriptStruct);
						
			// Blend-by-ptr constructor
			// Blend-by ptr 构造函数
			ENGINE_API FAttributeBlendData(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const TArrayView<const float> SourceWeights, const UScriptStruct* AttributeScriptStruct);

			// Blend remapped weights constructor
			// 混合重新映射的权重构造函数
			ENGINE_API FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const float> SourceWeights, const TArrayView<const int32> SourceWeightsIndices, const UScriptStruct* AttributeScriptStruct);

			// Accumulate using a single weight
			// 使用单个重量进行累加
			ENGINE_API FAttributeBlendData(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const float InUniformWeight, const UScriptStruct* AttributeScriptStruct);

			// Additive accumulate using a single weight
			// 使用单一权重累加
			ENGINE_API FAttributeBlendData(const TArrayView<const FStackAttributeContainer* const> SourceAttributes, const float InUniformWeight, EAdditiveAnimationType InAdditiveType, const UScriptStruct* AttributeScriptStruct);

			// Blend using per-bone blend weights
			// 使用每骨骼混合权重进行混合
			ENGINE_API FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const FPerBoneBlendWeight> InPerBoneBlendWeights, const UScriptStruct* AttributeScriptStruct);

			// Blend using BlendSample (per-bone) weight data 
			// 使用 BlendSample（每个骨骼）权重数据进行混合
			ENGINE_API FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, TArrayView<const int32> InPerBoneInterpolationIndices, const TArrayView<const FBlendSampleData> InBlendSampleDataCache, const UScriptStruct* AttributeScriptStruct);

			// Blend using BlendSample (per-bone) remapped weight data 
			// 使用 BlendSample（每个骨骼）重新映射权重数据进行混合
			ENGINE_API FAttributeBlendData(const TArrayView<const FStackAttributeContainer> SourceAttributes, TArrayView<const int32> InPerBoneInterpolationIndices, const TArrayView<const FBlendSampleData> InBlendSampleDataCache, TArrayView<const int32> InBlendSampleDataCacheIndices, const UScriptStruct* AttributeScriptStruct);

			// Blend (per-bone filtered) using (per-bone) weight data 
			// 使用（每个骨骼）权重数据混合（每个骨骼过滤）
			ENGINE_API FAttributeBlendData(const FStackAttributeContainer& BaseAttributes, const TArrayView<const FStackAttributeContainer> SourceAttributes, const TArrayView<const FPerBoneBlendWeight> InPerBoneBlendWeights, const UScriptStruct* AttributeScriptStruct);

			// Blend using (per-bone) weight data for one of the two inputs
			// 使用两个输入之一的（每个骨骼）权重数据进行混合
			ENGINE_API FAttributeBlendData(const FStackAttributeContainer& SourceAttributes1, const FStackAttributeContainer& SourceAttributes2, const TArrayView<const float> WeightsOfSource2, const UScriptStruct* AttributeScriptStruct);

			ENGINE_API FAttributeBlendData();
		private:
			ENGINE_API void ProcessAttributes(const FStackAttributeContainer& AttributeContainers, int32 SourceAttributesIndex, const UScriptStruct* AttributeScriptStruct);

			/** Retrieves the weight on a top-level container basis */
			/** 检索顶层容器的重量 */
			ENGINE_API float GetContainerWeight(int32 ContainerIndex) const;

			/* Retrieves the weight on a per-bone level basis according to the attribute and bone indices */
			/* 根据属性和骨骼索引检索每个骨骼级别的权重 */
			ENGINE_API float GetBoneWeight(int32 AttributeIndex, int32 BoneIndex) const;

			/** Tests for different weight basis */
			/** 不同重量基础的测试 */
			ENGINE_API bool HasBoneWeights() const;
			ENGINE_API bool HasContainerWeights() const;
		private:
			/** Structure containing overlapping attributes */
			/** 包含重叠属性的结构 */
			struct FAttributeSet
			{
				/** Pointers to attribute values */
				/** 指向属性值的指针 */
				TArray<const uint8*, FAnimStackAllocator> DataPtrs;

				/** Weight indices used to map to weight data */
				/** 用于映射到权重数据的权重指数 */
				TArray<int32, FAnimStackAllocator> WeightIndices;
				
				/** Identifier of the attribute */
				/** 属性的标识符 */
				const FAttributeId* Identifier;

				/** Highest weight value, and its weight index, that was processed */
				/** 处理后的最高权重值及其权重指数 */
				float HighestWeight;
				int32 HighestWeightedIndex;
			};

			/** Structure representing a unique (non-overlapping) attribute */
			/** 表示唯一（不重叠）属性的结构 */
			struct FUniqueAttribute
			{
				/** Identifier of the attribute */
				/** 属性的标识符 */
				const FAttributeId* Identifier;

				/** Weight index used to map to weight data */
				/** 用于映射到权重数据的权重指数 */
				int32 WeightIndex;

				/** Pointer to attribute value */
				/** 指向属性值的指针 */
				const uint8* DataPtr;
			};

			/** Processed unique and sets of attributes */
			/** 已处理的唯一属性和属性集 */
			TArray<FAttributeSet, FAnimStackAllocator> AttributeSets;
			TArray<FUniqueAttribute, FAnimStackAllocator> UniqueAttributes;

			/** Container level weight data */
			/** 集装箱液位重量数据 */
			float UniformWeight;
			 /* Contains container of per-bone weights */
			 /* 包含每个骨骼重量的容器 */
			TArrayView<const float> Weights;
			/* Contains container or BlendSampleDataCache remapping indices */
			/* 包含容器或 BlendSampleDataCache 重新映射索引 */
			TArrayView<const int32> WeightIndices; 
			/** Valid whenever performing an additive accumulate */
			/** 每当执行累加时有效 */
			EAdditiveAnimationType AdditiveType;

			/** Bone level weight data */
			/** 骨水平重量数据 */
			TArrayView<const FPerBoneBlendWeight> PerBoneWeights;
			TArray<int32, FAnimStackAllocator> HighestBoneWeightedIndices;
			inline const TArrayView<const float>& GetBoneWeights() const { return Weights; }
			bool bPerBoneFilter = false;
			
			/** Blend sample weight data */
			/** 混合样品重量数据 */
			TArrayView<const int32> PerBoneInterpolationIndices;
			TArrayView<const FBlendSampleData> BlendSampleDataCache;
			inline const TArrayView<const int32>& GetBlendSampleDataCacheIndices() const { return WeightIndices; }
		public:
			template<typename AttributeType>
			struct TAttributeSetIterator
			{
				friend struct FAttributeBlendData;
			protected:
				TAttributeSetIterator(const FAttributeBlendData& InData, const FAttributeSet& InCollection) : Data(InData), Collection(InCollection), CurrentIndex(-1) {}
			public:
				/** Return the value for the currently indexed entry in the attribute set */
				/** 返回属性集中当前索引条目的值 */
				const AttributeType& GetValue() const
				{
					check(CurrentIndex < Collection.DataPtrs.Num() && Collection.DataPtrs.IsValidIndex(CurrentIndex));
					return *(const AttributeType*)Collection.DataPtrs[CurrentIndex];
				}

				/** Returns (container level) weight value for the current attribute's container in the attribute set */
				/** 返回属性集中当前属性容器的（容器级别）权重值 */
				const float GetWeight() const
				{
					check(Data.HasContainerWeights());
					check(CurrentIndex < Collection.DataPtrs.Num() && Collection.WeightIndices.IsValidIndex(CurrentIndex));
					return Data.GetContainerWeight(Collection.WeightIndices[CurrentIndex]);
				}

				/** Returns (bone level) weight value for the current attribute its bone and container */
				/** 返回当前属性（其骨骼和容器）的（骨骼级别）权重值 */
				const float GetBoneWeight() const
				{
					check(Data.HasBoneWeights());
					check(CurrentIndex < Collection.DataPtrs.Num() && Collection.WeightIndices.IsValidIndex(CurrentIndex));
					return Data.GetBoneWeight(Collection.WeightIndices[CurrentIndex], Collection.Identifier->GetIndex());
				}

				/** Returns highest (container level) weighted value for the attribute set */
				/** 返回属性集的最高（容器级别）权重值 */
				const AttributeType& GetHighestWeightedValue() const
				{
					check(Data.HasContainerWeights());
					check(Collection.HighestWeightedIndex < Collection.DataPtrs.Num() && Collection.DataPtrs.IsValidIndex(Collection.HighestWeightedIndex));
					return *(const AttributeType*)Collection.DataPtrs[Collection.HighestWeightedIndex];
				}

				/** Returns highest (bone level) weighted value for the attribute set */
				/** 返回属性集的最高（骨骼级别）权重值 */
				const AttributeType& GetHighestBoneWeightedValue() const
				{
					check(Data.HasBoneWeights());

					int32 HighestIndex = INDEX_NONE;
					float Weight = -1.f;
					for (const int32 Index : Collection.WeightIndices)
					{
						const float BoneWeight = Data.GetBoneWeight(Index, Collection.Identifier->GetIndex());
						if (BoneWeight > Weight)
						{
							Weight = BoneWeight;
							HighestIndex = Index;
						}
					}
					ensure(HighestIndex != INDEX_NONE);
					return *(const AttributeType*)Collection.DataPtrs[Collection.WeightIndices.IndexOfByKey(HighestIndex)];
				}

				/** Returns highest (bone level) weighted value, and its weight for the attribute set */
				/** 返回最高（骨骼级别）权重值及其属性集的权重 */
				void GetHighestBoneWeighted(const AttributeType*& OutAttributePtr, float& OutWeight) const
				{
					check(Data.HasBoneWeights());

					int32 HighestIndex = INDEX_NONE;
					float Weight = -1.f;
					for (const int32 Index : Collection.WeightIndices)
					{
						const float BoneWeight = Data.GetBoneWeight(Index, Collection.Identifier->GetIndex());
						if (BoneWeight > Weight)
						{
							Weight = BoneWeight;
							HighestIndex = Index;
						}
					}
					ensure(HighestIndex != INDEX_NONE);
					OutAttributePtr = (const AttributeType*)Collection.DataPtrs[Collection.WeightIndices.IndexOfByKey(HighestIndex)];
					OutWeight = Weight;
				}

				/** Returns the identifier for the current attribute set */
				/** 返回当前属性集的标识符 */
				const FAttributeId& GetIdentifier() const
				{
					return *Collection.Identifier;
				}

				EAdditiveAnimationType GetAdditiveType() const
				{
					return Data.AdditiveType;
				}

				bool IsFilteredBlend() const
				{
					return Data.bPerBoneFilter;
				}

				/** Cycle through to next entry in the attribute set, returns false if the end was reached */
				/** 循环到属性集中的下一个条目，如果到达末尾则返回 false */
				bool Next()
				{
					++CurrentIndex;
					return CurrentIndex < Collection.DataPtrs.Num();
				}

				int32 GetIndex() const
				{
					return CurrentIndex;
				}
			protected:
				/** Outer object that creates this */
				/** 创建 this 的外部对象 */
				const FAttributeBlendData& Data;
				/** Attribute collection for current index */
				/** 当前索引的属性集合 */
				const FAttributeSet& Collection;
				int32 CurrentIndex;
			};

			struct TAttributeSetRawIterator
			{
				friend struct FAttributeBlendData;
			protected:
				TAttributeSetRawIterator(const FAttributeBlendData& InData, const FAttributeSet& InCollection) : Data(InData), Collection(InCollection), CurrentIndex(-1) {}
			public:
				/** Return the value for the currently indexed entry in the attribute set */
				/** 返回属性集中当前索引条目的值 */
				const uint8* GetValuePtr() const
				{
					check(CurrentIndex < Collection.DataPtrs.Num() && Collection.DataPtrs.IsValidIndex(CurrentIndex));
					return Collection.DataPtrs[CurrentIndex];
				}

				/** Returns (container level) weight value for the current attribute's container in the attribute set */
				/** 返回属性集中当前属性容器的（容器级别）权重值 */
				const float GetWeight() const
				{
					check(Data.HasContainerWeights());
					check(CurrentIndex < Collection.DataPtrs.Num() && Collection.WeightIndices.IsValidIndex(CurrentIndex));
					return Data.GetContainerWeight(Collection.WeightIndices[CurrentIndex]);
				}

				/** Returns (bone level) weight value for the current attribute its bone and container */
				/** 返回当前属性（其骨骼和容器）的（骨骼级别）权重值 */
				const float GetBoneWeight() const
				{
					check(Data.HasBoneWeights());
					check(CurrentIndex < Collection.DataPtrs.Num() && Collection.WeightIndices.IsValidIndex(CurrentIndex));
					return Data.GetBoneWeight(Collection.WeightIndices[CurrentIndex], Collection.Identifier->GetIndex());
				}

				/** Returns highest (container level) weighted value for the attribute set */
				/** 返回属性集的最高（容器级别）权重值 */
				const uint8* GetHighestWeightedValue() const
				{
					check(Data.HasContainerWeights());
					check(Collection.HighestWeightedIndex < Collection.DataPtrs.Num() && Collection.DataPtrs.IsValidIndex(Collection.HighestWeightedIndex));
					return Collection.DataPtrs[Collection.HighestWeightedIndex];
				}

				/** Returns highest (bone level) weighted value for the attribute set */
				/** 返回属性集的最高（骨骼级别）权重值 */
				const uint8* GetHighestBoneWeightedValue() const
				{
					check(Data.HasBoneWeights());

					int32 HighestIndex = INDEX_NONE;
					float Weight = -1.f;
					for (const int32 Index : Collection.WeightIndices)
					{
						const float BoneWeight = Data.GetBoneWeight(Index, Collection.Identifier->GetIndex());
						if (BoneWeight > Weight)
						{
							Weight = BoneWeight;
							HighestIndex = Index;
						}
					}
					ensure(HighestIndex != INDEX_NONE);
					return Collection.DataPtrs[Collection.WeightIndices.IndexOfByKey(HighestIndex)];
				}

				/** Returns highest (bone level) weighted value, and its weight for the attribute set */
				/** 返回最高（骨骼级别）权重值及其属性集的权重 */
				void GetHighestBoneWeighted(const uint8* OutAttributePtr, float& OutWeight) const
				{
					check(Data.HasBoneWeights());

					int32 HighestIndex = INDEX_NONE;
					float Weight = -1.f;
					for (const int32 Index : Collection.WeightIndices)
					{
						const float BoneWeight = Data.GetBoneWeight(Index, Collection.Identifier->GetIndex());
						if (BoneWeight > Weight)
						{
							Weight = BoneWeight;
							HighestIndex = Index;
						}
					}
					ensure(HighestIndex != INDEX_NONE);
					OutAttributePtr = Collection.DataPtrs[Collection.WeightIndices.IndexOfByKey(HighestIndex)];
					OutWeight = Weight;
				}

				/** Returns the identifier for the current attribute set */
				/** 返回当前属性集的标识符 */
				const FAttributeId& GetIdentifier() const
				{
					return *Collection.Identifier;
				}

				EAdditiveAnimationType GetAdditiveType() const
				{
					return Data.AdditiveType;
				}

				bool IsFilteredBlend() const
				{
					return Data.bPerBoneFilter;
				}

				/** Cycle through to next entry in the attribute set, returns false if the end was reached */
				/** 循环到属性集中的下一个条目，如果到达末尾则返回 false */
				bool Next()
				{
					++CurrentIndex;
					return CurrentIndex < Collection.DataPtrs.Num();
				}

				int32 GetIndex() const
				{
					return CurrentIndex;
				}
			protected:
				/** Outer object that creates this */
				/** 创建 this 的外部对象 */
				const FAttributeBlendData& Data;
				/** Attribute collection for current index */
				/** 当前索引的属性集合 */
				const FAttributeSet& Collection;
				int32 CurrentIndex;
			};

			template<typename AttributeType>
			struct TSingleIterator
			{
				friend struct FAttributeBlendData;
			protected:
				TSingleIterator(const FAttributeBlendData& InData, TArrayView<const FUniqueAttribute> InAttributes) : Data(InData), AttributesView(InAttributes), CurrentIndex(-1) {}
			public:
				/** Cycle through to next unique attribute, returns false if the end was reached */
				/** 循环到下一个唯一属性，如果到达末尾则返回 false */
				bool Next()
				{
					++CurrentIndex;
					return CurrentIndex < AttributesView.Num();
				}

				/** Return the value for the currently indexed unique attribute */
				/** 返回当前索引的唯一属性的值 */
				const AttributeType& GetValue() const
				{
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));
					return *(const AttributeType*)AttributesView[CurrentIndex].DataPtr;
				}

				/** Returns (container level) weight value for the unique attribute its container */
				/** 返回其容器的唯一属性的（容器级别）权重值 */
				const float GetWeight() const
				{
					check(Data.HasContainerWeights());
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));
					
					return Data.GetContainerWeight(AttributesView[CurrentIndex].WeightIndex);
				}
				
				/** Returns (bone level) weight value for the unique attribute its bone and container */
				/** 返回其骨骼和容器的唯一属性的（骨骼级别）权重值 */
				const float GetBoneWeight() const
				{
					check(Data.HasBoneWeights());
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));
					
					return Data.GetBoneWeight(AttributesView[CurrentIndex].WeightIndex, AttributesView[CurrentIndex].Identifier->GetIndex());
				}

				/** Returns whether or not the unique attribute its (bone level) weight is the highest across the containers */
				/** 返回唯一属性的（骨骼级别）权重是否是容器中最高的 */
				bool IsHighestBoneWeighted() const
				{
					check(Data.HasBoneWeights());
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));
					check(Data.HighestBoneWeightedIndices.IsValidIndex(AttributesView[CurrentIndex].Identifier->GetIndex()));

					return Data.HighestBoneWeightedIndices[AttributesView[CurrentIndex].Identifier->GetIndex()] == AttributesView[CurrentIndex].WeightIndex;
				}

				/** Returns the identifier for the current attribute set */
				/** 返回当前属性集的标识符 */
				const FAttributeId& GetIdentifier() const
				{
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));					
					return *AttributesView[CurrentIndex].Identifier;
				}

				EAdditiveAnimationType GetAdditiveType() const
				{
					return Data.AdditiveType;
				}
				
				bool IsFilteredBlend() const
				{
					return Data.bPerBoneFilter;
				}

			protected:
				/** Outer object that creates this */
				/** 创建 this 的外部对象 */
				const FAttributeBlendData& Data;
				TArrayView<const FUniqueAttribute> AttributesView;
				int32 CurrentIndex;
			};

			struct TSingleRawIterator
			{
				friend struct FAttributeBlendData;
			protected:
				TSingleRawIterator(const FAttributeBlendData& InData, TArrayView<const FUniqueAttribute> InAttributes) : Data(InData), AttributesView(InAttributes), CurrentIndex(-1) {}
			public:
				/** Cycle through to next unique attribute, returns false if the end was reached */
				/** 循环到下一个唯一属性，如果到达末尾则返回 false */
				bool Next()
				{
					++CurrentIndex;
					return CurrentIndex < AttributesView.Num();
				}

				/** Return the value for the currently indexed unique attribute */
				/** 返回当前索引的唯一属性的值 */
				const uint8* GetValuePtr() const
				{
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));
					return AttributesView[CurrentIndex].DataPtr;
				}

				/** Returns (container level) weight value for the unique attribute its container */
				/** 返回其容器的唯一属性的（容器级别）权重值 */
				const float GetWeight() const
				{
					check(Data.HasContainerWeights());
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));
					
					return Data.GetContainerWeight(AttributesView[CurrentIndex].WeightIndex);
				}
				
				/** Returns (bone level) weight value for the unique attribute its bone and container */
				/** 返回其骨骼和容器的唯一属性的（骨骼级别）权重值 */
				const float GetBoneWeight() const
				{
					check(Data.HasBoneWeights());
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));
					
					return Data.GetBoneWeight(AttributesView[CurrentIndex].WeightIndex, AttributesView[CurrentIndex].Identifier->GetIndex());
				}

				/** Returns whether or not the unique attribute its (bone level) weight is the highest across the containers */
				/** 返回唯一属性的（骨骼级别）权重是否是容器中最高的 */
				bool IsHighestBoneWeighted() const
				{
					check(Data.HasBoneWeights());
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));
					check(Data.HighestBoneWeightedIndices.IsValidIndex(AttributesView[CurrentIndex].Identifier->GetIndex()));

					return Data.HighestBoneWeightedIndices[AttributesView[CurrentIndex].Identifier->GetIndex()] == AttributesView[CurrentIndex].WeightIndex;
				}

				/** Returns the identifier for the current attribute set */
				/** 返回当前属性集的标识符 */
				const FAttributeId& GetIdentifier() const
				{
					check(CurrentIndex < AttributesView.Num() && AttributesView.IsValidIndex(CurrentIndex));					
					return *AttributesView[CurrentIndex].Identifier;
				}

				EAdditiveAnimationType GetAdditiveType() const
				{
					return Data.AdditiveType;
				}
				
				bool IsFilteredBlend() const
				{
					return Data.bPerBoneFilter;
				}

			protected:
				/** Outer object that creates this */
				/** 创建 this 的外部对象 */
				const FAttributeBlendData& Data;
				TArrayView<const FUniqueAttribute> AttributesView;
				int32 CurrentIndex;
			};

		public:
			template<typename AttributeType>
			void ForEachAttributeSet(TFunctionRef<void(TAttributeSetIterator<AttributeType>&)> ForEachFunction) const
			{
				for (const FAttributeSet& Collection : AttributeSets)
				{
					TAttributeSetIterator<AttributeType> It(*this, Collection);
					ForEachFunction(It);
				}
			}

			template<typename AttributeType>
			void ForEachUniqueAttribute(TFunctionRef<void(TSingleIterator<AttributeType>&)> ForEachFunction) const
			{
				TSingleIterator<AttributeType> It(*this, UniqueAttributes);
				ForEachFunction(It);
			}
			
			void ForEachAttributeSet(TFunctionRef<void(TAttributeSetRawIterator&)> ForEachFunction) const
			{
				for (const FAttributeSet& Collection : AttributeSets)
				{
					TAttributeSetRawIterator It(*this, Collection);
					ForEachFunction(It);
				}
			}

			void ForEachUniqueAttribute(TFunctionRef<void(TSingleRawIterator&)> ForEachFunction) const
			{
				TSingleRawIterator It(*this, UniqueAttributes);
				ForEachFunction(It);
			}
		};
	}
}
