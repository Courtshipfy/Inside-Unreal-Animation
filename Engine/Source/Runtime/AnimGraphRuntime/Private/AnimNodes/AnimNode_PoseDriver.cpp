// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/AnimNode_PoseDriver.h"
#include "AnimationRuntime.h"
#include "Animation/AnimCurveUtils.h"
#include "Serialization/CustomVersion.h"
#include "Animation/AnimInstanceProxy.h"
#include "RBF/RBFSolver.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_PoseDriver)


FAnimNode_PoseDriver::FAnimNode_PoseDriver()
	: DriveSource(EPoseDriverSource::Rotation)
	, DriveOutput(EPoseDriverOutput::DrivePoses)
	, LODThreshold(INDEX_NONE)
{
	RBFParams.DistanceMethod = ERBFDistanceMethod::SwingAngle;

#if WITH_EDITORONLY_DATA
	SoloTargetIndex = INDEX_NONE;
	bSoloDrivenOnly = false;

	RadialScaling_DEPRECATED = 0.25f;
	Type_DEPRECATED = EPoseDriverType::SwingOnly;
	TwistAxis_DEPRECATED = BA_X;
#endif
}

void FAnimNode_PoseDriver::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_PoseHandler::Initialize_AnyThread(Context);

	SourcePose.Initialize(Context);
}

void FAnimNode_PoseDriver::RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset)
{
	// Cache UIDs for driving curves
	// 用于驾驶曲线的缓存 UID
	PoseExtractContext.PoseCurves.Reset();
	const USkeleton* Skeleton = InPoseAsset->GetSkeleton();
	if (Skeleton)
	{
		const TArray<FName>& PoseNames = InPoseAsset->GetPoseFNames();
		for (FPoseDriverTarget& PoseTarget : PoseTargets)
		{
			const int32 PoseIndex = InPoseAsset->GetPoseIndexByName(PoseTarget.DrivenName);
			if (PoseIndex != INDEX_NONE)
			{
				// we keep pose index as that is the fastest way to search when extracting pose asset
				// 我们保留姿势索引，因为这是提取姿势资产时最快的搜索方式
				PoseTarget.PoseCurveIndex = PoseExtractContext.PoseCurves.Add(FPoseCurve(PoseIndex, PoseNames[PoseIndex], 0.f));
			}
			else
			{
				PoseTarget.PoseCurveIndex = INDEX_NONE;
			}
		}
	}
}

void FAnimNode_PoseDriver::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	FAnimNode_PoseHandler::CacheBones_AnyThread(Context);

	// Init pose input
	// 初始化姿势输入
	SourcePose.CacheBones(Context);

	const FBoneContainer& BoneContainer = Context.AnimInstanceProxy->GetRequiredBones();

	// Init bone refs
	// 初始化骨骼参考
	for (FBoneReference& SourceBoneRef : SourceBones)
	{
		SourceBoneRef.Initialize(BoneContainer);
	}

	for (FBoneReference& OnlyDriveBoneRef : OnlyDriveBones)
	{
		OnlyDriveBoneRef.Initialize(BoneContainer);
	}

	EvalSpaceBone.Initialize(BoneContainer);

	// Don't want to modify SourceBones, set weight to zero (if weight array is allocated)
	// 不想修改 SourceBones，将权重设置为零（如果分配了权重数组）
	for (FBoneReference& SourceBoneRef : SourceBones)
	{
		const FCompactPoseBoneIndex SourceCompactIndex = SourceBoneRef.GetCompactPoseIndex(BoneContainer);
		if (BoneBlendWeights.IsValidIndex(SourceCompactIndex.GetInt()))
		{
			BoneBlendWeights[SourceCompactIndex.GetInt()] = 0.f;
		}
	}

	// Check if there are valid OnlyDriveBones, since there might be a None entry.
	// 检查是否有有效的 OnlyDriveBones，因为可能有 None 条目。
	bHasOnlyDriveBones = false;
	for (const FBoneReference& BoneRef : OnlyDriveBones)
	{
		if (!BoneRef.BoneName.IsNone())
		{
			bHasOnlyDriveBones = true;
			break;
		}
	}

	// If we are filtering for only specific bones, set blend weight to zero for unwanted bones, and remember which bones to filter
	// 如果我们仅过滤特定骨骼，请将不需要的骨骼的混合权重设置为零，并记住要过滤的骨骼
	BonesToFilter.Reset();
	if (bHasOnlyDriveBones && CurrentPoseAsset.IsValid())
	{
		// Super call above should init BoneBlendWeights to compact pose size if CurrentPoseAsset is valid
		// 如果 CurrentPoseAsset 有效，上面的超级调用应该初始化 BoneBlendWeights 以压缩姿势大小
		check(BoneBlendWeights.Num() == BoneContainer.GetBoneIndicesArray().Num());

		const TArray<FName> TrackNames = CurrentPoseAsset.Get()->GetTrackNames();
		for (const auto& TrackName : TrackNames)
		{
			// Check if bone in OnlyDriveBones
			// 检查OnlyDriveBones中是否有骨骼
			if (!IsBoneDriven(TrackName))
			{
				int32 MeshBoneIndex = BoneContainer.GetPoseBoneIndexForBoneName(TrackName);
				FCompactPoseBoneIndex CompactBoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshBoneIndex));
				if (CompactBoneIndex != INDEX_NONE)
				{
					BoneBlendWeights[CompactBoneIndex.GetInt()] = 0.f; // Set blend weight for non-additive 
					BonesToFilter.Add(CompactBoneIndex); // Remember bones to filter out for additive
				}
			}
		}
	}

	PoseExtractContext.BonesRequired.SetNumZeroed(BoneBlendWeights.Num());
	for (int32 BoneIndex = 0; BoneIndex < BoneBlendWeights.Num(); BoneIndex++)
	{
		PoseExtractContext.BonesRequired[BoneIndex] = BoneBlendWeights[BoneIndex] > SMALL_NUMBER;
	}
}

