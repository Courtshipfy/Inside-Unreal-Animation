// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimNode_CustomProperty.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimLayerInterface.h"

#include "AnimNode_LinkedAnimGraph.generated.h"

struct FAnimInstanceProxy;
class UUserDefinedStruct;
struct FAnimBlueprintFunction;
class IAnimClassInterface;

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_LinkedAnimGraph : public FAnimNode_CustomProperty
{
	GENERATED_BODY()

public:

	ENGINE_API FAnimNode_LinkedAnimGraph();

	/** 
	 *  Input poses for the node, intentionally not accessible because if there's no input
	 *  nodes in the target class we don't want to show these as pins
	 */
	UPROPERTY()
	TArray<FPoseLink> InputPoses;

	/** List of input pose names, 1-1 with pose links about, built by the compiler */
	/** 输入姿势名称列表，1-1 带有姿势链接，由编译器构建 */
	UPROPERTY()
	TArray<FName> InputPoseNames;

	/** The class spawned for this linked instance */
	/** 为此链接实例生成的类 */
	UPROPERTY(EditAnywhere, Category = Settings)
	TSubclassOf<UAnimInstance> InstanceClass;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FName Tag_DEPRECATED;
#endif
	
	// The root node of the dynamically-linked graph
	// 动态链接图的根节点
	FAnimNode_Base* LinkedRoot;

	// Our node index
	// 我们的节点索引
	int32 NodeIndex;

	// Cached node index for our linked function
	// 我们链接函数的缓存节点索引
	int32 CachedLinkedNodeIndex;

protected:
	// Inertial blending duration to request next update (pulled from the prior state's blend out)
	// 请求下一次更新的惯性混合持续时间（从先前状态的混合中提取）
	float PendingBlendOutDuration;

	// Optional blend profile to use during inertial blending (pulled from the prior state's blend out)
	// 惯性混合期间使用的可选混合配置文件（从先前状态的混合中提取）
	UPROPERTY(Transient)
	TObjectPtr<const UBlendProfile> PendingBlendOutProfile;

	// Inertial blending duration to request next update (pulled from the new state's blend in)
	// 请求下一次更新的惯性混合持续时间（从新状态的混合中提取）
	float PendingBlendInDuration;

	// Optional blend profile to use during inertial blending (pulled from the new state's blend in)
	// 惯性混合期间使用的可选混合配置文件（从新状态的混合中拉出）
	UPROPERTY(Transient)
	TObjectPtr<const UBlendProfile> PendingBlendInProfile;

public:
	/** Whether named notifies will be received by this linked instance from other instances (outer or other linked instances) */
	/** 此链接实例是否将从其他实例（外部或其他链接实例）接收命名通知 */
	UPROPERTY(EditAnywhere, Category = Settings)
	uint8 bReceiveNotifiesFromLinkedInstances : 1;

	/** Whether named notifies will be propagated from this linked instance to other instances (outer or other linked instances) */
	/** 命名通知是否将从该链接实例传播到其他实例（外部或其他链接实例） */
	UPROPERTY(EditAnywhere, Category = Settings)
	uint8 bPropagateNotifiesToLinkedInstances : 1;

	/** Dynamically set the anim class of this linked instance */
	/** 动态设置此链接实例的动画类 */
	ENGINE_API void SetAnimClass(TSubclassOf<UAnimInstance> InClass, const UAnimInstance* InOwningAnimInstance);

	/** Get the function name we should be linking with when we call DynamicLink/Unlink */
	/** 获取调用 DynamicLink/Unlink 时应链接的函数名称 */
	ENGINE_API virtual FName GetDynamicLinkFunctionName() const;

	/** Get the dynamic link target */
	/** 获取动态链接目标 */
	ENGINE_API virtual UAnimInstance* GetDynamicLinkTarget(UAnimInstance* InOwningAnimInstance) const;

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ENGINE_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ENGINE_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ENGINE_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ENGINE_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ENGINE_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	// Initializes only the sub-graph that this node is linked to
	// 仅初始化该节点链接到的子图
	ENGINE_API void InitializeSubGraph_AnyThread(const FAnimationInitializeContext& Context);

	// Caches bones only for the sub graph that this node is linked to
	// 仅缓存该节点链接到的子图的骨骼
	ENGINE_API void CacheBonesSubGraph_AnyThread(const FAnimationCacheBonesContext& Context);

protected:

	ENGINE_API virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// Re-create the linked instances for this node
	// 重新创建该节点的链接实例
	ENGINE_API void ReinitializeLinkedAnimInstance(const UAnimInstance* InOwningAnimInstance, UAnimInstance* InNewAnimInstance = nullptr);

	// Shutdown the currently running instance
	// 关闭当前正在运行的实例
	ENGINE_API void TeardownInstance(const UAnimInstance* InOwningAnimInstance);

	// Check if the currently linked instance can be teared down
	// 检查当前链接的实例是否可以拆除
	virtual bool CanTeardownLinkedInstance(const UAnimInstance* LinkedInstance) const {return true;}

	// FAnimNode_CustomProperty interface
	// FAnimNode_CustomProperty接口
	virtual UClass* GetTargetClass() const override 
	{
		return *InstanceClass;
	}

#if WITH_EDITOR
	ENGINE_API virtual void HandleObjectsReinstanced_Impl(UObject* InSourceObject, UObject* InTargetObject, const TMap<UObject*, UObject*>& OldToNewInstanceMap) override;
#endif	// #if WITH_EDITOR

	/** Link up pose links dynamically with linked instance */
	/** 将姿势链接与链接实例动态链接 */
	ENGINE_API void DynamicLink(UAnimInstance* InOwningAnimInstance);

	/** Break any pose links dynamically with linked instance */
	/** 使用链接实例动态断开任何姿势链接 */
	ENGINE_API void DynamicUnlink(UAnimInstance* InOwningAnimInstance);

	/** Helper function for finding function inputs when linking/unlinking */
	/** 用于在链接/取消链接时查找函数输入的辅助函数 */
	ENGINE_API int32 FindFunctionInputIndex(const FAnimBlueprintFunction& AnimBlueprintFunction, const FName& InInputName);

	/** Request a blend when the active instance changes */
	/** 当活动实例发生变化时请求混合 */
	ENGINE_API void RequestBlend(const IAnimClassInterface* PriorAnimBPClass, const IAnimClassInterface* NewAnimBPClass);

	friend class UAnimInstance;

	// Stats
	// 统计数据
#if ANIMNODE_STATS_VERBOSE
	ENGINE_API virtual void InitializeStatID() override;
#endif

};

UE_DEPRECATED(4.24, "FAnimNode_SubInstance has been renamed to FAnimNode_LinkedAnimGraph")
typedef FAnimNode_LinkedAnimGraph FAnimNode_SubInstance;
