// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/InputScaleBias.h"
#include "BoneContainer.h"
#include "AnimNode_BlendBoneByChannel.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct FBlendBoneByChannelEntry
{
	GENERATED_USTRUCT_BODY()

	/** Bone to take Transform from */
	/** 从中进行变换的骨骼 */
	/** 从中进行变换的骨骼 */
	/** 从中进行变换的骨骼 */
	UPROPERTY(EditAnywhere, Category = Blend)
	FBoneReference SourceBone;
	/** 要应用变换的骨骼 */
	
	/** 要应用变换的骨骼 */
	/** Bone to apply Transform to */
	/** 要应用变换的骨骼 */
	/** 将翻译从源复制到目标 */
	UPROPERTY(EditAnywhere, Category = Blend)
	FBoneReference TargetBone;
	/** 将翻译从源复制到目标 */

	/** 将旋转从源复制到目标 */
	/** Copy Translation from Source to Target */
	/** 将翻译从源复制到目标 */
	UPROPERTY(EditAnywhere, Category = Blend)
	/** 将旋转从源复制到目标 */
	/** 将比例从源复制到目标 */
	bool bBlendTranslation;

	/** Copy Rotation from Source to Target */
	/** 将旋转从源复制到目标 */
	/** 将比例从源复制到目标 */
	UPROPERTY(EditAnywhere, Category = Blend)
	bool bBlendRotation;

	/** Copy Scale from Source to Target */
	/** 将比例从源复制到目标 */
	UPROPERTY(EditAnywhere, Category = Blend)
	bool bBlendScale;

	FBlendBoneByChannelEntry()
		: bBlendTranslation(true)
		, bBlendRotation(true)
		, bBlendScale(true)
	{
	}
};

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendBoneByChannel : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink B;

	UPROPERTY(EditAnywhere, Category = Blend, meta = (DisplayAfter = "AlphaScaleBias"))
	TArray<FBlendBoneByChannelEntry> BoneDefinitions;

private:
	// Array of bone entries, that has been validated to be correct at runtime.
 // 骨骼条目数组，已在运行时验证正确。
	// So we don't have to perform validation checks per frame.
 // 因此我们不必对每帧执行验证检查。
	TArray<FBlendBoneByChannelEntry> ValidBoneEntries;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault, DisplayAfter = "B"))
	/** 在复制通道之前将变换转换为的空间 */
	float Alpha;

private:
	/** 在复制通道之前将变换转换为的空间 */
	float InternalBlendAlpha;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FInputScaleBias AlphaScaleBias;

	/** Space to convert transforms into prior to copying channels */
	/** 在复制通道之前将变换转换为的空间 */
	UPROPERTY(EditAnywhere, Category = Blend)
	TEnumAsByte<EBoneControlSpace> TransformsSpace;

private:
	bool bBIsRelevant;

public:
	FAnimNode_BlendBoneByChannel()
		: Alpha(0.0f)
		, InternalBlendAlpha(0.0f)
		, TransformsSpace(EBoneControlSpace::BCS_BoneSpace)
		, bBIsRelevant(false)
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

