// BlockoutStudio - 点线面编辑库（M3：选点移动/挤出/倒角/塌陷/删除面）
#pragma once

#include "CoreMinimal.h"
#include "BlockoutTypes.h"
#include "DynamicMesh/DynamicMesh3.h"

using UE::Geometry::FDynamicMesh3;

class ABlockoutActor;

class BLOCKOUTSTUDIO_API FBlockoutMeshEditLib
{
public:
	// 选中顶点沿 Delta 移动（世界→本地换算由调用方处理）
	static bool MoveVertices(ABlockoutActor* Actor, const TArray<int32>& VertexIds, const FVector& Delta);

	// 挤出选中面（沿各自法向），Distance 为挤出距离（uu）
	static bool ExtrudeFaces(ABlockoutActor* Actor, const TArray<int32>& TriIds, float Distance);

	// 倒角选中顶点（切角重建邻域扇面），Amount 为切角比例 0-0.5
	static bool BevelVertices(ABlockoutActor* Actor, const TArray<int32>& VertexIds, float Amount);

	// 塌陷选中顶点到质心（退化三角自动清理）
	static bool CollapseVertices(ABlockoutActor* Actor, const TArray<int32>& VertexIds);

	// 删除选中面
	static bool DeleteFaces(ABlockoutActor* Actor, const TArray<int32>& TriIds);

	// ---------- ProBuilder 补齐：焊接/补洞/内插面/细分面/翻转法线 ----------
	// 焊接选中顶点（距离 ≤ Tolerance 合并到首点）
	static bool WeldVertices(ABlockoutActor* Actor, const TArray<int32>& VertexIds, float Tolerance = 1.f);
	// 补洞：边界边环自动扇面填充
	static bool FillHoles(ABlockoutActor* Actor);
	// 内插面：选中三角形向内缩 Amount 比例并生成侧壁
	static bool InsetFaces(ABlockoutActor* Actor, const TArray<int32>& TriIds, float Amount = 0.2f);
	// 细分面：选中三角形 1 拆 4（共享边中点）
	static bool SubdivideFaces(ABlockoutActor* Actor, const TArray<int32>& TriIds);
	// 翻转选中面法线（绕序反转）
	static bool FlipNormals(ABlockoutActor* Actor, const TArray<int32>& TriIds);

	// 拾取辅助：射线命中最近顶点/面，返回 id，-1 未命中
	static int32 PickVertex(ABlockoutActor* Actor, const FVector& RayOrigin, const FVector& RayDir, float MaxDist = 50.f);
	static int32 PickFace(ABlockoutActor* Actor, const FVector& RayOrigin, const FVector& RayDir);

	// 交集：一次拾取三角形内部点返回重心坐标用
	static bool RayTriangleHit(const FVector& A, const FVector& B, const FVector& C,
		const FVector& RayOrigin, const FVector& RayDir, double& OutT);

private:
	// 应用编辑后重建组件网格与缓存
	static void CommitMesh(ABlockoutActor* Actor, FDynamicMesh3& Mesh);
};
