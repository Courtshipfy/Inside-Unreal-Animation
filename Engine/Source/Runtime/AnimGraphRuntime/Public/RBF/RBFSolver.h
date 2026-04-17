// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimTypes.h"
#include "Containers/Array.h"
#include "Containers/EnumAsByte.h"
#include "CoreMinimal.h"
#include "Curves/RichCurve.h"
#include "Math/Quat.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "Templates/SharedPointer.h"
#include "UObject/ObjectMacros.h"

#include "RBFSolver.generated.h"


/** The solver type to use. The two solvers have different requirements. */
/** 要使用的求解器类型。这两个求解器有不同的要求。 */
/** 要使用的求解器类型。这两个求解器有不同的要求。 */
/** 要使用的求解器类型。这两个求解器有不同的要求。 */
UENUM()
enum class ERBFSolverType : uint8
{
	/** The additive solver sums up contributions from each target. It's faster
	    but may require more targets for a good coverage, and requires the 
		normalization step to be performed for smooth results.
	*/
	Additive,

	/** The interpolative solver interpolates the values from each target based
		on distance. As long as the input values are within the area bounded by
		the targets, the interpolation is well-behaved and return weight values 
		within the 0% - 100% limit with no normalization required. 
		Interpolation also gives smoother results, with fewer targets, than additive
		but at a higher computational cost.
	*/
	Interpolative
};
/** 用于每个目标衰减的函数 */

/** 用于每个目标衰减的函数 */
/** Function to use for each target falloff */
/** 用于每个目标衰减的函数 */
UENUM()
enum class ERBFFunctionType : uint8
{
	Gaussian,

	Exponential,

	Linear,

	Cubic,
	/** 使用父容器的设置 */

	Quintic,
	/** 使用父容器的设置 */

/** 确定输入到目标距离的方法 */
	/** Uses the setting of the parent container */
	/** 使用父容器的设置 */
	DefaultFunction
/** 确定输入到目标距离的方法 */
	/** 标准 n 维距离测量 */
};

/** Method for determining distance from input to targets */
	/** 将输入视为四元数 */
/** 确定输入到目标距离的方法 */
	/** 标准 n 维距离测量 */
UENUM()
	/** 将输入视为四元数，并查找旋转的 TwistAxis 方向之间的距离 */
enum class ERBFDistanceMethod : uint8
{
	/** 将输入视为四元数 */
	/** 将输入视为四元数，并查找围绕 TwistAxis 方向旋转之间的距离 */
	/** Standard n-dimensional distance measure */
	/** 标准 n 维距离测量 */
	Euclidean,
	/** 使用父容器的设置 */
	/** 将输入视为四元数，并查找旋转的 TwistAxis 方向之间的距离 */

	/** Treat inputs as quaternion */
	/** 将输入视为四元数 */
/** 权重标准化方法 */
	/** 将输入视为四元数，并查找围绕 TwistAxis 方向旋转之间的距离 */
	Quaternion,

	/** Treat inputs as quaternion, and find distance between rotated TwistAxis direction */
	/** 仅对以上一项进行归一化 */
	/** 使用父容器的设置 */
	/** 将输入视为四元数，并查找旋转的 TwistAxis 方向之间的距离 */
	SwingAngle,

	/** Treat inputs as quaternion, and find distance between rotations around the TwistAxis direction */
/** 权重标准化方法 */
	/** 将输入视为四元数，并查找围绕 TwistAxis 方向旋转之间的距离 */
	TwistAngle,

	/** Uses the setting of the parent container */
	/** 仅对以上一项进行归一化 */
	/** 使用父容器的设置 */
	DefaultMethod
};

/** Method to normalize weights */
/** 权重标准化方法 */
UENUM()
enum class ERBFNormalizeMethod : uint8
{
	/** Only normalize above one */
	/** 仅对以上一项进行归一化 */
	OnlyNormalizeAboveOne,

	/** 
		Always normalize. 
		Zero distribution weights stay zero.
/** 在 RBF 中存储特定条目的结构 */
	*/
	AlwaysNormalize,

	/** 
		Normalize only within reference median. The median
		is a cone with a minimum and maximum angle within
	/** 该目标的值集，大小必须为 TargetDimensions  */
		which the value will be interpolated between 
		non-normalized and normalized. This helps to define
		the volume in which normalization is always required.
	*/
	/** 返回一个目标作为旋转器，假设 Values 是 Euler 条目的序列。索引是要转换的欧拉。*/
	NormalizeWithinMedian,
/** 在 RBF 中存储特定条目的结构 */

