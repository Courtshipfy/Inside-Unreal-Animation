// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Blend Space. Contains functionality shared across all blend space objects
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "Animation/AnimationAsset.h"
#include "AnimationRuntime.h"
#include "AnimNodeBase.h"
#include "Containers/ArrayView.h"
#include "Animation/BoneSocketReference.h"
#include "BlendSpace.generated.h"

class UCachedAnalysisProperties;
class UBlendSpace;

/**
* The base class for properties to be used in analysis. Engine will inherit from this to define structures used for
* the functions it supports. User-defined functions will likely need their own analysis structures inheriting from
* this too.
*/
UCLASS(MinimalAPI, config=Engine)
class UAnalysisProperties : public UObject
{
	GENERATED_BODY()

public:
	virtual void InitializeFromCache(TObjectPtr<UCachedAnalysisProperties> Cache) {};
	virtual void MakeCache(TObjectPtr<UCachedAnalysisProperties>& Cache, UBlendSpace* BlendSpace) {};

	/** Analysis function for this axis */
	/** 该轴的分析函数 */
	/** 该轴的分析函数 */
	/** 该轴的分析函数 */
	UPROPERTY()
	FString Function = TEXT("None");
};
/** 插值数据类型。 */

/** 插值数据类型。 */
/** Interpolation data types. */
/** 插值数据类型。 */
UENUM()
enum EBlendSpaceAxis : int
{
	BSA_None UMETA(DisplayName = "None"),
	BSA_X UMETA(DisplayName = "Horizontal (X) Axis"),
	BSA_Y UMETA(DisplayName = "Vertical (Y) Axis")
};

UENUM()
enum class EPreferredTriangulationDirection : uint8
{
	None UMETA(DisplayName = "None", ToolTip = "None"),
	Tangential UMETA(DisplayName = "Tangential", ToolTip = "When there is ambiguity, rectangles will be split so that the inserted edge tends to not point towards the origin"),
	Radial UMETA(DisplayName = "Radial", ToolTip = "When there is ambiguity, rectangles will be split so that the inserted edge tends to point towards the origin")
};

UENUM()
enum class EBlendSpacePerBoneBlendMode : uint8
{
	ManualPerBoneOverride UMETA(DisplayName = "Manual Per Bone Override", ToolTip = "Manually specify the bones and their smoothing interpolation times."),
	BlendProfile UMETA(DisplayName = "Blend Profile", ToolTip = "Use a blend profile to specify the bone smoothing interpolation times.")
};

USTRUCT()
struct FBlendSpaceBlendProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = SampleSmoothing, meta = (DisplayName = "Blend Profile", UseAsBlendMask = true, EditConditionHides))
	TObjectPtr<UBlendProfile> BlendProfile;

	UPROPERTY(EditAnywhere, Category = SampleSmoothing, meta = (DisplayName = "Weight Speed", ClampMin = "0"))
	float TargetWeightInterpolationSpeedPerSec = 0.0f;
};

USTRUCT()
struct FInterpolationParameter
{
	GENERATED_BODY()

	/**
	 * Smoothing Time used to move smoothly across the blendpsace from the current parameters to the target
	 * parameters. The different Smoothing Types will treat this in different ways, but in general a value of
	 * zero will disable all smoothing, and larger values will smooth more.
	 */
	UPROPERTY(EditAnywhere, DisplayName = "Smoothing Time", Category=Parameter, meta = (ClampMin = "0"))
	float InterpolationTime = 0.f;

	/**
	 * Damping ratio - only used when the type is set to SpringDamper. A value of 1 will move quickly and
	 * smoothly to the target, without overshooting. Values as low as 0 can be used to encourage some overshoot,
	 * and values around 0.7 can make pose transitions look more natural.
	 */
	UPROPERTY(EditAnywhere, Category=Parameter, meta = (ClampMin = "0", EditCondition = "InterpolationType == EFilterInterpolationType::BSIT_SpringDamper && InterpolationTime > 0"))
	float DampingRatio = 1.f;

	/**
	 * Maximum speed, in real units. For example, if this axis is degrees then you could use a value of 90 to
	 * limit the turn rate to 90 degrees per second. Only used when greater than zero and the type is
	 * set to SpringDamper or Exponential.
	 */
	/** 用于过滤输入值以决定如何达到目标的平滑类型。 */
	UPROPERTY(EditAnywhere, Category=Parameter, meta = (ClampMin = "0"))
	float MaxSpeed = 0.f;
	/** 用于过滤输入值以决定如何达到目标的平滑类型。 */

	/** Type of smoothing used for filtering the input value to decide how to get to target. */
	/** 用于过滤输入值以决定如何达到目标的平滑类型。 */
	UPROPERTY(EditAnywhere, DisplayName = "Smoothing Type", Category=Parameter, meta = (EditCondition = "InterpolationTime > 0"))
	TEnumAsByte<EFilterInterpolationType> InterpolationType = EFilterInterpolationType::BSIT_SpringDamper;
};

USTRUCT()
struct FBlendParameter
{
	/** 该轴范围的最小值。 */
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, DisplayName = "Name", Category=BlendParameter)
	/** 该轴范围的最小值。 */
	/** 该轴范围的最大值。 */
	FString DisplayName;

	/** Minimum value for this axis range. */
	/** 该轴范围的最小值。 */
	/** 沿该轴的网格划分数。 */
	/** 该轴范围的最大值。 */
	UPROPERTY(EditAnywhere, DisplayName = "Minimum Axis Value", Category=BlendParameter, meta=(NoResetToDefault))
	float Min;

	/** 如果为 true，则在添加、移动或更改轴时，样本将始终捕捉到该轴上的网格。 */
	/** Maximum value for this axis range. */
	/** 沿该轴的网格划分数。 */
	/** 该轴范围的最大值。 */
	UPROPERTY(EditAnywhere, DisplayName = "Maximum Axis Value", Category=BlendParameter, meta=(NoResetToDefault))
	/** 如果为 true，则输入可以超出最小/最大范围，并且混合空间被视为在此轴上循环。如果为 false，则输入参数将被限制为该轴上的最小/最大值。 */
	float Max;

	/** 如果为 true，则在添加、移动或更改轴时，样本将始终捕捉到该轴上的网格。 */
	/** The number of grid divisions along this axis. */
	/** 沿该轴的网格划分数。 */
	UPROPERTY(EditAnywhere, DisplayName = "Grid Divisions", Category=BlendParameter, meta=(UIMin="1", ClampMin="1"))
	int32 GridNum;
	/** 如果为 true，则输入可以超出最小/最大范围，并且混合空间被视为在此轴上循环。如果为 false，则输入参数将被限制为该轴上的最小/最大值。 */

	/** If true then samples will always be snapped to the grid on this axis when added, moved, or the axes are changed. */
	/** 如果为 true，则在添加、移动或更改轴时，样本将始终捕捉到该轴上的网格。 */
	UPROPERTY(EditAnywhere, DisplayName = "Snap to Grid", Category = BlendParameter)
	bool bSnapToGrid;

	/** If true then the input can go outside the min/max range and the blend space is treated as being cyclic on this axis. If false then input parameters are clamped to the min/max values on this axis. */
	/** 如果为 true，则输入可以超出最小/最大范围，并且混合空间被视为在此轴上循环。如果为 false，则输入参数将被限制为该轴上的最小/最大值。 */
	UPROPERTY(EditAnywhere, DisplayName = "Wrap Input", Category = BlendParameter)
	bool bWrapInput;
	/** 返回每个网格的大小。 */

	FBlendParameter()
		: DisplayName(TEXT("None"))
		, Min(0.f)
		, Max(100.f)
		, GridNum(4) // TODO when changing GridNum's default value, it breaks all grid samples ATM - provide way to rebuild grid samples during loading
		, bSnapToGrid(false)
		, bWrapInput(false)
