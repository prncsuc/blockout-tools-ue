# BlockoutStudio — UE5 独立白盒关卡工具

独立打包的 Windows 白盒关卡编辑器（非 UE 插件）：关卡设计师双击 exe 即可搭建类 CS:GO / 类魂 / 类神海的白盒关卡，支持 FPS/TPS 跑图测试、录制、导出 FBX/OBJ/glTF 进 Unity/UE。基于 UE 5.6.1，操作便捷度对标 PMB – Blockout ToolKit。

## 功能亮点

- **参数化体块库**：18 种形状（基本体 + 墙/拱门/门洞墙/窗洞墙/柱廊/围墙/楼梯间等建筑构件），带缩略图卡片一键放置，参数与手柄拖拽实时改尺寸
- **专业编辑**：变换 gizmo（W/E/R）、世界/本地坐标系（B）、网格吸附（6 档步长）、表面吸附贴合、复制/阵列/镜像/8 种对齐、分组、撤销重做 150 步
- **建模**：点线面推拉/挤出/倒角/塌陷/焊接/细分，三类挖洞（门窗拖墙自动开洞、走廊自动连通、自定义多边形轮廓）
- **场景树**：多选、拖拽排序/编组、双击重命名、类型图标、可见性/锁定开关、搜索过滤、右键菜单
- **测试跑图**：UE 官方小白人（Manny/Quinn）FPS/TPS 双视角，走/跑/蹲/跳，CS/魂/神海三套尺度预设（1/2/3），跳跃高度/距离测量
- **录制**：轨迹录制回放 + 视频录制（AVI + MP4/WebM 双成品）
- **出图**：两点测距标注（M）、2D 俯视图 PNG/SVG、1m/2m/1.8m 人形比例尺
- **可靠**：自动保存、快照回滚（≥20）、崩溃恢复、1000 体块 193fps 实测

## 快速开始

```bat
:: 直接运行便携版
Packaged\Windows\BlockoutStudio.exe

:: 或从源码编译（UE 5.6.1）
F:\UE_5.6\Engine\Build\BatchFiles\Build.bat BlockoutStudioEditor Win64 Development -project="E:\BlockoutStudio\BlockoutStudio.uproject" -waitmutex
```

## 文档

| 文档 | 说明 |
|---|---|
| [docs/USER_MANUAL.md](docs/USER_MANUAL.md) | 完整使用手册（安装、界面、全部操作步骤、快捷键表、FAQ） |
| [REQUIREMENT_CHECK.md](REQUIREMENT_CHECK.md) | 需求规格逐条核对结论 |
| [ACCEPTANCE.md](ACCEPTANCE.md) | 验收清单实测结果 |
| [FEATURE_COMPARISON.md](FEATURE_COMPARISON.md) | 与 PMB / ProBuilder 功能对比 |
| [PROGRESS.md](PROGRESS.md) | 开发进度与关键决策 |
| [HANDOVER/](HANDOVER/README.md) | AI 开发交接包（背景/架构/踩坑/状态/验收/流程/skill 清单） |

## 环境

- Windows 10/11 x64，UE 5.6.1，VS 2022
- 导出单位：1uu = 1cm（FBX/glTF 已按 Y-up 规范输出，Unity/UE 自动换算）
- 视频转封装使用 ffmpeg（LGPL 动态链接，随包分发）

## 许可说明

- 本工具自研代码随仓库发布；`Content\PMB\` 为 PMB – Blockout ToolKit 正版资产（Epic Marketplace 标准许可，不得单独再分发）；`Content\Characters\` 等为 Epic 官方示例资产。
- ffmpeg 以 LGPL 动态链接方式分发（`ThirdParty\ffmpeg\`）。