	/** 以四元数形式返回目标，假设 Values 是 Euler 条目的序列。索引是要转换的欧拉。 */
	/** 
		Don't normalize at all. This should only be used with
		the interpolative method, if it is known that all input
		values will be within the area bounded by the targets.
	*/
	/** 该目标的值集，大小必须为 TargetDimensions  */
	/** 将此条目设置为来自提供的旋转器的 3 个浮点数 */
	NoNormalization,

	/** 将此条目设置为来自提供向量的 3 个浮点数 */
};

	/** 返回一个目标作为旋转器，假设 Values 是 Euler 条目的序列。索引是要转换的欧拉。*/
	/** 返回该目标的维度 */
/** Struct storing a particular entry within the RBF */
/** 在 RBF 中存储特定条目的结构 */
USTRUCT()
	/** 以四元数形式返回目标，假设 Values 是 Euler 条目的序列。索引是要转换的欧拉。 */
struct FRBFEntry
{
	GENERATED_BODY()
/** 有关 RBF 中特定目标的数据，包括比例因子 */

	/** Set of values for this target, size must be TargetDimensions  */
	/** 该目标的值集，大小必须为 TargetDimensions  */
	/** 将此条目设置为来自提供的旋转器的 3 个浮点数 */
	UPROPERTY(EditAnywhere, Category = RBFData)
	TArray<float> Values;
	/** 这个目标的影响有多大。 */
	/** 将此条目设置为来自提供向量的 3 个浮点数 */

	/** Return a target as an rotator, assuming Values is a sequence of Euler entries. Index is which Euler to convert.*/
	/** 返回一个目标作为旋转器，假设 Values 是 Euler 条目的序列。索引是要转换的欧拉。*/
	/** 返回该目标的维度 */
	ANIMGRAPHRUNTIME_API FRotator AsRotator(int32 Index) const;

	/** Return a target as a quaternion, assuming Values is a sequence of Euler entries. Index is which Euler to convert. */
	/** 以四元数形式返回目标，假设 Values 是 Euler 条目的序列。索引是要转换的欧拉。 */
	ANIMGRAPHRUNTIME_API FQuat AsQuat(int32 Index) const;

	ANIMGRAPHRUNTIME_API FVector AsVector(int32 Index) const;
/** 有关 RBF 中特定目标的数据，包括比例因子 */


	/** Set this entry to 3 floats from supplied rotator */
	/** 将此条目设置为来自提供的旋转器的 3 个浮点数 */
	ANIMGRAPHRUNTIME_API void AddFromRotator(const FRotator& InRot);
	/** Set this entry to 3 floats from supplied vector */
	/** 这个目标的影响有多大。 */
	/** 将此条目设置为来自提供向量的 3 个浮点数 */
	ANIMGRAPHRUNTIME_API void AddFromVector(const FVector& InVector);

	/** Return dimensionality of this target */
	/** 返回该目标的维度 */
	int32 GetDimensions() const
	{
		return Values.Num();
	}
};

/** Data about a particular target in the RBF, including scaling factor */
/** 有关 RBF 中特定目标的数据，包括比例因子 */
/** 存储RBF结果的结构体——目标索引和对应的权重 */
USTRUCT()
struct FRBFTarget : public FRBFEntry
{
	/** 目标指标 */
	GENERATED_BODY()

	/** 目标重量 */
	/** How large the influence of this target. */
	/** 这个目标的影响有多大。 */
	UPROPERTY(EditAnywhere, Category = RBFData)
	float ScaleFactor;

	/** Whether we want to apply an additional custom curve when activating this target. 
	    Ignored if the solver type is Interpolative. 
	*/
	UPROPERTY(EditAnywhere, Category = RBFData)
	bool bApplyCustomCurve;

	/** Custom curve to apply to activation of this target, if bApplyCustomCurve is true.
		Ignored if the solver type is Interpolative. */
	UPROPERTY(EditAnywhere, Category = RBFData)
/** RBF 求解器使用的参数 */
	FRichCurve CustomCurve;
/** 存储RBF结果的结构体——目标索引和对应的权重 */

