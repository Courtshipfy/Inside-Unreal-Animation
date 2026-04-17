// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/AnimNode_RandomPlayer.h"

#include "Algo/BinarySearch.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTrace.h"
#include "Animation/AnimStats.h"
#include "Animation/AnimSyncScope.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_RandomPlayer)

FAnimNode_RandomPlayer::FAnimNode_RandomPlayer()
    : CurrentPlayDataIndex(0)
    , bShuffleMode(false)
{
}

void FAnimNode_RandomPlayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_Base::Initialize_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);

	// Create a sanitized list of valid entries and only operate on those from here on in.
 // 创建一个经过清理的有效条目列表，并且仅对从这里开始的条目进行操作。
	ValidEntries.Empty(Entries.Num());
	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++)
	{
		FRandomPlayerSequenceEntry* Entry = &Entries[EntryIndex];

		if (Entry->Sequence == nullptr)
		{
			continue;
		}

		// If the likelihood of this entry playing is nil, then skip it as well.
  // 如果该条目播放的可能性为零，则也跳过它。
		if (!bShuffleMode && Entry->ChanceToPlay <= SMALL_NUMBER)
		{
			continue;
		}

		ValidEntries.Push(Entry);
	}

	const int32 NumValidEntries = ValidEntries.Num();

	if (NumValidEntries == 0)
	{
		// early out here, no need to do anything at all if we're not playing anything
  // 很早就出来了，如果我们不玩任何东西，根本不需要做任何事情
		return;
	}

	NormalizedPlayChances.Empty(NormalizedPlayChances.Num());
	NormalizedPlayChances.AddUninitialized(NumValidEntries);

	// Sanitize the data and sum up the range of the random chances so that
 // 清理数据并总结随机机会的范围，以便
	// we can normalize the individual chances below.
 // 我们可以将下面的个体机会标准化。
	float SumChances = 0.0f;
	for (FRandomPlayerSequenceEntry* Entry : ValidEntries)
	{
		SumChances += Entry->ChanceToPlay;

		if (Entry->MaxLoopCount < Entry->MinLoopCount)
		{
			Swap(Entry->MaxLoopCount, Entry->MinLoopCount);
		}

		if (Entry->MaxPlayRate < Entry->MinPlayRate)
		{
			Swap(Entry->MaxPlayRate, Entry->MinPlayRate);
		}

		Entry->BlendIn.Reset();
	}

	if (bShuffleMode)
	{
		// Seed the shuffle list, ignoring all last entry checks, since we're doing the
  // 为随机列表添加种子，忽略所有最后的条目检查，因为我们正在执行
		// initial build and don't care about the non-repeatability property (yet).
  // 初始构建并且不关心不可重复性属性（还）。
		BuildShuffleList(INDEX_NONE);
	}
	else
	{
		// Ensure that our chance sum is non-"zero" and non-negative.
  // 确保我们的机会总和非“零”且非负。
		check(SumChances > SMALL_NUMBER);

		// Construct a cumulative distribution function so that we can look up the
  // 构造一个累积分布函数，以便我们可以查找
		// index of the sequence using binary search on the [0-1) random number.
  // 对 [0-1) 随机数使用二分查找的序列索引。
		float CurrentChance = 0.0f;
		for (int32 Idx = 0; Idx < NumValidEntries; ++Idx)
		{
			CurrentChance += ValidEntries[Idx]->ChanceToPlay / SumChances;
			NormalizedPlayChances[Idx] = CurrentChance;
		}
		// Remove rounding errors (possibly slightly padding out the chance of the last item)
  // 消除舍入错误（可能稍微填充最后一项的机会）
		NormalizedPlayChances[NumValidEntries - 1] = 1.0f;
	}

	// Initialize random stream and pick first entry
 // 初始化随机流并选择第一个条目
	RandomStream.Initialize(FPlatformTime::Cycles());

	PlayData.Empty(2);
	PlayData.AddDefaulted(2);

	int32 CurrentEntry = GetNextValidEntryIndex();
	int32 NextEntry = GetNextValidEntryIndex();

	// Initialize the animation data for the first and the next sequence so that we can properly
 // 初始化第一个和下一个序列的动画数据，以便我们可以正确地
	// blend between them.
 // 他们之间融合。
	FRandomAnimPlayData& CurrentData = GetPlayData(ERandomDataIndexType::Current);
	InitPlayData(CurrentData, CurrentEntry, 1.0f);

	FRandomAnimPlayData& NextData = GetPlayData(ERandomDataIndexType::Next);
	InitPlayData(NextData, NextEntry, 0.0f);
}

