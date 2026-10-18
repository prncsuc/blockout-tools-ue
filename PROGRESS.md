---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 5227bc309b64902b9bfd2bb79d4de347_7631db5aca3b11f18b7f525400393706
    ReservedCode1: 0jQnRUWH0CLQ+OHrEOyh9VcQA266lsR5wgrY37ofdW9fHJ+q107Pu+az8dY3IUHgPQtlUcJ4jncicbUlW1qiQoZZjt5Dv1SZjAmCrKkZzXLgUOtN/Q39TIYrpvVYQWeG2PrX83anqdsVHfGI/HyHY5YkN6Og+CA3jbqZPMK48n/BsnGv66UmawN743c=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 5227bc309b64902b9bfd2bb79d4de347_7631db5aca3b11f18b7f525400393706
    ReservedCode2: 0jQnRUWH0CLQ+OHrEOyh9VcQA266lsR5wgrY37ofdW9fHJ+q107Pu+az8dY3IUHgPQtlUcJ4jncicbUlW1qiQoZZjt5Dv1SZjAmCrKkZzXLgUOtN/Q39TIYrpvVYQWeG2PrX83anqdsVHfGI/HyHY5YkN6Og+CA3jbqZPMK48n/BsnGv66UmawN743c=
---

---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 5227bc309b64902b9bfd2bb79d4de347_ac979e1ec9e811f196f9525400248c00
    ReservedCode1: BEsAjlMPHU2mTOUGP01YY5SiAQf6RnMDg0vhiCKeFx6I3IulXlIacr/zhoqWK6KHaSSiVszIcOrIM8Tqe3OxrmJxdja1SBpXALB/9RyjcScIcJoyRcnKezprJyQhVz1F2c209xn0U2B0q5WZHD9S4lrd7eA7cWpy71U3kYlSGO7/2smiO2bNYruzD5o=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 5227bc309b64902b9bfd2bb79d4de347_ac979e1ec9e811f196f9525400248c00
    ReservedCode2: BEsAjlMPHU2mTOUGP01YY5SiAQf6RnMDg0vhiCKeFx6I3IulXlIacr/zhoqWK6KHaSSiVszIcOrIM8Tqe3OxrmJxdja1SBpXALB/9RyjcScIcJoyRcnKezprJyQhVz1F2c209xn0U2B0q5WZHD9S4lrd7eA7cWpy71U3kYlSGO7/2smiO2bNYruzD5o=
---

﻿# BlockoutStudio 开发进度

UE5 独立白盒关卡工具（Windows 桌面程序），按《UE5 白盒关卡工具-需求规格与开发交接文 v1.1》执行。

## 阶段进度

| 阶段 | 内容 | 状态 |
|---|---|---|
| P0 | 项目骨架与核心源码 | 已完成，编译通过（bf5c5f9） |
| P1 | 编辑器交互（gizmo/输入/UI/GameMode） | 已完成，编译通过 |
| P2 | 子编辑与录制 | 已完成，编译通过 |
| P3 | 材质资产与外观 | 已完成，编译通过 |
| P4 | 导出与测量完善 | 未开始 |
| P5 | 验收测试（§8 清单） | 未开始 |
| P6 | 打包发布 | 未开始 |

## P0 已完成

- `Source/BlockoutStudio/BlockoutTypes.h`：17 种体块类型、枚举与结构定义
- `BlockoutMeshGen.h/.cpp`：全部原语网格生成（Box/Cylinder/Sphere/Capsule/Pipe/Staircase/Ramp/Wedge/Colonnade/Fence + 洞体），自研 FDynamicMesh3 构建；布尔经 GeometryScript ApplyMeshBoolean；自写盒投影 UV 与平滑法线
- `BlockoutTextureLib.h/.cpp`：6 种程序化纹理（PF_G8）
- `BlockoutActor.h/.cpp`：DynamicMeshComponent + MID 外观 + 高亮，CachedMesh 缓存供导出/测量
- `BlockoutEditorSubsystem.h/.cpp`：选择/变换/gizmo 拖拽/吸附/撤销 150 步/自动保存 JSON/分组
- `BlockoutMeasureLib.h/.cpp`：测量能力
- `BlockoutExportLib.h/.cpp`：OBJ / ASCII FBX / glTF / USDA / USDZ / 自定义格式导出
- `Build.cs`：依赖 Json/JsonUtilities/GeometryCore/GeometryFramework/GeometryScriptingCore/ImageWrapper

