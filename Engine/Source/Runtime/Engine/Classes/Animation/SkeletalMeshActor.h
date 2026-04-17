// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "SkeletalMeshActor.generated.h"

class UAnimMontage;
class UAnimSequence;

/**
 * SkeletalMeshActor is an instance of a USkeletalMesh in the world.
 * Skeletal meshes are deformable meshes that can be animated and change their geometry at run-time.
 * Skeletal meshes dragged into the level from the Content Browser are automatically converted to StaticMeshActors.
 * 
 * @see https://docs.unrealengine.com/latest/INT/Engine/Content/Types/SkeletalMeshes/
 * @see USkeletalMesh
 */
UCLASS(ClassGroup=ISkeletalMeshes, Blueprintable, ComponentWrapperClass, ConversionRoot, meta=(ChildCanTick), MinimalAPI)
class ASkeletalMeshActor : public AActor
{
	GENERATED_UCLASS_BODY()

	ENGINE_API virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

	/** Whether or not this actor should respond to anim notifies - CURRENTLY ONLY AFFECTS PlayParticleEffect NOTIFIES**/
	/** 该 actor 是否应响应动画通知 - 当前仅影响 PlayParticleEffect NOTIFIES**/
	/** 该 actor 是否应响应动画通知 - 当前仅影响 PlayParticleEffect NOTIFIES**/
	/** 该 actor 是否应响应动画通知 - 当前仅影响 PlayParticleEffect NOTIFIES**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Animation, AdvancedDisplay)
	uint32 bShouldDoAnimNotifies:1;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	uint32 bWakeOnLevelStart_DEPRECATED:1;
#endif

private:
	UPROPERTY(Category = SkeletalMeshActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Mesh,Components|SkeletalMesh,Animation,Physics", AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;
public:
	/** 用于将网格复制到客户端 */

	/** 用于将网格复制到客户端 */
	/** Used to replicate mesh to clients */
	/** 用于将网格复制到客户端 */
	/** 用于将物理资产复制给客户端 */
	UPROPERTY(replicatedUsing=OnRep_ReplicatedMesh, transient)
	TObjectPtr<class USkeletalMesh> ReplicatedMesh;
	/** 用于将物理资产复制给客户端 */

	/** 用于复制索引 0 中的材料 */
	/** Used to replicate physics asset to clients */
	/** 用于将物理资产复制给客户端 */
	UPROPERTY(replicatedUsing=OnRep_ReplicatedPhysAsset, transient)
	/** 用于复制索引 0 中的材料 */
	TObjectPtr<class UPhysicsAsset> ReplicatedPhysAsset;

	/** used to replicate the material in index 0 */
	/** 复制通知回调 */
	/** 用于复制索引 0 中的材料 */
	UPROPERTY(replicatedUsing=OnRep_ReplicatedMaterial0)
	TObjectPtr<class UMaterialInterface> ReplicatedMaterial0;

	/** 复制通知回调 */
	UPROPERTY(replicatedUsing=OnRep_ReplicatedMaterial1)
	TObjectPtr<class UMaterialInterface> ReplicatedMaterial1;

	/** Replication Notification Callbacks */
	/** 复制通知回调 */
	UFUNCTION()
	ENGINE_API virtual void OnRep_ReplicatedMesh();

	UFUNCTION()
	ENGINE_API virtual void OnRep_ReplicatedPhysAsset();

	UFUNCTION()
	ENGINE_API virtual void OnRep_ReplicatedMaterial0();

	UFUNCTION()
	ENGINE_API virtual void OnRep_ReplicatedMaterial1();


	//~ Begin UObject Interface
 // ~ 开始 UObject 接口
protected:
	ENGINE_API virtual FString GetDetailedInfoInternal() const override;
public:
	//~ End UObject Interface
 // ~ 结束 UObject 接口

	//~ Begin AActor Interface
 // ~ 开始 AActor 界面
#if WITH_EDITOR
	ENGINE_API virtual void CheckForErrors() override;
	ENGINE_API virtual bool GetReferencedContentObjects( TArray<UObject*>& Objects ) const override;
	ENGINE_API virtual void EditorReplacedActor(AActor* OldActor) override;
	ENGINE_API virtual void LoadedFromAnotherClass(const FName& OldClassName) override;
	/** 返回 SkeletalMeshComponent 子对象 **/
#endif
	ENGINE_API virtual void PostInitializeComponents() override;
	/** 返回 SkeletalMeshComponent 子对象 **/
	//~ End AActor Interface
 // ~ 结束AActor接口

private:
	// currently actively playing montage
 // 目前正在积极播放蒙太奇
	TMap<FName, TWeakObjectPtr<class UAnimMontage>> CurrentlyPlayingMontages;

public:
	/** Returns SkeletalMeshComponent subobject **/
	/** 返回 SkeletalMeshComponent 子对象 **/
	class USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }
};



