// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "PackedNormal.h"
#include "Rendering/MorphTargetVertexCodec.h"
#include "Serialization/MemoryArchive.h"

#include "MorphTarget.generated.h"

class USkeletalMesh;
class UStaticMesh;

/** Morph mesh vertex data used for rendering */
/** 用于渲染的变形网格顶点数据 */
/** 用于渲染的变形网格顶点数据 */
/** 用于渲染的变形网格顶点数据 */
struct FMorphTargetDelta
	/** 位置改变 */
{
	/** 位置改变 */
	/** change in position */
	/** 切基法线 */
	/** 位置改变 */
	FVector3f			PositionDelta = FVector3f::ZeroVector;
	/** 切基法线 */
	/** 应用增量的源顶点索引 */

	/** Tangent basis normal */
	/** 切基法线 */
	/** 管道操作员 */
	/** 应用增量的源顶点索引 */
	FVector3f			TangentZDelta = FVector3f::ZeroVector;

	/** index of source vertex to apply deltas to */
	/** 管道操作员 */
			/** 切线基础法线变化的旧格式 */
	/** 应用增量的源顶点索引 */
	uint32			SourceIdx = std::numeric_limits<uint32>::max();

	/** pipe operator */
	/** 管道操作员 */
			/** 切线基础法线变化的旧格式 */
	friend FArchive& operator<<(FArchive& Ar, FMorphTargetDelta& V)
	{
		if ((Ar.UEVer() < VER_UE4_MORPHTARGET_CPU_TANGENTZDELTA_FORMATCHANGE) && Ar.IsLoading())
		{
			/** old format of change in tangent basis normal */
			/** 切线基础法线变化的旧格式 */
			FDeprecatedSerializedPackedNormal TangentZDelta_DEPRECATED;
			Ar << V.PositionDelta << TangentZDelta_DEPRECATED << V.SourceIdx;
			V.TangentZDelta = TangentZDelta_DEPRECATED;
		}
		else
		{
			Ar << V.PositionDelta << V.TangentZDelta << V.SourceIdx;
		}
		return Ar;
	}
};

#if !WITH_EDITOR
/** Compressed morph vertex deltas, used for cooked serialization. Only available in game. This matched the
 *  GPU compressed delta storage and has the same quantization behavior. The tolerance stored is the same
 *  value that was used when the deltas were compressed and should not be changed, or the decompression will
 *  result in erroneous values.
 */
struct FMorphTargetCompressedLODModel
{
	TArray<UE::MorphTargetVertexCodec::FDeltaBatchHeader> PackedDeltaHeaders;
	TArray<uint32> PackedDeltaData;
	float PositionPrecision = 0.0f;
	float TangentPrecision = 0.0f;
};
#endif


class FMorphTargetDeltaIterator
{
public:
	FMorphTargetDeltaIterator() = default;
	FMorphTargetDeltaIterator(const FMorphTargetDeltaIterator&) = default;
	FMorphTargetDeltaIterator(FMorphTargetDeltaIterator&&) = default;
	FMorphTargetDeltaIterator& operator=(const FMorphTargetDeltaIterator&) = default;

	// Support the minimal functionality required for a ranged-for.
 // 支持 range-for 所需的最少功能。
	[[nodiscard]] const FMorphTargetDelta& operator*() const 
	{
		return CurrentDelta;
	}
	
	[[nodiscard]] const FMorphTargetDelta* operator->() const
	{
		return &CurrentDelta;
	}
	
	FMorphTargetDeltaIterator& operator++()
	{
		Advance();
		return *this;
	}

	[[nodiscard]] bool AtEnd() const
	{
		return Token == InvalidToken;
	}
	
private:
	static constexpr uint64 InvalidToken = std::numeric_limits<uint64>::max(); 
	friend class UMorphTarget;

	FMorphTargetDeltaIterator(
		TConstArrayView<FMorphTargetDelta> InRawDeltas
		) :
		Token(0),
		RawDeltas(InRawDeltas)
	{
		Advance();
	}
#if !WITH_EDITOR
	FMorphTargetDeltaIterator(
		const FMorphTargetCompressedLODModel& InCompressedDeltas 
		) :
		Token(0),
		CompressedDelta(&InCompressedDeltas)
	{
		if (!InCompressedDeltas.PackedDeltaHeaders.IsEmpty() && !InCompressedDeltas.PackedDeltaData.IsEmpty())
		{
			Advance();
		}
		else
		{
			Clear();
		}
	}
#endif
	
