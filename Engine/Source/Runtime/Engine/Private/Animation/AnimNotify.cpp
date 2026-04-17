// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimNotifies/AnimNotify.h"

#include "Animation/AnimNotifyQueue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify)

/////////////////////////////////////////////////////
// UAnimNotify
// U动画通知

UAnimNotify::UAnimNotify(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MeshContext(NULL)
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 200, 200, 255);
	bShouldFireInEditor = true;
#endif // WITH_EDITORONLY_DATA

	bIsNativeBranchingPoint = false;
}

void UAnimNotify::Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation)
{
}

void UAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	USkeletalMeshComponent* PrevContext = MeshContext;
	MeshContext = MeshComp;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	Notify(MeshComp, Animation);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	Received_Notify(MeshComp, Animation, EventReference);
	MeshContext = PrevContext;
}

void UAnimNotify::BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	const FAnimNotifyEventReference EventReference;
	Notify(BranchingPointPayload.SkelMeshComponent, BranchingPointPayload.SequenceAsset, EventReference);
}

class UWorld* UAnimNotify::GetWorld() const
{
	return (MeshContext ? MeshContext->GetWorld() : NULL);
}

/// @cond DOXYGEN_WARNINGS

FString UAnimNotify::GetNotifyName_Implementation() const
{
	FString NotifyName;

#if WITH_EDITORONLY_DATA
	if (UObject* ClassGeneratedBy = GetClass()->ClassGeneratedBy)
	{
		// GeneratedBy will be valid for blueprint types and gives a clean name without a suffix
  // generatedBy 对蓝图类型有效，并给出一个没有后缀的干净名称
		NotifyName = ClassGeneratedBy->GetName();
	}
	else
#endif
	{
		// Native notify classes are clean without a suffix otherwise
  // 本机通知类是干净的，没有后缀，否则
		NotifyName = GetClass()->GetName();
	}

	NotifyName.ReplaceInline(TEXT("AnimNotify_"), TEXT(""), ESearchCase::CaseSensitive);
	
	return NotifyName;
}

float UAnimNotify::GetDefaultTriggerWeightThreshold_Implementation() const
{
/// @endcond
	return ZERO_ANIMWEIGHT_THRESH;
}
// @endcond
// @endcond
// @endcond
// @endcond

/// @endcond
/// @endcond
/// @endcond

void UAnimNotify::PostLoad()
{
	Super::PostLoad();
#if WITH_EDITOR
	// Ensure that all loaded notifies are transactional
 // 确保所有加载的通知都是事务性的
	SetFlags(GetFlags() | RF_Transactional);

	// Make sure the asset isn't bogus (e.g., a looping particle system in a one-shot notify)
 // 确保资产不是伪造的（例如，一次性通知中的循环粒子系统）
	ValidateAssociatedAssets();
#endif
}

void UAnimNotify::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
#if WITH_EDITOR
	ValidateAssociatedAssets();
#endif
	Super::PreSave(ObjectSaveContext);
}

UObject* UAnimNotify::GetContainingAsset() const
{
	UObject* ContainingAsset = GetTypedOuter<UAnimSequenceBase>();
	if (ContainingAsset == nullptr)
	{
		ContainingAsset = GetOutermost();
	}
	return ContainingAsset;
}