/** 样本数据 */
	/** 返回每个网格的大小。 */
	{
	}

	float GetRange() const
	{
		return Max-Min;
	}
	/** Return size of each grid. */
/** 样本数据 */
	/** 返回每个网格的大小。 */
	float GetGridSize() const
	{
		return GetRange()/(float)GridNum;
	}
	
};

/** Sample data */
/** 样本数据 */
USTRUCT()
struct FBlendSample
{
	GENERATED_BODY()

	// For linked animations
 // 对于链接动画
	UPROPERTY(EditAnywhere, Category=BlendSample)
	TObjectPtr<class UAnimSequence> Animation;

	//blend 0->x, blend 1->y, blend 2->z
 // 混合 0->x，混合 1->y，混合 2->z

	UPROPERTY(EditAnywhere, Category=BlendSample)
	FVector SampleValue;
	
	UPROPERTY(EditAnywhere, Category = BlendSample, meta=(UIMin="0.01", UIMax="2.0", ClampMin="0.01", ClampMax="64.0"))
	float RateScale = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = BlendSample, meta=(InlineEditConditionToggle))
	bool bUseSingleFrameForBlending = false;
	
	// Single Frame Blending: If selected, instead of using the entire animation sequence, only the specified single frame is used for sampling the animation
 // 单帧混合：如果选择，则不使用整个动画序列，而是仅使用指定的单帧对动画进行采样
	UPROPERTY(EditAnywhere, Category = BlendSample, meta=(EditCondition="bUseSingleFrameForBlending", DisplayName="Use Single Frame", UIMin="0"))
	uint32 FrameIndexToSample = 0;

#if WITH_EDITORONLY_DATA
	// Whether or not this sample will be moved when the "analyse all" button is used. Note that, even if disabled,
 // 当使用“分析全部”按钮时是否移动该样本。请注意，即使禁用，
	// it will still be available for individual sample analysis/moving
 // 它仍然可用于单个样本分析/移动
	UPROPERTY(EditAnywhere, Category = BlendSample, meta=(UIMin="0.01", UIMax="2.0", ClampMin="0.01", ClampMax="64.0"))
	uint8 bIncludeInAnalyseAll : 1;

	UPROPERTY(transient)
	uint8 bIsValid : 1;

	// Cache the samples marker data counter so that we can track if it changes and revalidate the blendspace
 // 缓存样本标记数据计数器，以便我们可以跟踪它是否发生变化并重新验证混合空间
	int32 CachedMarkerDataUpdateCounter;

#endif // WITH_EDITORONLY_DATA

	FBlendSample()
		: Animation(nullptr)
		, SampleValue(0.f)
		, RateScale(1.0f)
#if WITH_EDITORONLY_DATA
		, bIncludeInAnalyseAll(true)
		, bIsValid(false)
		, CachedMarkerDataUpdateCounter(INDEX_NONE)
#endif // WITH_EDITORONLY_DATA
	{		
	}
	
	FBlendSample(class UAnimSequence* InAnim, FVector InValue, bool bInIsSnapped, bool bInIsValid) 
		: Animation(InAnim)
		, SampleValue(InValue)
		, RateScale(1.0f)
#if WITH_EDITORONLY_DATA
		, bIncludeInAnalyseAll(true)
		, bIsValid(bInIsValid)
		, CachedMarkerDataUpdateCounter(INDEX_NONE)
#endif // WITH_EDITORONLY_DATA
	{		 
	}
	
	bool operator==( const FBlendSample& Other ) const 
	/** 样本索引 */
	{
		return (Other.Animation == Animation && Other.SampleValue == SampleValue && Other.bUseSingleFrameForBlending == bUseSingleFrameForBlending && Other.FrameIndexToSample == FrameIndexToSample && FMath::IsNearlyEqual(Other.RateScale, RateScale));
	}

	/** 顶点位于标准化空间中 - 即在 0-1 范围内。 */
	/** 样本索引 */
	ENGINE_API float GetSamplePlayLength() const;
};

/**
	/** 顶点位于标准化空间中 - 即在 0-1 范围内。 */
 * This is the runtime representation of a segment which stores its vertices (start and end) in normalized space.
 */
USTRUCT()
struct FBlendSpaceSegment
{
	/** 边缘法向朝外 */
	GENERATED_BODY();

public:
	// Triangles have three vertices
 // 三角形有三个顶点
	static const int32 NUM_VERTICES = 2;
	/** 边缘法向朝外 */

	/** Indices into the samples */
	/** 样本索引 */
	UPROPERTY()
	int32 SampleIndices[NUM_VERTICES] = { INDEX_NONE, INDEX_NONE };

	/** The vertices are in the normalized space - i.e. in the range 0-1. */
	/** 顶点位于标准化空间中 - 即在 0-1 范围内。 */
	UPROPERTY()
	float Vertices[NUM_VERTICES]  = { 0.f, 0.f };
};

USTRUCT()
struct FBlendSpaceTriangleEdgeInfo
{
	GENERATED_BODY();

public:
	/** Edge normal faces out */
	/** 边缘法向朝外 */
	UPROPERTY()
	FVector2D Normal = FVector2D(0.f);

	UPROPERTY()
	int32 NeighbourTriangleIndex = INDEX_NONE;

	/**
	* IF there is no neighbor, then (a) we're on the perimeter and (b) these will be the indices of
	* triangles along the perimeter (next to the start and end of this edge, respectively) 
	*/
	UPROPERTY()
	/** 样本索引 */
	int32 AdjacentPerimeterTriangleIndices[2] = { INDEX_NONE, INDEX_NONE };

	/**
	 * The vertex index of the associated AdjacentPerimeterTriangle such that the perimeter edge is
	/** 顶点位于标准化空间中 - 即在 0-1 范围内。 */
	 * from this vertex to the next.
	 */
	/** 样本索引 */
	UPROPERTY()
	/** 从顶点索引开始并（逆时针）到下一个顶点的边的信息 */
	int32 AdjacentPerimeterVertexIndices[2]  = { INDEX_NONE, INDEX_NONE };
};

	/** 顶点位于标准化空间中 - 即在 0-1 范围内。 */
/**
* This is the runtime representation of a triangle. Each triangle stores its vertices etc in normalized space,
* with an index to the original samples.
 */
	/** 从顶点索引开始并（逆时针）到下一个顶点的边的信息 */
