// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Math/MathFwd.h"
#include "Math/Quat.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "UObject/ObjectMacros.h"

#include "TransformNoScale.generated.h"

USTRUCT(BlueprintType)
struct FTransformNoScale
{
	GENERATED_BODY()

	/**
	 * The identity transformation (Rotation = FRotator::ZeroRotator, Translation = FVector::ZeroVector, Scale = (1,1,1)).
	 */
	static ANIMATIONCORE_API const FTransformNoScale Identity;

	inline FTransformNoScale()
		: Location(ForceInitToZero)
		, Rotation(ForceInitToZero)
	{
	}

	inline FTransformNoScale(const FVector& InLocation, const FQuat& InRotation)
		: Location(InLocation)
		, Rotation(InRotation)
	{
	}

	inline FTransformNoScale(const FTransform& InTransform)
		: Location(InTransform.GetLocation())
		, Rotation(InTransform.GetRotation())
	{
	}

	inline FTransformNoScale& operator =(const FTransform& InTransform)
	{
		FromFTransform(InTransform);
		return *this;
	}

	inline operator FTransform() const
	{
		return ToFTransform();
	}

	/** The translation of this transform */
	/** 这个变换的翻译 */
	/** 这个变换的翻译 */
	/** 这个变换的翻译 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transform")
	FVector Location;
	/** 这个变换的旋转 */

	/** 这个变换的旋转 */
	/** The rotation of this transform */
	/** 这个变换的旋转 */
	/** 转换为 FTransform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transform")
	FQuat Rotation;
	/** 转换为 FTransform */

	/** Convert to an FTransform */
	/** 转换为 FTransform */
	/** 从 FTransform 转换 */
	inline FTransform ToFTransform() const
	{
		return FTransform(Rotation, Location, FVector::OneVector);
	/** 从 FTransform 转换 */
	}

	/** Convert from an FTransform */
	/** 从 FTransform 转换 */
	inline void FromFTransform(const FTransform& InTransform)
	{
		Location = InTransform.GetLocation();
		Rotation = InTransform.GetRotation();
	}
};
