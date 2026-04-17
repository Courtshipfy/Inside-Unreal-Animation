// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Keyframe reduction algorithm that simply removes every second key.
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimCompress.h"
#include "AnimCompress_RemoveEverySecondKey.generated.h"

UCLASS(MinimalAPI)
class UAnimCompress_RemoveEverySecondKey : public UAnimCompress
{
	GENERATED_UCLASS_BODY()

	/** Animations with fewer than MinKeys will not lose any keys. */
	/** 少于 MinKeys 的动画不会丢失任何关键点。 */
	UPROPERTY(EditAnywhere, Category=AnimationCompressionAlgorithm_RemoveEverySecondKey, meta=(UIMin=1, ClampMin=1))
	int32 MinKeys;

	/**
	 * If bStartAtSecondKey is true, remove keys 1,3,5,etc.
	 * If bStartAtSecondKey is false, remove keys 0,2,4,etc.
	 */
	UPROPERTY(EditAnywhere, Category=AnimationCompressionAlgorithm_RemoveEverySecondKey)
	uint32 bStartAtSecondKey:1;


protected:
	//~ Begin UAnimCompress Interface
	//~ 开始 UAnimCompress 界面
#if WITH_EDITOR
	virtual bool DoReduction(const FCompressibleAnimData& CompressibleAnimData, FCompressibleAnimDataResult& OutResult) override;
	virtual void PopulateDDCKey(const UE::Anim::Compression::FAnimDDCKeyArgs& KeyArgs, FArchive& Ar) override;
#endif // WITH_EDITOR
	//~ Begin UAnimCompress Interface
	//[翻译失败: ~ Begin UAnimCompress Interface]
};



