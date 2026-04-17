// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimBlueprintClassSubsystem_PropertyAccess.generated.h"

// Dummy class to prevent warnings on load of older subsystem implementation
// 用于防止加载旧子系统实现时出现警告的虚拟类
UCLASS(MinimalAPI)
class UAnimBlueprintClassSubsystem_PropertyAccess : public UObject
{
	GENERATED_BODY()
};