void FAnimNode_RandomPlayer::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	BlendWeight = Context.GetFinalBlendWeight();

	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	GetEvaluateGraphExposedInputs().Execute(Context);

	if (ValidEntries.Num() == 0)
	{
		// We don't have any entries, play data will be invalid - early out
  // 我们没有任何条目，播放数据将无效 - 提早退出
		return;
	}

	FRandomAnimPlayData* CurrentData = &GetPlayData(ERandomDataIndexType::Current);
	FRandomAnimPlayData* NextData = &GetPlayData(ERandomDataIndexType::Next);

	const UAnimSequenceBase* CurrentSequence = CurrentData->Entry->Sequence;

	// If we looped around, adjust the previous play time to always be before the current playtime,
 // 如果我们循环，将上一个播放时间调整为始终在当前播放时间之前，
	// since we can assume modulo. This makes the crossing check for the start time a lot simpler.
 // 因为我们可以假设模数。这使得开始时间的交叉检查变得更加简单。
	float AdjustedPreviousPlayTime = CurrentData->DeltaTimeRecord.GetPrevious();
	if (CurrentData->CurrentPlayTime < AdjustedPreviousPlayTime)
	{
		AdjustedPreviousPlayTime -= CurrentSequence->GetPlayLength();
	}

	// Did we cross the play start time? Decrement the loop counter. Once we're on the last loop, we can
 // 我们是否超过了比赛开始时间？减少循环计数器。一旦我们进入最后一个循环，我们就可以
	// start blending into the next animation.
 // 开始混合到下一个动画中。
	bool bHasLooped = AdjustedPreviousPlayTime < CurrentData->PlayStartTime && CurrentData->PlayStartTime <= CurrentData->CurrentPlayTime;
	if (bHasLooped)
	{
		// We've looped, update remaining
  // 我们已经循环了，更新剩余的
		--CurrentData->RemainingLoops;
	}

	bool bAdvanceToNextEntry = false;

	if (CurrentData->RemainingLoops <= 0)
	{
		const bool bNextAnimIsDifferent = CurrentData->Entry != NextData->Entry;

		// If we're in the blend window start blending, but only if we're moving to a new animation,
  // 如果我们在混合窗口中开始混合，但前提是我们要转向新的动画，
		// otherwise just keep looping.
  // 否则就继续循环。
		FRandomPlayerSequenceEntry& NextSequenceEntry = *NextData->Entry;

		// If the next animation is different, then smoothly blend between them. Otherwise
  // 如果下一个动画不同，则在它们之间平滑地混合。否则
		// we do a hard transition to the same play point. The next animation might play at
  // 我们很难过渡到同一个游戏点。下一个动画可能会在
		// a different rate, so we have to switch.
  // 不同的速率，所以我们必须切换。
		if (bNextAnimIsDifferent)
		{
			bool bDoBlending = false;

			// Are we already blending? Continue to do so. Special case for zero blend time as alpha will always be 1.
   // 我们已经混合了吗？继续这样做。零混合时间的特殊情况，因为 alpha 将始终为 1。
			if (NextSequenceEntry.BlendIn.GetBlendTime() > 0.0f && FAnimationRuntime::HasWeight(NextSequenceEntry.BlendIn.GetAlpha()))
			{
				bDoBlending = true;
			}
			else
			{
				// Check to see if we need to start the blending process.
    // 检查我们是否需要开始混合过程。
				float AmountPlayedSoFar = CurrentData->CurrentPlayTime - CurrentData->PlayStartTime;
				if (AmountPlayedSoFar < 0.0f)
				{
					AmountPlayedSoFar += CurrentSequence->GetPlayLength();
				}

				float TimeRemaining = CurrentSequence->GetPlayLength() - AmountPlayedSoFar;

				if (TimeRemaining <= NextSequenceEntry.BlendIn.GetBlendTime() || bHasLooped)
				{
					bDoBlending = true;
				}
			}

			if (bDoBlending)
			{
				// Blending to next
    // 混合到下一个
				NextSequenceEntry.BlendIn.Update(Context.GetDeltaTime());

				if (NextSequenceEntry.BlendIn.IsComplete())
				{
					// Set the play start time to be the current play time so that loop counts are properly
     // 将播放开始时间设置为当前播放时间，以便正确循环计数
					// maintained.
     // 维持。
					NextData->PlayStartTime = NextData->CurrentPlayTime;
					bAdvanceToNextEntry = true;
				}
				else
				{
					float BlendedAlpha = NextSequenceEntry.BlendIn.GetBlendedValue();

					if (BlendedAlpha < 1.0f)
					{
						NextData->BlendWeight = BlendedAlpha;
						CurrentData->BlendWeight = 1.0f - BlendedAlpha;
					}
				}
			}
		}
		else if (!bNextAnimIsDifferent && CurrentData->RemainingLoops < 0)
		{
			NextData->CurrentPlayTime = CurrentData->CurrentPlayTime;

			// Set the play start time to be the current play time so that loop counts are properly
   // 将播放开始时间设置为当前播放时间，以便正确循环计数
			// maintained.
   // 维持。
			NextData->PlayStartTime = NextData->CurrentPlayTime;
			bAdvanceToNextEntry = true;
		}
	}

	// Cache time to detect loops
 // 检测循环的缓存时间
	CurrentData->DeltaTimeRecord.SetPrevious(CurrentData->CurrentPlayTime);
	NextData->DeltaTimeRecord.SetPrevious(NextData->CurrentPlayTime);

	if (bAdvanceToNextEntry)
	{
		AdvanceToNextSequence();

		// Re-get data as we've switched over
  // 切换后重新获取数据
		CurrentData = &GetPlayData(ERandomDataIndexType::Current);
		NextData = &GetPlayData(ERandomDataIndexType::Next);
	}

	FAnimTickRecord TickRecord(CurrentData->Entry->Sequence, true, CurrentData->PlayRate, false, CurrentData->BlendWeight * BlendWeight, CurrentData->CurrentPlayTime, CurrentData->MarkerTickRecord);
	TickRecord.DeltaTimeRecord = &CurrentData->DeltaTimeRecord;
	TickRecord.GatherContextData(Context);

	UE::Anim::FAnimSyncGroupScope& SyncScope = Context.GetMessageChecked<UE::Anim::FAnimSyncGroupScope>();
	SyncScope.AddTickRecord(TickRecord, UE::Anim::FAnimSyncParams(), UE::Anim::FAnimSyncDebugInfo(Context));

	TRACE_ANIM_TICK_RECORD(Context, TickRecord);

	if (FAnimationRuntime::HasWeight(NextData->BlendWeight))
	{
		FAnimTickRecord NextTickRecord(NextData->Entry->Sequence, true, NextData->PlayRate, false, NextData->BlendWeight * BlendWeight, NextData->CurrentPlayTime, NextData->MarkerTickRecord);
		NextTickRecord.DeltaTimeRecord = &NextData->DeltaTimeRecord;
		NextTickRecord.GatherContextData(Context);

		SyncScope.AddTickRecord(NextTickRecord, UE::Anim::FAnimSyncParams(), UE::Anim::FAnimSyncDebugInfo(Context));

		TRACE_ANIM_TICK_RECORD(Context, NextTickRecord);
	}

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Current Sequence"), CurrentData ? CurrentData->Entry->Sequence : nullptr);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Current Weight"), CurrentData ? CurrentData->BlendWeight : 0.0f);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Next Sequence"), NextData ? NextData->Entry->Sequence : nullptr);
	TRACE_ANIM_NODE_VALUE(Context, TEXT("Next Weight"), NextData ? NextData->BlendWeight : 0.0f);
}

