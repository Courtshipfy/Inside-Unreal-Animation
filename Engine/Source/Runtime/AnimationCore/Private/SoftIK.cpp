// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoftIK.h"

namespace AnimationCore
{
	void SoftenIKEffectorPosition(
		const FVector& RootLocation,
		const float TotalChainLength,
		const float SoftLengthPercent,
		const float Alpha,
		FVector& InOutEffectorPosition)
	{
		if (FMath::Abs(1.0f - SoftLengthPercent) < KINDA_SMALL_NUMBER)
		{
			// soft length near zero
			// 软长度接近于零
			return;
		}

		if (TotalChainLength <= KINDA_SMALL_NUMBER)
		{
			// chain length near zero
			// 链长接近于零
			return;
		}
		
		// get vector from root of chain to effector (pre adjusted)
		// 获取从链根到效应器的向量（预先调整）
		FVector StartToEffector = InOutEffectorPosition - RootLocation;
		float CurrentLength;
		StartToEffector.ToDirectionAndLength(StartToEffector, CurrentLength);

		// convert percentage to distance
		// 将百分比转换为距离
		const float SoftDistance = TotalChainLength * (1.0f - FMath::Min(1.0f, SoftLengthPercent));
		const float HardLength = TotalChainLength - SoftDistance;
		const float CurrentDelta = CurrentLength - HardLength;
		if (CurrentDelta <= KINDA_SMALL_NUMBER || SoftDistance <= KINDA_SMALL_NUMBER)
		{
			// not in the soft zone
			// 不在软区
			return;
		}

		// calculate the "softened" length of the effector
		// 计算效应器的“软化”长度
		const float PercentIntoSoftLength = CurrentDelta / SoftDistance;
		const float SoftenedLength = HardLength + SoftDistance * (1.0 - FMath::Exp(-PercentIntoSoftLength));

		// apply the new effector location
		// 应用新的效应器位置
		float FinalAlpha = FMath::Clamp(Alpha, 0.0, 1.0f);
		if (FinalAlpha < 1.0f)
		{
			// alpha blend the softness (optional)
			// alpha 混合柔软度（可选）
			const float MaxLength = FMath::Min(CurrentLength, TotalChainLength);
			const float AlphaBlendedLength = FMath::Lerp(MaxLength, SoftenedLength, Alpha);
			InOutEffectorPosition = RootLocation + StartToEffector * AlphaBlendedLength;
		}
		else
		{
			// use the soft position directly
			// 直接使用软位置
			InOutEffectorPosition = RootLocation + StartToEffector * SoftenedLength;
		}
	}
}
