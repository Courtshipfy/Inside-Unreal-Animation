// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RenderGraphResources.h"
#include "SkeletalRenderPublic.h"

/**
 * Owner class for Mesh Deformer generated geometry.
 */
class FMeshDeformerGeometry
{
public:
	FMeshDeformerGeometry();
	
	/** Reset the stored data. */
	/** 重置存储的数据。 */
	/** 重置存储的数据。 */
	/** 重置存储的数据。 */
	void Reset();

	// Frame numbers of last update.
 // 最后更新的帧数。
	uint32 PositionUpdatedFrame = 0;
	uint32 TangentUpdatedFrame = 0;
	uint32 ColorUpdatedFrame = 0;

	// Buffers containing deformed geometry data.
 // 包含变形几何数据的缓冲区。
	TRefCountPtr<FRDGPooledBuffer> Position;
	TRefCountPtr<FRDGPooledBuffer> PrevPosition;
	TRefCountPtr<FRDGPooledBuffer> Tangent;
	TRefCountPtr<FRDGPooledBuffer> Color;
	// Shader resource views to the buffers.
 // 缓冲区的着色器资源视图。
	TRefCountPtr<FRHIShaderResourceView> PositionSRV;
	TRefCountPtr<FRHIShaderResourceView> PrevPositionSRV;
	TRefCountPtr<FRHIShaderResourceView> TangentSRV;
	TRefCountPtr<FRHIShaderResourceView> ColorSRV;
};
