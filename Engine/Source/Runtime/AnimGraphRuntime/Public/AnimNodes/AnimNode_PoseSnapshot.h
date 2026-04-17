// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/PoseSnapshot.h"
#include "AnimNode_PoseSnapshot.generated.h"

class UAnimInstance;

/** How to access the snapshot */
/** 如何访问快照 */
UENUM()
enum class ESnapshotSourceMode : uint8
{
	/** 
	 * Refer to an internal snapshot by name (previously stored with SavePoseSnapshot). 
	 * This can be more efficient than access via pin.
	 */
	NamedSnapshot,

	/** 
	 * Use a snapshot variable (previously populated using SnapshotPose).
	 * This is more flexible and allows poses to be modified and managed externally to the animation blueprint.
	 */
	SnapshotPin
};

/** Provide a snapshot pose, either from the internal named pose cache or via a supplied snapshot */
/** 从内部命名姿势缓存或通过提供的快照提供快照姿势 */
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_PoseSnapshot : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:	

	ANIMGRAPHRUNTIME_API FAnimNode_PoseSnapshot();

	/** FAnimNode_Base interface */
	/** FAnimNode_Base接口 */
	virtual bool HasPreUpdate() const override  { return true; }
	ANIMGRAPHRUNTIME_API virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	/** The name of the snapshot previously stored with SavePoseSnapshot */
	/** 之前使用 SavePoseSnapshot 存储的快照的名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot", meta = (PinShownByDefault))
	FName SnapshotName;

	/** Snapshot to use. This should be populated at first by calling SnapshotPose */
	/** 要使用的快照。这应该首先通过调用 SnapshotPose 来填充 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot", meta = (PinHiddenByDefault))
	FPoseSnapshot Snapshot;

	/** How to access the snapshot */
	/** 如何访问快照 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot", meta = (PinHiddenByDefault))
	ESnapshotSourceMode Mode;

private:
	/** Cache of target space bases to source space bases */
	/** 目标空间基地到源空间基地的缓存 */
	TArray<int32> SourceBoneMapping;

	/** Cached array of bone names for target's ref skeleton */
	/** 目标参考骨架的骨骼名称缓存数组 */
	TArray<FName> TargetBoneNames;

	/** Cached skeletal meshes we use to invalidate the bone mapping */
	/** 我们用来使骨骼映射无效的缓存骨架网格物体 */
	FName MappedSourceMeshName;
	FName MappedTargetMeshName;

	/** Cached skeletal mesh we used for updating the target bone name array */
	/** 我们用于更新目标骨骼名称数组的缓存骨架网格物体 */
	FName TargetBoneNameMesh;

private:
	/** Evaluation helper function - apply a snapshot pose to a pose */
	/** 评估辅助功能 - 将快照姿势应用于姿势 */
	ANIMGRAPHRUNTIME_API void ApplyPose(const FPoseSnapshot& PoseSnapshot, FCompactPose& OutPose);

	/** Evaluation helper function - cache the bone mapping between two skeletal meshes */
	/** 评估辅助函数 - 缓存两个骨架网格物体之间的骨骼映射 */
	ANIMGRAPHRUNTIME_API void CacheBoneMapping(FName SourceMeshName, FName TargetMeshName, const TArray<FName>& InSourceBoneNames, const TArray<FName>& InTargetBoneNames);
};
