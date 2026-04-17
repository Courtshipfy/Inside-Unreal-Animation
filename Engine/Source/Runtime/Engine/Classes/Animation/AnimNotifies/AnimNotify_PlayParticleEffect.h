// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayParticleEffect.generated.h"

class UAnimSequenceBase;
class UParticleSystem;
class USkeletalMeshComponent;
class UParticleSystemComponent;

UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Play Particle Effect"), MinimalAPI)
class UAnimNotify_PlayParticleEffect : public UAnimNotify
{
	GENERATED_BODY()

public:

	ENGINE_API UAnimNotify_PlayParticleEffect();

	// Begin UObject interface
 // 开始UObject接口
	ENGINE_API virtual void PostLoad() override;
#if WITH_EDITOR
	ENGINE_API virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End UObject interface
 // 结束UObject接口

	// Begin UAnimNotify interface
 // 开始UAnimNotify接口
	ENGINE_API virtual FString GetNotifyName_Implementation() const override;

	UE_DEPRECATED(5.0, "Please use the other Notify function instead")
	ENGINE_API virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	ENGINE_API virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
#if WITH_EDITOR
	ENGINE_API virtual void ValidateAssociatedAssets() override;
#endif
	// End UAnimNotify interface
 // 结束UAnimNotify接口

	// Particle System to Spawn
 // 生成的粒子系统
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(DisplayName="Particle System"))
	TObjectPtr<UParticleSystem> PSTemplate;

	// Location offset from the socket
 // 距插座的位置偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify")
	FVector LocationOffset;

	// Rotation offset from socket
 // 距套筒的旋转偏移
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify")
	FRotator RotationOffset;

	// Scale to spawn the particle system at
 // 缩放以在以下位置生成粒子系统
	UPROPERTY(EditAnywhere, Category="AnimNotify")
	FVector Scale;

private:
	// Cached version of the Rotation Offset already in Quat form
 // 旋转偏移的缓存版本已采用 Quat 形式
	FQuat RotationOffsetQuat;

protected:
	// Spawns the ParticleSystemComponent. Called from Notify.
 // 生成 ParticleSystemComponent。从通知中调用。
	ENGINE_API virtual UParticleSystemComponent* SpawnParticleSystem(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);

public:

	// Should attach to the bone/socket
 // 应附着在骨头/插座上
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify")
	uint32 Attached:1; 	//~ Does not follow coding standard due to redirection from BP

	// SocketName to attach to
 // 要附加到的 SocketName
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FName SocketName;
};



