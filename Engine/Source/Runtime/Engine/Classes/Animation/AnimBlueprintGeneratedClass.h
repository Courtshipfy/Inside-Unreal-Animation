// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Animation/AnimTypes.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Animation/AnimStateMachineTypes.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/ExposedValueHandler.h"
#include "Engine/PoseWatchRenderData.h"

#include "AnimBlueprintGeneratedClass.generated.h"

class UAnimGraphNode_Base;
class UAnimGraphNode_StateMachineBase;
class UAnimInstance;
class UAnimStateNode;
class UAnimStateNodeBase;
class UAnimStateAliasNode;
class UAnimStateTransitionNode;
class UEdGraph;
class USkeleton;
class UPoseWatch;
class UPoseWatchPoseElement;
struct FAnimSubsystem;
struct FAnimSubsystemInstance;
struct FPropertyAccessLibrary;

// Represents the debugging information for a single state within a state machine
// 表示状态机内单个状态的调试信息
USTRUCT()
struct FStateMachineStateDebugData
{
	GENERATED_BODY()

public:
	FStateMachineStateDebugData()
		: StateMachineIndex(INDEX_NONE)
		, StateIndex(INDEX_NONE)
		, Weight(0.0f)
		, ElapsedTime(0.0f)
	{
	}

	FStateMachineStateDebugData(int32 InStateMachineIndex, int32 InStateIndex, float InWeight, float InElapsedTime)
		: StateMachineIndex(InStateMachineIndex)
		, StateIndex(InStateIndex)
		, Weight(InWeight)
		, ElapsedTime(InElapsedTime)
	{}

	// The index of the state machine
 // 状态机索引
	int32 StateMachineIndex;

	// The index of the state
 // 状态指数
	int32 StateIndex;

	// The last recorded weight for this state
 // 该州最后记录的体重
	float Weight;

	// The time that this state has been active (only valid if this is the current state)
 // 该状态处于活动状态的时间（仅在当前状态时有效）
	float ElapsedTime;
};

// This structure represents debugging information for a single state machine
// 该结构表示单个状态机的调试信息
USTRUCT()
struct FStateMachineDebugData
{
	GENERATED_BODY()

public:
	FStateMachineDebugData()
		: MachineIndex(INDEX_NONE)
	{}

	struct FStateAliasTransitionStateIndexPair
	{
		int32 TransitionIndex;
		int32 AssociatedStateIndex;
	};

	// Map from state nodes to their state entry in a state machine
 // 从状态节点映射到状态机中的状态条目
	TMap<TWeakObjectPtr<UEdGraphNode>, int32> NodeToStateIndex;
	TMap<int32, TWeakObjectPtr<UAnimStateNodeBase>> StateIndexToNode;

	// Transition nodes may be associated w/ multiple transition indicies when the source state is an alias
 // 当源状态是别名时，转换节点可以与多个转换索引相关联
	TMultiMap<TWeakObjectPtr<UEdGraphNode>, int32> NodeToTransitionIndex;

	// Mapping between an alias and any transition indices it might be associated to (Both as source and target).
 // 别名和它可能关联的任何转换索引之间的映射（作为源和目标）。
	TMultiMap<TWeakObjectPtr<UAnimStateAliasNode>, FStateAliasTransitionStateIndexPair> StateAliasNodeToTransitionStatePairs;

	// The animation node that leads into this state machine (A3 only)
 // 进入此状态机的动画节点（仅限 A3）
	TWeakObjectPtr<UAnimGraphNode_StateMachineBase> MachineInstanceNode;

	// Index of this machine in the StateMachines array
 // StateMachines 数组中该机器的索引
	int32 MachineIndex;

public:
	ENGINE_API UEdGraphNode* FindNodeFromStateIndex(int32 StateIndex) const;
	ENGINE_API UEdGraphNode* FindNodeFromTransitionIndex(int32 TransitionIndex) const;
};

// This structure represents debugging information for a frame snapshot
// 该结构表示帧快照的调试信息
USTRUCT()
struct FAnimationFrameSnapshot
{
	GENERATED_USTRUCT_BODY()

