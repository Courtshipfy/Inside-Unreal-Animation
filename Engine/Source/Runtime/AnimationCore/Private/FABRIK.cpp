// Copyright Epic Games, Inc. All Rights Reserved.

#include "FABRIK.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FABRIK)

namespace AnimationCore
{
	/////////////////////////////////////////////////////
	// Implementation of the FABRIK IK Algorithm
 // FABRIK IK 算法的实现
	// Please see http://andreasaristidou.com/publications/FABRIK.pdf for more details
 // 请参阅 http://andreasaristidou.com/publications/FABRIK.pdf 了解更多详情

	bool SolveFabrik(TArray<FFABRIKChainLink>& InOutChain, const FVector& TargetPosition, double MaximumReach, double Precision, int32 MaxIterations)
	{
		bool bBoneLocationUpdated = false;
		double const RootToTargetDistSq = FVector::DistSquared(InOutChain[0].Position, TargetPosition);
		int32 const NumChainLinks = InOutChain.Num();

		// FABRIK algorithm - bone translation calculation
  // FABRIK算法-骨骼平移计算
		// If the effector is further away than the distance from root to tip, simply move all bones in a line from root to effector location
  // 如果效应器距离根部到尖端的距离更远，只需将所有骨骼从根部移动到效应器位置即可
		if (RootToTargetDistSq > FMath::Square(MaximumReach))
		{
			for (int32 LinkIndex = 1; LinkIndex < NumChainLinks; LinkIndex++)
			{
				FFABRIKChainLink const & ParentLink = InOutChain[LinkIndex - 1];
				FFABRIKChainLink & CurrentLink = InOutChain[LinkIndex];
				CurrentLink.Position = ParentLink.Position + (TargetPosition - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length;
			}
			bBoneLocationUpdated = true;
		}
		else // Effector is within reach, calculate bone translations to position tip at effector location
		{
			int32 const TipBoneLinkIndex = NumChainLinks - 1;

			// Check distance between tip location and effector location
   // 检查尖端位置和执行器位置之间的距离
			double Slop = FVector::Dist(InOutChain[TipBoneLinkIndex].Position, TargetPosition);
			if (Slop > Precision)
			{
				// Set tip bone at end effector location.
    // 将尖端骨骼设置在末端执行器位置。
				InOutChain[TipBoneLinkIndex].Position = TargetPosition;

				int32 IterationCount = 0;
				while ((Slop > Precision) && (IterationCount++ < MaxIterations))
				{
					// "Forward Reaching" stage - adjust bones from end effector.
     // “向前伸展”阶段 - 调整末端执行器的骨骼。
					for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
					{
						FFABRIKChainLink & CurrentLink = InOutChain[LinkIndex];
						FFABRIKChainLink const & ChildLink = InOutChain[LinkIndex + 1];

						CurrentLink.Position = ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length;
					}

					// "Backward Reaching" stage - adjust bones from root.
     // “后伸”阶段——从根部调整骨骼。
					for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
					{
						FFABRIKChainLink const & ParentLink = InOutChain[LinkIndex - 1];
						FFABRIKChainLink & CurrentLink = InOutChain[LinkIndex];

						CurrentLink.Position = ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length;
					}

					// Re-check distance between tip location and effector location
     // 重新检查尖端位置和执行器位置之间的距离
					// Since we're keeping tip on top of effector location, check with its parent bone.
     // 由于我们将尖端保持在效应器位置的顶部，因此请检查其父骨骼。
					Slop = FMath::Abs(InOutChain[TipBoneLinkIndex].Length - FVector::Dist(InOutChain[TipBoneLinkIndex - 1].Position, TargetPosition));
				}

				// Place tip bone based on how close we got to target.
    // 根据我们距离目标的距离放置尖端骨骼。
				{
					FFABRIKChainLink const & ParentLink = InOutChain[TipBoneLinkIndex - 1];
					FFABRIKChainLink & CurrentLink = InOutChain[TipBoneLinkIndex];

					CurrentLink.Position = ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length;
				}

				bBoneLocationUpdated = true;
			}
		}

		return bBoneLocationUpdated;
	}
}