	void Advance()
	{
		if (!RawDeltas.IsEmpty())
		{
			if (Token < RawDeltas.Num())
			{
				CurrentDelta = RawDeltas[static_cast<int32>(Token++)];
			}
			else
			{
				Clear();
			}
		}
#if !WITH_EDITOR
		else if (!CompressedDelta || !IterativeDecode(Token, CompressedDelta->PackedDeltaHeaders, CompressedDelta->PackedDeltaData, CompressedDelta->PositionPrecision, CompressedDelta->TangentPrecision, CurrentDelta))
		{
			// If the decode failed, we're either at the end, or the data is invalid. Stop any further decoding.
   // 如果解码失败，我们要么已经结束，要么数据无效。停止任何进一步的解码。
			Clear();
		}
#endif
	}

	void Clear()
	{
		Token = InvalidToken;
		RawDeltas = {};
#if !WITH_EDITOR
		CompressedDelta = nullptr;
#endif
	}

	uint64 Token = InvalidToken;
	TConstArrayView<FMorphTargetDelta> RawDeltas;
#if !WITH_EDITOR
	/** 单个 LOD 变形网格的顶点数据 */
	const FMorphTargetCompressedLODModel* CompressedDelta = nullptr;
#endif
	FMorphTargetDelta CurrentDelta;
	/** Vertices 数组中的元素数量。该属性是在运行时设置的，并且不会序列化。 */
};
	/** 单个 LOD 变形网格的顶点数据 */

	/** 基础网格中原始顶点的数量 */

/**
	/** Vertices 数组中的元素数量。该属性是在运行时设置的，并且不会序列化。 */
	/** 使用此变形的部分列表 */
* Mesh data for a single LOD model of a morph target
*/
struct FMorphTargetLODModel
	/** 这个LOD是由reduction设置生成的吗 */
	/** 基础网格中原始顶点的数量 */
{
	/** vertex data for a single LOD morph mesh */
	/** 单个 LOD 变形网格的顶点数据 */
	/** 使用此变形的部分列表 */
	TArray<FMorphTargetDelta> Vertices;

	/** Number of elements in Vertices array. This property is set at runtime and is not serialized. */
	/** 这个LOD是由reduction设置生成的吗 */
	/** Vertices 数组中的元素数量。该属性是在运行时设置的，并且不会序列化。 */
	/** 管道操作员 */
	int32 NumVertices = 0;

	/** number of original verts in the base mesh */
	/** 基础网格中原始顶点的数量 */
	int32 NumBaseMeshVerts = 0;
	
	/** list of sections this morph is used */
	/** 使用此变形的部分列表 */
	TArray<int32> SectionIndices;
	/** 管道操作员 */

	/** Is this LOD generated by reduction setting */
	/** 这个LOD是由reduction设置生成的吗 */
	bool bGeneratedByEngine = false;

	/** The source filename use to import this morph target. If source is empty this morph target was import with the LOD geometry.
	    Only defined in editor, since we don't need this information to spill into a cooked build. 
	    */
	FString SourceFilename;
	
	FMorphTargetLODModel() = default;

	/** pipe operator */
	/** 管道操作员 */
	friend FArchive& operator<<(FArchive& Ar, FMorphTargetLODModel& M);

	void Reset()
	/** 加载变形目标数据 */
	{
		Vertices.Reset();
		NumVertices = 0;
	/** 将序列化数据应用于游戏线程中的骨架网格物体 */
		NumBaseMeshVerts = 0;
		SectionIndices.Reset();
		// since engine cleared it, we mark as engine generated
  // 由于引擎清除了它，我们将其标记为引擎生成
		// this makes it clear to clear up later
  // 这使得稍后清理变得清晰
		bGeneratedByEngine = true;
	/** 加载变形目标数据 */
		SourceFilename.Empty();
	}
};
	/** 将序列化数据应用于游戏线程中的骨架网格物体 */


