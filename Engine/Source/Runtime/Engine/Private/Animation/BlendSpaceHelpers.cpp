// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/BlendSpaceHelpers.h"

#define BLENDSPACE_MINSAMPLE	3

#define LOCTEXT_NAMESPACE "AnimationBlendSpaceHelpers"

// Stores an edge or triangle index, and distance to a point, in anticipation of sorting.
// 存储边或三角形索引以及到点的距离，以进行排序。
struct FIndexAndDistance
{
	int32 Index;
	double Distance;
	FIndexAndDistance(int32 InIndex, double InDistance)
		: Index(InIndex), Distance(InDistance) {}
};

void FDelaunayTriangleGenerator::SetGridBox(const FBlendParameter& BlendParamX, const FBlendParameter& BlendParamY)
{
	FVector2D Min(BlendParamX.Min, BlendParamY.Min);
	FVector2D Max(BlendParamX.Max, BlendParamY.Max);
	FVector2D Mid = (Min + Max) * 0.5f;
	FVector2D Range = Max - Min;
	Range.X = FMath::Max(Range.X, UE_DELTA);
	Range.Y = FMath::Max(Range.Y, UE_DELTA);

	GridMin = Mid - Range * 0.5f;
	RecipGridSize = FVector2D(1.0, 1.0) / Range;
}

void FDelaunayTriangleGenerator::EmptyTriangles()
{
	for (int32 TriangleIndex = 0; TriangleIndex < TriangleList.Num(); ++TriangleIndex)
	{
		delete TriangleList[TriangleIndex];
	}

	TriangleList.Empty();
}
void FDelaunayTriangleGenerator::EmptySamplePoints()
{
	SamplePointList.Empty();
}

void FDelaunayTriangleGenerator::Reset()
{
	EmptyTriangles();
	EmptySamplePoints();
}

void FDelaunayTriangleGenerator::AddSamplePoint(const FVector2D& NewPoint, const int32 SampleIndex)
{
	checkf(!SamplePointList.Contains(NewPoint), TEXT("Found duplicate points in blendspace"));
	SamplePointList.Add(FVertex(NewPoint, SampleIndex));
}

void FDelaunayTriangleGenerator::Triangulate(EPreferredTriangulationDirection PreferredTriangulationDirection)
{
	if (SamplePointList.Num() == 0)
	{
		return;
	}
	else if (SamplePointList.Num() == 1)
	{
		// degenerate case 1
		// 退化情况1
		FTriangle Triangle(&SamplePointList[0]);
		AddTriangle(Triangle);
	}
	else if (SamplePointList.Num() == 2)
	{
		// degenerate case 2
		// 退化情况2
		FTriangle Triangle(&SamplePointList[0], &SamplePointList[1]);
		AddTriangle(Triangle);
	}
	else
	{
		SortSamples();

		// first choose first 3 points
		// 首先选择前3点
		for (int32 I = 2; I<SamplePointList.Num(); ++I)
		{
			GenerateTriangles(SamplePointList, I + 1);
		}

		// degenerate case 3: many points all collinear or coincident
		// 退化情况 3：许多点都共线或重合
		if (TriangleList.Num() == 0)
		{
			if (AllCoincident(SamplePointList))
			{
				// coincident case - just create one triangle
				// 重合情况 - 只需创建一个三角形
				FTriangle Triangle(&SamplePointList[0]);
				AddTriangle(Triangle);
			}
			else
			{
				// collinear case: create degenerate triangles between pairs of points
				// 共线情况：在点对之间创建退化三角形
				for (int32 PointIndex = 0; PointIndex < SamplePointList.Num() - 1; ++PointIndex)
				{
					FTriangle Triangle(&SamplePointList[PointIndex], &SamplePointList[PointIndex + 1]);
					AddTriangle(Triangle);
				}
			}
		}
	}
	AdjustEdgeDirections(PreferredTriangulationDirection);
}

static int32 GetCandidateEdgeIndex(const FTriangle* Triangle)
{
	bool bFoundVert = false;
	bool bFoundHor = false;
	bool bFoundSlope = false;
	int32 SlopeIndex = 0;
	for (int32 VertexIndex = 0; VertexIndex != 3; ++VertexIndex)
	{
		int32 VertexIndexNext = (VertexIndex + 1) % 3;
		if (Triangle->Vertices[VertexIndex]->Position.X == Triangle->Vertices[VertexIndexNext]->Position.X)
		{
			bFoundVert = true;
		}
		else if (Triangle->Vertices[VertexIndex]->Position.Y == Triangle->Vertices[VertexIndexNext]->Position.Y)
		{
			bFoundHor = true;
		}
		else
		{
			bFoundSlope = true;
			SlopeIndex = VertexIndex;
		}
	}
	if (bFoundVert && bFoundHor && bFoundSlope)
	{
		return SlopeIndex;
	}
	else
	{
		return -1;
	}
}

