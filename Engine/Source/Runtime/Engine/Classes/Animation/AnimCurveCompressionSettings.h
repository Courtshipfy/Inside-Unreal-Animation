// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimCompressionTypes.h"
#include "AnimCurveCompressionSettings.generated.h"

class UAnimCurveCompressionCodec;
class UAnimSequence;

/*
 * This object is used to wrap a curve compression codec. It allows a clean integration in the editor by avoiding the need
 * to create asset types and factory wrappers for every codec.
 */
UCLASS(hidecategories = Object, MinimalAPI)
class UAnimCurveCompressionSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	/** An animation curve compression codec. */
	/** 动画曲线压缩编解码器。 */
	/** 动画曲线压缩编解码器。 */
	/** [翻译失败: An animation curve compression codec.] */
	UPROPERTY(Category = Compression, Export, EditAnywhere, NoClear, meta = (EditInline))
	TObjectPtr<UAnimCurveCompressionCodec> Codec;

	//////////////////////////////////////////////////////////////////////////
	/** 允许我们将 DDC 序列化路径转换回编解码器对象 */

	/** 允许我们将 DDC 序列化路径转换回编解码器对象 */
	/** Allow us to convert DDC serialized path back into codec object */
	/** [翻译失败: Allow us to convert DDC serialized path back into codec object] */
	ENGINE_API UAnimCurveCompressionCodec* GetCodec(const FString& Path);

#if WITH_EDITORONLY_DATA
	/** 返回我们是否可以使用这些设置进行压缩。 */
	// UObject overrides
 // UObject 覆盖
	/** 返回我们是否可以使用这些设置进行压缩。 */
	ENGINE_API virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;

	/** Returns whether or not we can use these settings to compress. */
	/** [翻译失败: Returns whether or not we can use these settings to compress.] */
	ENGINE_API bool AreSettingsValid() const;

	/*
	/** 生成考虑当前设置和所选编解码器的 DDC 密钥。 */
	 * Compresses the animation curves inside the supplied sequence data.
	 * The resultant compressed data is applied to the OutCompressedData structure.
	/** 生成考虑当前设置和所选编解码器的 DDC 密钥。 */
	 */
	ENGINE_API bool Compress(const FCompressibleAnimData& AnimSeq, FCompressedAnimSequence& OutCompressedData) const;

	/** Generates a DDC key that takes into account the current settings and selected codec. */
	/** [翻译失败: Generates a DDC key that takes into account the current settings and selected codec.] */
	ENGINE_API void PopulateDDCKey(FArchive& Ar);
#endif
};
