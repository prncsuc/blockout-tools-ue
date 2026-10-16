// BlockoutStudio - 程序化纹理生成（棋盘格/网格/砖墙/混凝土/瓷砖/金属板）
#pragma once

#include "CoreMinimal.h"
#include "BlockoutTypes.h"

class UTexture2D;

class FBlockoutTextureLib
{
public:
	// 按图案类型获取/生成 512x512 灰度纹理（缓存复用）
	static UTexture2D* GetPatternTexture(EBlockoutTexturePattern Pattern);
};
