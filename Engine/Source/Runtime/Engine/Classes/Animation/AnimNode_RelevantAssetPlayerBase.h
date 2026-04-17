// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_RelevantAssetPlayerBase.generated.h"

/* Base class for any asset playing anim node */
/* 任何播放动画节点的资源的基类 */
/* 任何播放动画节点的资源的基类 */
/* 任何播放动画节点的资源的基类 */
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_AssetPlayerRelevancyBase : public FAnimNode_Base
{
	GENERATED_BODY()

	/** 获取与节点关联的动画资源，派生类应实现此功能 */
public:
	/** 获取与节点关联的动画资源，派生类应实现此功能 */
	/** Get the animation asset associated with the node, derived classes should implement this */
	/** 获取资产播放器节点内当前引用的时间 */
	/** 获取与节点关联的动画资源，派生类应实现此功能 */
	ENGINE_API virtual class UAnimationAsset* GetAnimAsset() const;
	/** 获取资产播放器节点内当前引用的时间 */
	/** 覆盖当前累计时间 */

	/** Get the currently referenced time within the asset player node */
	/** 获取资产播放器节点内当前引用的时间 */
	/** 覆盖当前累计时间 */
	ENGINE_API virtual float GetAccumulatedTime() const;

	/** Override the currently accumulated time */
	/** 覆盖当前累计时间 */
	ENGINE_API virtual void SetAccumulatedTime(float NewTime);

	// Functions to report data to getters, this is required for all asset players (but can't be pure abstract because of struct instantiation generated code).
 // 向 getter 报告数据的函数，这是所有资产播放器所必需的（但由于结构实例化生成的代码而不能是纯抽象的）。
	ENGINE_API virtual float GetCurrentAssetLength() const;
	ENGINE_API virtual float GetCurrentAssetTime() const;
	ENGINE_API virtual float GetCurrentAssetTimePlayRateAdjusted() const;

	// Does this asset player loop back to the start when it reaches the end?
 // 该资产播放器到达结尾时是否会循环回到开头？
	ENGINE_API virtual bool IsLooping() const;
	/** 获取该节点最后遇到的混合权重 */

	// Check whether this node should be ignored when testing for relevancy in state machines
 // 检查在状态机中测试相关性时是否应忽略此节点
	/** 获取该节点最后遇到的混合权重 */
	/** 将缓存的混合权重设置为零 */
	ENGINE_API virtual bool GetIgnoreForRelevancyTest() const;

	/** 将缓存的混合权重设置为零 */
	/** 获取该资产播放器拥有的增量时间记录（或 null） */
	// Set whether this node should be ignored when testing for relevancy in state machines
 // 设置在状态机中测试相关性时是否应忽略此节点
	ENGINE_API virtual bool SetIgnoreForRelevancyTest(bool bInIgnoreForRelevancyTest);
	/** 获取该资产播放器拥有的增量时间记录（或 null） */

	/** Get the last encountered blend weight for this node */
	/** 获取该节点最后遇到的混合权重 */
	ENGINE_API virtual float GetCachedBlendWeight() const;

	/** Set the cached blendweight to zero */
	/** 将缓存的混合权重设置为零 */
	ENGINE_API virtual void ClearCachedBlendWeight();

	/** Get the delta time record owned by this asset player (or null) */
	/** 获取该资产播放器拥有的增量时间记录（或 null） */
	ENGINE_API virtual const FDeltaTimeRecord* GetDeltaTimeRecord() const;
};
