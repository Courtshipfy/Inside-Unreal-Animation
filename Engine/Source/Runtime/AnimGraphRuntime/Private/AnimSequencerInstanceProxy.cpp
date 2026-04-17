// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimSequencerInstanceProxy.h"
#include "AnimSequencerInstance.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimSequencerInstanceProxy)

void FAnimSequencerInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);
	ConstructNodes();
	FullBodyBlendNode.bAdditiveNode = false;
	FullBodyBlendNode.bNormalizeAlpha = true;

	AdditiveBlendNode.bAdditiveNode = true;
	AdditiveBlendNode.bNormalizeAlpha = false;

	FullBodyBlendNode.ResetPoses();
	AdditiveBlendNode.ResetPoses();


	SnapshotNode.SnapshotName = UAnimSequencerInstance::SequencerPoseName;
	ClearSequencePlayerAndMirrorMaps();
	UpdateCounter.Reset();
	RootMotionOverride.Reset();
}

bool FAnimSequencerInstanceProxy::Evaluate(FPoseContext& Output)
{
	SequencerRootNode.Evaluate_AnyThread(Output);
	if (RootMotionOverride.IsSet())
	{
		if (!RootMotionOverride.GetValue().bBlendFirstChildOfRoot)
		{
			for (const FCompactPoseBoneIndex BoneIndex : Output.Pose.ForEachBoneIndex())
			{
				if (BoneIndex.IsRootBone())
				{
					Output.Pose[BoneIndex] = RootMotionOverride.GetValue().RootMotion;
					break;
				}
			}
		}
		else if (RootMotionOverride.GetValue().ChildBoneIndex != INDEX_NONE)
		{
			FCompactPoseBoneIndex PoseIndex = Output.Pose.GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(RootMotionOverride.GetValue().ChildBoneIndex);
			if (PoseIndex.IsValid())
			{
				Output.Pose[PoseIndex] = RootMotionOverride.GetValue().RootMotion;
			}
		}
	}
	
	RootBoneTransform.Reset();

	if (SwapRootBone != ESwapRootBone::SwapRootBone_None)
	{
		for (const FCompactPoseBoneIndex BoneIndex : Output.Pose.ForEachBoneIndex())
		{
			if (BoneIndex.IsRootBone())
			{
				RootBoneTransform = Output.Pose[BoneIndex];
				Output.Pose[BoneIndex] = FTransform::Identity;
				break;
			}
		}
	}

	return true;
}

void FAnimSequencerInstanceProxy::PostEvaluate(UAnimInstance* InAnimInstance)
{
	if (GetSkelMeshComponent() && SwapRootBone != ESwapRootBone::SwapRootBone_None)
	{
		if (RootBoneTransform.IsSet())
		{
			FTransform RelativeTransform = RootBoneTransform.GetValue();

			if (InitialTransform.IsSet())
			{
				RelativeTransform = RootBoneTransform.GetValue() * InitialTransform.GetValue();
			}

			if (SwapRootBone == ESwapRootBone::SwapRootBone_Component)
			{
				GetSkelMeshComponent()->SetRelativeLocationAndRotation(RelativeTransform.GetLocation(), RelativeTransform.GetRotation().Rotator());
			}
			else if (SwapRootBone == ESwapRootBone::SwapRootBone_Actor)
			{
				AActor* Actor = GetSkelMeshComponent()->GetOwner();
				if (Actor && Actor->GetRootComponent())
				{
					Actor->GetRootComponent()->SetRelativeLocationAndRotation(RelativeTransform.GetLocation(), RelativeTransform.GetRotation().Rotator());
				}
			}
		}
	}
}

void FAnimSequencerInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	UpdateCounter.Increment();

	SequencerRootNode.Update_AnyThread(InContext);
}

void FAnimSequencerInstanceProxy::ConstructNodes()
{
	// construct node link node for full body and additive to apply additive node
	// 为全身和附加构造节点链接节点以应用附加节点
	SequencerRootNode.Base.SetLinkNode(&FullBodyBlendNode);
	SequencerRootNode.Additive.SetLinkNode(&AdditiveBlendNode);

}

