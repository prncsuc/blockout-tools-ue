// BlockoutStudio - Slate 工具 UI 实现
#include "BlockoutEditorUI.h"
#include "BlockoutEditorPlayerController.h"
#include "BlockoutEditorSubsystem.h"
#include "BlockoutActor.h"
#include "BlockoutMeasureLib.h"
#include "BlockoutExportLib.h"
#include "BlockoutTopDownLib.h"
#include "BlockoutImportLib.h"
#include "BlockoutHUD.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IMenu.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// ---------- i18n 预留 ----------
namespace BlockoutUI
{
	static TMap<FString, FString> GChineseTable;
	FText T(const TCHAR* Key, const FString& ZhDefault)
	{
		const FString* Found = GChineseTable.Find(Key);
		return FText::FromString(Found ? *Found : ZhDefault);
	}
}

static UBlockoutEditorSubsystem* GetSub(TWeakObjectPtr<ABlockoutEditorPlayerController> PC)
{
	if (ABlockoutEditorPlayerController* P = PC.Get())
	{
		if (UGameInstance* GI = P->GetWorld()->GetGameInstance())
		{
			return GI->GetSubsystem<UBlockoutEditorSubsystem>();
		}
	}
	return nullptr;
}

static ABlockoutEditorPlayerController* GetPC(TWeakObjectPtr<ABlockoutEditorPlayerController> PC)
{
	return PC.Get();
}

void SBlockoutEditorUI::Construct(const FArguments& InArgs)
{
	PCPtr = InArgs._PC;

	// 形状选项
	const EBlockoutShape AllShapes[] = {
		EBlockoutShape::Cube, EBlockoutShape::Wall, EBlockoutShape::Staircase, EBlockoutShape::Ramp,
		EBlockoutShape::Wedge, EBlockoutShape::Arch, EBlockoutShape::DoorWall, EBlockoutShape::WindowWall,
		EBlockoutShape::Cylinder, EBlockoutShape::Sphere, EBlockoutShape::Capsule, EBlockoutShape::Pipe,
		EBlockoutShape::Colonnade, EBlockoutShape::Fence, EBlockoutShape::Stairwell, EBlockoutShape::Floor, EBlockoutShape::DoorPiece,
		EBlockoutShape::WindowPiece
	};
	const TCHAR* ShapeLabels[] = {
		TEXT("立方体"), TEXT("墙体"), TEXT("楼梯"), TEXT("斜坡"), TEXT("楔形"), TEXT("拱门"),
		TEXT("门洞墙"), TEXT("窗洞墙"), TEXT("圆柱"), TEXT("球体"), TEXT("胶囊体"), TEXT("管道"),
		TEXT("柱廊"), TEXT("围墙"), TEXT("楼梯间"), TEXT("地板"), TEXT("门构件"), TEXT("窗构件")
	};
	for (int32 i = 0; i < 18; ++i)
	{
		ShapeNames.Add(MakeShared<FString>(ShapeLabels[i]));
	}
	CurrentShapeName = ShapeNames[0];

	ChildSlot
	[
		SNew(SOverlay)

		// 顶部工具栏
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f))
			[
				BuildToolbar()
			]
		]

		// 左侧属性面板
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(8, 60, 0, 50))
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.03f, 0.03f, 0.03f, 0.85f))
			.Padding(FMargin(8))
			[
				SNew(SBox)
				.WidthOverride(300)
				.MaxDesiredHeight(600)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						BuildPropertyPanel()
					]
				]
			]
		]

		// 底部快捷键提示
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0, 0, 0, 36))
		[
			BuildHotkeyBar()
		]
	];
}