	FAnimationFrameSnapshot()
#if WITH_EDITORONLY_DATA
		: TimeStamp(0.0)
#endif
	{
	}
#if WITH_EDITORONLY_DATA
public:
	// The snapshot of data saved from the animation
 // 动画保存的数据快照
	TArray<uint8> SerializedData;

	// The time stamp for when this snapshot was taken (relative to the life timer of the object being recorded)
 // 拍摄此快照的时间戳（相对于正在记录的对象的生命周期）
	double TimeStamp;

public:
	void InitializeFromInstance(UAnimInstance* Instance);
	ENGINE_API void CopyToInstance(UAnimInstance* Instance);
#endif
};

struct FAnimBlueprintDebugData_NodeVisit
{
	int32 SourceID;
	int32 TargetID;
	float Weight;

	FAnimBlueprintDebugData_NodeVisit(int32 InSourceID, int32 InTargetID, float InWeight)
		: SourceID(InSourceID)
		, TargetID(InTargetID)
		, Weight(InWeight)
	{
	}
};


struct FAnimBlueprintDebugData_AttributeRecord
{
	FName Attribute;
	int32 OtherNode;

	FAnimBlueprintDebugData_AttributeRecord(int32 InOtherNode, FName InAttribute)
		: Attribute(InAttribute)
		, OtherNode(InOtherNode)
	{}
};

// This structure represents animation-related debugging information for an entire AnimBlueprint
// 该结构代表整个 AnimBlueprint 的动画相关调试信息
// (general debug information for the event graph, etc... is still contained in a FBlueprintDebugData structure)
// （事件图等的一般调试信息...仍然包含在 FBlueprintDebugData 结构中）
USTRUCT()
struct FAnimBlueprintDebugData
{
	GENERATED_USTRUCT_BODY()

	FAnimBlueprintDebugData()
#if WITH_EDITORONLY_DATA
		: SnapshotBuffer(NULL)
		, SnapshotIndex(INDEX_NONE)
#endif
	{
	}

#if WITH_EDITORONLY_DATA
public:
	// Map from state machine graphs to their corresponding debug data
 // 从状态机图映射到相应的调试数据
	TMap<TWeakObjectPtr<const UEdGraph>, FStateMachineDebugData> StateMachineDebugData;

	// Map from state graphs to their node
 // 从状态图到其节点的映射
	TMap<TWeakObjectPtr<const UEdGraph>, TWeakObjectPtr<UAnimStateNode> > StateGraphToNodeMap;

	// Map from transition graphs to their node
 // 从转移图到其节点的映射
	TMap<TWeakObjectPtr<const UEdGraph>, TWeakObjectPtr<UAnimStateTransitionNode> > TransitionGraphToNodeMap;

	// Map from custom transition blend graphs to their node
 // 从自定义过渡混合图映射到其节点
	TMap<TWeakObjectPtr<const UEdGraph>, TWeakObjectPtr<UAnimStateTransitionNode> > TransitionBlendGraphToNodeMap;

	// Map from animation node to their property index
 // 从动画节点映射到其属性索引
	TMap<TWeakObjectPtr<const UAnimGraphNode_Base>, int32> NodePropertyToIndexMap;

	// Map from node property index to source editor node
 // 从节点属性索引映射到源编辑器节点
	TMap<int32, TWeakObjectPtr<const UEdGraphNode> > NodePropertyIndexToNodeMap;

	// Map from animation node GUID to property index
 // 从动画节点 GUID 映射到属性索引
	TMap<FGuid, int32> NodeGuidToIndexMap;

	// Map from animation node to attributes
 // 从动画节点映射到属性
	TMap<TWeakObjectPtr<const UAnimGraphNode_Base>, TArray<FName>> NodeAttributes;

	// The debug data for each state machine state
 // 每个状态机状态的调试数据
	TArray<FStateMachineStateDebugData> StateData;	
	
	// History of snapshots of animation data
 // 动画数据快照的历史记录
	TSimpleRingBuffer<FAnimationFrameSnapshot>* SnapshotBuffer;

	// Mapping from graph pins to their folded properties.
 // 从图形引脚映射到其折叠属性。
	// Graph pins are unique per node instance and thus suitable as identifier for the properties.
 // 图形引脚对于每个节点实例都是唯一的，因此适合作为属性的标识符。
	TMap<FEdGraphPinReference, FProperty*> GraphPinToFoldedPropertyMap;

