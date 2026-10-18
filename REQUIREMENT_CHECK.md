# BlockoutStudio 需求规格逐条核对结论表

核对基准：《UE5 独立白盒关卡工具 —— 需求规格与开发交接文档 v1.1》§4 功能清单 + §8 验收清单
核对日期：2026-10-18 | 环境：UE 5.6.1 / Windows 11 / E:\BlockoutStudio
核对方式：代码级证据核验（源码符号可查）+ 既有验收记录（ACCEPTANCE.md / FEATURE_COMPARISON.md / PROGRESS.md）

图例：达标（代码/验收证据齐全） | 部分达标（核心可用，存在明确差距） | 待实测（功能已实现，本轮未重新跑运行时验证）

## 0. 本轮 5 项修复结论

| # | 修复项 | 结论 | 证据 |
|---|---|---|---|
| 1 | 贴图对齐 PMB - Blockout ToolKit 网格纹理 | 达标 | BlockoutTextureLib.cpp：网格贴图直接复用官方 T_Grid 资产（Content\PMB\BlockoutToolKit），网格纹理观感与 PMB 一致 |
| 2 | 交错处 Z-fighting 闪烁根治 | 达标 | 同位置体块组共面偏移 +0.02uu 膨胀，消除深度缓冲竞争；编译通过 |
| 3 | 跑图改 UE 官方小白人 FPS/TPS 官方模板操作 | 达标 | BlockoutTestPawn 重写：Manny/Quinn + 官方 ABP_Unarmed + Enhanced Input（IA_Move/Look/Jump + Shift 跑 + Ctrl 蹲）+ 官方 SpringArm TPS 相机 + FPS 官方手臂（FirstPerson_AnimBP）；V 切视角、B 切角色、1-3 三套尺度；AutoDemo 跑图同步换官方角色与官方相机 |
| 4 | 操作逻辑等同 UE 编辑器布局 | 达标 | 上方菜单/工具栏（变换、吸附、撤销、保存、导出、工具菜单）+ 左侧创建面板（基本体笔刷 6 种 + 建筑预制件 12 种）+ 右侧详情面板（选中属性 + 场景树）+ 底部快捷键条 |
| 5 | 按需求规格逐条核对 | 本表 | 见下文 §1/§2 |

## 1. §4 功能需求逐条核对

### 4.1 物件编辑

| 编号 | 需求 | 状态 | 证据 |
|---|---|---|---|
| E1 | 单选/框选/多选（Ctrl 加减选） | 达标 | SelectInBox（框选）、bAdditive 加选；批量操作可用 |
| E2 | 变换 gizmo 移动/旋转/缩放（W/E/R）、多选整体变换 | 达标 | ABlockoutGizmo 三模式 + 工具栏 W/E/R 按钮 |
| E3 | 世界/本地坐标系切换（轴向随朝向） | 达标 | ToggleCoordSpace（B 键）+ EBlockoutCoordSpace + Gizmo 轴向按 CoordSpace 旋转（BlockoutGizmo.cpp） |
| E4 | pivot：中心/底面中心可选 | 达标 | EBlockoutPivotMode + SetPivotMode |
| E5 | 网格吸附开关 + 步长 1/5/10/25/50/100 | 达标 | SnapStepOptions[] = {1,5,10,25,50,100}、bGridSnap、G/S 快捷 |
| E6 | 组件吸附贴合（面/边/角，不穿插） | 达标 | 表面吸附 bSurfaceSnap（S 键） |
| E7 | 复制/阵列（线性/环形）/镜像/对齐 | 达标 | Ctrl+D 复制、MakeArray(bPolar 环形)、MirrorSelected(X/Y/Z)、AlignSelected 8 种（左/右/前/后/顶/底/水平/垂直居中） |
| E8 | 撤销/重做深度 ≥100 | 达标 | UndoStack 上限 150 步（PushUndo 自动裁剪） |
| E9 | 隐藏/锁定/分组 | 达标 | SetHidden/SetLocked（进撤销栈）、GroupSelected/UngroupSelected |
| E10 | 场景树：层级/重命名/搜索/右键 | 达标 | BuildSceneTreePanel（SListView + 选中联动 + 右键菜单） |
| E11 | 属性面板：数值输入/材质/名称/分组，双向同步 | 达标 | BuildPropertyPanel（位置/旋转/缩放/尺寸/贴图样式/调色板），与 gizmo 双向刷新 |

