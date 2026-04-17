// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Engine/Blueprint.h"
#include "Interfaces/Interface_PreviewMeshProvider.h"
#include "AnimBlueprint.generated.h"

class SWidget;
class UAnimationAsset;
class USkeletalMesh;
class USkeleton;
class UPoseWatch;
class UPoseWatchFolder;
struct FAnimBlueprintDebugData;
class UAnimGraphNodeBinding;
class UClass;

USTRUCT()
struct FAnimGroupInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FLinearColor Color;

	FAnimGroupInfo()
		: Color(FLinearColor::White)
	{
	}
};

USTRUCT()
struct FAnimParentNodeAssetOverride
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UAnimationAsset> NewAsset;
	UPROPERTY()
	FGuid ParentNodeGuid;

	FAnimParentNodeAssetOverride(FGuid InGuid, UAnimationAsset* InNewAsset)
		: NewAsset(InNewAsset)
		, ParentNodeGuid(InGuid)
	{}

	FAnimParentNodeAssetOverride()
		: NewAsset(NULL)
	{}

	bool operator ==(const FAnimParentNodeAssetOverride& Other)
	{
		return ParentNodeGuid == Other.ParentNodeGuid;
	}
};

/** The method by which a preview animation blueprint is applied */
/** 应用预览动画蓝图的方法 */
/** 应用预览动画蓝图的方法 */
/** 应用预览动画蓝图的方法 */
UENUM()
enum class EPreviewAnimationBlueprintApplicationMethod : uint8
	/** 使用 LinkAnimClassLayers 应用预览动画蓝图 */
{
	/** 使用 LinkAnimClassLayers 应用预览动画蓝图 */
	/** Apply the preview animation blueprint using LinkAnimClassLayers */
	/** 使用 SetLinkedAnimGraphByTag 应用预览动画蓝图 */
	/** 使用 LinkAnimClassLayers 应用预览动画蓝图 */
	LinkedLayers,
	/** 使用 SetLinkedAnimGraphByTag 应用预览动画蓝图 */

	/** Apply the preview animation blueprint using SetLinkedAnimGraphByTag */
	/** 使用 SetLinkedAnimGraphByTag 应用预览动画蓝图 */
	LinkedAnimGraph,
};

/**
 * An Anim Blueprint is essentially a specialized Blueprint whose graphs control the animation of a Skeletal Mesh.
 * It can perform blending of animations, directly control the bones of the skeleton, and output a final pose
 * for a Skeletal Mesh each frame.
 */
UCLASS(BlueprintType, MinimalAPI)
class UAnimBlueprint : public UBlueprint, public IInterface_PreviewMeshProvider
{
	GENERATED_UCLASS_BODY()

	/**
	 * This is the target skeleton asset for anim instances created from this blueprint; all animations
	 * referenced by the BP should be compatible with this skeleton.  For advanced use only, it is easy
	 * to cause errors if this is modified without updating or replacing all referenced animations.
	 */
	UPROPERTY(AssetRegistrySearchable, EditAnywhere, AdvancedDisplay, Category=ClassOptions)
	TObjectPtr<USkeleton> TargetSkeleton;

	// List of animation sync groups
 // 动画同步组列表
	UPROPERTY()
	TArray<FAnimGroupInfo> Groups;

	// This is an anim blueprint that acts as a set of template functionality without being tied to a specific skeleton.
 // 这是一个动画蓝图，充当一组模板功能，而不绑定到特定的骨架。
	// Implies a null TargetSkeleton.
 // 意味着一个 null TargetSkeleton。
	UPROPERTY(AssetRegistrySearchable)
	bool bIsTemplate;
	
	/**
	 * Allows this anim Blueprint to update its native update, blend tree, montages and asset players on
	 * a worker thread. The compiler will attempt to pick up any issues that may occur with threaded update.
	 * For updates to run in multiple threads both this flag and the project setting "Allow Multi Threaded 
	 * Animation Update" should be set.
	 */
	UPROPERTY(EditAnywhere, Category = Optimization)
	bool bUseMultiThreadedAnimationUpdate;

	/**
	 * Selecting this option will cause the compiler to emit warnings whenever a call into Blueprint
	 * is made from the animation graph. This can help track down optimizations that need to be made.
	 */
	UPROPERTY(EditAnywhere, Category = Optimization)
	bool bWarnAboutBlueprintUsage;

	/** If true, linked animation layers will be instantiated only once per AnimClass instead of once per AnimInstance, AnimClass and AnimGroup.
	Extra instances will be created if two or more active anim graph override the same layer Function */
	UPROPERTY(EditDefaultsOnly, Category = Optimization)
	uint8 bEnableLinkedAnimLayerInstanceSharing : 1;

	// @todo document
 // @todo文档
	ENGINE_API class UAnimBlueprintGeneratedClass* GetAnimBlueprintGeneratedClass() const;

