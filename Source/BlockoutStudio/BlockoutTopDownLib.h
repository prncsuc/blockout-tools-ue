// BlockoutStudio - T2: 俯视 2D 出图（PNG 位图 / SVG 矢量）
// 仅依赖 ImageWrapper + 项目内 Actor 数据，输出白盒俯视平面图
#pragma once

#include "CoreMinimal.h"

class ABlockoutActor;

// 出图选项：可选尺寸标注 / 网格 / 名称
struct FBlockoutTopDownOptions
{
	bool bWithDims = true;
	bool bWithGrid = true;
	bool bWithNames = true;
	int32 MaxResolution = 2048; // 最长边像素
	float GridStep = 100.f;     // 网格间距（uu），100uu = 1m
};

class BLOCKOUTSTUDIO_API FBlockoutTopDownLib
{
public:
	// 俯视 2D 图导出：PNG 位图
	static bool ExportPNG(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, const FBlockoutTopDownOptions& Opt);

	// 俯视 2D 图导出：SVG 矢量（原生文字标注）
	static bool ExportSVG(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, const FBlockoutTopDownOptions& Opt);
};
