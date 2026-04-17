// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	This is a binding/unbinding functions for custom instances, specially for Sequencer tracks
	
	This is complicated because Sequencer Animation Track and ControlRig Tracks could be supported through this interface
	and it encapsulates lots of complications inside. 
	
	You can use one Animation Track - it's because this track doesn't take input from other pose, so this is always source
	You can use multiple ControlRig Tracks - this is because ControlRig could take inputs from other sources
	
	However this is not end of it. The way sequencer works is to allow you to add/remove anytime or any place. 
		
	So this behaves binding/unbinding depending on if you're source (Animation Track) or not (ControlRig). 
	
	If you want to be used by Animation Track, you should derive from ISequencerAnimationSupport and implement proper interfaces. 
	Now, you want to support layering, you'll have to support DoesSupportDifferentSourceAnimInstance to be true, and allow it to be used as source input. 
	
	1. ControlRigLayerInstance : this does support different source anim instance, and use it as a source of animation
	2. AnimSequencerInstance: this does not support different source anim instance, this acts as one.
	
	The code is to support, depending what role you have, you can be bound differently, so that you don't disturb what's currently available

=============================================================================*/ 


#pragma once
#include "Animation/AnimInstance.h"
#include "AnimSequencerInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "SequencerAnimationSupport.h"



