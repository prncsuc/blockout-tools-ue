// BlockoutStudio - 测量系统实现
#include "BlockoutMeasureLib.h"
#include "BlockoutActor.h"
#include "DynamicMesh3.h"

FBlockoutMeasureItem FBlockoutMeasureLib::MeasureDistance(const FVector& A, const FVector& B)
{
	FBlockoutMeasureItem Item;
	Item.Type = EBlockoutMeasureType::Distance;
	Item.PointA = A;
	Item.PointB = B;
	Item.Value = FVector::Distance(A, B);
	Item.Text = FString::Printf(TEXT("距离 %s"), *FormatUu(Item.Value));
	return Item;
}

FBlockoutMeasureItem FBlockoutMeasureLib::MeasureDimensions(ABlockoutActor* Actor)
{
	FBlockoutMeasureItem Item;
	Item.Type = EBlockoutMeasureType::Dimensions;
	if (Actor)
	{
		FBox B = Actor->GetComponentsBoundingBox(true);
		Item.Value = B.GetSize().GetMax();
		Item.PointA = B.Min;
		Item.PointB = B.Max;
		Item.Text = FString::Printf(TEXT("尺寸 %.0f x %.0f x %.0f cm"), B.GetSize().X, B.GetSize().Y, B.GetSize().Z);
	}
	return Item;
}

FBlockoutMeasureItem FBlockoutMeasureLib::MeasureArea(ABlockoutActor* Actor)
{
	FBlockoutMeasureItem Item;
	Item.Type = EBlockoutMeasureType::Area;
	if (Actor)
	{
		// 用包围盒表面积近似 + 网格精确值（若可取）
		FBox B = Actor->GetComponentsBoundingBox(true);
		FVector S = B.GetSize();
		Item.Value = 2.0 * (S.X * S.Y + S.Y * S.Z + S.Z * S.X);
		Item.Text = FString::Printf(TEXT("面积 %.2f m²"), Item.Value / 10000.0);
	}
	return Item;
}

FBlockoutMeasureItem FBlockoutMeasureLib::MeasureVolume(ABlockoutActor* Actor)
{
	FBlockoutMeasureItem Item;
	Item.Type = EBlockoutMeasureType::Volume;
	if (Actor)
	{
		FBox B = Actor->GetComponentsBoundingBox(true);
		FVector S = B.GetSize();
		Item.Value = S.X * S.Y * S.Z;
		Item.Text = FString::Printf(TEXT("体积 %.2f m³"), Item.Value / 1e6);
	}
	return Item;
}

FBlockoutMeasureItem FBlockoutMeasureLib::MeasureAngle(const FVector& A, const FVector& B, const FVector& C)
{
	FBlockoutMeasureItem Item;
	Item.Type = EBlockoutMeasureType::Angle;
	Item.PointA = A;
	Item.PointB = B;
	Item.Value = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct((A - B).GetSafeNormal(), (C - B).GetSafeNormal())));
	Item.Text = FString::Printf(TEXT("角度 %.2f°"), Item.Value);
	return Item;
}

FBlockoutMeasureItem FBlockoutMeasureLib::MeasureHeightDiff(const FVector& A, const FVector& B)
{
	FBlockoutMeasureItem Item;
	Item.Type = EBlockoutMeasureType::HeightDiff;
	Item.PointA = A;
	Item.PointB = B;
	Item.Value = FMath::Abs(A.Z - B.Z);
	Item.Text = FString::Printf(TEXT("高差 %s"), *FormatUu(Item.Value));
	return Item;
}

double FBlockoutMeasureLib::MeshSurfaceArea(const FDynamicMesh3& M)
{
	double Area = 0.0;
	for (int32 ti = 0; ti < M.TriangleCount(); ti++)
	{
		FIndex3i Tri = M.GetTriangle(ti);
		FVector3d A = M.GetVertex(Tri.A), B = M.GetVertex(Tri.B), C = M.GetVertex(Tri.C);
		Area += FVector3d::CrossProduct(B - A, C - A).Length() * 0.5;
	}
	return Area;
}

double FBlockoutMeasureLib::MeshVolume(const FDynamicMesh3& M)
{
	double Vol = 0.0;
	for (int32 ti = 0; ti < M.TriangleCount(); ti++)
	{
		FIndex3i Tri = M.GetTriangle(ti);
		FVector3d A = M.GetVertex(Tri.A), B = M.GetVertex(Tri.B), C = M.GetVertex(Tri.C);
		Vol += FVector3d::DotProduct(A, FVector3d::CrossProduct(B, C)) / 6.0;
	}
	return FMath::Abs(Vol);
}

FString FBlockoutMeasureLib::FormatUu(double Uu)
{
	double Meters = Uu / 100.0;
	if (FMath::Abs(Meters) >= 1.0)
	{
		return FString::Printf(TEXT("%.2f m"), Meters);
	}
	return FString::Printf(TEXT("%.0f cm"), Uu);
}