	/** Override the distance method used to calculate the distance from this target to
		the input. Ignored if the solver type is Interpolative. */
	/** 目标指标 */
	/** 输入数据有多少个维度 */
	UPROPERTY(EditAnywhere, Category = RBFData)
	ERBFDistanceMethod DistanceMethod;
	/** 目标重量 */

	/** Override the falloff function used to smooth the distance from this target to
		the input. Ignored if the solver type is Interpolative. */
	UPROPERTY(EditAnywhere, Category = RBFData)
	ERBFFunctionType FunctionType;

	FRBFTarget()
		: ScaleFactor(1.f)
		, bApplyCustomCurve(false)
		, DistanceMethod(ERBFDistanceMethod::DefaultMethod)
		, FunctionType(ERBFFunctionType::DefaultFunction)
	{}
};

	/* 根据目标之间的平均距离自动选取半径 */
/** RBF 求解器使用的参数 */
/** Struct for storing RBF results - target index and corresponding weight */
/** 存储RBF结果的结构体——目标索引和对应的权重 */
struct FRBFOutputWeight
{
	/** Index of target */
	/** 目标指标 */
	/** 输入数据有多少个维度 */
	int32 TargetIndex;
	/** Weight of target */
	/** DistanceMethod 为 SwingAngle 时使用的轴 */
	/** 目标重量 */
	float TargetWeight;

	FRBFOutputWeight(int32 InTargetIndex, float InTargetWeight)
	/** 权重低于该值我们就不必费心从目标返回贡献 */
		: TargetIndex(InTargetIndex)
		, TargetWeight(InTargetWeight)
	{}

	/** 用于标准化重量的方法 */
	FRBFOutputWeight()
		: TargetIndex(0)
		, TargetWeight(0.f)
	{}
	/** 中位数的旋转或位置（用于标准化） */
};

/** Parameters used by RBF solver */
	/* 根据目标之间的平均距离自动选取半径 */
	/** 用于中位数的最小距离 */
/** RBF 求解器使用的参数 */
USTRUCT(BlueprintType)
struct FRBFParams
{
	/** 用于中位数的最大距离 */
	GENERATED_BODY()

	/** How many dimensions input data has */
	/** 输入数据有多少个维度 */
	UPROPERTY()
	int32 TargetDimensions;
	/** 用于返回摆动轴单位方向向量的实用程序 */
	/** DistanceMethod 为 SwingAngle 时使用的轴 */

	/** Specifies the type of solver to use. The additive solver requires normalization, for the
		most part, whereas the Interpolative solver is not as reliant on it. The interpolative
		solver also has smoother blending, whereas the additive solver requires more targets but
	/** 权重低于该值我们就不必费心从目标返回贡献 */
/** 径向基函数求解器函数库 */
		has a more precise control over the influence of each target.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData)
	ERBFSolverType SolverType;
	/** 用于标准化重量的方法 */

	/** Default radius for each target. 
	/** 给定一组目标和新的输入条目，给出带有权重的激活目标列表 */
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData, meta = (EditCondition = "!bAutomaticRadius"))
	/** 中位数的旋转或位置（用于标准化） */
	float Radius;

	/** 给定一组目标和新的输入条目，给出带有权重的激活目标列表 */
	/* Automatically pick the radius based on the average distance between targets */
	/* 根据目标之间的平均距离自动选取半径 */
	/** 用于中位数的最小距离 */
	/** 用于查找每个目标到最近邻居目标的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData)
	bool bAutomaticRadius;

	/** 实用程序使用提供的参数查找两个条目之间的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData)
	/** 用于中位数的最大距离 */
	ERBFFunctionType Function;
	/** 返回给定目标的半径 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData)
	ERBFDistanceMethod DistanceMethod;
	/** 计算给定目标的最佳半径。返回半径 */

	/** Axis to use when DistanceMethod is SwingAngle */
	/** 用于返回摆动轴单位方向向量的实用程序 */
	/** DistanceMethod 为 SwingAngle 时使用的轴 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData)
	TEnumAsByte<EBoneAxis> TwistAxis;

	/** Weight below which we shouldn't bother returning a contribution from a target */
	/** 权重低于该值我们就不必费心从目标返回贡献 */