// ================= 工具栏 =================
TSharedRef<SWidget> SBlockoutEditorUI::BuildToolbar()
{
	return SNew(SHorizontalBox)

	// 创建体块下拉
	+ SHorizontalBox::Slot().AutoWidth().Padding(6, 4)
	[
		SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&ShapeNames)
		.InitiallySelectedItem(CurrentShapeName)
		.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
		{
			return SNew(STextBlock).Text(FText::FromString(*Item));
		})
		.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
		{
			CurrentShapeName = Item;
			const int32 Idx = ShapeNames.IndexOfByKey(Item);
			if (Idx != INDEX_NONE)
			{
				OnShapePicked((EBlockoutShape)Idx);
			}
		})
		[
			SNew(STextBlock).Text_Lambda([this]()
			{
				return FText::FromString(CurrentShapeName.IsValid() ? *CurrentShapeName : TEXT("创建体块"));
			})
		]
	]

	// 变换模式
	+ SHorizontalBox::Slot().AutoWidth().Padding(4, 4)
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("Move"), TEXT("移动(W)")))
		.ToolTipText(BlockoutUI::T(TEXT("MoveTip"), TEXT("切换到移动模式")))
		.OnClicked_Lambda([this]()
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->SetTransformMode(EBlockoutTransformMode::Translate);
			return FReply::Handled();
		})
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 4)
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("Rotate"), TEXT("旋转(E)")))
		.ToolTipText(BlockoutUI::T(TEXT("RotateTip"), TEXT("切换到旋转模式")))
		.OnClicked_Lambda([this]()
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->SetTransformMode(EBlockoutTransformMode::Rotate);
			return FReply::Handled();
		})
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 4)
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("Scale"), TEXT("缩放(R)")))
		.ToolTipText(BlockoutUI::T(TEXT("ScaleTip"), TEXT("切换到缩放模式")))
		.OnClicked_Lambda([this]()
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->SetTransformMode(EBlockoutTransformMode::Scale);
			return FReply::Handled();
		})
	]

	// 吸附开关
	+ SHorizontalBox::Slot().AutoWidth().Padding(10, 4)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]()
		{
			UBlockoutEditorSubsystem* S = GetSub(PCPtr);
			return (S && S->GetGridSnap()) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this](ECheckBoxState St)
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->SetGridSnap(St == ECheckBoxState::Checked);
		})
		[
			SNew(STextBlock).Text(BlockoutUI::T(TEXT("GridSnap"), TEXT("网格吸附(G)")))
		]
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(4, 4)
	[
		SNew(SCheckBox)
		.IsChecked_Lambda([this]()
		{
			UBlockoutEditorSubsystem* S = GetSub(PCPtr);
			return (S && S->GetSurfaceSnap()) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this](ECheckBoxState St)
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->SetSurfaceSnap(St == ECheckBoxState::Checked);
		})
		[
			SNew(STextBlock).Text(BlockoutUI::T(TEXT("SurfaceSnap"), TEXT("表面吸附(S)")))
		]
	]

	// 撤销/重做
	+ SHorizontalBox::Slot().AutoWidth().Padding(10, 4)
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("Undo"), TEXT("撤销(Ctrl+Z)")))
		.OnClicked_Lambda([this]()
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->Undo();
			return FReply::Handled();
		})
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 4)
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("Redo"), TEXT("重做(Ctrl+Y)")))
		.OnClicked_Lambda([this]()
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->Redo();
			return FReply::Handled();
		})
	]

	// 文件/导出
	+ SHorizontalBox::Slot().AutoWidth().Padding(10, 4)
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("Save"), TEXT("保存(P)")))
		.OnClicked_Lambda([this]()
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->SaveProject(S->GetDefaultProjectPath());
			Toast(TEXT("项目已保存"));
			return FReply::Handled();
		})
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 4)
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("Export"), TEXT("导出")))
		.OnClicked_Lambda([this]()
		{
			FVector2D Pos = FSlateApplication::Get().GetCursorPos();
			FSlateApplication::Get().PushMenu(
				TSharedPtr<IMenu>(),
				BuildExportMenu(),
				Pos,
				FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup));
			return FReply::Handled();
		})
	]

	// P4 工具菜单（测距/参考体/导入/快照/俯视出图）
	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 4)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("工具")))
		.OnClicked_Lambda([this]()
		{
			FVector2D Pos = FSlateApplication::Get().GetCursorPos();
			FSlateApplication::Get().PushMenu(
				TSharedPtr<IMenu>(),
				BuildToolsMenu(),
				Pos,
				FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup));
			return FReply::Handled();
		})
	]
	;
}

TSharedRef<SWidget> SBlockoutEditorUI::BuildShapeMenu()
{
	return SNew(STextBlock).Text(FText::FromString(TEXT("shape")));
}

TSharedRef<SWidget> SBlockoutEditorUI::BuildExportMenu()
{
	const TCHAR* Labels[] = { TEXT("导出 OBJ"), TEXT("导出 FBX"), TEXT("导出 glTF"), TEXT("导出 USD"), TEXT("导出 USDZ"), TEXT("导出 自定义") };
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	// D3：分组选项（按材质合并 vs 按物体）
	Box->AddSlot().AutoHeight()
	[
		SNew(SCheckBox)
		.IsChecked(bGroupByMaterial ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
		.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bGroupByMaterial = (S == ECheckBoxState::Checked); })
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("按材质分组（否则按物体）")))
		]
	];

	for (int32 i = 0; i < 6; ++i)
	{
		const int32 Idx = i;
		Box->AddSlot().AutoHeight()
		[
			SNew(SButton)
			.Text(FText::FromString(Labels[i]))
			.OnClicked_Lambda([this, Idx]()
			{
				OnExportPicked(Idx);
				return FReply::Handled();
			})
		];
	}
	return Box;
}

