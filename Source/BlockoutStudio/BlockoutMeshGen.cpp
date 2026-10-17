// BlockoutStudio - 参数化体块网格生成实现
#include "BlockoutMeshGen.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMeshOverlay.h"
#include "UDynamicMesh.h"
#include "Operations/MeshBoolean.h"

using namespace UE::Geometry;

namespace
{
	struct FQuadVert { FVector3d P; FVector3d N; };

	// 自动绕序的四边形（ExpectedN 为期望外法线）
	void AddQuadDir(FDynamicMesh3& M, const FVector3d& V0, const FVector3d& V1, const FVector3d& V2, const FVector3d& V3, const FVector3d& ExpectedN)
	{
		FVector3d N = FVector3d::CrossProduct(V1 - V0, V2 - V0);
		int32 A = M.AppendVertex(V0), B = M.AppendVertex(V1), C = M.AppendVertex(V2), D = M.AppendVertex(V3);
		if (FVector3d::DotProduct(N, ExpectedN) >= 0.0)
		{
			M.AppendTriangle(A, B, C);
			M.AppendTriangle(A, C, D);
		}
		else
		{
			M.AppendTriangle(A, C, B);
			M.AppendTriangle(A, D, C);
		}
	}

	void AddTriDir(FDynamicMesh3& M, const FVector3d& V0, const FVector3d& V1, const FVector3d& V2, const FVector3d& ExpectedN)
	{
		int32 A = M.AppendVertex(V0), B = M.AppendVertex(V1), C = M.AppendVertex(V2);
		FVector3d N = FVector3d::CrossProduct(V1 - V0, V2 - V0);
		if (FVector3d::DotProduct(N, ExpectedN) >= 0.0)
		{
			M.AppendTriangle(A, B, C);
		}
		else
		{
			M.AppendTriangle(A, C, B);
		}
	}

	// 双线性细分四边形
	void AddGridQuad(FDynamicMesh3& M, const FVector3d& V00, const FVector3d& V10, const FVector3d& V11, const FVector3d& V01, int32 Nx, int32 Ny, const FVector3d& ExpectedN)
	{
		TArray<int32> Idx;
		Idx.SetNum((Nx + 1) * (Ny + 1));
		for (int32 y = 0; y <= Ny; y++)
		{
			for (int32 x = 0; x <= Nx; x++)
			{
				double u = (double)x / Nx, v = (double)y / Ny;
				FVector3d P = V00 * (1 - u) * (1 - v) + V10 * u * (1 - v) + V11 * u * v + V01 * (1 - u) * v;
				Idx[y * (Nx + 1) + x] = M.AppendVertex(P);
			}
		}
		for (int32 y = 0; y < Ny; y++)
		{
			for (int32 x = 0; x < Nx; x++)
			{
				int32 A = Idx[y * (Nx + 1) + x], B = Idx[y * (Nx + 1) + x + 1];
				int32 C = Idx[(y + 1) * (Nx + 1) + x + 1], D = Idx[(y + 1) * (Nx + 1) + x];
				FVector3d N = FVector3d::CrossProduct(M.GetVertex(B) - M.GetVertex(A), M.GetVertex(C) - M.GetVertex(A));
				if (FVector3d::DotProduct(N, ExpectedN) >= 0.0)
				{
					M.AppendTriangle(A, B, C); M.AppendTriangle(A, C, D);
				}
				else
				{
					M.AppendTriangle(A, C, B); M.AppendTriangle(A, D, C);
				}
			}
		}
	}

	// 圆柱侧面（不含盖）
	void AddCylinderSide(FDynamicMesh3& M, double Diameter, double Z0, double Z1, int32 Segments)
	{
		double R = Diameter * 0.5;
		for (int32 i = 0; i < Segments; i++)
		{
			double A0 = 2.0 * PI * i / Segments, A1 = 2.0 * PI * (i + 1) / Segments;
			FVector3d P0(R * FMath::Cos(A0), R * FMath::Sin(A0), Z0);
			FVector3d P1(R * FMath::Cos(A1), R * FMath::Sin(A1), Z0);
			FVector3d T1(R * FMath::Cos(A1), R * FMath::Sin(A1), Z1);
			FVector3d T0(R * FMath::Cos(A0), R * FMath::Sin(A0), Z1);
			FVector3d Mid = (P0 + P1) * 0.5; Mid.Z = 0;
			AddQuadDir(M, P0, P1, T1, T0, Mid.GetSafeNormal());
		}
	}
}

