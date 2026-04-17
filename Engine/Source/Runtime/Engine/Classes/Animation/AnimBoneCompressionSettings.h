// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimBoneCompressionSettings.generated.h"

class UAnimBoneCompressionCodec;
class UAnimSequenceBase;

#if WITH_EDITORONLY_DATA
struct FCompressibleAnimData;
struct FCompressibleAnimDataResult;
namespace UE::Anim::Compression { struct FAnimDDCKeyArgs; }
#endif // WITH_EDITORONLY_DATA

/*
 * This object is used to wrap a bone compression codec. It allows a clean integration in the editor by avoiding the need
 * to create asset types and factory wrappers for every codec.
 */
UCLASS(hidecategories = Object, MinimalAPI)
class UAnimBoneCompressionSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	/** A list of animation bone compression codecs to try. Empty entries are ignored but the array cannot be empty. */
	/** 要尝试的动画骨骼压缩编解码器列表。空条目将被忽略，但数组不能为空。 */
	UPROPERTY(Category = Compression, Instanced, EditAnywhere, meta = (NoElementDuplicate))
	TArray<TObjectPtr<UAnimBoneCompressionCodec>> Codecs;

#if WITH_EDITORONLY_DATA
	/** When compressing, the best codec below this error threshold will be used. */
	/** 压缩时，将使用低于此错误阈值的最佳编解码器。 */
	UPROPERTY(Category = Compression, EditAnywhere, meta = (ClampMin = "0"))
	float ErrorThreshold;

	/** Any codec (even one that increases the size) with a lower error will be used until it falls below the threshold. */
	/** 将使用任何具有较低错误的编解码器（即使是增加大小的编解码器），直到其低于阈值。 */
	UPROPERTY(Category = Compression, EditAnywhere)
	bool bForceBelowThreshold;
#endif

	/** Allow us to convert DDC serialized path back into codec object */
	/** 允许我们将 DDC 序列化路径转换回编解码器对象 */
	ENGINE_API UAnimBoneCompressionCodec* GetCodec(const FString& DDCHandle);

	//////////////////////////////////////////////////////////////////////////

#if WITH_EDITORONLY_DATA
	// UObject overrides
	// UObject 覆盖
	ENGINE_API virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;

	/** Returns whether or not we can use these settings to compress. */
	/** 返回我们是否可以使用这些设置进行压缩。 */
	ENGINE_API bool AreSettingsValid() const;

	/** Returns whether or not a codec within is high fidelity. @see UAnimBoneCompressionCodec::IsHighFidelity */
	/** 返回内部编解码器是否为高保真度。 @see UAnimBoneCompressionCodec::IsHighFidelity */
	ENGINE_API bool IsHighFidelity(const FCompressibleAnimData& CompressibleAnimData) const;

	/*
	 * Compresses the animation bones inside the supplied sequence.
	 * The resultant compressed data is applied to the OutCompressedData structure.
	 */
	ENGINE_API bool Compress(const FCompressibleAnimData& AnimSeq, FCompressibleAnimDataResult& OutCompressedData) const;

	/** Generates a DDC key that takes into account the current settings, selected codec, input anim sequence and TargetPlatform */
	/** 生成考虑当前设置、所选编解码器、输入动画序列和 TargetPlatform 的 DDC 密钥 */
	ENGINE_API void PopulateDDCKey(const UE::Anim::Compression::FAnimDDCKeyArgs& KeyArgs, FArchive& Ar);

	/** Generates a DDC key that takes into account the current settings, selected codec, and input anim sequence. */
	/** 生成考虑当前设置、所选编解码器和输入动画序列的 DDC 密钥。 */
	UE_DEPRECATED(5.2, "This function has been deprecated. Override the one above instead.")
	ENGINE_API void PopulateDDCKey(const UAnimSequenceBase& AnimSeq, FArchive& Ar);

	/** Generates a DDC key that takes into account the current settings and selected codec. */
	/** 生成考虑当前设置和所选编解码器的 DDC 密钥。 */
	UE_DEPRECATED(5.1, "This function has been deprecated. Override the one above instead.")
	ENGINE_API void PopulateDDCKey(FArchive& Ar);
#endif
};