void FAnimNode_PoseDriver::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	FAnimNode_PoseHandler::UpdateAssetPlayer(Context);
	SourcePose.Update(Context);
}

void FAnimNode_PoseDriver::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FAnimNode_PoseHandler::GatherDebugData(DebugData);
	SourcePose.GatherDebugData(DebugData.BranchFlow(1.f));
}

float FAnimNode_PoseDriver::GetRadiusForTarget(const FRBFTarget& Target) const
{
	return FRBFSolver::GetRadiusForTarget(Target, RBFParams);
}

bool FAnimNode_PoseDriver::IsBoneDriven(FName BoneName) const
{
	for (const FBoneReference& BoneRef : OnlyDriveBones)
	{
		if (BoneRef.BoneName == BoneName)
		{
			return true;
		}
	}

	return false;
}


void FAnimNode_PoseDriver::GetRBFTargets(TArray<FRBFTarget>& OutTargets, const FBoneContainer* BoneContainer) const
{
	OutTargets.Reset();
	OutTargets.AddZeroed(PoseTargets.Num());

	// Create entry for each target
	// 为每个目标创建条目
	for (int32 TargetIdx = 0; TargetIdx < PoseTargets.Num(); TargetIdx++)
	{
		FRBFTarget& RBFTarget = OutTargets[TargetIdx];
		const FPoseDriverTarget& PoseTarget = PoseTargets[TargetIdx];

		// We want to make sure we always have the right number of Values in our RBFTarget. 
		// 我们希望确保 RBFTarget 中始终具有正确数量的值。
		// If bone entries are missing, we fill with zeroes
		// 如果骨骼条目丢失，我们用零填充
		for (int32 SourceIdx = 0; SourceIdx < SourceBones.Num(); SourceIdx++)
		{
			if (PoseTarget.BoneTransforms.IsValidIndex(SourceIdx))
			{
				const FPoseDriverTransform& BoneTransform = PoseTarget.BoneTransforms[SourceIdx];

				// Get Ref Transform
				// 获取参考变换
				FTransform RefBoneTransform = FTransform::Identity;
				if (bEvalFromRefPose && BoneContainer)
				{
					const FBoneReference& BoneReference = SourceBones[SourceIdx];
					if (BoneReference.HasValidSetup())
					{
						const FCompactPoseBoneIndex CompactPoseIndex = BoneReference.CachedCompactPoseIndex;
						if (CompactPoseIndex < BoneContainer->GetCompactPoseNumBones())
						{
							RefBoneTransform = BoneContainer->GetRefPoseTransform(CompactPoseIndex);
						}
					}
				}

				// Target Translation
				// 目标翻译
				if (DriveSource == EPoseDriverSource::Translation)
				{
					// Make translation relative to its Ref
					// 相对于其 Ref 进行翻译
					if (bEvalFromRefPose)
					{
						RBFTarget.AddFromVector(RefBoneTransform.Inverse().TransformPosition(BoneTransform.TargetTranslation));
					}
					else
					{
						RBFTarget.AddFromVector(BoneTransform.TargetTranslation);
					}
				}

				// Target Rotation
				// 目标旋转
				else
				{
					// Make rotation relative to its Ref
					// 相对于其 Ref 进行旋转
					if (bEvalFromRefPose)
					{	
						const FQuat TargetRotation = BoneTransform.TargetRotation.Quaternion();
						RBFTarget.AddFromRotator(RefBoneTransform.Inverse().TransformRotation(TargetRotation).Rotator());
					}
					else
					{
						RBFTarget.AddFromRotator(BoneTransform.TargetRotation);
					}
				}
			}
			else
			{
				RBFTarget.AddFromVector(FVector::ZeroVector);
			}
		}

		RBFTarget.ScaleFactor = PoseTarget.TargetScale;
		RBFTarget.bApplyCustomCurve = PoseTarget.bApplyCustomCurve;
		RBFTarget.CustomCurve = PoseTarget.CustomCurve;
		RBFTarget.DistanceMethod = PoseTarget.DistanceMethod;
		RBFTarget.FunctionType = PoseTarget.FunctionType;
	}
}


