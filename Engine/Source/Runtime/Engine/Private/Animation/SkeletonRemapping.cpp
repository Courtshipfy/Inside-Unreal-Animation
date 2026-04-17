// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/SkeletonRemapping.h"

#include "AnimationRuntime.h"
#include "Animation/Skeleton.h"

FSkeletonRemapping::FSkeletonRemapping(const USkeleton* InSourceSkeleton, const USkeleton* InTargetSkeleton)
	: SourceSkeleton(InSourceSkeleton)
	, TargetSkeleton(InTargetSkeleton)
{
	GenerateMapping();
}

void FSkeletonRemapping::RegenerateMapping()
{
	SourceToTargetBoneIndexes.Reset();
	TargetToSourceBoneIndexes.Reset();
	RetargetingTable.Reset();

	GenerateMapping();
}

void FSkeletonRemapping::GenerateMapping()
{
	if (!GetSourceSkeleton().IsValid() || !GetTargetSkeleton().IsValid())
	{
		return;
	}
	
	const FReferenceSkeleton& SourceReferenceSkeleton = SourceSkeleton->GetReferenceSkeleton();
	const FReferenceSkeleton& TargetReferenceSkeleton = TargetSkeleton->GetReferenceSkeleton();

	const int32 SourceNumBones = SourceReferenceSkeleton.GetNum();
	const int32 TargetNumBones = TargetReferenceSkeleton.GetNum();

	SourceToTargetBoneIndexes.SetNumUninitialized(SourceNumBones);
	TargetToSourceBoneIndexes.SetNumUninitialized(TargetNumBones);
	RetargetingTable.SetNumUninitialized(TargetNumBones);

	TArrayView<const FTransform> SourceLocalTransforms;
	TArrayView<const FTransform> TargetLocalTransforms;
/*
	// Build the mapping from source to target bones through the remapping rig if one exists
 // 通过重映射装备（如果存在）构建从源骨骼到目标骨骼的映射
	if (true)	// TODO_SKELETON_REMAPPING: Use a remapping rig if it exists
	{
		for (int32 SourceBoneIndex = 0; SourceBoneIndex < SourceNumBones; ++SourceBoneIndex)
		{
			const FName BoneName = SourceReferenceSkeleton.GetBoneName(SourceBoneIndex);
			int32 TargetBoneIndex = TargetReferenceSkeleton.FindBoneIndex(BoneName);
			SourceToTargetBoneIndexes[SourceBoneIndex] = TargetBoneIndex;
		}

		// Get the matched source and target rest poses from the remapping rig
  // 从重映射装备中获取匹配的源和目标静止姿势
		//
		// TODO_SKELETON_REMAPPING: For now we'll just use the skeleton's rest poses, but we should really be pulling these
  // TODO_SKELETON_REMAPPING：现在我们只使用骨架的休息姿势，但我们真的应该拉这些
		// poses from a remapping rig so that the use can better align the poses.  Note that we'll want the remapping rig
  // 来自重新映射装备的姿势，以便用户可以更好地对齐姿势。  请注意，我们需要重新映射装备
		// to be independent from the USkeleton itself because a skeleton may want to participate in multiple remappings with
  // 独立于 USkeleton 本身，因为骨架可能希望参与多个重新映射
		// different other skeletons and may have to pose itself differently to align with the different other skeletons
  // 不同的其他骨骼，并且可能必须以不同的姿势与不同的其他骨骼对齐
		//
		SourceLocalTransforms = MakeArrayView(SourceReferenceSkeleton.GetRefBonePose());
		TargetLocalTransforms = MakeArrayView(TargetReferenceSkeleton.GetRefBonePose());
	}
	else // Fall back to simple name matching if there is no rig
*/
	{
		// Match source to target bones by name lookup between source and target skeletons
  // 通过源骨骼和目标骨骼之间的名称查找将源骨骼与目标骨骼匹配
		for (int32 SourceBoneIndex = 0; SourceBoneIndex < SourceNumBones; ++SourceBoneIndex)
		{
			const FName BoneName = SourceReferenceSkeleton.GetBoneName(SourceBoneIndex);
			const int32 TargetBoneIndex = TargetReferenceSkeleton.FindBoneIndex(BoneName);
			SourceToTargetBoneIndexes[SourceBoneIndex] = TargetBoneIndex;
		}

		// Get the matched (hopefully) source and target rest poses from the source and target skeletons
  // 从源骨骼和目标骨骼中获取匹配的（希望）源和目标静止姿势
		SourceLocalTransforms = MakeArrayView(SourceReferenceSkeleton.GetRefBonePose());
		TargetLocalTransforms = MakeArrayView(TargetReferenceSkeleton.GetRefBonePose());
	}

	// Force the roots to map onto each other regardless of their names
 // 强制根相互映射，无论其名称如何
	SourceToTargetBoneIndexes[0] = 0;

	// Build the reverse mapping from target back to source bones
 // 构建从目标骨骼到源骨骼的反向映射
	FMemory::Memset(TargetToSourceBoneIndexes.GetData(), static_cast<uint8>(INDEX_NONE), TargetNumBones * TargetToSourceBoneIndexes.GetTypeSize());
	for (int32 SourceBoneIndex = 0; SourceBoneIndex < SourceNumBones; ++SourceBoneIndex)
	{
		const int32 TargetBoneIndex = SourceToTargetBoneIndexes[SourceBoneIndex];
		if (TargetBoneIndex != INDEX_NONE)
		{
			TargetToSourceBoneIndexes[TargetBoneIndex] = SourceBoneIndex;
		}
	}

	TArray<FTransform> SourceComponentTransforms;
	TArray<FTransform> TargetComponentTransforms;
	//		Q = P * D * R
 // Q = P * D * R
	FAnimationRuntime::FillUpComponentSpaceTransforms(SourceReferenceSkeleton, SourceLocalTransforms, SourceComponentTransforms);
	FAnimationRuntime::FillUpComponentSpaceTransforms(TargetReferenceSkeleton, TargetLocalTransforms, TargetComponentTransforms);

	// Calculate the retargeting constants to map from source skeleton space to target skeleton space
 // 计算从源骨架空间映射到目标骨架空间的重定向常量
	//
	// Simply remapping joint indices is usually not sufficient to give us the desired result pose if the source and target
 // 如果源和目标是简单地重新映射关节索引通常不足以为我们提供所需的结果姿势
	// skeletons are built with different conventions for joint orientations.  We therefore need to compute a remapping
 // 骨架是按照不同的关节方向惯例构建的。  因此我们需要计算一个重映射
	// between the source and target joint orientations, which we do in terms of delta rotations from the rest pose:
 // 在源关节方向和目标关节方向之间，我们根据静止姿势的增量旋转来实现：
 // Q = P * D * R
 // Q = P * D * R
 // Q = P * D * R
 // Q = P * D * R
	//		Ps * Ds * Rs --> Pt * Dt * Rt
 // Ps * Ds * Rs --> Pt * Dt * Rt
	//		Q = P * D * R
 // Q = P * D * R
	//
	//		Q = P * D * R
 // Q = P * D * R
	//		Q = P * D * R
 // Q = P * D * R
	//
	// where:
 // 在哪里：
	//		C * P = P * D
 // C * P = P * D
	//
	//		Q is the final joint orientation in component space
 // Q 是组件空间中的最终关节方向
 // Ps * Ds * Rs --> Pt * Dt * Rt
 // Ps * Ds * Rs --> Pt * Dt * Rt
	//		P is the parent joint orientation in component space
 // P 是组件空间中的父关节方向
 // Ps * Ds * Rs --> Pt * Dt * Rt
 // Ps * Ds * Rs --> Pt * Dt * Rt
	//		Ps * Ds * Ps⁻1 = Pt * Dt * Pt⁻1
	//		D is the delta rotation that we want to remap
 // D 是我们要重新映射的增量旋转
	//		Ps * Ds * Rs --> Pt * Dt * Rt
 // Ps * Ds * Rs --> Pt * Dt * Rt
	//		R is the local space rest pose orientation for the joint
 // R 是关节的局部空间静止姿势方向
 // C * P = P * D
 // C * P = P * D
	//		Dt = Pt⁻1 * Ps * Ds * Ps⁻1 * Pt
	//
 // C * P = P * D
 // C * P = P * D
	// We want to find a mapping from the source to the target:
 // 我们想要找到从源到目标的映射：
	//
 // Ps * Ds * Ps⁻1 = Pt * Dt * Pt⁻1
	//		L = D * R
 // L = D * R
	//		C * P = P * D
 // C * P = P * D
	//		Ps * Ds * Rs  -->  Pt * Dt * Rt
 // Ps * Ds * Rs --> Pt * Dt * Rt
 // Ps * Ds * Ps⁻1 = Pt * Dt * Pt⁻1
 // Dt = Pt⁻1 * Ps * Ds * Ps⁻1 * Pt
	//		Ps * Ds * Rs --> Pt * Dt * Rt
 // Ps * Ds * Rs --> Pt * Dt * Rt
	//
	// such that the deltas produce equivalent rotations on the mesh even if their parent rotation frames are different.
 // 这样，即使它们的父旋转坐标系不同，增量也会在网格上产生等效的旋转。
	//		Lt * Rt⁻1 = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt
 // Dt = Pt⁻1 * Ps * Ds * Ps⁻1 * Pt
 // L = D * R
 // L = D * R
	//		Ps * Ds * Ps⁻1 = Pt * Dt * Pt⁻1
	// In other words, we need to find Dt such that its component space rotation is equivalent to Ds.  To convert a rotation
 // 换句话说，我们需要找到Dt，使其分量空间旋转等于Ds。  转换旋转
	// from local space (D) to component space (C), given its parent (P), we have:
 // 从局部空间 (D) 到组件空间 (C)，给定其父空间 (P)，我们有：
	//		Lt = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt * Rt
	//
 // Lt * Rt⁻1 = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt
 // L = D * R
 // L = D * R
	//		Dt = Pt⁻1 * Ps * Ds * Ps⁻1 * Pt
	//		C * P = P * D
 // C * P = P * D
	//		C * P = P * D
 // C * P = P * D
 // Lt = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt * Rt
	//		C = P * D * P⁻¹		(this uses the sandwich product to rotate the rotation axis from local to mesh space)
 // C = P * D * P⁻1（这使用夹层产品将旋转轴从局部空间旋转到网格空间）
	//		Lt = Q0 * Ls * Q1
 // Lt = Q0 * Ls * Q1
 // Lt * Rt⁻1 = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt
	//		Q0 = Pt⁻1 * Ps
 // Lt = Q0 * Ls * Q1
 // Lt = Q0 * Ls * Q1
	//		L = D * R
 // L = D * R
 // Q0 = Pt⁻1 * Ps
	//
 // Q1 = Rs⁻1 * Ps⁻1 * Pt * Rt
	//		Q1 = Rs⁻1 * Ps⁻1 * Pt * Rt
 // Lt = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt * Rt
	// Setting the source and target component space rotations to be equal (Cs = Ct) then gives us:
 // 将源组件空间旋转和目标组件空间旋转设置为相等 (Cs = Ct)，然后我们可以得到：
	//
	//		Ps * Ds * Ps⁻¹ = Pt * Dt * Pt⁻¹
 // Ps * Ds * Ps⁻1 = Pt * Dt * Pt⁻1
	//		Lt * Rt⁻1 = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt
 // Lt = Q0 * Ls * Q1
 // Lt = Q0 * Ls * Q1
	//		Ps * Ds * Ps⁻1 = Pt * Dt * Pt⁻1
 // Q0 = Pt⁻1 * Ps
	//
 // Q1 = Rs⁻1 * Ps⁻1 * Pt * Rt
	// which we then solve for Dt to get:
 // 然后我们求解 Dt 得到：
	//
	//		Lt = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt * Rt
	//		Dt = Pt⁻¹ * Ps * Ds * Ps⁻¹ * Pt
 // Dt = Pt⁻1 * Ps * Ds * Ps⁻1 * Pt
	//		Dt = Pt⁻1 * Ps * Ds * Ps⁻1 * Pt
	//
	// However, when we're remapping an animation pose, we will have the local transforms rather than the deltas from the
 // 然而，当我们重新映射动画姿势时，我们将获得局部变换而不是来自
	//		Lt = Q0 * Ls * Q1
 // Lt = Q0 * Ls * Q1
	// rest pose, so we also need to convert between these local transforms and the equivalent deltas:
 // 休息姿势，所以我们还需要在这些局部变换和等效的增量之间进行转换：
	//		Q0 = Pt⁻1 * Ps
	//
	//		Q1 = Rs⁻1 * Ps⁻1 * Pt * Rt
	//		L = D * R
 // L = D * R
	//		D = L * R⁻¹
	//
	// Combining that with our equation for Dt, we get:
 // 将其与 Dt 方程相结合，我们得到：
	//
	//		Lt * Rt⁻¹ = Pt⁻¹ * Ps * Ls * Rs⁻¹ * Ps⁻¹ * Pt
 // Lt * Rt⁻1 = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt
	//		Lt * Rt⁻1 = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt
	//
	// Solving for Lt then gives us:
 // 求解 Lt 即可得出：
	//
	//		Lt = Pt⁻¹ * Ps * Ls * Rs⁻¹ * Ps⁻¹ * Pt * Rt
 // Lt = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt * Rt
	//		Lt = Pt⁻1 * Ps * Ls * Rs⁻1 * Ps⁻1 * Pt * Rt
	//
	// Finally, factoring out the constant terms (which we pre-compute here) gives us:
 // 最后，分解出常数项（我们在这里预先计算）得出：
	//
	//		Lt = Q0 * Ls * Q1
 // Lt = Q0 * Ls * Q1
	//		Lt = Q0 * Ls * Q1
 // Lt = Q0 * Ls * Q1
	//		Q0 = Pt⁻¹ * Ps
 // Q0 = Pt⁻1 * Ps
	//		Q0 = Pt⁻1 * Ps
	//		Q1 = Rs⁻¹ * Ps⁻¹ * Pt * Rt
 // Q1 = Rs⁻1 * Ps⁻1 * Pt * Rt
	//		Q1 = Rs⁻1 * Ps⁻1 * Pt * Rt
	//
	// Note that when remapping additive animations, we drop the rest pose terms, but we still need to convert between the
 // 请注意，在重新映射附加动画时，我们删除了其余的姿势项，但我们仍然需要在
	// source and target rotation frames. The terms drop because an additive rotation needs to be applied to a base one to
 // 源和目标旋转框架。这些术语下降是因为需要将附加旋转应用于基旋转
	// become a local space rotation. To that end, we can use any base as we are interested in the delta between source/target.
 // 成为局部空间旋转。为此，我们可以使用任何基数，因为我们对源/目标之间的增量感兴趣。
	// Using the bind pose as our base quickly cancels out the terms.
 // 使用绑定姿势作为我们的基础很快就可以消除这些项。
	// Dropping Rs and Rt from the equations above gives us:
 // 从上面的方程中去掉 Rs 和 Rt 可以得出：
	//
	//		Lt = Pt⁻¹ * Ps * Ls * Ps⁻¹ * Pt			(for additive animations)
 // Lt = Pt⁻1 * Ps * Ls * Ps⁻1 * Pt（对于附加动画）
	//
	// which is equivalent to the following in terms of our precomputed constants:
 // 就我们预先计算的常量而言，这相当于以下内容：
	//
	//		Lt = Q0 * Ls * Q0⁻¹						(for additive animations)
 // Lt = Q0 * Ls * Q0⁻¹（对于附加动画）
	//
	// For mesh space additive animations, the source rotations already represent a mesh space delta and as such no fix-up needs
 // 对于网格空间附加动画，源旋转已经代表网格空间增量，因此不需要修复
	// to applied for those.
 // 申请那些。

	RetargetingTable[0] = MakeTuple(FQuat::Identity, SourceLocalTransforms[0].GetRotation().Inverse() * TargetLocalTransforms[0].GetRotation());
	for (int32 TargetBoneIndex = 1; TargetBoneIndex < TargetNumBones; ++TargetBoneIndex)
	{
		const int32 SourceBoneIndex = TargetToSourceBoneIndexes[TargetBoneIndex];
		if (SourceBoneIndex != INDEX_NONE)
		{
			const int32 SourceParentIndex = SourceReferenceSkeleton.GetParentIndex(SourceBoneIndex);
			const int32 TargetParentIndex = TargetReferenceSkeleton.GetParentIndex(TargetBoneIndex);
			check(SourceParentIndex != INDEX_NONE);
			check(TargetParentIndex != INDEX_NONE);

			const FQuat PS = SourceComponentTransforms[SourceParentIndex].GetRotation();
			const FQuat PT = TargetComponentTransforms[TargetParentIndex].GetRotation();

			const FQuat RS = SourceLocalTransforms[SourceBoneIndex].GetRotation();
			const FQuat RT = TargetLocalTransforms[TargetBoneIndex].GetRotation();

			const FQuat Q0 = PT.Inverse() * PS;
			const FQuat Q1 = RS.Inverse() * PS.Inverse() * PT * RT;

			RetargetingTable[TargetBoneIndex] = MakeTuple(Q0, Q1);
		}
		else
		{
			RetargetingTable[TargetBoneIndex] = MakeTuple(FQuat::Identity, FQuat::Identity);
		}
	}
}