	// @todo document
 // @todo文档
	ENGINE_API class UAnimBlueprintGeneratedClass* GetAnimBlueprintSkeletonClass() const;

	ENGINE_API virtual void Serialize(FArchive& Ar) override;

#if WITH_EDITOR

	ENGINE_API virtual UClass* GetBlueprintClass() const override;

	// Inspects the hierarchy and looks for an override for the requested node GUID
 // 检查层次结构并查找请求的节点 GUID 的覆盖
	// @param NodeGuid - Guid of the node to search for
 // @param NodeGuid - 要搜索的节点的 Guid
	// @param bIgnoreSelf - Ignore this blueprint and only search parents, handy for finding parent overrides
 // @param bIgnoreSelf - 忽略此蓝图并仅搜索父级，方便查找父级覆盖
	ENGINE_API FAnimParentNodeAssetOverride* GetAssetOverrideForNode(FGuid NodeGuid, bool bIgnoreSelf = false) const ;

	// Inspects the hierarchy and builds a list of all asset overrides for this blueprint
 // 检查层次结构并构建此蓝图的所有资产覆盖的列表
	// @param OutOverrides - Array to fill with overrides
 // @param OutOverrides - 用于填充覆盖的数组
	// @return bool - Whether any overrides were found
 // @return bool - 是否发现任何覆盖
	ENGINE_API bool GetAssetOverrides(TArray<FAnimParentNodeAssetOverride*>& OutOverrides);

	/** 返回给定蓝图的最基本的动画蓝图（如果它是从另一个动画蓝图继承的，如果只有本机/非动画 BP 类是它的父级，则返回 null） */
	// UBlueprint interface
 // U蓝图界面
	virtual bool SupportedByDefaultBlueprintFactory() const override
	/** 返回给定蓝图的父动画蓝图（如果它是从另一个动画蓝图继承的，如果只有本机/非动画 BP 类是它的父级，则返回 null） */
	{
		return false;
	}

	/** 返回给定蓝图的最基本的动画蓝图（如果它是从另一个动画蓝图继承的，如果只有本机/非动画 BP 类是它的父级，则返回 null） */
	virtual bool IsValidForBytecodeOnlyRecompile() const override { return false; }
	ENGINE_API virtual bool CanAlwaysRecompileWhilePlayingInEditor() const override;
	// End of UBlueprint interface
 // UBlueprint接口结束
	/** 返回给定蓝图的父动画蓝图（如果它是从另一个动画蓝图继承的，如果只有本机/非动画 BP 类是它的父级，则返回 null） */

	// Finds the index of the specified group, or creates a new entry for it (unless the name is NAME_None, which will return INDEX_NONE)
 // 查找指定组的索引，或为其创建一个新条目（除非名称为 NAME_None，这将返回 INDEX_NONE）
	ENGINE_API int32 FindOrAddGroup(FName GroupName);

	/** Returns the most base anim blueprint for a given blueprint (if it is inherited from another anim blueprint, returning null if only native / non-anim BP classes are it's parent) */
	/** 返回给定蓝图的最基本的动画蓝图（如果它是从另一个动画蓝图继承的，如果只有本机/非动画 BP 类是它的父级，则返回 null） */
	static ENGINE_API UAnimBlueprint* FindRootAnimBlueprint(const UAnimBlueprint* DerivedBlueprint);

	/** Returns the parent anim blueprint for a given blueprint (if it is inherited from another anim blueprint, returning null if only native / non-anim BP classes are it's parent) */
	/** 返回给定蓝图的父动画蓝图（如果它是从另一个动画蓝图继承的，如果只有本机/非动画 BP 类是它的父级，则返回 null） */
	static ENGINE_API UAnimBlueprint* GetParentAnimBlueprint(const UAnimBlueprint* DerivedBlueprint);
	
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnOverrideChangedMulticaster, FGuid, UAnimationAsset*);

	typedef FOnOverrideChangedMulticaster::FDelegate FOnOverrideChanged;

	void RegisterOnOverrideChanged(const FOnOverrideChanged& Delegate)
	{
		OnOverrideChanged.Add(Delegate);
	}

	void UnregisterOnOverrideChanged(SWidget* Widget)
	{
		OnOverrideChanged.RemoveAll(Widget);
	}

	void NotifyOverrideChange(FAnimParentNodeAssetOverride& Override)
	{
		OnOverrideChanged.Broadcast(Override.ParentNodeGuid, Override.NewAsset);
	}

	ENGINE_API virtual void PostLoad() override;
	/** IInterface_PreviewMeshProvider接口 */
	ENGINE_API virtual bool FindDiffs(const UBlueprint* OtherBlueprint, FDiffResults& Results) const override;
	ENGINE_API virtual void SetObjectBeingDebugged(UObject* NewObject) override;
	ENGINE_API virtual bool SupportsAnimLayers() const override;
	ENGINE_API virtual bool SupportsEventGraphs() const override;
	ENGINE_API virtual bool SupportsDelegates() const override;
	/** 预览动画蓝图支持 */
	ENGINE_API virtual bool SupportsMacros() const override;
	ENGINE_API virtual bool SupportsInputEvents() const override;
	/** IInterface_PreviewMeshProvider接口 */
	ENGINE_API virtual bool AllowFunctionOverride(const UFunction* const InFunction) const override;
	ENGINE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	ENGINE_API virtual void GetTypeActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	ENGINE_API virtual void GetInstanceActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	/** 预览动画蓝图支持 */
