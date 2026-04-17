// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimationTypes.h: Render core module definitions.
=============================================================================*/

#pragma once
#include "CoreMinimal.h"
#include "CommonAnimTypes.generated.h"

/** Axis to represent direction */
/** 代表方向的轴 */
USTRUCT()
struct FAxis
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "FAxis")
	FVector Axis;

	UPROPERTY(EditAnywhere, Category = "FAxis")
	bool bInLocalSpace;

	FAxis(const FVector& InAxis = FVector::ForwardVector)
		:  bInLocalSpace(true) 
	{
		Axis = InAxis.GetSafeNormal();
	};

	/** return transformed axis based on ComponentSpaceTransform */
	/** 返回基于 ComponentSpaceTransform 的变换轴 */
	FVector GetTransformedAxis(const FTransform& ComponentSpaceTransform) const
	{
		if (bInLocalSpace)
		{
			return ComponentSpaceTransform.TransformVectorNoScale(Axis);
		}

		// if world transform, we don't have to transform
		// 如果世界改变，我们不必改变
		return Axis;
	}

	/** Initialize the set up */
	/** 初始化设置 */
	void Initialize()
	{
		Axis = Axis.GetSafeNormal();
	}

	/** return true if Valid data */
	/** 如果数据有效则返回 true */
	bool IsValid() const
	{
		return Axis.IsNormalized();
	}

	friend FArchive & operator<<(FArchive & Ar, FAxis & D)
	{
		Ar << D.Axis;
		Ar << D.bInLocalSpace;

		return Ar;
	}
};