// ================= P4 工具菜单 =================
TSharedRef<SWidget> SBlockoutEditorUI::BuildToolsMenu()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	auto AddBtn = [&Box](const FString& Label, TFunction<void()> Fn)
	{
		Box->AddSlot().AutoHeight()
		[
			SNew(SButton)
			.Text(FText::FromString(Label))
			.OnClicked_Lambda([Fn]()
			{
				Fn();
				return FReply::Handled();
			})
		];
	};

	AddBtn(TEXT("测距模式(M)"), [this]() { if (ABlockoutEditorPlayerController* P = GetPC(PCPtr)) P->OnKeyToggleMeasure(); });
	AddBtn(TEXT("测试模式(F5)"), [this]() { if (ABlockoutEditorPlayerController* P = GetPC(PCPtr)) P->ToggleTestMode(); });
	Box->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("--- T3 比例尺参考体 ---")))
	];
	AddBtn(TEXT("放置 1m 立方体"), [this]() { OnSpawnScaleRef(0); });
	AddBtn(TEXT("放置 2m 立方体"), [this]() { OnSpawnScaleRef(1); });
	AddBtn(TEXT("放置 1.8m 人形"), [this]() { OnSpawnScaleRef(2); });
	Box->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("--- D4/T2/D1 ---")))
	];
	AddBtn(TEXT("导入参考体(OBJ/glTF)"), [this]() { OnImportReference(); });
	AddBtn(TEXT("导出俯视图 PNG"), [this]() { OnExportTopDown(0); });
	AddBtn(TEXT("导出俯视图 SVG"), [this]() { OnExportTopDown(1); });
	AddBtn(TEXT("保存快照"), [this]() { OnSnapshotSave(); });
	AddBtn(TEXT("回滚到最近快照"), [this]() { OnSnapshotRestore(0); });
	Box->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("--- 点线面编辑（ProBuilder 对齐） ---")))
	];
	AddBtn(TEXT("内插面(T)"), [this]() { if (ABlockoutEditorPlayerController* P = GetPC(PCPtr)) P->MeshInsetFaces(); });
	AddBtn(TEXT("细分面(K)"), [this]() { if (ABlockoutEditorPlayerController* P = GetPC(PCPtr)) P->MeshSubdivideFaces(); });
	AddBtn(TEXT("翻转法线(J)"), [this]() { if (ABlockoutEditorPlayerController* P = GetPC(PCPtr)) P->MeshFlipFaces(); });
	AddBtn(TEXT("补洞(H)"), [this]() { if (ABlockoutEditorPlayerController* P = GetPC(PCPtr)) P->MeshFillHoles(); });
	AddBtn(TEXT("焊接顶点(N)"), [this]() { if (ABlockoutEditorPlayerController* P = GetPC(PCPtr)) P->MeshWeldVerts(); });
	return Box;
}

// T3：放置比例尺参考体（相机前方）
void SBlockoutEditorUI::OnSpawnScaleRef(int32 Type)
{
	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	ABlockoutEditorPlayerController* P = GetPC(PCPtr);
	if (!Sub || !P) return;
	FVector Origin, Dir;
	FVector Loc = FVector::ZeroVector;
	if (P->GetMouseRay(Origin, Dir))
	{
		Loc = Origin + Dir * 600.f;
	}
	Loc.Z = 0; // 落地
	if (Sub->SpawnScaleReference(Type, Loc))
	{
		Toast(TEXT("已放置比例尺参考体"));
	}
	else
	{
		Toast(TEXT("放置失败"));
	}
}

