// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

#define UE_API ANIMGRAPHRUNTIME_API

// Custom serialization version for assets/classes in the AnimGraphRuntime and AnimGraph modules
// AnimGraphRuntime 和 AnimGraph 模块中资源/类的自定义序列化版本
struct FAnimationCustomVersion
{
	enum Type
	{
		// Before any version changes were made in the plugin
  // 在插件中进行任何版本更改之前
		BeforeCustomVersionWasAdded = 0,

		// Added support for one-to-many component mappings to FAnimNode_BoneDrivenController,
  // 添加了对 FAnimNode_BoneDrivenController 的一对多组件映射的支持，
		// changed the range to apply to the input, and added a configurable method for updating the components
  // 更改了适用于输入的范围，并添加了用于更新组件的可配置方法
		BoneDrivenControllerMatchingMaya = 1,

		// Converted the range clamp into a remap function, rather than just clamping
  // 将范围钳位转换为重映射函数，而不仅仅是钳位
		BoneDrivenControllerRemapping = 2,

		// Added ability to offset angular ranges for constraints
  // 增加了偏移约束角度范围的能力
		AnimDynamicsAddAngularOffsets = 3,

		// Renamed Stretch Limits to better names
  // 将拉伸限制重命名为更好的名称
		RenamedStretchLimits = 4,

		// Convert IK to support FBoneSocketTarget
  // 转换 IK 以支持 FBoneSocketTarget
		ConvertIKToSupportBoneSocketTarget = 5,

		// -----<new versions can be added above this line>-------------------------------------------------
  // ------<可以在该行上方添加新版本>------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
 // 此自定义版本号的 GUID
	UE_API const static FGuid GUID;

private:
	FAnimationCustomVersion() {}
};

#undef UE_API