## 关键决策

- 网格生成不依赖 GeometryScript 建原语，自研 FDynamicMesh3 手工构建，仅布尔用 GeometryScript
- UI 纯 Slate 代码（中文），不依赖 UMG 资产
- 录制方案：MJPEG-AVI 自研容器；FBX 走自研 ASCII writer
- 材质：tools/create_materials.py 生成 `/BlockoutStudio/Materials/M_BlockoutBase`，缺失 fallback BasicShapeMaterial
- 参考资产：`Content/PMB/BlockoutToolKit`（PMB 正版插件白盒工具包，含 SnapBoole 吸附布尔、可调材质、T_Grid 网格纹理等），仅在本机参考，不入库
- 第三方资产（PMB/PMB_City/UEFirstPerson 模板）不推送公共仓库

## 环境

- 引擎：UE 5.6.1（安装目录记作 `<UE_ROOT>`，自行准备或向用户索要）
- 工具链：VS 2022 BuildTools 17.14（VCTools workload）
- 项目：本仓库克隆根目录（记作 `<REPO_ROOT>`）
- 网络：github 推送优先直连（`git -c http.proxy= -c https.proxy= push`）；若所在网络必须走代理，代理地址向用户索要后再配置

## 待完成

- [x] 编译验证 P0 代码（2026-08-21 通过）
- [x] BlockoutGizmo、PlayerController/HUD/输入、Slate UI、GameMode（2026-10-17 P1 通过）
- [x] 录制 R1-R3、M3 点线面子编辑（2026-10-17 P2 通过）
- [x] 材质生成脚本与资产（2026-10-17 P3 通过）：tools/create_materials.py + /Game/BlockoutStudio/Materials/M_BlockoutBase
- [x] P5 验收（2026-10-17 通过）：三关卡鸟瞰图各 3 张视角互异；跑图视频 v2 抽帧验证画面无遮挡无穿模、亮度正常、相机明显移动，全部通过，详见 ACCEPTANCE.md
- [ ] 打包（P6）


## P0 编译修复记录（2026-08-21）

- **UE5.6 API 迁移**：AddVertex/AddTriangle → AppendVertex/AppendTriangle；布尔改用 GeometryCore 的 UE::Geometry::FMeshBoolean（Compute()）；移除 GeometryScriptingCore 依赖；DynamicMesh 头文件加 `DynamicMesh/` 前缀；`bUseComplexAsSimpleCollision` → `EnableComplexAsSimpleCollision()`；`GetMesh()` 返回指针解引用拷贝至 CachedMesh
- **FIndex3i 取分量**：`GetIndex(i)` 不存在，改 `operator[]` 即 `Tri[i]`
- **属性层 API**：`HasNormalLayers()/HasUVLayers()` 不存在，改 `NumNormalLayers()/NumUVLayers() > 0`；补 `DynamicMeshAttributeSet.h` / `DynamicMeshOverlay.h` include 与 `using UE::Geometry::FIndex3i;`
- **头文件前置声明遮蔽**：`class FDynamicMesh3;` 导致 C2027，改为 include + `using UE::Geometry::FDynamicMesh3;`
- **类型定义对齐**：EBlockoutSurfaceType 枚举值统一为 Solid/Glass/Metal/Wood/Concrete/Grass/Dirt/Water/Wall；FBlockoutUndoOp 补 OpType/Timestamp；FBlockoutMeasureItem 补 Type/PointA/PointB/Value/Text；新增 EBlockoutMeasureType 枚举
- **UE5 double 化**：`FVector::operator[]` 返回 double&，局部引用改 `double&`；FMath::LinePlaneIntersection 用 3 参数返回交点版
- **源码编码**：全部 .h/.cpp 转 UTF-8 with BOM，修复 MSVC 按 GBK 解析中文字符串导致的 Printf 编译期格式校验误报（C7595）
- **结果**：BlockoutStudioEditor Win64 Development 编译通过，生成 UnrealEditor-BlockoutStudio.dll


## P1 已完成（2026-10-17）

