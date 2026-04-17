// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_Trail.h"
#include "Animation/AnimInstanceProxy.h"
#include "AngularLimit.h"
#include "Animation/AnimTrace.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_Trail)

/////////////////////////////////////////////////////
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail
// FAnimNode_Trail

DECLARE_CYCLE_STAT(TEXT("Trail Eval"), STAT_Trail_Eval, STATGROUP_Anim);

FAnimNode_Trail::FAnimNode_Trail()
	: ChainLength(2)
	, ChainBoneAxis(EAxis::X)
	, bInvertChainBoneAxis(false)
	, bLimitStretch(false)
	, bLimitRotation(false)
	, bUsePlanarLimit(false)
	, bActorSpaceFakeVel(false)
	, bReorientParentToChild(true)
	, bHadValidStrength(false)
#if WITH_EDITORONLY_DATA
	, bEnableDebug(false)
	, bShowBaseMotion(true)
	, bShowTrailLocation(false)
	, bShowLimit(true)
	, bEditorDebugEnabled(false)
	, DebugLifeTime(0.f)
	, TrailRelaxation_DEPRECATED(10.f)
#endif// #if WITH_EDITORONLY_DATA
	, MaxDeltaTime(0.f)
	, RelaxationSpeedScale(1.f)
	, StretchLimit(0)
	, FakeVelocity(FVector::ZeroVector)
#if WITH_EDITORONLY_DATA
	, TrailBoneRotationBlendAlpha_DEPRECATED(1.f)
#endif // WITH_EDITORONLY_DATA
	, LastBoneRotationAnimAlphaBlend(0.f)
{
	FRichCurve* TrailRelaxRichCurve = TrailRelaxationSpeed.GetRichCurve();
	TrailRelaxRichCurve->AddKey(0.f, 10.f);
	TrailRelaxRichCurve->AddKey(1.f, 5.f);
}

void FAnimNode_Trail::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(UpdateInternal)
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);

	ThisTimstep += Context.GetDeltaTime();

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Active Bone"), TrailBone.BoneName);
}

void FAnimNode_Trail::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" Active: %s)"), *TrailBone.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}


