// BlockoutStudio - 形状几何预览控件（Slate 自绘缩略图，无纹理资源依赖）
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "BlockoutTypes.h"

// 按 EBlockoutShape 在 Slate 中直接绘制 2D 轴测风格几何示意，
// 用于左侧创建面板卡片缩略图与场景树类型图标。
class BLOCKOUTSTUDIO_API SBlockoutShapePreview : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SBlockoutShapePreview)
		: _Shape(EBlockoutShape::Cube)
		, _LineColor(FLinearColor(0.85f, 0.85f, 0.9f))
		, _FillColor(FLinearColor(0.18f, 0.22f, 0.3f, 1.f))
		, _LineThickness(1.5f)
	{}
		SLATE_ATTRIBUTE(EBlockoutShape, Shape)
		SLATE_ATTRIBUTE(FLinearColor, LineColor)
		SLATE_ATTRIBUTE(FLinearColor, FillColor)
		SLATE_ATTRIBUTE(float, LineThickness)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SLeafWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	TAttribute<EBlockoutShape> ShapeAttr;
	TAttribute<FLinearColor> LineColorAttr;
	TAttribute<FLinearColor> FillColorAttr;
	TAttribute<float> LineThicknessAttr;

	// 将单位几何（0..1 归一化坐标，Y 向下）映射到控件局部空间并画线段集合
	void DrawPolyline(FSlateWindowElementList& OutDrawElements, const FGeometry& Geo, int32 LayerId,
		const TArray<TArray<FVector2f>>& Polylines, const FLinearColor& Color, float Thickness) const;
	static TArray<TArray<FVector2f>> GetShapePolylines(EBlockoutShape Shape);
};
