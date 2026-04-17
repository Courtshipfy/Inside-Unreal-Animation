// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/BoneControllerSolvers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BoneControllerSolvers)

FTransform FIKFootPelvisPullDownSolver::Solve(FTransform PelvisTransform, TArrayView<const float> FKFootDistancesToPelvis, TArrayView<const FVector> IKFootLocations, float DeltaTime)
{
	const FVector InitialPelvisLocation = PelvisTransform.GetLocation();
	
	const int32 IKFootLocationsCount = IKFootLocations.Num();
	const int32 FKFootDistancesCount = FKFootDistancesToPelvis.Num();
	check(IKFootLocationsCount > 0);
	check(FKFootDistancesCount > 0);
	check(FKFootDistancesCount == IKFootLocationsCount);

	FVector AdjustedPelvisLocation = InitialPelvisLocation;
	FVector DeltaAdjustment = FVector::ZeroVector;

	const double PerFootWeight = 1.0 / static_cast<double>(IKFootLocationsCount);
	const double AdjustmentDistMaxSquared = FMath::Pow(PelvisAdjustmentMaxDistance, 2.0);

	// Pull pelvis closer to feet iteratively
 // 反复将骨盆拉近脚部
	for (int32 Iter = 0; Iter < PelvisAdjustmentMaxIter; ++Iter)
	{
		const FVector PreAdjustmentLocation = AdjustedPelvisLocation;
		AdjustedPelvisLocation = FVector::ZeroVector;

		// Apply pelvis adjustment contributions from all IK/FK foot chains
  // 应用所有 IK/FK 脚链的骨盆调整贡献
		for (int32 Index = 0; Index < IKFootLocationsCount; ++Index)
		{
			const FVector IdealLocation = IKFootLocations[Index] + (PreAdjustmentLocation - IKFootLocations[Index]).GetSafeNormal() * FKFootDistancesToPelvis[Index];
			AdjustedPelvisLocation += (IdealLocation * PerFootWeight);
		}

		const FVector PrevDeltaAdjustment = DeltaAdjustment;
		DeltaAdjustment = AdjustedPelvisLocation - InitialPelvisLocation;
		const double DeltaAdjustmentDist = FVector::Dist(PrevDeltaAdjustment, DeltaAdjustment);

		// Keep track of how much delta adjustment is being applied per iteration
  // 跟踪每次迭代应用了多少增量调整
		if (DeltaAdjustmentDist <= PelvisAdjustmentErrorTolerance)
		{
			break;
		}
	}

	// Apply spring between initial and adjusted spring location to smooth out change over time
 // 在初始位置和调整后的弹簧位置之间应用弹簧，以平滑随时间的变化
	//PelvisAdjustmentInterp
 // 骨盆调整Interp
	PelvisAdjustmentInterp.Update(DeltaAdjustment, DeltaTime);

	// Apply an alpha with the initial pelvis location, to retain some of the original motion
 // 应用 alpha 与初始骨盆位置，以保留一些原始运动
	AdjustedPelvisLocation = InitialPelvisLocation + FMath::Lerp(FVector::ZeroVector, PelvisAdjustmentInterp.GetPosition(), PelvisAdjustmentInterpAlpha);

	// Guarantee that we don't over-adjust the pelvis beyond the specified distance tolerance
 // 保证我们不会过度调整骨盆超出规定的距离公差
	if (FVector::DistSquared(AdjustedPelvisLocation, InitialPelvisLocation) >= AdjustmentDistMaxSquared)
	{
		DeltaAdjustment = AdjustedPelvisLocation - InitialPelvisLocation;
		AdjustedPelvisLocation = InitialPelvisLocation + DeltaAdjustment.GetSafeNormal() * PelvisAdjustmentMaxDistance;
	}
	
	PelvisTransform.SetLocation(AdjustedPelvisLocation);
	return PelvisTransform;
}
