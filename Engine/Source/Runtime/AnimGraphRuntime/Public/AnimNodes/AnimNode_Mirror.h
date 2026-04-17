// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/MirrorDataTable.h"
#include "AnimNode_Mirror.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_MirrorBase : public FAnimNode_Base
{
	GENERATED_BODY()
public:
	ANIMGRAPHRUNTIME_API FAnimNode_MirrorBase(); 
	ANIMGRAPHRUNTIME_API FAnimNode_MirrorBase(const FAnimNode_MirrorBase&); 
	ANIMGRAPHRUNTIME_API ~FAnimNode_MirrorBase(); 

	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	// Get the MirrorDataTable
	// 获取镜像数据表
	ANIMGRAPHRUNTIME_API virtual UMirrorDataTable* GetMirrorDataTable() const;

	// Set the MirrorDataTable
	// 设置镜像数据表
	ANIMGRAPHRUNTIME_API virtual bool SetMirrorDataTable(UMirrorDataTable* MirrorTable);

	// Get Mirror State
	// 获取镜像状态
	ANIMGRAPHRUNTIME_API virtual bool GetMirror() const;
	// How long to blend using inertialization when switching  mirrored state
	// 切换镜像状态时使用惯性混合多长时间
	ANIMGRAPHRUNTIME_API virtual float GetBlendTimeOnMirrorStateChange() const;

	// Should bones mirror
	// 应该骨头镜子
	ANIMGRAPHRUNTIME_API virtual bool GetBoneMirroring() const;

	// Should the curves mirror
	// 曲线是否应该镜像
	ANIMGRAPHRUNTIME_API virtual bool GetCurveMirroring() const;

	// Should attributes mirror (based on the bone mirroring data in the mirror data table) 
	// 应该属性mirror（根据镜像数据表中的骨骼镜像数据）
	ANIMGRAPHRUNTIME_API virtual bool GetAttributeMirroring() const;

	// Whether to reset (reinitialize) the child (source) pose when the mirror state changes
	// 当镜像状态改变时是否重置（重新初始化）子（源）姿势
	ANIMGRAPHRUNTIME_API virtual bool GetResetChildOnMirrorStateChange() const;

	// Set Mirror State
	// 设置镜像状态
	ANIMGRAPHRUNTIME_API virtual bool SetMirror(bool bInMirror);

	// Set how long to blend using inertialization when switching  mirrored state
	// 设置切换镜像状态时使用惯性进行混合的时间
	// @return true if the value was set (it is dynamic), or false if it could not (it is not dynamic or pin exposed)
	// @return true 如果该值已设置（它是动态的），或者 false 如果它不能设置（它不是动态的或引脚暴露的）
	ANIMGRAPHRUNTIME_API virtual bool SetBlendTimeOnMirrorStateChange(float InBlendTime);

	// Set if bones mirror
	// 设置骨骼是否镜像
	// @return true if the value was set (it is dynamic), or false if it could not (it is not dynamic or pin exposed)
	// @return true 如果该值已设置（它是动态的），或者 false 如果它不能设置（它不是动态的或引脚暴露的）
	ANIMGRAPHRUNTIME_API virtual bool SetBoneMirroring(bool bInBoneMirroring);

	// Set if curves mirror
	// 设置曲线是否镜像
	// @return true if the value was set (it is dynamic), or false if it could not (it is not dynamic or pin exposed)
	// @return true 如果该值已设置（它是动态的），或者 false 如果它不能设置（它不是动态的或引脚暴露的）
	ANIMGRAPHRUNTIME_API virtual bool SetCurveMirroring(bool bInCurveMirroring);

	// Set if attributes mirror
	// 设置属性是否镜像
	// @return true if the value was set (it is dynamic), or false if it could not (it is not dynamic or pin exposed)
	// @return true 如果该值已设置（它是动态的），或者 false 如果它不能设置（它不是动态的或引脚暴露的）
	ANIMGRAPHRUNTIME_API virtual bool SetAttributeMirroring(bool bInAttributeMirroring);

	// Set whether to reset (reinitialize) the child (source) pose when the mirror state changes
	// 设置当镜像状态改变时是否重置（重新初始化）子（源）姿势
	// @return true if the value was set (it is dynamic), or false if it could not (it is not dynamic or pin exposed)
	// @return true 如果该值已设置（它是动态的），或者 false 如果它不能设置（它不是动态的或引脚暴露的）
	ANIMGRAPHRUNTIME_API virtual bool SetResetChildOnMirrorStateChange(bool bInResetChildOnMirrorStateChange);

	/** This only used by custom handlers, and it is advanced feature. */
	/** 这仅由自定义处理程序使用，并且是高级功能。 */
	ANIMGRAPHRUNTIME_API virtual void SetSourceLinkNode(FAnimNode_Base* NewLinkNode);

	/** This only used by custom handlers, and it is advanced feature. */
	/** 这仅由自定义处理程序使用，并且是高级功能。 */
	ANIMGRAPHRUNTIME_API virtual FAnimNode_Base* GetSourceLinkNode();
protected:
	UPROPERTY(EditAnywhere, Category = Links)
	FPoseLink Source;

private:
	bool bMirrorState;
	bool bMirrorStateIsValid;

	void FillCompactPoseAndComponentRefRotations(const FBoneContainer& BoneContainer);
	// Compact pose format of Mirror Bone Map
	// 镜像骨图的紧凑姿势格式
	TCustomBoneIndexArray<FCompactPoseBoneIndex, FCompactPoseBoneIndex> CompactPoseMirrorBones;

	// Pre-calculated component space of reference pose, which allows mirror to work with any joint orient 
	// 预先计算的参考姿势的组件空间，允许镜子与任何关节方向一起工作
	TCustomBoneIndexArray<FQuat, FCompactPoseBoneIndex> ComponentSpaceRefRotations;
};


USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_Mirror : public FAnimNode_MirrorBase
{
	GENERATED_BODY()

	friend class UAnimGraphNode_Mirror;

public:
	ANIMGRAPHRUNTIME_API FAnimNode_Mirror();
	ANIMGRAPHRUNTIME_API ~FAnimNode_Mirror();

	ANIMGRAPHRUNTIME_API virtual UMirrorDataTable* GetMirrorDataTable() const override;
	ANIMGRAPHRUNTIME_API virtual bool SetMirrorDataTable(UMirrorDataTable* MirrorTable) override;

	ANIMGRAPHRUNTIME_API virtual bool GetMirror() const override;
	ANIMGRAPHRUNTIME_API virtual float GetBlendTimeOnMirrorStateChange() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetBoneMirroring() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetCurveMirroring() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetAttributeMirroring() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetResetChildOnMirrorStateChange() const override;

	ANIMGRAPHRUNTIME_API virtual bool SetMirror(bool bInMirror) override;
	ANIMGRAPHRUNTIME_API virtual bool SetBlendTimeOnMirrorStateChange(float InBlendTime) override;
	ANIMGRAPHRUNTIME_API virtual bool SetBoneMirroring(bool bInBoneMirroring) override;
	ANIMGRAPHRUNTIME_API virtual bool SetCurveMirroring(bool bInCurveMirroring) override;
	ANIMGRAPHRUNTIME_API virtual bool SetAttributeMirroring(bool bInAttributeMirroring) override;
	ANIMGRAPHRUNTIME_API virtual bool SetResetChildOnMirrorStateChange(bool bInResetChildOnMirrorStateChange) override;

protected:

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinShownByDefault, FoldProperty))
	bool bMirror = true; 

	UPROPERTY(EditAnywhere, Category = Settings, meta = (FoldProperty))
	TObjectPtr<UMirrorDataTable> MirrorDataTable = nullptr;

	// Inertialization blend time to use when transitioning between mirrored and unmirrored states
	// 在镜像和非镜像状态之间转换时使用的惯性化混合时间
	UPROPERTY(EditAnywhere, Category = MirrorTransition, meta = (PinHiddenByDefault, FoldProperty))
	float BlendTime = 0.0f;
	// Whether to reset (reinitialize) the child (source) pose when the mirror state changes
	// 当镜像状态改变时是否重置（重新初始化）子（源）姿势
	UPROPERTY(EditAnywhere, Category = MirrorTransition, meta = (FoldProperty))
	bool bResetChild = false;

	UPROPERTY(EditAnywhere, Category = MirroredChannels, meta=(DisplayName="Bone", FoldProperty))
	bool bBoneMirroring = true;

	UPROPERTY(EditAnywhere, Category = MirroredChannels, meta=(DisplayName = "Curve", FoldProperty))
	bool bCurveMirroring = true;

	UPROPERTY(EditAnywhere, Category = MirroredChannels, meta=(DisplayName = "Attributes", FoldProperty))
	bool bAttributeMirroring = true;
