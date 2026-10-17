// BlockoutStudio - D4: 导入 OBJ / glTF / GLB 作为参考体网格
#pragma once

#include "CoreMinimal.h"

class BLOCKOUTSTUDIO_API FBlockoutImportLib
{
public:
	// 导入网格文件，输出顶点/三角形（uu 单位，UE Z-up）。
	// 约定：OBJ 原样按 uu 处理；glTF/GLB 为米制，自动 ×100 转 uu。
	// 成功返回 true；失败时 OutError 给出原因。
	static bool ImportMesh(const FString& FilePath, TArray<FVector>& OutVerts, TArray<int32>& OutTris, FString& OutName, FString& OutError);
};
