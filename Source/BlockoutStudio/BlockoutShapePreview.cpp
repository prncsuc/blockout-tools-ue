// BlockoutStudio - 形状几何预览控件实现：纯 Slate 线框绘制
#include "BlockoutShapePreview.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"

void SBlockoutShapePreview::Construct(const FArguments& InArgs)
{
	ShapeAttr = InArgs._Shape;
	LineColorAttr = InArgs._LineColor;
	FillColorAttr = InArgs._FillColor;
	LineThicknessAttr = InArgs._LineThickness;
}

FVector2D SBlockoutShapePreview::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(88.f, 54.f);
}

void SBlockoutShapePreview::DrawPolyline(FSlateWindowElementList& OutDrawElements, const FGeometry& Geo,
	int32 LayerId, const TArray<TArray<FVector2f>>& Polylines, const FLinearColor& Color, float Thickness) const
{
	const FVector2f Size(Geo.GetLocalSize());
	for (const TArray<FVector2f>& Line : Polylines)
	{
		if (Line.Num() < 2) continue;
		TArray<FVector2f> Pts;
		Pts.Reserve(Line.Num());
		for (const FVector2f& P : Line)
		{
			Pts.Add(FVector2f(P.X * Size.X, P.Y * Size.Y));
		}
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId, Geo.ToPaintGeometry(),
			Pts, ESlateDrawEffect::None, Color, true, Thickness);
	}
}

