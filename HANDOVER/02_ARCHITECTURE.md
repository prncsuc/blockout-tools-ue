---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 5227bc309b64902b9bfd2bb79d4de347_075eba9ccad211f18b7f525400393706
    ReservedCode1: 6U71+PzG2tl/jMLI4r7GFQnBsMqPADgTWlWFUSFsDoZRR5sYA7LbR8Ik+1AhVNHGwUHwSEzPwAH6jYyZlqrp93ZEGeGYF0CMfSueWJ/7OlO4yzSnU7xpOELPpiFo60daDOtYL3Tqia/fkzNBBWz56LNDxMLFMGaRiy2YZk+iwLMb7vhqqebxnJ2Cd5Y=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 5227bc309b64902b9bfd2bb79d4de347_075eba9ccad211f18b7f525400393706
    ReservedCode2: 6U71+PzG2tl/jMLI4r7GFQnBsMqPADgTWlWFUSFsDoZRR5sYA7LbR8Ik+1AhVNHGwUHwSEzPwAH6jYyZlqrp93ZEGeGYF0CMfSueWJ/7OlO4yzSnU7xpOELPpiFo60daDOtYL3Tqia/fkzNBBWz56LNDxMLFMGaRiy2YZk+iwLMb7vhqqebxnJ2Cd5Y=
---

# 02 架构与模块清单

## 1. 工程结构

```
<REPO_ROOT>\        # 本仓库根目录（BlockoutStudio.uproject 所在处）
├── BlockoutStudio.uproject        # UE 5.6.1 Game 项目（可打包，编辑功能全部 runtime 实现）
├── Source\BlockoutStudio\         # 全部 C++ 模块（单模块）
├── Content\
│   ├── Levels\                    # 示例关卡 JSON（level_dust2_final / dustyard / souls_castle / uncharted_ruins，含 demoPath）
│   ├── Characters\Mannequins\     # 官方小白人（Manny/Quinn + ABP_Unarmed + FirstPerson_AnimBP + Enhanced Input）
│   ├── PMB\BlockoutToolKit\       # PMB 正版资产（含 T_Grid 网格贴图）
│   └── UEFirstPerson\             # 官方第一人称资产
├── ThirdParty\ffmpeg\             # ffmpeg.exe（LGPL 动态链接，RuntimeDependencies 分发）
├── tools\                         # 关卡 JSON 生成脚本（Python，blk()/room()/corridor 辅助）
├── Packaged\Windows\              # Shipping 打包产物（Git LFS 管理）
├── docs\USER_MANUAL.md            # 用户手册
├── HANDOVER\                      # 本交接包
├── REQUIREMENT_CHECK.md           # 需求逐条核对（状态以此为准）
├── ACCEPTANCE.md / FEATURE_COMPARISON.md / PROGRESS.md
└── Build\ Intermediate\ Saved\    # 不入 git
```

## 2. C++ 模块清单（Source\BlockoutStudio\）