void FAnimSequencerInstanceProxy::AddReferencedObjects(UAnimInstance* InAnimInstance, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InAnimInstance, Collector);

	for (const TPair<uint32, FSequencerPlayerBase*>& IndexPlayerPair : SequencerToPlayerMap)
	{
		if(IndexPlayerPair.Value->IsOfType<FSequencerPlayerAnimSequence>())
		{
			FSequencerPlayerAnimSequence* SequencerPlayerAnimSequence = static_cast<FSequencerPlayerAnimSequence*>(IndexPlayerPair.Value);
			Collector.AddPropertyReferencesWithStructARO(FAnimNode_SequenceEvaluator_Standalone::StaticStruct(), &SequencerPlayerAnimSequence->PlayerNode);
		}
	}

	for (const TPair<uint32, FAnimNode_Mirror_Standalone*>& IndexMirrorPair : SequencerToMirrorMap)
	{
		Collector.AddPropertyReferencesWithStructARO(FAnimNode_Mirror_Standalone::StaticStruct(), IndexMirrorPair.Value);
	}
}

void FAnimSequencerInstanceProxy::ClearSequencePlayerAndMirrorMaps()
{
	for (TPair<uint32, FSequencerPlayerBase*>& Iter : SequencerToPlayerMap)
	{
		delete Iter.Value;
	}

	SequencerToPlayerMap.Empty();

	for (TPair<uint32, FAnimNode_Mirror_Standalone*>& Iter : SequencerToMirrorMap)
	{
		delete Iter.Value;
	}

	SequencerToMirrorMap.Empty();
}

void FAnimSequencerInstanceProxy::ResetPose()
{
	SequencerRootNode.Base.SetLinkNode(&SnapshotNode);
	//force evaluation?
	//力评价？
}	
void FAnimSequencerInstanceProxy::ResetNodes()
{
	FMemory::Memzero(FullBodyBlendNode.DesiredAlphas.GetData(), FullBodyBlendNode.DesiredAlphas.GetAllocatedSize());
	FMemory::Memzero(AdditiveBlendNode.DesiredAlphas.GetData(), AdditiveBlendNode.DesiredAlphas.GetAllocatedSize());
}

FAnimSequencerInstanceProxy::~FAnimSequencerInstanceProxy()
{
	ClearSequencePlayerAndMirrorMaps();
}

void FAnimSequencerInstanceProxy::InitAnimTrack(UAnimSequenceBase* InAnimSequence, uint32 SequenceId)
{
	if (InAnimSequence != nullptr)
	{
		FSequencerPlayerAnimSequence* PlayerState = FindPlayer<FSequencerPlayerAnimSequence>(SequenceId);
		if (PlayerState == nullptr)
		{
			const bool bIsAdditive = InAnimSequence->IsValidAdditive();
			FAnimNode_MultiWayBlend& BlendNode = (bIsAdditive) ? AdditiveBlendNode : FullBodyBlendNode;
			
			// you shouldn't allow additive animation to be added here, but if it changes type after
			// 您不应该允许在此处添加附加动画，但如果它在之后更改了类型
			// you'll see this warning coming up
			// 你会看到这个警告出现
			if (bIsAdditive && InAnimSequence->GetAdditiveAnimType() == AAT_RotationOffsetMeshSpace)
			{
				// this doesn't work
				// 这不起作用
				UE_LOG(LogAnimation, Warning, TEXT("ERROR: Animation [%s] in Sequencer has Mesh Space additive animation.  No support on mesh space additive animation. "), *GetNameSafe(InAnimSequence));
			}

			const int32 PoseIndex = BlendNode.AddPose() - 1;

			// add the new entry to map
			// 将新条目添加到地图
			FSequencerPlayerAnimSequence* NewPlayerState = new FSequencerPlayerAnimSequence();
			NewPlayerState->PoseIndex = PoseIndex;
			NewPlayerState->bAdditive = bIsAdditive;
			
			SequencerToPlayerMap.Add(SequenceId, NewPlayerState);

			// link player to mirror node,
			// 将玩家链接到镜像节点，
			FAnimNode_Mirror_Standalone* NewMirrorNode = new FAnimNode_Mirror_Standalone();
			NewMirrorNode->SetMirror(false);
			NewMirrorNode->SetSourceLinkNode(&NewPlayerState->PlayerNode);
			SequencerToMirrorMap.Add(SequenceId, NewMirrorNode); 

			// link mirror to blendnode, this will let you trigger notifies and so on
			// 将镜像链接到混合节点，这将让您触发通知等
			NewPlayerState->PlayerNode.SetTeleportToExplicitTime(false);
			BlendNode.Poses[PoseIndex].SetLinkNode(NewMirrorNode);

			// set player state
			// 设置玩家状态
			PlayerState = NewPlayerState;
		}

		// now set animation data to player
		// 现在将动画数据设置为播放器
		PlayerState->PlayerNode.SetSequence(InAnimSequence);
		PlayerState->PlayerNode.SetExplicitTime(0.f);

		// initialize player
		// 初始化播放器
		PlayerState->PlayerNode.Initialize_AnyThread(FAnimationInitializeContext(this));

		FAnimNode_Mirror_Standalone* Mirror = SequencerToMirrorMap.FindRef(SequenceId);
		if (Mirror)
		{
			Mirror->Initialize_AnyThread(FAnimationInitializeContext(this));
			Mirror->CacheBones_AnyThread(FAnimationCacheBonesContext(this));
		}
	}
}

