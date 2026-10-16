// BlockoutStudio - 程序化纹理生成实现
#include "BlockoutTextureLib.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

namespace
{
	TMap<EBlockoutTexturePattern, UTexture2D*> Cache;

	void DrawRect(TArray<uint8>& Pixels, int32 Size, int32 X0, int32 Y0, int32 X1, int32 Y1, uint8 V)
	{
		for (int32 y = FMath::Max(0, Y0); y < FMath::Min(Size, Y1); y++)
		{
			for (int32 x = FMath::Max(0, X0); x < FMath::Min(Size, X1); x++)
			{
				Pixels[y * Size + x] = V;
			}
		}
	}

	UTexture2D* CreateGrayTexture(const TArray<uint8>& Pixels, int32 Size)
	{
		UTexture2D* Tex = UTexture2D::CreateTransient(Size, Size, PF_G8);
		if (!Tex) return nullptr;
		void* Data = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num());
		Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
		Tex->SRGB = true;
		Tex->Filter = TF_Bilinear;
		Tex->AddressX = TA_Wrap;
		Tex->AddressY = TA_Wrap;
		Tex->UpdateResource();
		return Tex;
	}
}

UTexture2D* FBlockoutTextureLib::GetPatternTexture(EBlockoutTexturePattern Pattern)
{
	if (UTexture2D** Found = Cache.Find(Pattern)) return *Found;

	const int32 S = 512;
	TArray<uint8> P;
	P.Init(200, S * S);

	switch (Pattern)
	{
	case EBlockoutTexturePattern::Checker:
	{
		for (int32 y = 0; y < S; y++)
			for (int32 x = 0; x < S; x++)
				P[y * S + x] = (((x / 64) + (y / 64)) % 2 == 0) ? 210 : 150;
		break;
	}
	case EBlockoutTexturePattern::Grid:
	{
		// ProBuilder 风格：浅底 + 细网格线 + 每 4 格粗线
		DrawRect(P, S, 0, 0, S, S, 215);
		for (int32 i = 0; i <= S; i += 32)
		{
			bool bMajor = (i % 128 == 0);
			uint8 V = bMajor ? 120 : 175;
			int32 T = bMajor ? 3 : 1;
			DrawRect(P, S, i, 0, i + T, S, V);
			DrawRect(P, S, 0, i, S, i + T, V);
		}
		break;
	}
	case EBlockoutTexturePattern::Brick:
	{
		DrawRect(P, S, 0, 0, S, S, 170);
		const int32 BH = 64, BW = 128;
		for (int32 y = 0; y < S; y += BH)
		{
			int32 Offset = ((y / BH) % 2) * (BW / 2);
			for (int32 x = -BW; x < S; x += BW)
			{
				DrawRect(P, S, x + Offset + 3, y + 3, x + Offset + BW - 3, y + BH - 3, 195);
			}
		}
		break;
	}
	case EBlockoutTexturePattern::Concrete:
	{
		uint32 Seed = 12345;
		for (int32 i = 0; i < S * S; i++)
		{
			Seed = Seed * 1664525u + 1013904223u;
			uint8 Noise = (uint8)((Seed >> 24) % 48);
			P[i] = 155 + Noise;
		}
		DrawRect(P, S, 0, 0, S, 2, 140);
		break;
	}
	case EBlockoutTexturePattern::Tile:
	{
		DrawRect(P, S, 0, 0, S, S, 120);
		for (int32 y = 4; y < S; y += 128)
			for (int32 x = 4; x < S; x += 128)
				DrawRect(P, S, x, y, x + 120, y + 120, 225);
		break;
	}
	case EBlockoutTexturePattern::MetalPlate:
	{
		DrawRect(P, S, 0, 0, S, S, 165);
		for (int32 y = 8; y < S; y += 128)
		{
			for (int32 x = 8; x < S; x += 128)
			{
				DrawRect(P, S, x, y, x + 120, y + 120, 180);
				// 四角铆钉
				DrawRect(P, S, x + 4, y + 4, x + 12, y + 12, 90);
				DrawRect(P, S, x + 108, y + 4, x + 116, y + 12, 90);
				DrawRect(P, S, x + 4, y + 108, x + 12, y + 116, 90);
				DrawRect(P, S, x + 108, y + 108, x + 116, y + 116, 90);
			}
		}
		break;
	}
	default:
		break;
	}

	UTexture2D* Tex = CreateGrayTexture(P, S);
	Cache.Add(Pattern, Tex);
	return Tex;
}
