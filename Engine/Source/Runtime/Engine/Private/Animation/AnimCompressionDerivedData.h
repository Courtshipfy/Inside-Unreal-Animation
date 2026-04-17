// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

#if WITH_EDITOR
#include "DerivedDataPluginInterface.h"
#endif

#include "Animation/AnimCompressionTypes.h"

struct FAnimCompressContext;
struct FDerivedDataUsageStats;

#if WITH_EDITOR

//////////////////////////////////////////////////////////////////////////
// FDerivedDataAnimationCompression
// F派生数据动画压缩
class FDerivedDataAnimationCompression : public FDerivedDataPluginInterface
{
private:
	// The anim data to compress
	// 要压缩的动画数据
	FCompressibleAnimPtr DataToCompressPtr;

	// The Type of anim data to compress (makes up part of DDC key)
	// 要压缩的动画数据的类型（构成 DDC 密钥的一部分）
	const TCHAR* TypeName;

	// Bulk of asset DDC key
	// 批量资产DDC密钥
	const FString AssetDDCKey;

	// FAnimCompressContext to use during compression if we don't pull from the DDC
	// 如果我们不从 DDC 中提取数据，则在压缩期间使用 FAnimCompressContext
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TSharedPtr<FAnimCompressContext> CompressContext;
public:
	FDerivedDataAnimationCompression(const TCHAR* InTypeName, const FString& InAssetDDCKey, TSharedPtr<FAnimCompressContext> InCompressContext);	
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	virtual ~FDerivedDataAnimationCompression();

	void SetCompressibleData(FCompressibleAnimRef InCompressibleAnimData)
	{
		DataToCompressPtr = InCompressibleAnimData;
	}

	FCompressibleAnimPtr GetCompressibleData() const { return DataToCompressPtr; }

	uint64 GetMemoryUsage() const
	{
		return DataToCompressPtr.IsValid() ? DataToCompressPtr->GetApproxMemoryUsage() : 0;
	}

	virtual const TCHAR* GetPluginName() const override
	{
		return TypeName;
	}

	virtual const TCHAR* GetVersionString() const override;

	virtual FString GetPluginSpecificCacheKeySuffix() const override
	{
		return AssetDDCKey;
	}

	virtual FString GetDebugContextString() const override;

	virtual bool IsBuildThreadsafe() const override
	{
		return true;
	}

	virtual bool Build( TArray<uint8>& OutDataArray) override;

	/** Return true if we can build **/
	/** [翻译失败: Return true if we can build] **/
	bool CanBuild()
	{
		return DataToCompressPtr.IsValid();
	}
};

namespace AnimSequenceCookStats
{
	extern FDerivedDataUsageStats UsageStats;
}

#endif	//WITH_EDITOR
