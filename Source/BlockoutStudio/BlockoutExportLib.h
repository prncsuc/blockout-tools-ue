// BlockoutStudio - 导出系统（OBJ/FBX/glTF/USD/USDZ/自定义格式）
#pragma once

#include "CoreMinimal.h"
#include "BlockoutTypes.h"

class ABlockoutActor;

class BLOCKOUTSTUDIO_API FBlockoutExportLib
{
public:
	// EEp: 0=世界 1=本地（仅单物体时有意义）
	// bGroupByMaterial: D3 按材质分组（同色合并为一个网格），否则按物体分组
	static bool ExportOBJ(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bWorldSpace, bool bGroupByMaterial = false);
	static bool ExportFBX(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bGroupByMaterial = false);
	static bool ExportGLTF(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bIncludeLighting, bool bIncludeCamera, bool bGroupByMaterial = false);
	static bool ExportUSD(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bIncludePhysics, bool bPBR);
	static bool ExportUSDZ(const TArray<ABlockoutActor*>& Actors, const FString& FilePath, bool bIncludePhysics, bool bPBR);
	static bool ExportCustom(const TArray<ABlockoutActor*>& Actors, const FString& FilePath);
};
