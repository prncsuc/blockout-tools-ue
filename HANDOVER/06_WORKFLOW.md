---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 5227bc309b64902b9bfd2bb79d4de347_0a40d0d9cad211f18b7f525400393706
    ReservedCode1: 1c6qqRRyYIZkJLY2TEtiwwvjuXcsIlkFgKG3FF8ryPYcvoxBuX8krmwh3HMoyY3DdtzI1moTzlh4H4CLLRMUZDmFB0f3+hBHZ5fEBNgKVt65l6lTXuzhFr1zo8c73kstNzyCdbHTJzwvUg3+om8LnwVQM5CqfgFM0jq7WvvRu7Sw63pG6cVxA+nScF8=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 5227bc309b64902b9bfd2bb79d4de347_0a40d0d9cad211f18b7f525400393706
    ReservedCode2: 1c6qqRRyYIZkJLY2TEtiwwvjuXcsIlkFgKG3FF8ryPYcvoxBuX8krmwh3HMoyY3DdtzI1moTzlh4H4CLLRMUZDmFB0f3+hBHZ5fEBNgKVt65l6lTXuzhFr1zo8c73kstNzyCdbHTJzwvUg3+om8LnwVQM5CqfgFM0jq7WvvRu7Sw63pG6cVxA+nScF8=
---

# 06 工作流程硬性要求

> 多 AI 协同开发（Marvis 团队 + 接手 AI）的执行纪律。违反流程的产出视为不合格。

## 1. 执行节奏

1. **连续执行不中断**：用户是甩手掌柜，拿到任务连续执行到底，不等待用户确认（高风险破坏性操作除外）；
2. **每阶段收尾四件事**：自查 → `git commit` → `git push` → 更新 `PROGRESS.md`（阶段进度与关键决策）；
3. **commit 粒度**：每个功能块/修复块一次提交；提交信息中文或 Conventional Commits（`feat/fix/chore`）均可，历史上两种混用，保持可读即可；
4. **push 通道**：优先直连推送 `git -c http.proxy= -c https.proxy= push`（2026-10-18 实测直连成功）；无法直连时配置本地代理——**代理地址向用户索要**（详见 03_PITFALLS F1）。remote 含凭据时注意保护。

## 2. 产物与目录约定

| 位置 | 用途 |
|---|---|
| 工作区结果目录 `output\` | 最终交付产物（按功能分子目录：r2_video / d3_export / d5_perf / p3_collision 等；根目录以当前会话/环境约定为准） |
| 工作区中间目录 `temp\` | 中间产物（脚本、临时清单、manifest）——禁止写系统临时目录/桌面 |
| 仓库根（下称 `<REPO_ROOT>`） | 全部工程资产、文档、Packaged（LFS） |
| 仓库 `docs\screenshots\` | 验收/阶段截图留档 |
| 仓库 `HANDOVER\` | 交接文档（保持更新） |

- 分析/报告类结论尽量单文件整合；
- 对用户的交付文件需登记产物（declare_products + yyb-product 卡片），便于用户查看下载。

## 3. 诚实性红线（最高优先级）

1. **不许粉饰验收结果**：实测失败就写失败 + 原因 + 影响面；
2. **禁止静默砍功能**：无法实现的功能必须写入仓库 `UNIMPLEMENTED.md`（功能名/原因/替代方案），继续后续阶段，不得停下等用户；
3. 结论必须有代码/实测证据支撑（JSON 报告、截图、日志），禁止"代码里有=实现好"；
4. 外部信息标注来源。

## 4. 代码质量要求

1. C++ 源文件一律 **UTF-8 with BOM + CRLF**（.md 的 frontmatter 前不加 BOM）；
2. Shipping 前排查编辑器 Only API（见 03_PITFALLS A1）；
3. 每轮源码改动后编译验证：
   `<UE_ROOT>\Engine\Build\BatchFiles\Build.bat BlockoutStudioEditor Win64 Development -project="<REPO_ROOT>\BlockoutStudio.uproject" -waitmutex`（`<UE_ROOT>` = 本机 UE 5.6.1 安装目录，自行准备或向用户索要）；
4. 涉及变换/gizmo 的改动必须复测坐标系轴向（用户红线）；
5. 优先复用既有链路（如布尔结果写 bFreeform+Vertices/Triangles 复用撤销/序列化，不新增存档字段）。

## 5. git 管理约定

| 项 | 约定 |
|---|---|
| 分支 | main |
| LFS | `Packaged/**` 走 LFS；`*.uasset/umap` 不走 LFS（防超 1GB 额度） |
| 入库 | Content 资产全量入库（Characters/PMB/UEFirstPerson/Levels） |
| 忽略 | Build\ / Intermediate\ / Saved\ / DerivedDataCache\ |
| 历史 commit 主线 | ...→ b07a677(5项修复) → 62ffbc9(dust2) → f89d416(编码) → 2d4ca25(资产入库) → ced5b84(打包产物) → 40cf826(R2/D3/D5/P3) → 40784e7(UI收尾+文档+HANDOVER) |

## 6. 协同规则（多 Agent）

- 子 Agent（file-agent 等）只完成"当前任务目标"，全局目标仅供理解背景，不越权；
- 移交任务时交接信息必须完整：文件绝对路径、中间产物目录、约束条件、已收集数据；
- 任务完成后如实汇报结果与遗留，产物登记供用户查看。

## 7. 禁止事项汇总

- ❌ 粉饰/伪造验收结果、静默砍功能；
- ❌ 引用 DesktopPlatform / 编辑器 Only API 进 Shipping；
- ❌ 源文件存成无 BOM UTF-8 或 LF 换行；
- ❌ 破坏坐标系数学正确性且不复测；
- ❌ 绕过安全验证（Unity 许可证、GitHub 验证等）；
- ❌ 把中间产物当最终交付物登记。
*（内容由AI生成，仅供参考）*
