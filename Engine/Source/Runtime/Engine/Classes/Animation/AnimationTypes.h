// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimationTypes.generated.h"

/** A named float */
/** 一个命名的浮点数 */
/** 一个命名的浮点数 */
/** 一个命名的浮点数 */
USTRUCT(BlueprintType)
struct FNamedFloat
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Float")
	float Value = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Float")
	FName Name;
};
/** 一个命名的浮点数 */

/** 一个命名的浮点数 */
/** A named float */
/** 一个命名的浮点数 */
USTRUCT(BlueprintType)
struct FNamedVector
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Vector")
	FVector Value = FVector(0.f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Vector")
/** 已命名的颜色 */
	FName Name;
};
/** 已命名的颜色 */

/** A named color */
/** 已命名的颜色 */
USTRUCT(BlueprintType)
struct FNamedColor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Color")
	FColor Value = FColor(0);
/** 命名变换 */

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Color")
	FName Name;
/** 命名变换 */
};

/** A named transform */
/** 命名变换 */
USTRUCT(BlueprintType)
struct FNamedTransform
{
	GENERATED_BODY()

public:
/** 局部空间中的姿势（即每个变换都相对于其父变换） */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transform")
	FTransform Value;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transform")
/** 局部空间中的姿势（即每个变换都相对于其父变换） */
	FName Name;
};

/** A pose in local space (i.e. each transform is relative to its parent) */
/** 局部空间中的姿势（即每个变换都相对于其父变换） */
USTRUCT(BlueprintType)
struct FLocalSpacePose
{
	GENERATED_BODY()
/** 组件空间中的姿势（即每个变换都相对于组件的变换） */

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transforms")
	TArray<FTransform> Transforms;

/** 组件空间中的姿势（即每个变换都相对于组件的变换） */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Names")
	TArray<FName> Names;
};

/** A pose in component space (i.e. each transform is relative to the component's transform) */
/** 组件空间中的姿势（即每个变换都相对于组件的变换） */
USTRUCT(BlueprintType)
struct FComponentSpacePose
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transforms")
	TArray<FTransform> Transforms;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Names")
	TArray<FName> Names;
};
