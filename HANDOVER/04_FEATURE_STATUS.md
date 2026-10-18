# 04 已完成 / 未完成功能清单与遗留项

> 状态基准：`REQUIREMENT_CHECK.md`（2026-10-18 核对）+ 本交接轮 UI 收尾（commit 40784e7）。

## 1. 已完成功能（全部需求编号）

### 1.1 需求规格全部编号：E1–E11 / M1–M5 / T1–T3 / A1–A4 / P1–P5 / R1–R3 / D1–D5 均为「达标」

详细逐条证据见 `REQUIREMENT_CHECK.md` §1/§2。摘要：

- **物件编辑**：多选/框选、gizmo 移转缩、世界/本地坐标系(B)、pivot 模式、网格吸附 6 档、表面吸附、Ctrl+D 复制/阵列/镜像/8 种对齐、撤销 150 步、隐藏/锁定/分组、场景树、属性面板双向同步；
- **建模**：18 种参数化体块、6 面手柄拖拽改尺寸、点线面全套（推拉/挤出/倒角/塌陷/焊接/内插/细分/删面/翻法线）、三类挖洞（Auto-Align 构件开洞/自动连通/自定义多边形轮廓）、门洞墙/柱廊/围墙/楼梯间快速生成；
- **测量出图**：两点测距+标注(M)、2D topdown PNG+SVG、1m/2m/1.8m 比例尺；
- **材质**：6 分色预设、6 贴图样式（Grid 复用 PMB T_Grid 与观感一致）+Tiling、颜色+贴图叠加、Lit/Unlit/Wire 视图；
- **测试模式**：官方小白人 Enhanced Input（WASD/Shift 跑/Ctrl 蹲/空格跳）、V 切 FPS/TPS、B 切角色、1/2/3 三套尺度预设、跳高/跳距统计；**碰撞双关卡实测回归通过**（souls_castle 8916 帧 overlap=0 penetration=0；uncharted_ruins 9589 帧同）；
- **录制**：轨迹录制回放；视频 AVI + ffmpeg 转封装 **MP4(H.264)/WebM(VP8) 双成品**（R2 已从"部分达标"补齐为达标）；
- **数据**：自动保存+快照回滚+崩溃恢复、JSON 工程、OBJ/FBX/glTF/GLB/USD 导出、OBJ/glTF 导入；
- **性能**：1000 体块 avgFps **193.82**（1s 最低窗口 187.70），远超 60fps 目标（output\d5_perf\perf_report.json）；
- **UI 收尾（本交接轮新增）**：
  - 左侧创建面板 → **带缩略图的网格卡片**（`SBlockoutShapePreview` Slate 程序绘制几何预览，6 基本体+12 建筑预制件每项有直观图形，每行 3 列 + Tooltip）；
  - 场景树增强 → **多选（Ctrl/Shift）、拖拽排序（Above/Below）、拖拽编组（Onto）、双击行内重命名、形状类型迷你图标、可见性/锁定独立开关、搜索过滤（名称+类型）、右键菜单（重命名/复制/删除/隐藏/显示/锁定/解锁/编组/解组/全选/清空）**、视口与树选择双向同步 + 0.5s 轮询兜底；
  - `docs/USER_MANUAL.md` 详细使用说明；
  - `HANDOVER/` 本交接包。

### 1.2 实测/验收证据位置

| 证据 | 位置 |
|---|---|
| 碰撞回归 JSON | 会话 output\p3_collision\（双关卡 pass=true） |
| 性能报告 | 会话 output\d5_perf\perf_report.json |
| 导出样例 | 会话 output\d3_export\（四格式） |
| UE 导入实测 | 会话 output\d3_ue_import\（bounds 一致 [450.02,400.02,200.02]cm） |
| 演示视频/截图 | 会话 output\r2_video\ 等；关卡 demoPath 轨迹 |
| 需求逐条核对 | E:\BlockoutStudio\REQUIREMENT_CHECK.md |
| 验收勾选 | E:\BlockoutStudio\ACCEPTANCE.md |

## 2. 未完成 / 遗留项（如实列出，禁止静默砍）

| # | 项 | 现状 | 原因 | 替代方案/下一步 |
|---|---|---|---|---|
| 1 | **Unity 端导入实测补跑** | 未测（UE 端已测通过） | 本机 Unity 2021.3.8f1 Editor 许可证未激活，batchmode 报 "not been activated with a valid License"；不绕过安全验证 | 许可证激活后用 output\d3_export 样例（OBJ/FBX/glTF/GLB）在 Unity 2022+ 手动导入复验；导出端已按 Unity 规范（FBX Y-up/cm、glTF 米制）修正，预期无误 |
| 2 | **dust2 演示收尾** | 暂缓 | 当时优先级让位于功能补齐（R2/D3/D5/P3） | 运行 `-AutoDemoLevel=Content\Levels\level_dust2_final.json -AutoDemoOut=<dir>` 补录；已有 dustyard/souls_castle/uncharted_ruins 演示可覆盖验收 |
| 3 | **docs/screenshots 截图留档** | 缺失/不全 | 流程性遗留 | 需求 §0.2 要求每阶段截图存 docs\screenshots\；可在验收复跑时用 AutoDemo 鸟瞰截图补齐 |
| 4 | GitHub PAT 轮换 | 未做 | token 在需求原件明文出现，属用户侧操作 | 提醒用户 GitHub Settings → Developer settings 撤销更换 |
| 5 | i18n 完整英文版 | 仅预留结构 | 需求要求"中文为主，预留 i18n"，已满足 | 如需英文 UI 再扩展字符串表 |
| 6 | 神海"攀爬钩锁"深度手感 | 基础攀爬预设已实现 | 需求原文允许"可后置" | 按需增强 |

## 3. 建议下一步工作（优先级序）

1. Unity 许可证激活后补跑 D3 导入实测并更新 REQUIREMENT_CHECK.md；
2. 补齐 docs\screenshots\（各功能截图）与 dust2 演示视频；
3. 按 05_ACCEPTANCE.md 在目标机器复跑一遍验收清单并签字；
4. 提醒用户轮换 GitHub token；
5. （可选）打包一次最终 Shipping 便携 zip 发布。
