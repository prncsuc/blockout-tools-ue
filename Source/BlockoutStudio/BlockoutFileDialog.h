// BlockoutStudio - Win32 原生文件对话框（Shipping 可用，替代 DesktopPlatform）
#pragma once

#include "CoreMinimal.h"

class FBlockoutFileDialog
{
public:
	// Filter 格式："OBJ 文件|*.obj|FBX 文件|*.fbx"（与 DesktopPlatform 一致）
	static bool OpenFileDialog(const FString& Title, const FString& DefaultPath, const FString& Filter, TArray<FString>& OutFiles);
	static bool SaveFileDialog(const FString& Title, const FString& DefaultPath, const FString& Filter, TArray<FString>& OutFiles);
};
