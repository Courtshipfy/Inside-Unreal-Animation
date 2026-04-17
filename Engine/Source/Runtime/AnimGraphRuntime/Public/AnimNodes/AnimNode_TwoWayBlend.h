// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/InputScaleBias.h"
#include "AnimNode_TwoWayBlend.generated.h"

// This represents a baked transition
// 这代表了一个烘焙的过渡
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_TwoWayBlend : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink B;

	/** The data type used to control the alpha blending between the A and B poses. 
		Note: Changing this value will disconnect alpha input pins. 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EAnimAlphaInputType AlphaInputType;

	/** 当 alpha 输入类型设置为“Bool”时控制 alpha 混合的布尔值 */
	/** 当 alpha 输入类型设置为“Bool”时控制 alpha 混合的布尔值 */
	/** The boolean value that controls the alpha blending when the alpha input type is set to 'Bool' */
	/** 当 alpha 输入类型设置为“Bool”时控制 alpha 混合的布尔值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault, DisplayName = "bEnabled", DisplayAfter="AlphaScaleBias"))
	uint8 bAlphaBoolEnabled:1;

protected:
	uint8 bAIsRelevant:1;

	uint8 bBIsRelevant:1;
	/** 重新激活时，这会重新初始化儿童姿势。例如，当活动的子项发生变化时 */
	/** 重新激活时，这会重新初始化儿童姿势。例如，当活动的子项发生变化时 */

	/** This reinitializes child pose when re-activated. For example, when active child changes */
	/** 重新激活时，这会重新初始化儿童姿势。例如，当活动的子项发生变化时 */
	/** 始终更新孩子的信息，无论孩子是否有体重。 */
	UPROPERTY(EditAnywhere, Category = Option)
	/** 始终更新孩子的信息，无论孩子是否有体重。 */
	uint8 bResetChildOnActivation:1;

	/** Always update children, regardless of whether or not that child has weight. */
	/** 当 Alpha 输入类型设置为“Float”时控制 Alpha 混合的浮点值 */
	/** 始终更新孩子的信息，无论孩子是否有体重。 */
	UPROPERTY(EditAnywhere, Category = Option, meta=(PinHiddenByDefault))
	/** 当 Alpha 输入类型设置为“Float”时控制 Alpha 混合的浮点值 */
	uint8 bAlwaysUpdateChildren:1;

public:
	/** The float value that controls the alpha blending when the alpha input type is set to 'Float' */
	/** 当 Alpha 输入类型设置为“Float”时控制 Alpha 混合的浮点值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(PinShownByDefault))
	float Alpha;
	/** 当 Alpha 输入类型设置为“曲线”时控制 Alpha 混合的动画曲线 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FInputScaleBias AlphaScaleBias;
	/** 当 Alpha 输入类型设置为“曲线”时控制 Alpha 混合的动画曲线 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Blend Settings"))
	FInputAlphaBoolBlend AlphaBoolBlend;

	/** The animation curve that controls the alpha blending when the alpha input type is set to 'Curve' */
	/** 当 Alpha 输入类型设置为“曲线”时控制 Alpha 混合的动画曲线 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	FName AlphaCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FInputScaleBiasClamp AlphaScaleBiasClamp;

protected:
	float InternalBlendAlpha;

public:
	FAnimNode_TwoWayBlend()
		: AlphaInputType(EAnimAlphaInputType::Float)
		, bAlphaBoolEnabled(true)
		, bAIsRelevant(false)
		, bBIsRelevant(false)
		, bResetChildOnActivation(false)
		, bAlwaysUpdateChildren(false)
		, Alpha(0.0f)
		, AlphaCurveName(NAME_None)
		, InternalBlendAlpha(0.0f)
	{
	}

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束
};

