// BlockoutStudio - 点线面编辑库实现
#include "BlockoutMeshEditLib.h"
#include "BlockoutActor.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

using UE::Geometry::FIndex3i;

void FBlockoutMeshEditLib::CommitMesh(ABlockoutActor* Actor, FDynamicMesh3& Mesh)
{
	Mesh.CompactInPlace();
	Actor->CachedMesh = Mesh;

	// 回写自由网格数据（存档用）
	Actor->Data.bFreeform = true;
	Actor->Data.Vertices.Reset();
	Actor->Data.Triangles.Reset();
	for (int32 Vid = 0; Vid < Mesh.MaxVertexID(); ++Vid)
	{
		if (!Mesh.IsVertex(Vid)) continue;
		const FVector3d V = Mesh.GetVertex(Vid);
		Actor->Data.Vertices.Add(FVector(V.X, V.Y, V.Z));
	}
	// 顶点紧凑后索引映射：CompactInPlace 后 ID 连续
	for (int32 Vid = 0; Vid < Mesh.VertexCount(); ++Vid)
	{
		// no-op：保证顶点连续
	}
	for (int32 Tid = 0; Tid < Mesh.MaxTriangleID(); ++Tid)
	{
		if (!Mesh.IsTriangle(Tid)) continue;
		const FIndex3i T = Mesh.GetTriangle(Tid);
		Actor->Data.Triangles.Add(T.A);
		Actor->Data.Triangles.Add(T.B);
		Actor->Data.Triangles.Add(T.C);
	}
	Actor->Rebuild();
}

bool FBlockoutMeshEditLib::MoveVertices(ABlockoutActor* Actor, const TArray<int32>& VertexIds, const FVector& Delta)
{
	if (!Actor || VertexIds.Num() == 0) return false;
	FDynamicMesh3 Mesh = Actor->CachedMesh;
	const FVector3d D(Delta.X, Delta.Y, Delta.Z);
	int32 Moved = 0;
	for (int32 Vid : VertexIds)
	{
		if (!Mesh.IsVertex(Vid)) continue;
		Mesh.SetVertex(Vid, Mesh.GetVertex(Vid) + D);
		++Moved;
	}
	if (Moved == 0) return false;
	CommitMesh(Actor, Mesh);
	return true;
}

bool FBlockoutMeshEditLib::ExtrudeFaces(ABlockoutActor* Actor, const TArray<int32>& TriIds, float Distance)
{
	if (!Actor || TriIds.Num() == 0) return false;
	FDynamicMesh3 Mesh = Actor->CachedMesh;

	TSet<int32> TriSet;
	for (int32 T : TriIds) { if (Mesh.IsTriangle(T)) TriSet.Add(T); }
	if (TriSet.Num() == 0) return false;

	// 收集旧顶点 → 生成偏移新顶点
	TMap<int32, int32> OldToNew;
	for (int32 Tid : TriSet)
	{
		const FIndex3i T = Mesh.GetTriangle(Tid);
		// 面法向（局部空间）
		const FVector3d A = Mesh.GetVertex(T.A);
		const FVector3d B = Mesh.GetVertex(T.B);
		const FVector3d C = Mesh.GetVertex(T.C);
		const FVector3d N = FVector3d::CrossProduct(B - A, C - A).GetSafeNormal();
		const int32 Verts[3] = { T.A, T.B, T.C };
		for (int32 k = 0; k < 3; ++k)
		{
			const int32 Old = Verts[k];
			if (!OldToNew.Contains(Old))
			{
				const int32 NewId = Mesh.AppendVertex(Mesh.GetVertex(Old) + N * (double)Distance);
				OldToNew.Add(Old, NewId);
			}
		}
	}

	// 顶面：原三角形拓扑映射到新顶点
	for (int32 Tid : TriSet)
	{
		const FIndex3i T = Mesh.GetTriangle(Tid);
		Mesh.AppendTriangle(FIndex3i(OldToNew[T.A], OldToNew[T.B], OldToNew[T.C]));
	}

	// 侧壁：选中三角形的边界边（邻接三角形未被选中）
	for (int32 Tid : TriSet)
	{
		const FIndex3i T = Mesh.GetTriangle(Tid);
		const FIndex3i Nbr = Mesh.GetTriNeighbourTris(Tid);
		// 边 i 为对顶点 i 的边：AB->Nbr.C, BC->Nbr.A, CA->Nbr.B
		const int32 Edges[3][2] = { {T.A, T.B}, {T.B, T.C}, {T.C, T.A} };
		const int32 NbrTri[3] = { Nbr.C, Nbr.A, Nbr.B };
		for (int32 e = 0; e < 3; ++e)
		{
			const int32 V0 = Edges[e][0], V1 = Edges[e][1];
			// 共享该边的邻接三角形未被选中才是边界边
			const int32 Nt = NbrTri[e];
			const bool bBoundary = (Nt == FDynamicMesh3::InvalidID) || !TriSet.Contains(Nt);
			if (!bBoundary) continue;
			const int32 N0 = OldToNew[V0], N1 = OldToNew[V1];
			Mesh.AppendTriangle(FIndex3i(V0, V1, N1));
			Mesh.AppendTriangle(FIndex3i(V0, N1, N0));
		}
	}

	// 删除原面（保新顶点）
	for (int32 Tid : TriSet)
	{
		Mesh.RemoveTriangle(Tid, false, false);
	}
	CommitMesh(Actor, Mesh);
	return true;
}