void FAnimNode_Trail::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
	SCOPE_CYCLE_COUNTER(STAT_Trail_Eval);

	check(OutBoneTransforms.Num() == 0);
	const float TimeStep = (MaxDeltaTime > 0.f)? FMath::Clamp(ThisTimstep, 0.f, MaxDeltaTime) : ThisTimstep;
	ThisTimstep = 0.f;

	if( ChainBoneIndices.Num() <= 0 )
	{
		return;
	}

	checkSlow (ChainBoneIndices.Num() == ChainLength);
	checkSlow (PerJointTrailData.Num() == ChainLength);
	// The incoming BoneIndex is the 'end' of the spline chain. We need to find the 'start' by walking SplineLength bones up hierarchy.
 // 传入的 BoneIndex 是样条链的“末端”。我们需要通过沿着 SplineLength 骨骼向上层级结构来找到“起点”。
	// Fail if we walk past the root bone.
 // 如果我们走过根骨就会失败。
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	FTransform BaseTransform;
 	if (BaseJoint.IsValidToEvaluate(BoneContainer))
 	{
		FCompactPoseBoneIndex BasePoseIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(BaseJoint.BoneIndex));
		FTransform BaseBoneTransform = Output.Pose.GetComponentSpaceTransform(BasePoseIndex);
 		BaseTransform = BaseBoneTransform * ComponentTransform;
 	}
	else
	{
		BaseTransform = ComponentTransform;
	}

	OutBoneTransforms.AddZeroed(ChainLength);

	// this should be checked outside
 // 这应该在外面检查
	checkSlow (TrailBone.IsValidToEvaluate(BoneContainer));

	// If we have >0 this frame, but didn't last time, record positions of all the bones.
 // 如果我们这一帧>0，但上次没有，则记录所有骨骼的位置。
	// Also do this if number has changed or array is zero.
 // 如果数字已更改或数组为零，也请执行此操作。
	//@todo I don't think this will work anymore. if Alpha is too small, it won't call evaluate anyway
 // @todo 我认为这不再有效。如果 Alpha 太小，它不会调用评估
	// so this has to change. AFAICT, this will get called only FIRST TIME
 // 所以这必须改变。 AFAICT，这只会在第一次被调用
	bool bHasValidStrength = (Alpha > 0.f);
	if(bHasValidStrength && !bHadValidStrength)
	{
		for(int32 i=0; i<ChainBoneIndices.Num(); i++)
		{
			if (BoneContainer.Contains(IntCastChecked<FBoneIndexType>(ChainBoneIndices[i])))
			{
				FCompactPoseBoneIndex ChildIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ChainBoneIndices[i]));
				const FTransform& ChainTransform = Output.Pose.GetComponentSpaceTransform(ChildIndex);
				TrailBoneLocations[i] = ChainTransform.GetTranslation();
			}
			else
			{
				TrailBoneLocations[i] = FVector::ZeroVector;
			}
		}
		OldBaseTransform = BaseTransform;
	}
	bHadValidStrength = bHasValidStrength;

	// transform between last frame and now.
 // 在上一帧和现在之间进行变换。
	FTransform OldToNewTM = OldBaseTransform.GetRelativeTransform(BaseTransform);

	// Add fake velocity if present to all but root bone
 // 如果除了根骨骼之外的所有骨骼都存在假速度，则添加假速度
	if(!FakeVelocity.IsZero())
	{
		FVector FakeMovement = -FakeVelocity * TimeStep;

		if (bActorSpaceFakeVel)
		{
			FTransform BoneToWorld(Output.AnimInstanceProxy->GetActorTransform());
			BoneToWorld.RemoveScaling();
			FakeMovement = BoneToWorld.TransformVector(FakeMovement);
		}

		FakeMovement = BaseTransform.InverseTransformVector(FakeMovement);
		// Then add to each bone
  // 然后添加到每个骨骼
		for(int32 i=1; i<TrailBoneLocations.Num(); i++)
		{
			TrailBoneLocations[i] += FakeMovement;
		}
	}

	// Root bone of trail is not modified.
 // 路径的根骨骼未修改。
	FCompactPoseBoneIndex RootIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ChainBoneIndices[0])); 
	const FTransform& ChainTransform = Output.Pose.GetComponentSpaceTransform(RootIndex);
	OutBoneTransforms[0] = FBoneTransform(RootIndex, ChainTransform);
	TrailBoneLocations[0] = ChainTransform.GetTranslation();

	TArray<FTransform> DebugPlaneTransforms;
#if WITH_EDITORONLY_DATA
	if (bUsePlanarLimit)
	{
		DebugPlaneTransforms.AddDefaulted(PlanarLimits.Num());
	}
