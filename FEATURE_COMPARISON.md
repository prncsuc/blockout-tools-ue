# BlockoutStudio 功能对照表（代码级逐功能比对）

比对对象：
- **本项目**：E:\BlockoutStudio（Source\BlockoutStudio，UE5.6 白盒关卡工具）
- **参考插件 A**：F:\UE_5.6\Engine\Plugins\Marketplace\Blockoutd2a06d30e0e2V13（Blockout Tools，C++ Actor 式）
- **参考插件 B**：E:\tianfynx_blockout\Content\Blockout_Toolkit（Blockout Toolkit，蓝图资产包）
- **需求规格**：UE5 白盒关卡工具-需求规格与开发交接文档（功能编号 E1-E11 / M1-M5 / T1-T3 / A1-A4 / P1-P5 / R1-R3 / D1-D5）

图例：✅ 已实现（代码可查） / 🔧 本批补齐 / ♻️ 直接复用参考资源 / ⚠️ 部分实现

## 1. Blockout Tools 插件（A）逐功能比对

| 功能 | A 实现位置 | 本项目对应 | 状态 |
|---|---|---|---|
| 参数化体块（盒/柱/球/楼梯/斜坡等） | BlockoutToolsParent.h/.cpp 模板 Actor | BlockoutMeshGen.BuildShape（18 种 EBlockoutShape） | ✅ |
| 体块运行时改参数重建 | BlockoutToolsParent::Rebuild | BlockoutActor::Rebuild（参数化 + bFreeform 双路径） | ✅ |
| 棋盘格/网格程序材质（可调亮度/粗糙度/颜色） | BlockoutToolsSettings.h（MaterialType/GridColor/GridSize/CheckerLuminance/Roughness/TopColor/WorldAligned） | M_BlockoutBase（BaseColor/Tiling/Pattern/PatternStrength/Highlight/TopColor/TopStrength/WorldAligned/Roughness/Metallic），create_materials.py 生成 | ✅ |
| 世界对齐 UV（防拉伸/闪烁） | WorldAligned | WorldAligned 三平面投影 UV（本批改造） | 🔧 |
| 顶面提亮（体积感） | TopColor | TopColor/TopStrength（本批改造） | 🔧 |
| 自定义材质替换 | CustomMaterial | 属性面板 SurfaceType + SetVector/SetScalar/SetTexture 参数注入 | ✅ |
| 碰撞配置 | CollisionProfile | BlockoutActor 碰撞按 Shape 生成（BlockoutMeshGen 统一） | ✅ |
| 投射阴影开关 | CastShadows | Actor SetCastShadow / 属性面板 | ✅ |
| 网格吸附放置 | GridSize 吸附 | bGridSnap + GridSize（E1） | ✅ |
| 撤销/重做 | 编辑器事务 | PushUndo/Undo/Redo（Z/Y） | ✅ |
| 场景保存/加载 | .uasset | 关卡 JSON（version/savedAt/blocks） | ✅ |
| 导出 OBJ/STL | 无 | BlockoutExportLib（OBJ/STL/glTF/DAE） | ✅ 超出 |
| 俯视图出图 | 无 | BlockoutTopDownLib（PNG/SVG） | ✅ 超出 |
| 测距/比例尺 | 无 | BlockoutMeasureLib + T3 参考体 | ✅ 超出 |
| 录制回放 | 无 | BlockoutRecordLib（R1-R3） | ✅ 超出 |

## 2. Blockout Toolkit（B）逐功能比对

| 功能 | B 实现 | 本项目对应 | 状态 |
|---|---|---|---|
| 预制件库（网格模块拼装） | Master_Blockout_Mesh_BP + 模块化网格 | 参数化体块 + 门/窗/拱/柱廊等构件（DoorPiece/WindowPiece/Arch） | ✅ |
| 多色网格贴图（Grid_1~7_T） | 7 套贴图 + Grid_Multi_Color 材质 | Pattern=Grid/Brick/Concrete/Tile/MetalPlate 程序生成 + **复用 Grid 贴图资产**（Content\PMB\BlockoutToolKit） | ✅ / ♻️ |
| 预制件材质质量 | 手工调校 PBR | M_BlockoutBase 世界对齐 + 顶面提亮；**质量不足场景直接引用 Toolkit 资产** | ♻️ |
| 手柄拖拽改尺寸（Handle Controls） | 蓝图手柄 | **M2：6 面手柄拾取 + BeginResizeDrag/UpdateResizeDrag/EndResizeDrag + HUD 可视化** | 🔧 |
| Auto-Align 自动开洞（门窗拖墙开洞） | 蓝图布尔 | **M4：TryAutoAlignPiece/CutHoleIntoWall/CutCustomHole（BooleanOp 减法）** | 🔧 |
| 场景层级树 | 蓝图列表 | **E10：SBlockoutEditorUI::BuildSceneTreePanel（SListView + 选中联动 + 数量变化刷新）** | 🔧 |
| 拖拽/旋转/缩放 gizmo | 蓝图 gizmo | ABlockoutGizmo + 三模式（W/E/R） | ✅ |
| 测试模式跑动 | 蓝图角色 | F5 TestMode + **Manny/Quinn 官方小白人（SKM_Manny_Simple + ABP_Unarmed）可见跑动** | 🔧 |

## 3. 交互方案（用户要求 4）

| 方案 | 行为 | 切换入口 |
|---|---|---|
| Blockout Toolkit 式 | 单击放置构件、M2 手柄拖拽改尺寸（手柄可视化开启） | UI 工具菜单「切换交互方案」 |
| ProBuilder 式 | 点线面编辑优先（Tab 顶点/面循环、挤出/内插/细分/焊接/翻转法线/补洞），手柄隐藏 | 同上，一键互切 |

实现：EBlockoutInteractionMode（BlockoutTypes.h）+ UBlockoutEditorSubsystem::Set/GetInteractionMode；HUD 手柄绘制、PC 手柄拾取、Tab 子编辑循环均按模式分流。

## 4. 演示品质修复（用户要求 1/2）

| 问题 | 根因 | 修复 |
|---|---|---|
| 体块像面片 | 材质单平面 UV、无顶面着色差异 | WorldAligned 三平面投影 + TopColor/TopStrength 顶面提亮（create_materials.py 重生成 M_BlockoutBase） |
| 贴图闪烁 | CreateTransient 单 mip、1px 细线远距摩尔纹 | BlockoutTextureLib 手动生成 mip 链 + TMGS_NoMipmaps + TEXTUREGROUP_World |
| 跑图没角色 | AutoDemo Phase2 隐藏 TestPawn | 移除 SetActorHiddenInGame(true)，挂 SKM_Manny_Simple + ABP_Unarmed + BS_Idle_Walk_Run |

## 5. 未实现 / 后续批次

- dust2 关卡 + 演示视频（用户第 6 点，留后续批）
- 视频输出 MP4/WebM 转封装（当前 AVI MJPEG，属后处理，不影响功能）
