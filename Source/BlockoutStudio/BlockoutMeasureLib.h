// BlockoutStudio - 测量系统（距离/尺寸/面积/体积/角度/高差/比例尺）
#pragma once

#include "CoreMinimal.h"
#include "BlockoutTypes.h"
#include "DynamicMesh/DynamicMesh3.h"

using UE::Geometry::FDynamicMesh3;

class ABlockoutActor;

class BLOCKOUTSTUDIO_API FBlockoutMeasureLib
{
public:
	// 点到点距离
	static FBlockoutMeasureItem MeasureDistance(const FVector& A, const FVector& B);
	// 选中物体尺寸（包围盒三边）
	static FBlockoutMeasureItem MeasureDimensions(ABlockoutActor* Actor);
	// 物体表面积
	static FBlockoutMeasureItem MeasureArea(ABlockoutActor* Actor);
	// 物体体积
	static FBlockoutMeasureItem MeasureVolume(ABlockoutActor* Actor);
	// 三点角度（B 为角顶点）
	static FBlockoutMeasureItem MeasureAngle(const FVector& A, const FVector& B, const FVector& C);
	// 高差（A-B 的 Z 距）
	static FBlockoutMeasureItem MeasureHeightDiff(const FVector& A, const FVector& B);
	// 网格表面积/体积计算
	static double MeshSurfaceArea(const FDynamicMesh3& M);
	static double MeshVolume(const FDynamicMesh3& M);
	// 单位格式化（1m=100uu）
	static FString FormatUu(double Uu);
};
