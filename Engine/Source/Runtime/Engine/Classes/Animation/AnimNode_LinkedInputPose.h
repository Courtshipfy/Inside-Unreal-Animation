// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimCurveTypes.h"
#include "BonePose.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_LinkedInputPose.generated.h"

USTRUCT()
struct FAnimNode_LinkedInputPose : public FAnimNode_Base
{
	GENERATED_BODY()

	/** The default name of this input pose */
	/** 该输入姿势的默认名称 */
	/** 该输入姿势的默认名称 */
	/** 该输入姿势的默认名称 */
	static ENGINE_API const FName DefaultInputPoseName;

	FAnimNode_LinkedInputPose()
		: Name(DefaultInputPoseName)
		, Graph(NAME_None)
		, bIsCachedInputPoseInitialized(false)
		, bIsOutputLinked(true)
		, OuterGraphNodeIndex(INDEX_NONE)
		, InputProxy(nullptr)
	{
	}
	/** 此链接的输入姿势节点的姿势的名称，用于标识此图的输入。 */

	/** 此链接的输入姿势节点的姿势的名称，用于标识此图的输入。 */
	/** The name of this linked input pose node's pose, used to identify the input of this graph. */
	/** 此链接的输入姿势节点的姿势的名称，用于标识此图的输入。 */
	/** 该链接输入位姿节点所在的图形，由编译器填充 */
	UPROPERTY(EditAnywhere, Category = "Inputs", meta = (NeverAsPin))
	FName Name;
	/** 该链接输入位姿节点所在的图形，由编译器填充 */

	/** 输入姿势，可选择动态链接到另一个图形 */
	/** The graph that this linked input pose node is in, filled in by the compiler */
	/** 该链接输入位姿节点所在的图形，由编译器填充 */
	UPROPERTY()
	/** 输入姿势，可选择动态链接到另一个图形 */
	FName Graph;

	/** Input pose, optionally linked dynamically to another graph */
	/** 输入姿势，可选择动态链接到另一个图形 */
	UPROPERTY()
	FPoseLink InputPose;

	/** 
	 * If this linked input pose is not dynamically linked, this cached data will be populated by the calling 
	 * linked instance node before this graph is processed.
	 */
	FCompactHeapPose CachedInputPose;
	FBlendedHeapCurve CachedInputCurve;
	UE::Anim::FHeapAttributeContainer CachedAttributes;

	// CachedInputPose can have bone data allocated but uninitialized.
 // CachedInputPose 可以分配但未初始化的骨骼数据。
	// This can happen if an anim graph has an Input Pose node with nothing populating it (e.g. if it's played as the only animbp on an actor).
 // 如果动画图有一个输入姿势节点但没有填充任何内容（例如，如果它作为演员上唯一的动画播放），则可能会发生这种情况。
	uint8 bIsCachedInputPoseInitialized : 1;

	// True if this linked input pose output is connected to the graph root
 // 如果此链接的输入姿势输出连接到图根，则为 True
	UPROPERTY(meta = (BlueprintCompilerGeneratedDefaults))
	uint8 bIsOutputLinked : 1;

	// The node index of the currently-linked outer node
 // 当前链接的外节点的节点索引
	int32 OuterGraphNodeIndex;



	// FAnimNode_Base interface
 // FAnimNode_Base接口
#if ENABLE_ANIMGRAPH_TRAVERSAL_DEBUG
	/** 由链接的实例节点调用以动态地将其链接到外部图 */
	/** 由链接的实例节点调用以动态地将其链接到外部图 */
	ENGINE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ENGINE_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
#endif
	/** 由链接的实例节点调用以动态取消其与外部图的链接 */
	/** 由链接的实例节点调用以动态取消其与外部图的链接 */
	ENGINE_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ENGINE_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ENGINE_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	/** 获取输入时使用的代理，动态链接时设置 */
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束
	/** 获取输入时使用的代理，动态链接时设置 */

	/** Called by linked instance nodes to dynamically link this to an outer graph */
	/** 由链接的实例节点调用以动态地将其链接到外部图 */
	ENGINE_API void DynamicLink(FAnimInstanceProxy* InInputProxy, FPoseLinkBase* InPoseLink, int32 InOuterGraphNodeIndex);

	/** Called by linked instance nodes to dynamically unlink this to an outer graph */
	/** 由链接的实例节点调用以动态取消其与外部图的链接 */
	ENGINE_API void DynamicUnlink();

private:
	/** The proxy to use when getting inputs, set when dynamically linked */
	/** 获取输入时使用的代理，动态链接时设置 */
	FAnimInstanceProxy* InputProxy;
};

UE_DEPRECATED(4.24, "FAnimNode_SubInput has been renamed to FAnimNode_LinkedInputPose")
typedef FAnimNode_LinkedInputPose FAnimNode_SubInput;