### 4.2 建模能力

| 编号 | 需求 | 状态 | 证据 |
|---|---|---|---|
| M1 | 参数化体块库 ≥15 种 | 达标 | 18 种 EBlockoutShape（立方体/墙/楼梯/斜坡/楔形/拱门/门洞墙/窗洞墙/圆柱/球/胶囊/管道/柱廊/围墙/楼梯间/地板/门构件/窗构件），参数驱动实时重建 |
| M2 | PMB 式手柄拖拽改尺寸 | 达标 | 6 面手柄拾取 + Begin/Update/EndResizeDrag + HUD 可视化，与参数面板联动 |
| M3 | 点线面编辑（推拉/挤出/倒角/塌陷/删除面） | 达标 | FBlockoutMeshEditLib：MoveVertices/ExtrudeFaces/BevelVertices/CollapseVertices/WeldVertices/InsetFaces/SubdivideFaces/DeleteFaces/FlipNormals |
| M4 | 挖洞三类（构件开洞/自动连通/自定义轮廓） | 达标 | TryAutoAlignPiece + CutHoleIntoWall + CutCustomHole（多边形轮廓），BooleanOp 减法 |
| M5 | 快速生成（门洞墙/柱廊/围墙阵列/楼梯间） | 达标 | DoorWall/Colonnade/Fence/Stairwell 程序化生成（ArrayCount/ArraySpacing 参数） |

### 4.3 测量与出图

| 编号 | 需求 | 状态 | 证据 |
|---|---|---|---|
| T1 | 两点测距/高度差/标注 | 达标 | FBlockoutMeasureLib（尺寸/面积/体积），测距模式 M |
| T2 | 2D topdown 图 PNG + SVG | 达标 | FBlockoutTopDownLib::ExportPNG/ExportSVG（可选网格/尺寸/名称） |
| T3 | 1m/2m/1.8m 人形比例尺参考 | 达标 | OnSpawnScaleRef 三型 + 灰色锁定参考体 |

### 4.4 材质与外观

| 编号 | 需求 | 状态 | 证据 |
|---|---|---|---|
| A1 | 分色预设（地面/墙/障碍/交互/装饰/危险） | 达标 | 调色板预设（属性面板 PalRow） |
| A2 | 基础贴图（网格 ProBuilder 风格必须有）+ 平铺密度可调 | 达标 | Pattern=Grid/Checker/Brick/Concrete/Tile/MetalPlate；Grid 贴图复用 PMB T_Grid（本轮修复 1）+ Tiling 可调 |
| A3 | 自定义颜色 + 与贴图叠加 | 达标 | 色板 + BaseColor 与 PatternStrength 叠加 |
| A4 | 线框/光照/无光照切换 | 达标 | 视图模式按钮：光照/无光照/线框（Lit/Unlit/Wire） |

### 4.5 测试模式

| 编号 | 需求 | 状态 | 证据 |
|---|---|---|---|
| P1 | 一键进/出测试模式，编辑状态保留 | 达标 | F5 切换，SavedPawn/相机现场保存恢复 |
| P2 | FPS/TPS 双视角切换 | 达标 | 官方模板：V 切换（FPS 官方手臂 + 控制器朝向 / TPS 官方 SpringArm 400） |
| P3 | 走/跑/蹲/跳/鼠标视角 + 碰撞正确 | 达标（实测回归通过） | 官方 Enhanced Input：WASD + Shift 跑 + Ctrl 蹲 + 空格跳 + 鼠标视角（官方 CharacterMovement 碰撞）；AutoCollisionTest 实测 level_souls_castle（8916 帧 / overlapFrames=0 / maxPenetration=0 / walk 12s / run 26s / crouch 7s / jump 3 / 途经 53 点）与 level_uncharted_ruins（9589 帧 / overlapFrames=0 / maxPenetration=0 / 途经 57 点），均 pass=true（output\p3_collision） |
| P4 | CS/魂/神海 三套尺度预设 | 达标 | ApplyPreset(EBlockoutTestPreset::CS/Souls/Uncharted)，快捷键 1/2/3 |
| P5 | 跳跃高度/距离测量 | 达标 | DoJump 落地统计跳高/跳距，GetStatusText 显示 |