/*
// this isn't used yet. If we want to optimize it, we could do this way, but right now the way sequencer updates, we don't have a good point 
// 这个还没用过。如果我们想优化它，我们可以这样做，但是现在定序器更新的方式，我们没有一个好的点
// where we could just clear one sequence id. We just clear all the weights before update. 
// 我们可以只清除一个序列 ID。我们只是在更新之前清除所有权重。
// once they go out of range, they don't get called anymore, so there is no good point of tearing down
// 一旦超出范围，就不会再被调用，所以拆除没有什么好处
// there is multiple tear down point but we couldn't find where only happens once activated and once getting out
// 有多个拆卸点，但我们找不到只有在激活和退出后才会发生的位置
// because sequencer finds the nearest point, not exact point, it doesn't have good point of tearing down
// 因为sequencer找到的是最近的点，而不是精确的点，所以它没有好的拆解点
void FAnimSequencerInstanceProxy::TermAnimTrack(int32 SequenceId)
{
	FSequencerPlayerState* PlayerState = FindPlayer(SequenceId);

	if (PlayerState)
	{
		FAnimNode_MultiWayBlend& BlendNode = (PlayerState->bAdditive) ? AdditiveBlendNode : FullBodyBlendNode;

		// remove the pose from blend node
		// 从混合节点中删除姿势
		BlendNode.Poses.RemoveAt(PlayerState->PoseIndex);
		BlendNode.DesiredAlphas.RemoveAt(PlayerState->PoseIndex);

		// remove from Sequence Map
		// 从序列图中删除
		SequencerToPlayerMap.Remove(SequenceId);
	}
}*/

void FAnimSequencerInstanceProxy::UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, uint32 SequenceId, float InPosition, float Weight, bool bFireNotifies)
{
	UpdateAnimTrack(InAnimSequence, SequenceId, TOptional<FRootMotionOverride>(), TOptional<float>(), InPosition, Weight, bFireNotifies, nullptr);
}

void FAnimSequencerInstanceProxy::UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, uint32 SequenceId, TOptional<float> InFromPosition, float InToPosition, float Weight, bool bFireNotifies)
{
	UpdateAnimTrack(InAnimSequence, SequenceId, TOptional<FRootMotionOverride>(), InFromPosition, InToPosition, Weight, bFireNotifies, nullptr);
}

void FAnimSequencerInstanceProxy::UpdateAnimTrackWithRootMotion(UAnimSequenceBase* InAnimSequence, int32 SequenceId, const TOptional<FRootMotionOverride>& RootMotion, float InFromPosition, float InToPosition, float Weight, bool bFireNotifies)
{
	UpdateAnimTrack(InAnimSequence, SequenceId, RootMotion, InFromPosition, InToPosition, Weight, bFireNotifies, nullptr);
}

void FAnimSequencerInstanceProxy::UpdateAnimTrackWithRootMotion(UAnimSequenceBase* InAnimSequence, int32 SequenceId, const TOptional<FRootMotionOverride>& RootMotion, float InFromPosition, float InToPosition, float Weight, bool bFireNotifies, UMirrorDataTable* InMirrorDataTable)
{
	UpdateAnimTrack(InAnimSequence, SequenceId, RootMotion, InFromPosition, InToPosition, Weight, bFireNotifies, InMirrorDataTable);
}