- `BlockoutGizmo.h/.cpp`：变换 Gizmo 状态与轴拾取（射线-线段距离判定，支持世界/本地坐标）
- `BlockoutEditorPlayerController.h/.cpp`：完整输入层
  - 相机：右键旋转视角 + WASDQE 飞行 + 滚轮推进
  - 左键：gizmo 轴拾取拖拽 → 物体拾取（射线-AABB slab 求交）→ 空白框选
  - 快捷键：W/E/R 变换模式、Ctrl+Z/Y 撤销重做、Ctrl+D 复制、Del 删除、G/S 吸附、B 坐标空间、F 聚焦、Ctrl+A 全选、P 保存、O/U 分组、Tab 隐藏 UI、Esc 取消选择
- `BlockoutHUD.h/.cpp`：gizmo 三轴渲染（红绿蓝+高亮）、框选矩形、测量标注、状态栏、Toast 提示
- `BlockoutEditorUI.h/.cpp`：纯 Slate 中文 UI（i18n 预留 BlockoutUI::T）
  - 顶部工具栏：17 种体块创建下拉、变换模式、吸附开关、撤销重做、保存、导出菜单
  - 左侧属性面板：名称、位置、颜色 RGB、纹理平铺、7 项形状参数、测量按钮（尺寸/面积/体积）
  - 右键菜单：复制/删除/分组/取消分组
  - 底部快捷键提示条
- `BlockoutGameMode.h/.cpp`：GameMode + 无碰撞相机 Pawn
- `Config/DefaultInput.ini`：轴映射（MoveForward/Right/Up/MouseX/Y/Wheel）
- `Config/DefaultEngine.ini`：默认 GameMode 指向 BlockoutGameMode
- Build.cs 增加 DesktopPlatform（导出/保存文件对话框）

## 按键冲突处理约定

- W/E/R 空闲=变换模式；右键按住时 WASDQE=飞行移动
- Ctrl 修饰组合键（Z/Y/D/A）与单键功能隔离

## P2 已完成（2026-10-17）

- `BlockoutMeshEditLib.h/.cpp`（M3 点线面编辑）：
  - MoveVertices（选点移动）/ ExtrudeFaces（面挤出，自动补侧壁）/ BevelVertices（顶点倒角，按邻接三角形重建）/ CollapseVertices（多点塌陷至质心）/ DeleteFaces（删面）
  - PickVertex / PickFace 射线拾取（射线-线段距离、射线-三角形 Möller–Trumbore）
  - CommitMesh 统一回写 CachedMesh 与 FBlockoutObjectData（bFreeform/Vertices/Triangles），编辑结果可随 JSON 存档持久化
- `BlockoutRecordLib.h/.cpp`（R1-R3 录制）：
  - R1 录制：StartRecording/StopRecording/AddFrame/AddJpegFrame（30fps 相机轨迹 + 视口帧）
  - R2 视频导出：WriteMjpegAVI 自研 RIFF 容器（avih/strl/strh/strf(MJPG)/movi/idx1），零第三方依赖；SaveTrajectoryJSON/LoadTrajectoryJSON 轨迹 JSON 存取
  - R3 回放：SamplePlayback 时间插值（位置 Lerp + 旋转 Slerp）
- `BlockoutEditorPlayerController` 接入：
  - F9 录制开关（停止自动保存轨迹 JSON + MJPEG-AVI 至 Saved/Records）；录制逐帧采集（30fps 节流），视口截帧 JPEG Q80、缩至 1280 宽
  - 轨迹回放驱动 Pawn（超时自动停）
  - F8 子编辑模式循环（物体→顶点→面）；顶点/面拾取（Ctrl 加选）
  - X 面挤出 / C 顶点倒角 / V 顶点塌陷 / Del 面模式下删面（各操作前 PushUndo 快照）
- P2 编译修复：UE5.6 `GetCompressed(80)` 直接返回 TArray64；`ERGBFormat` 无 RGB 值（FColor 即 BGRA 直喂）；`FSceneViewport` 需 include Slate/SceneViewport.h；邻接查询用 `GetTriNeighbourTris`（GetEdgeTriangles 不存在）；三角形遍历用 MaxTriangleID+IsTriangle（GetAllTriangleIDs 不存在）
- 结果：BlockoutStudioEditor Win64 Development 编译通过

## P3 已完成（2026-10-17）

