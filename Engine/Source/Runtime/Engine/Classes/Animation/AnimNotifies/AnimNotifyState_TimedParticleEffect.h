// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_TimedParticleEffect.generated.h"

class UParticleSystem;
class USkeletalMeshComponent;

// Timed Particle Effect Notify
// 定时粒子效果通知
// Allows a looping particle effect to be played in an animation that will activate
// 允许在激活的动画中播放循环粒子效果
// at the beginning of the notify and deactivate at the end.
// 在通知开始时并在结束时停用。
UCLASS(Blueprintable, meta = (DisplayName = "Timed Particle Effect"), MinimalAPI)
class UAnimNotifyState_TimedParticleEffect : public UAnimNotifyState
{
	GENERATED_UCLASS_BODY()

	// The particle system template to use when spawning the particle component
	// 生成粒子组件时使用的粒子系统模板
	UPROPERTY(EditAnywhere, Category = ParticleSystem, meta = (ToolTip = "The particle system to spawn for the notify state"))
	TObjectPtr<UParticleSystem> PSTemplate;

	// The socket within our mesh component to attach to when we spawn the particle component
	// 当我们生成粒子组件时要连接到的网格组件内的套接字
	UPROPERTY(EditAnywhere, Category = ParticleSystem, meta = (ToolTip = "The socket or bone to attach the system to"))
	FName SocketName;

	// Offset from the socket / bone location
	// 距插槽/骨骼位置的偏移
	UPROPERTY(EditAnywhere, Category = ParticleSystem, meta = (ToolTip = "Offset from the socket or bone to place the particle system"))
	FVector LocationOffset;

	// Offset from the socket / bone rotation
	// 距插槽/骨骼旋转的偏移
	UPROPERTY(EditAnywhere, Category = ParticleSystem, meta = (ToolTip = "Rotation offset from the socket or bone for the particle system"))
	FRotator RotationOffset;

	// Whether or not we destroy the component at the end of the notify or instead just stop
	// 我们是否在通知结束时销毁组件或者只是停止
	// the emitters.
	// 发射器。
	UPROPERTY(EditAnywhere, Category = ParticleSystem, meta = (DisplayName = "Destroy Immediately", ToolTip = "Whether the particle system should be immediately destroyed at the end of the notify state or be allowed to finish"))
	bool bDestroyAtEnd;

#if WITH_EDITORONLY_DATA
	// The following arrays are used to handle property changes during a state. Because we can't
	// 以下数组用于处理状态期间的属性更改。因为我们不能
	// store any stateful data here we can't know which emitter is ours. The best metric we have
	// 在这里存储任何状态数据，我们无法知道哪个发射器是我们的。我们拥有的最佳指标
	// is an emitter on our Mesh Component with the same template and socket name we have defined.
	// 是网格组件上的发射器，具有与我们定义的模板和套接字名称相同的名称。
	// Because these can change at any time we need to track previous versions when we are in an
	// 因为这些可能随时改变，所以当我们处于一个新的环境时，我们需要跟踪以前的版本。
	// editor build. Refactor when stateful data is possible, tracking our component instead.
	// 编辑器构建。当有状态数据可能时进行重构，而是跟踪我们的组件。
	UPROPERTY(transient)
	TArray<TObjectPtr<UParticleSystem>> PreviousPSTemplates;

	UPROPERTY(transient)
	TArray<FName> PreviousSocketNames;
	
#endif

#if WITH_EDITOR
	ENGINE_API virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
#endif

	UE_DEPRECATED(5.0, "Please use the other NotifyBegin function instead")
	ENGINE_API virtual void NotifyBegin(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float TotalDuration) override;
	UE_DEPRECATED(5.0, "Please use the other NotifyTick function instead")
	ENGINE_API virtual void NotifyTick(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float FrameDeltaTime) override;
	UE_DEPRECATED(5.0, "Please use the other NotifyEnd function instead")
	ENGINE_API virtual void NotifyEnd(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation) override;
	
	ENGINE_API virtual void NotifyBegin(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	ENGINE_API virtual void NotifyTick(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	ENGINE_API virtual void NotifyEnd(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, const FAnimNotifyEventReference& EventReference) override;

	// Overridden from UAnimNotifyState to provide custom notify name.
	// 从 UAnimNotifyState 重写以提供自定义通知名称。
	ENGINE_API FString GetNotifyName_Implementation() const override;

protected:
	ENGINE_API bool ValidateParameters(USkeletalMeshComponent* MeshComp);
};
