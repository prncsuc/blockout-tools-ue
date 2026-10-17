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
| P3 | 材质资产与外观 | 未开始 |
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

- 引擎：F:\UE_5.6（UE 5.6.1）
- 工具链：VS 2022 BuildTools 17.14（VCTools workload）
- 项目：E:\BlockoutStudio
- 网络：github 访问经本地代理 127.0.0.1:26001（git http.sslBackend=schannel）

## 待完成

- [x] 编译验证 P0 代码（2026-08-21 通过）
- [x] BlockoutGizmo、PlayerController/HUD/输入、Slate UI、GameMode（2026-10-17 P1 通过）
- [x] 录制 R1-R3、M3 点线面子编辑（2026-10-17 P2 通过）
- [ ] 材质生成脚本与资产
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
*（内容由AI生成，仅供参考）*
