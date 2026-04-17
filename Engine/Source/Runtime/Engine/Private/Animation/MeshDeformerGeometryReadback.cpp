// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/MeshDeformerGeometryReadback.h"
#include "MeshDescription.h"

FMeshDeformerGeometryReadbackRequest::~FMeshDeformerGeometryReadbackRequest()
{
	// Signal to the requester that the request was not successfully fulfilled
 // 向请求者发出信号，表明请求未成功满足
	
	if (MeshDescriptionCallback_AnyThread.IsSet())
	{
		if (!bMeshDescriptionHandled)
		{
			MeshDescriptionCallback_AnyThread(FMeshDescription());
		}
	}
	
	if (VertexDataArraysCallback_AnyThread.IsSet())
	{
		if (!bVertexDataArraysHandled)
		{
			VertexDataArraysCallback_AnyThread(FMeshDeformerGeometryReadbackVertexDataArrays());
		}
	}
}