	// Node visit structure
 // 节点访问结构
	using FNodeVisit = FAnimBlueprintDebugData_NodeVisit;

	// History of activated nodes
 // 激活节点的历史记录
	TArray<FNodeVisit> UpdatedNodesThisFrame;

	// Record of attribute transfer between nodes
 // 节点间属性传递记录
	using FAttributeRecord = FAnimBlueprintDebugData_AttributeRecord;

	// History of node attributes that are output from and input to nodes
 // 从节点输出和输入到节点的节点属性的历史记录
	TMap<int32, TArray<FAttributeRecord>> NodeInputAttributesThisFrame;
	TMap<int32, TArray<FAttributeRecord>> NodeOutputAttributesThisFrame;

	// History of node syncs - maps from player node index to graph-determined group name
 // 节点同步的历史记录 - 从玩家节点索引映射到图形确定的组名称
	TMap<int32, FName> NodeSyncsThisFrame;

	// Values output by nodes
 // 节点输出的值
	struct FNodeValue
	{
		FString Text;
		int32 NodeID;

		FNodeValue(const FString& InText, int32 InNodeID)
			: Text(InText)
			, NodeID(InNodeID)
		{}
	};

	// Values output by nodes
 // 节点输出的值
	TArray<FNodeValue> NodeValuesThisFrame;

	// Record of a sequence player's state
 // 序列播放器状态记录
	struct FSequencePlayerRecord
	{
		FSequencePlayerRecord(int32 InNodeID, float InPosition, float InLength, int32 InFrameCount)
			: NodeID(InNodeID)
			, Position(InPosition)
			, Length(InLength) 
			, FrameCount(InFrameCount)
		{}

		int32 NodeID;
		float Position;
		float Length;
		int32 FrameCount;
	};

	// All sequence player records this frame
 // 所有序列播放器都会记录此帧
	TArray<FSequencePlayerRecord> SequencePlayerRecordsThisFrame;

	// Record of a blend space player's state
 // 混合空间玩家状态记录
	struct FBlendSpacePlayerRecord
	{
		FBlendSpacePlayerRecord(int32 InNodeID, const UBlendSpace* InBlendSpace, const FVector& InPosition, const FVector& InFilteredPosition)
			: NodeID(InNodeID)
			, BlendSpace(InBlendSpace)
			, Position(InPosition)
			, FilteredPosition(InFilteredPosition)
		{}

		int32 NodeID;
		TWeakObjectPtr<const UBlendSpace> BlendSpace;
		FVector Position;
		FVector FilteredPosition;
	};

	// All blend space player records this frame
 // 所有混合空间播放器都会记录此帧
	TArray<FBlendSpacePlayerRecord> BlendSpacePlayerRecordsThisFrame;

	// Active pose watches to track
 // 手表可追踪的主动姿势
	TArray<FAnimNodePoseWatch> AnimNodePoseWatch;

	// Index of snapshot
 // 快照索引
	int32 SnapshotIndex;
public:

	~FAnimBlueprintDebugData()
	{
		if (SnapshotBuffer != NULL)
		{
			delete SnapshotBuffer;
		}
		SnapshotBuffer = NULL;
	}



	bool IsReplayingSnapshot() const { return SnapshotIndex != INDEX_NONE; }
	ENGINE_API void TakeSnapshot(UAnimInstance* Instance);
	ENGINE_API float GetSnapshotLengthInSeconds();
	ENGINE_API int32 GetSnapshotLengthInFrames();
	ENGINE_API void SetSnapshotIndexByTime(UAnimInstance* Instance, double TargetTime);
	ENGINE_API void SetSnapshotIndex(UAnimInstance* Instance, int32 NewIndex);
	ENGINE_API void ResetSnapshotBuffer();