void FBlockoutMeshGen::BuildBox(FDynamicMesh3& M, double X, double Y, double Z, int32 SegX, int32 SegY, int32 SegZ)
{
	double hx = X * 0.5, hy = Y * 0.5, hz = Z * 0.5;
	SegX = FMath::Max(1, SegX); SegY = FMath::Max(1, SegY); SegZ = FMath::Max(1, SegZ);
	// 顶 +Z / 底 -Z
	AddGridQuad(M, { -hx,-hy,hz }, { hx,-hy,hz }, { hx,hy,hz }, { -hx,hy,hz }, SegX, SegY, { 0,0,1 });
	AddGridQuad(M, { -hx,-hy,-hz }, { -hx,hy,-hz }, { hx,hy,-hz }, { hx,-hy,-hz }, SegX, SegY, { 0,0,-1 });
	// +X / -X
	AddGridQuad(M, { hx,-hy,-hz }, { hx,hy,-hz }, { hx,hy,hz }, { hx,-hy,hz }, SegY, SegZ, { 1,0,0 });
	AddGridQuad(M, { -hx,-hy,-hz }, { -hx,-hy,hz }, { -hx,hy,hz }, { -hx,hy,-hz }, SegY, SegZ, { -1,0,0 });
	// +Y / -Y
	AddGridQuad(M, { -hx,hy,-hz }, { -hx,hy,hz }, { hx,hy,hz }, { hx,hy,-hz }, SegX, SegZ, { 0,1,0 });
	AddGridQuad(M, { -hx,-hy,-hz }, { hx,-hy,-hz }, { hx,-hy,hz }, { -hx,-hy,hz }, SegX, SegZ, { 0,-1,0 });
}

void FBlockoutMeshGen::BuildCylinder(FDynamicMesh3& M, double Diameter, double Height, int32 Segments, bool bSmooth)
{
	Segments = FMath::Clamp(Segments, 3, 128);
	double R = Diameter * 0.5, h = Height * 0.5;
	AddCylinderSide(M, Diameter, -h, h, Segments);
	int32 CT = M.AppendVertex(FVector3d(0, 0, h));
	int32 CB = M.AppendVertex(FVector3d(0, 0, -h));
	for (int32 i = 0; i < Segments; i++)
	{
		double A0 = 2.0 * PI * i / Segments, A1 = 2.0 * PI * (i + 1) / Segments;
		int32 T0 = M.AppendVertex(FVector3d(R * FMath::Cos(A0), R * FMath::Sin(A0), h));
		int32 T1 = M.AppendVertex(FVector3d(R * FMath::Cos(A1), R * FMath::Sin(A1), h));
		int32 B0 = M.AppendVertex(FVector3d(R * FMath::Cos(A0), R * FMath::Sin(A0), -h));
		int32 B1 = M.AppendVertex(FVector3d(R * FMath::Cos(A1), R * FMath::Sin(A1), -h));
		M.AppendTriangle(CT, T0, T1);
		M.AppendTriangle(CB, B1, B0);
	}
}

void FBlockoutMeshGen::BuildSphere(FDynamicMesh3& M, double Diameter, int32 Segments)
{
	int32 Lon = FMath::Clamp(Segments, 4, 64), Lat = FMath::Clamp(Segments / 2, 3, 32);
	double R = Diameter * 0.5;
	TArray<int32> Rows;
	Rows.SetNum((Lat + 1) * Lon);
	for (int32 i = 0; i <= Lat; i++)
	{
		double Theta = PI * i / Lat;
		for (int32 j = 0; j < Lon; j++)
		{
			double Phi = 2.0 * PI * j / Lon;
			FVector3d P(R * FMath::Sin(Theta) * FMath::Cos(Phi), R * FMath::Sin(Theta) * FMath::Sin(Phi), R * FMath::Cos(Theta));
			Rows[i * Lon + j] = M.AppendVertex(P);
		}
	}
	for (int32 i = 0; i < Lat; i++)
	{
		for (int32 j = 0; j < Lon; j++)
		{
			int32 J1 = (j + 1) % Lon;
			int32 V00 = Rows[i * Lon + j], V10 = Rows[i * Lon + J1];
			int32 V01 = Rows[(i + 1) * Lon + j], V11 = Rows[(i + 1) * Lon + J1];
			if (i > 0) M.AppendTriangle(V00, V01, V11);
			if (i < Lat - 1) M.AppendTriangle(V00, V11, V10);
		}
	}
}

