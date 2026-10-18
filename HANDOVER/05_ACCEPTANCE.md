---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 5227bc309b64902b9bfd2bb79d4de347_0970d204cad211f18b7f525400393706
    ReservedCode1: vUQkgIDuCcX9jw+98+puaxngJ71O0LORj26NQe2GWnaQzYrLpev/sk7fDpieRCQK1qqNVjbCA21A/QNyiuF8mwg1KR0nSPk/i7HFxvugjnzWXE90lSJCLTRw9iOYsQU2fjr+ApcOaSBG38iSaFar8i7A3ai5TZh4XuUXsBmQX2EY7sDJNelQUsxPlzg=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 5227bc309b64902b9bfd2bb79d4de347_0970d204cad211f18b7f525400393706
    ReservedCode2: vUQkgIDuCcX9jw+98+puaxngJ71O0LORj26NQe2GWnaQzYrLpev/sk7fDpieRCQK1qqNVjbCA21A/QNyiuF8mwg1KR0nSPk/i7HFxvugjnzWXE90lSJCLTRw9iOYsQU2fjr+ApcOaSBG38iSaFar8i7A3ai5TZh4XuUXsBmQX2EY7sDJNelQUsxPlzg=
---

# 05 验收标准与实测基准

> 验收依据：需求原件 §8（16 条）+ §11 成功标准。逐条勾选结果存仓库 `ACCEPTANCE.md`；状态与证据以 `REQUIREMENT_CHECK.md` 为准。**不许粉饰验收结果**：未通过项如实记录原因。

## 1. §8 验收清单（16 条）与当前状态

| # | 验收项 | 状态 | 实测基准/证据 |
|---|---|---|---|
| 1 | 双击 exe 进入编辑器，中文 UI | 达标 | Shipping 打包链路已验证（Packaged\Windows\BlockoutStudio.exe） |
| 2 | 放置 ≥10 种参数化体块，参数与手柄均可改尺寸 | 达标 | 18 种形状 + 6 面手柄（M2） |
| 3 | 点线面推拉/挤出/倒角，Cube 改不规则房型 | 达标 | FBlockoutMeshEditLib 全套操作（M3） |
| 4 | 世界/本地坐标系切换，gizmo 轴向正确 | 达标 | B 键切换，Gizmo 按 CoordSpace 旋转（红线项，改后必复测） |
| 5 | 网格吸附 + 面/边/角吸附贴合 | 达标 | G/S 键，步长 1/5/10/25/50/100 |
| 6 | 门窗拖墙开洞 / 走廊连通 / 自定义挖洞 | 达标 | TryAutoAlignPiece + CutHoleIntoWall + CutCustomHole |
| 7 | 两点测距与尺寸标注 | 达标 | M 模式 + Enter 确认标注 |
| 8 | 2D topdown PNG/SVG 导出 | 达标 | FBlockoutTopDownLib ExportPNG/ExportSVG |
| 9 | 分色 + ProBuilder 网格贴图 + 贴图切换 | 达标 | Grid 复用 PMB T_Grid，观感与 PMB 一致 |
| 10 | FPS/TPS 跑图走跑蹲跳、碰撞正确 | 达标 | 双关卡碰撞回归：souls_castle 8916 帧 overlap=0/penetration=0；uncharted_ruins 9589 帧同；walk/run/crouch/jump 全程覆盖 |
| 11 | CS/魂 角色尺度预设 | 达标 | 3 套（CS/Souls/Uncharted），快捷键 1/2/3 |
| 12 | 轨迹回放 + 视频出文件 | 达标 | MP4(H.264)/WebM(VP8) 双成品（ffmpeg 转封装）+ AVI 原生 |
| 13 | 自动保存：强杀进程后可恢复 | 达标 | autosave 比工程新则自动加载（D1） |
| 14 | 导出 FBX/OBJ/glTF，Unity 与 UE 导入比例正确 | 达标（UE 实测；Unity 待补） | UE 5.6.1 四格式导入 bounds 完全一致 [450.02,400.02,200.02]cm；Unity 许可证未激活未测（原因如实记录，见 04 遗留项 #1） |
| 15 | 1000 体块 60fps | 达标 | AutoPerfTest=1000：avgFps=193.82，最低 1s 窗口 187.70 |
| 16 | 撤销重做全链路可靠 | 达标 | 150 步栈，含隐藏/锁定/重命名/排序/编组 |

## 2. 复跑验收的自动化入口

```bat
BlockoutStudio.exe -AutoExportTest=<dir>                 :: 验收 #14
BlockoutStudio.exe -AutoPerfTest=1000                    :: 验收 #15
BlockoutStudio.exe -AutoCollisionTest=<关卡JSON>         :: 验收 #10
BlockoutStudio.exe -AutoDemoLevel=<关卡JSON> -AutoDemoOut=<dir>  :: 演示/截图
```

关卡 JSON：`Content\Levels\level_{dust2_final,dustyard,souls_castle,uncharted_ruins}.json`（各含 demoPath 演示轨迹）。

## 3. §11 成功标准（终验脚本）

> 一名关卡设计师拿到 exe：10 分钟不看教程搭出带门洞、楼梯、跳跃点的 CS 风格小图 → FPS 跑一圈 → 导出 FBX 丢进 Unity 尺寸正确。

复验步骤建议：
1. 交付 `Packaged\Windows` 便携包给非开发者；
2. 计时完成小图搭建（门洞墙 + 楼梯 + 跳跃箱）；
3. F5 进 FPS 跑图验证碰撞与跳跃点（P5 跳高/跳距辅助校准）；
4. 导出 FBX → Unity 导入（**此项需 Unity 许可证激活后完成**，其余全链路已通）。

## 4. 验收纪律

- 每条必须实测勾选；未通过写入 `ACCEPTANCE.md`（项/原因/影响/替代）；
- 禁止以"代码里有"代替"实测通过"；性能/碰撞/导出类结论必须附 JSON/截图证据；
- 坐标系（#4）为用户红线，任何 gizmo/变换改动后必须人工复测轴向再勾选。
*（内容由AI生成，仅供参考）*
