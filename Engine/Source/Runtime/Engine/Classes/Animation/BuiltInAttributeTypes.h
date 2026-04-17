// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AttributeTraits.h"
#include "AnimationRuntime.h"

#include "Algo/Transform.h"

#include "BuiltInAttributeTypes.generated.h"

/** Attribute type supporting the legacy TVariant<float> atttributes */
/** 支持旧版 TVariant<float> 属性的属性类型 */
USTRUCT(BlueprintType)
struct FFloatAnimationAttribute 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FloatAnimationAttribute)
	float Value = 0.f;

	void Accumulate(const FFloatAnimationAttribute& Attribute, float Weight, EAdditiveAnimationType AdditiveType)
	{
		Value += Attribute.Value * Weight;
	}

	void MakeAdditive(const FFloatAnimationAttribute& BaseAttribute)
	{
		Value -= BaseAttribute.Value;
	}

	FFloatAnimationAttribute Multiply(float Weight) const
	{
		FFloatAnimationAttribute Out;
		Out.Value = Value * Weight;
		return Out;
	}

	void Interpolate(const FFloatAnimationAttribute& Attribute, float Alpha)
	{
		Value *= (1.f - Alpha);
		Value += (Attribute.Value * Alpha);
	}
};

/** Attribute type supporting the legacy TVariant<int32> atttributes */
/** 支持旧版 TVariant<int32> 属性的属性类型 */
USTRUCT(BlueprintType)
struct FIntegerAnimationAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=IntegerAnimationAttribute)
	int32 Value = 0;

	void Accumulate(const FIntegerAnimationAttribute& Attribute, float Weight, EAdditiveAnimationType AdditiveType)
	{
		Value += (int32)(Attribute.Value * Weight);
	}

	void MakeAdditive(const FIntegerAnimationAttribute& BaseAttribute)
	{
		Value -= BaseAttribute.Value;
	}

	FIntegerAnimationAttribute Multiply(float Weight) const
	{
		FIntegerAnimationAttribute Out;
		Out.Value = (int32)(Value * Weight);
		return Out;
	}

	void Interpolate(const FIntegerAnimationAttribute& Attribute, float Alpha)
	{
		Value = FMath::TruncToInt32(Value * (1.f - Alpha));
		Value += FMath::TruncToInt32(Attribute.Value * Alpha);
	}
};

/** Attribute type supporting the legacy TVariant<FString> attributes */
/** 支持旧版 TVariant<FString> 属性的属性类型 */
USTRUCT(BlueprintType)
struct FStringAnimationAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=StringAnimationAttribute)
	FString Value;
};

inline uint32 GetTypeHash(const FStringAnimationAttribute& Key)
{
	return GetTypeHash(Key.Value);
}

/** Attribute type supporting the legacy TVariant<FTransform> attributes */
/** 支持旧版 TVariant<FTransform> 属性的属性类型 */
USTRUCT(BlueprintType)
struct FTransformAnimationAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TransformAnimationAttribute)
	FTransform Value;

	void Accumulate(const FTransformAnimationAttribute& Attribute, float Weight, EAdditiveAnimationType AdditiveType)
	{
		//if (FAnimWeight::IsRelevant(Weight))
		//if (FAnimWeight::IsRelevant(Weight))
		{
			const ScalarRegister VBlendWeight(Weight);

			if (AdditiveType == AAT_None)
			{
				Value.AccumulateWithShortestRotation(Attribute.Value, VBlendWeight);
			}
			else
			{
				if (FAnimWeight::IsFullWeight(Weight))
				{
					Value.AccumulateWithAdditiveScale(Attribute.Value, VBlendWeight);
				}
				else
				{
					FTransform::BlendFromIdentityAndAccumulate(Value, Attribute.Value, VBlendWeight);
				}
			}
		}
	}

	void MakeAdditive(const FTransformAnimationAttribute& BaseAttribute)
	{
		FAnimationRuntime::ConvertTransformToAdditive(Value, BaseAttribute.Value);
	}

	void Normalize()
	{
		Value.NormalizeRotation();
	}
	
	FTransformAnimationAttribute Multiply(const float Weight) const
	{
		FTransformAnimationAttribute Out;

		const ScalarRegister VBlendWeight(Weight);
		Out.Value = Value * VBlendWeight;

		return Out;
	}

	void Interpolate(const FTransformAnimationAttribute& Attribute, float Alpha)
	{
		Value.BlendWith(Attribute.Value, Alpha);
	}
};