void FBlockoutMeshGen::BuildCapsule(FDynamicMesh3& M, double Diameter, double Height, int32 Segments)
{
	Segments = FMath::Clamp(Segments, 4, 64);
	double R = Diameter * 0.5;
	double CylH = FMath::Max(0.0, Height - Diameter);
	AddCylinderSide(M, Diameter, -CylH * 0.5, CylH * 0.5, Segments);
	// 上下半球
	int32 Lat = FMath::Clamp(Segments / 2, 3, 32);
	for (int32 half = 0; half < 2; half++)
	{
		double ZBase = half == 0 ? CylH * 0.5 : -CylH * 0.5;
		int32 CT = M.AppendVertex(FVector3d(0, 0, ZBase + (half == 0 ? R : -R)));
		for (int32 i = 0; i < Lat; i++)
		{
			double T0 = PI * 0.5 * i / Lat, T1 = PI * 0.5 * (i + 1) / Lat;
			for (int32 j = 0; j < Segments; j++)
			{
				double P0 = 2.0 * PI * j / Segments, P1 = 2.0 * PI * (j + 1) / Segments;
				auto Get = [&](double T, double P, double Sign) {
					return FVector3d(R * FMath::Sin(T) * FMath::Cos(P), R * FMath::Sin(T) * FMath::Sin(P), ZBase + Sign * R * FMath::Cos(T));
				};
				double Sign = half == 0 ? 1.0 : -1.0;
				FVector3d A = Get(T0, P0, Sign), B = Get(T0, P1, Sign), C = Get(T1, P1, Sign), D = Get(T1, P0, Sign);
				if (i == 0)
				{
					AddTriDir(M, M.GetVertex(CT), half == 0 ? D : A, half == 0 ? C : B, (A + C) * 0.5 - FVector3d(0, 0, ZBase));
				}
				else
				{
					AddQuadDir(M, A, B, C, D, (A + C) * 0.5 - FVector3d(0, 0, ZBase));
				}
			}
		}
	}
}

void FBlockoutMeshGen::BuildPipe(FDynamicMesh3& M, double Diameter, double WallThickness, double Height, int32 Segments)
{
	double h = Height * 0.5;
	double InnerD = FMath::Max(1.0, Diameter - 2.0 * WallThickness);
	AddCylinderSide(M, Diameter, -h, h, Segments);
	// 内壁（法线朝内）
	double R = InnerD * 0.5;
	for (int32 i = 0; i < Segments; i++)
	{
		double A0 = 2.0 * PI * i / Segments, A1 = 2.0 * PI * (i + 1) / Segments;
		FVector3d P0(R * FMath::Cos(A0), R * FMath::Sin(A0), -h);
		FVector3d P1(R * FMath::Cos(A1), R * FMath::Sin(A1), -h);
		FVector3d T1(R * FMath::Cos(A1), R * FMath::Sin(A1), h);
		FVector3d T0(R * FMath::Cos(A0), R * FMath::Sin(A0), h);
		FVector3d Mid = (P0 + P1) * 0.5; Mid.Z = 0;
		AddQuadDir(M, P0, P1, T1, T0, -Mid.GetSafeNormal());
	}
	// 上下环面
	double Ro = Diameter * 0.5;
	for (int32 i = 0; i < Segments; i++)
	{
		double A0 = 2.0 * PI * i / Segments, A1 = 2.0 * PI * (i + 1) / Segments;
		FVector3d OT0(Ro * FMath::Cos(A0), Ro * FMath::Sin(A0), h), OT1(Ro * FMath::Cos(A1), Ro * FMath::Sin(A1), h);
		FVector3d IT0(R * FMath::Cos(A0), R * FMath::Sin(A0), h), IT1(R * FMath::Cos(A1), R * FMath::Sin(A1), h);
		AddQuadDir(M, OT0, OT1, IT1, IT0, FVector3d(0, 0, 1));
		FVector3d OB0(Ro * FMath::Cos(A0), Ro * FMath::Sin(A0), -h), OB1(Ro * FMath::Cos(A1), Ro * FMath::Sin(A1), -h);
		FVector3d IB0(R * FMath::Cos(A0), R * FMath::Sin(A0), -h), IB1(R * FMath::Cos(A1), R * FMath::Sin(A1), -h);
		AddQuadDir(M, OB0, OB1, IB1, IB0, FVector3d(0, 0, -1));
	}
}