protected:
	// Broadcast when an override is changed, allowing derived blueprints to be updated
 // 当覆盖更改时广播，允许更新派生蓝图
	FOnOverrideChangedMulticaster OnOverrideChanged;
	/** 检查动画实例是否是该动画 BP 的活动调试对象 */
#endif	// #if WITH_EDITOR

public:
	/** 获取该动画 BP 的调试数据 */
	/** IInterface_PreviewMeshProvider interface */
	/** IInterface_PreviewMeshProvider接口 */
	ENGINE_API virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true) override;
	/** 检查动画实例是否是该动画 BP 的活动调试对象 */
	ENGINE_API virtual USkeletalMesh* GetPreviewMesh(bool bFindIfNotSet = false) override;
	ENGINE_API virtual USkeletalMesh* GetPreviewMesh() const override;

	/** 获取该动画 BP 的调试数据 */
	/** Preview anim blueprint support */
	/** 预览动画蓝图支持 */
	ENGINE_API void SetPreviewAnimationBlueprint(UAnimBlueprint* InPreviewAnimationBlueprint);
	ENGINE_API UAnimBlueprint* GetPreviewAnimationBlueprint() const;

	ENGINE_API void SetPreviewAnimationBlueprintApplicationMethod(EPreviewAnimationBlueprintApplicationMethod InMethod);
	ENGINE_API EPreviewAnimationBlueprintApplicationMethod GetPreviewAnimationBlueprintApplicationMethod() const;

	ENGINE_API void SetPreviewAnimationBlueprintTag(FName InTag);
	ENGINE_API FName GetPreviewAnimationBlueprintTag() const;

public:
	/** Check if the anim instance is the active debug object for this anim BP */
	/** 检查动画实例是否是该动画 BP 的活动调试对象 */
	ENGINE_API bool IsObjectBeingDebugged(const UObject* AnimInstance) const;

	/** Get the debug data for this anim BP */
	/** 获取该动画 BP 的调试数据 */
	ENGINE_API FAnimBlueprintDebugData* GetDebugData() const;

#if WITH_EDITORONLY_DATA
public:
	// Queue a refresh of the set of anim blueprint extensions that this anim blueprint hosts.
 // 对该动画蓝图托管的一组动画蓝图扩展的刷新进行排队。
	// Usually called from anim graph nodes to ensure that extensions that are no longer required are cleaned up.
 // 通常从动画图节点调用，以确保不再需要的扩展被清理。
	void RequestRefreshExtensions() { bRefreshExtensions = true; }

	// Check if the anim BP is compatible with this one (for linked instancing). Checks target skeleton, template flags
 // 检查动画 BP 是否与此兼容（对于链接实例）。检查目标骨架、模板标志
	// blueprint type.
 // 蓝图类型。
	// Note compatibility is directional - e.g. template anim BPs can be instanced within any 'regular' anim BP, but not
 // 注意兼容性是有方向性的 - 例如模板动画 BP 可以在任何“常规”动画 BP 中实例化，但不能
	// vice versa
 // 反之亦然
	// @param	InAnimBlueprint		The anim blueprint to check for compatibility
 // @param InAnimBlueprint 用于检查兼容性的动画蓝图
	ENGINE_API bool IsCompatible(const UAnimBlueprint* InAnimBlueprint) const;
	
	// Check if the asset path of a skeleton, template and interface flags are compatible with this anim blueprint
 // 检查骨架、模板和界面标志的资源路径是否与该动画蓝图兼容
	// (for linked instancing)
 // （对于链接实例）
	// @param	InSkeletonAsset		The asset path of the skeleton asset used by the anim blueprint
 // @param InSkeletonAsset 动画蓝图使用的骨架资源的资源路径
	/** 预览此资源时使用的默认骨架网格物体 - 仅当您使用此资源打开角色时才适用*/
	// @param	bInIsTemplate		Whether the anim blueprint to check is a template
 // @param bInIsTemplate 要检查的动画蓝图是否是模板
	// @param	bInIsInterface		Whether the anim blueprint to check is an interface
 // @param bInIsInterface 要检查的动画蓝图是否是一个接口
	ENGINE_API bool IsCompatibleByAssetString(const FString& InSkeletonAsset, bool bInIsTemplate, bool bInIsInterface) const;
	
	// Get the default binding type that any new nodes will use when created
 // 获取任何新节点在创建时将使用的默认绑定类型
	UClass* GetDefaultBindingClass() const { return DefaultBindingClass; }

	virtual void NotifyGraphRenamed(class UEdGraph* Graph, FName OldName, FName NewName) override;
	/** 任何新节点在创建时将使用的默认绑定类型 */

	// Event that is broadcast to inform observers that the node title has changed
 // 广播事件以通知观察者节点标题已更改
	// The default SAnimationGraphNode uses this to invalidate cached node title text
 // 默认的 SAnimationGraphNode 使用它来使缓存的节点标题文本无效
	/** 应用预览动画蓝图的方法，无论是作为覆盖层还是作为链接实例 */
	DECLARE_EVENT_ThreeParams(UAnimBlueprint, FOnGraphRenamedEvent, UEdGraph*, FName, FName);
	FOnGraphRenamedEvent& OnGraphRenamedEvent() { return GraphRenameEvent; }
	/** 预览此资源时使用的默认骨架网格物体 - 仅当您使用此资源打开角色时才适用*/

	/** 通过 LinkAnimGraphByTag 应用预览动画蓝图时要使用的标签 */
