// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimNotifies/AnimNotifyState_TimedParticleEffect.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_TimedParticleEffect)

UAnimNotifyState_TimedParticleEffect::UAnimNotifyState_TimedParticleEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PSTemplate = nullptr;
	LocationOffset.Set(0.0f, 0.0f, 0.0f);
	RotationOffset = FRotator(0.0f, 0.0f, 0.0f);
}

void UAnimNotifyState_TimedParticleEffect::NotifyBegin(USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float TotalDuration)
{

}


void UAnimNotifyState_TimedParticleEffect::NotifyBegin(USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	// ensure deprecated path is called because a call to Super is not made
 // 确保调用已弃用的路径，因为未调用 Super
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	NotifyBegin(MeshComp, Animation, TotalDuration);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	// Only spawn if we've got valid params
 // 仅当我们有有效参数时才生成
	if(ValidateParameters(MeshComp))
	{
		UParticleSystemComponent* NewComponent = UGameplayStatics::SpawnEmitterAttached(PSTemplate, MeshComp, SocketName, LocationOffset, RotationOffset, EAttachLocation::KeepRelativeOffset, !bDestroyAtEnd);
	}
	Received_NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void UAnimNotifyState_TimedParticleEffect::NotifyTick(USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float FrameDeltaTime)
{
}

void UAnimNotifyState_TimedParticleEffect::NotifyTick(USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	NotifyTick(MeshComp, Animation, FrameDeltaTime);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	Received_NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UAnimNotifyState_TimedParticleEffect::NotifyEnd(USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation)
{
}

void UAnimNotifyState_TimedParticleEffect::NotifyEnd(USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, const FAnimNotifyEventReference& EventReference)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	NotifyEnd(MeshComp, Animation);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	TArray<USceneComponent*> Children;
	MeshComp->GetChildrenComponents(false, Children);

	for(USceneComponent* Component : Children)
	{
		if(UParticleSystemComponent* ParticleComponent = Cast<UParticleSystemComponent>(Component))
		{
			bool bSocketMatch = ParticleComponent->GetAttachSocketName() == SocketName;
			bool bTemplateMatch = ParticleComponent->Template == PSTemplate;

#if WITH_EDITORONLY_DATA
			// In editor someone might have changed our parameters while we're ticking; so check 
   // 在编辑器中，有人可能在我们勾选时更改了我们的参数；所以检查一下
			// previous known parameters too.
   // 以前已知的参数也是如此。
			bSocketMatch |= PreviousSocketNames.Contains(ParticleComponent->GetAttachSocketName());
			bTemplateMatch |= PreviousPSTemplates.Contains(ParticleComponent->Template);
#endif

			if(bSocketMatch && bTemplateMatch && !ParticleComponent->bWasDeactivated)
			{
				// Either destroy the component or deactivate it to have it's active particles finish.
    // 要么破坏该组件，要么停用它，以完成其活动粒子。
				// The component will auto destroy once all particle are gone.
    // 一旦所有粒子消失，该组件将自动销毁。
				if(bDestroyAtEnd)
				{
					ParticleComponent->DestroyComponent();
				}
				else
				{
					ParticleComponent->DeactivateSystem();
				}

#if WITH_EDITORONLY_DATA
				// No longer need to track previous values as we've found our component
    // 不再需要跟踪以前的值，因为我们已经找到了我们的组件
				// and removed it.
    // 并将其删除。
				PreviousPSTemplates.Empty();
				PreviousSocketNames.Empty();
#endif
				// Removed a component, no need to continue
    // 删除了一个组件，无需继续
				break;
			}
		}
	}

	Received_NotifyEnd(MeshComp, Animation, EventReference);
}

bool UAnimNotifyState_TimedParticleEffect::ValidateParameters(USkeletalMeshComponent* MeshComp)
{
	bool bValid = true;

	if(!PSTemplate)
	{
		bValid = false;
	}
	else if(!MeshComp->DoesSocketExist(SocketName) && MeshComp->GetBoneIndex(SocketName) == INDEX_NONE)
	{
		bValid = false;
	}

	return bValid;
}

FString UAnimNotifyState_TimedParticleEffect::GetNotifyName_Implementation() const
{
	if(PSTemplate)
	{
		return PSTemplate->GetName();
	}

	return UAnimNotifyState::GetNotifyName_Implementation();
}

#if WITH_EDITOR
void UAnimNotifyState_TimedParticleEffect::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	if(PropertyAboutToChange)
	{
		if(PropertyAboutToChange->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UAnimNotifyState_TimedParticleEffect, PSTemplate) && PSTemplate != NULL)
		{
			PreviousPSTemplates.Add(PSTemplate);
		}

		if(PropertyAboutToChange->GetName() == GET_MEMBER_NAME_STRING_CHECKED(UAnimNotifyState_TimedParticleEffect, SocketName) && SocketName != FName(TEXT("None")))
		{
			PreviousSocketNames.Add(SocketName);
		}
	}
}
#endif