void FAnimNode_PoseDriver::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	QUICK_SCOPE_CYCLE_COUNTER(STAT_PoseDriver_Eval);

	if (!IsLODEnabled(Output.AnimInstanceProxy))
	{
		SourcePose.Evaluate(Output);
		return;
	}

	FPoseContext SourceData(Output);
	SourcePose.Evaluate(SourceData);

	// Udpate DrivenIDs if needed
	// 如果需要，更新 DrivenID
	if (bCachedDrivenIDsAreDirty)
	{
		if (CurrentPoseAsset.IsValid())
		{
			RebuildPoseList(Output.AnimInstanceProxy->GetRequiredBones(), CurrentPoseAsset.Get());
		}
	}

	// Get the index of the source bone
	// 获取源骨骼的索引
	const FBoneContainer& BoneContainer = SourceData.Pose.GetBoneContainer();

	RBFInput.Values.Reset();

	SourceBoneTMs.Reset();
	bool bFoundAnyBone = false;
	for (const FBoneReference& SourceBoneRef : SourceBones)
	{
		FTransform SourceBoneTM = FTransform::Identity;

		const FCompactPoseBoneIndex SourceCompactIndex = SourceBoneRef.GetCompactPoseIndex(BoneContainer);
		if (SourceCompactIndex.GetInt() != INDEX_NONE)
		{
			// If evaluating in alternative bone space, have to build component space pose
			// 如果在替代骨骼空间中进行评估，则必须构建组件空间姿势
			if (EvalSpaceBone.IsValidToEvaluate(BoneContainer))
			{
				FCSPose<FCompactPose> CSPose;
				CSPose.InitPose(SourceData.Pose);

				const FCompactPoseBoneIndex EvalSpaceCompactIndex = EvalSpaceBone.GetCompactPoseIndex(BoneContainer);
				FTransform EvalSpaceCompSpace = CSPose.GetComponentSpaceTransform(EvalSpaceCompactIndex);
				FTransform SourceBoneCompSpace = CSPose.GetComponentSpaceTransform(SourceCompactIndex);

				SourceBoneTM = SourceBoneCompSpace.GetRelativeTransform(EvalSpaceCompSpace);
			}
			// If just evaluating in local space, just grab from local space pose
			// 如果只是在局部空间中评估，只需从局部空间姿势中抓取
			else
			{
				// Relative to Ref Pose
				// 相对于参考位姿
				if (bEvalFromRefPose && SourceCompactIndex.GetInt() < BoneContainer.GetCompactPoseNumBones())
				{
					SourceBoneTM = SourceData.Pose[SourceCompactIndex].GetRelativeTransform(BoneContainer.GetRefPoseTransform(SourceCompactIndex));
				}
				else
				{
					SourceBoneTM = SourceData.Pose[SourceCompactIndex];
				}
			}

			bFoundAnyBone = true;
		}


		// Build RBFInput entry
		// 构建 RBFInput 条目
		if (DriveSource == EPoseDriverSource::Translation)
		{
			RBFInput.AddFromVector(SourceBoneTM.GetTranslation());
		}
		else
		{
			RBFInput.AddFromRotator(SourceBoneTM.Rotator());
		}

		// Record this so we can use it for drawing in edit mode
		// 记录下来，以便我们可以在编辑模式下使用它进行绘图
		SourceBoneTMs.Add(SourceBoneTM);
	}

	// Do nothing if bone is no bones are found/all LOD-ed out
	// 如果骨头不存在，则不执行任何操作 未找到骨头/所有 LOD 均已删除
	if (!bFoundAnyBone)
	{
		Output = SourceData;
		return;
	}

	RBFParams.TargetDimensions = SourceBones.Num() * 3;

	OutputWeights.Reset();
	
	// Use SoloTarget, no need to Solve
	// 使用SoloTarget，无需Solve
