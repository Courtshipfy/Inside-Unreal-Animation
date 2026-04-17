// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NodeChain.generated.h"

/** A chain of nodes in a hierarchy */
/** 层次结构中的节点链 */
/** 层次结构中的节点链 */
/** 层次结构中的节点链 */
USTRUCT()
struct FNodeChain
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FName> Nodes;
};