void FDelaunayTriangleGenerator::AdjustEdgeDirections(EPreferredTriangulationDirection PreferredTriangulationDirection)
{
	if (PreferredTriangulationDirection == EPreferredTriangulationDirection::None)
	{
		return;
	}

	for (int32 TriangleIndex0 = 0; TriangleIndex0 != TriangleList.Num(); ++TriangleIndex0)
	{
		FTriangle* Triangle0 = TriangleList[TriangleIndex0];

		// Check if it is axis aligned and if so which is the hypotenuse edge
		// 检查它是否与轴对齐，如果是，哪条是斜边
		int32 EdgeIndex0 = GetCandidateEdgeIndex(Triangle0);
		if (EdgeIndex0 >= 0)
		{
			int32 EdgeIndexNext0 = (EdgeIndex0 + 1) % 3;
			// Only continue if we would want to flip this edge
			// 仅当我们想要翻转此边缘时才继续
			FVector2D EdgeDir = Triangle0->Vertices[EdgeIndexNext0]->Position - Triangle0->Vertices[EdgeIndex0]->Position;
			FVector2D EdgePos = (Triangle0->Vertices[EdgeIndexNext0]->Position + Triangle0->Vertices[EdgeIndex0]->Position) * 0.5f;

			// Slope is true if we go up from left to right
			// 如果我们从左向右上升，则坡度为真
			bool bUpSlope = EdgeDir.X * EdgeDir.Y > 0.0f;
			// Desired slope depends on the quadrant
			// 所需的斜率取决于象限
			bool bDesiredUpSlope = PreferredTriangulationDirection == EPreferredTriangulationDirection::Tangential ? EdgePos.X * EdgePos.Y < 0.0f : EdgePos.X * EdgePos.Y >= 0.0f;

			if (bUpSlope == bDesiredUpSlope)
			{
				continue;
			}

			// Look for a triangle that is also axis aligned and facing this sloping edge
			// 寻找一个同样轴对齐并面向该倾斜边缘的三角形
			int32 SampleIndexStart = Triangle0->Vertices[EdgeIndex0]->SampleIndex;
			int32 SampleIndexEnd = Triangle0->Vertices[(EdgeIndex0 + 1) % 3]->SampleIndex;
			int32 EdgeIndex1;
			int32 TriangleIndex1 = FindTriangleIndexWithEdge(SampleIndexEnd, SampleIndexStart, &EdgeIndex1);
			if (TriangleIndex1 < 0)
			{
				continue;
			}
			check(EdgeIndex1 >= 0);
			FTriangle* Triangle1 = TriangleList[TriangleIndex1];
			if (GetCandidateEdgeIndex(Triangle1) == EdgeIndex1)
			{
				// Flip the edge across the pair of triangles. Note that after this the triangles
				// 将边缘翻转穿过这对三角形。请注意，在此之后的三角形
				// will contain incorrect edge info, but we don't need that any more.
				// 将包含不正确的边缘信息，但我们不再需要它了。
				int32 EdgeIndexPrev1 = (EdgeIndex1 + 2) % 3;
				int32 EdgeIndexNext1 = (EdgeIndex1 + 1) % 3;
				int32 EdgeIndexPrev0 = (EdgeIndex0 + 2) % 3;

				Triangle0->Vertices[EdgeIndexNext0] = Triangle1->Vertices[EdgeIndexPrev1];
				Triangle1->Vertices[EdgeIndexNext1] = Triangle0->Vertices[EdgeIndexPrev0];
				Triangle0->UpdateCenter();
				Triangle1->UpdateCenter();
			}
		}
	}
}

void FDelaunayTriangleGenerator::SortSamples()
{
	// Populate sorting array with sample points and their original (blend space -> sample data) indices
	// 使用样本点及其原始（混合空间 -> 样本数据）索引填充排序数组

	struct FComparePoints
	{
		FORCEINLINE bool operator()( const FVertex& A, const FVertex& B ) const
		{
			// the sorting happens from -> +X, -> +Y,  -> for now ignore Z ->+Z
			// 排序发生在 -> +X, -> +Y, -> 现在忽略 Z ->+Z
			if( A.Position.Y == B.Position.Y ) // same, then compare Y
			{
				return A.Position.X < B.Position.X;
			}
			return A.Position.Y < B.Position.Y;			
		}
	};
	// sort all points
	// 对所有点进行排序
	SamplePointList.Sort( FComparePoints() );
}

