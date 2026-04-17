// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "BoneControllers/AnimNode_RigidBody.h"
#include "AnimNode_RigidBody_Library.generated.h"

USTRUCT(BlueprintType)
struct FRigidBodyAnimNodeReference : public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_RigidBody FInternalNodeType;
};

// Exposes operations to be performed on a rigid body anim node
// 公开要在刚体动画节点上执行的操作
UCLASS(Experimental, MinimalAPI)
class UAnimNodeRigidBodyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get a rigid body anim node context from an anim node context */
	/** 从动画节点上下文获取刚体动画节点上下文 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Dynamics", meta = (BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static ANIMGRAPHRUNTIME_API FRigidBodyAnimNodeReference ConvertToRigidBodyAnimNode(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);

	/** Get a rigid body anim node context from an anim node context (pure) */
	/** 从动画节点上下文获取刚体动画节点上下文（纯） */
	UFUNCTION(BlueprintPure, Category = "Animation|Dynamics", meta = (BlueprintThreadSafe, DisplayName = "Convert to rigid body"))
	static void ConvertToRigidBodyAnimNodePure(const FAnimNodeReference& Node, FRigidBodyAnimNodeReference& RigidBodyAnimNode, bool& Result);

	/** Set the physics asset on the rigid body anim graph node (RBAN). */
	/** 在刚体动画图形节点 (RBAN) 上设置物理资源。 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Dynamics", meta = (BlueprintThreadSafe))
	static ANIMGRAPHRUNTIME_API FRigidBodyAnimNodeReference SetOverridePhysicsAsset(const FRigidBodyAnimNodeReference& Node, UPhysicsAsset* PhysicsAsset);
};