void FAnimNode_RandomPlayer::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(RandomPlayer, !IsInGameThread());

	if (ValidEntries.Num() == 0)
	{
		Output.ResetToRefPose();
		return;
	}

	FRandomAnimPlayData& CurrentData = GetPlayData(ERandomDataIndexType::Current);
	FRandomAnimPlayData& NextData = GetPlayData(ERandomDataIndexType::Next);

	UAnimSequenceBase* CurrentSequence = CurrentData.Entry->Sequence;

	if (!FMath::IsNearlyEqualByULP(CurrentData.BlendWeight, 1.0f))
	{
		FAnimInstanceProxy* AnimProxy = Output.AnimInstanceProxy;

		// Start Blending
  // 开始混合
		FCompactPose Poses[2];
		FBlendedCurve Curves[2];
		UE::Anim::FStackAttributeContainer Attributes[2];
		float Weights[2];

		const FBoneContainer& RequiredBone = AnimProxy->GetRequiredBones();
		Poses[0].SetBoneContainer(&RequiredBone);
		Poses[1].SetBoneContainer(&RequiredBone);

		Curves[0].InitFrom(RequiredBone);
		Curves[1].InitFrom(RequiredBone);

		Weights[0] = CurrentData.BlendWeight;
		Weights[1] = NextData.BlendWeight;

		UAnimSequenceBase* NextSequence = NextData.Entry->Sequence;

		FAnimationPoseData CurrentPoseData(Poses[0], Curves[0], Attributes[0]);
		FAnimationPoseData NextPoseData(Poses[1], Curves[1], Attributes[1]);

		CurrentSequence->GetAnimationPose(CurrentPoseData, FAnimExtractContext(static_cast<double>(CurrentData.CurrentPlayTime), AnimProxy->ShouldExtractRootMotion(), CurrentData.DeltaTimeRecord, CurrentData.RemainingLoops > 0));
		NextSequence->GetAnimationPose(NextPoseData, FAnimExtractContext(static_cast<double>(NextData.CurrentPlayTime), AnimProxy->ShouldExtractRootMotion(), NextData.DeltaTimeRecord, NextData.RemainingLoops > 0));

		FAnimationPoseData AnimationPoseData(Output);
		FAnimationRuntime::BlendPosesTogether(Poses, Curves, Attributes, Weights, AnimationPoseData);
	}
	else
	{
		// Single animation, no blending needed.
  // 单一动画，无需混合。
		FAnimationPoseData AnimationPoseData(Output);
		CurrentSequence->GetAnimationPose(AnimationPoseData, FAnimExtractContext(static_cast<double>(CurrentData.CurrentPlayTime), Output.AnimInstanceProxy->ShouldExtractRootMotion(), CurrentData.DeltaTimeRecord, CurrentData.RemainingLoops > 0));
	}
}

