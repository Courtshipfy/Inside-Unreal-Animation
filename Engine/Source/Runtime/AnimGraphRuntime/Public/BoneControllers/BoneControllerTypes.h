// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BoneControllerTypes.generated.h"

struct FAnimInstanceProxy;

// Specifies the evaluation mode of an animation warping node
// 指定动画扭曲节点的评估模式
UENUM(BlueprintInternalUseOnly)
enum class EWarpingEvaluationMode : uint8
{
	// Animation warping evaluation parameters are driven by user settings.
 // 动画变形评估参数由用户设置驱动。
	Manual,
	// Animation warping evaluation parameters are graph-driven. This means some
 // 动画变形评估参数是图形驱动的。这意味着一些
	// properties of the node are automatically computed using the accumulated 
 // 使用累积的值自动计算节点的属性
	// root motion delta contribution of the animation graph leading into it.
 // 引导到它的动画图的根运动增量贡献。
	Graph
};

// The supported spaces of a corresponding input vector value
// 对应输入向量值的支持空间
UENUM(BlueprintInternalUseOnly)
enum class EWarpingVectorMode : uint8
{
	// Component-space input vector
 // 分量空间输入向量
	ComponentSpaceVector,
	// Actor-space input vector
 // 演员空间输入向量
	ActorSpaceVector,
	// World-space input vector
 // 世界空间输入向量
	WorldSpaceVector,
	// IK Foot Root relative local-space input vector
 // IK 脚根相对局部空间输入向量
	IKFootRootLocalSpaceVector,
};

// Vector values which may be specified in a configured space
// 可以在配置的空间中指定的向量值
USTRUCT(BlueprintType)
struct FWarpingVectorValue
{
	GENERATED_BODY()

	// Space of the corresponding Vector value
 // 对应Vector值的空间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(PinShownByDefault))
	EWarpingVectorMode Mode = EWarpingVectorMode::ComponentSpaceVector;

	// Specifies a vector relative to the space defined by Mode
 // 指定相对于 Mode 定义的空间的向量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(PinShownByDefault))
	FVector Value = FVector::ZeroVector;

	// Retrieves a normalized Component-space direction from the specified DirectionMode and Direction value
 // 从指定的 DirectionMode 和 Direction 值检索标准化的组件空间方向
	ANIMGRAPHRUNTIME_API FVector AsComponentSpaceDirection(const FAnimInstanceProxy* AnimInstanceProxy, const FTransform& IKFootRootTransform) const;
};