// D4：导入 OBJ/glTF 参考体
void SBlockoutEditorUI::OnImportReference()
{
	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	ABlockoutEditorPlayerController* P = GetPC(PCPtr);
	if (!Sub || !P) return;

	IDesktopPlatform* DP = FDesktopPlatformModule::Get();
	if (!DP) { Toast(TEXT("无法打开文件对话框")); return; }
	TArray<FString> OutFiles;
	const void* Parent = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	if (!DP->OpenFileDialog(Parent, TEXT("导入参考体"), TEXT(""), TEXT(""), TEXT("网格文件|*.obj;*.gltf;*.glb"), EFileDialogFlags::None, OutFiles) || OutFiles.Num() == 0)
	{
		return;
	}
	TArray<FVector> V;
	TArray<int32> T;
	FString Name, Err;
	if (!FBlockoutImportLib::ImportMesh(OutFiles[0], V, T, Name, Err))
	{
		Toast(FString::Printf(TEXT("导入失败：%s"), *Err));
		return;
	}
	FVector Origin, Dir;
	FVector Loc = FVector::ZeroVector;
	if (P->GetMouseRay(Origin, Dir)) Loc = Origin + Dir * 600.f;
	Loc.Z = 0;
	if (Sub->SpawnReferenceMesh(V, T, Name, Loc))
	{
		Toast(FString::Printf(TEXT("已导入参考体：%s（%d 顶点）"), *Name, V.Num()));
	}
}

// T2：俯视 2D 出图
void SBlockoutEditorUI::OnExportTopDown(int32 Type)
{
	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	if (!Sub) return;
	TArray<ABlockoutActor*> Targets = Sub->GetAllBlocks();
	if (Targets.Num() == 0) { Toast(TEXT("场景为空")); return; }

	IDesktopPlatform* DP = FDesktopPlatformModule::Get();
	if (!DP) { Toast(TEXT("无法打开文件对话框")); return; }
	TArray<FString> OutFiles;
	const void* Parent = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	const TCHAR* Filter = Type == 0 ? TEXT("PNG|*.png") : TEXT("SVG|*.svg");
	if (!DP->SaveFileDialog(Parent, TEXT("导出俯视图"), TEXT(""), TEXT(""), Filter, EFileDialogFlags::None, OutFiles) || OutFiles.Num() == 0)
	{
		return;
	}
	FBlockoutTopDownOptions Opt;
	const bool bOk = (Type == 0)
		? FBlockoutTopDownLib::ExportPNG(Targets, OutFiles[0], Opt)
		: FBlockoutTopDownLib::ExportSVG(Targets, OutFiles[0], Opt);
	Toast(bOk ? FString::Printf(TEXT("俯视图已导出：%s"), *OutFiles[0]) : TEXT("俯视图导出失败"));
}

// D1：快照
void SBlockoutEditorUI::OnSnapshotSave()
{
	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	if (!Sub) return;
	if (Sub->AutoSave(true))
	{
		Toast(TEXT("快照已保存（保留最近 20 个）"));
	}
	else
	{
		Toast(TEXT("快照保存失败"));
	}
}

void SBlockoutEditorUI::OnSnapshotRestore(int32 Index)
{
	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	if (!Sub) return;
	TArray<FString> Files = Sub->GetSnapshotFiles();
	if (Files.Num() == 0) { Toast(TEXT("暂无快照")); return; }
	if (!Files.IsValidIndex(Index)) { Toast(TEXT("快照索引无效")); return; }
	if (Sub->RestoreSnapshot(Files[Index]))
	{
		Toast(FString::Printf(TEXT("已回滚：%s"), *FPaths::GetCleanFilename(Files[Index])));
	}
	else
	{
		Toast(TEXT("回滚失败"));
	}
}

void SBlockoutEditorUI::OnShapePicked(EBlockoutShape Shape)
{
	if (ABlockoutEditorPlayerController* P = GetPC(PCPtr))
	{
		P->CreateBlockAtCursor(Shape);
	}
}

