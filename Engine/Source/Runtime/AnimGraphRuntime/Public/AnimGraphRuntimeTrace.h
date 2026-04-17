// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimTrace.h"

#if ANIM_TRACE_ENABLED

struct FAnimationBaseContext;
struct FAnimNode_BlendSpacePlayerBase;
struct FAnimNode_BlendSpaceGraphBase;

struct FAnimGraphRuntimeTrace
{
	/** Helper function to output debug info for blendspace player nodes */
	/** 用于输出混合空间玩家节点调试信息的辅助函数 */
	/** 用于输出混合空间玩家节点调试信息的辅助函数 */
	/** 用于输出混合空间玩家节点调试信息的辅助函数 */
	ANIMGRAPHRUNTIME_API static void OutputBlendSpacePlayer(const FAnimationBaseContext& InContext, const FAnimNode_BlendSpacePlayerBase& InNode);
	/** 用于输出混合空间节点调试信息的辅助函数 */

	/** 用于输出混合空间节点调试信息的辅助函数 */
	/** Helper function to output debug info for blendspace nodes */
	/** 用于输出混合空间节点调试信息的辅助函数 */
	ANIMGRAPHRUNTIME_API static void OutputBlendSpace(const FAnimationBaseContext& InContext, const FAnimNode_BlendSpaceGraphBase& InNode);
};

#define TRACE_BLENDSPACE_PLAYER(Context, Node) \
	FAnimGraphRuntimeTrace::OutputBlendSpacePlayer(Context, Node);

#define TRACE_BLENDSPACE(Context, Node) \
	FAnimGraphRuntimeTrace::OutputBlendSpace(Context, Node);

#else

#define TRACE_BLENDSPACE_PLAYER(Context, Node)
#define TRACE_BLENDSPACE(Context, Node)

#endif