bool FBlockoutMeshEditLib::BevelVertices(ABlockoutActor* Actor, const TArray<int32>& VertexIds, float Amount)
{
	if (!Actor || VertexIds.Num() == 0) return false;
	FDynamicMesh3 Mesh = Actor->CachedMesh;
	const double Alpha = FMath::Clamp((double)Amount, 0.05, 0.45);

	for (int32 Vid : VertexIds)
	{
		if (!Mesh.IsVertex(Vid)) continue;
		const FVector3d Center = Mesh.GetVertex(Vid);

		// 邻接顶点（经邻接三角形收集去重）
		TArray<int32> NbrTris;
		Mesh.GetVtxTriangles(Vid, NbrTris);
		TSet<int32> NbrSet;
		for (int32 Tid : NbrTris)
		{
			if (!Mesh.IsTriangle(Tid)) continue;
			const FIndex3i T = Mesh.GetTriangle(Tid);
			if (T.A != Vid) NbrSet.Add(T.A);
			if (T.B != Vid) NbrSet.Add(T.B);
			if (T.C != Vid) NbrSet.Add(T.C);
		}
		TArray<int32> NbrVerts = NbrSet.Array();
		if (NbrVerts.Num() < 3) continue;

		// 每条邻接边上取插值点
		TArray<int32> NewRing;
		for (int32 Nv : NbrVerts)
		{
			const FVector3d Np = Mesh.GetVertex(Nv);
			const FVector3d P = Center + (Np - Center) * Alpha;
			NewRing.Add(Mesh.AppendVertex(P));
		}

		// 删除含该顶点的三角形
		TArray<int32> VtxTris;
		Mesh.GetVtxTriangles(Vid, VtxTris);
		for (int32 Tid : VtxTris)
		{
			Mesh.RemoveTriangle(Tid, false, false);
		}

		// 用新点圈重建扇面（中心点 + 环）
		const int32 NewCenter = Mesh.AppendVertex(Center);
		const int32 N = NewRing.Num();
		for (int32 i = 0; i < N; ++i)
		{
			Mesh.AppendTriangle(FIndex3i(NewCenter, NewRing[i], NewRing[(i + 1) % N]));
		}
	}
	CommitMesh(Actor, Mesh);
	return true;
}

bool FBlockoutMeshEditLib::CollapseVertices(ABlockoutActor* Actor, const TArray<int32>& VertexIds)
{
	if (!Actor || VertexIds.Num() == 0) return false;
	FDynamicMesh3 Mesh = Actor->CachedMesh;

	// 目标质心
	FVector3d Centroid = FVector3d::ZeroVector;
	int32 Count = 0;
	TSet<int32> Keep;
	for (int32 Vid : VertexIds)
	{
		if (!Mesh.IsVertex(Vid)) continue;
		Centroid += Mesh.GetVertex(Vid);
		Keep.Add(Vid);
		++Count;
	}
	if (Count < 2) return false;
	Centroid /= (double)Count;

	const int32 Target = Keep.Array()[0];
	Mesh.SetVertex(Target, Centroid);

	// 重映射三角形引用
	TArray<int32> AllTris;
	for (int32 Tid = 0; Tid < Mesh.MaxTriangleID(); ++Tid)
	{
		if (Mesh.IsTriangle(Tid)) AllTris.Add(Tid);
	}
	for (int32 Tid : AllTris)
	{
		const FIndex3i T = Mesh.GetTriangle(Tid);
		int32 V[3] = { T.A, T.B, T.C };
		bool bChanged = false;
		for (int32 k = 0; k < 3; ++k)
		{
			if (Keep.Contains(V[k]) && V[k] != Target) { V[k] = Target; bChanged = true; }
		}
		// 退化三角清理
		if (V[0] == V[1] || V[1] == V[2] || V[0] == V[2])
		{
			Mesh.RemoveTriangle(Tid, false, false);
			continue;
		}
		if (bChanged)
		{
			Mesh.SetTriangle(Tid, FIndex3i(V[0], V[1], V[2]));
		}
	}
	CommitMesh(Actor, Mesh);
	return true;
}

