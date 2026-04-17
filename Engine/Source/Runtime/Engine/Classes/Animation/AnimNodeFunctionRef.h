// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNodeFunctionRef.generated.h"

struct FAnimNode_StateMachine;
struct FPoseLink;
struct FPoseLinkBase;
struct FComponentSpacePoseLink;
struct FAnimNode_Base;
struct FAnimationBaseContext;
struct FAnimationInitializeContext;
struct FAnimationUpdateContext;
struct FPoseContext;
struct FComponentSpacePoseContext;
struct FAnimInstanceProxy;

/**
 * Cached function name/ptr that is resolved at init time
 */
USTRUCT()
struct FAnimNodeFunctionRef
{
	GENERATED_BODY()

public:
	// Cache the function ptr from the name
	// 从名称中缓存函数 ptr
	ENGINE_API void Initialize(const UClass* InClass);
	
	// Call the function
	// 调用函数
	ENGINE_API void Call(UObject* InObject, void* InParameters = nullptr) const;

	// Set the function via name
	// 通过名称设置功能
	void SetFromFunctionName(FName InName) { FunctionName = InName; ClassName = NAME_None; }

	// Set the function via a function
	// 通过函数设置函数
	ENGINE_API void SetFromFunction(UFunction* InFunction);
	
	// Get the function name
	// 获取函数名
	FName GetFunctionName() const { return FunctionName; }
	
	// Get the function we reference
	// 获取我们引用的函数
	UFunction* GetFunction() const { return Function; }
	
	// Check if we reference a valid function
	// 检查我们是否引用了有效的函数
	bool IsValid() const { return Function != nullptr; }

	// Override operator== as we only need to compare class/function names
	// 覆盖运算符==，因为我们只需要比较类/函数名称
	bool operator==(const FAnimNodeFunctionRef& InOther) const
	{
		return ClassName == InOther.ClassName && FunctionName == InOther.FunctionName;
	}
	
private:
	// The name of the class to call the function with. If this is NAME_None, we assume this is a 'thiscall', if it is valid then we assume (and verify) we should call the function on a function library CDO.
	// 用于调用函数的类的名称。如果这是 NAME_None，我们假设这是一个“thiscall”，如果它有效，那么我们假设（并验证）我们应该在函数库 CDO 上调用该函数。
	UPROPERTY()
	FName ClassName = NAME_None;

	// The name of the function to call
	// 要调用的函数的名称
	UPROPERTY()
	FName FunctionName = NAME_None;	

	// The class to use to call the function with, recovered by looking for a class of name FunctionName
	// 用于调用函数的类，通过查找名称为 FunctionName 的类来恢复
	UPROPERTY(Transient)
	TObjectPtr<const UClass> Class = nullptr;
	
	// The function to call, recovered by looking for a function of name FunctionName
	// 要调用的函数，通过查找名称为 FunctionName 的函数来恢复
	UPROPERTY(Transient)
	TObjectPtr<UFunction> Function = nullptr;
};

template<>
struct TStructOpsTypeTraits<FAnimNodeFunctionRef> : public TStructOpsTypeTraitsBase2<FAnimNodeFunctionRef>
{
	enum
	{
		WithIdenticalViaEquality = true,
	};
};

namespace UE { namespace Anim {

// Wrapper used to call anim node functions
// 用于调用动画节点函数的包装器
struct FNodeFunctionCaller
{
private:
	friend struct ::FPoseLinkBase;
	friend struct ::FPoseLink;
	friend struct ::FComponentSpacePoseLink;
	friend struct ::FAnimInstanceProxy;
	friend struct ::FAnimNode_StateMachine;
	
	// Call the InitialUpdate function of this node
	// 调用该节点的InitialUpdate函数
	static void InitialUpdate(const FAnimationUpdateContext& InContext, FAnimNode_Base& InNode);

	// Call the BecomeRelevant function of this node
	// [翻译失败: Call the BecomeRelevant function of this node]
	static void BecomeRelevant(const FAnimationUpdateContext& InContext, FAnimNode_Base& InNode);

	// Call the Update function of this node
	// 调用该节点的Update函数
	static void Update(const FAnimationUpdateContext& InContext, FAnimNode_Base& InNode);

public:
	// Call a generic function for this node
	// 为此节点调用通用函数
	ENGINE_API static void CallFunction(const FAnimNodeFunctionRef& InFunction, const FAnimationBaseContext& InContext, FAnimNode_Base& InNode);
};

}}
