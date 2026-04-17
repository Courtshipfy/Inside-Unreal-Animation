// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimCurveCompressionCodec.h"
#include "Animation/AnimSequence.h"
#include "UObject/FortniteMainBranchObjectVersion.h"
#include "UObject/FortniteReleaseBranchCustomObjectVersion.h"
#include "UObject/Package.h"
#include "Interfaces/ITargetPlatform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimCurveCompressionCodec)

UAnimCurveCompressionCodec::UAnimCurveCompressionCodec(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAnimCurveCompressionCodec::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FFortniteMainBranchObjectVersion::GUID);
	Ar.UsingCustomVersion(FFortniteReleaseBranchCustomObjectVersion::GUID);

	if (Ar.CustomVer(FFortniteMainBranchObjectVersion::GUID) < FFortniteMainBranchObjectVersion::RemoveAnimCurveCompressionCodecInstanceGuid)
	{
#if !WITH_EDITOR
		if (Ar.CustomVer(FFortniteReleaseBranchCustomObjectVersion::GUID) >= FFortniteReleaseBranchCustomObjectVersion::SerializeAnimCurveCompressionCodecGuidOnCook)
#endif // !WITH_EDITOR
		{
			// We serialized a Guid, read it now and discard it
			// 我们序列化了一个 Guid，现在读取它并丢弃它
			check(Ar.IsLoading());
			FGuid InstanceGuid;
			Ar << InstanceGuid;
		}
	}
}

#if WITH_EDITORONLY_DATA
int64 UAnimCurveCompressionCodec::EstimateCompressionMemoryUsage(const UAnimSequence& AnimSequence) const
{
#if WITH_EDITOR
	// This is a conservative estimate that gives a codec enough space to create two raw copies of the input data.
	// 这是一个保守的估计，为编解码器提供了足够的空间来创建输入数据的两个原始副本。
	return 2 * int64(AnimSequence.GetApproxCurveRawSize());
#else
	return -1;
#endif // WITH_EDITOR
}

void UAnimCurveCompressionCodec::PopulateDDCKey(FArchive& Ar)
{
	// We use the UClass name to compute the DDC key to avoid two codecs with equivalent properties (e.g. none)
	// 我们使用 UClass 名称来计算 DDC 密钥，以避免两个编解码器具有相同的属性（例如没有）
	// from having the same DDC key. Two codecs with the same values and class name can have the same DDC key
	// 具有相同的 DDC 密钥。具有相同值和类名的两个编解码器可以具有相同的 DDC 密钥
	// since the caller (e.g. anim sequence) factors in raw data and the likes. For a codec class that derives
	// 因为调用者（例如动画序列）会影响原始数据等。对于派生的编解码器类
	// from this, it is their responsibility to factor in compression settings and other inputs into the DDC key.
	// 由此看来，他们有责任将压缩设置和其他输入考虑到 DDC 密钥中。
	FString ClassName = GetClass()->GetName();
	Ar << ClassName;
}
#endif