/** 
* The key function in Delaunay Triangulation
* return true if the TestPoint is WITHIN the triangle circumcircle
*	http://en.wikipedia.org/wiki/Delaunay_triangulation 
*/
FDelaunayTriangleGenerator::ECircumCircleState FDelaunayTriangleGenerator::GetCircumcircleState(const FTriangle* T, const FVertex& TestPoint)
{
	const int32 NumPointsPerTriangle = 3;

	// First off, normalize all the points
	// 首先，标准化所有点
	FVector2D NormalizedPositions[NumPointsPerTriangle];
	
	// Unrolled loop
	// 展开的循环
	NormalizedPositions[0] = (T->Vertices[0]->Position - GridMin) * RecipGridSize;
	NormalizedPositions[1] = (T->Vertices[1]->Position - GridMin) * RecipGridSize;
	NormalizedPositions[2] = (T->Vertices[2]->Position - GridMin) * RecipGridSize;

	const FVector2D NormalizedTestPoint = ( TestPoint.Position - GridMin ) * RecipGridSize;

	// ignore Z, eventually this has to be on plane
	// 忽略Z，最终这必须在平面上
	// http://en.wikipedia.org/wiki/Delaunay_triangulation - determinant
	// http://en.wikipedia.org/wiki/Delaunay_triangulation - 行列式
	const double M00 = NormalizedPositions[0].X - NormalizedTestPoint.X;
	const double M01 = NormalizedPositions[0].Y - NormalizedTestPoint.Y;
	const double M02 = NormalizedPositions[0].X * NormalizedPositions[0].X - NormalizedTestPoint.X * NormalizedTestPoint.X
		+ NormalizedPositions[0].Y*NormalizedPositions[0].Y - NormalizedTestPoint.Y * NormalizedTestPoint.Y;

	const double M10 = NormalizedPositions[1].X - NormalizedTestPoint.X;
	const double M11 = NormalizedPositions[1].Y - NormalizedTestPoint.Y;
	const double M12 = NormalizedPositions[1].X * NormalizedPositions[1].X - NormalizedTestPoint.X * NormalizedTestPoint.X
		+ NormalizedPositions[1].Y * NormalizedPositions[1].Y - NormalizedTestPoint.Y * NormalizedTestPoint.Y;

	const double M20 = NormalizedPositions[2].X - NormalizedTestPoint.X;
	const double M21 = NormalizedPositions[2].Y - NormalizedTestPoint.Y;
	const double M22 = NormalizedPositions[2].X * NormalizedPositions[2].X - NormalizedTestPoint.X * NormalizedTestPoint.X
		+ NormalizedPositions[2].Y * NormalizedPositions[2].Y - NormalizedTestPoint.Y * NormalizedTestPoint.Y;

	const double Det = M00*M11*M22+M01*M12*M20+M02*M10*M21 - (M02*M11*M20+M01*M10*M22+M00*M12*M21);
	
	// When the vertices are sorted in a counterclockwise order, the determinant is positive if and only if Testpoint lies inside the circumcircle of T.
	// 当顶点按逆时针顺序排序时，当且仅当测试点位于 T 的外接圆内时，行列式才为正。
	if (Det < 0.0)
	{
		return ECCS_Outside;
	}
	else
	{
		// On top of the triangle edge
		// 在三角形边缘的顶部
		if (FMath::IsNearlyZero(Det, UE_DOUBLE_SMALL_NUMBER))
		{
			return ECCS_On;
		}
		else
		{
			return ECCS_Inside;
		}
	}
}

int32 FDelaunayTriangleGenerator::FindTriangleIndexWithEdge(int32 SampleIndex0, int32 SampleIndex1, int32* VertexIndex) const
{
	for (int32 TriangleIndex = 0; TriangleIndex != TriangleList.Num(); ++TriangleIndex)
	{
		const FTriangle* Triangle = TriangleList[TriangleIndex];
		if (Triangle->Vertices[0]->SampleIndex == SampleIndex0 && Triangle->Vertices[1]->SampleIndex == SampleIndex1)
		{
			if (VertexIndex)
			{
				*VertexIndex = 0;
			}
			return TriangleIndex;
		}
		if (Triangle->Vertices[1]->SampleIndex == SampleIndex0 && Triangle->Vertices[2]->SampleIndex == SampleIndex1)
		{
			if (VertexIndex)
			{
				*VertexIndex = 1;
			}
			return TriangleIndex;
		}
		if (Triangle->Vertices[2]->SampleIndex == SampleIndex0 && Triangle->Vertices[0]->SampleIndex == SampleIndex1)
		{
			if (VertexIndex)
			{
				*VertexIndex = 2;
			}
			return TriangleIndex;
		}
	}
	// Perfectly normal to get here - when looking for a triangle that is off the outside edge of
	// 到达这里是完全正常的 - 当寻找一个不在外边缘的三角形时
	// the graph.
	// 图表。
	return INDEX_NONE;
}