void FBlockoutMeshGen::BuildStaircase(FDynamicMesh3& M, double L, double W, double H, int32 Steps, bool bSpiral)
{
	Steps = FMath::Clamp(Steps, 1, 64);
	if (bSpiral)
	{
		// 简化螺旋楼梯：绕 Z 轴旋转台阶
		double AngleStep = 360.0 / Steps;
		for (int32 i = 0; i < Steps; i++)
		{
			FDynamicMesh3 Tmp;
			BuildBox(Tmp, L, W, H / Steps);
			FTransform T(FRotator(0, AngleStep * i, 0), FVector(0, 0, -H * 0.5 + H * (i + 0.5) / Steps));
			// 逐顶点变换合并
			for (int32 vi = 0; vi < Tmp.VertexCount(); vi++)
			{
				FVector3d P = Tmp.GetVertex(vi);
				M.AppendVertex(T.TransformPosition(P));
			}
			for (int32 ti = 0; ti < Tmp.TriangleCount(); ti++)
			{
				FIndex3i Tri = Tmp.GetTriangle(ti);
				M.AppendTriangle(M.VertexCount() - Tmp.VertexCount() + Tri.A, M.VertexCount() - Tmp.VertexCount() + Tri.B, M.VertexCount() - Tmp.VertexCount() + Tri.C);
			}
		}
		return;
	}
	double Depth = L / Steps;
	for (int32 i = 0; i < Steps; i++)
	{
		double Sh = H * (i + 1) / Steps;
		FDynamicMesh3 Tmp;
		BuildBox(Tmp, Depth, W, Sh);
		int32 Base = M.VertexCount();
		FVector3d Off(-L * 0.5 + Depth * (i + 0.5), 0, -H * 0.5 + Sh * 0.5);
		for (int32 vi = 0; vi < Tmp.VertexCount(); vi++) M.AppendVertex(Tmp.GetVertex(vi) + Off);
		for (int32 ti = 0; ti < Tmp.TriangleCount(); ti++)
		{
			FIndex3i Tri = Tmp.GetTriangle(ti);
			M.AppendTriangle(Base + Tri.A, Base + Tri.B, Base + Tri.C);
		}
	}
}

void FBlockoutMeshGen::BuildRamp(FDynamicMesh3& M, double L, double W, double H)
{
	double hx = L * 0.5, hy = W * 0.5, hz = H * 0.5;
	FVector3d V0(-hx, -hy, -hz), V1(hx, -hy, -hz), V2(hx, hy, -hz), V3(-hx, hy, -hz);
	FVector3d V4(hx, -hy, hz), V5(hx, hy, hz);
	AddQuadDir(M, V0, V1, V2, V3, FVector3d(0, 0, -1));                // 底
	AddQuadDir(M, V0, V3, V5, V4, FVector3d(-H, 0, L).GetSafeNormal()); // 斜面（-X 低 +X 高）
	AddTriDir(M, V0, V4, V1, FVector3d(0, -1, 0));                      // -Y 侧面
	AddTriDir(M, V3, V2, V5, FVector3d(0, 1, 0));                       // +Y 侧面
	AddQuadDir(M, V1, V2, V5, V4, FVector3d(1, 0, 0));                  // +X 竖面
}

void FBlockoutMeshGen::BuildWedge(FDynamicMesh3& M, double L, double W, double H)
{
	double hx = L * 0.5, hy = W * 0.5, hz = H * 0.5;
	FVector3d B0(-hx, -hy, -hz), B1(hx, -hy, -hz), B2(hx, hy, -hz), B3(-hx, hy, -hz);
	FVector3d R0(-hx, 0, hz), R1(hx, 0, hz);
	AddQuadDir(M, B0, B1, B2, B3, FVector3d(0, 0, -1));            // 底
	AddQuadDir(M, B0, B3, R1, R0, FVector3d(0, -W, H).GetSafeNormal()); // -Y 斜面
	AddQuadDir(M, B1, B2, R1, R0, FVector3d(0, W, H).GetSafeNormal());  // +Y 斜面
	AddTriDir(M, B0, R0, B1, FVector3d(-1, 0, 0));
	AddTriDir(M, B3, B2, R1, FVector3d(1, 0, 0));
	AddTriDir(M, B0, R0, B3, FVector3d(-1, 0, 0));
	AddTriDir(M, B1, R1, B2, FVector3d(1, 0, 0));
}

