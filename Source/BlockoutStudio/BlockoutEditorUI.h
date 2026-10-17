// BlockoutStudio - Slate 工具 UI：工具栏、属性面板、右键菜单、快捷键提示（i18n 预留）
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "BlockoutTypes.h"

class ABlockoutEditorPlayerController;
class ABlockoutActor;

// i18n 预留：当前仅中文，后续可切换语言表
namespace BlockoutUI
{
	BLOCKOUTSTUDIO_API FText T(const TCHAR* Key, const FString& ZhDefault);
}

class BLOCKOUTSTUDIO_API SBlockoutEditorUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBlockoutEditorUI) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ABlockoutEditorPlayerController>, PC)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<ABlockoutEditorPlayerController> PCPtr;

	// ---------- 工具栏 ----------
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildShapeMenu();
	TSharedRef<SWidget> BuildExportMenu();
	void OnShapePicked(EBlockoutShape Shape);
	void OnExportPicked(int32 Format);

	// ---------- 属性面板 ----------
	TSharedRef<SWidget> BuildPropertyPanel();
	TSharedRef<SWidget> MakeNumBox(TFunction<float()> Getter, TFunction<void(float)> Setter, float MinV, float MaxV, const FString& Label);
	void RefreshPropertyPanel();

	// ---------- 快捷键提示 ----------
	TSharedRef<SWidget> BuildHotkeyBar();

	// ---------- 右键菜单 ----------
	virtual FReply OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev) override;
	void OpenContextMenu(const FVector2D& ScreenPos);

	// ---------- 提示 ----------
	void Toast(const FString& Msg);

	// 选中缓存（用于检测变化重建面板）
	UPROPERTY() TArray<ABlockoutActor*> CachedSelection;
	TSharedPtr<class SVerticalBox> PropertyBox;

	// 形状选项
	TArray<TSharedPtr<FString>> ShapeNames;
	TSharedPtr<FString> CurrentShapeName;
};