class FAnimCustomInstanceHelper
{
public:
	/** 
	 * Called to bind a typed UAnimCustomInstance to an existing skeletal mesh component 
	 * @return the current (or newly created) UAnimCustomInstance
	 */
	template<typename InstanceClassType>
	static InstanceClassType* BindToSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent, bool& bOutWasCreated)
	{
		bOutWasCreated = false;
		// make sure to tick and refresh all the time when ticks
  // 确保勾选并在勾选时一直刷新
		// @TODO: this needs restoring post-binding
  // @TODO：这需要恢复绑定后
		InSkeletalMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
#if WITH_EDITOR
		InSkeletalMeshComponent->SetUpdateAnimationInEditor(true);
		InSkeletalMeshComponent->SetUpdateClothInEditor(true);
#endif
		TArray<USceneComponent*> ChildComponents;
		InSkeletalMeshComponent->GetChildrenComponents(true, ChildComponents);
		for (USceneComponent* ChildComponent : ChildComponents)
		{
			USkeletalMeshComponent* ChildSkelMeshComp = Cast<USkeletalMeshComponent>(ChildComponent);
			if (ChildSkelMeshComp)
			{
				ChildSkelMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
#if WITH_EDITOR
				ChildSkelMeshComp->SetUpdateAnimationInEditor(true);
				ChildSkelMeshComp->SetUpdateClothInEditor(true);
#endif
			}
		}
		// we use sequence instance if it's using anim blueprint that matches. Otherwise, we create sequence player
  // 如果序列实例使用匹配的动画蓝图，则我们使用序列实例。否则，我们创建序列播放器
		// this might need more check - i.e. making sure if it's same skeleton and so on, 
  // 这可能需要更多检查 - 即确保它是否是相同的骨架等等，
		// Ideally we could just call NeedToSpawnAnimScriptInstance call, which is protected now
  // 理想情况下，我们可以只调用 NeedToSpawnAnimScriptInstance 调用，该调用现在受到保护
		const bool bShouldCreateCustomInstance = ShouldCreateCustomInstancePlayer(InSkeletalMeshComponent);
		UAnimInstance* CurrentAnimInstance = InSkeletalMeshComponent->AnimScriptInstance;
		// See if we have SequencerInterface from current instance
  // 查看当前实例是否有 SequencerInterface
		ISequencerAnimationSupport* CurrentSequencerInterface = Cast<ISequencerAnimationSupport>(CurrentAnimInstance);
		const bool bCurrentlySequencerInterface = CurrentSequencerInterface != nullptr;
		const bool bCreateSequencerInterface = InstanceClassType::StaticClass()->ImplementsInterface(USequencerAnimationSupport::StaticClass());
		bool bSupportDifferentSourceAnimInstance = false;

		if (bCreateSequencerInterface)
		{
			InstanceClassType* DefaultObject = InstanceClassType::StaticClass()->template GetDefaultObject<InstanceClassType>();
			ISequencerAnimationSupport* SequencerSupporter = Cast<ISequencerAnimationSupport>(DefaultObject);
			bSupportDifferentSourceAnimInstance = SequencerSupporter && SequencerSupporter->DoesSupportDifferentSourceAnimInstance();
		}

		// if it should use sequence instance and current one doesn't support Sequencer Interface, we fall back to old behavior
  // 如果它应该使用序列实例并且当前实例不支持 Sequencer 接口，我们将回到旧的行为
		if (bShouldCreateCustomInstance && !bCurrentlySequencerInterface)
		{
			// this has to wrap around with this because we don't want to reinitialize everytime they come here
   // 这必须围绕这个，因为我们不想每次他们来到这里时都重新初始化
			// SetAnimationMode will reinitiaize it even if it's same, so make sure we just call SetAnimationMode if not AnimationCustomMode
   // 即使它是相同的，SetAnimationMode 也会重新初始化它，所以请确保我们只调用 SetAnimationMode（如果不是 AnimationCustomMode）
			if (InSkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationCustomMode)
			{
				InSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationCustomMode);
			}

			if (Cast<InstanceClassType>(InSkeletalMeshComponent->AnimScriptInstance) == nullptr || !InSkeletalMeshComponent->AnimScriptInstance->GetClass()->IsChildOf(InstanceClassType::StaticClass()))

			{
				InstanceClassType* SequencerInstance = NewObject<InstanceClassType>(InSkeletalMeshComponent, InstanceClassType::StaticClass());
				InSkeletalMeshComponent->AnimScriptInstance = SequencerInstance;
				InSkeletalMeshComponent->AnimScriptInstance->InitializeAnimation();
				bOutWasCreated = true;
				return SequencerInstance;
			}
			// if it's the same type it's expecting, returns the one
   // 如果它与期望的类型相同，则返回该类型
			else if (InSkeletalMeshComponent->AnimScriptInstance->GetClass()->IsChildOf(InstanceClassType::StaticClass()))
			{
				return Cast<InstanceClassType>(InSkeletalMeshComponent->AnimScriptInstance);
			}
		}
		else 
		{
			// if it's the same type it's expecting, returns the one
   // 如果它与期望的类型相同，则返回该类型
			if (CurrentAnimInstance->GetClass()->IsChildOf(InstanceClassType::StaticClass()))
			{
				return Cast<InstanceClassType>(InSkeletalMeshComponent->AnimScriptInstance);
			}
			// if currently it is sequencer interface, check to see if
   // 如果当前是sequencer接口，检查是否
			else if (bCurrentlySequencerInterface)
			{
				// this is a case where SequencerInstance is created later, currently it has SequencerInteface
    // 这是稍后创建SequencerInstance的情况，目前它有SequencerInteface
				UAnimInstance* CurSourceInstance = CurrentSequencerInterface->GetSourceAnimInstance();
				// if no source, create new one, and assign the new instance if current sequencer interface supports
    // 如果没有源，则创建新的源，并在当前定序器接口支持的情况下分配新实例
				if (CurSourceInstance == nullptr)
				{
					// if current doesn't have source instance and if it does support different source animation
     // 如果当前没有源实例并且它是否支持不同的源动画
					if (CurrentSequencerInterface->DoesSupportDifferentSourceAnimInstance())
					{
						// create new one requested, and set to new source
      // 创建新的请求，并设置为新的源
						InstanceClassType* NewSequencerInstance = NewObject<InstanceClassType>(InSkeletalMeshComponent, InstanceClassType::StaticClass());
						//if it's sequencer inteface, create one, and assign
      // 如果是音序器接口，则创建一个并分配
						NewSequencerInstance->InitializeAnimation();
						CurrentSequencerInterface->SetSourceAnimInstance(NewSequencerInstance);
						bOutWasCreated = true;
						return NewSequencerInstance;
					}
					else
					{
						UE_LOG(LogAnimation, Warning, TEXT("Currently Sequencer doesn't support Source Instance. They're not compatible to work together."));
					}
				}
				// if source is same as what's requested, just return this.
    // 如果源与请求的相同，则返回该源。
				else if (CurSourceInstance->GetClass()->IsChildOf(InstanceClassType::StaticClass()))
				{
					// nothing to do? 
     // 无事可做？
					// it has already same type
     // 它已经有相同的类型
					return Cast<InstanceClassType>(CurSourceInstance);
				}
				// if this doesn't support different source anim instances, but the new class does
    // 如果这不支持不同的源动画实例，但新类支持
				// see if we could switch it up
    // 看看我们是否可以切换它
				else if (bCreateSequencerInterface && bSupportDifferentSourceAnimInstance && !CurrentSequencerInterface->DoesSupportDifferentSourceAnimInstance())
				{
					InstanceClassType* NewInstance = NewObject<InstanceClassType>(InSkeletalMeshComponent, InstanceClassType::StaticClass());
					ISequencerAnimationSupport* NewSequencerInterface = Cast<ISequencerAnimationSupport>(NewInstance);
					check(NewSequencerInterface);
					if (NewSequencerInterface->DoesSupportDifferentSourceAnimInstance())
					{
						InSkeletalMeshComponent->AnimScriptInstance = NewInstance;
						ensureAlways(NewInstance != CurSourceInstance);
						NewSequencerInterface->SetSourceAnimInstance(CurSourceInstance);

						InSkeletalMeshComponent->AnimScriptInstance->InitializeAnimation();
						bOutWasCreated = true;
						return Cast<InstanceClassType>(InSkeletalMeshComponent->AnimScriptInstance);
					}
				}
			}
	/** 调用以取消 UAnimCustomInstance 与现有骨架网格物体组件的绑定 */
			// if requested one is support sequencer animation?
   // 如果要求的话，是否支持音序器动画？
			else if (bCreateSequencerInterface && bSupportDifferentSourceAnimInstance)
			{
				InstanceClassType* NewInstance = NewObject<InstanceClassType>(InSkeletalMeshComponent, InstanceClassType::StaticClass());
				ISequencerAnimationSupport* NewSequencerInterface = Cast<ISequencerAnimationSupport>(NewInstance);
				check(NewSequencerInterface);
				if (NewSequencerInterface->DoesSupportDifferentSourceAnimInstance())
				{
					InSkeletalMeshComponent->AnimScriptInstance = NewInstance;
					NewSequencerInterface->SetSourceAnimInstance(CurrentAnimInstance);

					InSkeletalMeshComponent->AnimScriptInstance->InitializeAnimation();
					bOutWasCreated = true;
					return Cast<InstanceClassType>(InSkeletalMeshComponent->AnimScriptInstance);
				}
			}
		}

		return nullptr;
	}

	/** 调用以取消 UAnimCustomInstance 与现有骨架网格物体组件的绑定 */
	/** Called to unbind a UAnimCustomInstance to an existing skeletal mesh component */
	/** 调用以取消 UAnimCustomInstance 与现有骨架网格物体组件的绑定 */
	template<typename InstanceClassType>
	static void UnbindFromSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
	{
#if WITH_EDITOR
		InSkeletalMeshComponent->SetUpdateAnimationInEditor(false);
		InSkeletalMeshComponent->SetUpdateClothInEditor(false);
#endif

		if (InSkeletalMeshComponent->GetAnimationMode() == EAnimationMode::Type::AnimationCustomMode)
		{
			UAnimInstance* AnimInstance = InSkeletalMeshComponent->GetAnimInstance();
			// if same type, we're fine
   // 如果类型相同，我们没问题
			InstanceClassType* SequencerInstance = Cast<InstanceClassType>(AnimInstance);
			if (SequencerInstance)
			{
				bool bClearAnimScriptInstance = true;

				if(ISequencerAnimationSupport* SequencerInterface = Cast<ISequencerAnimationSupport>(AnimInstance))
				{
					if (SequencerInterface->DoesSupportDifferentSourceAnimInstance())
					{
						UAnimInstance* SourceAnimInstance = SequencerInterface->GetSourceAnimInstance();
						// if we have source, replace with it
      // 如果我们有源，则替换为它
						if (SourceAnimInstance)
						{
							// clear before you remove
       // 删除之前先清除
							SequencerInterface->SetSourceAnimInstance(nullptr);
							InSkeletalMeshComponent->AnimScriptInstance = SourceAnimInstance;
							bClearAnimScriptInstance = false;
						}
					}
				}
				
				if(bClearAnimScriptInstance)
				{
					InSkeletalMeshComponent->ClearAnimScriptInstance();
				}
			}
			else // if not, we'd like to see if SequencerSupport
			{
				ISequencerAnimationSupport* SequencerInterface = Cast<ISequencerAnimationSupport>(AnimInstance);
				if (SequencerInterface && SequencerInterface->DoesSupportDifferentSourceAnimInstance())
				{
					UAnimInstance* SourceInstance = SequencerInterface->GetSourceAnimInstance();
					SequencerInstance = Cast<InstanceClassType>(SourceInstance);
					
					if (SequencerInstance)
					{
						SequencerInterface->SetSourceAnimInstance(nullptr);
					}
					// this can be animBP, we want to return that
     // 这可以是 animBP，我们想要返回它
					else if (SourceInstance)
					{
						SequencerInterface->SetSourceAnimInstance(nullptr);
						InSkeletalMeshComponent->AnimScriptInstance = SourceInstance;
					}
				}
			}
		}
		else if (InSkeletalMeshComponent->GetAnimationMode() == EAnimationMode::Type::AnimationBlueprint)
		{
			UAnimInstance* AnimInstance = InSkeletalMeshComponent->GetAnimInstance();
			ISequencerAnimationSupport* SequencerInterface = Cast<ISequencerAnimationSupport>(AnimInstance);
			if (SequencerInterface && SequencerInterface->DoesSupportDifferentSourceAnimInstance())
			{
				UAnimInstance* SourceInstance = SequencerInterface->GetSourceAnimInstance();
				if (SourceInstance)
				{
					SequencerInterface->SetSourceAnimInstance(nullptr);
					InSkeletalMeshComponent->AnimScriptInstance = SourceInstance;
					AnimInstance = SourceInstance;
				}
			}
	/** BindToSkeletalMeshComponent 的辅助函数 */
		
			if (AnimInstance)
			{
				const TArray<UAnimInstance*>& LinkedInstances = const_cast<const USkeletalMeshComponent*>(InSkeletalMeshComponent)->GetLinkedAnimInstances();
				for (UAnimInstance* LinkedInstance : LinkedInstances)
				{
					// Sub anim instances are always forced to do a parallel update 
     // 子动画实例总是被迫进行并行更新
					LinkedInstance->UpdateAnimation(0.0f, false, UAnimInstance::EUpdateAnimationFlag::ForceParallelUpdate);
				}

				AnimInstance->UpdateAnimation(0.0f, false);
			}

			// Update space bases to reset it back to ref pose
   // 更新空间基地以将其重置回参考姿势
			InSkeletalMeshComponent->RefreshBoneTransforms();
			InSkeletalMeshComponent->RefreshFollowerComponents();
			InSkeletalMeshComponent->UpdateComponentToWorld();
		}

		// if not game world, don't clean this up
  // 如果不是游戏世界，就不要清理它
		if (InSkeletalMeshComponent->GetWorld() != nullptr && InSkeletalMeshComponent->GetWorld()->IsGameWorld() == false)
		{
			InSkeletalMeshComponent->ClearMotionVector();
		}
	}
	/** BindToSkeletalMeshComponent 的辅助函数 */

private:
	/** Helper function for BindToSkeletalMeshComponent */
	/** BindToSkeletalMeshComponent 的辅助函数 */
	static ANIMGRAPHRUNTIME_API bool ShouldCreateCustomInstancePlayer(const USkeletalMeshComponent* SkeletalMeshComponent);
};