USTRUCT(BlueprintType)
struct FVectorAnimationAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VectorAnimationAttribute)
	FVector Value = FVector::ZeroVector;

	void Accumulate(const FVectorAnimationAttribute& Attribute, float Weight, EAdditiveAnimationType AdditiveType)
	{
		// if (FAnimWeight::IsRelevant(Weight))
		// if (FAnimWeight::IsRelevant(Weight))
		{
			Value += Attribute.Value * Weight;
		}
	}

	void MakeAdditive(const FVectorAnimationAttribute& BaseAttribute)
	{
		Value = Value - BaseAttribute.Value;
	}

	void Normalize()
	{
		Value.Normalize();
	}
	
	FVectorAnimationAttribute Multiply(const float Weight) const
	{
		FVectorAnimationAttribute Out;
		Out.Value = Value * Weight;

		return Out;
	}

	void Interpolate(const FVectorAnimationAttribute& Attribute, float Alpha)
	{
		Value = FMath::Lerp<FVector>(Value, Attribute.Value, Alpha);
	}
};

USTRUCT(BlueprintType)
struct FQuaternionAnimationAttribute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=QuaternionAnimationAttribute)
	FQuat Value = FQuat::Identity;

	void Accumulate(const FQuaternionAnimationAttribute& Attribute, float Weight, EAdditiveAnimationType AdditiveType)
	{
		// if (FAnimWeight::IsRelevant(Weight))
		// if (FAnimWeight::IsRelevant(Weight))
		{
			if (AdditiveType == AAT_None)
			{
				const FQuat WeightedRotation = Attribute.Value * Weight;
				
				// From VectorAccumulateQuaternionShortestPath
				// 来自 VectorAccumulateQuaternionShortestPath
				// Blend rotation
				// 混合旋转
				//     To ensure the 'shortest route', we make sure the dot product between the both rotations is positive.
				//     为了确保“最短路径”，我们确保两次旋转之间的点积为正。
				//     const float Bias = (|A.B| >= 0 ? 1 : -1)
				//     const float 偏差 = (|A.B| >= 0 ? 1 : -1)
				//     return A + B * Bias;
				//     返回 A + B * 偏差；
				const FQuat::FReal DotResult = (Value | WeightedRotation);
				const FQuat::FReal Bias = FMath::FloatSelect(DotResult, FQuat::FReal(1.0f), FQuat::FReal(-1.0f));
				
				Value += WeightedRotation * Bias;
			}
			else
			{
				// Quaternion equivalent of FTransform::BlendFromIdentityAndAccumulate
				// FTransform::BlendFromIdentityAndAccumulate 的四元数等效项
				const FQuat WeightedRotation = FQuat::FastLerp(FQuat::Identity, Attribute.Value, Weight).GetNormalized();
				Value = WeightedRotation * Value;
			}
			
		}
	}

	void MakeAdditive(const FQuaternionAnimationAttribute& BaseAttribute)
	{
		Value = Value * BaseAttribute.Value.Inverse();
		Value.Normalize();
	}

	void Normalize()
	{
		Value.Normalize();
	}
	
	FQuaternionAnimationAttribute Multiply(const float Weight) const
	{
		FQuaternionAnimationAttribute Out;
		Out.Value = Value * Weight;

		return Out;
	}

	void Interpolate(const FQuaternionAnimationAttribute& Attribute, float Alpha)
	{
		Value = FQuat::FastLerp(Value, Attribute.Value, Alpha).GetNormalized();
	}
};

USTRUCT()
struct FNonBlendableQuaternionAnimationAttribute : public FQuaternionAnimationAttribute
{
	GENERATED_BODY()
};

USTRUCT()
struct FNonBlendableVectorAnimationAttribute : public FVectorAnimationAttribute
{
	GENERATED_BODY()
};

USTRUCT()
struct FNonBlendableTransformAnimationAttribute : public FTransformAnimationAttribute
{
	GENERATED_BODY()
};

USTRUCT()
struct FNonBlendableFloatAnimationAttribute : public FFloatAnimationAttribute
{
	GENERATED_BODY()
};

USTRUCT()
struct FNonBlendableIntegerAnimationAttribute : public FIntegerAnimationAttribute
{
	GENERATED_BODY()
};

namespace UE
{
	namespace Anim
	{
		/** Integer attribute is step-interpolated by default */
		/** 默认情况下，整数属性是逐步插值的 */
		template<>
		struct TAttributeTypeTraits<FIntegerAnimationAttribute> : public TAttributeTypeTraitsBase<FIntegerAnimationAttribute>
		{
			enum
			{
				StepInterpolate = true,
			};
		};