TArray<struct FBlendSpaceTriangle> FDelaunayTriangleGenerator::CalculateTriangles() const
{
	TArray<struct FBlendSpaceTriangle> Triangles;
	Triangles.Reserve(TriangleList.Num());
	for (int32 TriangleIndex = 0; TriangleIndex != TriangleList.Num(); ++TriangleIndex)
	{
		const FTriangle* Triangle = TriangleList[TriangleIndex];

		FBlendSpaceTriangle NewTriangle;
		
		FVector2D Vertices[3];
		Vertices[0] = (Triangle->Vertices[0]->Position - GridMin) * RecipGridSize;
		Vertices[1] = (Triangle->Vertices[1]->Position - GridMin) * RecipGridSize;
		Vertices[2] = (Triangle->Vertices[2]->Position - GridMin) * RecipGridSize;

		NewTriangle.Vertices[0] = Vertices[0];
		NewTriangle.Vertices[1] = Vertices[1];
		NewTriangle.Vertices[2] = Vertices[2];
		NewTriangle.SampleIndices[0] = Triangle->Vertices[0]->SampleIndex;
		NewTriangle.SampleIndices[1] = Triangle->Vertices[1]->SampleIndex;
		NewTriangle.SampleIndices[2] = Triangle->Vertices[2]->SampleIndex;

		for (int32 Index = 0; Index != FBlendSpaceTriangle::NUM_VERTICES; ++Index)
		{
			FBlendSpaceTriangleEdgeInfo& EdgeInfo = NewTriangle.EdgeInfo[Index];
			int32 IndexNext = (Index + 1) % FBlendSpaceTriangle::NUM_VERTICES;

			EdgeInfo.NeighbourTriangleIndex = FindTriangleIndexWithEdge(
				Triangle->Vertices[IndexNext]->SampleIndex, Triangle->Vertices[Index]->SampleIndex);
			FVector2D EdgeDir = Vertices[IndexNext] - Vertices[Index];

			// Triangles are wound anticlockwise as viewed from above - rotate the edge 90 deg clockwise
			// 从上方观察，三角形是逆时针缠绕的 - 将边缘顺时针旋转 90 度
			// to make it be the outwards pointing normal.
			// 使其成为向外指向的法线。
			EdgeInfo.Normal = FVector2D(EdgeDir.Y, -EdgeDir.X).GetSafeNormal();

			// Update these if necessary when all triangles have been processed
			// 处理完所有三角形后，如有必要，请更新这些
			EdgeInfo.AdjacentPerimeterTriangleIndices[0] = INDEX_NONE;
			EdgeInfo.AdjacentPerimeterTriangleIndices[1] = INDEX_NONE;
			EdgeInfo.AdjacentPerimeterVertexIndices[0] = INDEX_NONE;
			EdgeInfo.AdjacentPerimeterVertexIndices[1] = INDEX_NONE;
		}
		Triangles.Add(NewTriangle);
	}

	for (int32 TriangleIndex = 0; TriangleIndex != Triangles.Num(); ++TriangleIndex)
	{
		FBlendSpaceTriangle& Triangle = Triangles[TriangleIndex];
		for (int32 Index = 0; Index != FBlendSpaceTriangle::NUM_VERTICES; ++Index)
		{
			FBlendSpaceTriangleEdgeInfo& EdgeInfo = Triangle.EdgeInfo[Index];
			if (EdgeInfo.NeighbourTriangleIndex == INDEX_NONE)
			{
				int32 IndexNext = (Index + 1) % FBlendSpaceTriangle::NUM_VERTICES;

				int32 SampleIndex = Triangle.SampleIndices[Index];
				int32 SampleIndexNext = Triangle.SampleIndices[IndexNext];

				// Update the triangle info to allow traversal around the perimeter of the
				// 更新三角形信息以允许围绕三角形的周界进行遍历
				// triangulated region. Iterate through all the other triangles...
				// 三角区域。遍历所有其他三角形...
				for (int32 OtherTriangleIndex = 0; OtherTriangleIndex != Triangles.Num(); ++OtherTriangleIndex)
				{
					if (OtherTriangleIndex == TriangleIndex)
						continue;

					const FBlendSpaceTriangle& OtherTriangle = Triangles[OtherTriangleIndex];
					// ... then check for a vertex that matches the edge we're considering
					// ...然后检查与我们正在考虑的边相匹配的顶点
					for (int32 VertexIndex = 0; VertexIndex != FBlendSpaceTriangle::NUM_VERTICES; ++VertexIndex)
					{
						int32 VertexIndexPrev = (VertexIndex + FBlendSpaceTriangle::NUM_VERTICES - 1) % FBlendSpaceTriangle::NUM_VERTICES;
						if (OtherTriangle.SampleIndices[VertexIndex] == SampleIndex
							&& OtherTriangle.EdgeInfo[VertexIndexPrev].NeighbourTriangleIndex == INDEX_NONE)
						{
							// We found a perimeter triangle that comes before our edge
							// 我们发现在我们的边缘之前有一个周边三角形
							EdgeInfo.AdjacentPerimeterTriangleIndices[0] = OtherTriangleIndex;
							EdgeInfo.AdjacentPerimeterVertexIndices[0] = VertexIndexPrev;
						}
						if (OtherTriangle.SampleIndices[VertexIndex] == SampleIndexNext
							&& OtherTriangle.EdgeInfo[VertexIndex].NeighbourTriangleIndex == INDEX_NONE)
						{
							// We found a perimeter triangle that comes after our edge
							// 我们发现在我们的边缘之后有一个周边三角形
							EdgeInfo.AdjacentPerimeterTriangleIndices[1] = OtherTriangleIndex;
							EdgeInfo.AdjacentPerimeterVertexIndices[1] = VertexIndex ;
						}
					}
				}
			}
		}
	}
	return Triangles;
}