	ENGINE_API void ResetNodeVisitSites();
	ENGINE_API void RecordNodeVisit(int32 TargetNodeIndex, int32 SourceNodeIndex, float BlendWeight);
	ENGINE_API void RecordNodeVisitArray(const TArray<FNodeVisit>& Nodes);
	ENGINE_API void RecordNodeAttribute(int32 TargetNodeIndex, int32 SourceNodeIndex, FName InAttribute);
	ENGINE_API void RecordNodeAttributeMaps(const TMap<int32, TArray<FAttributeRecord>>& InInputAttributes, const TMap<int32, TArray<FAttributeRecord>>& InOutputAttributes);
	ENGINE_API void RecordNodeSync(int32 InSourceNodeIndex, FName InSyncGroup);
	ENGINE_API void RecordNodeSyncsArray(const TMap<int32, FName>& InNodeSyncs);
	ENGINE_API void RecordStateData(int32 StateMachineIndex, int32 StateIndex, float Weight, float ElapsedTime);
	ENGINE_API void RecordNodeValue(int32 InNodeID, const FString& InText);
	ENGINE_API void RecordSequencePlayer(int32 InNodeID, float InPosition, float InLength, int32 InFrameCount);
	ENGINE_API void RecordBlendSpacePlayer(int32 InNodeID, const UBlendSpace* InBlendSpace, const FVector& InPosition, const FVector& InFilteredPosition);

	ENGINE_API void AddPoseWatch(int32 NodeID, UPoseWatchPoseElement* const InPoseWatchPoseElement);
	ENGINE_API void RemovePoseWatch(int32 NodeID);
	ENGINE_API void ForEachActiveVisiblePoseWatchPoseElement(const TFunctionRef<void(FAnimNodePoseWatch&)>& InFunction);
	ENGINE_API void DisableAllPoseWatches();

	ENGINE_API TArrayView<const FName> GetNodeAttributes(TWeakObjectPtr<UAnimGraphNode_Base> InAnimGraphNode) const;
#endif
};

// 'Marker' structure for mutable data. This is used as a base struct for mutable data to be inserted into by the anim
// 可变数据的“标记”结构。它用作动画插入的可变数据的基本结构
// BP compiler.
// BP编译器。
USTRUCT()
struct FAnimBlueprintMutableData
{
	GENERATED_BODY()
};

// 'Marker' structure for constant data. This is used as a base struct for constant data to be inserted into by the anim
// 常量数据的“标记”结构。它用作动画插入的常量数据的基本结构
	/** 此蓝图类的目标骨架 */
// BP compiler if there is no existing archetype sparse class data.
// BP编译器如果不存在现有的原型稀疏类数据。
USTRUCT()
struct FAnimBlueprintConstantData
	/** 动画列表通知状态机（或其他任何东西）可能引用 */
{
	GENERATED_BODY()
};

#if WITH_EDITORONLY_DATA
namespace EPropertySearchMode
{
	enum Type
	{
		OnlyThis,
		Hierarchy
	};
}
#endif

// Struct type generated by the anim BP compiler. Used for sparse class data and mutable data area.
// 由anim BP编译器生成的结构类型。用于稀疏类数据和可变数据区域。
// Only really needed to hide the struct from the content browser (via IsAsset override)
// 仅真正需要从内容浏览器中隐藏该结构（通过 IsAsset 覆盖）
UCLASS(MinimalAPI)
class UAnimBlueprintGeneratedStruct : public UScriptStruct
{
	GENERATED_BODY()

	// UObject interface
 // UObject接口
	virtual bool IsAsset() const override { return false; }
};

UCLASS(MinimalAPI)
class UAnimBlueprintGeneratedClass : public UBlueprintGeneratedClass, public IAnimClassInterface
{
	GENERATED_UCLASS_BODY()

	friend class FAnimBlueprintCompilerContext;
	friend class FAnimBlueprintGeneratedClassCompiledData;
	friend class FKismetDebugUtilities;
	friend class UAnimBlueprintExtension_Base;

	// List of state machines present in this blueprint class
 // 此蓝图类中存在的状态机列表
	UPROPERTY()
	TArray<FBakedAnimationStateMachine> BakedStateMachines;
	/** 此蓝图类的目标骨架 */

	/** Target skeleton for this blueprint class */
	/** 此蓝图类的目标骨架 */
	UPROPERTY(AssetRegistrySearchable)
	/** 动画列表通知状态机（或其他任何东西）可能引用 */
	TObjectPtr<USkeleton> TargetSkeleton;

	/** A list of anim notifies that state machines (or anything else) may reference */
	/** 动画列表通知状态机（或其他任何东西）可能引用 */
	UPROPERTY()
	TArray<FAnimNotifyEvent> AnimNotifies;