#endif // WITH_EDITORONLY_DATA

	checkSlow(RotationLimits.Num() == ChainLength);
	checkSlow(RotationOffsets.Num() == ChainLength);

	// first solve trail locations
 // 首先解决踪迹位置
	for (int32 i = 1; i < ChainBoneIndices.Num(); i++)
	{
		// Parent bone position in component space.
  // 组件空间中的父骨骼位置。
		FCompactPoseBoneIndex ParentIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ChainBoneIndices[i - 1]));
		FVector ParentPos = TrailBoneLocations[i - 1];
		FVector ParentAnimPos = Output.Pose.GetComponentSpaceTransform(ParentIndex).GetTranslation();

		// Child bone position in component space.
  // 组件空间中的子骨骼位置。
		FCompactPoseBoneIndex ChildIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(ChainBoneIndices[i]));
		FVector ChildPos = OldToNewTM.TransformPosition(TrailBoneLocations[i]); // move from 'last frames component' frame to 'this frames component' frame
		FVector ChildAnimPos = Output.Pose.GetComponentSpaceTransform(ChildIndex).GetTranslation();

		// Desired parent->child offset.
  // 所需的父->子偏移量。
		FVector TargetDelta = (ChildAnimPos - ParentAnimPos);

		// Desired child position.
  // 所需的儿童位置。
		FVector ChildTarget = ParentPos + TargetDelta;

		// Find vector from child to target
  // 查找从孩子到目标的向量
		FVector Error = (ChildTarget - ChildPos);
		// Calculate how much to push the child towards its target
  // 计算将孩子推向目标的程度
		const float SpeedScale = RelaxationSpeedScaleInputProcessor.ApplyTo(RelaxationSpeedScale, TimeStep);
		const float Correction = FMath::Clamp<float>(TimeStep * SpeedScale * PerJointTrailData[i].TrailRelaxationSpeedPerSecond, 0.f, 1.f);

		// Scale correction vector and apply to get new world-space child position.
  // 缩放校正向量并应用以获得新的世界空间子位置。
		TrailBoneLocations[i] = ChildPos + (Error * Correction);

		// Limit stretch first
  // 首先限制拉伸
		// If desired, prevent bones stretching too far.
  // 如果需要，防止骨骼拉伸过度。
		if (bLimitStretch)
		{
			double RefPoseLength = TargetDelta.Size();
			FVector CurrentDelta = TrailBoneLocations[i] - TrailBoneLocations[i - 1];
			double CurrentLength = CurrentDelta.Size();

			// If we are too far - cut it back (just project towards parent particle).
   // 如果我们太远 - 将其缩小（仅投影到父粒子）。
			if ((CurrentLength - RefPoseLength > StretchLimit) && CurrentLength > UE_DOUBLE_SMALL_NUMBER)
			{
				FVector CurrentDir = CurrentDelta / CurrentLength;
				TrailBoneLocations[i] = TrailBoneLocations[i - 1] + (CurrentDir * (RefPoseLength + StretchLimit));
			}
		}
		
		// set planar limit if used
  // 设置平面限制（如果使用）
		if (bUsePlanarLimit)
		{
			for (int32 Index = 0; Index<PlanarLimits.Num(); ++Index)
			{
				const FAnimPhysPlanarLimit& PlanarLimit = PlanarLimits[Index];
				FTransform LimitPlaneTransform = PlanarLimit.PlaneTransform;

				if (PlanarLimit.DrivingBone.IsValidToEvaluate(BoneContainer))
				{
					FCompactPoseBoneIndex DrivingBoneIndex = PlanarLimit.DrivingBone.GetCompactPoseIndex(BoneContainer);

					FTransform DrivingBoneTransform = Output.Pose.GetComponentSpaceTransform(DrivingBoneIndex);
					LimitPlaneTransform *= DrivingBoneTransform;
				}

				FPlane LimitPlane(LimitPlaneTransform.GetLocation(), LimitPlaneTransform.GetUnitAxis(EAxis::Z));
#if WITH_EDITORONLY_DATA				
				DebugPlaneTransforms[Index] = LimitPlaneTransform;
#endif // #if WITH_EDITORONLY_DATA
				const double DistanceFromPlane = LimitPlane.PlaneDot(TrailBoneLocations[i]);
				if (DistanceFromPlane < 0.0)
				{
					TrailBoneLocations[i] -= DistanceFromPlane*FVector(LimitPlane.X, LimitPlane.Y, LimitPlane.Z);
				}
			}
		}

		// Modify child matrix
  // 修改子矩阵
		OutBoneTransforms[i] = FBoneTransform(ChildIndex, Output.Pose.GetComponentSpaceTransform(ChildIndex));
		OutBoneTransforms[i].Transform.SetTranslation(TrailBoneLocations[i]);

		// reorient parent to child 
  // 重新调整父母对孩子的方向
		if (bReorientParentToChild)
		{
			FVector CurrentBoneDir = OutBoneTransforms[i - 1].Transform.TransformVector(GetAlignVector(ChainBoneAxis, bInvertChainBoneAxis));
			CurrentBoneDir = CurrentBoneDir.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER);

			// Calculate vector from parent to child.
   // 计算从父级到子级的向量。
			FVector DeltaTranslation = OutBoneTransforms[i].Transform.GetTranslation() - OutBoneTransforms[i - 1].Transform.GetTranslation();
			FVector NewBoneDir = FVector(DeltaTranslation).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER);

			// Calculate a quaternion that gets us from our current rotation to the desired one.
   // 计算一个四元数，使我们从当前的旋转到所需的旋转。
			FQuat DeltaLookQuat = FQuat::FindBetweenNormals(CurrentBoneDir, NewBoneDir);
			FQuat ParentRotation = OutBoneTransforms[i - 1].Transform.GetRotation();
			FQuat NewRotation = DeltaLookQuat * ParentRotation;
			if (bLimitRotation)
			{
				// right now we're setting rotation of parent
    // 现在我们正在设置父级的轮换
				// if we want to limit rotation, try limit parent rotation
    // 如果我们想限制旋转，请尝试限制父旋转
				FQuat GrandParentRotation = FQuat::Identity;
				if (i == 1)
				{
					const FCompactPoseBoneIndex GrandParentIndex = BoneContainer.GetParentBoneIndex(ParentIndex);
					if (GrandParentIndex != INDEX_NONE)
					{
						GrandParentRotation = Output.Pose.GetComponentSpaceTransform(GrandParentIndex).GetRotation();
					}
				}
				else
				{
					// get local
     // 本地化
					GrandParentRotation = OutBoneTransforms[i - 2].Transform.GetRotation();
				}

				// we're fixing up parent local rotation here
    // 我们正在修复父本地轮换
				FQuat NewLocalRotation = GrandParentRotation.Inverse() * NewRotation;
				const FQuat& RefRotation = BoneContainer.GetRefPoseTransform(ParentIndex).GetRotation();
				const FRotationLimit& RotationLimit = RotationLimits[i - 1];
				// we limit to ref rotaiton
    // 我们限制参考旋转
				if (AnimationCore::ConstrainAngularRangeUsingEuler(NewLocalRotation, RefRotation, RotationLimit.LimitMin + RotationOffsets[i - 1], RotationLimit.LimitMax + RotationOffsets[i - 1]))
				{
					// if we changed rotaiton, let's find new tranlstion
     // 如果我们改变旋转，让我们找到新的转换
					NewRotation = GrandParentRotation * NewLocalRotation;
					FVector NewTransltion = NewRotation.Vector() * DeltaTranslation.Size();
					// we don't want to go to target, this creates very poppy motion. 
     // 我们不想达到目标，这会产生非常糟糕的动作。
					// @todo: to do this better, I feel we need alpha to blend into external limit and blend back to it
     // @todo：为了做得更好，我觉得我们需要阿尔法融入外部限制并混合回它
					FVector AdjustedLocation = FMath::Lerp(DeltaTranslation, NewTransltion, Correction) + OutBoneTransforms[i - 1].Transform.GetTranslation();
					OutBoneTransforms[i].Transform.SetTranslation(AdjustedLocation);
					// update new trail location, so that next chain will use this info
     // 更新新的踪迹位置，以便下一个链将使用此信息
					TrailBoneLocations[i] = AdjustedLocation;
				}
			}

			// clamp rotation, but translation is still there - should fix translation
   // 夹紧旋转，但平移仍然存在 - 应该修复平移
			OutBoneTransforms[i - 1].Transform.SetRotation(NewRotation);
		}
	}

	// For the last bone in the chain, use the rotation from the bone above it.
 // 对于链中的最后一个骨骼，使用其上方骨骼的旋转。
	FQuat LeafRotation = FQuat::FastLerp(OutBoneTransforms[ChainLength - 2].Transform.GetRotation(), OutBoneTransforms[ChainLength - 1].Transform.GetRotation(), LastBoneRotationAnimAlphaBlend);
	LeafRotation.Normalize();
	OutBoneTransforms[ChainLength - 1].Transform.SetRotation(LeafRotation);

