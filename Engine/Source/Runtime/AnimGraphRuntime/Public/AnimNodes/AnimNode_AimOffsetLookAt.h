// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNode_AimOffsetLookAt.generated.h"

/** 
 * This node uses a source transform of a socket on the skeletal mesh to automatically calculate
 * Yaw and Pitch directions for a referenced aim offset given a point in the world to look at.
 */
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_AimOffsetLookAt : public FAnimNode_BlendSpacePlayer
{
	GENERATED_BODY()

	/** Cached local transform of the source socket */
	/** 源套接字的缓存本地转换 */
	/** 源套接字的缓存本地转换 */
	/** 源套接字的缓存本地转换 */
	FTransform SocketLocalTransform;
	/** 枢轴套接字的缓存本地转换  */

	/** 枢轴套接字的缓存本地转换  */
	/** Cached local transform of the pivot socket  */
	/** 枢轴套接字的缓存本地转换  */
	FTransform PivotSocketLocalTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink BasePose;

	/*
	* Max LOD that this node is allowed to run
	* For example if you have LODThreshold to be 2, it will run until LOD 2 (based on 0 index)
	* when the component LOD becomes 3, it will stop update/evaluate
	* currently transition would be issue and that has to be re-visited
	*/
	/** 用作查看源的插槽或骨骼。然后它将指向 LookAtLocation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "LOD Threshold"))
	int32 LODThreshold;
	/** 用作查看源的插槽或骨骼。然后它将指向 LookAtLocation */

	/** Socket or bone to treat as the look at source. This will then be pointed at LookAtLocation */
	/** 用作查看源的插槽或骨骼。然后它将指向 LookAtLocation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinHiddenByDefault))
	FName SourceSocketName;

	/** 
	 * Socket or bone to treat as the look at pivot (optional). This will overwrite the translation of the 
	 * source socket transform improve the lookat direction, especially when the target is close 
	/** 位置，在世界空间中观察 */
	 * to the character 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinHiddenByDefault))
	/** 位置，在世界空间中观察 */
	/** 套接字变换中的方向考虑“向前”或查看轴 */
	FName PivotSocketName;

	/** Location, in world space to look at */
	/** 位置，在世界空间中观察 */
	/** 该节点混合到输出姿势中的数量 */
	/** 套接字变换中的方向考虑“向前”或查看轴 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinShownByDefault))
	FVector LookAtLocation;

	/** 缓存计算的混合输入 */
	/** Direction in the socket transform to consider the 'forward' or look at axis */
	/** 该节点混合到输出姿势中的数量 */
	/** 套接字变换中的方向考虑“向前”或查看轴 */
	/** 缓存对源套接字骨骼的引用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinHiddenByDefault))
	FVector SocketAxis;

	/** 缓存对枢轴插槽骨骼的引用 */
	/** 缓存计算的混合输入 */
	/** Amount of this node to blend into the output pose */
	/** 该节点混合到输出姿势中的数量 */
	/** 缓存标志指示是否启用 LOD 阈值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinShownByDefault))
	/** 缓存对源套接字骨骼的引用 */
	float Alpha;

	/** Cached calculated blend input */
	/** 缓存对枢轴插槽骨骼的引用 */
	/** 缓存计算的混合输入 */
	FVector CurrentBlendInput;

	/** 缓存标志指示是否启用 LOD 阈值 */
	/** Cached reference to the source socket's bone */
	/** 缓存对源套接字骨骼的引用 */
	FBoneReference SocketBoneReference;

	/** Cached reference to the pivot socket's bone */
	/** 缓存对枢轴插槽骨骼的引用 */
	FBoneReference PivotSocketBoneReference;

	/** Cached flag to indicate whether LOD threshold is enabled */
	/** 缓存标志指示是否启用 LOD 阈值 */
	bool bIsLODEnabled;

public:
	ANIMGRAPHRUNTIME_API FAnimNode_AimOffsetLookAt();

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

	// FAnimNode_BlendSpacePlayer interface
 // FAnimNode_BlendSpacePlayer接口
	ANIMGRAPHRUNTIME_API virtual FVector GetPosition() const override;

	ANIMGRAPHRUNTIME_API void UpdateFromLookAtTarget(FPoseContext& LocalPoseContext);
};