USTRUCT()
struct FBlendSpaceTriangle
{
	GENERATED_BODY();

public:
	// Triangles have three vertices
 // 三角形有三个顶点
	static const int32 NUM_VERTICES = 3;

public:

	/** Indices into the samples */
	/** 样本索引 */
	UPROPERTY(EditAnywhere, Category = EditorElement)
	int32 SampleIndices[NUM_VERTICES] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };

	/** The vertices are in the normalized space - i.e. in the range 0-1. */
	/** 顶点位于标准化空间中 - 即在 0-1 范围内。 */
	UPROPERTY(EditAnywhere, Category = EditorElement)
	FVector2D Vertices[NUM_VERTICES] = { FVector2D(0.f), FVector2D(0.f), FVector2D(0.f) };
	
	/** Info for the edge starting at the vertex index and going (anti-clockwise) to the next vertex */
	/** 从顶点索引开始并（逆时针）到下一个顶点的边的信息 */
	UPROPERTY(EditAnywhere, Category = EditorElement)
	FBlendSpaceTriangleEdgeInfo EdgeInfo[NUM_VERTICES];
};

USTRUCT()
struct FWeightedBlendSample
{
	GENERATED_BODY();

public:
	FWeightedBlendSample(int32 Index = INDEX_NONE, float Weight = 0) : SampleIndex(Index), SampleWeight(Weight) {}

public:
	UPROPERTY()
	int32 SampleIndex;

	UPROPERTY()
	float SampleWeight;
};

/**
* The runtime data used for interpolating. Note that only one of Segments/Triangles will be in use,
* depending on the dimensionality of the data.
*/
USTRUCT()
struct FBlendSpaceData
{
	GENERATED_BODY();
public:
	void GetSamples(
		TArray<FWeightedBlendSample>& OutWeightedSamples,
		const TArray<int32>&          InDimensionIndices,
		const FVector&                InSamplePosition,
		int32&                        InOutTriangulationIndex) const;

	void Empty()
	{
		Segments.Empty();
		Triangles.Empty();
	}

	bool IsEmpty() const 
	{
		return Segments.Num() == 0 && Triangles.Num() == 0;
	}
public:
	UPROPERTY()
	TArray<FBlendSpaceSegment> Segments;

	UPROPERTY()
	TArray<FBlendSpaceTriangle> Triangles;

private:
	void GetSamples1D(
		TArray<FWeightedBlendSample>& OutWeightedSamples,
		const TArray<int32>&          InDimensionIndices,
		const FVector&                InSamplePosition,
		int32&                        InOutSegmentIndex) const;

	void GetSamples2D(
		TArray<FWeightedBlendSample>& OutWeightedSamples,
		const TArray<int32>&          InDimensionIndices,
		const FVector&                InSamplePosition,
/** 网格元素权重的结果 **/
		int32&                        InOutTriangleIndex) const;
};

/**
 * Each elements in the grid
 */
USTRUCT()
struct FEditorElement
/** 网格元素权重的结果 **/
{
	GENERATED_BODY()

	// for now we only support triangles
 // 目前我们只支持三角形
	static const int32 MAX_VERTICES = 3;

	UPROPERTY(EditAnywhere, Category=EditorElement)
	int32 Indices[MAX_VERTICES];

	UPROPERTY(EditAnywhere, Category=EditorElement)
	float Weights[MAX_VERTICES];

	FEditorElement()
	{
		for (int32 ElementIndex = 0; ElementIndex < MAX_VERTICES; ElementIndex++)
		{
			Indices[ElementIndex] = INDEX_NONE;
			Weights[ElementIndex] = 0;
		}
	}
	
};

/** result of how much weight of the grid element **/
/** 网格元素权重的结果 **/
USTRUCT()
struct FGridBlendSample
{
	GENERATED_BODY()

	UPROPERTY()
	struct FEditorElement GridElement;

	UPROPERTY()
	float BlendWeight;

	FGridBlendSample()
		: BlendWeight(0)
	{
	}

};

USTRUCT()
struct FPerBoneInterpolation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=FPerBoneInterpolation)
	FBoneReference BoneReference;

	/**
	* If greater than zero, this is the speed at which the sample weights are allowed to change for this specific bone.
	*
	* A speed of 1 means a sample weight can change from zero to one (or one to zero) in one second.
	* A speed of 2 means that this would take half a second.
	*
	* Smaller values mean slower adjustments of the sample weights, and thus more smoothing. However, a
	* value of zero disables this smoothing entirely.
	* 
	* If set, the value overrides the overall Sample Weight Speed which will no longer affect this bone.
	*/
	/** 访问受保护的变量名称时需要 */
	UPROPERTY(EditAnywhere, Category=FPerBoneInterpolation, meta=(DisplayName="Weight Speed"))
	float InterpolationSpeedPerSec;

	FPerBoneInterpolation()
		: InterpolationSpeedPerSec(6.f)
	{}

	void Initialize(const USkeleton* Skeleton)
	/** 访问受保护的变量名称时需要 */
	{
		BoneReference.Initialize(Skeleton);
	}
};

UENUM()
namespace ENotifyTriggerMode
{
	enum Type : int
	{
		AllAnimations UMETA(DisplayName="All Animations"),
		HighestWeightedAnimation UMETA(DisplayName="Highest Weighted Animation"),
		None,
	};
}

/**
 * Allows multiple animations to be blended between based on input parameters
 */
UCLASS(config=Engine, hidecategories=Object, MinimalAPI, BlueprintType)
class UBlendSpace : public UAnimationAsset, public IInterpolationIndexProvider
{
	GENERATED_UCLASS_BODY()
public:

	/** Required for accessing protected variable names */
	/** 访问受保护的变量名称时需要 */
	friend class FBlendSpaceDetails;
	friend class FBlendSampleDetails;
	friend class UAnimGraphNode_BlendSpaceGraphBase;
	friend class SBlendSpaceGridWidget;

	//~ Begin UObject Interface
 // ~ 开始 UObject 接口
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~ End UObject Interface
 // ~ 结束 UObject 接口

	//~ Begin UAnimationAsset Interface
 // ~ 开始 UAnimationAsset 接口
	/** 返回给定的附加动画类型是否与混合空间类型兼容 */
	virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const override;
	// this is used in editor only when used for transition getter
 // 仅当用于转换 getter 时才在编辑器中使用
	// this doesn't mean max time. In Sequence, this is SequenceLength,
 // 这并不意味着最长时间。在序列中，这是 SequenceLength，
	// but for BlendSpace CurrentTime is normalized [0,1], so this is 1
 // 但对于 BlendSpace CurrentTime 是标准化的 [0,1]，所以这是 1
	/** 返回给定的附加动画类型是否与混合空间类型兼容 */
	virtual float GetPlayLength() const override { return 1.f; }
	virtual TArray<FName>* GetUniqueMarkerNames() override;
	virtual bool IsValidAdditive() const override;
#if WITH_EDITOR
	virtual bool GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets, bool bRecursive = true) override;
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap) override;
	virtual int32 GetMarkerUpdateCounter() const;
	void    RuntimeValidateMarkerData();