#if WITH_EDITORONLY_DATA
	if (bEnableDebug || bEditorDebugEnabled)
	{
		if (bShowBaseMotion)
		{
			// draw new velocity on new base transform
   // 在新的基础变换上绘制新的速度
			FVector PreviousLoc = OldBaseTransform.GetLocation();
			FVector NewLoc = BaseTransform.GetLocation();
			Output.AnimInstanceProxy->AnimDrawDebugDirectionalArrow(PreviousLoc, NewLoc, 5.f, FColor::Red, false, DebugLifeTime);
		}

		if (bShowTrailLocation)
		{
			const int32 TrailNum = TrailBoneLocations.Num();
			if (TrailDebugColors.Num() != TrailNum)
			{
				TrailDebugColors.Reset();
				TrailDebugColors.AddUninitialized(TrailNum);

				for (int32 Index = 0; Index < TrailNum; ++Index)
				{
					TrailDebugColors[Index] = FColor::MakeRandomColor();
				}
			}
			// draw trail positions
   // 绘制轨迹位置
			for (int32 Index = 0; Index < TrailNum - 1; ++Index)
			{
				FVector PreviousLoc = ComponentTransform.TransformPosition(TrailBoneLocations[Index]);
				FVector NewLoc = ComponentTransform.TransformPosition(TrailBoneLocations[Index + 1]);
				Output.AnimInstanceProxy->AnimDrawDebugLine(PreviousLoc, NewLoc, TrailDebugColors[Index], false, DebugLifeTime);
			}
		}

		// draw limits
  // 划定界限
		if (bShowLimit)
		{
			if (bUsePlanarLimit)
			{
				const int32 PlaneLimitNum = DebugPlaneTransforms.Num();
				if (PlaneDebugColors.Num() != PlaneLimitNum)
				{
					PlaneDebugColors.Reset();
					PlaneDebugColors.AddUninitialized(PlaneLimitNum);

					for (int32 Index = 0; Index < PlaneLimitNum; ++Index)
					{
						PlaneDebugColors[Index] = FColor::MakeRandomColor();
					}
				}

				// draw plane info
    // 绘制平面信息
				for (int32 Index = 0; Index < PlaneLimitNum; ++Index)
				{
					const FTransform& PlaneTransform =  DebugPlaneTransforms[Index];
					FTransform WorldPlaneTransform = PlaneTransform * ComponentTransform;
					Output.AnimInstanceProxy->AnimDrawDebugPlane(WorldPlaneTransform, 40.f, PlaneDebugColors[Index], false, DebugLifeTime, 0.5);
					Output.AnimInstanceProxy->AnimDrawDebugDirectionalArrow(WorldPlaneTransform.GetLocation(),
						WorldPlaneTransform.GetLocation() + WorldPlaneTransform.GetRotation().RotateVector(FVector(0, 0, 40)), 10.f, PlaneDebugColors[Index], false, DebugLifeTime, 0.5f);
				}
				
			}
		}
	}
