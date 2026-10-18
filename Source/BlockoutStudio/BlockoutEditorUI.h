// BlockoutStudio - Slate 工具 UI：工具栏、属性面板、右键菜单、快捷键提示（i18n 预留）
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Input/Events.h"
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
	TSharedRef<SWidget> BuildCreatePanel();   // 左侧创建面板（UE 编辑器布局）：预制件 + 笔刷
	TSharedRef<SWidget> BuildShapeMenu();
	TSharedRef<SWidget> BuildExportMenu();
	TSharedRef<SWidget> BuildToolsMenu();   // P4：测距/参考体/导入/快照/俯视出图
	void OnShapePicked(EBlockoutShape Shape);
	void OnExportPicked(int32 Format);

	// ---------- P4 工具动作 ----------
	void OnExportTopDown(int32 Type);        // 0=PNG 1=SVG
	void OnImportReference();                // D4 导入 OBJ/glTF 参考体
	void OnSpawnScaleRef(int32 Type);        // T3: 0=1m 1=2m 2=1.8m 人形
	void OnSnapshotSave();                   // D1 保存快照
	void OnSnapshotRestore(int32 Index);     // D1 回滚第 N 个快照
	bool bGroupByMaterial = false;           // D3 按材质/物体分组导出

	// ---------- 属性面板 ----------
	TSharedRef<SWidget> BuildPropertyPanel();
	TSharedRef<SWidget> MakeNumBox(TFunction<float()> Getter, TFunction<void(float)> Setter, float MinV, float MaxV, const FString& Label);
	void RefreshPropertyPanel();

	// ---------- 快捷键提示 ----------
	TSharedRef<SWidget> BuildHotkeyBar();

	// ---------- 右键菜单 ----------
	virtual FReply OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev) override;
	void OpenContextMenu(const FVector2D& ScreenPos);

	// ---------- E10 场景树 ----------
	TSharedRef<SWidget> BuildSceneTreePanel();
	void RefreshSceneTree();
	void OnSceneTreeSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	void SyncTreeSelectionFromSub();                     // 视口选中 → 树选中同步
	void OnSceneTreeRenameCommitted(const FText& NewText, ETextCommit::Type CommitType, TWeakObjectPtr<ABlockoutActor> Actor);
	FReply OnSceneTreeDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TSharedPtr<FString> Item);
	FReply OnSceneTreeAcceptDrop(const FDragDropEvent& Event, EItemDropZone DropZone, TSharedPtr<FString> TargetItem);
	TOptional<EItemDropZone> OnSceneTreeCanAcceptDrop(const FDragDropEvent& Event, EItemDropZone DropZone, TSharedPtr<FString> TargetItem);
	TSharedPtr<class SWidget> BuildSceneTreeContextMenu();
	void SetSelectedHidden(bool bHidden);                // 右键菜单批量
	void SetSelectedLocked(bool bLocked);
	FString SceneSearchText;                             // 搜索过滤关键字
	bool bSyncingTreeSelection = false;
	float TreeRefreshTimer = 0.f;
	TSharedPtr<class SListView<TSharedPtr<FString>>> SceneTreeView;
	TArray<TSharedPtr<FString>> SceneTreeItems;      // 显示名列表
	TArray<TWeakObjectPtr<ABlockoutActor>> SceneTreeActors; // 与显示名一一对应
	TArray<TSharedPtr<class SInlineEditableTextBlock>> SceneTreeEditors; // 与显示名一一对应（重命名用）
	int32 LastBlockCount = -1;

	// ---------- 提示 ----------
	void Toast(const FString& Msg);

	// 选中缓存（用于检测变化重建面板）
	UPROPERTY() TArray<ABlockoutActor*> CachedSelection;
	TSharedPtr<class SVerticalBox> PropertyBox;

	// 形状选项
	TArray<TSharedPtr<FString>> ShapeNames;
	TSharedPtr<FString> CurrentShapeName;
};
