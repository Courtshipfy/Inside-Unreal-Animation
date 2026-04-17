// Copyright Epic Games, Inc. All Rights Reserved.

#include "RBF/RBFInterpolator.h"
#include "AutoRTFM.h"

// Just to be sure, also added this in Eigen.Build.cs
// 为了确定，还在 Eigen.Build.cs 中添加了这个
#ifndef EIGEN_MPL2_ONLY
#define EIGEN_MPL2_ONLY
#endif

#if defined(_MSC_VER) && USING_CODE_ANALYSIS
#pragma warning(push)
#pragma warning(disable:6294) // Ill-defined for-loop:  initial condition does not satisfy test.  Loop body not executed.
#endif
PRAGMA_DEFAULT_VISIBILITY_START
THIRD_PARTY_INCLUDES_START
#include <Eigen/LU>
THIRD_PARTY_INCLUDES_END
PRAGMA_DEFAULT_VISIBILITY_END
#if defined(_MSC_VER) && USING_CODE_ANALYSIS
#pragma warning(pop)
#endif

bool FRBFInterpolatorBase::SetUpperKernel(
	const TArrayView<float>& UpperKernel, 
	int32 Size
)
{
	using namespace Eigen;

	bool bSuccess = false;

	// The matrix logic below is self-contained and can run in the open without affecting
	// 下面的矩阵逻辑是独立的，可以在开放状态下运行，不会影响
	// transactional correctness.
	// 交易的正确性。
	// `Coeffs` is the only externally-visible alteration to program state.
	// “Coeffs”是对程序状态的唯一外部可见的改变。
	// Allocating and zeroing Coeffs before we enter the open ensures that it can be rolled
	// 在我们进入开局之前分配和清零 Coeffs 确保它可以滚动
	// back safely if the transaction is aborted; we don't need a RecordOpenWrite below.
	// 如果事务中止则安全返回；我们不需要下面的 RecordOpenWrite。
	Coeffs.Init(0.0f, Size * Size);

	UE_AUTORTFM_OPEN
	{
		MatrixXf FullKernel;

		// Construct the full kernel from the upper half calculated in the templated
		// 从模板中计算的上半部分构建完整的内核
		// portion.
		// 部分。
		FullKernel = MatrixXf::Identity(Size, Size);

		for (int32 c = 0, i = 0; i < Size; i++)	
		{
			for (int32 j = i; j < Size; j++)
			{
				FullKernel(i, j) = FullKernel(j, i) = UpperKernel[c++];
			}
		}

		// Usually the RBF formulation is computed by solving for:
		// 通常，RBF 公式是通过求解以下公式来计算的：
		// 
		//   A * w = T
		//   A * w = T
		//
		// Where A is the target kernel is a symmetric matrix containing the distance 
		// 其中A是目标核，是包含距离的对称矩阵
		// between each node, w is the weights we want, and T is the target vector whose
		// 每个节点之间，w是我们想要的权重，T是目标向量，其
		// values we want to interpolate.
		// 我们想要插值的值。
		//
		// However, in our case, we consider the activation of each node's output value
		// 然而，在我们的例子中，我们考虑每个节点输出值的激活
		// to be a part of a N-dimensional vector, whose size is the same as the node count,
		// 是 N 维向量的一部分，其大小与节点数相同，
		// and therefore, collectively, same as the target kernel's dimensions. 
		// 因此，总的来说，与目标内核的尺寸相同。
		// Each row is all zeros except for the activation value of 1.0 for a node at that
		// 除了该节点的激活值为 1.0 之外，每一行均为零
		// node's index, effectively forming an identity matrix.
		// 节点的索引，有效地形成单位矩阵。
		// 
		// This allows us to reformulate the problem as:
		// 这使我们能够将问题重新表述为：
		//
		//   A * w = I
		//   A * w = 我
		//
		// Or in other words:  
		// 或者换句话说：
		//
		//   w = A^-1
		//   w = A^-1
		//
		// Eigen will now happily pick LU factorization with partial pivoting for 
		// Eigen 现在很乐意选择带有部分旋转的 LU 分解
		// the inverse, which is blazingly fast.
		// 相反，速度非常快。

		Map<MatrixXf> Result(Coeffs.GetData(), Size, Size);

		// If the matrix is non-invertible, return now and bail out.
		// 如果矩阵不可逆，则立即返回并退出。
		float Det = FullKernel.determinant();
		if (!FMath::IsNearlyZero(Det))
		{
			Result = FullKernel.inverse();
			bSuccess = true;
		}
	};

	return bSuccess;
}