#endif
};


USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_Mirror_Standalone : public FAnimNode_MirrorBase
{
	GENERATED_BODY()

	friend class UAnimGraphNode_Mirror;

public:
	ANIMGRAPHRUNTIME_API FAnimNode_Mirror_Standalone();
	ANIMGRAPHRUNTIME_API FAnimNode_Mirror_Standalone(const FAnimNode_Mirror_Standalone&);
	ANIMGRAPHRUNTIME_API ~FAnimNode_Mirror_Standalone();

	ANIMGRAPHRUNTIME_API virtual UMirrorDataTable* GetMirrorDataTable() const override;
	ANIMGRAPHRUNTIME_API virtual bool SetMirrorDataTable(UMirrorDataTable* MirrorTable) override;

	ANIMGRAPHRUNTIME_API virtual bool GetMirror() const override;
	ANIMGRAPHRUNTIME_API virtual float GetBlendTimeOnMirrorStateChange() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetBoneMirroring() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetCurveMirroring() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetAttributeMirroring() const override;
	ANIMGRAPHRUNTIME_API virtual bool GetResetChildOnMirrorStateChange() const override;

	ANIMGRAPHRUNTIME_API virtual bool SetMirror(bool bInMirror) override;
	ANIMGRAPHRUNTIME_API virtual bool SetBlendTimeOnMirrorStateChange(float InBlendTime) override;
	ANIMGRAPHRUNTIME_API virtual bool SetBoneMirroring(bool bInBoneMirroring) override;
	ANIMGRAPHRUNTIME_API virtual bool SetCurveMirroring(bool bInCurveMirroring) override;
	ANIMGRAPHRUNTIME_API virtual bool SetAttributeMirroring(bool bInAttributeMirroring) override;
	ANIMGRAPHRUNTIME_API virtual bool SetResetChildOnMirrorStateChange(bool bInResetChildOnMirrorStateChange) override;

protected:

	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinShownByDefault))
	bool bMirror = true;

	UPROPERTY(EditAnywhere, Category = Settings)
	TObjectPtr<UMirrorDataTable> MirrorDataTable = nullptr;

	// Inertialization blend time to use when transitioning between mirrored and unmirrored states
	// 在镜像和非镜像状态之间转换时使用的惯性化混合时间
	UPROPERTY(EditAnywhere, Category = MirrorTransition, meta = (PinHiddenByDefault, FoldProperty))
	float BlendTime = 0.0f;

	// Whether to reset (reinitialize) the child (source) pose when the mirror state changes
	// 当镜像状态改变时是否重置（重新初始化）子（源）姿势
	UPROPERTY(EditAnywhere, Category = MirrorTransition, meta = (FoldProperty))
	bool bResetChild = false;

	UPROPERTY(EditAnywhere, Category = MirroredChannels, meta = (DisplayName = "Bone", FoldProperty))
	bool bBoneMirroring = true;

	UPROPERTY(EditAnywhere, Category = MirroredChannels, meta = (DisplayName = "Curve", FoldProperty))
	bool bCurveMirroring = true;

	UPROPERTY(EditAnywhere, Category = MirroredChannels, meta = (DisplayName = "Attributes", FoldProperty))
	bool bAttributeMirroring = true;
};
