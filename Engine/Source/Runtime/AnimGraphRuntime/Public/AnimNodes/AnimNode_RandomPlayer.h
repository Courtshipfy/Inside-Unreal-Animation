// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AlphaBlend.h"
#include "Animation/AnimNode_RelevantAssetPlayerBase.h"
#include "Animation/AnimationAsset.h"
#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "UObject/ObjectMacros.h"

#include "AnimNode_RandomPlayer.generated.h"

enum class ERandomDataIndexType
{
	Current,
	Next,
};

/** The random player node holds a list of sequences and parameter ranges which will be played continuously
  * In a random order. If shuffle mode is enabled then each entry will be played once before repeating any
  */
USTRUCT(BlueprintInternalUseOnly)
struct FRandomPlayerSequenceEntry
{
	GENERATED_BODY()

	FRandomPlayerSequenceEntry()
	    : Sequence(nullptr)
	    , ChanceToPlay(1.0f)
	    , MinLoopCount(0)
	    , MaxLoopCount(0)
	    , MinPlayRate(1.0f)
	    , MaxPlayRate(1.0f)
	{
	}

	/** Sequence to play when this entry is picked */
	/** 选择该条目时播放的顺序 */
	/** 选择该条目时播放的顺序 */
	/** 选择该条目时播放的顺序 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (DisallowedClasses = "/Script/Engine.AnimMontage"))
	TObjectPtr<UAnimSequenceBase> Sequence;
	/** 当不处于随机播放模式时，这是该条目将播放的机会（针对所有其他样本机会进行标准化） */

	/** 当不处于随机播放模式时，这是该条目将播放的机会（针对所有其他样本机会进行标准化） */
	/** When not in shuffle mode, this is the chance this entry will play (normalized against all other sample chances) */
	/** 当不处于随机播放模式时，这是该条目将播放的机会（针对所有其他样本机会进行标准化） */
	/** 该条目在结束之前循环的最小次数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (UIMin = "0", ClampMin = "0"))
	float ChanceToPlay;
	/** 该条目在结束之前循环的最小次数 */

	/** 该条目在结束之前循环的最大次数 */
	/** Minimum number of times this entry will loop before ending */
	/** 该条目在结束之前循环的最小次数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (UIMin = "0", ClampMin = "0"))
	/** 该条目在结束之前循环的最大次数 */
	/** 此条目的最低播放率 */
	int32 MinLoopCount;

	/** Maximum number of times this entry will loop before ending */
	/** 该条目在结束之前循环的最大次数 */
	/** 此条目的最大播放速率 */
	/** 此条目的最低播放率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (UIMin = "0", ClampMin = "0"))
	int32 MaxLoopCount;

	/** 当此条目混合在另一个条目之上时使用的混合属性 */
	/** Minimum playrate for this entry */
	/** 此条目的最大播放速率 */
	/** 此条目的最低播放率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (UIMin = "0", ClampMin = "0"))
	float MinPlayRate;

	/** 当此条目混合在另一个条目之上时使用的混合属性 */
	/** Maximum playrate for this entry */
	/** 此条目的最大播放速率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (UIMin = "0", ClampMin = "0"))
	float MaxPlayRate;

	/** Blending properties used when this entry is blending in ontop of another entry */
	/** 当此条目混合在另一个条目之上时使用的混合属性 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FAlphaBlend BlendIn;
};

struct FRandomAnimPlayData
{
	// Index into the real sequence entry list, not the valid entry list.
 // 索引到真实序列条目列表，而不是有效条目列表。
	FRandomPlayerSequenceEntry* Entry = nullptr;

	// The time at which the animation started playing. Used to initialize
 // 动画开始播放的时间。用于初始化
	// the play for this animation and detect when a loop has occurred.
 // 播放此动画并检测何时发生循环。
	float PlayStartTime = 0.0f;

	// The time at which the animation is currently playing.
 // 动画当前播放的时间。
	float CurrentPlayTime = 0.0f;

	// Delta time record for this play through
 // 本次游戏的 Delta 时间记录
	FDeltaTimeRecord DeltaTimeRecord;

	// Calculated play rate
 // 计算播放率
	float PlayRate = 0.0f;

	// Current blend weight
 // 当前混合重量
	float BlendWeight = 0.0f;

	// Calculated loops remaining
 // 计算剩余循环数
	/** 要随机逐步执行的序列列表 */
	/** 要随机逐步执行的序列列表 */
	int32 RemainingLoops = 0;