void FBlockoutMeshGen::BuildColonnade(FDynamicMesh3& M, double ColumnDiameter, double ColumnHeight, int32 Count, double Spacing)
{
	Count = FMath::Clamp(Count, 1, 64);
	double StartX = -Spacing * (Count - 1) * 0.5;
	for (int32 i = 0; i < Count; i++)
	{
		FDynamicMesh3 Tmp;
		BuildCylinder(Tmp, ColumnDiameter, ColumnHeight, 16);
		int32 Base = M.VertexCount();
		for (int32 vi = 0; vi < Tmp.VertexCount(); vi++) M.AppendVertex(Tmp.GetVertex(vi) + FVector3d(StartX + Spacing * i, 0, 0));
		for (int32 ti = 0; ti < Tmp.TriangleCount(); ti++)
		{
			FIndex3i Tri = Tmp.GetTriangle(ti);
			M.AppendTriangle(Base + Tri.A, Base + Tri.B, Base + Tri.C);
		}
	}
}

void FBlockoutMeshGen::BuildFence(FDynamicMesh3& M, double L, double W, double H, double PostSpacing)
{
	// 矮墙 + 周期立柱
	{
		FDynamicMesh3 Tmp;
		BuildBox(Tmp, L, W, H * 0.5);
		int32 Base = M.VertexCount();
		for (int32 vi = 0; vi < Tmp.VertexCount(); vi++) M.AppendVertex(Tmp.GetVertex(vi) + FVector3d(0, 0, -H * 0.25));
		for (int32 ti = 0; ti < Tmp.TriangleCount(); ti++)
		{
			FIndex3i Tri = Tmp.GetTriangle(ti);
			M.AppendTriangle(Base + Tri.A, Base + Tri.B, Base + Tri.C);
		}
	}
	int32 NumPosts = FMath::Clamp(FMath::FloorToInt(L / FMath::Max(50.0, PostSpacing)) + 1, 1, 64);
	for (int32 i = 0; i < NumPosts; i++)
	{
		FDynamicMesh3 Tmp;
		BuildBox(Tmp, W * 1.3, W * 1.3, H);
		int32 Base = M.VertexCount();
		double X = -L * 0.5 + L * i / (NumPosts - 1);
		for (int32 vi = 0; vi < Tmp.VertexCount(); vi++) M.AppendVertex(Tmp.GetVertex(vi) + FVector3d(X, 0, 0));
		for (int32 ti = 0; ti < Tmp.TriangleCount(); ti++)
		{
			FIndex3i Tri = Tmp.GetTriangle(ti);
			M.AppendTriangle(Base + Tri.A, Base + Tri.B, Base + Tri.C);
		}
	}
}

void FBlockoutMeshGen::BuildStairwell(FDynamicMesh3& M, double L, double W, double H, int32 Steps, double WallThickness)
{
	// 楼梯 + 双侧墙 + 上层平台（M5 快速生成：楼梯间）
	auto AppendBox = [&M](double X, double Y, double Z, const FVector3d& Off)
	{
		FDynamicMesh3 Tmp;
		BuildBox(Tmp, X, Y, Z);
		int32 Base = M.VertexCount();
		for (int32 vi = 0; vi < Tmp.VertexCount(); vi++) M.AppendVertex(Tmp.GetVertex(vi) + Off);
		for (int32 ti = 0; ti < Tmp.TriangleCount(); ti++)
		{
			FIndex3i Tri = Tmp.GetTriangle(ti);
			M.AppendTriangle(Base + Tri.A, Base + Tri.B, Base + Tri.C);
		}
	};

	// 1) 楼梯主体（沿 +Y 上升），台阶宽 W - 2*墙厚
	const double InnerW = FMath::Max(50.0, W - WallThickness * 2.0);
	{
		FDynamicMesh3 Tmp;
		BuildStaircase(Tmp, L, InnerW, H, Steps, false);
		int32 Base = M.VertexCount();
		for (int32 vi = 0; vi < Tmp.VertexCount(); vi++) M.AppendVertex(Tmp.GetVertex(vi));
		for (int32 ti = 0; ti < Tmp.TriangleCount(); ti++)
		{
			FIndex3i Tri = Tmp.GetTriangle(ti);
			M.AppendTriangle(Base + Tri.A, Base + Tri.B, Base + Tri.C);
		}
	}
	// 2) 双侧墙（随楼梯高度渐升的近似：整段满高墙）
	const double WallH = H + 100.0;
	AppendBox(WallThickness, L, WallH, FVector3d(-InnerW * 0.5 - WallThickness * 0.5, 0, -H * 0.5 + WallH * 0.5));
	AppendBox(WallThickness, L, WallH, FVector3d(InnerW * 0.5 + WallThickness * 0.5, 0, -H * 0.5 + WallH * 0.5));
	// 3) 上层平台（Y 端，厚 20）
	AppendBox(W, W * 0.8, 20.0, FVector3d(0, L * 0.5 + W * 0.4, -H * 0.5 + 10.0));
}

