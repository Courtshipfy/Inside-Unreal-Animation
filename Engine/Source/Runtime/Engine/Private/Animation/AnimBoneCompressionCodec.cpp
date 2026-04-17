// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimBoneCompressionCodec.h"
#include "Animation/AnimBoneCompressionSettings.h"
#include "Animation/AnimBoneDecompressionData.h"
#include "Animation/AnimSequence.h"
#include "Misc/MemStack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimBoneCompressionCodec)

UAnimBoneCompressionCodec::UAnimBoneCompressionCodec(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Description(TEXT("None"))
{
}

#if WITH_EDITORONLY_DATA
int64 UAnimBoneCompressionCodec::EstimateCompressionMemoryUsage(const UAnimSequence& AnimSequence) const
{
#if WITH_EDITOR
	// This is a conservative estimate that gives a codec enough space to create two raw copies of the input data.
	// 这是一个保守的估计，为编解码器提供了足够的空间来创建输入数据的两个原始副本。
	return 2 * int64(AnimSequence.GetApproxBoneRawSize());
#else
	return -1;
#endif // WITH_EDITOR
}
#endif // WITH_EDITORONLY_DATA

UAnimBoneCompressionCodec* UAnimBoneCompressionCodec::GetCodec(const FString& DDCHandle)
{
	const FString ThisHandle = GetCodecDDCHandle();
	return ThisHandle == DDCHandle ? this : nullptr;
}

FString UAnimBoneCompressionCodec::GetCodecDDCHandle() const
{
	// In the DDC, we store a handle to this codec. It must be unique within the parent settings asset
	// 在 DDC 中，我们存储该编解码器的句柄。它在父设置资产中必须是唯一的
	// and all children/sibling codecs. Imagine we have a settings asset with codec A and B.
	// 以及所有子/兄弟编解码器。假设我们有一个带有编解码器 A 和 B 的设置资产。
	// A sequence is compressed with it and selects codec B.
	// 用它压缩序列并选择编解码器 B。
	// The settings asset is duplicated. It will have the same DDC key and the data will not re-compress.
	// 设置资源重复。它将具有相同的 DDC 密钥，并且数据不会重新压缩。
	// When we attempt to load from the DDC, we will have a handle created with the original settings
	// 当我们尝试从 DDC 加载时，我们将使用原始设置创建一个句柄
	// asset pointing to codec B. We must be able to find this codec B in the duplicated asset as well.
	// 指向编解码器 B 的资产。我们也必须能够在重复的资产中找到该编解码器 B。

	FString Handle;
	Handle.Reserve(128);

	GetFName().AppendString(Handle);

	const UObject* Obj = GetOuter();
	while (Obj != nullptr && !Obj->IsA<UAnimBoneCompressionSettings>())
	{
		Handle += TEXT(".");

		Obj->GetFName().AppendString(Handle);

		Obj = Obj->GetOuter();
	}

	return Handle;
}

#if WITH_EDITORONLY_DATA
void UAnimBoneCompressionCodec::PopulateDDCKey(const UE::Anim::Compression::FAnimDDCKeyArgs& KeyArgs, FArchive& Ar)
{
}

void UAnimBoneCompressionCodec::PopulateDDCKey(const UAnimSequenceBase& AnimSeq, FArchive& Ar)
{
}

void UAnimBoneCompressionCodec::PopulateDDCKey(FArchive& Ar)
{
}
#endif


// Default implementation that codecs should override and implement in a more performant way
// 编解码器应以更高效的方式覆盖和实现的默认实现
// It performs a conversion between SoA and AoS, making copies of the data between the formats
// 它执行 SoA 和 AoS 之间的转换，在格式之间复制数据
void UAnimBoneCompressionCodec::DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const UE::Anim::FAnimPoseDecompressionData& DecompressionData) const
{
	const int32 NumTransforms = DecompressionData.GetOutAtomRotations().Num();

	FMemMark Mark(FMemStack::Get());

	TArray<FTransform, TMemStackAllocator<>> OutAtoms;
	OutAtoms.SetNum(NumTransforms);

	// Right now we only support same size arrays
	// 目前我们只支持相同大小的数组
	check(DecompressionData.GetOutAtomRotations().Num() == DecompressionData.GetOutAtomTranslations().Num() && DecompressionData.GetOutAtomRotations().Num() == DecompressionData.GetOutAtomScales3D().Num());

	// Copy atoms data into the FTransform array
	// 将原子数据复制到 FTransform 数组中
	ITERATE_NON_OVERLAPPING_ARRAYS_START(FQuat, DecompressionData.GetOutAtomRotations(), FTransform, OutAtoms, NumTransforms)
		ItSecond->SetRotation(*ItFirst);
	ITERATE_NON_OVERLAPPING_ARRAYS_END()

	ITERATE_NON_OVERLAPPING_ARRAYS_START(FVector, DecompressionData.GetOutAtomTranslations(), FTransform, OutAtoms, NumTransforms)
		ItSecond->SetTranslation(*ItFirst);
	ITERATE_NON_OVERLAPPING_ARRAYS_END()

	ITERATE_NON_OVERLAPPING_ARRAYS_START(FVector, DecompressionData.GetOutAtomScales3D(), FTransform, OutAtoms, NumTransforms)
		ItSecond->SetScale3D(*ItFirst);
	ITERATE_NON_OVERLAPPING_ARRAYS_END()

	// Decompress using default FTransform function
	// 使用默认的 FTransform 函数解压缩
	TArrayView<FTransform> OutAtomsView = OutAtoms;
	DecompressPose(DecompContext, DecompressionData.GetRotationPairs(), DecompressionData.GetTranslationPairs(), DecompressionData.GetScalePairs(), OutAtomsView);

	// Copy back the result
	// 将结果复制回
	ITERATE_NON_OVERLAPPING_ARRAYS_START(FTransform, OutAtoms, FQuat, DecompressionData.GetOutAtomRotations(), NumTransforms)
		*ItSecond = ItFirst->GetRotation();
	ITERATE_NON_OVERLAPPING_ARRAYS_END()

	ITERATE_NON_OVERLAPPING_ARRAYS_START(FTransform, OutAtoms, FVector, DecompressionData.GetOutAtomTranslations(), NumTransforms)
		*ItSecond = ItFirst->GetTranslation();
	ITERATE_NON_OVERLAPPING_ARRAYS_END()

	ITERATE_NON_OVERLAPPING_ARRAYS_START(FTransform, OutAtoms, FVector, DecompressionData.GetOutAtomScales3D(), NumTransforms)
		*ItSecond = ItFirst->GetScale3D();
	ITERATE_NON_OVERLAPPING_ARRAYS_END()
}