void SBlockoutEditorUI::OnExportPicked(int32 Format)
{
	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	ABlockoutEditorPlayerController* P = GetPC(PCPtr);
	if (!Sub || !P) return;

	TArray<ABlockoutActor*> Targets = Sub->GetSelection();
	if (Targets.Num() == 0) Targets = Sub->GetAllBlocks();
	if (Targets.Num() == 0) { Toast(TEXT("没有可导出的体块")); return; }

	// 文件保存对话框
	IDesktopPlatform* DP = FDesktopPlatformModule::Get();
	if (!DP) { Toast(TEXT("无法打开文件对话框")); return; }

	TArray<FString> OutFiles;
	const void* Parent = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	const TCHAR* Ext[] = { TEXT("obj"), TEXT("fbx"), TEXT("gltf"), TEXT("usd"), TEXT("usdz"), TEXT("bobj") };
	const TCHAR* Filter[] = {
		TEXT("OBJ|*.obj"), TEXT("FBX|*.fbx"), TEXT("glTF|*.gltf"),
		TEXT("USD|*.usd"), TEXT("USDZ|*.usdz"), TEXT("Blockout|*.bobj")
	};

	const bool bOk = DP->SaveFileDialog(Parent, TEXT("导出白盒模型"), TEXT(""), TEXT(""), Filter[Format], EFileDialogFlags::None, OutFiles);
	if (!bOk || OutFiles.Num() == 0) return;

	const FString Path = OutFiles[0];
	const bool bGroup = bGroupByMaterial;
	bool bDone = false;
	switch (Format)
	{
	case 0: bDone = FBlockoutExportLib::ExportOBJ(Targets, Path, true, bGroup); break;
	case 1: bDone = FBlockoutExportLib::ExportFBX(Targets, Path, bGroup); break;
	case 2: bDone = FBlockoutExportLib::ExportGLTF(Targets, Path, true, true, bGroup); break;
	case 3: bDone = FBlockoutExportLib::ExportUSD(Targets, Path, false, true); break;
	case 4: bDone = FBlockoutExportLib::ExportUSDZ(Targets, Path, false, true); break;
	case 5: bDone = FBlockoutExportLib::ExportCustom(Targets, Path); break;
	}
	Toast(bDone ? FString::Printf(TEXT("导出成功：%s"), *Path) : TEXT("导出失败"));
}

// ================= 属性面板 =================
TSharedRef<SWidget> SBlockoutEditorUI::MakeNumBox(TFunction<float()> Getter, TFunction<void(float)> Setter, float MinV, float MaxV, const FString& Label)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 2)
		[
			SNew(SBox).WidthOverride(64)
			[
				SNew(STextBlock).Text(FText::FromString(Label))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 2)
		[
			SNew(SSpinBox<float>)
			.MinValue(MinV)
			.MaxValue(MaxV)
			.MinSliderValue(MinV)
			.MaxSliderValue(MaxV)
			.Value_Lambda([Getter]() { return Getter(); })
			.OnValueChanged_Lambda([Setter](float V) { Setter(V); })
		];
}

TSharedRef<SWidget> SBlockoutEditorUI::BuildPropertyPanel()
{
	PropertyBox = SNew(SVerticalBox);
	return SAssignNew(PropertyBox, SVerticalBox);
}

