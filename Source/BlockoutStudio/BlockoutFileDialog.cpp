// BlockoutStudio - Win32 原生文件对话框实现（GetOpenFileNameW / GetSaveFileNameW）
#include "BlockoutFileDialog.h"

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"
#include <commdlg.h>
#include "Windows/HideWindowsPlatformTypes.h"

static FString FilterToWin(const FString& Filter)
{
	// "OBJ|*.obj|All|*.*" -> "OBJ\0*.obj\0All\0*.*\0\0"
	FString S = Filter;
	S.ReplaceCharInline(TEXT('|'), TEXT('\0'));
	S.AppendChar(TEXT('\0'));
	S.AppendChar(TEXT('\0'));
	return S;
}

bool FBlockoutFileDialog::OpenFileDialog(const FString& Title, const FString& DefaultPath, const FString& Filter, TArray<FString>& OutFiles)
{
	TCHAR Buf[4096] = { 0 };
	FString WinFilter = FilterToWin(Filter);
	OPENFILENAME Ofn;
	FMemory::Memzero(Ofn);
	Ofn.lStructSize = sizeof(Ofn);
	Ofn.lpstrTitle = *Title;
	Ofn.lpstrFile = Buf;
	Ofn.nMaxFile = UE_ARRAY_COUNT(Buf);
	Ofn.lpstrFilter = *WinFilter;
	Ofn.lpstrInitialDir = *DefaultPath;
	Ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;
	if (!GetOpenFileName(&Ofn)) return false;
	// 解析多选（目录\0文件1\0文件2\0\0 或单文件完整路径）
	const TCHAR* P = Buf;
	FString First(P);
	P += First.Len() + 1;
	if (*P == 0)
	{
		OutFiles.Add(First);
		return true;
	}
	const FString Dir = First;
	while (*P != 0)
	{
		FString Name(P);
		OutFiles.Add(Dir / Name);
		P += Name.Len() + 1;
	}
	return OutFiles.Num() > 0;
}

bool FBlockoutFileDialog::SaveFileDialog(const FString& Title, const FString& DefaultPath, const FString& Filter, TArray<FString>& OutFiles)
{
	TCHAR Buf[4096] = { 0 };
	FString WinFilter = FilterToWin(Filter);
	OPENFILENAME Ofn;
	FMemory::Memzero(Ofn);
	Ofn.lStructSize = sizeof(Ofn);
	Ofn.lpstrTitle = *Title;
	Ofn.lpstrFile = Buf;
	Ofn.nMaxFile = UE_ARRAY_COUNT(Buf);
	Ofn.lpstrFilter = *WinFilter;
	Ofn.lpstrInitialDir = *DefaultPath;
	Ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
	if (!GetSaveFileName(&Ofn)) return false;
	OutFiles.Add(FString(Buf));
	return true;
}

#else

bool FBlockoutFileDialog::OpenFileDialog(const FString&, const FString&, const FString&, TArray<FString>&) { return false; }
bool FBlockoutFileDialog::SaveFileDialog(const FString&, const FString&, const FString&, TArray<FString>&) { return false; }

#endif