- `tools/create_materials.py`（材质资产生成，UnrealEditor-Cmd + PythonScriptPlugin 驱动）：
  - 生成 `/Game/BlockoutStudio/Materials/M_BlockoutBase`（材质包落盘 Content/BlockoutStudio/Materials/）
  - 参数与 `ABlockoutActor::EnsureMID` 对齐：BaseColor(Vector)/Tiling(Scalar)/Pattern(TextureSampleParameter2D)/PatternStrength(Scalar)/Highlight(Scalar)/HighlightColor(Vector)/Roughness/Metallic
  - 节点接线：TexCoord×Tiling→Pattern.UV；BaseColor×PatternRGB 按 PatternStrength Lerp→BaseColor；HighlightColor×Highlight→Emissive
  - 运行方式：`UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<脚本>`（-run=pythonscript 必须走 Cmd；资产根路径必须 /Game/）
- A1-A4 材质外观接入（属性面板）：
  - A1 用途分色预设：地面/墙体/障碍/可交互/装饰/危险 一键切换（UE 白盒灰阶+强调色，联动 SetColor+SetTexturePattern）
  - A2 基础贴图：纯色/棋盘格/ProBuilder 网格/砖墙/混凝土/瓷砖/金属板 按钮组（BlockoutTextureLib 程序化纹理），平铺密度可调
  - A3 自定义颜色：色板快捷色块 + RGB 数值输入，颜色与贴图可叠加（Lerp 混合）
  - A4 视图模式：无光照/光照/线框（r.ShowFlag.Lighting / r.ShowFlag.Wireframe）
- 外观接线：BlockoutActor EnsureMID 优先加载 M_BlockoutBase，缺失 fallback BasicShapeMaterial
- 结果：BlockoutStudioEditor Win64 Development 编译通过

## P4 已完成（2026-10-17）

- T1 两点测距（M 键切换模式）：
  - 点击取第一点，实时 HUD 青色虚线预览（距离+高差+仰角综合标注），第二击直接生成标注，Enter 放置
  - PickWorldPoint 射线拾取（Actor 包围盒 slab 求交 → 地面 Z=0 兜底）
  - HUD DrawMeasureItems 新增 MeasurePreview 实时预览绘制
- T2 俯视 2D 出图：BlockoutTopDownLib 导出 PNG（位图）/ SVG（矢量+原生文字标注），选项含尺寸标注/网格/名称（FBlockoutTopDownOptions）
- T3 比例尺参考体：SpawnScaleReference 0=1m 立方 / 1=2m 立方 / 2=1.8m 人形（头/躯干/双腿拼盒），走 bFreeform 自由网格、灰色锁定
- D3 导出分组与 FBX 缩放：
  - ExportOBJ/ExportFBX/ExportGLTF 新增 bGroupByMaterial，MergeByMaterial 按颜色合并网格（Material_RRGGBB 命名）
  - FBX 顶点 x0.01（cm→m 适配 Unity 米制）；FBX LclTranslation / glTF node translation 置零，修复顶点已含世界变换时的双重平移
  - 导出菜单新增「按材质分组（否则按物体）」复选框
- D4 导入参考体：BlockoutImportLib 导入 OBJ/glTF/GLB（glTF 米制 Y-up 自动转 uu Z-up x100），SpawnReferenceMesh 生成灰色锁定自由网格参考体
- D1 快照 20 版本：AutoSave 快照后 PruneSnapshots(20) 滚动清理，GetSnapshotFiles（新→旧）/RestoreSnapshot 回滚
- UI：工具栏新增「工具」菜单（测距/参考体/导入/俯视出图/快照），导出菜单新增分组复选框
- P4 编译修复：FLinearColor 无 Cyan 静态成员；FJsonObject::TryGetStringField 第二参为 FString&；测试入口 OnKeyToggleMeasure 移至 public
- 结果：BlockoutStudioEditor Win64 Development 编译通过

## P5 已完成（2026-10-17）

- 验收范围：三测试关卡（类魂/类 CS/类神海）鸟瞰图 + 跑图视频 v2 修复版，逐项验收清单与结论见 `ACCEPTANCE.md`
- 鸟瞰图：三关卡各 3 张（aerial_1/2/3.png），同关卡 3 张视角互异，验收通过
- 跑图视频 v2：三关卡抽帧（f0/f1/f2）验证，画面无遮挡无穿模、亮度正常、相机明显移动，验收通过
  - level_cs_dustyard 场景开阔导致帧间透视变化较小，但像素差 14-27%（pct_changed）确认相机在移动，判定通过
- 配套源码修复：BlockoutEditorPlayerController / BlockoutEditorSubsystem 跑图演示与鸟瞰截图 v2 修复（详见踩坑记录）

## 踩坑记录