	// Indices for each of the saved pose nodes that require updating, in the order they need to get updates, per layer
 // 每层需要更新的每个保存的姿势节点的索引，按照它们需要获取更新的顺序
	UPROPERTY()
	TMap<FName, FCachedPoseIndices> OrderedSavedPoseIndicesMap;

	// The various anim functions that this class holds (created during GenerateAnimationBlueprintFunctions)
 // 该类拥有的各种动画函数（在GenerateAnimationBlueprintFunctions期间创建）
	TArray<FAnimBlueprintFunction> AnimBlueprintFunctions;

	// The arrays of anim nodes; this is transient generated data (created during Link)
 // 动画节点数组；这是瞬态生成的数据（在链接期间创建）
	TArray<FStructProperty*> AnimNodeProperties;
	TArray<FStructProperty*> LinkedAnimGraphNodeProperties;
	TArray<FStructProperty*> LinkedAnimLayerNodeProperties;
	TArray<FStructProperty*> PreUpdateNodeProperties;
	TArray<FStructProperty*> DynamicResetNodeProperties;
	TArray<FStructProperty*> StateMachineNodeProperties;
	TArray<FStructProperty*> InitializationNodeProperties;

	// Array of sync group names in the order that they are requested during compile
 // 同步组名称数组（按照编译期间请求的顺序排列）
	UPROPERTY()
	TArray<FName> SyncGroupNames;

#if WITH_EDITORONLY_DATA
	// Deprecated - moved to FAnimSubsystem_Base
 // 已弃用 - 移至 FAnimSubsystem_Base
	UPROPERTY()
	TArray<FExposedValueHandler> EvaluateGraphExposedInputs_DEPRECATED;
#endif

	// Indices for any Asset Player found within a specific (named) Anim Layer Graph, or implemented Anim Interface Graph
 // 在特定（命名）动画层图或实现的动画接口图中找到的任何资产播放器的索引
	UPROPERTY()
	TMap<FName, FGraphAssetPlayerInformation> GraphAssetPlayerInformation;

	// Per layer graph blending options
 // 每层图形混合选项
	UPROPERTY()
	TMap<FName, FAnimGraphBlendOptions> GraphBlendOptions;

private:
	// Constant/folded anim node data
 // 恒定/折叠动画节点数据
	UPROPERTY()
	TArray<FAnimNodeData> AnimNodeData;

	// Map from anim node struct to info about that struct (used to accelerate property name lookups)
 // 从动画节点结构映射到有关该结构的信息（用于加速属性名称查找）
	UPROPERTY()
	TMap<TObjectPtr<const UScriptStruct>, FAnimNodeStructData> NodeTypeMap;

	// Cached properties used to access 'folded' anim node properties
 // 用于访问“折叠”动画节点属性的缓存属性
	TArray<FProperty*> MutableProperties;
	TArray<FProperty*> ConstantProperties;

	// Cached properties used to access subsystem properties
 // 用于访问子系统属性的缓存属性
	TArray<FStructProperty*> ConstantSubsystemProperties;
	TArray<FStructProperty*> MutableSubsystemProperties;

	// Property for the object's mutable data area
 // 对象的可变数据区域的属性
	FStructProperty* MutableNodeDataProperty = nullptr;