private:
	FOnGraphRenamedEvent GraphRenameEvent;

public:
	/** 如果设置，则需要根据生成的节点刷新扩展 */
	// Array of overrides to asset containing nodes in the parent that have been overridden
 // 对资产的覆盖数组，其中包含父级中已被覆盖的节点
	UPROPERTY()
	TArray<FAnimParentNodeAssetOverride> ParentAssetOverrides;

	/** 任何新节点在创建时将使用的默认绑定类型 */
	// Array of active pose watches (pose watches allows us to see the bone pose at a 
 // 一系列主动姿势手表（姿势手表允许我们以一定的速度查看骨骼姿势
	// particular point of the anim graph and control debug draw for unselected anim nodes).
 // 动画图的特定点和未选择的动画节点的控制调试绘制）。
	UPROPERTY()
	TArray<TObjectPtr<UPoseWatchFolder>> PoseWatchFolders;
	/** 应用预览动画蓝图的方法，无论是作为覆盖层还是作为链接实例 */
	
	UPROPERTY()
	TArray<TObjectPtr<UPoseWatch>> PoseWatches;

	/** 通过 LinkAnimGraphByTag 应用预览动画蓝图时要使用的标签 */
private:
	friend class FAnimBlueprintCompilerContext;
	
	/** The default skeletal mesh to use when previewing this asset - this only applies when you open Persona using this asset*/
	/** 如果设置，则需要根据生成的节点刷新扩展 */
	/** [翻译失败: The default skeletal mesh to use when previewing this asset - this only applies when you open Persona using this asset]*/
	UPROPERTY(duplicatetransient, AssetRegistrySearchable)
	TSoftObjectPtr<class USkeletalMesh> PreviewSkeletalMesh;

	/** 
	 * An animation Blueprint to overlay with this Blueprint. When working on layers, this allows this Blueprint to be previewed in the context of another 'outer' anim blueprint. 
	 * Setting this is the equivalent of running the preview animation blueprint on the preview mesh, then calling SetLayerOverlay with this anim blueprint.
	 */
	UPROPERTY(duplicatetransient, AssetRegistrySearchable)
	TSoftObjectPtr<class UAnimBlueprint> PreviewAnimationBlueprint;

	/** The default binding type that any new nodes will use when created */
	/** [翻译失败: The default binding type that any new nodes will use when created] */
	UPROPERTY(EditAnywhere, Category=Bindings, meta=(AllowedClasses="/Script/AnimGraph.AnimGraphNodeBinding", ShowDisplayNames=true, NoClear))
	TObjectPtr<UClass> DefaultBindingClass;

	/** The method by which a preview animation blueprint is applied, either as an overlay layer, or as a linked instance */
	/** 应用预览动画蓝图的方法，无论是作为覆盖层还是作为链接实例 */
	UPROPERTY()
	EPreviewAnimationBlueprintApplicationMethod PreviewAnimationBlueprintApplicationMethod;

	/** The tag to use when applying a preview animation blueprint via LinkAnimGraphByTag */
	/** 通过 LinkAnimGraphByTag 应用预览动画蓝图时要使用的标签 */
	UPROPERTY()
	FName PreviewAnimationBlueprintTag;

	/** If set, then extensions need to be refreshed according to spawned nodes */
	/** 如果设置，则需要根据生成的节点刷新扩展 */
	bool bRefreshExtensions;
#endif // WITH_EDITORONLY_DATA
};