bool FBlockoutMeshEditLib::DeleteFaces(ABlockoutActor* Actor, const TArray<int32>& TriIds)
{
	if (!Actor || TriIds.Num() == 0) return false;
	FDynamicMesh3 Mesh = Actor->CachedMesh;
	int32 Removed = 0;
	for (int32 Tid : TriIds)
	{
		if (Mesh.IsTriangle(Tid))
		{
			Mesh.RemoveTriangle(Tid, false, false);
			++Removed;
		}
	}
	if (Removed == 0) return false;
	CommitMesh(Actor, Mesh);
	return true;
}

bool FBlockoutMeshEditLib::RayTriangleHit(const FVector& A, const FVector& B, const FVector& C,
	const FVector& RayOrigin, const FVector& RayDir, double& OutT)
{
	// Moller-Trumbore
	const FVector E1 = B - A;
	const FVector E2 = C - A;
	const FVector P = FVector::CrossProduct(RayDir, E2);
	const double Det = FVector::DotProduct(E1, P);
	if (FMath::Abs(Det) < 1e-9) return false;
	const double InvDet = 1.0 / Det;
	const FVector T = RayOrigin - A;
	const double U = FVector::DotProduct(T, P) * InvDet;
	if (U < 0.0 || U > 1.0) return false;
	const FVector Q = FVector::CrossProduct(T, E1);
	const double V = FVector::DotProduct(RayDir, Q) * InvDet;
	if (V < 0.0 || U + V > 1.0) return false;
	const double Dist = FVector::DotProduct(E2, Q) * InvDet;
	if (Dist < 1e-4) return false;
	OutT = Dist;
	return true;
}

int32 FBlockoutMeshEditLib::PickVertex(ABlockoutActor* Actor, const FVector& RayOrigin, const FVector& RayDir, float MaxDist)
{
	if (!Actor) return -1;
	const FDynamicMesh3& Mesh = Actor->CachedMesh;
	int32 Best = -1;
	double BestScore = MaxDist;
	for (int32 Vid = 0; Vid < Mesh.MaxVertexID(); ++Vid)
	{
		if (!Mesh.IsVertex(Vid)) continue;
		const FVector3d P = Mesh.GetVertex(Vid);
		const FVector WorldP = Actor->GetActorTransform().TransformPosition(FVector(P.X, P.Y, P.Z));
		// 点到射线距离
		const FVector ToP = WorldP - RayOrigin;
		const double Proj = FVector::DotProduct(ToP, RayDir);
		if (Proj < 0.0) continue;
		const FVector Closest = RayOrigin + RayDir * Proj;
		const double Dist = FVector::Dist(Closest, WorldP);
		if (Dist < BestScore)
		{
			BestScore = Dist;
			Best = Vid;
		}
	}
	return Best;
}

int32 FBlockoutMeshEditLib::PickFace(ABlockoutActor* Actor, const FVector& RayOrigin, const FVector& RayDir)
{
	if (!Actor) return -1;
	const FDynamicMesh3& Mesh = Actor->CachedMesh;
	const FTransform T = Actor->GetActorTransform();
	int32 Best = -1;
	double BestT = TNumericLimits<double>::Max();
	for (int32 Tid = 0; Tid < Mesh.MaxTriangleID(); ++Tid)
	{
		if (!Mesh.IsTriangle(Tid)) continue;
		const FIndex3i Tri = Mesh.GetTriangle(Tid);
		const FVector3d A3 = Mesh.GetVertex(Tri.A);
		const FVector3d B3 = Mesh.GetVertex(Tri.B);
		const FVector3d C3 = Mesh.GetVertex(Tri.C);
		const FVector A = T.TransformPosition(FVector(A3.X, A3.Y, A3.Z));
		const FVector B = T.TransformPosition(FVector(B3.X, B3.Y, B3.Z));
		const FVector C = T.TransformPosition(FVector(C3.X, C3.Y, C3.Z));
		double HitT = 0.0;
		if (RayTriangleHit(A, B, C, RayOrigin, RayDir, HitT) && HitT < BestT)
		{
			BestT = HitT;
			Best = Tid;
		}
	}
	return Best;
}