void FBlockoutMeshGen::BuildRectPrismHole(FDynamicMesh3& M, double Width, double Height, double Depth)
{
	// 截面 XZ（宽 Width 高 Height），沿 Y 拉伸 Depth
	double hw = Width * 0.5, hh = Height * 0.5, hd = Depth * 0.5;
	FVector3d V0(-hw, -hd, -hh), V1(hw, -hd, -hh), V2(hw, -hd, hh), V3(-hw, -hd, hh);
	FVector3d V4(-hw, hd, -hh), V5(hw, hd, -hh), V6(hw, hd, hh), V7(-hw, hd, hh);
	AddQuadDir(M, V0, V1, V2, V3, FVector3d(0, -1, 0));
	AddQuadDir(M, V5, V4, V7, V6, FVector3d(0, 1, 0));
	AddQuadDir(M, V0, V4, V5, V1, FVector3d(0, 0, -1));
	AddQuadDir(M, V3, V2, V6, V7, FVector3d(0, 0, 1));
	AddQuadDir(M, V0, V3, V7, V4, FVector3d(-1, 0, 0));
	AddQuadDir(M, V1, V5, V6, V2, FVector3d(1, 0, 0));
}

void FBlockoutMeshGen::BuildCirclePrismHole(FDynamicMesh3& M, double Diameter, double Depth, int32 Segments)
{
	Segments = FMath::Clamp(Segments, 3, 64);
	double R = Diameter * 0.5, hd = Depth * 0.5;
	for (int32 i = 0; i < Segments; i++)
	{
		double A0 = 2.0 * PI * i / Segments, A1 = 2.0 * PI * (i + 1) / Segments;
		FVector3d P0(R * FMath::Cos(A0), -hd, R * FMath::Sin(A0));
		FVector3d P1(R * FMath::Cos(A1), -hd, R * FMath::Sin(A1));
		FVector3d T1(R * FMath::Cos(A1), hd, R * FMath::Sin(A1));
		FVector3d T0(R * FMath::Cos(A0), hd, R * FMath::Sin(A0));
		FVector3d Mid = (P0 + P1) * 0.5; Mid.Y = 0;
		AddQuadDir(M, P0, P1, T1, T0, Mid.GetSafeNormal());
		int32 C0 = M.AppendVertex(FVector3d(0, -hd, 0)), C1 = M.AppendVertex(FVector3d(0, hd, 0));
		int32 A = M.AppendVertex(P0), B = M.AppendVertex(P1), C = M.AppendVertex(T1), D = M.AppendVertex(T0);
		M.AppendTriangle(C0, A, B);
		M.AppendTriangle(C1, C, D);
	}
}

void FBlockoutMeshGen::BuildPolygonPrismHole(FDynamicMesh3& M, const TArray<FVector2D>& PolygonXZ, double Depth)
{
	int32 N = PolygonXZ.Num();
	if (N < 3) return;
	double hd = Depth * 0.5;
	for (int32 i = 0; i < N; i++)
	{
		const FVector2D& A = PolygonXZ[i];
		const FVector2D& B = PolygonXZ[(i + 1) % N];
		FVector3d P0(A.X, -hd, A.Y), P1(B.X, -hd, B.Y), T1(B.X, hd, B.Y), T0(A.X, hd, A.Y);
		FVector3d Out((A + B) * 0.5 - FVector2D(0, 0), 0); // 径向近似
		AddQuadDir(M, P0, P1, T1, T0, FVector3d(B.Y - A.Y, 0, -(B.X - A.X)));
		int32 C0 = M.AppendVertex(FVector3d(0, -hd, 0)), C1 = M.AppendVertex(FVector3d(0, hd, 0));
		int32 v0 = M.AppendVertex(P0), v1 = M.AppendVertex(P1), v2 = M.AppendVertex(T1), v3 = M.AppendVertex(T0);
		M.AppendTriangle(C0, v0, v1);
		M.AppendTriangle(C1, v2, v3);
	}
}

bool FBlockoutMeshGen::BooleanOp(FDynamicMesh3& A, const FDynamicMesh3& B, const FTransform& BTransform, bool bSubtract)
{
	FDynamicMesh3 Result;
	FTransformSRT3d XformA = FTransformSRT3d::Identity();
	FTransformSRT3d XformB(BTransform);
	FMeshBoolean Op(&A, XformA, &B, XformB, &Result,
		bSubtract ? FMeshBoolean::EBooleanOp::Difference : FMeshBoolean::EBooleanOp::Union);
	bool bOk = Op.Compute();
	if (bOk && Result.TriangleCount() > 0)
	{
		A = MoveTemp(Result);
	}
	return bOk && A.TriangleCount() > 0;
}