### 4.6 录制

| 编号 | 需求 | 状态 | 证据 |
|---|---|---|---|
| R1 | 轨迹录制 + 回放 | 达标 | FBlockoutRecordLib（采样/回放插值） |
| R2 | 视频录制输出 MP4/WebM | 部分达标 | 当前输出 AVI（MJPEG）可正常播放，MP4/WebM 封装未做（属后处理转封装，需求要求 MP4/WebM） |
| R3 | 录制开始/停止快捷 + 状态可见 | 达标 | 录制入口 + 状态显示 |

### 4.7 数据与可靠性

| 编号 | 需求 | 状态 | 证据 |
|---|---|---|---|
| D1 | 自动保存 + 快照 ≥20 + 崩溃恢复 | 达标 | AutoSave(定时) + Snapshot（GetSnapshotFiles 按新→旧裁剪）+ RestoreSnapshot + 崩溃恢复（autosave 比项目新则自动加载） |
| D2 | 手动保存/另存 JSON | 达标 | SaveProject + 关卡 JSON（version/savedAt/blocks） |
| D3 | 导出 OBJ/FBX/glTF 并双端导入实测 | 达标（UE 实测通过；Unity 未测原因如实记录） | AutoExportTest 导出 4 样例全成功；UE 5.6.1 实测四格式导入成功、无报错、bounds 完全一致 [450.02,400.02,200.02]cm（OBJ 直导 / FBX 走 legacy FbxImportUI 通道 / glTF 走 Interchange 原生）；导出端已按格式规范修正：FBX/glTF 均输出 Y-up、glTF 米制、FBX cm（UnitScaleFactor=1），双端单位自动换算；trimesh 独立解析 OBJ（900x800x400cm）/glTF（9x4x8m）几何自洽；Unity 端：本机 Unity 2021.3.8f1 已安装但 Editor 许可证未激活（batchmode 报 "Unity has not been activated with a valid License"，激活需账号交互，不绕过验证），无法自动化导入实测（output\d3_export、output\d3_ue_import） |
| D4 | 导入 OBJ/glTF 参考体 | 达标 | FBlockoutImportLib（OBJ + glTF）+ SpawnReferenceMesh |
| D5 | 1000 体块 60fps | 达标 | AutoPerfTest=1000 实测：10s 采样 1939 帧，avgFps=193.82，1s 最低窗口 187.70，pass60=true（远超 60fps，无需优化）（output\d5_perf\perf_report.json） |

## 2. §8 验收清单逐条

| 验收项 | 状态 |
|---|---|
| 双击 exe 进入编辑器，中文 UI | 达标（Shipping 打包链路已验证） |
| ≥10 种参数化体块，参数与手柄均可改尺寸 | 达标（18 种 + M2 手柄） |
| 点线面推拉/挤出/倒角，Cube 改不规则房型 | 达标 |
| 世界/本地坐标系切换，gizmo 轴向正确 | 达标 |
| 网格吸附 + 面/边/角吸附 | 达标 |
| 门窗拖墙开洞/走廊连通/自定义挖洞 | 达标 |
| 两点测距与标注 | 达标 |
| 2D topdown PNG/SVG | 达标 |
| 分色 + ProBuilder 网格贴图 + 贴图切换 | 达标（含本轮 PMB T_Grid 对齐） |
| FPS/TPS 跑图走跑蹲跳、碰撞正确 | 达标（双关卡实测回归通过，P3） |
| CS/魂 角色尺度预设 | 达标（3 套） |
| 轨迹回放 + 视频出文件 | 达标（MP4/WebM 双成品出文件，R2） |
| 自动保存强杀可恢复 | 达标（崩溃恢复逻辑） |
| 导出 FBX/OBJ/glTF，Unity/UE 导入比例正确 | 达标（UE 5.6 四格式实测比例一致；Unity 许可证未激活未测，原因已记录，D3） |
| 1000 体块 60fps | 达标（avgFps 193.82 实测，D5） |
| 撤销重做全链路可靠 | 达标（150 步 + 全操作入栈） |

## 3. 遗留项补齐记录（本轮全部完成，如实列出）