// 归一化坐标系：X 0..1 左->右，Y 0..1 上->下。各形状返回若干折线。
TArray<TArray<FVector2f>> SBlockoutShapePreview::GetShapePolylines(EBlockoutShape Shape)
{
	TArray<TArray<FVector2f>> Out;

	auto P = [](float X, float Y) { return FVector2f(X, Y); };
	auto Line = [&Out](std::initializer_list<FVector2f> Pts)
	{
		Out.Add(TArray<FVector2f>(Pts));
	};
	// 折线闭合
	auto Box = [&Out](float X0, float Y0, float X1, float Y1)
	{
		Out.Add({ FVector2f(X0, Y0), FVector2f(X1, Y0), FVector2f(X1, Y1), FVector2f(X0, Y1), FVector2f(X0, Y0) });
	};
	// 圆/弧 多段线拟合
	auto Arc = [&Out](FVector2f C, float Rx, float Ry, float A0Deg, float A1Deg, int32 Segs, bool bClose)
	{
		TArray<FVector2f> Pts;
		for (int32 i = 0; i <= Segs; ++i)
		{
			const float A = FMath::DegreesToRadians(FMath::Lerp(A0Deg, A1Deg, float(i) / Segs));
			Pts.Add(FVector2f(C.X + Rx * FMath::Cos(A), C.Y + Ry * FMath::Sin(A)));
		}
		if (bClose) Pts.Add(Pts[0]);
		Out.Add(MoveTemp(Pts));
	};

	switch (Shape)
	{
	case EBlockoutShape::Cube:
	{
		// 轴测立方体
		Box(0.18f, 0.32f, 0.62f, 0.78f);
		Line({ P(0.18f, 0.32f), P(0.34f, 0.18f), P(0.80f, 0.18f), P(0.62f, 0.32f) });
		Line({ P(0.62f, 0.78f), P(0.80f, 0.62f), P(0.80f, 0.18f) });
		Line({ P(0.34f, 0.18f), P(0.34f, 0.62f), P(0.18f, 0.78f) });
		Line({ P(0.34f, 0.62f), P(0.80f, 0.62f) });
		break;
	}
	case EBlockoutShape::Wall:
	{
		Box(0.08f, 0.38f, 0.92f, 0.68f);
		Line({ P(0.08f, 0.38f), P(0.20f, 0.26f), P(0.92f, 0.26f) }); // 顶面
		Line({ P(0.92f, 0.38f), P(0.92f, 0.26f) });
		Line({ P(0.92f, 0.68f), P(0.92f, 0.56f), P(0.20f, 0.56f) });
		Line({ P(0.08f, 0.68f), P(0.08f, 0.56f) });
		break;
	}
	case EBlockoutShape::Staircase:
	{
		// 阶梯折线 + 底线
		TArray<FVector2f> Steps;
		float X = 0.12f, Y = 0.80f;
		const float DX = 0.13f, DY = 0.11f;
		Steps.Add(P(X, Y));
		for (int32 i = 0; i < 5; ++i)
		{
			Y -= DY; Steps.Add(P(X, Y)); X += DX; Steps.Add(P(X, Y));
		}
		Out.Add(Steps);
		Line({ P(0.12f, 0.80f), P(0.77f, 0.80f) });
		Line({ P(0.77f, 0.80f), P(0.77f, 0.25f) });
		break;
	}
	case EBlockoutShape::Ramp:
	{
		Line({ P(0.12f, 0.80f), P(0.88f, 0.80f), P(0.88f, 0.25f), P(0.12f, 0.80f) });
		break;
	}
	case EBlockoutShape::Wedge:
	{
		// 楔形：直角梯形+斜边
		Box(0.16f, 0.55f, 0.84f, 0.80f);
		Line({ P(0.16f, 0.55f), P(0.84f, 0.22f) });
		Line({ P(0.84f, 0.22f), P(0.84f, 0.55f) });
		break;
	}
	case EBlockoutShape::Arch:
	{
		Box(0.12f, 0.30f, 0.88f, 0.82f);
		Arc(FVector2f(0.5f, 0.30f), 0.26f, 0.22f, 180.f, 0.f, 12, false); // 拱顶
		Arc(FVector2f(0.5f, 0.82f), 0.18f, 0.24f, 180.f, 0.f, 12, false); // 洞
		Line({ P(0.32f, 0.82f), P(0.32f, 0.58f) });
		Line({ P(0.68f, 0.82f), P(0.68f, 0.58f) });
		break;
	}
	case EBlockoutShape::DoorWall:
	{
		Box(0.10f, 0.22f, 0.90f, 0.82f);
		// 门洞
		Box(0.38f, 0.42f, 0.62f, 0.82f);
		break;
	}
	case EBlockoutShape::WindowWall:
	{
		Box(0.10f, 0.22f, 0.90f, 0.82f);
		Box(0.34f, 0.38f, 0.66f, 0.58f);
		break;
	}
	case EBlockoutShape::Cylinder:
	{
		Box(0.28f, 0.24f, 0.72f, 0.80f);
		Arc(FVector2f(0.5f, 0.24f), 0.22f, 0.08f, 0.f, 360.f, 20, true);  // 顶椭圆
		Arc(FVector2f(0.5f, 0.80f), 0.22f, 0.08f, 0.f, 360.f, 20, true);  // 底椭圆
		break;
	}
	case EBlockoutShape::Sphere:
	{
		Arc(FVector2f(0.5f, 0.5f), 0.34f, 0.34f, 0.f, 360.f, 24, true);
		Arc(FVector2f(0.5f, 0.5f), 0.34f, 0.12f, 0.f, 360.f, 18, true);  // 赤道
		break;
	}
	case EBlockoutShape::Capsule:
	{
		Line({ P(0.32f, 0.26f), P(0.32f, 0.74f) });
		Line({ P(0.68f, 0.26f), P(0.68f, 0.74f) });
		Arc(FVector2f(0.5f, 0.26f), 0.18f, 0.14f, 180.f, 0.f, 12, false); // 顶半圆
		Arc(FVector2f(0.5f, 0.74f), 0.18f, 0.14f, 0.f, 180.f, 12, false); // 底半圆
		break;
	}
	case EBlockoutShape::Pipe:
	{
		Arc(FVector2f(0.5f, 0.5f), 0.32f, 0.30f, 0.f, 360.f, 24, true);
		Arc(FVector2f(0.5f, 0.5f), 0.16f, 0.15f, 0.f, 360.f, 20, true);
		break;
	}
	case EBlockoutShape::Colonnade:
	{
		Line({ P(0.08f, 0.18f), P(0.92f, 0.18f) }); // 顶梁
		Line({ P(0.08f, 0.28f), P(0.92f, 0.28f) });
		Line({ P(0.08f, 0.82f), P(0.92f, 0.82f) }); // 台基
		for (float X : { 0.16f, 0.38f, 0.60f, 0.82f })
		{
			Box(X - 0.035f, 0.28f, X + 0.035f, 0.82f);
		}
		break;
	}
	case EBlockoutShape::Fence:
	{
		Line({ P(0.08f, 0.30f), P(0.92f, 0.30f) }); // 横杆
		Line({ P(0.08f, 0.44f), P(0.92f, 0.44f) });
		for (float X : { 0.12f, 0.30f, 0.48f, 0.66f, 0.84f })
		{
			Line({ P(X, 0.20f), P(X, 0.82f) });     // 立柱
		}
		Line({ P(0.08f, 0.82f), P(0.92f, 0.82f) });
		break;
	}
	case EBlockoutShape::Stairwell:
	{
		Box(0.12f, 0.16f, 0.88f, 0.86f);            // 外轮廓
		Box(0.38f, 0.36f, 0.62f, 0.62f);            // 中间井
		Line({ P(0.12f, 0.86f), P(0.38f, 0.62f) }); // 跑道斜线
		Line({ P(0.88f, 0.16f), P(0.62f, 0.36f) });
		break;
	}
	case EBlockoutShape::Floor:
	{
		// 扁平平行四边形（地板）
		Line({ P(0.10f, 0.62f), P(0.52f, 0.38f), P(0.92f, 0.55f), P(0.50f, 0.80f), P(0.10f, 0.62f) });
		Line({ P(0.28f, 0.51f), P(0.70f, 0.68f) }); // 分格线
		Line({ P(0.70f, 0.465f), P(0.30f, 0.70f) });
		break;
	}
	case EBlockoutShape::DoorPiece:
	{
		Box(0.28f, 0.16f, 0.72f, 0.86f);            // 门框
		Box(0.36f, 0.26f, 0.64f, 0.86f);            // 门板
		Arc(FVector2f(0.60f, 0.56f), 0.035f, 0.035f, 0.f, 360.f, 10, true); // 门把手
		break;
	}
	case EBlockoutShape::WindowPiece:
	{
		Box(0.20f, 0.22f, 0.80f, 0.78f);            // 窗框
		Line({ P(0.50f, 0.22f), P(0.50f, 0.78f) }); // 田字分格
		Line({ P(0.20f, 0.50f), P(0.80f, 0.50f) });
		break;
	}
	case EBlockoutShape::Custom:
	default:
	{
		Box(0.16f, 0.20f, 0.84f, 0.80f);
		Line({ P(0.16f, 0.20f), P(0.84f, 0.80f) }); // 网格线
		Line({ P(0.84f, 0.20f), P(0.16f, 0.80f) });
		Line({ P(0.50f, 0.20f), P(0.50f, 0.80f) });
		Line({ P(0.16f, 0.50f), P(0.84f, 0.50f) });
		break;
	}
	}
	return Out;
}

int32 SBlockoutShapePreview::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const EBlockoutShape Shape = ShapeAttr.Get();
	const FLinearColor LineColor = LineColorAttr.Get();
	const FLinearColor FillColor = FillColorAttr.Get();
	const float Thickness = LineThicknessAttr.Get();

	// 背景填充
	FSlateColorBrush Brush(FillColor);
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		&Brush, ESlateDrawEffect::None, FillColor);

	// 几何线框
	DrawPolyline(OutDrawElements, AllottedGeometry, LayerId + 1,
		GetShapePolylines(Shape), LineColor, Thickness);

	return LayerId + 1;
}
