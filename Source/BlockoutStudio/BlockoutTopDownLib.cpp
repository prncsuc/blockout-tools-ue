// BlockoutStudio - T2: 俯视 2D 出图（PNG 位图 / SVG 矢量）
#include "BlockoutTopDownLib.h"
#include "BlockoutActor.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"

namespace
{
	// ---------------- 5x7 点阵字体（PNG 文本标注用，仅 ASCII；其余字符以占位块渲染） ----------------
	struct FGlyph { TCHAR C; const char* Rows[7]; };

	const FGlyph GTable[] =
	{
		{ TEXT('0'), { ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." } },
		{ TEXT('1'), { "..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###." } },
		{ TEXT('2'), { ".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####" } },
		{ TEXT('3'), { "#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###." } },
		{ TEXT('4'), { "...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#." } },
		{ TEXT('5'), { "#####", "#....", "####.", "....#", "....#", "#...#", ".###." } },
		{ TEXT('6'), { "..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###." } },
		{ TEXT('7'), { "#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..." } },
		{ TEXT('8'), { ".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###." } },
		{ TEXT('9'), { ".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.." } },
		{ TEXT('.'), { ".....", ".....", ".....", ".....", ".....", ".##..", ".##.." } },
		{ TEXT('-'), { ".....", ".....", ".....", "#####", ".....", ".....", "....." } },
		{ TEXT('m'), { ".....", ".....", "##.#.", "#.#.#", "#.#.#", "#.#.#", "#.#.#" } },
		{ TEXT('x'), { ".....", ".....", "#...#", ".#.#.", "..#..", ".#.#.", "#...#" } },
		{ TEXT(' '), { ".....", ".....", ".....", ".....", ".....", ".....", "....." } },
		{ TEXT('A'), { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
		{ TEXT('B'), { "####.", "#...#", "#...#", "####.", "#...#", "#...#", "####." } },
		{ TEXT('C'), { ".###.", "#...#", "#....", "#....", "#....", "#...#", ".###." } },
		{ TEXT('D'), { "####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####." } },
		{ TEXT('E'), { "#####", "#....", "#....", "####.", "#....", "#....", "#####" } },
		{ TEXT('F'), { "#####", "#....", "#....", "####.", "#....", "#....", "#...." } },
		{ TEXT('G'), { ".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####" } },
		{ TEXT('H'), { "#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
		{ TEXT('I'), { ".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###." } },
		{ TEXT('J'), { "..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.." } },
		{ TEXT('K'), { "#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#" } },
		{ TEXT('L'), { "#....", "#....", "#....", "#....", "#....", "#....", "#####" } },
		{ TEXT('M'), { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" } },
		{ TEXT('N'), { "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#" } },
		{ TEXT('O'), { ".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
		{ TEXT('P'), { "####.", "#...#", "#...#", "####.", "#....", "#....", "#...." } },
		{ TEXT('Q'), { ".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#" } },
		{ TEXT('R'), { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" } },
		{ TEXT('S'), { ".####", "#....", "#....", ".###.", "....#", "....#", "####." } },
		{ TEXT('T'), { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
		{ TEXT('U'), { "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
		{ TEXT('V'), { "#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.." } },
		{ TEXT('W'), { "#...#", "#...#", "#...#", "#.#.#", "#.#.#", "##.##", "#...#" } },
		{ TEXT('X'), { "#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#" } },
		{ TEXT('Y'), { "#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.." } },
		{ TEXT('Z'), { "#####", "....#", "...#.", "..#..", ".#...", "#....", "#####" } },
	};

	const char* const Placeholder[7] = { "#####", "#...#", "#...#", "#...#", "#...#", "#...#", "#####" };

	const char* GlyphRows(TCHAR C)
	{
		C = FChar::ToUpper(C);
		for (const FGlyph& G : GTable)
		{
			if (G.C == C) return G.Rows[0];
		}
		// 逐行取用需要返回7行，这里改用整表检索（见 DrawText5x7）
		return nullptr;
	}

	const FGlyph* FindGlyph(TCHAR C)
	{
		C = FChar::ToUpper(C);
		for (const FGlyph& G : GTable)
		{
			if (G.C == C) return &G;
		}
		return nullptr;
	}

	// ---------------- 画布 ----------------
	struct FCanvas2D
	{
		TArray<FColor> Pixels;
		int32 W = 0, H = 0;

		void Init(int32 InW, int32 InH, FColor Clear)
		{
			W = InW; H = InH;
			Pixels.Init(Clear, W * H);
		}
		void Set(int32 X, int32 Y, FColor C)
		{
			if (X >= 0 && Y >= 0 && X < W && Y < H) Pixels[Y * W + X] = C;
		}
		void FillRect(int32 X0, int32 Y0, int32 X1, int32 Y1, FColor C)
		{
			for (int32 Y = Y0; Y <= Y1; ++Y)
				for (int32 X = X0; X <= X1; ++X) Set(X, Y, C);
		}
		void StrokeRect(int32 X0, int32 Y0, int32 X1, int32 Y1, FColor C)
		{
			for (int32 X = X0; X <= X1; ++X) { Set(X, Y0, C); Set(X, Y1, C); }
			for (int32 Y = Y0; Y <= Y1; ++Y) { Set(X0, Y, C); Set(X1, Y, C); }
		}
		void Line(int32 X0, int32 Y0, int32 X1, int32 Y1, FColor C)
		{
			const int32 DX = FMath::Abs(X1 - X0), SX = X0 < X1 ? 1 : -1;
			const int32 DY = -FMath::Abs(Y1 - Y0), SY = Y0 < Y1 ? 1 : -1;
			int32 Err = DX + DY;
			while (true)
			{
				Set(X0, Y0, C);
				if (X0 == X1 && Y0 == Y1) break;
				const int32 E2 = 2 * Err;
				if (E2 >= DY) { Err += DY; X0 += SX; }
				if (E2 <= DX) { Err += DX; Y0 += SY; }
			}
		}
		void Text5x7(const FString& Str, int32 X, int32 Y, FColor C, int32 Scale)
		{
			int32 CX = X;
			for (int32 i = 0; i < Str.Len(); ++i)
			{
				const TCHAR Ch = Str[i];
				const FGlyph* G = FindGlyph(Ch);
				const char* const* Rows = G ? G->Rows : Placeholder;
				for (int32 r = 0; r < 7; ++r)
				{
					for (int32 c = 0; c < 5; ++c)
					{
						if (Rows[r][c] == '#')
						{
							for (int32 sy = 0; sy < Scale; ++sy)
								for (int32 sx = 0; sx < Scale; ++sx)
									Set(CX + c * Scale + sx, Y + r * Scale + sy, C);
						}
					}
				}
				CX += 6 * Scale;
			}
		}
	};

	// 世界 XY 包围盒
	struct FFootprint
	{
		FVector2D Min, Max;
		FLinearColor Color;
		FString Name;
	};

	void GatherFootprints(const TArray<ABlockoutActor*>& Actors, TArray<FFootprint>& Out)
	{
		for (ABlockoutActor* A : Actors)
		{
			if (!IsValid(A)) continue;
			const FBox B = A->GetComponentsBoundingBox(true);
			FFootprint F;
			F.Min = FVector2D(B.Min.X, B.Min.Y);
			F.Max = FVector2D(B.Max.X, B.Max.Y);
			F.Color = A->Data.Color;
			F.Name = A->Data.Name;
			Out.Add(F);
		}
	}
}

// ---------------- PNG ----------------
bool FBlockoutTopDownLib::ExportPNG(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, const FBlockoutTopDownOptions& Opt)
{
	TArray<FFootprint> Items;
	GatherFootprints(Actors, Items);
	if (Items.Num() == 0) return false;

	FBox2D Total(EForceInit::ForceInit);
	for (const FFootprint& F : Items) { Total += F.Min; Total += F.Max; }
	const float Margin = 200.f;
	Total.Min -= FVector2D(Margin, Margin);
	Total.Max += FVector2D(Margin, Margin);

	const FVector2D Ext = Total.GetSize();
	const float MaxDim = FMath::Max(Ext.X, Ext.Y);
	const float Scale = (float)Opt.MaxResolution / FMath::Max(MaxDim, 1.f);
	const int32 W = FMath::Clamp((int32)(Ext.X * Scale), 64, 8192);
	const int32 H = FMath::Clamp((int32)(Ext.Y * Scale), 64, 8192);

	FCanvas2D C;
	C.Init(W, H, FColor::White);

	// 世界坐标 → 像素（Y 翻转）
	auto ToPx = [&](const FVector2D& P) -> FVector2D
	{
		return FVector2D((P.X - Total.Min.X) * Scale, (Total.Max.Y - P.Y) * Scale);
	};

	// 网格
	if (Opt.bWithGrid && Opt.GridStep > 1.f)
	{
		const FColor GridC(215, 215, 215);
		for (float X = FMath::CeilToFloat(Total.Min.X / Opt.GridStep) * Opt.GridStep; X <= Total.Max.X; X += Opt.GridStep)
		{
			const FVector2D A = ToPx(FVector2D(X, Total.Min.Y));
			const FVector2D B = ToPx(FVector2D(X, Total.Max.Y));
			C.Line((int32)A.X, (int32)A.Y, (int32)B.X, (int32)B.Y, GridC);
		}
		for (float Y = FMath::CeilToFloat(Total.Min.Y / Opt.GridStep) * Opt.GridStep; Y <= Total.Max.Y; Y += Opt.GridStep)
		{
			const FVector2D A = ToPx(FVector2D(Total.Min.X, Y));
			const FVector2D B = ToPx(FVector2D(Total.Max.X, Y));
			C.Line((int32)A.X, (int32)A.Y, (int32)B.X, (int32)B.Y, GridC);
		}
	}

	// 物体轮廓（填充 + 描边 + 标注）
	for (const FFootprint& F : Items)
	{
		const FVector2D P0 = ToPx(F.Min);
		const FVector2D P1 = ToPx(F.Max);
		const int32 X0 = (int32)FMath::Min(P0.X, P1.X), X1 = (int32)FMath::Max(P0.X, P1.X);
		const int32 Y0 = (int32)FMath::Min(P0.Y, P1.Y), Y1 = (int32)FMath::Max(P0.Y, P1.Y);
		const FColor Fill((uint8)(F.Color.R * 255.f), (uint8)(F.Color.G * 255.f), (uint8)(F.Color.B * 255.f));
		C.FillRect(X0, Y0, X1, Y1, Fill);
		C.StrokeRect(X0, Y0, X1, Y1, FColor(40, 40, 40));

		const int32 CX = (X0 + X1) / 2, CY = (Y0 + Y1) / 2;
		if (Opt.bWithDims)
		{
			const FString Dim = FString::Printf(TEXT("%.1fm x %.1fm"),
				(F.Max.X - F.Min.X) / 100.f, (F.Max.Y - F.Min.Y) / 100.f);
			C.Text5x7(Dim, CX - Dim.Len() * 3, CY - 8, FColor::Black, 2);
		}
		if (Opt.bWithNames && !F.Name.IsEmpty())
		{
			C.Text5x7(F.Name, CX - F.Name.Len() * 3, CY + 8, FColor::Black, 2);
		}
	}

	// PNG 编码（BGRA）
	IImageWrapperModule& WrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = WrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid()) return false;
	if (!Wrapper->SetRaw(C.Pixels.GetData(), C.Pixels.Num() * sizeof(FColor), W, H, ERGBFormat::BGRA, 8)) return false;
	const TArray64<uint8>& Compressed = Wrapper->GetCompressed(100);
	if (Compressed.Num() == 0) return false;
	TArray<uint8> Bytes;
	Bytes.Append(Compressed.GetData(), (int64)Compressed.Num());
	return FFileHelper::SaveArrayToFile(Bytes, *FilePath);
}

// ---------------- SVG ----------------
bool FBlockoutTopDownLib::ExportSVG(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, const FBlockoutTopDownOptions& Opt)
{
	TArray<FFootprint> Items;
	GatherFootprints(Actors, Items);
	if (Items.Num() == 0) return false;

	FBox2D Total(EForceInit::ForceInit);
	for (const FFootprint& F : Items) { Total += F.Min; Total += F.Max; }
	const float Margin = 200.f;
	Total.Min -= FVector2D(Margin, Margin);
	Total.Max += FVector2D(Margin, Margin);

	const FVector2D Ext = Total.GetSize();

	TStringBuilder<8192> Sb;
	Sb.Appendf(TEXT("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %.2f %.2f\" width=\"%.0f\" height=\"%.0f\">\n"),
		Ext.X, Ext.Y, Ext.X, Ext.Y);
	Sb.Append(TEXT("<rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" fill=\"white\"/>\n"));

	// 世界坐标 → SVG（Y 翻转）
	auto ToSv = [&](const FVector2D& P) -> FVector2D
	{
		return FVector2D(P.X - Total.Min.X, Total.Max.Y - P.Y);
	};

	if (Opt.bWithGrid && Opt.GridStep > 1.f)
	{
		for (float X = FMath::CeilToFloat(Total.Min.X / Opt.GridStep) * Opt.GridStep; X <= Total.Max.X; X += Opt.GridStep)
		{
			const FVector2D A = ToSv(FVector2D(X, Total.Min.Y));
			const FVector2D B = ToSv(FVector2D(X, Total.Max.Y));
			Sb.Appendf(TEXT("<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#D7D7D7\" stroke-width=\"1\"/>\n"), A.X, A.Y, B.X, B.Y);
		}
		for (float Y = FMath::CeilToFloat(Total.Min.Y / Opt.GridStep) * Opt.GridStep; Y <= Total.Max.Y; Y += Opt.GridStep)
		{
			const FVector2D A = ToSv(FVector2D(Total.Min.X, Y));
			const FVector2D B = ToSv(FVector2D(Total.Max.X, Y));
			Sb.Appendf(TEXT("<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"#D7D7D7\" stroke-width=\"1\"/>\n"), A.X, A.Y, B.X, B.Y);
		}
	}

	for (const FFootprint& F : Items)
	{
		const FVector2D P = ToSv(F.Min);
		const float Rw = F.Max.X - F.Min.X, Rh = F.Max.Y - F.Min.Y;
		Sb.Appendf(TEXT("<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" fill=\"rgba(%d,%d,%d,0.75)\" stroke=\"#282828\" stroke-width=\"2\"/>\n"),
			P.X, P.Y, Rw, Rh,
			(int32)(F.Color.R * 255.f), (int32)(F.Color.G * 255.f), (int32)(F.Color.B * 255.f));

		const float CX = P.X + Rw / 2.f, CY = P.Y + Rh / 2.f;
		if (Opt.bWithDims)
		{
			Sb.Appendf(TEXT("<text x=\"%.2f\" y=\"%.2f\" font-size=\"18\" text-anchor=\"middle\" font-family=\"monospace\" fill=\"black\">%.1fm x %.1fm</text>\n"),
				CX, CY - 4.f, Rw / 100.f, Rh / 100.f);
		}
		if (Opt.bWithNames && !F.Name.IsEmpty())
		{
			Sb.Appendf(TEXT("<text x=\"%.2f\" y=\"%.2f\" font-size=\"18\" text-anchor=\"middle\" font-family=\"sans-serif\" fill=\"black\">%s</text>\n"),
				CX, CY + 18.f, *F.Name);
		}
	}

	Sb.Append(TEXT("</svg>\n"));
	return FFileHelper::SaveStringToFile(Sb.ToString(), *FilePath);
}