- **Shipping 包 GameViewport::ReadPixels 读回全零致跑图视频黑屏**：Shipping 配置下视口像素读回恒为全零，录出的视频整段黑屏。
  - 原因：GameViewport::ReadPixels 在 Shipping 包不可用（渲染读回被裁剪/优化）。
  - 解决：改用 USceneCaptureComponent2D + UTextureRenderTarget2D 截帧；固定 AutoExposureMinBrightness / AutoExposureMaxBrightness = 1.0 并设 AutoExposureBias = 2.5 提亮画面；关卡加载后动态 Spawn DirectionalLight（强度 8）+ SkyLight（强度 1.2）补光。
- **鸟瞰机位判据 AutoDemoTimer<0.1f 在低帧率下失效致视角重复**：低帧率下单帧时间增量超过判据窗口，按计时器触发机位切换会漏拍/重复，3 张鸟瞰图视角不互异。
  - 原因：基于浮点计时器的单帧判据依赖帧率，低帧率下判据被跳过。
  - 解决：改用 int32 AutoDemoPlacedShot 标记法，按已拍摄张数无条件拍满 N 个机位，与帧率解耦。
- **跑图视频相机被 Pawn mesh 遮挡穿模**：默认相机位于 Pawn 内部/贴身，画面被自身网格遮挡甚至穿模。
  - 解决：改第三人称跟随相机（后方 600、Z+260）；跑图阶段隐藏 Pawn 网格并关闭碰撞。
*（内容由AI生成，仅供参考）*

## 2026-10-18 批次：演示品质修复 + 功能补齐 + 交互方案切换

- ① 演示品质：M_BlockoutBase 世界对齐三平面 UV + 顶面提亮（防面片感）；BlockoutTextureLib 手动 mip 链 + TMGS_NoMipmaps + TEXTUREGROUP_World（修闪烁）
- ② 跑图角色：TestPawn 挂 SKM_Manny_Simple + ABP_Unarmed；AutoDemo 移除隐藏角色逻辑（可见跑动）
- ③ 功能比对：产出 FEATURE_COMPARISON.md（与 BlockoutTools 插件、Blockout Toolkit 逐功能对照）；M2 手柄拖拽改尺寸 / M4 Auto-Align 开洞 / E10 场景树全部落地
- ④ 材质/贴图质量：Toolkit 贴图资产复用（Content/PMB/BlockoutToolKit），程序材质参数对齐 Grid_Multi_Color
- ⑤ 交互方案：EBlockoutInteractionMode（Toolkit 式 / ProBuilder 式）UI 菜单一键切换，手柄可视化/拾取与 Tab 子编辑循环按模式分流
- 编译：BlockoutStudioEditor Win64 Development 通过；源码 UTF-8 BOM + CRLF 已核对


## 2026-10-18 批次：UI 收尾 + 文档 + HANDOVER 交接包

- ① 左侧创建面板改造：新增 SBlockoutShapePreview（Slate 程序绘制形状缩略图折线几何），BuildCreatePanel 改为缩略图网格卡片（88x54 预览 + 名称 + Tooltip，每行 3 列），6 基本体笔刷 + 12 建筑预制件每项均有直观图形
- ② 场景树增强：多选（Ctrl/Shift）、拖拽排序（ReorderBlock）、拖拽编组（AssignGroupToActor，Onto 目标行）、双击行内重命名（SInlineEditableTextBlock）、形状类型迷你图标、可见性/锁定行内开关、搜索过滤（名称+类型）、右键菜单全套；视口↔树选择双向同步 + 0.5s 低频轮询兜底
- ③ docs/USER_MANUAL.md 详细使用手册（安装运行/界面布局/全部操作/快捷键表/双模式切换/导出导入/录制/FAQ）
- ④ HANDOVER/ AI 交接包：README + 01 项目背景与需求要点 + 02 架构与模块清单 + 03 踩坑全集 + 04 已完成/未完成与遗留项 + 05 验收标准 + 06 工作流程 + 07 skill 清单
- ⑤ 根 README.md 补齐；全部文档与资产 commit+push
- UE 5.6 Slate 经验：OnPaint 第3参数 const FSlateRect&；FDragDropOperation 在 Input/DragAndDrop.h；MakeBox 需 FSlateBrush*（取局部对象地址）；MakeLines 用 TArray<FVector2f> 值传；拖拽接收委托挂 STableRow 非 SListView
