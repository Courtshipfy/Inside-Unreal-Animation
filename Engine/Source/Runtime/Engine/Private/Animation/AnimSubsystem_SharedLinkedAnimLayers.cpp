// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimSubsystem_SharedLinkedAnimLayers.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "ObjectTrace.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimSubsystem_SharedLinkedAnimLayers)

namespace SharedLinkedAnimLayersConsoleCommands
{
	static int32 MarkLayerAsGarbageOnUninitialize = 0;
	static FAutoConsoleVariableRef CVarMarkLayerAsGarbageOnUninitialize(
		TEXT("a.MarkLayerAsGarbageOnUninitialize"), MarkLayerAsGarbageOnUninitialize,
		TEXT("Whether to mark the layers as garbage after uinitializing them."),
		ECVF_Default);
}

#if LINKEDANIMLAYERSDATA_INTEGRITYCHECKS
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimNode_LinkedAnimLayer.h"

namespace
{
	// Check that function is either linked (!bIsFree) or unlinked (bIsFree)
	// 检查函数是否已链接 (!bIsFree) 或未链接 (bIsFree)
	void CheckLayerDataIntegrity(const UAnimInstance* AnimInstance, FName FunctionName, bool bIsFree)
	{
		bool bFound = false;

		TArray<const UAnimInstance*> AllAnimInstances;
		const USkeletalMeshComponent* SkelMesh = AnimInstance->GetSkelMeshComponent();
		AllAnimInstances.Add(SkelMesh->GetAnimInstance());
		AllAnimInstances.Append(SkelMesh->GetLinkedAnimInstances());
		for (const UAnimInstance* LinkedAnimInstance : AllAnimInstances)
		{
			const IAnimClassInterface* const NewLinkedInstanceClass = IAnimClassInterface::GetFromClass(LinkedAnimInstance->GetClass());
			for (const FStructProperty* const LayerNodeProperty : NewLinkedInstanceClass->GetLinkedAnimLayerNodeProperties())
			{
				const FAnimNode_LinkedAnimLayer* const LinkedAnimLayerNode = LayerNodeProperty->ContainerPtrToValuePtr<const FAnimNode_LinkedAnimLayer>(LinkedAnimInstance);
				if (LinkedAnimLayerNode->GetDynamicLinkFunctionName() == FunctionName)
				{
					if (LinkedAnimLayerNode->GetTargetInstance<const UAnimInstance>() == AnimInstance)
					{
						// Function should be free to link but isn't
						// [翻译失败: Function should be free to link but isn't]
						if (bIsFree)
						{
							check(0);
						}
						else
						{
							// Function shouldn't be linked more than once
							// 函数不应链接多次
							check(!bFound);
							bFound = true;
						}
					}
				}
			}
		}
		// Function is either found or free
		// 函数已找到或已释放
		check(bIsFree || bFound);
	}
}

#endif // LINKEDANIMLAYERSDATA_INTEGRITYCHECKS

void FLinkedAnimLayerInstanceData::AddLinkedFunction(FName Function, UAnimInstance* AnimInstance)
{
#if LINKEDANIMLAYERSDATA_INTEGRITYCHECKS
	CheckLayerDataIntegrity(Instance, Function, true);
#endif
	check(!LinkedFunctions.Contains(Function));
	LinkedFunctions.Add(Function, AnimInstance);
}

void FLinkedAnimLayerInstanceData::RemoveLinkedFunction(FName Function)
{
#if LINKEDANIMLAYERSDATA_INTEGRITYCHECKS
	CheckLayerDataIntegrity(Instance, Function, true);
#endif
	check(LinkedFunctions.Contains(Function));
	LinkedFunctions.Remove(Function);
}

FLinkedAnimLayerInstanceData* FLinkedAnimLayerClassData::FindInstanceData(const UAnimInstance* AnimInstance)
{
	return InstancesData.FindByPredicate([AnimInstance](FLinkedAnimLayerInstanceData& InstanceData) {return InstanceData.Instance == AnimInstance; });
}

