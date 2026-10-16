# BlockoutStudio 开发进度

UE5 独立白盒关卡工具（Windows 桌面程序），按《UE5 白盒关卡工具-需求规格与开发交接文 v1.1》执行。

## 阶段进度

| 阶段 | 内容 | 状态 |
|---|---|---|
| P0 | 项目骨架与核心源码 | 代码完成，待编译验证 |
| P1 | 编辑器交互（gizmo/输入/UI/GameMode） | 未开始 |
| P2 | 子编辑与录制 | 未开始 |
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

- [ ] 编译验证 P0 代码
- [ ] BlockoutGizmo、PlayerController/HUD/输入、Slate UI、GameMode
- [ ] 录制 R1-R3、M3 点线面子编辑
- [ ] 材质生成脚本与资产
- [ ] 打包（P6）