void FAnimNode_RandomPlayer::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);

	DebugData.AddDebugItem(DebugLine, true);
}

UAnimationAsset* FAnimNode_RandomPlayer::GetAnimAsset() const
{
	UAnimationAsset* AnimationAsset = nullptr;

	if (ValidEntries.Num() > 0)
	{
		const FRandomAnimPlayData& CurrentPlayData = GetPlayData(ERandomDataIndexType::Current);
		AnimationAsset = (CurrentPlayData.Entry != nullptr) ? CurrentPlayData.Entry->Sequence.Get() : nullptr;
	}

	return AnimationAsset;
}

float FAnimNode_RandomPlayer::GetAccumulatedTime() const
{
	float AccumulatedTime = 0.f;

	if (ValidEntries.Num() > 0)
	{
		const FRandomAnimPlayData& CurrentPlayData = GetPlayData(ERandomDataIndexType::Current);

		return CurrentPlayData.CurrentPlayTime;
	}

	return AccumulatedTime;
}

bool FAnimNode_RandomPlayer::GetIgnoreForRelevancyTest() const
{
	return GET_ANIM_NODE_DATA(bool, bIgnoreForRelevancyTest);
}

bool FAnimNode_RandomPlayer::SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest)
{
#if WITH_EDITORONLY_DATA
	bIgnoreForRelevancyTest = bInIgnoreForRelevancyTest;
#endif

	if (bool* bIgnoreForRelevancyTestPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bIgnoreForRelevancyTest))
	{
		*bIgnoreForRelevancyTestPtr = bInIgnoreForRelevancyTest;
		return true;
	}

	return false;
}

float FAnimNode_RandomPlayer::GetCachedBlendWeight() const
{
	return BlendWeight;
}

void FAnimNode_RandomPlayer::ClearCachedBlendWeight()
{
	BlendWeight = 0.f;
}

const FDeltaTimeRecord* FAnimNode_RandomPlayer::GetDeltaTimeRecord() const
{
	if (ValidEntries.Num() > 0)
	{
		const FRandomAnimPlayData& CurrentPlayData = GetPlayData(ERandomDataIndexType::Current);
		return &CurrentPlayData.DeltaTimeRecord;
	}
	return nullptr;
}

int32 FAnimNode_RandomPlayer::GetNextValidEntryIndex()
{
	check(ValidEntries.Num() > 0);

	if (bShuffleMode)
	{
		// Get the top value, don't allow realloc
  // 获取最高值，不允许重新分配
		int32 Index = ShuffleList.Pop(EAllowShrinking::No);

		// If we cleared the shuffles, rebuild for the next round, indicating
  // 如果我们清除了洗牌，则为下一轮重建，表明
		// the current value so that we don't pop that one off again next time.
  // 当前值，这样我们下次就不会再次弹出该值。
		if (ShuffleList.Num() == 0)
		{
			BuildShuffleList(Index);
		}

		return Index;
	}
	else
	{
		float RandomVal = RandomStream.GetFraction();

		// Search the cumulative distribution array for the last entry that's
  // 在累积分布数组中搜索最后一个条目
		// smaller or equal to the random value. That becomes our new animation.
  // 小于或等于随机值。这成为我们的新动画。
		return Algo::UpperBound(NormalizedPlayChances, RandomVal);
	}
}

