// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

namespace UE
{
	namespace Anim
	{
		struct FAttributeBlendData;
		struct FStackAttributeContainer;

		/** Interface required to implement for user-defined blending behaviour of an animation attribute type. See TAttributeBlendOperator for an example implementation. */
		/** 实现动画属性类型的用户定义混合行为所需的接口。有关示例实现，请参阅 TAttributeBlendOperator。 */
		class IAttributeBlendOperator
		{
		public:
			virtual ~IAttributeBlendOperator() {}

			/** Invoked when two or multiple sets of attribute container inputs are to be blended together*/
			/** 当要将两组或多组属性容器输入混合在一起时调用*/
			virtual void Blend(const FAttributeBlendData& BlendData, FStackAttributeContainer* OutAttributes) const = 0;

			/** Invoked when two or multiple sets of attribute container inputs are to be blended together, using individual bone weights */
			/** 当要使用单独的骨骼权重将两组或多组属性容器输入混合在一起时调用 */
			virtual void BlendPerBone(const FAttributeBlendData& BlendData, FStackAttributeContainer* OutAttributes) const = 0;
			
			/** Invoked when an attribute container A is expected to override attributes in container B */
			/** 当属性容器 A 需要覆盖容器 B 中的属性时调用 */
			virtual void Override(const FAttributeBlendData& BlendData, FStackAttributeContainer* OutAttributes) const = 0;

			/** Invoked when an attribute container A is accumulated into container B */
			/** 当属性容器A累积到容器B中时调用 */
			virtual void Accumulate(const FAttributeBlendData& BlendData, FStackAttributeContainer* OutAttributes) const = 0;

			/** Invoked when an attribute container is supposed to be made additive with regards to container B */
			/** 当属性容器应该与容器 B 相加时调用 */
			virtual void ConvertToAdditive(const FAttributeBlendData& BlendData, FStackAttributeContainer* OutAdditiveAttributes) const = 0;

			/** Invoked to interpolate between two individual attribute type values, according to the provided alpha */
			/** 调用以根据提供的 alpha 在两个单独的属性类型值之间进行插值 */
			virtual void Interpolate(const void* FromData, const void* ToData, float Alpha, void* InOutData) const = 0;
		};
	}
}
