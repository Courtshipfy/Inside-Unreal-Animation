// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimStats.h"
#include "AnimNode_CustomProperty.generated.h"

/** 
 * Custom property node that you'd like to expand pin by reflecting internal instance (we call TargetInstance here)
 * 
 *  Used by sub anim instance or control rig node 
 *	where you have internal instance and would like to reflect to AnimNode as a pin
 * 
 *  To make pin working, you need storage inside of AnimInstance (SourceProperties/SourcePropertyNames)
 *  So this creates storage inside of AnimInstance with the unique custom property name
 *	and it copies to the actually TargetInstance here to allow the information be transferred in runtime (DestProperties/DestPropertyNames)
 * 
 *  TargetInstance - UObject derived instance that has certain dest properties
 *  Source - AnimInstance's copy properties that is used to store the data 
 */
USTRUCT()
struct FAnimNode_CustomProperty : public FAnimNode_Base
{
	GENERATED_BODY()

public:

	ENGINE_API FAnimNode_CustomProperty();
	ENGINE_API FAnimNode_CustomProperty(const FAnimNode_CustomProperty&);
	ENGINE_API ~FAnimNode_CustomProperty();

	/* Set Target Instance */
	/* 设置目标实例 */
	/* 设置目标实例 */
	/* 设置目标实例 */
	ENGINE_API void SetTargetInstance(UObject* InInstance);
	/* 为了方便起见，按类型获取目标实例 */

	/* 为了方便起见，按类型获取目标实例 */
	/* Get Target Instance by type for convenience */
	/* 为了方便起见，按类型获取目标实例 */
	template<class T>
	T* GetTargetInstance() const
	{
		if (IsValid(TargetInstance))
		{
			return Cast<T>(TargetInstance);
		}

		return nullptr;
	}

	// We only subscribe to the OnInitializeAnimInstance path because we need to cache our source object, so we only
 // 我们只订阅OnInitializeAnimInstance路径，因为我们需要缓存我们的源对象，所以我们只
	// override these methods in editor at the moment
 // 目前在编辑器中重写这些方法
#if WITH_EDITOR	
	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ENGINE_API virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }

	// Handle object reinstancing in editor
 // 在编辑器中处理对象重新实例化
	/** 要使用的源属性列表，1-1 以及下面的目标名称，由编译器构建 */
	/** 要使用的源属性列表，1-1 以及下面的目标名称，由编译器构建 */
	ENGINE_API void HandleObjectsReinstanced(const TMap<UObject*, UObject*>& OldToNewInstanceMap);
#endif

	/** 要使用的目标属性列表，1-1 上面带有源名称，由编译器构建 */
	/** 要使用的目标属性列表，1-1 上面带有源名称，由编译器构建 */
protected:
	/** List of source properties to use, 1-1 with Dest names below, built by the compiler */
	/** 要使用的源属性列表，1-1 以及下面的目标名称，由编译器构建 */
	UPROPERTY(meta=(BlueprintCompilerGeneratedDefaults))
	/** 这是在运行时分配的将运行的实际实例。由子班设置。 */
	/** 这是在运行时分配的将运行的实际实例。由子班设置。 */
	TArray<FName> SourcePropertyNames;

	/** List of destination properties to use, 1-1 with Source names above, built by the compiler */
	/** 要从中推送的调用源实例实例上的属性列表  */
	/** 要使用的目标属性列表，1-1 上面带有源名称，由编译器构建 */
	/** 要从中推送的调用源实例实例上的属性列表  */
	UPROPERTY(meta=(BlueprintCompilerGeneratedDefaults))
	/** TargetInstance 上要推送到的属性列表，初始化时根据名称列表构建 */
	TArray<FName> DestPropertyNames;

	/** TargetInstance 上要推送到的属性列表，初始化时根据名称列表构建 */
	/** This is the actual instance allocated at runtime that will run. Set by child class. */
	/** 这是在运行时分配的将运行的实际实例。由子班设置。 */
	UPROPERTY(Transient)
	TObjectPtr<UObject> TargetInstance;
	/* 将源实例的属性传播到目标实例*/

	/** List of properties on the calling Source Instances instance to push from  */
	/** 要从中推送的调用源实例实例上的属性列表  */
	/** 获取目标类别 */
	/* 将源实例的属性传播到目标实例*/
	TArray<FProperty*> SourceProperties;

	/** List of properties on the TargetInstance to push to, built from name list when initialised */
	/** 获取目标类别 */
	/** TargetInstance 上要推送到的属性列表，初始化时根据名称列表构建 */
	TArray<FProperty*> DestProperties;
	
	/* Initialize property links from the source instance, in this case AnimInstance 
	 * Compiler creates those properties during compile time */
	ENGINE_API virtual void InitializeProperties(const UObject* InSourceInstance, UClass* InTargetClass);

	/* Propagate the Source Instances' properties to Target Instance*/
	/* 将源实例的属性传播到目标实例*/
	ENGINE_API virtual void PropagateInputProperties(const UObject* InSourceInstance);
	/** 这是源实例，已缓存以帮助重新实例化 */

	/** Get Target Class */
	/** 获取目标类别 */
	virtual UClass* GetTargetClass() const PURE_VIRTUAL(FAnimNode_CustomProperty::GetTargetClass, return nullptr;);

	/** 这是源实例，已缓存以帮助重新实例化 */
#if WITH_EDITOR
	/**
	 * Handle object reinstancing override point.
	 * When objects are replaced in editor, the FCoreUObjectDelegates::OnObjectsReplaced is called before reference
	 * replacement, so we cannot handle replacement until later. This call is made on the first PreUpdate after object
	 * replacement.
	 */
	ENGINE_API virtual void HandleObjectsReinstanced_Impl(UObject* InSourceObject, UObject* InTargetObject, const TMap<UObject*, UObject*>& OldToNewInstanceMap);
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
protected:
	/** This is the source instance, cached to help with re-instancing */
	/** 这是源实例，已缓存以帮助重新实例化 */
	UPROPERTY(Transient)
	TObjectPtr<UObject> SourceInstance;
#endif

	// Stats
 // 统计数据
#if ANIMNODE_STATS_VERBOSE
	// Cached StatID for this node
 // 该节点的缓存 StatID
	TStatId StatID;
	virtual void InitializeStatID() { StatID = FDynamicStats::CreateStatId<FStatGroup_STATGROUP_Anim>(FString(TEXT("Unknown"))); }
#endif // ANIMNODE_STATS_VERBOSE

	friend class UAnimGraphNode_CustomProperty;
	friend class UAnimInstance;
	friend class UControlRigLayerInstance;
};

template<>
struct TStructOpsTypeTraits<FAnimNode_CustomProperty> : public TStructOpsTypeTraitsBase2<FAnimNode_CustomProperty>
{
	enum
	{
		WithPureVirtual = true,
	};
};