/** 径向基函数求解器函数库 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData)
	float WeightThreshold;

	/** Method to use for normalizing the weight */
	/** 用于标准化重量的方法 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData)
	ERBFNormalizeMethod NormalizeMethod;
	/** 给定一组目标和新的输入条目，给出带有权重的激活目标列表 */

	/** Rotation or position of median (used for normalization) */
	/** 中位数的旋转或位置（用于标准化） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData, meta = (EditCondition = "NormalizeMethod == ERBFNormalizeMethod::NormalizeWithinMedian"))
	FVector MedianReference;
	/** 给定一组目标和新的输入条目，给出带有权重的激活目标列表 */

	/** Minimum distance used for median */
	/** 用于中位数的最小距离 */
	/** 用于查找每个目标到最近邻居目标的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData, meta = (UIMin = "0", UIMax = "90", EditCondition = "NormalizeMethod == ERBFNormalizeMethod::NormalizeWithinMedian"))
	float MedianMin;

	/** 实用程序使用提供的参数查找两个条目之间的距离 */
	/** Maximum distance used for median */
	/** 用于中位数的最大距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RBFData, meta = (UIMin = "0", UIMax = "90", EditCondition = "NormalizeMethod == ERBFNormalizeMethod::NormalizeWithinMedian"))
	/** 返回给定目标的半径 */
	float MedianMax;

	ANIMGRAPHRUNTIME_API FRBFParams();
	/** 计算给定目标的最佳半径。返回半径 */

	/** Util for returning unit direction vector for swing axis */
	/** 用于返回摆动轴单位方向向量的实用程序 */
	ANIMGRAPHRUNTIME_API FVector GetTwistAxisVector() const;
};

struct ANIMGRAPHRUNTIME_API FRBFSolverData;

/** Library of Radial Basis Function solver functions */
/** 径向基函数求解器函数库 */
struct FRBFSolver
{
	/** Given a list of targets, verify which ones are valid for solving the RBF setup. This is mostly about removing identical targets
		which invalidates the interpolative solver. Returns true if all targets are valid. */
	static ANIMGRAPHRUNTIME_API bool ValidateTargets(const FRBFParams& Params, const TArray<FRBFTarget>& Targets, TArray<int>& InvalidTargets);

	/** Given a set of targets and new input entry, give list of activated targets with weights */
	/** 给定一组目标和新的输入条目，给出带有权重的激活目标列表 */
	static ANIMGRAPHRUNTIME_API TSharedPtr<const FRBFSolverData> InitSolver(const FRBFParams& Params, const TArray<FRBFTarget>& Targets);

	static ANIMGRAPHRUNTIME_API bool IsSolverDataValid(const FRBFSolverData& SolverData, const FRBFParams& Params, const TArray<FRBFTarget>& Targets);

	/** Given a set of targets and new input entry, give list of activated targets with weights */
	/** 给定一组目标和新的输入条目，给出带有权重的激活目标列表 */
	static ANIMGRAPHRUNTIME_API void Solve(const FRBFSolverData& SolverData, const FRBFParams& Params, const TArray<FRBFTarget>& Targets, const FRBFEntry& Input, TArray<FRBFOutputWeight>& OutputWeights);

	/** Util to find distance to nearest neighbour target for each target */
	/** 用于查找每个目标到最近邻居目标的距离 */
	static ANIMGRAPHRUNTIME_API bool FindTargetNeighbourDistances(const FRBFParams& Params, const TArray<FRBFTarget>& Targets, TArray<float>& NeighbourDists);

	/** Util to find distance between two entries, using provided params */
	/** 实用程序使用提供的参数查找两个条目之间的距离 */
	static ANIMGRAPHRUNTIME_API float FindDistanceBetweenEntries(const FRBFEntry& A, const FRBFEntry& B, const FRBFParams& Params, ERBFDistanceMethod OverrideMethod = ERBFDistanceMethod::DefaultMethod);

	/** Returns the radius for a given target */
	/** 返回给定目标的半径 */
	static ANIMGRAPHRUNTIME_API float GetRadiusForTarget(const FRBFTarget& Target, const FRBFParams& Params);

	/** Compute the optimal radius for the given targets. Returns the radius */
	/** 计算给定目标的最佳半径。返回半径 */
	static ANIMGRAPHRUNTIME_API float GetOptimalRadiusForTargets(const FRBFParams& Params, const TArray<FRBFTarget>& Targets);
};
