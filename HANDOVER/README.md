# HANDOVER — BlockoutStudio AI 开发交接包

> **本目录是给接手本项目的 AI 开发者的完整交接包**。读完本 README 后按顺序阅读各分册，即可在不依赖任何口头上下文的情况下继续开发。
> 交接日期：2026-10-18 | 项目状态：v1.0 收尾完成（全部功能开发完毕 + UI 收尾 + 文档齐全）

## 阅读顺序

| 顺序 | 文件 | 内容 |
|---|---|---|
| 1 | [01_PROJECT_BRIEF.md](01_PROJECT_BRIEF.md) | 项目背景、目标、需求规格要点（E/M/T/A/P/R/D 全功能编号）、用户红线 |
| 2 | [02_ARCHITECTURE.md](02_ARCHITECTURE.md) | 架构与模块清单、代码事实、关键 API、数据格式 |
| 3 | [03_PITFALLS.md](03_PITFALLS.md) | **历史开发全部踩坑与避坑经验（必读，避免重蹈覆辙）** |
| 4 | [04_FEATURE_STATUS.md](04_FEATURE_STATUS.md) | 已完成 / 未完成功能清单、遗留项、下一步建议 |
| 5 | [05_ACCEPTANCE.md](05_ACCEPTANCE.md) | 验收标准与实测结果基准 |
| 6 | [06_WORKFLOW.md](06_WORKFLOW.md) | 工作流程硬性要求（commit+push 节奏、产物约定、禁止事项） |
| 7 | [07_SKILLS.md](07_SKILLS.md) | 建议后续 AI 具备的能力 / skill 清单 |

## 一分钟上手

```bat
:: 环境
引擎   : UE 5.6.1 @ F:\UE_5.6
工程   : E:\BlockoutStudio\BlockoutStudio.uproject
仓库   : https://github.com/prncsuc/blockout-tools-ue (main 分支，remote 已内嵌 token；2026-10-18 起推荐直连推送，代理 127.0.0.1:26001 证书已失效，详见 03_PITFALLS F1)

:: 编译
F:\UE_5.6\Engine\Build\BatchFiles\Build.bat BlockoutStudioEditor Win64 Development -project="E:\BlockoutStudio\BlockoutStudio.uproject" -waitmutex

:: Shipping 打包（约 1 分钟）
F:\UE_5.6\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project="E:\BlockoutStudio\BlockoutStudio.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -prereqs -archivedirectory="E:\BlockoutStudio\Packaged\Windows" -unattended
```

## 仓库文档地图

| 文件 | 说明 |
|---|---|
| `README.md` | 项目主页与快速开始 |
| `docs/USER_MANUAL.md` | 完整用户使用手册（安装/界面/操作/快捷键/FAQ） |
| `REQUIREMENT_CHECK.md` | 需求规格逐条核对结论表（状态以此为准） |
| `ACCEPTANCE.md` | 验收清单逐条勾选结果 |
| `FEATURE_COMPARISON.md` | 与 PMB / ProBuilder 功能对比 |
| `PROGRESS.md` | 开发进度与关键决策记录 |
| `HANDOVER/` | 本交接包 |

## 三条最重要的红线

1. **坐标系数学必须正确**（世界/本地切换、UE 左手系 Z-up）——用户因 web 版坐标系错误而否定过旧项目；
2. **禁止静默砍功能**——做不了就写入 `UNIMPLEMENTED.md`（功能名/原因/替代方案）并继续，不停下等用户；
3. **C++ 源文件必须 UTF-8 with BOM + CRLF**；Markdown 的 YAML frontmatter 前不加 BOM。
