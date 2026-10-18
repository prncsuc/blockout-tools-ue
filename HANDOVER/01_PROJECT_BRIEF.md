---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 5227bc309b64902b9bfd2bb79d4de347_06b48c57cad211f18b7f525400393706
    ReservedCode1: GWcT+tcQ4KVFDOXqhQTKsiEBpFQtBxkb5fRhzPmoOxmTEu6x8jd97ozUqwmhtYuFsqY8wa1N359LVZVhDAqLv9QxVvFG4kHYNKRUtMi3HB0SrW79tVmvtjcFHUHjUOXtxpx+ryok+uaAXILagG94DxqlXUebNnidJ0CLUdfQDRIj+SAh8b/Q7v/Fmy0=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 5227bc309b64902b9bfd2bb79d4de347_06b48c57cad211f18b7f525400393706
    ReservedCode2: GWcT+tcQ4KVFDOXqhQTKsiEBpFQtBxkb5fRhzPmoOxmTEu6x8jd97ozUqwmhtYuFsqY8wa1N359LVZVhDAqLv9QxVvFG4kHYNKRUtMi3HB0SrW79tVmvtjcFHUHjUOXtxpx+ryok+uaAXILagG94DxqlXUebNnidJ0CLUdfQDRIj+SAh8b/Q7v/Fmy0=
---

# 01 项目背景与需求规格要点

## 1. 背景与目标

- **用户**：游戏客户端开发者（Unity 为主，二次元大世界 ARPG），正在学习关卡策划方向；用户是"甩手掌柜"模式——下达需求后不回答问题，AI 需连续自主执行到底；
- **目标**：用 UE5 制作一个**独立打包的白盒关卡编辑器程序**（Windows exe，**不是 UE 编辑器插件**），关卡设计师双击 exe 即可搭建类 CS:GO / 类魂 / 类神海游戏的白盒关卡；
- **对标**：操作便捷度**不低于 UE 的 PMB – Blockout ToolKit**（功能对标 PMB，点线面对标 Unity ProBuilder）；
- **交付**：Win64 Shipping 便携版（免安装 zip）、导出 FBX/OBJ/glTF 供 Unity/UE 读取；
- **成功标准一句话**：一名关卡设计师拿到 exe，10 分钟内不看教程搭出一个带门洞、楼梯、跳跃点的 CS 风格小图，进 FPS 模式跑一圈，导出 FBX 丢进 Unity 尺寸正确。

## 2. 需求规格要点

> 完整需求原件：《UE5白盒关卡工具-需求规格与开发交接文 v1.1》（364 行，**唯一完整需求来源**）。该文档为仓库外资料，**不在本仓库内——请向用户索要**，或向用户确认可查阅的历史会话/知识库。本节只摘要点，冲突时以原件 §4 功能清单为准。

### 2.1 功能编号体系（开发与验收均按编号对照）

| 前缀 | 域 | 编号 | 要点 |
|---|---|---|---|
| E | 物件编辑 | E1–E11 | 选择/框选/多选(Ctrl)、gizmo 移转缩(W/E/R)、**世界/本地坐标系(B)**、pivot、网格吸附(1/5/10/25/50/100)、表面吸附贴合、复制/阵列/镜像/对齐、撤销≥100(实为150)、隐藏/锁定/分组、场景树、属性面板双向同步 |
| M | 建模 | M1–M5 | 参数化体块库≥15(实为18)、PMB 式手柄拖拽改尺寸、点线面编辑(推拉/挤出/倒角/塌陷/删除)、三类挖洞(构件开洞/自动连通/自定义轮廓)、快速生成(门洞墙/柱廊/围墙阵列/楼梯间) |
| T | 测量出图 | T1–T3 | 两点测距+标注、2D topdown 图 PNG+SVG、1m/2m/1.8m 人形比例尺 |
| A | 材质外观 | A1–A4 | 分色预设(地面/墙/障碍/交互/装饰/危险)、贴图(ProBuilder 网格风格必须有)+平铺可调、自定义颜色+叠加、线框/光照/无光照切换 |
| P | 测试模式 | P1–P5 | 一键进/出(F5，编辑状态保留)、FPS/TPS 双视角、走/跑/蹲/跳+碰撞正确、CS/魂/神海三套尺度预设、跳跃高度/距离测量 |
| R | 录制 | R1–R3 | 轨迹录制回放、视频录制出文件(MP4/WebM)、快捷开始停止+状态可见 |
| D | 数据可靠性 | D1–D5 | 自动保存+快照≥20+崩溃恢复、手动保存/另存 JSON、导出 OBJ/FBX/glTF 且 Unity+UE 双端导入比例正确、导入 OBJ/glTF 参考体、1000 体块 60fps |