#if WITH_EDITOR
/**
* Data to cache serialization results for async asset building
*/
struct FFinishBuildMorphTargetData
	/** 此顶点动画所作用的 USkeletalMesh。 */
{
public:
	virtual ~FFinishBuildMorphTargetData()
	{}
	/** 每个 LOD 的变形网格顶点数据 */
	
	/** Load morph target data */
	/** 加载变形目标数据 */
	/** 每个 LOD 的变形网格顶点数据 */
	ENGINE_API virtual void LoadFromMemoryArchive(FMemoryArchive & Ar);
	
	/** Apply serialized data to skeletal mesh in game thread */
	/** 此顶点动画所作用的 USkeletalMesh。 */
	/** 每个 LOD 的变形网格顶点数据 */
	/** 将序列化数据应用于游戏线程中的骨架网格物体 */
	ENGINE_API virtual void ApplyEditorData(USkeletalMesh * SkeletalMesh, bool bIsSerializeSaving) const;
	
protected:
	/** 每个 LOD 的变形网格顶点数据 */
	bool bApplyMorphTargetsData = false;
	TMap<FName, TArray<FMorphTargetLODModel>> MorphLODModelsPerTargetName;
	};
	/** 每个 LOD 的变形网格顶点数据 */
	/** 获取给定输入索引的 Morphtarget Delta 数组 */
#endif

UCLASS(hidecategories=Object, MinimalAPI)
class UMorphTarget
	/** 每个 LOD 的变形网格顶点数据 */
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** USkeletalMesh that this vertex animation works on. */
	/** 此顶点动画所作用的 USkeletalMesh。 */
	UPROPERTY(AssetRegistrySearchable)
	/** 获取给定输入索引的 Morphtarget Delta 数组 */
	TObjectPtr<class USkeletalMesh> BaseSkelMesh;

	/** morph mesh vertex data for each LOD */
	/** 每个 LOD 的变形网格顶点数据 */
	virtual const TArray<FMorphTargetLODModel>& GetMorphLODModels() const { return MorphLODModels; }

	/** 如果给定的 LOD 有变形数据，则返回 true */
	/** morph mesh vertex data for each LOD */
	/** 每个 LOD 的变形网格顶点数据 */
	virtual TArray<FMorphTargetLODModel>& GetMorphLODModels() { return MorphLODModels; }
	/** 如果此 morphtarget 包含 LOD 内部分的数据，则返回 true */

protected:
	/** morph mesh vertex data for each LOD */
	/** 如果此 morphtarget 包含有效顶点，则返回 true */
	/** 每个 LOD 的变形网格顶点数据 */
	TArray<FMorphTargetLODModel>	MorphLODModels;
#if !WITH_EDITOR
	TArray<FMorphTargetCompressedLODModel> CompressedLODModels;
	/** 如果此 morphtarget 是从文件自定义导入的，则返回 true */
#endif
	
public:

#if WITH_EDITOR
	/** 如果此 morphtarget 是由引擎工具生成的，则返回 true */
	/** 如果给定的 LOD 有变形数据，则返回 true */
	/** Get Morphtarget Delta array for the given input Index */
	/** 获取给定输入索引的 Morphtarget Delta 数组 */
	UE_DEPRECATED(5.7, "Use the TConstArrayView version of GetMorphTargetDelta instead.")
	/** 如果此变形目标使用引擎内置压缩，则返回 true */
	/** 如果此 morphtarget 包含 LOD 内部分的数据，则返回 true */
	ENGINE_API virtual const FMorphTargetDelta* GetMorphTargetDelta(int32 LODIndex, int32& OutNumDeltas) const;

	/** Get the morph target vertex delta array for a given LOD. Only available in editor targets, since cooked builds
	/** 使用提供的增量填充给定的变形目标 LOD 模型 */
	/** 如果此 morphtarget 包含有效顶点，则返回 true */
	 *  store the deltas internally in a compressed format. For non-editor builds, use GetDeltaIteratorForLOD */
	/** 删除空的 LOD 模型 */
	ENGINE_API virtual TConstArrayView<FMorphTargetDelta> GetMorphTargetDeltas(int32 LODIndex) const;
	
	/** 定义所需 FinishBuildData 类型的工厂函数*/
