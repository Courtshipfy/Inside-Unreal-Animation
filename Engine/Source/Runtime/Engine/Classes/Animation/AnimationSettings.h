// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimationSettings.h: Declares the AnimationSettings class.
=============================================================================*/

#pragma once

#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_5
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Animation/AnimSequence.h"
#endif
#include "Engine/DeveloperSettings.h"
#include "CustomAttributes.h"
#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_5
#include "MirrorDataTable.h"
#include "StructUtils/UserDefinedStruct.h"
#endif
#include "AnimationSettings.generated.h"

class UUserDefinedStruct;
struct FCustomAttributeSetting;
struct FMirrorFindReplaceExpression;

/**
 * Default animation settings.
 */
UCLASS(config=Engine, defaultconfig, meta=(DisplayName="Animation"), MinimalAPI)
class UAnimationSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	/** Compression version for recompress commandlet, bump this to trigger full recompressed, otherwise only new imported animations will be recompressed */
	/** 重新压缩命令行开关的压缩版本，碰撞此触发完全重新压缩，否则仅新导入的动画将被重新压缩 */
	/** 重新压缩命令行开关的压缩版本，碰撞此触发完全重新压缩，否则仅新导入的动画将被重新压缩 */
	/** [翻译失败: Compression version for recompress commandlet, bump this to trigger full recompressed, otherwise only new imported animations will be recompressed] */
	UPROPERTY(config, VisibleAnywhere, Category = Compression)
	int32 CompressCommandletVersion;
	/** 除了任何带有插槽的骨骼之外，还需要更高精度处理的骨骼名称列表 */

	/** 除了任何带有插槽的骨骼之外，还需要更高精度处理的骨骼名称列表 */
	/** List of bone names to treat with higher precision, in addition to any bones with sockets */
	/** 除了任何带有插槽的骨骼之外，还需要更高精度处理的骨骼名称列表 */
	/** 如果为 true，这将强制重新压缩每个动画，不应选中启用 */
	UPROPERTY(config, EditAnywhere, Category = Compression)
	TArray<FString> KeyEndEffectorsMatchNameArray;
	/** 如果为 true，这将强制重新压缩每个动画，不应选中启用 */

	/** 如果为 true 并且现有压缩误差大于替代压缩阈值，则将使用具有较低误差的任何压缩技术（即使是增加大小的技术），直到其低于阈值 */
	/** If true, this will forcibly recompress every animation, this should not be checked in enabled */
	/** 如果为 true，这将强制重新压缩每个动画，不应选中启用 */
	UPROPERTY(config, EditAnywhere, Category = Compression)
	/** 如果为 true 并且现有压缩误差大于替代压缩阈值，则将使用具有较低误差的任何压缩技术（即使是增加大小的技术），直到其低于阈值 */
	bool ForceRecompression;

	/** If true and the existing compression error is greater than Alternative Compression Threshold, then any compression technique (even one that increases the size) with a lower error will be used until it falls below the threshold */
	/** 如果为 true 并且现有压缩误差大于替代压缩阈值，则将使用具有较低误差的任何压缩技术（即使是增加大小的技术），直到其低于阈值 */
	UPROPERTY(config, EditAnywhere, Category = Compression)
	bool bForceBelowThreshold;
	/** 如果为 true 并且现有压缩误差大于替代压缩阈值，则替代压缩阈值将有效提升到现有错误级别 */

	/** If true, then the animation will be first recompressed with its current compressor if non-NULL, or with the global default compressor (specified in the engine ini) 
	* Also known as "Run Current Default Compressor"
	*/
	/** 如果为 true 并且现有压缩误差大于替代压缩阈值，则替代压缩阈值将有效提升到现有错误级别 */
	/** 如果为 true，重新压缩将记录性能信息 */
	UPROPERTY(config, EditAnywhere, Category = Compression)
	bool bFirstRecompressUsingCurrentOrDefault;

	/** If true and the existing compression error is greater than Alternative Compression Threshold, then Alternative Compression Threshold will be effectively raised to the existing error level */
	/** 如果为 true，动画轨迹数据将从专用服务器烘焙数据中剥离 */
	/** 如果为 true 并且现有压缩误差大于替代压缩阈值，则替代压缩阈值将有效提升到现有错误级别 */
	/** 如果为 true，重新压缩将记录性能信息 */
	UE_DEPRECATED(5.1, "This is being removed because it is unused")
	UPROPERTY(config /*, EditAnywhere, Category = Compression*/, meta = (DeprecatedProperty, DeprecationMessage = "No longer used."))
	/** 如果为 true，则 4.19 之前的骨架网格物体初始化期间零滴答动画的行为 */
	bool bRaiseMaxErrorToExisting;

	/** 如果为 true，动画轨迹数据将从专用服务器烘焙数据中剥离 */
	/** If true, recompression will log performance information */
	/** [翻译失败: If true, recompression will log performance information] */
	UPROPERTY(config, EditAnywhere, Category = Performance)
	bool bEnablePerformanceLog;
	/** 如果为 true，则 4.19 之前的骨架网格物体初始化期间零滴答动画的行为 */

	/** 直接导入其相应骨骼名称的动画属性名称列表。含义字段允许将属性名称置于上下文中并为其定制工具。 */
	/** If true, animation track data will be stripped from dedicated server cooked data */
	/** 如果为 true，动画轨迹数据将从专用服务器烘焙数据中剥离 */
	UPROPERTY(config, EditAnywhere, Category = Performance)
	bool bStripAnimationDataOnDedicatedServer;

	/** If true, pre-4.19 behavior of zero-ticking animations during skeletal mesh init */
	/** 如果为 true，则 4.19 之前的骨架网格物体初始化期间零滴答动画的行为 */
	UPROPERTY(config, EditAnywhere, Category = Performance)
	/** 直接导入其相应骨骼名称的动画属性名称列表。含义字段允许将属性名称置于上下文中并为其定制工具。 */
	/** 所有动画属性都直接导入到骨骼上的骨骼名称列表。 */
	bool bTickAnimationOnSkeletalMeshInit;

	/** Names that identify bone animation attributes representing the individual components of a timecode and a subframe along with a take name.
	    These will be included in the list of bone custom attribute names to import. */
	/** 动画属性特定混合类型（按名称） */
	UPROPERTY(config, EditAnywhere, Category = AnimationAttributes, meta=(DisplayName="Bone Timecode Animation Attribute name settings"))
	FTimecodeCustomAttributeNameSettings BoneTimecodeCustomAttributeNameSettings;

	/** List of animation attribute names to import directly on their corresponding bone names. The meaning field allows to contextualize the attribute name and customize tooling for it. */
	/** 默认动画属性混合类型 */
	/** 直接导入其相应骨骼名称的动画属性名称列表。含义字段允许将属性名称置于上下文中并为其定制工具。 */
	/** 所有动画属性都直接导入到骨骼上的骨骼名称列表。 */
	UPROPERTY(config, EditAnywhere, Category = AnimationAttributes, meta=(DisplayName="Bone Animation Attributes names"))
	TArray<FCustomAttributeSetting> BoneCustomAttributesNames;
	/** 将 FBX 节点变换曲线导入为属性时要匹配的名称（可以使用 ? 和 * 通配符） */

	/** Gets the complete list of bone animation attribute names to consider for import.
	/** 动画属性特定混合类型（按名称） */
	    This includes the designated timecode animation attributes as well as other bone animation attributes identified in the settings. */
	/** 将用户定义的结构注册为动画属性*/
	UFUNCTION(BlueprintPure, Category = AnimationAttributes)
	ENGINE_API TArray<FString> GetBoneCustomAttributeNamesToImport() const;

	/** 默认动画属性混合类型 */
	/** 查找并替换用于镜像的表达式  */
	/** List of bone names for which all animation attributes are directly imported on the bone. */
	/** 所有动画属性都直接导入到骨骼上的骨骼名称列表。 */
	UPROPERTY(config, EditAnywhere, Category = AnimationAttributes, meta=(DisplayName="Bone names with Animation Attributes"))
	TArray<FString> BoneNamesWithCustomAttributes;
	/** （重新）初始化任何基于动画的数据时使用的项目特定默认帧速率 */
	/** 将 FBX 节点变换曲线导入为属性时要匹配的名称（可以使用 ? 和 * 通配符） */

	/** Animation Attribute specific blend types (by name) */
	/** 动画属性特定混合类型（按名称） */
	/** 是否强制项目仅使用 SupportedFrameRates 中的条目作为动画资源，如果禁用，则会发出警告 */
	UPROPERTY(config, EditAnywhere, Category = AnimationAttributes)
	/** 将用户定义的结构注册为动画属性*/
	TMap<FName, ECustomAttributeBlendType> AttributeBlendModes;

	/** Default Animation Attribute blend type */
	/** 默认动画属性混合类型 */
	/** 返回项目特定的默认帧速率 */
	/** 查找并替换用于镜像的表达式  */
	UPROPERTY(config, EditAnywhere, Category = AnimationAttributes)
	ECustomAttributeBlendType DefaultAttributeBlendMode;

	/** Names to match against when importing FBX node transform curves as attributes (can use ? and * wildcards) */
	/** （重新）初始化任何基于动画的数据时使用的项目特定默认帧速率 */
	/** 将 FBX 节点变换曲线导入为属性时要匹配的名称（可以使用 ? 和 * 通配符） */
	UPROPERTY(config, EditAnywhere, Category = AnimationAttributes)
	TArray<FString> TransformAttributeNames;

	/** 是否强制项目仅使用 SupportedFrameRates 中的条目作为动画资源，如果禁用，则会发出警告 */
	/** Register user defined structs as animation attributes*/
	/** 将用户定义的结构注册为动画属性*/
	UPROPERTY(config, EditAnywhere, DisplayName="User Defined Struct Animation Attributes (Runtime only, Non-blendable)", Category = AnimationAttributes, meta=(AllowedClasses="/Script/Engine.UserDefinedStruct"))
	TArray<TSoftObjectPtr<UUserDefinedStruct>> UserDefinedStructAttributes;

	/** Find and Replace Expressions used for mirroring  */
	/** 返回项目特定的默认帧速率 */
	/** 查找并替换用于镜像的表达式  */
	UPROPERTY(config, EditAnywhere, Category = Mirroring)
	TArray<FMirrorFindReplaceExpression> MirrorFindReplaceExpressions;

	/** Project specific default frame-rate used when (re)initializing any animation based data */
	/** （重新）初始化任何基于动画的数据时使用的项目特定默认帧速率 */
	UPROPERTY(config, EditAnywhere, Category = AnimationData)
	FFrameRate DefaultFrameRate;

	/** Whether to enforce the project to only use entries from SupportedFrameRates for the animation assets, if disable will warn instead */
	/** 是否强制项目仅使用 SupportedFrameRates 中的条目作为动画资源，如果禁用，则会发出警告 */
	UPROPERTY(config, EditAnywhere, Category = AnimationData)
	bool bEnforceSupportedFrameRates;
public:
	static UAnimationSettings * Get() { return CastChecked<UAnimationSettings>(UAnimationSettings::StaticClass()->GetDefaultObject()); }

	/** Returns the project specific default frame-rate */
	/** 返回项目特定的默认帧速率 */
	ENGINE_API const FFrameRate& GetDefaultFrameRate() const;

#if WITH_EDITOR
	ENGINE_API virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
};
