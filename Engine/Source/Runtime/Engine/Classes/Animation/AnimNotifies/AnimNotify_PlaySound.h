// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlaySound.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
class USoundBase;

UCLASS(const, hidecategories=Object, collapsecategories, Config = Game, meta=(DisplayName="Play Sound"), MinimalAPI)
class UAnimNotify_PlaySound : public UAnimNotify
{
	GENERATED_BODY()

public:

	ENGINE_API UAnimNotify_PlaySound();

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

	// Sound to Play
	// 播放声音
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	TObjectPtr<USoundBase> Sound;

	// Volume Multiplier
	// 音量倍增器
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	float VolumeMultiplier;

	// Pitch Multiplier
	// 音调乘数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(ExposeOnSpawn = true))
	float PitchMultiplier;

	// If this sound should follow its owner
	// 如果这个声音应该跟随它的主人
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	uint32 bFollow:1;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "AnimNotify")
	uint32 bPreviewIgnoreAttenuation:1;
#endif

	// Socket or bone name to attach sound to
	// 将声音附加到的插槽或骨骼名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta=(EditCondition="bFollow", ExposeOnSpawn = true))
	FName AttachName;
};