void FAnimSequencerInstanceProxy::UpdateAnimTrackWithRootMotion(const FAnimSequencerData& InAnimSequencerData)
{
	SwapRootBone = InAnimSequencerData.SwapRootBone;
	InitialTransform = InAnimSequencerData.InitialTransform;
	UpdateAnimTrack(InAnimSequencerData.AnimSequence, InAnimSequencerData.SequenceId, InAnimSequencerData.RootMotion, InAnimSequencerData.FromPosition, InAnimSequencerData.ToPosition, InAnimSequencerData.Weight, InAnimSequencerData.bFireNotifies, InAnimSequencerData.MirrorDataTable);
}

void FAnimSequencerInstanceProxy::UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, uint32 SequenceId, const TOptional<FRootMotionOverride>& InRootMotionOverride, TOptional<float> InFromPosition, float InToPosition, float Weight, bool bFireNotifies, UMirrorDataTable* InMirrorDataTable)
{
	EnsureAnimTrack(InAnimSequence, SequenceId);

	FSequencerPlayerAnimSequence* PlayerState = FindPlayer<FSequencerPlayerAnimSequence>(SequenceId);

	PlayerState->PlayerNode.SetExplicitTime(InToPosition);
	if (InFromPosition.IsSet())
	{
		// Set the internal time accumulator at the "from" time so that the player node will correctly evaluate the
		// 将内部时间累加器设置为“起始”时间，以便玩家节点正确评估
		// desired "from/to" range. We also disable the reinitialization code so it doesn't mess up that time we
		// 所需的“从/到”范围。我们还禁用重新初始化代码，这样我们就不会搞砸了
		// just set.
		// 刚刚设置。
		PlayerState->PlayerNode.SetExplicitPreviousTime(InFromPosition.GetValue());
		PlayerState->PlayerNode.SetReinitializationBehavior(ESequenceEvalReinit::NoReset);
	}

	FAnimNode_Mirror_Standalone* MirrorNode = SequencerToMirrorMap.FindRef(SequenceId);
	if (MirrorNode)
	{
		MirrorNode->SetMirror(InMirrorDataTable != nullptr);
		UMirrorDataTable* OldMirrorDataTable = MirrorNode->GetMirrorDataTable();
		MirrorNode->SetMirrorDataTable(InMirrorDataTable);

		if (InMirrorDataTable && OldMirrorDataTable != InMirrorDataTable)
		{
			MirrorNode->CacheBones_AnyThread(FAnimationCacheBonesContext(this));
		}
	}

	// if no fire notifies, we can teleport to explicit time
	// 如果没有火灾通知，我们可以传送到明确的时间
	PlayerState->PlayerNode.SetTeleportToExplicitTime(!bFireNotifies);
	// if moving to 0.f, we mark this to teleport. Otherwise, do not use explicit time
	// 如果移动到 0.f，我们将其标记为传送。否则，不要使用明确的时间
	FAnimNode_MultiWayBlend& BlendNode = (PlayerState->bAdditive) ? AdditiveBlendNode : FullBodyBlendNode;
	BlendNode.DesiredAlphas[PlayerState->PoseIndex] = Weight;

	// if additive, apply alpha value correctlyeTick
	// 如果相加，则正确应用 alpha 值eTick
	// this will be used when apply additive is blending correct total alpha to additive
	// 当应用添加剂将正确的总 alpha 值与添加剂混合时，将使用此选项
	if (PlayerState->bAdditive)
	{
		SequencerRootNode.Alpha = BlendNode.GetTotalAlpha();
	}
	RootMotionOverride = InRootMotionOverride;
}
void FAnimSequencerInstanceProxy::EnsureAnimTrack(UAnimSequenceBase* InAnimSequence, uint32 SequenceId)
{
	FSequencerPlayerAnimSequence* PlayerState = FindPlayer<FSequencerPlayerAnimSequence>(SequenceId);
	if (!PlayerState)
	{
		InitAnimTrack(InAnimSequence, SequenceId);
	}
	else if (PlayerState->PlayerNode.GetSequence() != InAnimSequence)
	{
		PlayerState->PlayerNode.SetSequence(InAnimSequence);
	}
}