#endif //#if WITH_EDITORONLY_DATA
	// Update OldBaseTransform
 // 更新 OldBaseTransform
	OldBaseTransform = BaseTransform;
}

bool FAnimNode_Trail::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if bones are valid
 // 如果骨骼有效
	if (TrailBone.IsValidToEvaluate(RequiredBones))
	{
		for (auto& ChainIndex : ChainBoneIndices)
		{
			if (ChainIndex == INDEX_NONE)
			{
				// unfortunately there is no easy way to communicate this back to user other than spamming here because this gets called every frame
    // 不幸的是，除了这里的垃圾邮件之外，没有简单的方法可以将其传达给用户，因为每帧都会调用它
				// originally tried in AnimGraphNode, but that doesn't know hierarchy so I can't verify it there. Maybe should try with USkeleton asset there. @todo
    // 最初在 AnimGraphNode 中尝试过，但它不知道层次结构，所以我无法在那里验证它。也许应该尝试使用 USkeleton 资产。 @todo
				return false;
			}
			else if (RequiredBones.Contains(IntCastChecked<FBoneIndexType>(ChainIndex)) == false)
			{
				return false;
			}
		}
	}

	return (ChainBoneIndices.Num() > 0);
}

void FAnimNode_Trail::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(InitializeBoneReferences)
	TrailBone.Initialize(RequiredBones);
	BaseJoint.Initialize(RequiredBones);

	// initialize chain bone indices
 // 初始化链骨索引
	ChainBoneIndices.Reset();
	if (ChainLength > 1 && TrailBone.IsValidToEvaluate(RequiredBones))
	{
		ChainBoneIndices.AddZeroed(ChainLength);

		int32 WalkBoneIndex = TrailBone.BoneIndex;
		ChainBoneIndices[ChainLength - 1] = WalkBoneIndex;

		for(int32 i = 1; i < ChainLength; i++)
		{
			//Insert indices at the start of array, so that parents are before children in the array.
   // 在数组的开头插入索引，以便父项位于数组中的子项之前。
			int32 TransformIndex = ChainLength - (i + 1);

			// if reached to root or invalid, invalidate the data
   // 如果到达根或无效，则使数据无效
			if(WalkBoneIndex == INDEX_NONE || WalkBoneIndex == 0)
			{
				ChainBoneIndices[TransformIndex] = INDEX_NONE;
			}
			else
			{
				// Get parent bone.
    // 获取父骨骼。
				WalkBoneIndex = RequiredBones.GetParentBoneIndex(WalkBoneIndex);
				ChainBoneIndices[TransformIndex] = WalkBoneIndex;
			}
		}
	}

	for (FAnimPhysPlanarLimit& PlanarLimit : PlanarLimits)
	{
		PlanarLimit.DrivingBone.Initialize(RequiredBones);
	}
}