#if WITH_EDITORONLY_DATA
	if (PoseTargets.IsValidIndex(SoloTargetIndex))
	{
		OutputWeights.Add(FRBFOutputWeight(SoloTargetIndex, 1.0f));
	}
	else
#endif
	// Solve Weights
	// 求解权重
	{
		// Get target array as RBF types
		// 获取 RBF 类型的目标数组
		GetRBFTargets(RBFTargets, &BoneContainer);

		if (!SolverData.IsValid() || !FRBFSolver::IsSolverDataValid(*SolverData, RBFParams, RBFTargets))
		{
			SolverData = FRBFSolver::InitSolver(RBFParams, RBFTargets);
		}

		// Run RBF solver
		// 运行 RBF 求解器
		FRBFSolver::Solve(*SolverData, RBFParams, RBFTargets, RBFInput, OutputWeights);
	}

	// Track if we have filled Output with valid pose
	// 跟踪我们是否已用有效姿势填充输出
	bool bHaveValidPose = false;

	// Process active targets (if any)
	// 处理活动目标（如果有）
	if (OutputWeights.Num() > 0)
	{
		// Drive Poses from PoseAsset
		// 从 PoseAsset 驱动姿势
		if (DriveOutput == EPoseDriverOutput::DrivePoses)
		{
			const UPoseAsset* CachedPoseAsset = CurrentPoseAsset.Get();
				
			// Check if PoseAsset is assignedand and compatible
			// 检查 PoseAsset 是否已分配且兼容
			if (CachedPoseAsset && CachedPoseAsset->GetSkeleton() != nullptr)
			{
				// clear the value before setting it. 
				// 设置前先清除该值。
				for (int32 PoseIndex = 0; PoseIndex < PoseExtractContext.PoseCurves.Num(); ++PoseIndex)
				{
					PoseExtractContext.PoseCurves[PoseIndex].Value = 0.f;
				}

				// Then fill in weight for any driven poses
				// 然后填写任何驱动姿势的重量
				for (const FRBFOutputWeight& Weight : OutputWeights)
				{
					const FPoseDriverTarget& PoseTarget = PoseTargets[Weight.TargetIndex];
					const int32 PoseIndex = PoseTarget.PoseCurveIndex;
					if (PoseIndex != INDEX_NONE)
					{
						PoseExtractContext.PoseCurves[PoseIndex].Value = Weight.TargetWeight;
					}
				}

				FPoseContext CurrentPose(Output);
				FAnimationPoseData CurrentAnimationPoseData(CurrentPose);

				// Evaluate PoseAsset
				// 评估 PoseAsset
				if (CachedPoseAsset->GetAnimationPose(CurrentAnimationPoseData, PoseExtractContext))
				{
					// If Additive, Set Source and OnlyDrive Bones to Zero and Accumulate Pose
					// 如果是累加性的，则将源骨骼和仅驱动骨骼设置为零并累积姿势
					if (CurrentPoseAsset->IsValidAdditive())
					{
						const FTransform AdditiveIdentity(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);

						// Don't want to modify SourceBones, set additive offset to zero (not identity transform, as need zero scale)
						// 不想修改 SourceBones，将附加偏移设置为零（不是恒等变换，因为需要零比例）
						for (const FBoneReference& SourceBoneRef : SourceBones)
						{
							const FCompactPoseBoneIndex SourceCompactIndex = SourceBoneRef.GetCompactPoseIndex(BoneContainer);
							CurrentPose.Pose[SourceCompactIndex] = AdditiveIdentity;
						}

						// If filtering for specific bones, filter out bones using BonesToFilter array
						// 如果过滤特定骨骼，请使用 BonesToFilter 数组过滤掉骨骼
						if (bHasOnlyDriveBones)
						{
							for (FCompactPoseBoneIndex BoneIndex : BonesToFilter)
							{
								CurrentPose.Pose[BoneIndex] = AdditiveIdentity;
							}
						}

						// Start by copying input
						// 从复制输入开始
						Output = SourceData;

						FAnimationPoseData AccumulatedAnimationPoseData(Output); // out
						FAnimationRuntime::AccumulateAdditivePose(AccumulatedAnimationPoseData, CurrentAnimationPoseData, 1.f, EAdditiveAnimationType::AAT_LocalSpaceBase);
					}

					// If Non-Additive, Blend between Source and Current filtering with OnlyDrive Bones
					// 如果非相加，则使用 OnlyDrive Bones 在源过滤和当前过滤之间进行混合
					else
					{
						const FAnimationPoseData SourceAnimationPoseData(SourceData);
						FAnimationPoseData BlendedAnimationPoseData(Output); // out
						FAnimationRuntime::BlendTwoPosesTogetherPerBone(SourceAnimationPoseData, CurrentAnimationPoseData, BoneBlendWeights, BlendedAnimationPoseData);
					}

					bHaveValidPose = true;
				}
			}
		}

		// Drive curves (morphs, materials etc)
		// 驱动曲线（变形、材质等）
		else if (DriveOutput == EPoseDriverOutput::DriveCurves)
		{
			// Start by copying input
			// 从复制输入开始
			Output = SourceData;

			// Then set curves based on target weights
			// 然后根据目标权重设置曲线
			FBlendedCurve DrivenCurves;
			for (const FRBFOutputWeight& Weight : OutputWeights)
			{
				FPoseDriverTarget& PoseTarget = PoseTargets[Weight.TargetIndex];
				if (PoseTarget.DrivenName != NAME_None)
				{
					DrivenCurves.Add(PoseTarget.DrivenName, Weight.TargetWeight);
				}
			}

			// Merge driven curves into output
			// 将驱动曲线合并到输出中
			UE::Anim::FNamedValueArrayUtils::Union(Output.Curve, DrivenCurves, [](UE::Anim::FCurveElement& InOutElement, const UE::Anim::FCurveElement& InElement0, UE::Anim::ENamedValueUnionFlags InFlags)
			{
				if(EnumHasAnyFlags(InFlags, UE::Anim::ENamedValueUnionFlags::ValidArg1))
				{
					InOutElement.Value = InElement0.Value;
					InOutElement.Flags |= InElement0.Flags;
				}
			});

			bHaveValidPose = true;
		}
	}

	// No valid pose, just pass through
	// 没有有效的姿势，只是通过
	if (!bHaveValidPose)
	{
		Output = SourceData;
	}