UAnimInstance* FLinkedAnimLayerClassData::FindOrAddInstanceForLinking(UAnimInstance* OwningInstance, FName Function, bool& bIsNewInstance)
{
	USkeletalMeshComponent* Mesh = OwningInstance->GetSkelMeshComponent();

	for (FLinkedAnimLayerInstanceData& LayerInstanceData : InstancesData)
	{
		// Check if function is already linked
		// 检查函数是否已经链接
		if (!LayerInstanceData.GetLinkedFunctions().Contains(Function))
		{
			// Re-add persistent instance of first function re-bind
			// 重新添加第一个函数重新绑定的持久实例
			if (LayerInstanceData.IsPersistent() && LayerInstanceData.GetLinkedFunctions().Num() == 0)
			{
				// Make sure the bones to update are up to date with LOD changes / bone visibility / cosmetics, etc.
				// 确保要更新的骨骼是最新的，包括 LOD 更改/骨骼可见性/外观等。
				LayerInstanceData.Instance->RecalcRequiredBones();
				check(!Mesh->GetLinkedAnimInstances().Contains(LayerInstanceData.Instance));
				Mesh->GetLinkedAnimInstances().Add(LayerInstanceData.Instance);
			}

			bIsNewInstance = false;
			LayerInstanceData.AddLinkedFunction(Function, LayerInstanceData.Instance);
			// not linked, use this instance
			// 未链接，使用此实例
			return LayerInstanceData.Instance;
		}
	}

	// Create a new object
	// 创建一个新对象
	bIsNewInstance = true;
	UAnimInstance* NewAnimInstance = NewObject<UAnimInstance>(Mesh, Class);
	NewAnimInstance->bCreatedByLinkedAnimGraph = true;
	NewAnimInstance->InitializeAnimation();

	if(Mesh->HasBegunPlay())
	{
		NewAnimInstance->NativeBeginPlay();
		NewAnimInstance->BlueprintBeginPlay();
	}
	
	FLinkedAnimLayerInstanceData& NewInstanceData = AddInstance(NewAnimInstance);
	NewInstanceData.AddLinkedFunction(Function, NewAnimInstance);
	OwningInstance->GetSkelMeshComponent()->GetLinkedAnimInstances().Add(NewAnimInstance);

	return NewAnimInstance;
}

FLinkedAnimLayerInstanceData& FLinkedAnimLayerClassData::AddInstance(UAnimInstance* AnimInstance)
{
	// First instance we create for persistent layer is marked has persistent
	// 我们为持久层创建的第一个实例被标记为具有持久性
	InstancesData.Push(FLinkedAnimLayerInstanceData(AnimInstance, bIsPersistent && InstancesData.Num() == 0));
	return InstancesData.Last();
}

void FLinkedAnimLayerClassData::RemoveLinkedFunction(UAnimInstance* AnimInstance, FName Function)
{
	if (FLinkedAnimLayerInstanceData* InstanceData = FindInstanceData(AnimInstance))
	{
		InstanceData->RemoveLinkedFunction(Function);
		if (InstanceData->GetLinkedFunctions().Num() == 0)
		{
			if (USkeletalMeshComponent* Mesh = InstanceData->Instance->GetSkelMeshComponent())
			{
				Mesh->GetLinkedAnimInstances().Remove(AnimInstance);
			}
			if (!InstanceData->IsPersistent())
			{
				RemoveInstance(AnimInstance);
			}
		}
	}
}

void FLinkedAnimLayerClassData::RemoveInstance(UAnimInstance* AnimInstance)
{
	for (int i = 0; i < InstancesData.Num(); ++i)
	{
		if (InstancesData[i].Instance == AnimInstance)
		{
			const FLinkedAnimLayerInstanceData& InstanceData = InstancesData[i];

			// If we have no function linked, the instance should never be part of the skeletal mesh component
			// 如果我们没有链接函数，则实例永远不应该成为骨架网格物体组件的一部分
			check(!InstanceData.Instance->GetSkelMeshComponent() || !InstanceData.Instance->GetSkelMeshComponent()->GetLinkedAnimInstances().Contains(InstanceData.Instance));
			// Make sure no functions are still linked
			// 确保没有任何功能仍然链接
			check(InstanceData.GetLinkedFunctions().Num() == 0);
			check(!InstanceData.IsPersistent());

			// Since UninitializeAnimation can make calls to the shared layer system via self layer nodes, cleanup before Uninitializing to prevent unnecessary checks
			// 由于UninitializeAnimation可以通过自层节点调用共享层系统，因此在Uninitializing之前进行清理以防止不必要的检查
			InstancesData.RemoveAt(i);

			AnimInstance->UninitializeAnimation();
			TRACE_OBJECT_LIFETIME_END(AnimInstance);
			if (SharedLinkedAnimLayersConsoleCommands::MarkLayerAsGarbageOnUninitialize)
			{
				AnimInstance->MarkAsGarbage();
			}
			return;
		}
	}
	check(0);
}