FRandomAnimPlayData& FAnimNode_RandomPlayer::GetPlayData(ERandomDataIndexType Type)
{
	// PlayData only holds two entries. We swap between them in AdvanceToNextSequence
 // PlayData 仅保存两个条目。我们在 AdvanceToNextSequence 中交换它们
	// by setting CUrrentPlayDataIndex to either 0 or 1. Hence the modulo 2 magic below.
 // 通过将CUrrentPlayDataIndex 设置为0 或1。因此下面的模2 魔法。
	if (Type == ERandomDataIndexType::Current)
	{
		return PlayData[CurrentPlayDataIndex];
	}
	else
	{
		return PlayData[(CurrentPlayDataIndex + 1) % 2];
	}
}

const FRandomAnimPlayData& FAnimNode_RandomPlayer::GetPlayData(ERandomDataIndexType Type) const
{
	return const_cast<FAnimNode_RandomPlayer*>(this)->GetPlayData(Type);
}

void FAnimNode_RandomPlayer::InitPlayData(FRandomAnimPlayData& Data, int32 InValidEntryIndex, float InBlendWeight)
{
	FRandomPlayerSequenceEntry* Entry = ValidEntries[InValidEntryIndex];

	Data.Entry = Entry;
	Data.BlendWeight = InBlendWeight;
	Data.PlayRate = static_cast<float>(RandomStream.FRandRange(Entry->MinPlayRate, Entry->MaxPlayRate));
	Data.RemainingLoops = FMath::Clamp(RandomStream.RandRange(Entry->MinLoopCount, Entry->MaxLoopCount), 0, MAX_int32);

	Data.PlayStartTime = 0.0f;
	Data.CurrentPlayTime = 0.0f;
	Data.DeltaTimeRecord = FDeltaTimeRecord();
	Data.MarkerTickRecord.Reset();
}

void FAnimNode_RandomPlayer::AdvanceToNextSequence()
{
	// Get the next sequence entry to use.
 // 获取要使用的下一个序列条目。
	int32 NextEntry = GetNextValidEntryIndex();

	// Switch play data by flipping it between 0 and 1.
 // 通过在 0 和 1 之间翻转来切换播放数据。
	CurrentPlayDataIndex = (CurrentPlayDataIndex + 1) % 2;

	// Get our play data
 // 获取我们的游戏数据
	FRandomAnimPlayData& CurrentData = GetPlayData(ERandomDataIndexType::Current);
	FRandomAnimPlayData& NextData = GetPlayData(ERandomDataIndexType::Next);

	// Reset blend weights
 // 重置混合权重
	CurrentData.BlendWeight = 1.0f;
	CurrentData.Entry->BlendIn.Reset();

	// Set up data for next switch
 // 设置下一次切换的数据
	InitPlayData(NextData, NextEntry, 0.0f);
}

void FAnimNode_RandomPlayer::BuildShuffleList(int32 LastEntry)
{
	ShuffleList.Reset(ValidEntries.Num());

	// Build entry index list
 // 建立条目索引列表
	const int32 NumValidEntries = ValidEntries.Num();
	for (int32 i = 0; i < NumValidEntries; ++i)
	{
		ShuffleList.Add(i);
	}

	// Shuffle the list
 // 随机排列列表
	const int32 NumShuffles = ShuffleList.Num() - 1;
	for (int32 i = 0; i < NumShuffles; ++i)
	{
		int32 SwapIdx = RandomStream.RandRange(i, NumShuffles);
		ShuffleList.Swap(i, SwapIdx);
	}

	// Make sure we don't play the same thing twice in a row
 // 确保我们不会连续玩同一件事两次
	if (ShuffleList.Num() > 1 && ShuffleList.Last() == LastEntry)
	{
		// Swap the last with a random entry.
  // 将最后一个与随机条目交换。
		ShuffleList.Swap(RandomStream.RandRange(0, ShuffleList.Num() - 2), ShuffleList.Num() - 1);
	}
}