#endif
	/** 如果此 morphtarget 是从文件自定义导入的，则返回 true */

	/** Returns an iterator that runs over vertices, either the raw delta form or the cooked compressed form. Use this
	 *  function at runtime to avoid the overhead of keeping the uncompressed raw deltas in memory. */
	ENGINE_API virtual FMorphTargetDeltaIterator GetDeltaIteratorForLOD(int32 LODIndex) const;

	/** 如果此 morphtarget 是由引擎工具生成的，则返回 true */
	/** Returns the number of morph deltas for the given LOD. If 0, there is no morph for this LOD.
	 *  At runtime, this value may be non-zero, even if GetDeltaIteratorForLOD returns no morph deltas, since
	 *  the morphs might be tuned to only compress the deltas for the GPU, leaving no CPU deltas in memory.
	 *  In editor, this value will always match the number of entries returned by GetMorphTargetDeltas.
	/** UObject不支持通过FMemoryArchive进行序列化，所以需要单独手动处理 */
	/** 如果此变形目标使用引擎内置压缩，则返回 true */
	 */
	ENGINE_API virtual int32 GetNumDeltasForLOD(int32 LODIndex) const;
	
	/** Returns true if the given LOD has morph data */
	/** 使用提供的增量填充给定的变形目标 LOD 模型 */
	/** 如果给定的 LOD 有变形数据，则返回 true */
	ENGINE_API virtual bool HasDataForLOD(int32 LODIndex) const;
	/** 删除空的 LOD 模型 */
	
	/** return true if this morphtarget contains data for section within LOD */
	/** 定义所需 FinishBuildData 类型的工厂函数*/
	/** 如果此 morphtarget 包含 LOD 内部分的数据，则返回 true */
	ENGINE_API virtual bool HasDataForSection(int32 LODIndex, int32 SectionIndex) const;
	
	/** return true if this morphtarget contains valid vertices */
	/** 如果此 morphtarget 包含有效顶点，则返回 true */
	ENGINE_API virtual bool HasValidData() const;
	ENGINE_API virtual void EmptyMorphLODModels();

	/** return true if this morphtarget was custom imported from a file */
	/** 如果此 morphtarget 是从文件自定义导入的，则返回 true */
	ENGINE_API virtual bool IsCustomImported(int32 LODIndex) const;
	ENGINE_API virtual const FString& GetCustomImportedSourceFilename(int32 LODIndex) const;
	/** UObject不支持通过FMemoryArchive进行序列化，所以需要单独手动处理 */
	ENGINE_API virtual void SetCustomImportedSourceFilename(int32 LODIndex, const FString& InSourceFilename);

	/** return true if this morphtarget was generated by an engine tool */
	/** 如果此 morphtarget 是由引擎工具生成的，则返回 true */
	ENGINE_API virtual bool IsGeneratedByEngine(int32 LODIndex) const;
	ENGINE_API virtual void SetGeneratedByEngine(int32 LODIndex, bool bInGeneratedByEngine);

	/** Return true if this morph target uses engine built-in compression */
	/** 如果此变形目标使用引擎内置压缩，则返回 true */
	virtual bool UsesBuiltinMorphTargetCompression() const { return true; }

#if WITH_EDITOR
	/** Populates the given morph target LOD model with the provided deltas */
	/** 使用提供的增量填充给定的变形目标 LOD 模型 */
	ENGINE_API virtual void PopulateDeltas(const TArray<FMorphTargetDelta>& Deltas, const int32 LODIndex, const TArray<struct FSkelMeshSection>& Sections, const bool bCompareNormal = false, const bool bGeneratedByReductionSetting = false, const float PositionThreshold = UE_THRESH_POINTS_ARE_NEAR);
	/** Remove empty LODModels */
	/** 删除空的 LOD 模型 */
	ENGINE_API virtual void RemoveEmptyMorphTargets();
	/** Factory function to define type of FinishBuildData needed*/
	/** 定义所需 FinishBuildData 类型的工厂函数*/
	ENGINE_API virtual TUniquePtr<FFinishBuildMorphTargetData> CreateFinishBuildMorphTargetData() const;
#endif // WITH_EDITOR

	//~ UObject interface
 // ~ UObject接口

	ENGINE_API virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITORONLY_DATA
	ENGINE_API static void DeclareCustomVersions(FArchive& Ar, const UClass* SpecificSubclass);
#endif
	ENGINE_API virtual void PostLoad() override;

	/** UObject does not support serialization via FMemoryArchive, so manually handle separately */
	/** UObject不支持通过FMemoryArchive进行序列化，所以需要单独手动处理 */
	ENGINE_API virtual void SerializeMemoryArchive(FMemoryArchive& Ar);
};