		/** String attribute is not blend-able by default */
		/** 默认情况下，字符串属性不可混合 */
		template<>
		struct TAttributeTypeTraits<FStringAnimationAttribute> : public TAttributeTypeTraitsBase<FStringAnimationAttribute>
		{
			enum
			{
				IsBlendable = false,
			};
		};

		/** Transform attribute requires normalization */
		/** 变换属性需要标准化 */
		template<>
		struct TAttributeTypeTraits<FTransformAnimationAttribute> : public TAttributeTypeTraitsBase<FTransformAnimationAttribute>
		{
			enum
			{
				RequiresNormalization = true,
			};
		};

		/** Quaternion attribute requires normalization */
		/** 四元数属性需要标准化 */
		template<>
		struct TAttributeTypeTraits<FQuaternionAnimationAttribute> : public TAttributeTypeTraitsBase<FQuaternionAnimationAttribute>
		{
			enum
			{
				RequiresNormalization = true,
			};
		};

		/** Non blendable types*/
		/** 不可混合类型*/
		template<>
		struct TAttributeTypeTraits<FNonBlendableQuaternionAnimationAttribute> : public TAttributeTypeTraitsBase<FNonBlendableQuaternionAnimationAttribute>
		{
			enum
			{
				IsBlendable = false,
			};
		};

		/** Non blendable types*/
		/** 不可混合类型*/
		template<>
		struct TAttributeTypeTraits<FNonBlendableVectorAnimationAttribute> : public TAttributeTypeTraitsBase<FNonBlendableVectorAnimationAttribute>
		{
			enum
			{
				IsBlendable = false,
			};
		};
		
		/** Non blendable types*/
		/** 不可混合类型*/
		template<>
		struct TAttributeTypeTraits<FNonBlendableTransformAnimationAttribute> : public TAttributeTypeTraitsBase<FNonBlendableTransformAnimationAttribute>
		{
			enum
			{
				IsBlendable = false,
			};
		};
		
		template<>
		struct TAttributeTypeTraits<FNonBlendableFloatAnimationAttribute> : public TAttributeTypeTraitsBase<FNonBlendableFloatAnimationAttribute>
		{
			enum
			{
				IsBlendable = false,
			};
		};

		template<>
		struct TAttributeTypeTraits<FNonBlendableIntegerAnimationAttribute> : public TAttributeTypeTraitsBase<FNonBlendableIntegerAnimationAttribute>
		{
			enum
			{
				IsBlendable = false,
			};
		};

#if WITH_EDITOR
		/** Helper functionality allowing the user to add an attribute with a typed value array */
		/** 帮助程序功能允许用户添加带有类型化值数组的属性 */
		template<typename AttributeType, typename ValueType>
		bool AddTypedCustomAttribute(const FName& AttributeName, const FName& BoneName, UAnimSequenceBase* AnimSequenceBase, TArrayView<const float> Keys, TArrayView<const ValueType> Values, bool bShouldTransact = true)
		{
			const FAnimationAttributeIdentifier Identifier = UAnimationAttributeIdentifierExtensions::CreateAttributeIdentifier(AnimSequenceBase, AttributeName, BoneName, AttributeType::StaticStruct());

			IAnimationDataController& Controller = AnimSequenceBase->GetController();
			if (AnimSequenceBase->GetDataModelInterface()->FindAttribute(Identifier) || Controller.AddAttribute(Identifier))
			{
				TArray<AttributeType> AttributeValues; 
				Algo::Transform(Values, AttributeValues, [](const ValueType& Value)
				{
					AttributeType Attribute;
					Attribute.Value = Value;
					return Attribute;
				});

				return Controller.SetTypedAttributeKeys<AttributeType>(Identifier, Keys, MakeArrayView(AttributeValues), bShouldTransact);
			}

			return false;
		}		
#endif // WITH_EDITOR
	}
}


UCLASS()
class UBuiltInAttributesExtensions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = Attributes, meta = (ScriptMethod))
	static bool AddTransformAttribute(UAnimSequenceBase* AnimSequenceBase, const FName& AttributeName, const FName& BoneName, const TArray<float>& Keys, const TArray<FTransform>& Values)
	{
		return UE::Anim::AddTypedCustomAttribute<FTransformAnimationAttribute, FTransform>(AttributeName, BoneName, AnimSequenceBase, MakeArrayView(Keys), MakeArrayView(Values));
	}
#endif // WITH_EDITOR
};