| 文件/类 | 职责 |
|---|---|
| `BlockoutEditorGameMode` | 游戏模式入口 |
| `BlockoutEditorPlayerController` | **全部快捷键绑定与双交互模式切换**（编辑/测试）；输入处理（Ctrl 加选、拖拽、F5/F8/F9 等） |
| `BlockoutEditorUI` (`SBlockoutEditorUI`, ~3500 行 Slate) | 三区 UI：上工具栏、左侧创建面板（缩略图卡片）、右侧属性面板+场景树、底部快捷条。场景树含多选/拖拽排序/编组/行内重命名/可见性锁定 CheckBox/搜索过滤/右键菜单；Tick 内 SyncTreeSelectionFromSub 双向同步 + 0.5s 低频轮询兜底刷新 |
| `BlockoutShapePreview` (`SBlockoutShapePreview`) | Slate 程序绘制形状缩略图：OnPaint 折线几何预览（FSlateDrawElement::MakeLines/MakeBox），`GetShapePolylines(EBlockoutShape)` 返回各形状归一化轮廓 |
| `BlockoutEditorSubsystem` (`UBlockoutEditorSubsystem`) | 核心 API：SpawnBlock/LoadProject/GetAllBlocks/ReorderBlock/AssignGroupToActor/RenameActor/SetHidden/SetLocked、撤销栈 PushUndo（150 步）、快照/自动保存 |
| `BlockoutActor` (`ABlockoutActor`) | 体块 Actor；**Rebuild() 双路径**：`bFreeform && Vertices/Triangles 非空` → 顶点+三角直建（FinalizeMesh(false,100)）；否则按 Shape/Params 参数化 BuildShape。布尔/点线面结果写回 `FBlockoutObjectData.Vertices/Triangles` + `bFreeform=true` 即可复用撤销/序列化/重建全链路 |
| `BlockoutGizmo` (`ABlockoutGizmo`) | 自研 runtime gizmo：移动/旋转/缩放三模式、6 面尺寸手柄（Begin/Update/EndResizeDrag）、轴向按 CoordSpace（世界/本地）旋转 |
| `BlockoutTestPawn` | 测试跑图 Pawn：官方 Manny/Quinn + ABP_Unarmed + Enhanced Input（IA_Move/Look/Jump + Shift 跑 + Ctrl 蹲）+ 官方 SpringArm TPS + FPS 官方手臂；ApplyPreset(CS/Souls/Uncharted)；DoJump 落地统计跳高/跳距 |
| `FBlockoutMeshEditLib` | 点线面编辑：MoveVertices/ExtrudeFaces/BevelVertices/CollapseVertices/WeldVertices/InsetFaces/SubdivideFaces/DeleteFaces/FlipNormals |
| `FBlockoutExportLib` | 导出 OBJ/FBX/glTF/USD；FBX/glTF 输出 Y-up（glTF 米制、FBX UnitScaleFactor=1 cm），Unity/UE 双端自动换算 |
| `FBlockoutImportLib` | 导入 OBJ/glTF 为参考体（SpawnReferenceMesh） |
| `FBlockoutRecordLib` | 轨迹录制采样/回放插值；视频录制 AVI(MJPEG) + ffmpeg 转封装 MP4(H.264)/WebM(VP8) 双成品 |
| `FBlockoutMeasureLib` | 测距（两点距离/高度差/角度/面积/体积）+ 标注 |
| `FBlockoutTopDownLib` | 2D 俯视图导出 PNG/SVG（可选网格/尺寸/名称） |
| `FBlockoutTextureLib` | 贴图样式（Grid/Checker/Brick/Concrete/Tile/MetalPlate）；Grid 复用 PMB T_Grid |
| `FBlockoutFileDialog` | Win32 通用文件对话框（**替代 DesktopPlatform，见 03_PITFALLS**） |

## 3. 核心数据格式

### 3.1 工程/关卡 JSON

```json
{
  "version": 1,
  "savedAt": "2026-10-18T00:00:00",
  "blocks": [
    {
      "Id": "...", "Name": "...",
      "Shape": "Wall",
      "Params": { "X": 400, "Y": 20, "Z": 200 },
      "Transform": { "Location": [x,y,z], "Rotation": [p,y,r], "Scale": [1,1,1] },
      "Color": [r,g,b], "TexturePattern": "Grid",
      "GroupId": "", "bHidden": false, "bLocked": false,
      "bFreeform": false, "Vertices": [], "Triangles": []
    }
  ]
}
```

- tools\ 下 Python 脚本按此格式生成关卡，统一 blk()/room()/corridor 辅助函数；
- 规模自检：按 X/Y/Z 包围盒输出。

### 3.2 自动化命令行参数（Shipping exe）

`-AutoExportTest=<dir>` / `-AutoPerfTest=<n>` / `-AutoCollisionTest=<关卡JSON>` / `-AutoDemoLevel=<关卡JSON> -AutoDemoOut=<dir>`，运行完自动退出，输出 JSON 报告。

## 4. 双交互模式

| 模式 | 进入 | 行为 |
|---|---|---|
| 编辑模式 | 默认/退出测试 | 完整编辑 UI + gizmo + 场景树 |
| 测试模式 | F5（Esc/F5 退出） | 官方小白人跑图，SavedPawn/相机现场保存恢复，编辑状态保留 |

## 5. 关键代码事实（易踩）

- `BuildCreatePanel` 用 static 数组 + lambda，注意**捕获陷阱**（悬垂引用/复制语义）；
- 撤销：`PushUndo` 自动裁剪至 150 步；隐藏/锁定/重命名/拖拽排序/编组都入栈；
- 场景树拖拽：`FDragDropOperation` 派生 `FBlockoutTreeDragOp`（含装饰器文本）；接收委托挂 `STableRow`（非 SListView）；OntoItem=编组、Above/Below=排序（ReorderBlock）；
- 共面体块 +0.02uu 膨胀防 Z-fighting；
- AutoDemo 机位用 `AutoDemoPlacedShot` 标记法（Timer<0.1f 会失效）。
*（内容由AI生成，仅供参考）*
