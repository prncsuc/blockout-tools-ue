---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 5227bc309b64902b9bfd2bb79d4de347_0ae61ea3cad211f196f9525400248c00
    ReservedCode1: NUPc9LRaJfHWe2M4D9tmuy0h5eFYKlK/xo6k98YkTKMwyyBRhOA2yJPOCqpe3v/BW/5ChmtoclbaoIlSZ2gZvMcsnDBU7noxtctSvO+JL/o6hcI9jb1SCjRswYxFQy6zLY00mbi7AwpdNFhadgnv+z/na5GI/mXI9rZiUPRzoPa5NgUHd52+me+MSwA=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 5227bc309b64902b9bfd2bb79d4de347_0ae61ea3cad211f196f9525400248c00
    ReservedCode2: NUPc9LRaJfHWe2M4D9tmuy0h5eFYKlK/xo6k98YkTKMwyyBRhOA2yJPOCqpe3v/BW/5ChmtoclbaoIlSZ2gZvMcsnDBU7noxtctSvO+JL/o6hcI9jb1SCjRswYxFQy6zLY00mbi7AwpdNFhadgnv+z/na5GI/mXI9rZiUPRzoPa5NgUHd52+me+MSwA=
---

# 07 建议后续 AI 具备的能力 / skill 清单

> 本项目开发全程用到的能力画像。接手 AI 环境建议至少具备以下技能/工具，否则相应任务会低效或无法完成。

## 1. 必备能力（按优先级）

| # | 能力/skill | 用途 | 本项目用例 |
|---|---|---|---|
| 1 | **UE C++ 开发能力**（UE 5.6 API 熟练） | 主体开发：Slate UI、DynamicMesh/GeometryScript、CharacterMovement、Enhanced Input、序列化 | 全部功能模块 |
| 2 | **Shell/构建工具调用** | 编译（Build.bat）、打包（RunUAT）、git 操作 | 每轮改动的编译验证与 Shipping 打包 |
| 3 | **Python 脚本执行** | 关卡 JSON 程序化生成、编码规范化（utf-8-sig+CRLF 批量转换）、数据校验/抽帧验证 | tools\ 关卡生成、验收数据分析 |
| 4 | **Git/GitHub 操作**（含代理、LFS） | commit+push 节奏、LFS 管理、冲突处理 | 全程 |
| 5 | **文档写作 skill**（document-writer 级） | README/USER_MANUAL/HANDOVER/REQUIREMENT_CHECK 等结构化文档 | 本轮文档收尾 |
| 6 | **文件整理/检索 skill**（file-organizer / file-search） | 仓库资产盘点、产物管理、按规则归档 | 资产入库、产物目录整理 |

## 2. 强烈建议具备

| # | 能力/skill | 用途 |
|---|---|---|
| 7 | **图像分析**（analyze_image 级） | 抽帧验证演示视频、截图验收核对 |
| 8 | **PDF/Office 读写** | 读需求原件（md）、输出对比表格 |
| 9 | **长任务规划 skill**（planning-with-files 级） | 多阶段开发的进度持久化与断点续传（本项目跨度长、多轮会话） |
| 10 | **ffmpeg/多媒体处理** | 视频转封装（MP4/WebM）、录屏抽帧 |
| 11 | **知识库沉淀 skill**（如 marvis-llm-wiki） | 领域知识（UE API 坑、打包链路）结构化沉淀，跨会话复用 |

## 3. 任务类型 → skill 映射（实战经验）

| 任务 | 推荐路线 |
|---|---|
| 生成/修改文档（README/报告） | document-writer / docx skill |
| Excel 清单（功能矩阵、验收表） | excel-processing-and-analysis |
| 整理仓库目录/归档 | file-organizer（先蓝图后执行） |
| 找历史文档/资料 | file-search（语义）或按文件名精确搜 |
| 图片批量处理/截图整理 | image-processing / image-search |
| 演示视频生成（若需图转视频） | photo-to-video（PPT 需求用 ppt-creator/pptx，注意路由） |
| 超长批量任务 | planning-with-files |

## 4. 环境侧依赖（非 skill，但必须可访问）

| 依赖 | 说明 |
|---|---|
| UE 5.6.1（安装目录自行准备或向用户索要，下称 `<UE_ROOT>`） | 编译与打包指定引擎版本 |
| VS 2022 BuildTools | MSVC 14.44 工具链 |
| ffmpeg @ `ThirdParty\ffmpeg\ffmpeg.exe` | 视频转封装（LGPL 动态链接，已随仓库分发） |
| 网络代理（仅当 GitHub 无法直连时需要） | 代理地址向用户索要；2026-10-18 实测直连可推送，详见 03_PITFALLS F1 |
| Unity Editor（需自行准备） | D3 导入实测补跑用；历史验证环境为 Unity 2021.3.8f1，许可证需用户激活 |
| 历史会话记忆/知识库 | 本项目大量上下文在多轮会话中沉淀（UE 坑、链路方法），新 AI 建议先读本 HANDOVER 全部文件再动手 |

## 5. 开工自检清单（新 AI 第一天）

- [ ] 通读 `HANDOVER/` 7 个文件 + `REQUIREMENT_CHECK.md` + `PROGRESS.md`；
- [ ] 确认引擎路径与编译命令可跑通（Development Editor）；
- [ ] 确认 git remote/代理可用（`git fetch` 试通）；
- [ ] 跑一次 `-AutoPerfTest=100` 确认打包 exe 可用；
- [ ] 阅读 `docs/USER_MANUAL.md` 了解用户视角操作；
- [ ] 查看 04_FEATURE_STATUS.md 遗留项，按优先级继续。
*（内容由AI生成，仅供参考）*