### 2.2 用户原话需求清单（必须全部覆盖）

1. 便于制作类 CS:GO、类魂、类神海的关卡白盒；2. TPS/FPS 角色测试；3. 录制功能；4. 导出 OBJ/FBX 等；5. 自动保存；6. PMB 有的功能都要有且便捷度不差；7. 参数改尺寸 + 推拉改尺寸 + 点线面编辑；8. 快速量尺寸；9. 自定义挖洞；10. 2D topdown 图；11. 坐标系操作位移旋转缩放；12. 位移吸附贴合；13. 基础贴图 + 颜色 + ProBuilder 风格网格贴图。

### 2.3 用户红线（负面清单，§10）

1. **坐标系错误不可接受**（web 版因此被否，UE 左手系 Z-up，世界/本地变换数学必须严格正确）；
2. **基本物件编辑必须扎实**（E1–E11 是底线）；
3. **必须独立打包程序**，不是插件、不是网页；
4. 操作便捷度不低于 PMB，做不到的列明并给替代交互；
5. **禁止静默砍功能**：无法实现的写 `UNIMPLEMENTED.md`（功能名/原因/替代方案），继续后续工作。

### 2.4 产品形态

- 独立 Windows 桌面程序（UE5 打包，Win64，DX12/SM5，绿色便携 zip）；
- 双模式一体一键切换：**编辑模式**（viewport+编辑 UI）/ **测试模式**（FPS/TPS 跑图）；
- UI 中文为主，预留 i18n。

## 3. 关键参考

| 参考 | 说明 |
|---|---|
| PMB – Blockout ToolKit | UE 资产（Blueprints，依赖 Geometry Script 插件）；三大核心：Auto-Align Boolean、Dynamic Mesh Actor、Handle Controls。开发前需自行获取正版（Epic Marketplace 市场获取或向用户索要），已随仓库存放于 `Content\PMB\BlockoutToolKit`（网格贴图 T_Grid 已被本工具复用）。许可注意：可合入打包，禁止单独再分发资产文件 |
| Unity ProBuilder | 点线面子层级编辑交互参考 |
| layout tools (KluiYao) | 网页版关卡交互思路 |
| 旧 web 版原型 `prncsuc/blockout-tools` | **交互被用户否定，禁止照搬**；功能清单可参考（12 种体块/分色/挖洞/跑图/录屏/JSON/GLB/OBJ/FBX 导出等） |

## 4. 需求原件 §0 默认决策（摘录）

- 仓库 `github.com/prncsuc/blockout-tools-ue`，main 分支；提交信息建议 Conventional Commits；
- 导出单位：1uu=1cm，FBX 面向 Unity 按规范 Y-up/cm（Unity 自动换算）；
- 视频录制优先免许可风险编码（WebM/VP8 或 ffmpeg LGPL 动态链接）；
- 示例内容：3 个示例白盒（类 CS 竞技小图、类魂地牢、类神海攀爬遗迹）各含跑图录屏/轨迹；
- 每阶段收尾：自查 → git commit → git push → 更新 `PROGRESS.md`。
*（内容由AI生成，仅供参考）*