void SBlockoutEditorUI::RefreshPropertyPanel()
{
	if (!PropertyBox.IsValid()) return;
	PropertyBox->ClearChildren();

	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	if (!Sub) return;

	const TArray<ABlockoutActor*>& Sel = Sub->GetSelection();

	PropertyBox->AddSlot().AutoHeight().Padding(2, 4)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Sel.Num() == 0
			? TEXT("未选中物体")
			: FString::Printf(TEXT("选中：%s%s"), *Sel[0]->Data.Name, Sel.Num() > 1 ? TEXT(" 等") : TEXT(""))))
	];

	if (Sel.Num() == 0) return;
	ABlockoutActor* A = Sel[0];
	if (!IsValid(A)) return;

	// 名称
	PropertyBox->AddSlot().AutoHeight().Padding(2, 2)
	[
		SNew(SEditableTextBox)
		.Text(FText::FromString(A->Data.Name))
		.OnTextCommitted_Lambda([this, A](const FText& NewText, ETextCommit::Type)
		{
			if (UBlockoutEditorSubsystem* S = GetSub(PCPtr))
			{
				S->RenameActor(A, NewText.ToString());
			}
		})
	];

	// 变换
	PropertyBox->AddSlot().AutoHeight().Padding(2, 6)
	[
		SNew(STextBlock).Text(BlockoutUI::T(TEXT("Transform"), TEXT("变换")))
	];
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const int32 A_ = Axis;
		PropertyBox->AddSlot().AutoHeight()
		[
			MakeNumBox(
				[A, A_]() { return (float)(A->GetActorLocation().operator[](A_)); },
				[this, A, A_](float V)
				{
					FVector L = A->GetActorLocation();
					L.operator[](A_) = V;
					A->SetActorLocation(L);
				},
				-1000000.f, 1000000.f,
				FString::Printf(TEXT("位置 %c"), A_ == 0 ? 'X' : (A_ == 1 ? 'Y' : 'Z')))
		];
	}

	// 外观
	PropertyBox->AddSlot().AutoHeight().Padding(2, 6)
	[
		SNew(STextBlock).Text(BlockoutUI::T(TEXT("Appearance"), TEXT("外观")))
	];

	// A1 用途分色预设（UE 白盒惯例灰阶 + 强调色），一键切换
	{
		struct FPresetDef { const TCHAR* Label; FLinearColor Color; EBlockoutTexturePattern Pattern; };
		const FPresetDef Presets[] = {
			{ TEXT("地面"), FLinearColor(0.35f, 0.35f, 0.35f, 1.f), EBlockoutTexturePattern::Grid },
			{ TEXT("墙体"), FLinearColor(0.60f, 0.60f, 0.60f, 1.f), EBlockoutTexturePattern::Grid },
			{ TEXT("障碍"), FLinearColor(0.75f, 0.75f, 0.75f, 1.f), EBlockoutTexturePattern::Checker },
			{ TEXT("可交互"), FLinearColor(0.20f, 0.50f, 1.00f, 1.f), EBlockoutTexturePattern::Grid },
			{ TEXT("装饰"), FLinearColor(0.35f, 0.65f, 0.35f, 1.f), EBlockoutTexturePattern::Brick },
			{ TEXT("危险"), FLinearColor(1.00f, 0.25f, 0.20f, 1.f), EBlockoutTexturePattern::Checker },
		};
		TSharedPtr<SHorizontalBox> Row1, Row2;
		PropertyBox->AddSlot().AutoHeight().Padding(2, 2)
		[
			SAssignNew(Row1, SHorizontalBox)
		];
		PropertyBox->AddSlot().AutoHeight().Padding(2, 2)
		[
			SAssignNew(Row2, SHorizontalBox)
		];
		for (int32 i = 0; i < UE_ARRAY_COUNT(Presets); ++i)
		{
			const FPresetDef& P = Presets[i];
			TSharedPtr<SHorizontalBox> Row = (i < 3) ? Row1 : Row2;
			Row->AddSlot().AutoWidth().Padding(1)
			[
				SNew(SButton)
				.Text(FText::FromString(P.Label))
				.OnClicked_Lambda([this, P]()
				{
					if (UBlockoutEditorSubsystem* S = GetSub(PCPtr))
					{
						S->SetColor(P.Color);
						S->SetTexturePattern(P.Pattern, 4.f);
					}
					return FReply::Handled();
				})
			];
		}
	}

	// A2 基础贴图（棋盘格/网格/砖墙/混凝土/瓷砖/金属板）+ 平铺密度
	{
		struct FPatternDef { const TCHAR* Label; EBlockoutTexturePattern Pattern; };
		const FPatternDef Patterns[] = {
			{ TEXT("纯色"), EBlockoutTexturePattern::None },
			{ TEXT("棋盘格"), EBlockoutTexturePattern::Checker },
			{ TEXT("网格"), EBlockoutTexturePattern::Grid },
			{ TEXT("砖墙"), EBlockoutTexturePattern::Brick },
			{ TEXT("混凝土"), EBlockoutTexturePattern::Concrete },
			{ TEXT("瓷砖"), EBlockoutTexturePattern::Tile },
			{ TEXT("金属板"), EBlockoutTexturePattern::MetalPlate },
		};
		for (int32 Row = 0; Row < 2; ++Row)
		{
			TSharedPtr<SHorizontalBox> PatRow;
			PropertyBox->AddSlot().AutoHeight().Padding(2, 2)
			[
				SAssignNew(PatRow, SHorizontalBox)
			];
			const int32 Begin = Row * 4, End = FMath::Min<int32>(Begin + 4, (int32)UE_ARRAY_COUNT(Patterns));
			for (int32 i = Begin; i < End; ++i)
			{
				const FPatternDef& P = Patterns[i];
				PatRow->AddSlot().AutoWidth().Padding(1)
				[
					SNew(SButton)
					.Text(FText::FromString(P.Label))
					.OnClicked_Lambda([this, A, P]()
					{
						if (UBlockoutEditorSubsystem* S = GetSub(PCPtr))
						{
							S->SetTexturePattern(P.Pattern, A->Data.Tiling);
						}
						return FReply::Handled();
					})
				];
			}
		}
	}

	// A3 自定义颜色（色板）
	{
		const FLinearColor Palette[] = {
			FLinearColor::White, FLinearColor(0.5f, 0.5f, 0.5f, 1.f), FLinearColor::Black,
			FLinearColor::Red, FLinearColor::Green, FLinearColor::Blue,
			FLinearColor::Yellow, FLinearColor(1.f, 0.5f, 0.f, 1.f),
		};
		TSharedPtr<SHorizontalBox> PalRow;
		PropertyBox->AddSlot().AutoHeight().Padding(2, 2)
		[
			SAssignNew(PalRow, SHorizontalBox)
		];
		for (const FLinearColor& C : Palette)
		{
			PalRow->AddSlot().AutoWidth().Padding(1)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(C)
				.OnClicked_Lambda([this, C]()
				{
					if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->SetColor(C);
					return FReply::Handled();
				})
				[
					SNew(SBox).WidthOverride(16).HeightOverride(16)
				]
			];
		}
	}

	// A4 视图模式：无光照 / 光照 / 线框叠加
	{
		auto SetViewMode = [this](const TCHAR* Cmd)
		{
			if (UWorld* W = PCPtr.IsValid() ? PCPtr->GetWorld() : nullptr)
			{
				GEngine->Exec(W, Cmd);
			}
		};
		PropertyBox->AddSlot().AutoHeight().Padding(2, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(1)
			[
				SNew(SButton)
				.Text(BlockoutUI::T(TEXT("Unlit"), TEXT("无光照")))
				.OnClicked_Lambda([SetViewMode]()
				{
					SetViewMode(TEXT("r.ShowFlag.Lighting 0"));
					SetViewMode(TEXT("r.ShowFlag.Wireframe 0"));
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(1)
			[
				SNew(SButton)
				.Text(BlockoutUI::T(TEXT("Lit"), TEXT("光照")))
				.OnClicked_Lambda([SetViewMode]()
				{
					SetViewMode(TEXT("r.ShowFlag.Lighting 1"));
					SetViewMode(TEXT("r.ShowFlag.Wireframe 0"));
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(1)
			[
				SNew(SButton)
				.Text(BlockoutUI::T(TEXT("Wire"), TEXT("线框")))
				.OnClicked_Lambda([SetViewMode]()
				{
					SetViewMode(TEXT("r.ShowFlag.Wireframe 1"));
					return FReply::Handled();
				})
			]
		];
	}

	PropertyBox->AddSlot().AutoHeight()
	[
		MakeNumBox(
			[A]() { return A->Data.Color.R; },
			[this, A](float V)
			{
				A->Data.Color.R = V;
				A->ApplyAppearance();
			},
			0.f, 1.f, TEXT("颜色R"))
	];
	PropertyBox->AddSlot().AutoHeight()
	[
		MakeNumBox(
			[A]() { return A->Data.Color.G; },
			[this, A](float V)
			{
				A->Data.Color.G = V;
				A->ApplyAppearance();
			},
			0.f, 1.f, TEXT("颜色G"))
	];
	PropertyBox->AddSlot().AutoHeight()
	[
		MakeNumBox(
			[A]() { return A->Data.Color.B; },
			[this, A](float V)
			{
				A->Data.Color.B = V;
				A->ApplyAppearance();
			},
			0.f, 1.f, TEXT("颜色B"))
	];
	PropertyBox->AddSlot().AutoHeight()
	[
		MakeNumBox(
			[A]() { return A->Data.Tiling; },
			[this, A](float V)
			{
				A->Data.Tiling = V;
				A->ApplyAppearance();
			},
			0.1f, 20.f, TEXT("纹理平铺"))
	];

	// 形状参数
	PropertyBox->AddSlot().AutoHeight().Padding(2, 6)
	[
		SNew(STextBlock).Text(BlockoutUI::T(TEXT("Params"), TEXT("形状参数")))
	];
	struct FParamDef { float FBlockoutShapeParams::*Ptr; const TCHAR* Label; float MinV; float MaxV; };
	const FParamDef Defs[] = {
		{ &FBlockoutShapeParams::Length, TEXT("长度X"), 1.f, 100000.f },
		{ &FBlockoutShapeParams::Width, TEXT("宽度Y"), 1.f, 100000.f },
		{ &FBlockoutShapeParams::Height, TEXT("高度Z"), 1.f, 100000.f },
		{ &FBlockoutShapeParams::Thickness, TEXT("厚度"), 1.f, 10000.f },
		{ &FBlockoutShapeParams::AngleDeg, TEXT("角度"), 0.f, 360.f },
		{ &FBlockoutShapeParams::HoleWidth, TEXT("洞宽"), 0.f, 50000.f },
		{ &FBlockoutShapeParams::HoleHeight, TEXT("洞高"), 0.f, 50000.f },
	};
	for (const FParamDef& D : Defs)
	{
		auto MemberPtr = D.Ptr;
		PropertyBox->AddSlot().AutoHeight()
		[
			MakeNumBox(
				[A, MemberPtr]() { return A->Data.Params.*MemberPtr; },
				[this, A, MemberPtr](float V)
				{
					A->Data.Params.*MemberPtr = V;
					if (UBlockoutEditorSubsystem* S = GetSub(PCPtr)) S->SetShapeParams(A, A->Data.Params);
				},
				D.MinV, D.MaxV, FString(D.Label))
		];
	}

	// 测量按钮
	PropertyBox->AddSlot().AutoHeight().Padding(2, 8)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(BlockoutUI::T(TEXT("MeasDim"), TEXT("尺寸")))
			.OnClicked_Lambda([this, A]()
			{
				FBlockoutMeasureItem Item = FBlockoutMeasureLib::MeasureDimensions(A);
				Toast(Item.Text);
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(BlockoutUI::T(TEXT("MeasArea"), TEXT("面积")))
			.OnClicked_Lambda([this, A]()
			{
				FBlockoutMeasureItem Item = FBlockoutMeasureLib::MeasureArea(A);
				Toast(Item.Text);
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(BlockoutUI::T(TEXT("MeasVol"), TEXT("体积")))
			.OnClicked_Lambda([this, A]()
			{
				FBlockoutMeasureItem Item = FBlockoutMeasureLib::MeasureVolume(A);
				Toast(Item.Text);
				return FReply::Handled();
			})
		]
	];
}

// ================= 快捷键提示 =================
TSharedRef<SWidget> SBlockoutEditorUI::BuildHotkeyBar()
{
	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.4f))
		[
			SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Text(BlockoutUI::T(TEXT("Hotkeys"),
				TEXT("快捷键：W/E/R 变换 | 左键选择/拖Gizmo | 框选 | 右键旋转视角 | WASDQE 飞行 | G网格吸附 | S表面吸附 | B坐标空间 | F聚焦 | Ctrl+Z/Y 撤销重做 | Ctrl+D 复制 | Del 删除 | Tab 隐藏UI")))
		];
}

// ================= 右键菜单 =================
FReply SBlockoutEditorUI::OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev)
{
	if (Ev.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OpenContextMenu(Ev.GetScreenSpacePosition());
		return FReply::Handled();
	}
	return SCompoundWidget::OnMouseButtonDown(Geo, Ev);
}

void SBlockoutEditorUI::OpenContextMenu(const FVector2D& ScreenPos)
{
	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	if (!Sub) return;

	auto CloseAnd = [this](TFunction<void()> Action)
	{
		FSlateApplication::Get().DismissAllMenus();
		Action();
	};

	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox)

	+ SVerticalBox::Slot().AutoHeight()
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("CtxDup"), TEXT("复制 (Ctrl+D)")))
		.OnClicked_Lambda([Sub, CloseAnd]()
		{
			CloseAnd([Sub]() { Sub->DuplicateSelected(); });
			return FReply::Handled();
		})
	]
	+ SVerticalBox::Slot().AutoHeight()
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("CtxDel"), TEXT("删除 (Del)")))
		.OnClicked_Lambda([Sub, CloseAnd]()
		{
			CloseAnd([Sub]() { Sub->DeleteSelected(); });
			return FReply::Handled();
		})
	]
	+ SVerticalBox::Slot().AutoHeight()
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("CtxGroup"), TEXT("分组")))
		.OnClicked_Lambda([Sub, CloseAnd]()
		{
			CloseAnd([Sub]() { Sub->GroupSelected(); });
			return FReply::Handled();
		})
	]
	+ SVerticalBox::Slot().AutoHeight()
	[
		SNew(SButton)
		.Text(BlockoutUI::T(TEXT("CtxUnGroup"), TEXT("取消分组")))
		.OnClicked_Lambda([Sub, CloseAnd]()
		{
			CloseAnd([Sub]() { Sub->UngroupSelected(); });
			return FReply::Handled();
		})
	];

	FSlateApplication::Get().PushMenu(
		TSharedPtr<IMenu>(),
		Menu,
		ScreenPos,
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
	);
}

void SBlockoutEditorUI::Toast(const FString& Msg)
{
	ABlockoutEditorPlayerController* P = GetPC(PCPtr);
	if (!P) return;
	if (AHUD* Hud = P->GetHUD())
	{
		if (ABlockoutHUD* BHud = Cast<ABlockoutHUD>(Hud))
		{
			BHud->ShowToast(Msg);
		}
	}
}

// ================= Tick 刷新 =================
void SBlockoutEditorUI::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UBlockoutEditorSubsystem* Sub = GetSub(PCPtr);
	if (!Sub) return;

	// 选中集合变化时刷新属性面板
	if (Sub->GetSelection() != CachedSelection)
	{
		CachedSelection = Sub->GetSelection();
		RefreshPropertyPanel();
	}
}
