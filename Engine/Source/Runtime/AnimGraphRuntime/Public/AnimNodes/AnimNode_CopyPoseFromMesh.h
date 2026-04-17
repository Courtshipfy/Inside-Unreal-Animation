// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_CopyPoseFromMesh.generated.h"

class USkeletalMeshComponent;
struct FAnimInstanceProxy;

/**
 *	Simple controller to copy a bone's transform to another one.
 */

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_CopyPoseFromMesh : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	/*  This is used by default if it's valid */
	/*  如果有效则默认使用 */
	/*  如果有效则默认使用 */
	/*  如果有效则默认使用 */
	UPROPERTY(BlueprintReadWrite, transient, Category=Copy, meta=(PinShownByDefault))
	TWeakObjectPtr<USkeletalMeshComponent> SourceMeshComponent;
	/* 如果 SourceMeshComponent 无效，并且如果这是 true，它将查找附加的父级作为源 */

	/* 如果 SourceMeshComponent 无效，并且如果这是 true，它将查找附加的父级作为源 */
	/* If SourceMeshComponent is not valid, and if this is true, it will look for attahced parent as a source */
	/* 如果 SourceMeshComponent 无效，并且如果这是 true，它将查找附加的父级作为源 */
	/* 也从 SouceMeshComponent 复制曲线。如果此实例还包含曲线属性，这将复制曲线 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (NeverAsPin))
	uint8 bUseAttachedParent : 1;
	/* 也从 SouceMeshComponent 复制曲线。如果此实例还包含曲线属性，这将复制曲线 */

	/* 从 SourceMeshComponent 复制自定义属性（动画属性） */
	/* Copy curves also from SouceMeshComponent. This will copy the curves if this instance also contains curve attributes */
	/* 也从 SouceMeshComponent 复制曲线。如果此实例还包含曲线属性，这将复制曲线 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (NeverAsPin))
	/* 从 SourceMeshComponent 复制自定义属性（动画属性） */
	/* 使用根空间变换复制到目标姿势。默认情况下，它复制它们的相对变换（骨骼空间）*/
	uint8 bCopyCurves : 1;
  
	/* Copy custom attributes (animation attributes) from SourceMeshComponent */
	/* 从 SourceMeshComponent 复制自定义属性（动画属性） */
	/* 如果您想指定复制根，请使用它 - 这将确保仅复制该关节的下方（包括） */
	/* 使用根空间变换复制到目标姿势。默认情况下，它复制它们的相对变换（骨骼空间）*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "Copy Custom Attributes(Animation Attributes)",Category = Copy, meta = (NeverAsPin))
	bool bCopyCustomAttributes;

	/* Use root space transform to copy to the target pose. By default, it copies their relative transform (bone space)*/
	/* 如果您想指定复制根，请使用它 - 这将确保仅复制该关节的下方（包括） */
	/* 使用根空间变换复制到目标姿势。默认情况下，它复制它们的相对变换（骨骼空间）*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (NeverAsPin))
	uint8 bUseMeshPose : 1;

	/* If you want to specify copy root, use this - this will ensure copy only below of this joint (inclusively) */
	/* 如果您想指定复制根，请使用它 - 这将确保仅复制该关节的下方（包括） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (NeverAsPin))
	FName RootBoneToCopy;

	ANIMGRAPHRUNTIME_API FAnimNode_CopyPoseFromMesh();

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual bool HasPreUpdate() const override { return true; }
	ANIMGRAPHRUNTIME_API virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

private:
	// this is source mesh references, so that we could compare and see if it has changed
 // 这是源网格参考，以便我们可以比较并查看它是否已更改
	TWeakObjectPtr<USkeletalMeshComponent>	CurrentlyUsedSourceMeshComponent;
	TWeakObjectPtr<USkeletalMesh>			CurrentlyUsedSourceMesh;
	TWeakObjectPtr<USkeletalMesh>			CurrentlyUsedMesh;

	// target mesh 
 // 目标网格
	TWeakObjectPtr<USkeletalMesh>			CurrentlyUsedTargetMesh;
	// cache of target space bases to source space bases
 // 目标空间基地到源空间基地的缓存
	TMap<int32, int32> BoneMapToSource;
	TMap<int32, int32> SourceBoneToTarget;

	// Cached transforms, copied on the game thread
 // 缓存的变换，在游戏线程上复制
	TArray<FTransform> SourceMeshTransformArray;

	// Cached curves, copied on the game thread
 // 缓存曲线，复制到游戏线程上
	FBlendedHeapCurve SourceCurves;

	// Cached attributes, copied on the game thread
 // 缓存属性，在游戏线程上复制
	UE::Anim::FMeshAttributeContainer SourceCustomAttributes;

	// reinitialize mesh component 
 // 重新初始化网格组件
	ANIMGRAPHRUNTIME_API void ReinitializeMeshComponent(USkeletalMeshComponent* NewSkeletalMeshComponent, USkeletalMeshComponent* TargetMeshComponent);
	ANIMGRAPHRUNTIME_API void RefreshMeshComponent(USkeletalMeshComponent* TargetMeshComponent);
};
