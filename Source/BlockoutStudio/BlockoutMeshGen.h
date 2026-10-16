// BlockoutStudio - 参数化体块网格生成（原语构建 + GeometryScript 布尔）
#pragma once

#include "CoreMinimal.h"
#include "BlockoutTypes.h"

class FDynamicMesh3;

class FBlockoutMeshGen
{
public:
	// 依据形状与参数构建网格（几何中心为原点，Z 向上，单位 uu）
	static void BuildShape(EBlockoutShape Shape, const FBlockoutShapeParams& P, FDynamicMesh3& OutMesh);

	// 基础原语
	static void BuildBox(FDynamicMesh3& M, double X, double Y, double Z, int32 SegX = 1, int32 SegY = 1, int32 SegZ = 1);
	static void BuildCylinder(FDynamicMesh3& M, double Diameter, double Height, int32 Segments, bool bSmooth = true);
	static void BuildSphere(FDynamicMesh3& M, double Diameter, int32 Segments);
	static void BuildCapsule(FDynamicMesh3& M, double Diameter, double Height, int32 Segments);
	static void BuildPipe(FDynamicMesh3& M, double Diameter, double WallThickness, double Height, int32 Segments);
	static void BuildStaircase(FDynamicMesh3& M, double L, double W, double H, int32 Steps, bool bSpiral = false);
	static void BuildRamp(FDynamicMesh3& M, double L, double W, double H);
	static void BuildWedge(FDynamicMesh3& M, double L, double W, double H);
	static void BuildColonnade(FDynamicMesh3& M, double ColumnDiameter, double ColumnHeight, int32 Count, double Spacing);
	static void BuildFence(FDynamicMesh3& M, double L, double W, double H, double PostSpacing);

	// 洞体生成（用于布尔挖洞，沿 +Y 方向拉伸 Depth，截面位于 XZ 平面）
	static void BuildRectPrismHole(FDynamicMesh3& M, double Width, double Height, double Depth);
	static void BuildCirclePrismHole(FDynamicMesh3& M, double Diameter, double Depth, int32 Segments);
	static void BuildPolygonPrismHole(FDynamicMesh3& M, const TArray<FVector2D>& PolygonXZ, double Depth);

	// 布尔运算：A = A op B（B 使用 BTransform 变换）。返回是否成功。
	static bool BooleanOp(FDynamicMesh3& A, const FDynamicMesh3& B, const FTransform& BTransform, bool bSubtract);

	// 后处理：平直/平滑法线 + 盒式 UV 投影
	static void FinalizeMesh(FDynamicMesh3& M, bool bSmoothNormals = false, double UVScale = 100.0);
};