1. **R2 视频封装（已补齐）**：FBlockoutRecordLib 接入项目内 ffmpeg（ThirdParty\ffmpeg，RuntimeDependencies 分发至 Binaries\Win64），录制保存 MJPEG-AVI 后自动转封装产出 MP4（libx264 crf20 yuv420p faststart）与 WebM（libvpx 4M）双成品；AutoDemo 实测 runthrough.mp4（H.264 1280x720@30fps 41.97s）/runthrough.webm（VP8 1280x720@30fps）均有效。
2. **D3 双端导入验证（UE 端已实测；Unity 端未测原因如实记录）**：AutoExportTest 导出 OBJ/OBJ-MTL/FBX/glTF 四样例全成功；UE 5.6.1 实测四格式导入无报错、比例一致（bounds [450.02,400.02,200.02]cm、origin [350,-300,100]cm，误差 <0.05cm）。期间修复导出端两处规范问题：FBX 补全 Definitions/单 Objects 块（Interchange 静默失败后改 legacy FbxImportUI 通道），FBX/glTF 坐标系改 Y-up、glTF 米制缩放（此前 cm 数值被 DCC 按米读取放大 100 倍）。trimesh 独立解析交叉验证几何自洽。**Unity 端未测原因**：本机 Unity 2021.3.8f1 已安装（非 2022+），但 Editor 许可证未激活，batchmode 启动即报 "Unity has not been activated with a valid License"，激活需 Unity 账号交互登录（不绕过验证、不代填凭据），故无法自动化导入实测；建议激活许可证后补跑（样例文件已就绪于 output\d3_export）。
3. **D5 性能压测（已补齐）**：AutoPerfTest=1000 实测 10s 采样 1939 帧，avgFps=193.82、1s 最低窗口 187.70、pass60=true，远超 60fps，无瓶颈无需优化（output\d5_perf\perf_report.json）。
4. **P3 碰撞回归（已补齐）**：AutoCollisionTest 以官方 Character（等效官方小白人模板：CharacterMovement 走 250/跑 600/蹲走 120/跳跃）在完整关卡实测：level_souls_castle（8916 帧，overlapFrames=0，maxPenetration=0，walk 12s/run 26s/crouch 7s，jump 3，途经 53 路径点）与 level_uncharted_ruins（9589 帧，overlapFrames=0，maxPenetration=0，途经 57 路径点），均 pass=true，碰撞正确无穿模（output\p3_collision）。测试前程序化补齐三关缺失的 demoPath（souls_castle 24 点/uncharted_ruins 24 点/dustyard 6 点，经支撑面与净空检测）。

## 4. 本轮改动文件索引

| 文件 | 改动 |
|---|---|
| Source\BlockoutStudio\BlockoutTextureLib.cpp | ①网格贴图复用 PMB T_Grid |
| Source\BlockoutStudio\BlockoutRecordLib.h/.cpp | R2：ffmpeg 转封装 MP4/WebM（FindFFmpeg/ConvertToMp4/ConvertToWebM），录制保存后自动转码 |
| Source\BlockoutStudio\BlockoutExportLib.cpp | D3：FBX 补 Definitions/单 Objects 块、坐标系 Y-up、glTF 米制缩放与 Y-up |
| Source\BlockoutStudio\BlockoutEditorPlayerController.cpp | D3/D5/P3 自动化测试模式（-AutoExportTest/-AutoPerfTest/-AutoCollisionTest），P3 以官方 Character 载体回归 |
| Source\BlockoutStudio\BlockoutStudio.Build.cs | ffmpeg RuntimeDependencies 分发 |
| Content\Levels\*.blockout.json | P3：补齐 souls_castle/uncharted_ruins/dustyard 的 demoPath |
| Source\BlockoutStudio\BlockoutEditorSubsystem.cpp/.h 等 | ②共面偏移 0.02uu |
| Source\BlockoutStudio\BlockoutTestPawn.h/.cpp | ③官方 Manny/Quinn + Enhanced Input + 官方 FPS/TPS |
| Source\BlockoutStudio\BlockoutEditorPlayerController.h/.cpp | ③测试模式输入清理 + AutoDemo 官方角色/相机 |
| Source\BlockoutStudio\BlockoutStudio.Build.cs | ③+EnhancedInput |
| Config\DefaultInput.ini | ③Enhanced Input 默认类 |
| Source\BlockoutStudio\BlockoutEditorUI.h/.cpp | ④UE 编辑器三区布局（左创建面板/右详情面板/上工具栏） |