	// Pointers to each subsystem, for easier debugging
 // 指向每个子系统的指针，以便于调试
	TArray<const FAnimSubsystem*> Subsystems;

#if WITH_EDITORONLY_DATA
	// Flag indicating the persistent result of calling VerifyNodeDataLayout() on load/compile
 // 指示加载/编译时调用VerifyNodeDataLayout()的持久结果的标志
	bool bDataLayoutValid = true;
#endif
	
public:
	// IAnimClassInterface interface
 // IAnimClassInterface 接口
	virtual const TArray<FBakedAnimationStateMachine>& GetBakedStateMachines() const override { return GetRootClass()->GetBakedStateMachines_Direct(); }
	virtual USkeleton* GetTargetSkeleton() const override { return TargetSkeleton; }
	virtual const TArray<FAnimNotifyEvent>& GetAnimNotifies() const override { return GetRootClass()->GetAnimNotifies_Direct(); }
	virtual const TArray<FStructProperty*>& GetAnimNodeProperties() const override { return AnimNodeProperties; }
	virtual const TArray<FStructProperty*>& GetLinkedAnimGraphNodeProperties() const override { return LinkedAnimGraphNodeProperties; }
	virtual const TArray<FStructProperty*>& GetLinkedAnimLayerNodeProperties() const override { return LinkedAnimLayerNodeProperties; }
	virtual const TArray<FStructProperty*>& GetPreUpdateNodeProperties() const override { return PreUpdateNodeProperties; }
	virtual const TArray<FStructProperty*>& GetDynamicResetNodeProperties() const override { return DynamicResetNodeProperties; }
	virtual const TArray<FStructProperty*>& GetStateMachineNodeProperties() const override { return StateMachineNodeProperties; }
	virtual const TArray<FStructProperty*>& GetInitializationNodeProperties() const override { return InitializationNodeProperties; }
	virtual const TArray<FName>& GetSyncGroupNames() const override { return GetRootClass()->GetSyncGroupNames_Direct(); }
	virtual const TMap<FName, FCachedPoseIndices>& GetOrderedSavedPoseNodeIndicesMap() const override { return GetRootClass()->GetOrderedSavedPoseNodeIndicesMap_Direct(); }
	virtual int32 GetSyncGroupIndex(FName SyncGroupName) const override { return GetSyncGroupNames().IndexOfByKey(SyncGroupName); }

	virtual const TArray<FAnimBlueprintFunction>& GetAnimBlueprintFunctions() const override { return AnimBlueprintFunctions; }
	virtual const TMap<FName, FGraphAssetPlayerInformation>& GetGraphAssetPlayerInformation() const override { return GetRootClass()->GetGraphAssetPlayerInformation_Direct(); }
	virtual const TMap<FName, FAnimGraphBlendOptions>& GetGraphBlendOptions() const override { return GetRootClass()->GetGraphBlendOptions_Direct(); }

private:
	virtual const TArray<FBakedAnimationStateMachine>& GetBakedStateMachines_Direct() const override { return BakedStateMachines; }
	virtual const TArray<FAnimNotifyEvent>& GetAnimNotifies_Direct() const override { return AnimNotifies; }
	virtual const TArray<FName>& GetSyncGroupNames_Direct() const override { return SyncGroupNames; }
	virtual const TMap<FName, FCachedPoseIndices>& GetOrderedSavedPoseNodeIndicesMap_Direct() const override { return OrderedSavedPoseIndicesMap; }
	virtual const TMap<FName, FGraphAssetPlayerInformation>& GetGraphAssetPlayerInformation_Direct() const override { return GraphAssetPlayerInformation; }
	virtual const TMap<FName, FAnimGraphBlendOptions>& GetGraphBlendOptions_Direct() const override { return GraphBlendOptions; }
	
private:
	ENGINE_API virtual const void* GetConstantNodeValueRaw(int32 InIndex) const override;
	ENGINE_API virtual const void* GetMutableNodeValueRaw(int32 InIndex, const UObject* InObject) const override;
	ENGINE_API virtual const FAnimBlueprintMutableData* GetMutableNodeData(const UObject* InObject) const override;
	ENGINE_API virtual FAnimBlueprintMutableData* GetMutableNodeData(UObject* InObject) const override;
	ENGINE_API virtual const void* GetConstantNodeData() const override;
	virtual TArrayView<const FAnimNodeData> GetNodeData() const override { return AnimNodeData; }

	ENGINE_API virtual int32 GetAnimNodePropertyIndex(const UScriptStruct* InNodeType, FName InPropertyName) const override;
	ENGINE_API virtual int32 GetAnimNodePropertyCount(const UScriptStruct* InNodeType) const override;
	
	ENGINE_API virtual void ForEachSubsystem(TFunctionRef<EAnimSubsystemEnumeration(const FAnimSubsystemContext&)> InFunction) const override;
	ENGINE_API virtual void ForEachSubsystem(UObject* InObject, TFunctionRef<EAnimSubsystemEnumeration(const FAnimSubsystemInstanceContext&)> InFunction) const override;
	ENGINE_API virtual const FAnimSubsystem* FindSubsystem(UScriptStruct* InSubsystemType) const override;

#if WITH_EDITORONLY_DATA
	virtual bool IsDataLayoutValid() const override { return bDataLayoutValid; };
#endif
	
