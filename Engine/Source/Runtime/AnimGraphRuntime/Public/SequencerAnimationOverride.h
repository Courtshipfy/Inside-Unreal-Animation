// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreTypes.h"
#include "HAL/Platform.h"
#include "UObject/Interface.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

#include "SequencerAnimationOverride.generated.h"

#define UE_API ANIMGRAPHRUNTIME_API

/**
 * Sequencer Animation Track Override interface.
 * Anim blueprints can override this to provide Sequencer with instructions on how to override this blueprint during Sequencer takeover.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class USequencerAnimationOverride : public UInterface
{
	GENERATED_BODY()
};

class ISequencerAnimationOverride
{
	GENERATED_BODY()

public:

	// Whether this animation blueprint allows Sequencer to override this anim instance and replace it during Sequencer playback.
 // 此动画蓝图是否允许 Sequencer 覆盖此动画实例并在 Sequencer 播放期间替换它。
	UFUNCTION(BlueprintNativeEvent, Category = "Sequencer", meta = (CallInEditor = "true"))
	UE_API bool AllowsCinematicOverride() const;
	
	virtual bool AllowsCinematicOverride_Implementation() const { return false; }

	// Should return a list of valid slot names for Sequencer to output to in the case that Sequencer is not permitted to override the anim instance.
 // 如果不允许 Sequencer 覆盖动画实例，则应返回 Sequencer 输出的有效槽名称列表。
	// Will be chosen by the user in drop down on the skeletal animation section properties. Should be named descriptively, as in some contexts (UEFN), the user
 // 将由用户在骨骼动画部分属性的下拉列表中选择。应以描述性方式命名，如在某些上下文中 (UEFN)，用户
	// will not be able to view the animation blueprint itself to determine the mixing behavior of the slot.
 // 将无法查看动画蓝图本身来确定插槽的混合行为。
	UFUNCTION(BlueprintNativeEvent, Category = "Sequencer", meta = (CallInEditor = "true"))
	UE_API TArray<FName> GetSequencerAnimSlotNames() const;

	virtual	TArray<FName> GetSequencerAnimSlotName_Implementation() const { return TArray<FName>(); }

	static TScriptInterface<ISequencerAnimationOverride> GetSequencerAnimOverride(USkeletalMeshComponent* SkeletalMeshComponent)
	{
		if (TSubclassOf<UAnimInstance> AnimInstanceClass = SkeletalMeshComponent->GetAnimClass())
		{
			if (UAnimInstance* AnimInstance = AnimInstanceClass->GetDefaultObject<UAnimInstance>())
			{
				if (AnimInstance->Implements<USequencerAnimationOverride>())
				{
					TScriptInterface<ISequencerAnimationOverride> AnimOverride = AnimInstance;
					if (AnimOverride.GetObject())
					{
						return AnimOverride;
					}
				}
			}
		}
		return nullptr;
	}
};


#undef UE_API