bool FDelaunayTriangleGenerator::IsCollinear(const FVertex* A, const FVertex* B, const FVertex* C)
{
	const FVector2D Diff1 = B->Position - A->Position;
	const FVector2D Diff2 = C->Position - A->Position;
	double Cross = Diff1 ^ Diff2;
	return (Cross == 0.f);
}

bool FDelaunayTriangleGenerator::AllCoincident(const TArray<FVertex>& InPoints)
{
	if (InPoints.Num() > 0)
	{
		const FVertex& FirstPoint = InPoints[0];
		for (int32 PointIndex = 0; PointIndex < InPoints.Num(); ++PointIndex)
		{
			const FVertex& Point = InPoints[PointIndex];
			if (Point.Position != FirstPoint.Position)
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

bool FDelaunayTriangleGenerator::FlipTriangles(const int32 TriangleIndexOne, const int32 TriangleIndexTwo)
{
	const FTriangle* A = TriangleList[TriangleIndexOne];
	const FTriangle* B = TriangleList[TriangleIndexTwo];

	// if already optimized, don't have to do any
	// 如果已经优化，则无需执行任何操作
	FVertex* TestPt = A->FindNonSharingPoint(B);

	// If it's not inside, we don't have to do any
	// 如果它不在里面，我们就不必做任何事情
	if (GetCircumcircleState(A, *TestPt) != ECCS_Inside)
	{
		return false;
	}

	FTriangle NewTriangles[2];
	int32 TrianglesMade = 0;

	for (int32 VertexIndexOne = 0; VertexIndexOne < 2; ++VertexIndexOne)
	{
		for (int32 VertexIndexTwo = VertexIndexOne + 1; VertexIndexTwo < 3; ++VertexIndexTwo)
		{
			// Check if these vertices form a valid triangle (should be non-colinear)
			// 检查这些顶点是否形成一个有效的三角形（应该是非共线的）
			if (IsEligibleForTriangulation(A->Vertices[VertexIndexOne], A->Vertices[VertexIndexTwo], TestPt))
			{
				// Create the new triangle and check if the final (original) vertex falls inside or outside of it's circumcircle
				// 创建新三角形并检查最终（原始）顶点是否落在其外接圆内部或外部
				const FTriangle NewTriangle(A->Vertices[VertexIndexOne], A->Vertices[VertexIndexTwo], TestPt);
				const int32 VertexIndexThree = 3 - (VertexIndexTwo + VertexIndexOne);
				if (GetCircumcircleState(&NewTriangle, *A->Vertices[VertexIndexThree]) == ECCS_Outside)
				{
					// If so store the triangle and increment the number of triangles
					// 如果是这样，则存储三角形并增加三角形的数量
					checkf(TrianglesMade < 2, TEXT("Incorrect number of triangles created"));
					NewTriangles[TrianglesMade] = NewTriangle;
					++TrianglesMade;
				}
			}
		}
	}
	
	// In case two triangles were generated the flip was successful so we can add them to the list
	// 如果生成了两个三角形，则翻转成功，因此我们可以将它们添加到列表中
	if (TrianglesMade == 2)
	{
		AddTriangle(NewTriangles[0], false);
		AddTriangle(NewTriangles[1], false);
	}

	return TrianglesMade == 2;
}

void FDelaunayTriangleGenerator::AddTriangle(FTriangle& newTriangle, bool bCheckHalfEdge/*=true*/)
{
	// see if it's same vertices
	// 看看是否有相同的顶点
	for (int32 I=0;I<TriangleList.Num(); ++I)
	{
		if (newTriangle == *TriangleList[I])
		{
			return;
		}

		if (bCheckHalfEdge && newTriangle.HasSameHalfEdge(TriangleList[I]))
		{
			return;
		}
	}

	TriangleList.Add(new FTriangle(newTriangle));
}

int32 FDelaunayTriangleGenerator::GenerateTriangles(TArray<FVertex>& PointList, const int32 TotalNum)
{
	if (TotalNum == BLENDSPACE_MINSAMPLE)
	{
		if (IsEligibleForTriangulation(&PointList[0], &PointList[1], &PointList[2]))
		{
			FTriangle Triangle(&PointList[0], &PointList[1], &PointList[2]);
			AddTriangle(Triangle);
		}
	}
	else if (TriangleList.Num() == 0)
	{
		FVertex * TestPoint = &PointList[TotalNum-1];

		// so far no triangle is made, try to make it with new points that are just entered
		// 到目前为止还没有创建三角形，尝试用刚刚输入的新点来创建它
		for (int32 I=0; I<TotalNum-2; ++I)
		{
			if (IsEligibleForTriangulation(&PointList[I], &PointList[I+1], TestPoint))
			{
				FTriangle NewTriangle (&PointList[I], &PointList[I+1], TestPoint);
				AddTriangle(NewTriangle);
			}
		}
	}
	else
	{
		// get the last addition
		// 获取最后添加的内容
		FVertex * TestPoint = &PointList[TotalNum-1];
		int32 TriangleNum = TriangleList.Num();
	
		for (int32 I=0; I<TriangleList.Num(); ++I)
		{
			FTriangle * Triangle = TriangleList[I];
			if (IsEligibleForTriangulation(Triangle->Vertices[0], Triangle->Vertices[1], TestPoint))
			{
				FTriangle NewTriangle (Triangle->Vertices[0], Triangle->Vertices[1], TestPoint);
				AddTriangle(NewTriangle);
			}

			if (IsEligibleForTriangulation(Triangle->Vertices[0], Triangle->Vertices[2], TestPoint))
			{
				FTriangle NewTriangle (Triangle->Vertices[0], Triangle->Vertices[2], TestPoint);
				AddTriangle(NewTriangle);
			}

			if (IsEligibleForTriangulation(Triangle->Vertices[1], Triangle->Vertices[2], TestPoint))
			{
				FTriangle NewTriangle (Triangle->Vertices[1], Triangle->Vertices[2], TestPoint);
				AddTriangle(NewTriangle);
			}
		}

		// this is locally optimization part
		// 这是局部优化部分
		// we need to make sure all triangles are locally optimized. If not optimize it. 
		// 我们需要确保所有三角形都是局部优化的。如果不优化的话。
		for (int32 I=0; I<TriangleList.Num(); ++I)
		{
			FTriangle * A = TriangleList[I];
			for (int32 J=I+1; J<TriangleList.Num(); ++J)
			{
				FTriangle * B = TriangleList[J];

				// does share same edge
				// 确实共享相同的边缘
				if (A->DoesShareSameEdge(B))
				{
					// then test to see if locally optimized
					// 然后测试一下是否本地优化
					if (FlipTriangles(I, J))
					{
						// if this flips, remove current triangle
						// 如果翻转，则删除当前三角形
						delete TriangleList[I];
						delete TriangleList[J];
						//I need to remove J first because other wise, 
						//我需要先删除 J 因为否则，
						//  index J isn't valid anymore
						//  索引 J 不再有效
						TriangleList.RemoveAt(J);
						TriangleList.RemoveAt(I);
						// start over since we modified triangle
						// 自从我们修改了三角形后重新开始
						// once we don't have any more to flip, we're good to go!
						// 一旦我们不再需要翻转，我们就可以出发了！
						I=-1;
						break;
					}
				}
			}
		}
	}

	return TriangleList.Num();
}

static FVector GetBaryCentric2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
	double a = ((B.Y - C.Y) * (Point.X - C.X) + (C.X - B.X) * (Point.Y - C.Y)) / ((B.Y - C.Y) * (A.X - C.X) + (C.X - B.X) * (A.Y - C.Y));
	double b = ((C.Y - A.Y) * (Point.X - C.X) + (A.X - C.X) * (Point.Y - C.Y)) / ((B.Y - C.Y) * (A.X - C.X) + (C.X - B.X) * (A.Y - C.Y));

	return FVector(a, b, 1.0 - a - b);
}


bool FBlendSpaceGrid::FindTriangleThisPointBelongsTo(const FVector2D& TestPoint, FVector& OutBarycentricCoords, FTriangle*& OutTriangle, const TArray<FTriangle*>& TriangleList) const
{
	// Calculate distance from point to triangle and sort the triangle list accordingly
	// 计算点到三角形的距离并对三角形列表进行相应排序
	TArray<FIndexAndDistance> SortedTriangles;
	SortedTriangles.AddUninitialized(TriangleList.Num());
	for (int32 TriangleIndex=0; TriangleIndex<TriangleList.Num(); ++TriangleIndex)
	{
		SortedTriangles[TriangleIndex].Index = TriangleIndex;
		SortedTriangles[TriangleIndex].Distance = TriangleList[TriangleIndex]->GetDistance(TestPoint);
	}
	SortedTriangles.Sort([](const FIndexAndDistance &A, const FIndexAndDistance &B) { return A.Distance < B.Distance; });

	// Now loop over the sorted triangles and test the barycentric coordinates with the point
	// 现在循环遍历排序后的三角形并用该点测试重心坐标
	for (const FIndexAndDistance& SortedTriangle : SortedTriangles)
	{
		FTriangle* Triangle = TriangleList[SortedTriangle.Index];

		FVector Coords = GetBaryCentric2D(TestPoint, Triangle->Vertices[0]->Position, Triangle->Vertices[1]->Position, Triangle->Vertices[2]->Position);

		// Z coords often has precision error because it's derived from 1-A-B, so do more precise check
		// Z坐标由于是从1-A-B推导出来的，所以经常会有精度误差，所以要进行更精确的检查
		if (FMath::Abs(Coords.Z) < UE_KINDA_SMALL_NUMBER)
		{
			Coords.Z = 0.f;
		}

		// Is the point inside of the triangle, or on it's edge (Z coordinate should always match since the blend samples are set in 2D)
		// 是三角形内部的点，还是其边缘上的点（Z 坐标应始终匹配，因为混合样本是在 2D 中设置的）
		if ( 0.f <= Coords.X && Coords.X <= 1.0 && 0.f <= Coords.Y && Coords.Y <= 1.0 && 0.f <= Coords.Z && Coords.Z <= 1.0 )
		{
			OutBarycentricCoords = Coords;
			OutTriangle = Triangle;
			return true;
		}
	}

	return false;
}

static FVector2D ClosestPointOnSegment2D(const FVector2D& Point, const FVector2D& StartPoint, const FVector2D& EndPoint, double& T)
{
	const FVector2D Segment = EndPoint - StartPoint;
	const FVector2D VectToPoint = Point - StartPoint;

	// See if closest point is before StartPoint
	// 查看最近的点是否在 StartPoint 之前
	const double Dot1 = VectToPoint | Segment;
	if (Dot1 <= 0)
	{
		T = 0.0;
		return StartPoint;
	}

	// See if closest point is beyond EndPoint
	// 查看最近点是否超出 EndPoint
	const double Dot2 = Segment | Segment;
	if (Dot2 <= Dot1)
	{
		T = 1.0;
		return EndPoint;
	}

	// Closest Point is within segment
	// 最近点位于线段内
	T = Dot1 / Dot2;
	return StartPoint + Segment * T;
}


void FBlendSpaceGrid::GenerateGridElements(const TArray<FVertex>& SamplePoints, const TArray<FTriangle*>& TriangleList)
{
	check (NumGridDivisions.X > 0 && NumGridDivisions.Y > 0 );
	check (GridMax.ComponentwiseAllGreaterThan(GridMin));

	const int32 TotalNumGridPoints = NumGridPointsForAxis.X * NumGridPointsForAxis.Y;

	GridPoints.Empty(TotalNumGridPoints);

	if (SamplePoints.Num() == 0 || TriangleList.Num() == 0)
	{
		return;
	}

	GridPoints.AddDefaulted(TotalNumGridPoints);

	FVector2D GridPointPosition;		
	for (int32 GridPositionX = 0; GridPositionX < NumGridPointsForAxis.X; ++GridPositionX)
	{
		for (int32 GridPositionY = 0; GridPositionY < NumGridPointsForAxis.Y; ++GridPositionY)
		{
			FTriangle * SelectedTriangle = NULL;
			FEditorElement& GridPoint = GetElement(GridPositionX, GridPositionY);

			GridPointPosition = GetPosFromIndex(GridPositionX, GridPositionY);

			FVector Weights;
			if ( FindTriangleThisPointBelongsTo(GridPointPosition, Weights, SelectedTriangle, TriangleList) )
			{
				// found it
				// [翻译失败: found it]
				GridPoint.Weights[0] = Weights.X;
				GridPoint.Weights[1] = Weights.Y;
				GridPoint.Weights[2] = Weights.Z;
				GridPoint.Indices[0] = SelectedTriangle->Vertices[0]->SampleIndex;
				GridPoint.Indices[1] = SelectedTriangle->Vertices[1]->SampleIndex;
				GridPoint.Indices[2] = SelectedTriangle->Vertices[2]->SampleIndex;
				check(GridPoint.Indices[0] != INDEX_NONE);
				check(GridPoint.Indices[1] != INDEX_NONE);
				check(GridPoint.Indices[2] != INDEX_NONE);
			}
			else
			{
				// Work through all the edges and find the one with a point closest to this grid position.
				// [翻译失败: Work through all the edges and find the one with a point closest to this grid position.]
				int32 ClosestTriangleIndex = 0;
				int32 ClosestEdgeIndex = 0;
				double ClosestDistance = UE_DOUBLE_BIG_NUMBER;
				double ClosestT = 0.0;

				// Just walk through all the edges from all the triangles and find the closest ones
				// 只需遍历所有三角形的所有边并找到最接近的边
				for (int32 TriangleIndex = 0; TriangleIndex < TriangleList.Num(); ++TriangleIndex)
				{
					const FTriangle* Triangle = TriangleList[TriangleIndex];
					for (int32 EdgeIndex = 0 ; EdgeIndex != 3 ; ++EdgeIndex)
					{
						double T;
						const FVector2D ClosestPoint = ClosestPointOnSegment2D(
							GridPointPosition,
							Triangle->Edges[EdgeIndex].Vertices[0]->Position,
							Triangle->Edges[EdgeIndex].Vertices[1]->Position,
							T);
						const double Distance = (ClosestPoint - GridPointPosition).SizeSquared();

						if (Distance < ClosestDistance)
						{
							ClosestTriangleIndex = TriangleIndex;
							ClosestEdgeIndex = EdgeIndex;
							ClosestDistance = Distance;
							ClosestT = T;
						}
					}
				}

				const FTriangle* Triangle = TriangleList[ClosestTriangleIndex];

				GridPoint.Weights[0] = 1.0 - ClosestT;
				GridPoint.Indices[0] = Triangle->Edges[ClosestEdgeIndex].Vertices[0]->SampleIndex;
				GridPoint.Weights[1] = ClosestT;
				GridPoint.Indices[1] = Triangle->Edges[ClosestEdgeIndex].Vertices[1]->SampleIndex;
			}
		}
	}
}

/** 
* Convert grid index (GridX, GridY) to triangle coords and returns FVector2D
*/
const FVector2D FBlendSpaceGrid::GetPosFromIndex(const int32 GridX, const int32 GridY) const
{
	// grid X starts from 0 -> N when N == GridSizeX
	// 当 N == GridSizeX 时，网格 X 从 0 -> N 开始
	// grid Y starts from 0 -> N when N == GridSizeY
	// 当 N == GridSizeY 时，网格 Y 从 0 -> N 开始
	// LeftBottom will map to Grid 0, 0
	// LeftBottom 将映射到网格 0, 0
	// RightTop will map to Grid N, N
	// RightTop 将映射到网格 N, N

	FVector2D CoordDim = GridMax - GridMin;
	FVector2D EachGridSize = CoordDim / NumGridDivisions;

	// for now only 2D
	// 目前只有 2D
	return FVector2D(GridX*EachGridSize.X+GridMin.X, GridY*EachGridSize.Y+GridMin.Y);
}

const FEditorElement& FBlendSpaceGrid::GetElement(const int32 GridX, const int32 GridY) const
{
	check (NumGridPointsForAxis.X >= GridX);
	check (NumGridPointsForAxis.Y >= GridY);

	check (GridPoints.Num() > 0 );
	return GridPoints[GridY * NumGridPointsForAxis.X + GridX];
}

FEditorElement& FBlendSpaceGrid::GetElement(const int32 GridX, const int32 GridY)
{
	FEditorElement& Test = const_cast<FEditorElement &>(std::as_const(*this).GetElement(GridX, GridY));

	check (NumGridPointsForAxis.X >= GridX);
	check (NumGridPointsForAxis.Y >= GridY);

	check (GridPoints.Num() > 0 );
	FEditorElement& Test2 = GridPoints[GridY * NumGridPointsForAxis.X + GridX];
	return Test2;
}

#undef LOCTEXT_NAMESPACE