 // FAnimNode_RelevantAssetPlayerBase
 // FAnimNode_RelevantAssetPlayerBase
 // FAnimNode_RelevantAssetPlayerBase
 // FAnimNode_RelevantAssetPlayerBase
	// Marker tick record for this play through
 // 本次播放的标记刻度记录
	FMarkerTickRecord MarkerTickRecord;
};

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_RandomPlayer : public FAnimNode_AssetPlayerRelevancyBase
{
	GENERATED_BODY()
	// FAnimNode_RelevantAssetPlayerBase
 // FAnimNode_RelevantAssetPlayerBase
	// FAnimNode_RelevantAssetPlayerBase
 // FAnimNode_RelevantAssetPlayerBase

	ANIMGRAPHRUNTIME_API FAnimNode_RandomPlayer();

public:
	/** List of sequences to randomly step through */
	/** 要随机逐步执行的序列列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<FRandomPlayerSequenceEntry> Entries;

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

	// FAnimNode_RelevantAssetPlayerBase
 // FAnimNode_RelevantAssetPlayerBase
	// FAnimNode_RelevantAssetPlayerBase
 // FAnimNode_RelevantAssetPlayerBase
	ANIMGRAPHRUNTIME_API virtual UAnimationAsset* GetAnimAsset() const override;
	ANIMGRAPHRUNTIME_API virtual float GetAccumulatedTime() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetIgnoreForRelevancyTest() const override;
	ANIMGRAPHRUNTIME_API virtual bool SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest) override;
	ANIMGRAPHRUNTIME_API virtual float GetCachedBlendWeight() const override;
	ANIMGRAPHRUNTIME_API virtual void ClearCachedBlendWeight() override;
	ANIMGRAPHRUNTIME_API virtual const FDeltaTimeRecord* GetDeltaTimeRecord() const override;
	// End of FAnimNode_RelevantAssetPlayerBase
 // FAnimNode_RelevantAssetPlayerBase 结束

private:
	// Return the index of the next FRandomPlayerSequenceEntry to play, from the list
 // 返回列表中下一个要播放的 FRandomPlayerSequenceEntry 的索引
	// of valid playable entries (ValidEntries).
 // 有效的可播放条目（ValidEntries）。
	int32 GetNextValidEntryIndex();

	// Return the play data for either the currently playing animation or the next
 // 返回当前正在播放的动画或下一个动画的播放数据
	// animation to blend into.
 // 要融入的动画。
	FRandomAnimPlayData& GetPlayData(ERandomDataIndexType Type);
	const FRandomAnimPlayData& GetPlayData(ERandomDataIndexType Type) const;

	// Initialize the play data with the given index into the ValidEntries array and
 // 使用给定索引将播放数据初始化到 ValidEntries 数组中，并
	// a specific blend weight. All other member data will be reset to their default values.
 // 特定的混合重量。所有其他成员数据将重置为其默认值。
	void InitPlayData(FRandomAnimPlayData& Data, int32 InValidEntryIndex, float InBlendWeight);

	// Advance to the next playable sequence. This is only called once a sequence is fully
 // 前进到下一个可播放的序列。仅当序列完全完成后才会调用此函数
	// blended or there's a hard switch to the same playable entry.
 // 混合或硬切换到相同的可播放条目。
	void AdvanceToNextSequence();

	// Build a new ShuffleList array, which is a shuffled index list of all the valid
 // 构建一个新的ShuffleList数组，它是所有有效的打乱索引列表
	// playable entries in ValidEntries. The LastEntry can be set to a valid entry index to
 // ValidEntries 中的可播放条目。 LastEntry 可以设置为有效的条目索引
	/** 该节点最后遇到的混合权重 */
	// ensure that the top/last item in the shuffle list will be a different value from it;
 // 确保随机列表中的顶部/最后一项与其具有不同的值；
	// pass in INDEX_NONE to disable the check.
 // 传入 INDEX_NONE 以禁用检查。
	void BuildShuffleList(int32 LastEntry);

	// List of valid sequence entries
 // 有效序列条目列表
	TArray<FRandomPlayerSequenceEntry*> ValidEntries;

	// Normalized list of play chances when we aren't using shuffle mode
 // 当我们不使用随机播放模式时的标准化播放机会列表
	TArray<float> NormalizedPlayChances;

	// Play data for the current and next sequence
 // 播放当前和下一个序列的数据
	/** 该节点最后遇到的混合权重 */
	TArray<FRandomAnimPlayData> PlayData;

	// Index of the 'current' data set in the PlayData array.
 // PlayData 数组中“当前”数据集的索引。
	int32 CurrentPlayDataIndex;

	// List to store transient shuffle stack in shuffle mode.
 // 在随机播放模式下存储临时随机播放堆栈的列表。
	TArray<int32> ShuffleList;

	// Random number source
 // 随机数来源
	FRandomStream RandomStream;

#if WITH_EDITORONLY_DATA
	// If true, "Relevant anim" nodes that look for the highest weighted animation in a state will ignore this node
 // 如果为 true，则在某个状态中查找权重最高的动画的“相关动画”节点将忽略此节点
	UPROPERTY(EditAnywhere, Category = Relevancy, meta = (FoldProperty, PinHiddenByDefault))
	bool bIgnoreForRelevancyTest = false;
#endif // WITH_EDITORONLY_DATA

protected:
	/** Last encountered blend weight for this node */
	/** 该节点最后遇到的混合权重 */
	UPROPERTY(BlueprintReadWrite, Transient, Category = DoNotEdit)
	float BlendWeight = 0.0f;

public:
	/** When shuffle mode is active we will never loop a sequence beyond MaxLoopCount
	  * without visiting each sequence in turn (no repeats). Enabling this will ignore
	  * ChanceToPlay for each entry
	  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bShuffleMode;
};