void FLinkedAnimLayerClassData::SetPersistence(bool bInIsPersistent)
{
	if (bInIsPersistent != IsPersistent())
	{
		bIsPersistent = bInIsPersistent;
		// If persistence is added we mark the current linked instance, if there is one, as persistent
		// 如果添加了持久性，我们将当前链接的实例（如果有）标记为持久性
		if (bIsPersistent)
		{
			if (InstancesData.Num())
			{
				InstancesData[0].SetPersistence(true);
			}
		}
		// If we remove persistence however make sure any currently unlinked instance is correctly deleted
		// 但是，如果我们删除持久性，请确保正确删除任何当前未链接的实例
		else // if (!bIsPersistent)
		{
			int32 InstanceIndex = InstancesData.IndexOfByPredicate([](const FLinkedAnimLayerInstanceData& InstanceData) {return InstanceData.IsPersistent(); });
			if (InstanceIndex != INDEX_NONE)
			{
				FLinkedAnimLayerInstanceData& InstanceData = InstancesData[InstanceIndex];
				InstanceData.SetPersistence(false);
				if (InstanceData.GetLinkedFunctions().Num() == 0)
				{
					RemoveInstance(InstanceData.Instance);
				}
			}
		}
	}
}
FAnimSubsystem_SharedLinkedAnimLayers* FAnimSubsystem_SharedLinkedAnimLayers::GetFromMesh(USkeletalMeshComponent* SkelMesh)
{
#if WITH_EDITOR
	if (GIsReinstancing)
	{
		return nullptr;
	}
#endif
	check(SkelMesh);

	// In some cases we have a PostProcessAnimInstance but no AnimScriptInstance
	// 在某些情况下，我们有 PostProcessAnimInstance 但没有 AnimScriptInstance
	if (SkelMesh->GetAnimInstance())
	{
		return SkelMesh->GetAnimInstance()->FindSubsystem<FAnimSubsystem_SharedLinkedAnimLayers>();
	}
	return nullptr;
}

void FAnimSubsystem_SharedLinkedAnimLayers::Reset()
{
	PersistentClasses.Empty();
	ClassesData.Empty(ClassesData.Num());
}

FLinkedAnimLayerInstanceData* FAnimSubsystem_SharedLinkedAnimLayers::FindInstanceData(const UAnimInstance* AnimInstance)
{
	if (FLinkedAnimLayerClassData* ClassData = FindClassData(AnimInstance->GetClass()))
	{
		return ClassData->FindInstanceData(AnimInstance);
	}
	return nullptr;
}

FLinkedAnimLayerClassData* FAnimSubsystem_SharedLinkedAnimLayers::FindClassData(TSubclassOf<UAnimInstance> AnimClass) 
{
	return ClassesData.FindByPredicate([AnimClass](const FLinkedAnimLayerClassData& ClassData) {return ClassData.GetClass() == AnimClass; });
}

FLinkedAnimLayerClassData& FAnimSubsystem_SharedLinkedAnimLayers::FindOrAddClassData(TSubclassOf<UAnimInstance> AnimClass)
{
	if (FLinkedAnimLayerClassData* Result = FindClassData(AnimClass))
	{
		return *Result;
	}
	bool bIsPersistent = PersistentClasses.Find(AnimClass) != INDEX_NONE;
	ClassesData.Add(FLinkedAnimLayerClassData(AnimClass, bIsPersistent));
	return ClassesData.Last();
};

void FAnimSubsystem_SharedLinkedAnimLayers::RemovePersistentAnimLayerClass(TSubclassOf<UAnimInstance> AnimInstanceClass)
{
	// When a class loses its persistency, make sure to clean it up if it's already unlinked
	// 当一个类失去持久性时，如果它已经取消链接，请确保清理它
	int32 ClassIndex = ClassesData.IndexOfByPredicate([AnimInstanceClass](const FLinkedAnimLayerClassData& ClassData) {return ClassData.GetClass() == AnimInstanceClass; });
	if (ClassIndex != INDEX_NONE)
	{
		FLinkedAnimLayerClassData& ClassData = ClassesData[ClassIndex];
		ClassData.SetPersistence(false);
	}
	PersistentClasses.Remove(AnimInstanceClass);

}

UAnimInstance* FAnimSubsystem_SharedLinkedAnimLayers::AddLinkedFunction(UAnimInstance* OwningInstance, TSubclassOf<UAnimInstance> AnimClass, FName Function, bool& bIsNewInstance)
{
	FLinkedAnimLayerClassData& ClassData = FindOrAddClassData(AnimClass);
	return ClassData.FindOrAddInstanceForLinking(OwningInstance, Function, bIsNewInstance);
}

void FAnimSubsystem_SharedLinkedAnimLayers::RemoveLinkedFunction(UAnimInstance* AnimInstance, FName Function)
{
	int32 ClassIndex = ClassesData.IndexOfByPredicate([AnimInstance](const FLinkedAnimLayerClassData& ClassData) {return ClassData.GetClass() == AnimInstance->GetClass(); });
	if (ClassIndex != INDEX_NONE)
	{
		FLinkedAnimLayerClassData& ClassData = ClassesData[ClassIndex];
		ClassData.RemoveLinkedFunction(AnimInstance, Function);
		if (ClassData.GetInstancesData().Num() == 0)
		{
			ClassesData.RemoveAt(ClassIndex);
		}
	}
}