void FBlockoutMeshGen::BuildShape(EBlockoutShape Shape, const FBlockoutShapeParams& P, FDynamicMesh3& OutMesh)
{
	OutMesh.Clear();
	switch (Shape)
	{
	case EBlockoutShape::Cube:
	case EBlockoutShape::Floor:
		BuildBox(OutMesh, P.Length, P.Width, P.Height);
		break;
	case EBlockoutShape::Wall:
		BuildBox(OutMesh, P.Length, P.Thickness, P.Height);
		break;
	case EBlockoutShape::Staircase:
		BuildStaircase(OutMesh, P.Length, P.Width, P.Height, P.Steps, false);
		break;
	case EBlockoutShape::Ramp:
		BuildRamp(OutMesh, P.Length, P.Width, P.Height);
		break;
	case EBlockoutShape::Wedge:
		BuildWedge(OutMesh, P.Length, P.Width, P.Height);
		break;
	case EBlockoutShape::Arch:
	case EBlockoutShape::DoorWall:
	case EBlockoutShape::WindowWall:
	{
		BuildBox(OutMesh, P.Length, P.Thickness, P.Height);
		FDynamicMesh3 Hole;
		double HoleH = (Shape == EBlockoutShape::Arch) ? P.HoleHeight * 0.5 : P.HoleHeight;
		double ZOff = (Shape == EBlockoutShape::WindowWall) ? P.HoleOffsetZ
			: (Shape == EBlockoutShape::Arch ? -P.Height * 0.5 + HoleH : -P.Height * 0.5 + P.HoleHeight * 0.5);
		if (Shape == EBlockoutShape::Arch)
		{
			// 矩形下部 + 圆形上部组合洞
			FDynamicMesh3 Rect;
			BuildRectPrismHole(Rect, P.HoleWidth, HoleH, P.Thickness * 2);
			FDynamicMesh3 Circ;
			BuildCirclePrismHole(Circ, P.HoleWidth, P.Thickness * 2, 24);
			BooleanOp(Rect, Circ, FTransform(FVector(0, 0, HoleH * 0.5)), false);
			Hole = Rect;
			ZOff = -P.Height * 0.5 + HoleH * 0.5;
		}
		else
		{
			BuildRectPrismHole(Hole, P.HoleWidth, P.HoleHeight, P.Thickness * 2);
		}
		BooleanOp(OutMesh, Hole, FTransform(FVector(0, 0, ZOff)), true);
		break;
	}
	case EBlockoutShape::Cylinder:
		BuildCylinder(OutMesh, P.Length, P.Height, P.Segments);
		break;
	case EBlockoutShape::Sphere:
		BuildSphere(OutMesh, P.Length, P.Segments);
		break;
	case EBlockoutShape::Capsule:
		BuildCapsule(OutMesh, P.Length, P.Height, P.Segments);
		break;
	case EBlockoutShape::Pipe:
		BuildPipe(OutMesh, P.Length, P.Thickness, P.Height, P.Segments);
		break;
	case EBlockoutShape::Colonnade:
		BuildColonnade(OutMesh, P.Length, P.Height, P.ArrayCount, P.ArraySpacing);
		break;
	case EBlockoutShape::Fence:
		BuildFence(OutMesh, P.Length, P.Thickness, P.Height, P.ArraySpacing);
		break;
	case EBlockoutShape::Stairwell:
		BuildStairwell(OutMesh, P.Length, P.Width, P.Height, P.Steps, P.Thickness);
		break;
	case EBlockoutShape::DoorPiece:
		BuildBox(OutMesh, P.Length, P.Width, P.Height);
		break;
	case EBlockoutShape::WindowPiece:
		BuildBox(OutMesh, P.Length, P.Width, P.Height);
		break;
	default:
		BuildBox(OutMesh, P.Length, P.Width, P.Height);
		break;
	}
}

