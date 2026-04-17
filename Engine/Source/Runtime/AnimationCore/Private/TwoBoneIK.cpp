// Copyright Epic Games, Inc. All Rights Reserved.

#include "TwoBoneIK.h"

namespace AnimationCore
{

void SolveTwoBoneIK(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, bool bAllowStretching, double StartStretchRatio, double MaxStretchScale)
{
	double LowerLimbLength = (InOutEndTransform.GetLocation() - InOutJointTransform.GetLocation()).Size();
	double UpperLimbLength = (InOutJointTransform.GetLocation() - InOutRootTransform.GetLocation()).Size();
	SolveTwoBoneIK(InOutRootTransform, InOutJointTransform, InOutEndTransform, JointTarget, Effector, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);
}

void SolveTwoBoneIK(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget, const FVector& Effector, double UpperLimbLength, double LowerLimbLength, bool bAllowStretching, double StartStretchRatio, double MaxStretchScale)
{
	FVector OutJointPos, OutEndPos;

	FVector RootPos = InOutRootTransform.GetLocation();
	FVector JointPos = InOutJointTransform.GetLocation();
	FVector EndPos = InOutEndTransform.GetLocation();

	// IK solver
 // IK解算器
	AnimationCore::SolveTwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, OutJointPos, OutEndPos, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);

	// Update transform for upper bone.
 // 更新上部骨骼的变换。
	{
		// Get difference in direction for old and new joint orientations
  // 获得新旧关节方向的方向差异
		FVector const OldDir = (JointPos - RootPos).GetSafeNormal();
		FVector const NewDir = (OutJointPos - RootPos).GetSafeNormal();
		// Find Delta Rotation take takes us from Old to New dir
  // 查找 Delta Rotation 将我们从旧目录带到新目录
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation
  // 通过这个增量旋转来旋转我们的联合四元数
		InOutRootTransform.SetRotation(DeltaRotation * InOutRootTransform.GetRotation());
		// And put joint where it should be.
  // 并将关节放在应该的位置。
		InOutRootTransform.SetTranslation(RootPos);

	}

	// update transform for middle bone
 // 更新中间骨骼的变换
	{
		// Get difference in direction for old and new joint orientations
  // 获得新旧关节方向的方向差异
		FVector const OldDir = (EndPos - JointPos).GetSafeNormal();
		FVector const NewDir = (OutEndPos - OutJointPos).GetSafeNormal();

		// Find Delta Rotation take takes us from Old to New dir
  // 查找 Delta Rotation 将我们从旧目录带到新目录
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation
  // 通过这个增量旋转来旋转我们的联合四元数
		InOutJointTransform.SetRotation(DeltaRotation * InOutJointTransform.GetRotation());
		// And put joint where it should be.
  // 并将关节放在应该的位置。
		InOutJointTransform.SetTranslation(OutJointPos);

	}

	// Update transform for end bone.
 // 更新末端骨骼的变换。
	// currently not doing anything to rotation
 // 目前没有对轮换做任何事情
	// keeping input rotation
 // 保持输入旋转
	// Set correct location for end bone.
 // 设置端骨的正确位置。
	InOutEndTransform.SetTranslation(OutEndPos);
}

void SolveTwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, bool bAllowStretching, double StartStretchRatio, double MaxStretchScale)
{
	const double LowerLimbLength = (EndPos - JointPos).Size();
	const double UpperLimbLength = (JointPos - RootPos).Size();

	SolveTwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, OutJointPos, OutEndPos, UpperLimbLength, LowerLimbLength, bAllowStretching, StartStretchRatio, MaxStretchScale);
}

void SolveTwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget, const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, double UpperLimbLength, double LowerLimbLength, bool bAllowStretching, double StartStretchRatio, double MaxStretchScale)
{
	// This is our reach goal.
 // 这是我们达到的目标。
	FVector DesiredPos = Effector;
	FVector DesiredDelta = DesiredPos - RootPos;
	double DesiredLength = DesiredDelta.Size();

	// Find lengths of upper and lower limb in the ref skeleton.
 // 在参考骨骼中查找上肢和下肢的长度。
	// Use actual sizes instead of ref skeleton, so we take into account translation and scaling from other bone controllers.
 // 使用实际尺寸而不是参考骨架，因此我们考虑了其他骨骼控制器的平移和缩放。
	double MaxLimbLength = LowerLimbLength + UpperLimbLength;

	// Check to handle case where DesiredPos is the same as RootPos.
 // 检查以处理 DesiredPos 与 RootPos 相同的情况。
	FVector	DesiredDir;
	if (DesiredLength < DOUBLE_KINDA_SMALL_NUMBER)
	{
		DesiredLength = DOUBLE_KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1, 0, 0);
	}
	else
	{
		DesiredDir = DesiredDelta.GetSafeNormal();
	}

	// Get joint target (used for defining plane that joint should be in).
 // 获取关节目标（用于定义关节所在的平面）。
	FVector JointTargetDelta = JointTarget - RootPos;
	const double JointTargetLengthSqr = JointTargetDelta.SizeSquared();

	// Same check as above, to cover case when JointTarget position is the same as RootPos.
 // 与上面相同的检查，以覆盖当 JointTarget 位置与 RootPos 相同时的情况。
	FVector JointPlaneNormal, JointBendDir;
	if (JointTargetLengthSqr < FMath::Square(DOUBLE_KINDA_SMALL_NUMBER))
	{
		JointBendDir = FVector(0, 1, 0);
		JointPlaneNormal = FVector(0, 0, 1);
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;

		// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
  // 如果我们试图将肢体指向与我们应该移动关节的方向相同的方向，
		// we have to just pick 2 random vector perp to DesiredDir and each other.
  // 我们只需为 DesiredDir 和彼此选择 2 个随机向量 perp。
		if (JointPlaneNormal.SizeSquared() < FMath::Square(DOUBLE_KINDA_SMALL_NUMBER))
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();

			// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
   // 通过沿 DesiredDir 删除 JointTargetDelta 的任何组件，找到参考系的最终成员。
			// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
   // 这永远不应该留下零向量，因为我们已经检查过 DesiredDir 和 JointTargetDelta 不平行。
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
			JointBendDir.Normalize();
		}
	}

	//UE_LOG(LogAnimationCore, Log, TEXT("UpperLimb : %0.2f, LowerLimb : %0.2f, MaxLimb : %0.2f"), UpperLimbLength, LowerLimbLength, MaxLimbLength);
 // UE_LOG(LogAnimationCore, Log, TEXT("上肢: %0.2f, 下肢: %0.2f, MaxLimb: %0.2f"), UpperLimbLength, LowerLimbLength, MaxLimbLength);

	if (bAllowStretching)
	{
		const double ScaleRange = MaxStretchScale - StartStretchRatio;
		if (ScaleRange > DOUBLE_KINDA_SMALL_NUMBER && MaxLimbLength > DOUBLE_KINDA_SMALL_NUMBER)
		{
			const double ReachRatio = DesiredLength / MaxLimbLength;
			const double ScalingFactor = (MaxStretchScale - 1.0) * FMath::Clamp((ReachRatio - StartStretchRatio) / ScaleRange, 0.0, 1.0);
			if (ScalingFactor > DOUBLE_KINDA_SMALL_NUMBER)
			{
				LowerLimbLength *= (1.0 + ScalingFactor);
				UpperLimbLength *= (1.0 + ScalingFactor);
				MaxLimbLength *= (1.0 + ScalingFactor);
			}
		}
	}

	OutEndPos = DesiredPos;
	OutJointPos = JointPos;

	// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
 // 如果我们试图达到超出肢体长度的目标，请将其限制在可解决的目标上并完全延伸肢体。
	if (DesiredLength >= MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
		OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
	}
	else
	{
		// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
  // 所以我们有一个已知边长的三角形。我们可以算出DesiredDir与上肢方向的夹角
		// using the sin rule:
  // 使用罪恶规则：
		const double TwoAB = 2.0 * UpperLimbLength * DesiredLength;

		const double CosAngle = (TwoAB != 0.0) ? ((UpperLimbLength*UpperLimbLength) + (DesiredLength*DesiredLength) - (LowerLimbLength*LowerLimbLength)) / TwoAB : 0.0;

		// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
  // 如果 CosAngle 小于 0，则上臂实际上指向 DesiredDir 的相反方向，因此我们对此进行处理。
		const bool bReverseUpperBone = (CosAngle < 0.0);

		// Angle between upper limb and DesiredDir
  // 上肢与 DesiredDir 之间的角度
		// ACos clamps internally so we dont need to worry about out-of-range values here.
  // ACos 在内部进行钳位，因此我们无需担心此处的值超出范围。
		const double Angle = FMath::Acos(CosAngle);

		// Now we calculate the distance of the joint from the root -> effector line.
  // 现在我们计算关节距根 -> 效应器线的距离。
		// This forms a right-angle triangle, with the upper limb as the hypotenuse.
  // 这形成一个直角三角形，上肢为斜边。
		const double JointLineDist = UpperLimbLength * FMath::Sin(Angle);

		// And the final side of that triangle - distance along DesiredDir of perpendicular.
  // 该三角形的最后一条边 - 沿 DesiredDir 垂直的距离。
		// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
  // ProjJointDistSqr 不能为负数，因为 JointLineDist 必须 <= UpperLimbLength，因为 appSin(Angle) <= 1。
		const double ProjJointDistSqr = (UpperLimbLength*UpperLimbLength) - (JointLineDist*JointLineDist);
		// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
  // 虽然这不应该是负数，但有时 Xbox 版本会产生 -0.f，导致 ProjJointDist 为 NaN
		// so now I branch it. 						
  // 所以现在我分支它。
		double ProjJointDist = (ProjJointDistSqr > 0.0) ? FMath::Sqrt(ProjJointDistSqr) : 0.0;
		if (bReverseUpperBone)
		{
			ProjJointDist *= -1.f;
		}

		// So now we can work out where to put the joint!
  // 现在我们可以找出将关节放置在哪里了！
		OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
	}
}

}