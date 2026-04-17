// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* Stores the raw rich curves as FCompressedRichCurve internally with optional key reduction and key time quantization.
*/

#include "CoreMinimal.h"
#include "Animation/AnimCurveCompressionCodec.h"
#include "AnimCurveCompressionCodec_CompressedRichCurve.generated.h"

UCLASS(meta = (DisplayName = "Compressed Rich Curves"), MinimalAPI)
class UAnimCurveCompressionCodec_CompressedRichCurve : public UAnimCurveCompressionCodec
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	/** Max error allowed when compressing the rich curves */
	/** 压缩丰富曲线时允许的最大误差 */
	/** 压缩丰富曲线时允许的最大误差 */
	/** 压缩丰富曲线时允许的最大误差 */
	UPROPERTY(Category = Compression, EditAnywhere, meta = (ClampMin = "0"))
	float MaxCurveError;
	/** 是否使用动画序列采样率或显式值 */

	/** 是否使用动画序列采样率或显式值 */
	/** Whether to use the animation sequence sample rate or an explicit value */
	/** [翻译失败: Whether to use the animation sequence sample rate or an explicit value] */
	/** 测量曲线误差时使用的采样率 */
	UPROPERTY(Category = Compression, EditAnywhere)
	bool UseAnimSequenceSampleRate;
	/** 测量曲线误差时使用的采样率 */

	/** Sample rate to use when measuring the curve error */
	/** 测量曲线误差时使用的采样率 */
	UPROPERTY(Category = Compression, EditAnywhere, meta = (ClampMin = "0", EditCondition = "!UseAnimSequenceSampleRate"))
	float ErrorSampleRate;
#endif

	//////////////////////////////////////////////////////////////////////////

#if WITH_EDITORONLY_DATA
	// UAnimCurveCompressionCodec overrides
 // UAnimCurveCompressionCodec 覆盖
	ENGINE_API virtual bool Compress(const FCompressibleAnimData& AnimSeq, FAnimCurveCompressionResult& OutResult) override;
	ENGINE_API virtual void PopulateDDCKey(FArchive& Ar) override;
#endif
	
	ENGINE_API virtual void DecompressCurves(const FCompressedAnimSequence& AnimSeq, FBlendedCurve& Curves, float CurrentTime) const override;
	ENGINE_API virtual float DecompressCurve(const FCompressedAnimSequence& AnimSeq, FName CurveName, float CurrentTime) const override;
};
