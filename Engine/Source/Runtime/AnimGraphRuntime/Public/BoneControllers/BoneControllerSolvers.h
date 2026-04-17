// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "Engine/SpringInterpolator.h"
#include "Math/Transform.h"
#include "Math/UnrealMathSSE.h"
#include "UObject/ObjectMacros.h"

#include "BoneControllerSolvers.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct FIKFootPelvisPullDownSolver
{
	GENERATED_BODY()

	// Specifies the spring interpolation parameters applied during pelvis adjustment
	// 指定骨盆调整期间应用的弹簧插值参数
	UPROPERTY(EditAnywhere, Category=Settings)
	FVectorRK4SpringInterpolator PelvisAdjustmentInterp;

	// Specifies an alpha between the original and final adjusted pelvis locations
	// 指定原始骨盆位置和最终调整后的骨盆位置之间的 Alpha
	// This is used to retain some degree of the original pelvis motion
	// 这用于保留一定程度的原始骨盆运动
	UPROPERTY(EditAnywhere, Category=Settings, meta=(ClampMin="0.0", ClampMax="1.0"))
	double PelvisAdjustmentInterpAlpha = 0.5;

	// Specifies the maximum displacement the pelvis can be adjusted relative to its original location
	// 指定骨盆相对于其原始位置可调整的最大位移
	UPROPERTY(EditAnywhere, Category=Settings, meta=(ClampMin="0.0"))
	double PelvisAdjustmentMaxDistance = 10.0;

	// Specifies the pelvis adjustment distance error that is tolerated for each iteration of the solver
	// 指定解算器每次迭代所容忍的骨盆调整距离误差
	// 
	// When it is detected that the pelvis adjustment distance is incrementing at a value lower or equal
	// 当检测到骨盆调整距离以低于或等于的值递增时
	// to this value for each iteration, the solve will halt. Lower values will marginally increase visual
	// 每次迭代达到此值时，求解将停止。较低的值会稍微增加视觉效果
	// quality at the cost of performance, but may require a higher PelvisAdjustmentMaxIter to be specified
	// 以性能为代价的质量，但可能需要指定更高的 PelvisAdjustmentMaxIter
	//
	// The default value of 0.01 specifies 1 centimeter of error
	// 默认值 0.01 指定 1 厘米的误差
	UPROPERTY(EditAnywhere, Category=Advanced, meta=(ClampMin="0.001"))
	double PelvisAdjustmentErrorTolerance = 0.01;

	// Specifies the maximum number of iterations to run for the pelvis adjustment solver
	// 指定骨盆调整解算器运行的最大迭代次数
	// Higher iterations will guarantee closer PelvisAdjustmentErrorTolerance convergence at the cost of performance
	// 更高的迭代将保证更接近的 PelvisAdjustmentErrorTolerance 收敛，但会牺牲性能
	UPROPERTY(EditAnywhere, Category=Advanced, meta=(ClampMin="0"))
	int32 PelvisAdjustmentMaxIter = 3;

	// Iteratively pulls the character pelvis towards the ground based on the relationship of driven IK foot targets versus FK foot limits
	// 根据驱动 IK 脚部目标与 FK 脚部限制的关系，迭代地将角色骨盆拉向地面
	ANIMGRAPHRUNTIME_API FTransform Solve(FTransform PelvisTransform, TArrayView<const float> FKFootDistancesToPelvis, TArrayView<const FVector> IKFootLocations, float DeltaTime);
};