	// Called internally post-load defaults and by the compiler after compilation is completed 
 // 在加载后默认值内部调用，并在编译完成后由编译器调用
	ENGINE_API void OnPostLoadDefaults(UObject* Object);

	// Called by the compiler to make sure that data tables are initialized. This is needed to patch the sparse class
 // 由编译器调用以确保数据表已初始化。这是修补稀疏类所必需的
	// data for child anim BP overrides 
 // 子动画 BP 覆盖的数据
	ENGINE_API void InitializeAnimNodeData(UObject* DefaultObject, bool bForce) const;

#if WITH_EDITORONLY_DATA
	// Verify that the serialized NodeTypeMap can be used with the current set of native node data layouts
 // 验证序列化的 NodeTypeMap 是否可以与当前的本机节点数据布局集一起使用
	// Sets internal bDataLayoutValid flag
 // 设置内部 bDataLayoutValid 标志
	ENGINE_API bool VerifyNodeDataLayout();
#endif

public:
#if WITH_EDITORONLY_DATA
	FAnimBlueprintDebugData AnimBlueprintDebugData;

	FAnimBlueprintDebugData& GetAnimBlueprintDebugData()
	{
		return AnimBlueprintDebugData;
	}

	template<typename StructType>
	const int32* GetNodePropertyIndexFromHierarchy(const UAnimGraphNode_Base* Node)
	{
		TArray<const UBlueprintGeneratedClass*> BlueprintHierarchy;
		GetGeneratedClassesHierarchy(this, BlueprintHierarchy);

		for (const UBlueprintGeneratedClass* Blueprint : BlueprintHierarchy)
		{
			if (const UAnimBlueprintGeneratedClass* AnimBlueprintClass = Cast<UAnimBlueprintGeneratedClass>(Blueprint))
			{
				const int32* SearchIndex = AnimBlueprintClass->AnimBlueprintDebugData.NodePropertyToIndexMap.Find(Node);
				if (SearchIndex)
				{
					return SearchIndex;
				}
			}

		}
		return NULL;
	}

	template<typename StructType>
	const int32* GetNodePropertyIndex(const UAnimGraphNode_Base* Node, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis)
	{
		return (SearchMode == EPropertySearchMode::OnlyThis) ? AnimBlueprintDebugData.NodePropertyToIndexMap.Find(Node) : GetNodePropertyIndexFromHierarchy<StructType>(Node);
	}

	template<typename StructType>
	int32 GetLinkIDForNode(const UAnimGraphNode_Base* Node, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis)
	{
		const int32* pIndex = GetNodePropertyIndex<StructType>(Node, SearchMode);
		if (pIndex)
		{
			return (AnimNodeProperties.Num() - 1 - *pIndex); //@TODO: Crazysauce
		}
		return -1;
	}

	template<typename StructType>
	FStructProperty* GetPropertyForNode(const UAnimGraphNode_Base* Node, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis)
	{
		const int32* pIndex = GetNodePropertyIndex<StructType>(Node, SearchMode);
		if (pIndex)
		{
			if (FStructProperty* AnimationProperty = AnimNodeProperties[AnimNodeProperties.Num() - 1 - *pIndex])
			{
				if (AnimationProperty->Struct->IsChildOf(StructType::StaticStruct()))
				{
					return AnimationProperty;
				}
			}
		}

		return NULL;
	}

	template<typename StructType>
	StructType* GetPropertyInstance(UObject* Object, const UAnimGraphNode_Base* Node, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis)
	{
		FStructProperty* AnimationProperty = GetPropertyForNode<StructType>(Node);
		if (AnimationProperty)
		{
			return AnimationProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
		}

		return NULL;
	}

	template<typename StructType>
	StructType* GetPropertyInstance(UObject* Object, FGuid NodeGuid, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis)
	{
		const int32* pIndex = GetNodePropertyIndexFromGuid(NodeGuid, SearchMode);
		if (pIndex)
		{
			if (FStructProperty* AnimProperty = AnimNodeProperties[AnimNodeProperties.Num() - 1 - *pIndex])
			{
				if (AnimProperty->Struct->IsChildOf(StructType::StaticStruct()))
				{
					return AnimProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
				}
			}
		}

		return NULL;
	}

