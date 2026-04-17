// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#include "Animation/AnimNotifyQueue.h"
#include "AnimNotify.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEvent;
#if WITH_EDITORONLY_DATA
class FPrimitiveDrawInterface;
class FCanvas;
class FSceneView;
#endif

USTRUCT()
struct FBranchingPointNotifyPayload
{
public:
	GENERATED_USTRUCT_BODY()

	USkeletalMeshComponent* SkelMeshComponent;
	UAnimSequenceBase* SequenceAsset;
	FAnimNotifyEvent* NotifyEvent;
	int32 MontageInstanceID;
	bool bReachedEnd = false;

	FBranchingPointNotifyPayload()
		: SkelMeshComponent(nullptr)
		, SequenceAsset(nullptr)
		, NotifyEvent(nullptr)
		, MontageInstanceID(INDEX_NONE)
	{}

	FBranchingPointNotifyPayload(USkeletalMeshComponent* InSkelMeshComponent, UAnimSequenceBase* InSequenceAsset, FAnimNotifyEvent* InNotifyEvent, int32 InMontageInstanceID, bool bInReachedEnd = false)
		: SkelMeshComponent(InSkelMeshComponent)
		, SequenceAsset(InSequenceAsset)
		, NotifyEvent(InNotifyEvent)
		, MontageInstanceID(InMontageInstanceID)
		, bReachedEnd(bInReachedEnd)
	{}
};

UCLASS(abstract, Blueprintable, const, hidecategories=Object, collapsecategories, MinimalAPI)
class UAnimNotify : public UObject
{
	GENERATED_UCLASS_BODY()

	/** 
	 * Implementable event to get a custom name for the notify
	 */
	UFUNCTION(BlueprintNativeEvent)
	ENGINE_API FString GetNotifyName() const;

	UFUNCTION(BlueprintImplementableEvent, meta=(AutoCreateRefTerm="EventReference"))
	ENGINE_API bool Received_Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) const;

#if WITH_EDITORONLY_DATA
	/** Color of Notify in editor */
	/** 编辑器中通知的颜色 */
	/** 编辑器中通知的颜色 */
	/** 编辑器中通知的颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotify)
	FColor NotifyColor;
	/** 此通知实例是否应在动画编辑器中触发 */

	/** 此通知实例是否应在动画编辑器中触发 */
	/** Whether this notify instance should fire in animation editors */
	/** 此通知实例是否应在动画编辑器中触发 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category=AnimNotify)
	bool bShouldFireInEditor;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	virtual void OnAnimNotifyCreatedInEditor(FAnimNotifyEvent& ContainingAnimNotifyEvent) {};
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const { return true; }
	virtual void ValidateAssociatedAssets() {}
	/** 覆盖此设置以防止在动画编辑器中触发此通知类型 */
	virtual void DrawInEditor(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp, const UAnimSequenceBase* Animation, const FAnimNotifyEvent& NotifyEvent) const {}
	virtual void DrawCanvasInEditor(FCanvas& Canvas, FSceneView& View, USkeletalMeshComponent* MeshComp, const UAnimSequenceBase* Animation, const FAnimNotifyEvent& NotifyEvent) const {}
	/** 覆盖此设置以防止在动画编辑器中触发此通知类型 */

	/** Override this to prevent firing this notify type in animation editors */
	/** 覆盖此设置以防止在动画编辑器中触发此通知类型 */
	virtual bool ShouldFireInEditor() { return bShouldFireInEditor; }
#endif

	UE_DEPRECATED(5.0, "Please use the other Notify function instead")
	ENGINE_API virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);
	ENGINE_API virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference);
	ENGINE_API virtual void BranchingPointNotify(FBranchingPointNotifyPayload& BranchingPointPayload);

	// @todo document 
 // @todo文档
	/** 创建此类通知时使用的 TriggerWeightThreshold */
	virtual FString GetEditorComment() 
	{ 
	/** 创建此类通知时使用的 TriggerWeightThreshold */
		return TEXT(""); 
	}

	/** TriggerWeightThreshold to use when creating notifies of this type */
	/** 创建此类通知时使用的 TriggerWeightThreshold */
	UFUNCTION(BlueprintNativeEvent)
	ENGINE_API float GetDefaultTriggerWeightThreshold() const;

	// @todo document 
 // @todo文档
	virtual FLinearColor GetEditorColor() 
	{ 
#if WITH_EDITORONLY_DATA
		return FLinearColor(NotifyColor); 
#else
		return FLinearColor::Black;
#endif // WITH_EDITORONLY_DATA
	}

	/**
	 * We don't instance UAnimNotify objects along with the animations they belong to, but
	/** U对象接口 */
	 * we still need a way to see which world this UAnimNotify is currently operating on.
	 * So this retrieves a contextual world pointer, from the triggering animation/mesh.  
	/** U对象接口 */
	/** 结束UObject接口 */
	 * 
	 * @return NULL if this isn't in the middle of a Received_Notify(), otherwise it's the world belonging to the Mesh passed to Received_Notify()
	/** 当在 Montage 上使用时，此通知始终是一个分支点。 */
	 */
	/** 结束UObject接口 */
	ENGINE_API virtual class UWorld* GetWorld() const override;

	/** 当在 Montage 上使用时，此通知始终是一个分支点。 */
	/** UObject Interface */
	/** U对象接口 */
	/* 我们当前正在触发 UAnimNotify 的网格物体（因此我们可以检索每个实例的信息） */
	ENGINE_API virtual void PostLoad() override;
	ENGINE_API virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	/** End UObject Interface */
	/** 结束UObject接口 */

	/* 我们当前正在触发 UAnimNotify 的网格物体（因此我们可以检索每个实例的信息） */
	/** This notify is always a branching point when used on Montages. */
	/** 当在 Montage 上使用时，此通知始终是一个分支点。 */
	bool bIsNativeBranchingPoint;

protected:
	ENGINE_API UObject* GetContainingAsset() const;

private:
	/* The mesh we're currently triggering a UAnimNotify for (so we can retrieve per instance information) */
	/* 我们当前正在触发 UAnimNotify 的网格物体（因此我们可以检索每个实例的信息） */
	class USkeletalMeshComponent* MeshContext;
};