#if WITH_EDITORONLY_DATA
	else if (!bSoloDrivenOnly && PoseTargets.IsValidIndex(SoloTargetIndex))
	{
		SourceBoneTMs.Reset();
		const FPoseDriverTarget& PoseTarget = PoseTargets[SoloTargetIndex];

		for (int32 SourceIdx = 0; SourceIdx < SourceBones.Num(); SourceIdx++)
		{
			const FBoneReference& SourceBoneRef = SourceBones[SourceIdx];
			const FCompactPoseBoneIndex SourceCompactIndex = SourceBoneRef.GetCompactPoseIndex(BoneContainer);

			if (PoseTarget.BoneTransforms.IsValidIndex(SourceIdx) && SourceCompactIndex.GetInt() != INDEX_NONE)
			{
				FTransform& TargetTransform = Output.Pose[SourceCompactIndex];
				const FPoseDriverTransform& SourceTransform = PoseTarget.BoneTransforms[SourceIdx];

				if (DriveSource == EPoseDriverSource::Translation)
				{
					TargetTransform.SetTranslation(SourceTransform.TargetTranslation);
				}
				else
				{
					TargetTransform.SetRotation(SourceTransform.TargetRotation.Quaternion());
				}
				SourceBoneTMs.Add(TargetTransform);
			}
		}
	}
#endif
}