void FSkeletonRemapping::ComposeWith(const FSkeletonRemapping& OtherSkeletonRemapping)
{
	check(OtherSkeletonRemapping.SourceSkeleton == TargetSkeleton);

	TargetSkeleton = OtherSkeletonRemapping.TargetSkeleton;

	const int32 SourceNumBones = SourceToTargetBoneIndexes.Num();
	const int32 TargetNumBones = OtherSkeletonRemapping.TargetToSourceBoneIndexes.Num();

	TArray<int32> NewTargetToSourceBoneIndexes;
	TArray<TTuple<FQuat, FQuat>> NewRetargetingTable;

	NewTargetToSourceBoneIndexes.SetNumUninitialized(TargetNumBones);
	NewRetargetingTable.SetNumUninitialized(TargetNumBones);

	// Compose the retargeting constants
 // 编写重定向常量
	for (int32 NewTargetBoneIndex = 0; NewTargetBoneIndex < TargetNumBones; ++NewTargetBoneIndex)
	{
		const int32 OldTargetBoneIndex = OtherSkeletonRemapping.TargetToSourceBoneIndexes[NewTargetBoneIndex];
		const int32 OldSourceBoneIndex = (OldTargetBoneIndex != INDEX_NONE) ? TargetToSourceBoneIndexes[OldTargetBoneIndex] : INDEX_NONE;

		if (OldSourceBoneIndex != INDEX_NONE)
		{
			const TTuple<FQuat, FQuat>& OldQQ = RetargetingTable[OldTargetBoneIndex];
			const TTuple<FQuat, FQuat>& NewQQ = OtherSkeletonRemapping.RetargetingTable[NewTargetBoneIndex];
			const FQuat Q0 = NewQQ.Get<0>() * OldQQ.Get<0>();
			const FQuat Q1 = OldQQ.Get<1>() * NewQQ.Get<1>();

			NewTargetToSourceBoneIndexes[NewTargetBoneIndex] = OldSourceBoneIndex;
			NewRetargetingTable[NewTargetBoneIndex] = MakeTuple(Q0, Q1);
		}
		else
		{
			NewTargetToSourceBoneIndexes[NewTargetBoneIndex] = INDEX_NONE;
			NewRetargetingTable[NewTargetBoneIndex] = MakeTuple(FQuat::Identity, FQuat::Identity);
		}
	}

	TargetToSourceBoneIndexes = MoveTemp(NewTargetToSourceBoneIndexes);
	RetargetingTable = MoveTemp(NewRetargetingTable);

	// Rebuild the mapping from source bones to the target bones
 // 重建从源骨骼到目标骨骼的映射
	FMemory::Memset(SourceToTargetBoneIndexes.GetData(), static_cast<uint8>(INDEX_NONE), SourceNumBones * SourceToTargetBoneIndexes.GetTypeSize());
	for (int32 TargetBoneIndex = 0; TargetBoneIndex < TargetNumBones; ++TargetBoneIndex)
	{
		const int32 SourceBoneIndex = TargetToSourceBoneIndexes[TargetBoneIndex];
		if (SourceBoneIndex != INDEX_NONE)
		{
			SourceToTargetBoneIndexes[SourceBoneIndex] = TargetBoneIndex;
		}
	}
}

const TArray<SmartName::UID_Type>& FSkeletonRemapping::GetSourceToTargetCurveMapping() const
{
	static TArray<SmartName::UID_Type> Dummy;
	return Dummy;
}