	template<typename StructType>
	StructType& GetPropertyInstanceChecked(UObject* Object, const UAnimGraphNode_Base* Node, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis)
	{
		const int32 Index = AnimBlueprintDebugData.NodePropertyToIndexMap.FindChecked(Node);
		FStructProperty* AnimationProperty = AnimNodeProperties[AnimNodeProperties.Num() - 1 - Index];
		check(AnimationProperty);
		check(AnimationProperty->Struct->IsChildOf(StructType::StaticStruct()));
		return *AnimationProperty->ContainerPtrToValuePtr<StructType>((void*)Object);
	}

	// Gets the property index from the original UAnimGraphNode's GUID. Does not remap to property order.
 // 从原始 UAnimGraphNode 的 GUID 获取属性索引。不重新映射到属性顺序。
	ENGINE_API const int32* GetNodePropertyIndexFromGuid(FGuid Guid, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis);

	// Gets the remapped property index from the original UAnimGraphNode's GUID. Can be used to index the AnimNodeProperties array.
 // 从原始 UAnimGraphNode 的 GUID 获取重新映射的属性索引。可用于索引 AnimNodeProperties 数组。
	ENGINE_API int32 GetNodeIndexFromGuid(FGuid Guid, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis);

	ENGINE_API const UEdGraphNode* GetVisualNodeFromNodePropertyIndex(int32 PropertyIndex, EPropertySearchMode::Type SearchMode = EPropertySearchMode::OnlyThis) const;
#endif

	// Called after Link to patch up references to the nodes in the CDO
 // 在 Link 之后调用以修补对 CDO 中节点的引用
	ENGINE_API void LinkFunctionsToDefaultObjectNodes(UObject* DefaultObject);

	// Populates AnimBlueprintFunctions according to the UFunction(s) on this class
 // 根据此类上的 UFunction 填充 AnimBlueprintFunctions
	ENGINE_API void GenerateAnimationBlueprintFunctions();

	// Build the properties that we cache for our constant data
 // 构建我们为常量数据缓存的属性
	ENGINE_API void BuildConstantProperties();

	// Get the fixed names of our generated structs
 // 获取我们生成的结构的固定名称
	static ENGINE_API FName GetConstantsStructName();
	static ENGINE_API FName GetMutablesStructName();
	
#if WITH_EDITOR
	void HandleReinitializeObjectAfterCompile(UObject* Object) const override;
	ENGINE_API virtual void PrepareToConformSparseClassData(UScriptStruct* SparseClassDataArchetypeStruct) override;
	ENGINE_API virtual void ConformSparseClassData(UObject* Object) override;
#endif

	// UObject interface
 // UObject接口
	ENGINE_API virtual void Serialize(FArchive& Ar) override;
	// End of UObject interface
 // UObject接口结束

	// UStruct interface
 // US结构接口
	ENGINE_API virtual void Link(FArchive& Ar, bool bRelinkExistingProperties) override;
	// End of UStruct interface
 // UStruct 接口结束

	// UClass interface
 // U类接口
	ENGINE_API virtual void PurgeClass(bool bRecompilingOnLoad) override;
	ENGINE_API virtual uint8* GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const override;
	ENGINE_API virtual void PostLoadDefaultObject(UObject* Object) override;
	ENGINE_API virtual void PostLoad() override;
	// End of UClass interface
 // UClass接口结束
};

template<typename NodeType>
NodeType* GetNodeFromPropertyIndex(UObject* AnimInstanceObject, const IAnimClassInterface* AnimBlueprintClass, int32 PropertyIndex)
{
	if (PropertyIndex != INDEX_NONE)
	{
		FStructProperty* NodeProperty = AnimBlueprintClass->GetAnimNodeProperties()[AnimBlueprintClass->GetAnimNodeProperties().Num() - 1 - PropertyIndex]; //@TODO: Crazysauce
		check(NodeProperty->Struct == NodeType::StaticStruct());
		return NodeProperty->ContainerPtrToValuePtr<NodeType>(AnimInstanceObject);
	}

	return NULL;
}