#endif
	//~ End UAnimationAsset Interface
 // ~ 结束 UAnimationAsset 接口
	
	// Begin IInterpolationIndexProvider Overrides
 // 开始 IInterpolationIndexProvider 覆盖

	/**
	/** 混合参数的访问器 **/
	 * Sorts the PerBoneBlend data into a form that can be repeatedly used in GetPerBoneInterpolationIndex
	 */
	/** 混合参数的访问器 **/
	/** 获取此混合空间示例数据 */
	virtual TSharedPtr<IInterpolationIndexProvider::FPerBoneInterpolationData> GetPerBoneInterpolationData(const USkeleton* Skeleton) const override;

	/**
	/** 返回给定索引处的混合样本，将对无效索引进行断言 */
	/** 获取此混合空间示例数据 */
	* Get PerBoneInterpolationIndex for the input BoneIndex
	* If nothing found, return INDEX_NONE
	*/
	/** 返回给定索引处的混合样本，将对无效索引进行断言 */
	virtual int32 GetPerBoneInterpolationIndex(const FCompactPoseBoneIndex& InCompactPoseBoneIndex, const FBoneContainer& RequiredBones, const IInterpolationIndexProvider::FPerBoneInterpolationData* Data) const override;

	/**
	* Get PerBoneInterpolationIndex for the input BoneIndex
	* If nothing found, return INDEX_NONE
	*/
	virtual int32 GetPerBoneInterpolationIndex(const FSkeletonPoseBoneIndex InSkeletonBoneIndex, const USkeleton* TargetSkeleton, const IInterpolationIndexProvider::FPerBoneInterpolationData* Data) const override;
	// End IInterpolationIndexProvider Overrides
 // 结束 IInterpolationIndexProvider 覆盖

	/** Returns whether or not the given additive animation type is compatible with the blendspace type */
	/** 从样本数据列表计算动画长度的实用函数 **/
	/** 返回给定的附加动画类型是否与混合空间类型兼容 */
	ENGINE_API virtual bool IsValidAdditiveType(EAdditiveAnimationType AdditiveType) const;

	/**
	/** 从样本数据列表计算动画长度的实用函数 **/
	 * BlendSpace Get Animation Pose function
	 */
	UE_DEPRECATED(4.26, "Use GetAnimationPose with other signature")
	ENGINE_API void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, /*out*/ FCompactPose& OutPose, /*out*/ FBlendedCurve& OutCurve) const;
	/** 更新 BlendSpace 过滤参数 - 不需要完全初始化的值 **/
	
	UE_DEPRECATED(5.0, "Use GetAnimationPose with extraction context signature")
	ENGINE_API void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, /*out*/ FAnimationPoseData& OutAnimationPoseData) const;
	/** 返回夹紧和/或包裹后的混合输入 */

	UE_DEPRECATED(5.0, "Use GetAnimationPose with extraction context signature")
	/** 更新 BlendSpace 过滤参数 - 不需要完全初始化的值 **/
	ENGINE_API void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, TArrayView<FPoseLink> InPoseLinks, /*out*/ FPoseContext& Output) const;

	ENGINE_API void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, const FAnimExtractContext& ExtractionContext, /*out*/ FAnimationPoseData& OutAnimationPoseData) const;
	/** 返回夹紧和/或包裹后的混合输入 */

	ENGINE_API void GetAnimationPose(TArray<FBlendSampleData>& BlendSampleDataCache, TArrayView<FPoseLink> InPoseLinks, const FAnimExtractContext& ExtractionContext, /*out*/ FPoseContext& Output) const;

	/** Accessor for blend parameter **/
	/** 混合参数的访问器 **/
	ENGINE_API const FBlendParameter& GetBlendParameter(const int32 Index) const;

	/** Get this blend spaces sample data */
	/** 获取此混合空间示例数据 */
	const TArray<struct FBlendSample>& GetBlendSamples() const { return SampleData; }

	/** Returns the Blend Sample at the given index, will assert on invalid indices */
	/** 返回给定索引处的混合样本，将对无效索引进行断言 */
	ENGINE_API const struct FBlendSample& GetBlendSample(const int32 SampleIndex) const;

	/**
	* Get Grid Samples from BlendInput
	* It will return all samples that has weight > KINDA_SMALL_NUMBER
	*
	* @param	BlendInput	BlendInput X, Y, Z corresponds to BlendParameters[0], [1], [2]
	* @param	InOutCachedTriangulationIndex	The previous index into triangulation/segmentation to warm-start the search. 
	/** 基于 Filter 数据对 BlendInput 进行插值 **/
	* @param	bCombineAnimations	Will combine samples that point to the same animation. Useful when processing, but confusing when viewing
	*
	* @return	true if it has valid OutSampleDataList, false otherwise
	/** 计算轴比例因子。请参见 AxisToScaleAnimation。 */
	*/
	ENGINE_API bool GetSamplesFromBlendInput(const FVector &BlendInput, TArray<FBlendSampleData> & OutSampleDataList, int32& InOutCachedTriangulationIndex, bool bCombineAnimations) const;

	/** Utility function to calculate animation length from sample data list **/
	/** 使用给定的动画序列验证混合空间的样本数据 */
	/** 基于 Filter 数据对 BlendInput 进行插值 **/
	/** 从样本数据列表计算动画长度的实用函数 **/
	ENGINE_API float GetAnimationLengthFromSampleData(const TArray<FBlendSampleData>& SampleDataList) const;
	/** 验证包含的数据 */

	/** 计算轴比例因子。请参见 AxisToScaleAnimation。 */
	/** 
	/** 添加样品 */
	 * Initialize BlendSpace filtering for runtime. Filtering supports multiple dimensions, defaulting to
	 * two (since we don't have 3D BlendSpaces yet) 
	**/
	/** 使用给定的动画序列验证混合空间的样本数据 */
	ENGINE_API void InitializeFilter(FBlendFilter* Filter, int NumDimensions = 2) const;

	/** 编辑样本 */
	/** Update BlendSpace filtering parameters - values that don't require a full initialization **/
	/** 验证包含的数据 */
	/** 更新 BlendSpace 过滤参数 - 不需要完全初始化的值 **/
	ENGINE_API void UpdateFilterParams(FBlendFilter* Filter) const;

	/** 添加样品 */
	/** 更新网格示例上的动画 */
	/** Returns the blend input after clamping and/or wrapping */
	/** 返回夹紧和/或包裹后的混合输入 */
	ENGINE_API FVector GetClampedAndWrappedBlendInput(const FVector& BlendInput) const;
	/** 删除样本 */

	/** 
	 * Updates a cached set of blend samples according to internal parameters, blendspace position and a delta time. Used internally to GetAnimationPose().
	/** 获取此混合空间的样本点数 */
	/** 编辑样本 */
	 * Note that this function does not perform any filtering internally.
 	 * @param	InBlendSpacePosition	The current position parameter of the blendspace
	/** 结合存储的样本数据检查样本索引是否有效 */
	 * @param	InOutSampleDataCache	The sample data cache. Previous frames samples are re-used in the case of target weight interpolation
	 * @param	InDeltaTime				The tick time for this update
	 */
	ENGINE_API bool UpdateBlendSamples(const FVector& InBlendSpacePosition, float InDeltaTime, TArray<FBlendSampleData>& InOutSampleDataCache, int32& InOutCachedTriangulationIndex) const;
	/** 更新网格示例上的动画 */
	
	/**
	 * Resets a cached set of blend samples to match a given input time. All samples will be advanced using sync marker if possible, otherwise, their time will just be set match the input normalized time.
	/** 删除样本 */
	 * 
	 * @param	InOutSampleDataCache			The sample data cache to use.
	 * @param	InNormalizedCurrentTime			The time to match when advancing samples. 
	/** 返回与 GetGridSamples 返回的元素关联的样本位置 */
	/** 获取此混合空间的样本点数 */
	 * @param	bLooping						If true, advance samples as a looping blend space would.
	 * @param	bMatchSyncPhases				If true, all follower samples will pass the same amount of markers the leader sample has passed to match its sync phase. Otherwise, followers samples will only match their next valid sync position.  
	/** 返回与坐标关联的样本位置 */
	 */
	/** 结合存储的样本数据检查样本索引是否有效 */
	ENGINE_API void ResetBlendSamples(TArray<FBlendSampleData>& InOutSampleDataCache, float InNormalizedCurrentTime, bool bLooping, bool bMatchSyncPhases = true) const;

	/**
	 * Allows the user to iterate through all the data samples available in the blend space.
	 * @param Func The function to run for each blend sample
	 */
	ENGINE_API void ForEachImmutableSample(const TFunctionRef<void(const FBlendSample&)> Func) const;
	
	/** Interpolate BlendInput based on Filter data **/
	/** 基于 Filter 数据对 BlendInput 进行插值 **/
	ENGINE_API FVector FilterInput(FBlendFilter* Filter, const FVector& BlendInput, float DeltaTime) const;

	/** 返回与 GetGridSamples 返回的元素关联的样本位置 */
	/** Computes the axis scale factor. See AxisToScaleAnimation. */
	/** 计算轴比例因子。请参见 AxisToScaleAnimation。 */
	ENGINE_API float ComputeAxisScaleFactor(const FVector& BlendSpacePosition, const FVector& FilteredBlendInput) const;
	/** 验证给定的动画序列和包含的混合空间数据 */
	/** 返回与坐标关联的样本位置 */

#if WITH_EDITOR	
	/** 检查混合空间是否包含附加类型与动画序列匹配的样本 */
	/** Validates sample data for blendspaces using the given animation sequence */
	/** 使用给定的动画序列验证混合空间的样本数据 */
	ENGINE_API static void UpdateBlendSpacesUsingAnimSequence(UAnimSequenceBase* Sequence);
	/** 检查混合空间是否仅包含附加样本 */

	/** Validates the contained data */
	/** 验证包含的数据 */
	/** 检查动画序列的骨架是否与此混合空间兼容 */
	ENGINE_API void ValidateSampleData();

	/** Add samples */
	/** 检查动画序列附加类型是否与此混合空间兼容 */
	/** 添加样品 */
	ENGINE_API int32 AddSample(const FVector& SampleValue);
	ENGINE_API int32 AddSample(UAnimSequence* AnimationSequence, const FVector& SampleValue);
	/** 根据混合空间的当前内容验证提供的混合样本 */

	ENGINE_API void ExpandRangeForSample(const FVector& SampleValue);
	
	/** edit samples */
	/** 验证给定的动画序列和包含的混合空间数据 */
	/** 检查给定的样本值是否不太接近现有样本点 **/
	/** 编辑样本 */
	ENGINE_API bool	EditSampleValue(const int32 BlendSampleIndex, const FVector& NewValue);

	/** 检查混合空间是否包含附加类型与动画序列匹配的样本 */
	UE_DEPRECATED(5.0, "Please use ReplaceSampleAnimation instead")
	ENGINE_API bool	UpdateSampleAnimation(UAnimSequence* AnimationSequence, const FVector& SampleValue);

	/** 检查混合空间是否仅包含附加样本 */
	/** update animation on grid sample */
	/** 更新网格示例上的动画 */
	ENGINE_API bool	ReplaceSampleAnimation(const int32 BlendSampleIndex, UAnimSequence* AnimationSequence);
	/** 检查动画序列的骨架是否与此混合空间兼容 */

	/** delete samples */
	/** 返回可用于缩放动画速度的轴。 **/
	/** 删除样本 */
	/** 检查动画序列附加类型是否与此混合空间兼容 */
	ENGINE_API bool	DeleteSample(const int32 BlendSampleIndex);
	/** 初始化每个骨骼混合 **/
	
	/** Get the number of sample points for this blend space */
	/** 根据混合空间的当前内容验证提供的混合样本 */
	/** 勾选 SampleDataList 中除 HighestWeightIndex 之外的样本。 */
	/** 获取此混合空间的样本点数 */
	int32 GetNumberOfBlendSamples()  const { return SampleData.Num(); }

	/** Check whether or not the sample index is valid in combination with the stored sample data */
	/** 结合存储的样本数据检查样本索引是否有效 */
	/** 返回钳位到有效范围的混合输入，除非该轴已设置为换行，在这种情况下不会进行钳位 **/
	/** 检查给定的样本值是否不太接近现有样本点 **/
	ENGINE_API bool IsValidBlendSampleIndex(const int32 SampleIndex) const;

	/** 将 BlendInput 转换为网格空间 */
	/**
	* return GridSamples from this BlendSpace
	*
	/** 将 BlendInput 转换为网格空间 */
	* @param	OutGridElements
	*
	* @return	Number of OutGridElements
	/** 返回索引处的网格元素，如果索引无效，则返回 NULL */
	*/
	ENGINE_API const TArray<FEditorElement>& GetGridSamples() const;

	/** 用于将样本权重从 OldSampleDataList 插值到 NewSampleDataList 并将插值结果复制回 FinalSampleDataList 的实用函数 **/
	/** Returns the sample position associated with the elements returned by GetGridSamples */
	/** 返回与 GetGridSamples 返回的元素关联的样本位置 */
	ENGINE_API FVector GetGridPosition(int32 GridIndex) const;
	/** 返回混合空间样本上设置的所有动画是否与给定的附加类型匹配 */
	/** 返回可用于缩放动画速度的轴。 **/

	/** Returns the sample position associated with the coordinates */
	/** 检查给定的样本点是否重叠 */
	/** 返回与坐标关联的样本位置 */
	/** 初始化每个骨骼混合 **/
	ENGINE_API FVector GetGridPosition(int32 GridX, int32 GridY) const;

	/**
	/** 勾选 SampleDataList 中除 HighestWeightIndex 之外的样本。 */
	/** 如果在边界周围，则捕捉到边界以避免无效数据的空洞 **/
	 * Returns the runtime triangulation etc data
	 */
	ENGINE_API const FBlendSpaceData& GetBlendSpaceData() const;

	/**
	/** 返回钳位到有效范围的混合输入，除非该轴已设置为换行，在这种情况下不会进行钳位 **/
	 * Runs triangulation/segmentation to update our grid and BlendSpaceData structures
	 */
	ENGINE_API void ResampleData();
	/** 将 BlendInput 转换为网格空间 */

	/**
	 * Sets up BlendSpaceData based on Line elements
	/** 将 BlendInput 转换为网格空间 */
	 */
	ENGINE_API void SetBlendSpaceData(const TArray<FBlendSpaceSegment>& Segments);

	/** 返回索引处的网格元素，如果索引无效，则返回 NULL */
	/** 指示是否有任何样本具有在网格空间中应用旋转偏移的标志 */
	/** Validate that the given animation sequence and contained blendspace data */
	/** 验证给定的动画序列和包含的混合空间数据 */
	ENGINE_API bool ValidateAnimationSequence(const UAnimSequence* AnimationSequence) const;
	/** 用于将样本权重从 OldSampleDataList 插值到 NewSampleDataList 并将插值结果复制回 FinalSampleDataList 的实用函数 **/
	/** 每个输入轴的输入平滑参数 */

	/** Check if the blend spaces contains samples whos additive type match that of the animation sequence */
	/** 检查混合空间是否包含附加类型与动画序列匹配的样本 */
	/** 返回混合空间样本上设置的所有动画是否与给定的附加类型匹配 */
	ENGINE_API bool DoesAnimationMatchExistingSamples(const UAnimSequence* AnimationSequence) const;
	
	/** Check if the the blendspace contains additive samples only */	
	/** 检查给定的样本点是否重叠 */
	/** 检查混合空间是否仅包含附加样本 */
	ENGINE_API bool ShouldAnimationBeAdditive() const;

	/** Check if the animation sequence's skeleton is compatible with this blendspace */
	/** 缓存属性用于在新创建时初始化属性。 */
	/** 检查动画序列的骨架是否与此混合空间兼容 */
	ENGINE_API bool IsAnimationCompatibleWithSkeleton(const UAnimSequence* AnimationSequence) const;
	/** 如果在边界周围，则捕捉到边界以避免无效数据的空洞 **/

	/** Check if the animation sequence additive type is compatible with this blend space */
	/** 检查动画序列附加类型是否与此混合空间兼容 */
	ENGINE_API bool IsAnimationCompatible(const UAnimSequence* AnimationSequence) const;

	/** Validates supplied blend sample against current contents of blendspace */
	/** 根据混合空间的当前内容验证提供的混合样本 */
	ENGINE_API bool ValidateSampleValue(const FVector& SampleValue, int32 OriginalIndex = INDEX_NONE) const;

	ENGINE_API bool IsSampleWithinBounds(const FVector &SampleValue) const;

	/** Check if given sample value isn't too close to existing sample point **/
	/** 检查给定的样本值是否不太接近现有样本点 **/
	ENGINE_API bool IsTooCloseToExistingSamplePoint(const FVector& SampleValue, int32 OriginalIndex) const;
#endif

protected:
	/**
	/** 指示是否有任何样本具有在网格空间中应用旋转偏移的标志 */
	* Get Grid Samples from BlendInput, From Input, it will populate OutGridSamples with the closest grid points.
	*
	* @param	BlendInput			BlendInput X, Y, Z corresponds to BlendParameters[0], [1], [2]
	* @param	OutBlendSamples		Populated with the samples nearest the BlendInput
	/** 每个输入轴的输入平滑参数 */
	*
	*/
	void GetRawSamplesFromBlendInput(const FVector& BlendInput, TArray<FGridBlendSample, TInlineAllocator<4> >& OutBlendSamples) const;

	/** Returns the axis which can be used to scale animation speed. **/
	/** 返回可用于缩放动画速度的轴。 **/
	virtual EBlendSpaceAxis GetAxisToScale() const { return AxisToScaleAnimation; }

	/** Initialize Per Bone Blend **/
	/** 初始化每个骨骼混合 **/
	void InitializePerBoneBlend();

	/** 缓存属性用于在新创建时初始化属性。 */
	/** Ticks the samples in SampleDataList apart from the HighestWeightIndex one. */
	/** 勾选 SampleDataList 中除 HighestWeightIndex 之外的样本。 */
	void TickFollowerSamples(
		TArray<FBlendSampleData> &SampleDataList, const int32 HighestWeightIndex, FAnimAssetTickContext &Context, 
		bool bResetMarkerDataOnFollowers, bool bLooping, const UMirrorDataTable* MirrorDataTable = nullptr) const;

	/** Returns the blend input clamped to the valid range, unless that axis has been set to wrap in which case no clamping is done **/
	/** 返回钳位到有效范围的混合输入，除非该轴已设置为换行，在这种情况下不会进行钳位 **/
	FVector GetClampedBlendInput(const FVector& BlendInput) const;
	
	/** Translates BlendInput to grid space */
	/** 将 BlendInput 转换为网格空间 */
	/** 是否允许样本之间基于标记的同步（如果标记不存在，则不会强制同步） */
	FVector ConvertBlendInputToGridSpace(const FVector& BlendInput) const;

	/** Translates BlendInput to grid space */
	/** 将 BlendInput 转换为网格空间 */
	/** 如果为真，则所有跟随者样本将通过与领导者样本相同数量的标记以匹配其同步相位。否则，跟随者样本将仅匹配其下一个有效同步位置。 */
	FVector GetNormalizedBlendInput(const FVector& BlendInput) const;

	/** Returns the grid element at Index or NULL if Index is not valid */
	/** 返回索引处的网格元素，如果索引无效，则返回 NULL */
	const FEditorElement* GetGridSampleInternal(int32 Index) const;
	/** 预览附加 BlendSpace 的基本姿势 **/
	
	/** Utility function to interpolate weight of samples from OldSampleDataList to NewSampleDataList and copy back the interpolated result to FinalSampleDataList **/
	/** 用于将样本权重从 OldSampleDataList 插值到 NewSampleDataList 并将插值结果复制回 FinalSampleDataList 的实用函数 **/
	bool InterpolateWeightOfSampleData(float DeltaTime, const TArray<FBlendSampleData> & OldSampleDataList, const TArray<FBlendSampleData> & NewSampleDataList, TArray<FBlendSampleData> & FinalSampleDataList) const;

	/** 这是混合空间中任何样本的最大长度。 **/
	/** Returns whether or not all animation set on the blend space samples match the given additive type */
	/** 返回混合空间样本上设置的所有动画是否与给定的附加类型匹配 */
	bool ContainsMatchingSamples(EAdditiveAnimationType AdditiveType) const;

	/** Checks if the given samples points overlap */
	/** 检查给定的样本点是否重叠 */
	bool IsSameSamplePoint(const FVector& SamplePointA, const FVector& SamplePointB) const;	

#if WITH_EDITOR
	bool ContainsNonAdditiveSamples() const;
	void UpdatePreviewBasePose();
	/** If around border, snap to the border to avoid empty hole of data that is not valid **/
	/** 如果为 true，则在运行时通过网格完成插值。如果为 false，则插值使用三角测量。 */
	/** 如果在边界周围，则捕捉到边界以避免无效数据的空洞 **/
	virtual void SnapSamplesToClosestGridPoint();
#endif // WITH_EDITOR

	/** 当三角测量必须做出任意选择时的首选边缘方向 */
	ENGINE_API static bool IsSingleFrameBlendingIndexInBounds(const FBlendSample& BlendSample);
private:
	// Internal helper function for GetAnimationPose variants
 // GetAnimationPose 变体的内部辅助函数
	void GetAnimationPose_Internal(TArray<FBlendSampleData>& BlendSampleDataCache, TArrayView<FPoseLink> InPoseLinks, FAnimInstanceProxy* InProxy, bool bInExpectsAdditivePose, const FAnimExtractContext& ExtractionContext, /*out*/ FAnimationPoseData& OutAnimationPoseData) const;

	// Internal helper function for UpdateBlendSamples and TickAssetPlayer
 // UpdateBlendSamples 和 TickAssetPlayer 的内部辅助函数
	bool UpdateBlendSamples_Internal(const FVector& InBlendSpacePosition, float InDeltaTime, TArray<FBlendSampleData>& InOutOldSampleDataList, TArray<FBlendSampleData>& InOutSampleDataCache, int32& InOutCachedTriangulationIndex) const;	

	/** 是否允许样本之间基于标记的同步（如果标记不存在，则不会强制同步） */
public:
#if WITH_EDITORONLY_DATA
	UE_DEPRECATED(5.1, "This property is deprecated. Please use/see bContainsRotationOffsetMeshSpaceSamples instead")
	bool bRotationBlendInMeshSpace_DEPRECATED;
	/** 如果为真，则所有跟随者样本将通过与领导者样本相同数量的标记以匹配其同步相位。否则，跟随者样本将仅匹配其下一个有效同步位置。 */
#endif

	/** Indicates whether any samples have the flag to apply rotation offsets in mesh space */
	/** 指示是否有任何样本具有在网格空间中应用旋转偏移的标志 */
	UPROPERTY()
	/** 预览附加 BlendSpace 的基本姿势 **/
	bool bContainsRotationOffsetMeshSpaceSamples;

	/** Input Smoothing parameters for each input axis */
	/** 每个输入轴的输入平滑参数 */
	UPROPERTY(EditAnywhere, Category = InputInterpolation)
	/** 这是混合空间中任何样本的最大长度。 **/
	FInterpolationParameter	InterpolationParam[3];

#if WITH_EDITORONLY_DATA
	/** 
	* Analysis properties for each axis. Note that these can be null. They will be created/set from 
	* FBlendSpaceDetails::HandleAnalysisFunctionChanged
	*/
	UPROPERTY(EditAnywhere, Category = AnalysisProperties)
	TObjectPtr<UAnalysisProperties> AnalysisProperties[3];

	/** Cached properties used to initialize properties when newly created. */
	/** 缓存属性用于在新创建时初始化属性。 */
	/** 示例动画数据 */
	/** 如果为 true，则在运行时通过网格完成插值。如果为 false，则插值使用三角测量。 */
	TObjectPtr<UCachedAnalysisProperties> CachedAnalysisProperties[3];
#endif

	/** 网格样本，子类强加的索引方案 */
	/**
	/** 当三角测量必须做出任意选择时的首选边缘方向 */
	* If greater than zero, this is the speed at which the sample weights are allowed to change.
	* 
	/** 运行时数据的容器，可以是线段、三角剖分或四面体 */
	* A speed of 1 means a sample weight can change from zero to one (or one to zero) in one second.
	* A speed of 2 means that this would take half a second.
	* 
	* This allows the Blend Space to switch to new parameters without going through intermediate states, 
	/** 每个轴的混合参数。 **/
	* effectively blending between where it was and where the new target is. For example, imagine we have 
	* a blend space for locomotion, moving left, forward and right. Now if you interpolate the inputs of 
	* the blend space itself, from one extreme to the other, you will go from left, to forward, to right. 
	* As an alternative, by setting this Sample Weight Speed to a value higher than zero, it will go 
	* directly from left to right, without going through moving forward first.
	* 
	* Smaller values mean slower adjustments of the sample weights, and thus more smoothing. However, a 
	* value of zero disables this smoothing entirely.
	*/
	UPROPERTY(EditAnywhere, Category = SampleSmoothing, meta = (DisplayName = "Weight Speed", ClampMin = "0"))
	float TargetWeightInterpolationSpeedPerSec = 0.0f;

	/** 重置为参考姿势。它确实根据添加剂或不应用不同的反射 */
	/**
	 * If set then this eases in/out the sample weight adjustments, using the speed to determine how much smoothing to apply.
	 */
	/** 使用数据中维度的顺序 - 例如[1, 2] 表示使用 Y 和 Z 的 2D 混合 */
	UPROPERTY(EditAnywhere, Category = SampleSmoothing, meta = (DisplayName = "Smoothing"))
	bool bTargetWeightInterpolationEaseInOut = true;

	/**
	 * If set then blending is performed in mesh space if there are per-bone sample smoothing overrides.
	 * 
	 * Note that mesh space blending is significantly more expensive (slower) than normal blending when the 
	 * samples are regular animations (i.e. not additive animations that are already set to apply in mesh 
	 * space), and is typically only useful if you want some parts of the skeleton to achieve a pose 
	 * in mesh space faster or slower than others - for example to make the head move faster than the 
	 * body/arms when aiming, so the character looks at the target slightly before aiming at it.
	 * 
	 * Note also that blend space assets with additive/mesh space samples will always blend in mesh space, and 
	 * also that enabling this option with blend space graphs producing additive/mesh space samples may cause
	 * undesired results.
	 */
	UPROPERTY(EditAnywhere, Category = SampleSmoothing)
	/** 示例动画数据 */
	bool bAllowMeshSpaceBlending = false;

	/** 
	* The default looping behavior of this blend space.
	/** 网格样本，子类强加的索引方案 */
	* Asset players can override this
	*/
	UPROPERTY(EditAnywhere, Category=Animation)
	bool bLoop = true;
	/** 运行时数据的容器，可以是线段、三角剖分或四面体 */

	/** Whether to allow marker based sync between the samples (it won't force sync if the markers don't exist) */
	/** 是否允许样本之间基于标记的同步（如果标记不存在，则不会强制同步） */
	UPROPERTY(EditAnywhere, Category = Animation)
	/** 每个轴的混合参数。 **/
	bool bAllowMarkerBasedSync = true;

	/** If true, all follower samples will pass the same amount of markers the leader sample has passed to match its sync phase. Otherwise, followers samples will only match their next valid sync position. */
	/** 如果为真，则所有跟随者样本将通过与领导者样本相同数量的标记以匹配其同步相位。否则，跟随者样本将仅匹配其下一个有效同步位置。 */
	UPROPERTY(EditAnywhere, Category = Animation)
	bool bShouldMatchSyncPhases = false;
	
#if WITH_EDITORONLY_DATA
	/** Preview Base pose for additive BlendSpace **/
	/** 预览附加 BlendSpace 的基本姿势 **/
	UPROPERTY(EditAnywhere, Category = AdditiveSettings)
	TObjectPtr<UAnimSequence> PreviewBasePose;
	/** 重置为参考姿势。它确实根据添加剂或不应用不同的反射 */
#endif // WITH_EDITORONLY_DATA

	/** This is the maximum length of any sample in the blendspace. **/
	/** 使用数据中维度的顺序 - 例如[1, 2] 表示使用 Y 和 Z 的 2D 混合 */
	/** 这是混合空间中任何样本的最大长度。 **/
	UPROPERTY()
	float AnimLength;

	/** The current mode used by the BlendSpace to decide which animation notifies to fire. Valid options are:
	- AllAnimations: All notify events will fire
	- HighestWeightedAnimation: Notify events will only fire from the highest weighted animation
	- None: No notify events will fire from any animations
	*/
	UPROPERTY(EditAnywhere, Category = AnimationNotifies)
	TEnumAsByte<ENotifyTriggerMode::Type> NotifyTriggerMode;

	/** If true then interpolation is done via a grid at runtime. If false the interpolation uses the triangulation. */
	/** 如果为 true，则在运行时通过网格完成插值。如果为 false，则插值使用三角测量。 */
	UPROPERTY(EditAnywhere, Category = InputInterpolation, meta = (DisplayName="Use Grid"))
	bool bInterpolateUsingGrid = false;

	/** Preferred edge direction when the triangulation has to make an arbitrary choice */
	/** 当三角测量必须做出任意选择时的首选边缘方向 */
	UPROPERTY(EditAnywhere, Category = InputInterpolation)
	EPreferredTriangulationDirection PreferredTriangulationDirection = EPreferredTriangulationDirection::Tangential;

protected:

	/**
	 * There are two ways to use per pone sample smoothing: Blend profiles and manually maintaining the per bone overrides.
	 */
	UPROPERTY(EditAnywhere, Category = SampleSmoothing)
	EBlendSpacePerBoneBlendMode PerBoneBlendMode = EBlendSpacePerBoneBlendMode::ManualPerBoneOverride;

	/**
	 * Per bone sample smoothing settings, which affect the specified bone and all its descendants in the skeleton.
	 * These act as overrides to the global sample smoothing speed, which means the global sample smoothing speed does
	 * not affect these bones. Note that they also override each other - so a per-bone setting on the chest will not
	 * affect the hand if there is a per-bone setting on the arm.
	 */
	UPROPERTY(EditAnywhere, Category = SampleSmoothing, meta = (DisplayName="Per Bone Overrides", EditCondition = "PerBoneBlendMode == EBlendSpacePerBoneBlendMode::ManualPerBoneOverride", EditConditionHides))
	TArray<FPerBoneInterpolation> ManualPerBoneOverrides;

	/**
	 * Reference to a blend profile of the corresponding skeleton to be used for per bone smoothing in case the per bone blend mode is set to use a blend profile.
	 **/
	UPROPERTY(EditAnywhere, Category = SampleSmoothing, meta = (DisplayName = "Per Bone Overrides", EditCondition = "PerBoneBlendMode == EBlendSpacePerBoneBlendMode::BlendProfile", EditConditionHides))
	FBlendSpaceBlendProfile PerBoneBlendProfile;

	/**
	 * Stores the actual bone references and their smoothing interpolation speeds used by the blend space. This will be either filled by the manual per bone overrides or
	 * the blend profile, depending on the set per bone blend mode.
	 **/
	TArray<FPerBoneInterpolation> PerBoneBlendValues;

	/** Track index to get marker data from. Samples are tested for the suitability of marker based sync
	    during load and if we can use marker based sync we cache an index to a representative sample here */
	UPROPERTY()
	int32 SampleIndexWithMarkers = INDEX_NONE;

	/** Sample animation data */
	/** 示例动画数据 */
	UPROPERTY(EditAnywhere, Category=BlendSamples)
	TArray<struct FBlendSample> SampleData;

	/** Grid samples, indexing scheme imposed by subclass */
	/** 网格样本，子类强加的索引方案 */
	UPROPERTY()
	TArray<struct FEditorElement> GridSamples;

	/** Container for the runtime data, which could be line segments, triangulation or tetrahedrons */
	/** 运行时数据的容器，可以是线段、三角剖分或四面体 */
	UPROPERTY()
	FBlendSpaceData BlendSpaceData;
	
	/** Blend Parameters for each axis. **/
	/** 每个轴的混合参数。 **/
	UPROPERTY(EditAnywhere, Category = BlendParametersTest)
	struct FBlendParameter BlendParameters[3];

	/**
	 * If you have input smoothing, this specifies the axis on which to scale the animation playback speed. E.g. for 
	 * locomotion animation, the speed axis will scale the animation speed in order to make up the difference 
	 * between the target and the result of blending the samples.
	 */
	UPROPERTY(EditAnywhere, Category = InputInterpolation)
	TEnumAsByte<EBlendSpaceAxis> AxisToScaleAnimation;

	/** Reset to reference pose. It does apply different refpose based on additive or not */
	/** 重置为参考姿势。它确实根据添加剂或不应用不同的反射 */
	void ResetToRefPose(FCompactPose& OutPose) const;

	/** The order in which to use the dimensions in the data - e.g. [1, 2] means a 2D blend using Y and Z */
	/** 使用数据中维度的顺序 - 例如[1, 2] 表示使用 Y 和 Z 的 2D 混合 */
	UPROPERTY()
	TArray<int32> DimensionIndices;

#if WITH_EDITOR
private:
	// Track whether we have updated markers so cached data can be updated
 // 跟踪我们是否更新了标记，以便可以更新缓存的数据
	int32 MarkerDataUpdateCounter;
protected:
	FVector PreviousAxisMinMaxValues[3];
	float   PreviousGridSpacings[3];
#endif	

private:

	void GetRawSamplesFromBlendInput1D(const FVector& BlendInput, TArray<FGridBlendSample, TInlineAllocator<4> >& OutBlendSamples) const;

	void GetRawSamplesFromBlendInput2D(const FVector& BlendInput, TArray<FGridBlendSample, TInlineAllocator<4> >& OutBlendSamples) const;

	/** Fill up local GridElements from the grid elements that are created using the sorted points
	*	This will map back to original index for result
	*
	*  @param	SortedPointList		This is the pointlist that are used to create the given GridElements
	*								This list contains subsets of the points it originally requested for visualization and sorted
	*
	*/
	void FillupGridElements(const TArray<FEditorElement>& GridElements, const TArray<int32>& InDimensionIndices);

	void EmptyGridElements();

	void ClearBlendSpaceData();

	void SetBlendSpaceData(const TArray<FBlendSpaceTriangle>& Triangles);

	void ResampleData1D();
	void ResampleData2D();


	/** Get the Editor Element from Index
	*
	* @param	XIndex	Index of X
	* @param	YIndex	Index of Y
	*
	* @return	FEditorElement * return the grid data
	*/
	const FEditorElement* GetEditorElement(int32 XIndex, int32 YIndex) const;
};