FVector FAnimNode_Trail::GetAlignVector(EAxis::Type AxisOption, bool bInvert)
{
	FVector AxisDir;

	if (AxisOption == EAxis::X)
	{
		AxisDir = FVector(1, 0, 0);
	}
	else if (AxisOption == EAxis::Y)
	{
		AxisDir = FVector(0, 1, 0);
	}
	else
	{
		AxisDir = FVector(0, 0, 1);
	}

	if (bInvert)
	{
		AxisDir *= -1.f;
	}

	return AxisDir;
}

void FAnimNode_Trail::PostLoad()
{
#if WITH_EDITORONLY_DATA
	if (TrailRelaxation_DEPRECATED != 10.f)
	{
		FRichCurve* TrailRelaxRichCurve = TrailRelaxationSpeed.GetRichCurve();
		TrailRelaxRichCurve->Reset();
		TrailRelaxRichCurve->AddKey(0.f, TrailRelaxation_DEPRECATED);
		TrailRelaxRichCurve->AddKey(1.f, TrailRelaxation_DEPRECATED);
		// since we don't know if it's same as default or not, we have to keep default
  // 因为我们不知道它是否与默认值相同，所以我们必须保留默认值
		// if default, the default constructor will take care of it. If not, we'll reset
  // 如果默认，默认构造函数将处理它。如果没有，我们将重置
		TrailRelaxation_DEPRECATED = 10.f;
	}
	EnsureChainSize();
#endif
}

void FAnimNode_Trail::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);

	// allocated all memory here in initialize
 // 在初始化时在这里分配所有内存
	PerJointTrailData.Reset();
	TrailBoneLocations.Reset();
	if(ChainLength > 1)
	{
		PerJointTrailData.AddZeroed(ChainLength);
		TrailBoneLocations.AddZeroed(ChainLength);

		float Interval = (ChainLength > 1)? (1.f/(ChainLength-1)) : 0.f;
		const FRichCurve* TrailRelaxRichCurve = TrailRelaxationSpeed.GetRichCurveConst();
		check(TrailRelaxRichCurve);
		for(int32 Idx=0; Idx<ChainLength; ++Idx)
		{
			PerJointTrailData[Idx].TrailRelaxationSpeedPerSecond = TrailRelaxRichCurve->Eval(Interval * Idx);
		}
	}

	RelaxationSpeedScaleInputProcessor.Reinitialize();
}

#if WITH_EDITOR
void FAnimNode_Trail::EnsureChainSize()
{
	if (RotationLimits.Num() != ChainLength)
	{
		const int32 CurNum = RotationLimits.Num();
		if (CurNum >= ChainLength)
		{
			RotationLimits.SetNum(ChainLength);
			RotationOffsets.SetNum(ChainLength);
		}
		else
		{
			RotationLimits.AddDefaulted(ChainLength - CurNum);
			RotationOffsets.AddZeroed(ChainLength - CurNum);
		}
	}
}

#endif // WITH_EDITOR