void FBlockoutMeshGen::FinalizeMesh(FDynamicMesh3& M, bool bSmoothNormals, double UVScale)
{
	M.EnableAttributes();
	if (!M.HasAttributes() || M.Attributes()->NumNormalLayers() == 0) M.Attributes()->SetNumNormalLayers(1);
	if (!M.HasAttributes() || M.Attributes()->NumUVLayers() == 0) M.Attributes()->SetNumUVLayers(1);
	FDynamicMeshNormalOverlay* NOverlay = M.Attributes()->GetNormalLayer(0);
	FDynamicMeshUVOverlay* UOverlay = M.Attributes()->GetUVLayer(0);
	if (NOverlay) NOverlay->ClearElements();
	if (UOverlay) UOverlay->ClearElements();

	// 面法线缓存 + 顶点->相邻面映射（用于平滑法线）
	TArray<FVector3d> FaceNormals;
	FaceNormals.SetNum(M.TriangleCount());
	TMap<int32, TArray<int32>> VtxTris;
	for (int32 ti = 0; ti < M.TriangleCount(); ti++)
	{
		FIndex3i Tri = M.GetTriangle(ti);
		FVector3d A = M.GetVertex(Tri.A), B = M.GetVertex(Tri.B), C = M.GetVertex(Tri.C);
		FaceNormals[ti] = FVector3d::CrossProduct(B - A, C - A).GetSafeNormal();
		VtxTris.FindOrAdd(Tri.A).Add(ti);
		VtxTris.FindOrAdd(Tri.B).Add(ti);
		VtxTris.FindOrAdd(Tri.C).Add(ti);
	}
	// 位置焊接表（用于平滑法线）
	TMap<uint64, TArray<int32>> WeldMap;
	if (bSmoothNormals)
	{
		for (int32 vi = 0; vi < M.VertexCount(); vi++)
		{
			FVector3d V = M.GetVertex(vi);
			uint64 Key = HashCombine(GetTypeHash(FVector3f(V)), GetTypeHash(V.Z));
			WeldMap.FindOrAdd(Key).Add(vi);
		}
	}

	auto SmoothNormalAt = [&](int32 TriId, int32 CornerIdx) -> FVector3f
	{
		FVector3d FN = FaceNormals[TriId];
		if (!bSmoothNormals) return FVector3f(FN);
		int32 CornerVid = M.GetTriangle(TriId)[CornerIdx];
		FVector3d V = M.GetVertex(CornerVid);
		uint64 Key = HashCombine(GetTypeHash(FVector3f(V)), GetTypeHash(V.Z));
		FVector3d Acc = FN;
		if (const TArray<int32>* Ids = WeldMap.Find(Key))
		{
			for (int32 Other : *Ids)
			{
				if (const TArray<int32>* Tris = VtxTris.Find(Other))
				{
					for (int32 tid : *Tris)
					{
						if (FVector3d::DotProduct(FaceNormals[tid], FN) > 0.5) Acc += FaceNormals[tid];
					}
				}
			}
		}
		return FVector3f(Acc.GetSafeNormal());
	};

	for (int32 ti = 0; ti < M.TriangleCount(); ti++)
	{
		FIndex3i Tri = M.GetTriangle(ti);
		FVector3d V[3] = { M.GetVertex(Tri.A), M.GetVertex(Tri.B), M.GetVertex(Tri.C) };
		FVector3d FN = FVector3d::CrossProduct(V[1] - V[0], V[2] - V[0]).GetSafeNormal();
		// 盒式 UV：按主轴投影
		int32 Axis = 0;
		FMath::Abs(FN.X) > FMath::Abs(FN.Y) ? (Axis = FMath::Abs(FN.X) > FMath::Abs(FN.Z) ? 0 : 2) : (Axis = FMath::Abs(FN.Y) > FMath::Abs(FN.Z) ? 1 : 2);
		FVector2f UV[3];
		for (int32 c = 0; c < 3; c++)
		{
			if (Axis == 0) UV[c] = FVector2f(V[c].Y / UVScale, V[c].Z / UVScale);
			else if (Axis == 1) UV[c] = FVector2f(V[c].X / UVScale, V[c].Z / UVScale);
			else UV[c] = FVector2f(V[c].X / UVScale, V[c].Y / UVScale);
		}
		if (UOverlay)
		{
			int32 E0 = UOverlay->AppendElement(UV[0]), E1 = UOverlay->AppendElement(UV[1]), E2 = UOverlay->AppendElement(UV[2]);
			UOverlay->SetTriangle(ti, FIndex3i(E0, E1, E2));
		}
		if (NOverlay)
		{
			int32 E0 = NOverlay->AppendElement(SmoothNormalAt(ti, 0));
			int32 E1 = NOverlay->AppendElement(SmoothNormalAt(ti, 1));
			int32 E2 = NOverlay->AppendElement(SmoothNormalAt(ti, 2));
			NOverlay->SetTriangle(ti, FIndex3i(E0, E1, E2));
		}
	}
}
