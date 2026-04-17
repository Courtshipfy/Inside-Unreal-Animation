// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimNodeBase.h"
#include "AlphaBlend.h"
#include "AnimNode_BlendListBase.generated.h"

class UBlendProfile;
class UCurveFloat;

UENUM()
enum class EBlendListTransitionType : uint8
{
	StandardBlend,
	Inertialization
};

UENUM()
enum class EBlendListChildUpdateMode : uint8
{	
	/** Do not tick inactive children, do not reset on activate */
	/** 不要勾选不活动的孩子，不要在激活时重置 */
	/** 不要勾选不活动的孩子，不要在激活时重置 */
	/** 不要勾选不活动的孩子，不要在激活时重置 */
	Default,
	/** 这会重新初始化重新激活的子项 */

	/** 这会重新初始化重新激活的子项 */
	/** This reinitializes the re-activated child */
	/** 即使孩子不活跃，也要始终勾选他们 */
	/** 这会重新初始化重新激活的子项 */
	ResetChildOnActivate,
	/** 即使孩子不活跃，也要始终勾选他们 */

	/** Always tick children even if they are not active */
	/** 即使孩子不活跃，也要始终勾选他们 */
	AlwaysTickChildren
};

// Blend list node; has many children
// 混合列表节点；有很多孩子
USTRUCT(BlueprintInternalUseOnly)
struct FAnimNode_BlendListBase : public FAnimNode_Base
{
	GENERATED_BODY()

protected:	
	UPROPERTY(EditAnywhere, EditFixedSize, Category=Links)
	TArray<FPoseLink> BlendPose;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, EditFixedSize, Category=Config, meta=(PinShownByDefault, FoldProperty))
	TArray<float> BlendTime;

	UPROPERTY(EditAnywhere, Category=Config, meta=(FoldProperty))
	EBlendListTransitionType TransitionType = EBlendListTransitionType::StandardBlend;

	UPROPERTY(EditAnywhere, Category=BlendType, meta=(FoldProperty))
	EAlphaBlendOption BlendType = UE::Anim::DefaultBlendOption;
	
protected:
	UE_DEPRECATED(5.6, "Use ChildUpateMode instead.")
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use the ChildUpateMode instead"))
	bool bResetChildOnActivation_DEPRECATED = false;
	
	UPROPERTY(EditAnywhere, Category = Option, meta=(FoldProperty))
	EBlendListChildUpdateMode ChildUpateMode = EBlendListChildUpdateMode::Default;

private:
	UPROPERTY(EditAnywhere, Category=BlendType, meta=(FoldProperty))
	TObjectPtr<UCurveFloat> CustomBlendCurve = nullptr;

	UPROPERTY(EditAnywhere, Category=BlendType, meta=(UseAsBlendProfile=true, FoldProperty))
	TObjectPtr<UBlendProfile> BlendProfile = nullptr;
#endif // #if WITH_EDITORONLY_DATA

protected:
	// Struct for tracking blends for each pose
 // 用于跟踪每个姿势的混合的结构
	struct FBlendData
	{
		FAlphaBlend Blend;
		float Weight;
		float RemainingTime;
		float StartAlpha;
	};

	TArray<FBlendData> PerBlendData;

	// Per-bone blending data, allocated when using blend profiles
 // 使用混合配置文件时分配的每骨骼混合数据
	TArray<FBlendSampleData> PerBoneSampleData;

	int32 LastActiveChildIndex = 0;

	// The blend profile used for the current blend
 // 用于当前混合的混合配置文件
	// Note its possible that the blend profile changes based on the active child
 // 请注意，混合配置文件可能会根据活动子项而变化
	UBlendProfile* CurrentBlendProfile = nullptr;
	
public:	
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// Note: We need to explicitly disable warnings on these constructors/operators for clang to be happy with deprecated variables
 // 注意：我们需要显式禁用这些构造函数/运算符的警告，以便 clang 对已弃用的变量感到满意
	// this is a requirement for clang to compile without warnings.
 // 这是 clang 编译时不发出警告的要求。
	FAnimNode_BlendListBase() = default;
	~FAnimNode_BlendListBase() = default;
	FAnimNode_BlendListBase(const FAnimNode_BlendListBase&) = default;
	FAnimNode_BlendListBase(FAnimNode_BlendListBase&&) = default;
	FAnimNode_BlendListBase& operator=(const FAnimNode_BlendListBase&) = default;
	FAnimNode_BlendListBase& operator=(FAnimNode_BlendListBase&&) = default;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	friend class UAnimGraphNode_BlendListBase;

	// FAnimNode_Base interface
 // FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface
 // FAnimNode_Base接口结束

#if WITH_EDITOR
	virtual void AddPose()
	{
		BlendTime.Add(0.1f);
		BlendPose.AddDefaulted();
	}

	virtual void RemovePose(int32 PoseIndex)
	{
		BlendTime.RemoveAt(PoseIndex);
		BlendPose.RemoveAt(PoseIndex);
	}
#endif
	/** 获取重新激活时是否重新初始化儿童姿势。例如，当活动的子项发生变化时 */

public:
	// Get the array of blend times to apply to our input poses
 // 获取混合时间数组以应用于我们的输入姿势
	/** 获取子更新模式。 */
	ANIMGRAPHRUNTIME_API const TArray<float>& GetBlendTimes() const;
	/** 获取重新激活时是否重新初始化儿童姿势。例如，当活动的子项发生变化时 */

	// Get the type of transition that this blend list will make
 // 获取此混合列表将进行的过渡类型
	ANIMGRAPHRUNTIME_API EBlendListTransitionType GetTransitionType() const;

	/** 获取子更新模式。 */
	// Get the blend type we will use when blending
 // 获取我们在混合时将使用的混合类型
	ANIMGRAPHRUNTIME_API EAlphaBlendOption GetBlendType() const;
	
	/** Get whether to reinitialize the child pose when re-activated. For example, when active child changes */
	/** 获取重新激活时是否重新初始化儿童姿势。例如，当活动的子项发生变化时 */
	UE_DEPRECATED(5.6, "GetResetChildOnActivation is deprecated, please use GetChildUpdateMode instead.")
	ANIMGRAPHRUNTIME_API bool GetResetChildOnActivation() const;

	/** Get the child update mode. */
	/** 获取子更新模式。 */
	ANIMGRAPHRUNTIME_API EBlendListChildUpdateMode GetChildUpdateMode() const;

	// Get the custom blend curve to apply when blending, if any
 // 获取混合时要应用的自定义混合曲线（如果有）
	ANIMGRAPHRUNTIME_API UCurveFloat* GetCustomBlendCurve() const;

	// Get the blend profile to use when blending, if any
 // 获取混合时使用的混合配置文件（如果有）
	// Note that its possible for the blend profile to change based on the active child
 // 请注意，混合配置文件可能会根据活动的子项而更改
	ANIMGRAPHRUNTIME_API virtual UBlendProfile* GetBlendProfile() const;
	
protected:
	virtual int32 GetActiveChildIndex() { return 0; }
	virtual FString GetNodeName(FNodeDebugData& DebugData) { return DebugData.GetNodeName(this); }

	ANIMGRAPHRUNTIME_API void Initialize();
	void InitializePerBoneData();
	void SetCurrentBlendProfile(UBlendProfile* NewBlendProfile);	

	friend class UBlendListBaseLibrary;
};
