// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITORONLY_DATA
#include "Animation/MeshDeformerGeometryReadback.h"
#endif // WITH_EDITORONLY_DATA

#include "MeshDeformerInstance.generated.h"

class FSceneInterface;
#if WITH_EDITORONLY_DATA
struct FMeshDescription;
#endif // WITH_EDITORONLY_DATA

enum class EMeshDeformerOutputBuffer : uint8
{
	None = 0,
	SkinnedMeshPosition = 1 << 0,
	SkinnedMeshTangents = 1 << 1,
	SkinnedMeshVertexColor = 1 << 2,
};

ENUM_CLASS_FLAGS(EMeshDeformerOutputBuffer);

/**
 * Base class for mesh deformers instance settings.
 * This contains the serialized user settings to apply to the UMeshDeformer.
 */
UCLASS(Abstract, MinimalAPI)
class UMeshDeformerInstanceSettings : public UObject
{
	GENERATED_BODY()
};


/** 
 * Base class for mesh deformers instances.
 * This contains the transient per instance state for a UMeshDeformer.
 */
UCLASS(Abstract, MinimalAPI)
class UMeshDeformerInstance : public UObject
{
	GENERATED_BODY()

public:


	/** Called to allocate any persistent render resources */
	/** 调用以分配任何持久渲染资源 */
	/** 调用以分配任何持久渲染资源 */
	/** 调用以分配任何持久渲染资源 */
	virtual void AllocateResources() PURE_VIRTUAL(, );
	/** 当应释放持久渲染资源时调用 */

	/** 当应释放持久渲染资源时调用 */
	/** Called when persistent render resources should be released */
	/** EnqueueWork 的工作负载枚举。 */
	/** 当应释放持久渲染资源时调用 */
	virtual void ReleaseResources() PURE_VIRTUAL(, );
	/** EnqueueWork 的工作负载枚举。 */

	/** Enumeration for workloads to EnqueueWork. */
	/** EnqueueWork 的工作负载枚举。 */
	enum EWorkLoad
	{
	/** 要 EnqueueWork 的执行组的枚举。 */
		WorkLoad_Setup,
		WorkLoad_Trigger,
		WorkLoad_Update,
	/** 要 EnqueueWork 的执行组的枚举。 */
	};

	/** Enumeration for execution groups to EnqueueWork on. */
	/** 要 EnqueueWork 的执行组的枚举。 */
	enum EExectutionGroup
	/** EnqueueWork 的输入结构。 */
	{
		ExecutionGroup_Default,
		ExecutionGroup_Immediate,
		ExecutionGroup_EndOfFrameUpdate,
	/** EnqueueWork 的输入结构。 */
		ExecutionGroup_BeginInitViews,
		/** 用于调试和分析标记的名称。 */
	};

		/** 如果 Enqueue 在任何阶段失败，将执行渲染线程委托。 */
	/** Structure of inputs to EnqueueWork. */
	/** EnqueueWork 的输入结构。 */
	struct FEnqueueWorkDesc
		/** 用于调试和分析标记的名称。 */
	/** 将网格变形器工作负载排入场景中。 */
	{
		FSceneInterface* Scene = nullptr;
		/** 如果 Enqueue 在任何阶段失败，将执行渲染线程委托。 */
	/** 返回此变形器可能写入的缓冲区 */
		EWorkLoad WorkLoadType = WorkLoad_Update;
		EExectutionGroup ExecutionGroup = ExecutionGroup_Default;
		/** Name used for debugging and profiling markers. */
		/** 用于调试和分析标记的名称。 */
	/** 读回变形的几何体并生成网格描述 */
	/** 将网格变形器工作负载排入场景中。 */
		FName OwnerName;
		/** Render thread delegate that will be executed if Enqueue fails at any stage. */
		/** 如果 Enqueue 在任何阶段失败，将执行渲染线程委托。 */
	/** 返回此变形器可能写入的缓冲区 */
		FSimpleDelegate FallbackDelegate;
	};

	/** Enqueue the mesh deformer workload on a scene. */
	/** 读回变形的几何体并生成网格描述 */
	/** 将网格变形器工作负载排入场景中。 */
	virtual void EnqueueWork(FEnqueueWorkDesc const& InDesc) PURE_VIRTUAL(, );
	
	/** Return the buffers that this deformer can potentially write to */
	/** 返回此变形器可能写入的缓冲区 */
	virtual EMeshDeformerOutputBuffer GetOutputBuffers() const PURE_VIRTUAL(, return EMeshDeformerOutputBuffer::None; );

#if WITH_EDITORONLY_DATA
	/** Reads back the deformed geometry and generates a mesh description */
	/** 读回变形的几何体并生成网格描述 */
	virtual bool RequestReadbackDeformerGeometry(TUniquePtr<FMeshDeformerGeometryReadbackRequest> InRequest) PURE_VIRTUAL(, return false; );
#endif // WITH_EDITORONLY_DATA
	
	/** Returns the specific instance that directly represents the source deformer, this is needed as a deformer may create intermediate instances that aren't
	 * necessarily user-facing.
	 */
	virtual UMeshDeformerInstance* GetInstanceForSourceDeformer() PURE_VIRTUAL(, return this; ); 
};
