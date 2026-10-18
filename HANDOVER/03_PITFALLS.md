---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: 5227bc309b64902b9bfd2bb79d4de347_081a6317cad211f18b7f525400393706
    ReservedCode1: viKl7I/kfhI2isAvit/Zzz7w0bSVNvzHRmHoRdVIdJ/LCYLrgEI39nHsFDhA1OKLgLNlT+8t+Hnfgv6aglHfE6Ymx7Ts2TD8863HDq/d8d37JPyuHWhNOiOwd7vW4BTXcy7o4usQ42UA4qQUbxTrl6sDzjr/7U/nlDmopU1K/k0GoTlfgTpiYXxt/oI=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: 5227bc309b64902b9bfd2bb79d4de347_081a6317cad211f18b7f525400393706
    ReservedCode2: viKl7I/kfhI2isAvit/Zzz7w0bSVNvzHRmHoRdVIdJ/LCYLrgEI39nHsFDhA1OKLgLNlT+8t+Hnfgv6aglHfE6Ymx7Ts2TD8863HDq/d8d37JPyuHWhNOiOwd7vW4BTXcy7o4usQ42UA4qQUbxTrl6sDzjr/7U/nlDmopU1K/k0GoTlfgTpiYXxt/oI=
---

# 03 历史踩坑与避坑全集（必读）

> 本项目全部历史踩坑记录。接手后修改代码/打包/提交前请先过一遍本清单，绝大部分坑会重复出现。

## A. 编译与打包

### A1. Shipping 打包暴露"编辑器 Only API"
- **现象**：Development Editor 编译一直通过，`RunUAT BuildCookRun -clientconfig=Shipping` 才报 `GetActorLabel`/`IsHiddenEd` 不是成员、`UTexture2D::MipGenSettings` 不存在。
- **原因**：这些 API 仅 `WITH_EDITOR`/`WITH_EDITORONLY_DATA` 下存在，编辑器配置定义了宏所以不报错。
- **解法**：`GetActorLabel()`→`GetActorNameOrLabel()`；`IsHiddenEd()`→`IsHidden()`；编辑器专属赋值用 `#if WITH_EDITORONLY_DATA` 包裹。**改后全局再搜一遍 `GetActorLabel`/`IsHiddenEd`/`GEditor` 残留。教训：Shipping 前不能只看 Development Editor 编译通过。**

### A2. DesktopPlatform 模块不可再分发
- **现象**：Shipping 打包失败（DesktopPlatform 许可不允许随产品分发）。
- **解法**：改用 Win32 通用文件对话框 `FBlockoutFileDialog`（自研封装），**永不引用 IDesktopPlatform**。

### A3. 打包 archive 不更新已有目录
- RunUAT `-archive` 对已存在输出目录可能不覆盖全部文件，exe 需手动补拷验证。

## B. 运行时渲染/截图

### B1. ReadPixels 截图全黑（Shipping）
- **现象**：运行时 `ReadPixels` 读到全零像素。
- **解法**：改用 `SceneCaptureComponent2D` 截图 + `DirectionalLight`(强度约 8) + `SkyLight`(强度约 1.2) 固定曝光。

### B2. Z-fighting 闪烁
- 共面体块组做 **+0.02uu 膨胀偏移**，消除深度缓冲竞争。

### B3. 世界对齐材质与 mip 链
- 演示品质问题曾因材质未世界对齐 + 未建 mip 链导致，需注意 `Tiling` 与 mip 设置。

## C. UE API 签名坑

| 坑 | 正确姿势 |
|---|---|
| `FPlatformProcess::CreateProc` 返回值 | `FProcHandle`，不是 `void*` |
| `ComputePenetration` 签名 | `(FMTDResult&, const FCollisionShape, FVector, FQuat)` |
| `TBulkData::Realloc` | 先 `Realloc` 再 `Lock`（顺序反了会崩） |
| Slate `OnPaint` 第 3 参数（UE 5.6） | `const FSlateRect&`，不是 `FSlateClipRect` |
| `FSlateDrawElement::MakeBox` | 需要 `const FSlateBrush*`；传局部 `FSlateColorBrush` 时取地址 `&Brush` |
| `FSlateDrawElement::MakeLines` | `TArray<FVector2f>` 值传重载；`MakeArrayView()` 会匹配失败 |
| `FDragDropOperation` 头文件 | `Input/DragAndDrop.h`（**不是** `Widgets/DragDrop/...`，后者不存在） |
| SListView 拖拽接收 | `OnCanAcceptDrop`/`OnAcceptDrop` 挂在 **STableRow 行模板** FArguments 上，SListView 没有这两个委托 |
| STableRow 拖拽发起 | `OnDragDetected(this, &Class::Fn, Item)` 可用 delegate payload 绑定行数据 |
| 编辑器 UI lambda | `BuildCreatePanel` static 数组 + lambda 注意捕获生命周期 |

## D. 动画/演示/录制

### D1. AutoDemoTimer < 0.1f 失效
- Timer 间隔不得小于 0.1s；自动演示机位改用 **AutoDemoPlacedShot 标记法**（显式放置机位，不靠裸 Timer 驱动）。

### D2. 录制视频格式
- 原生产出 AVI(MJPEG) 可播放；MP4/WebM 用内置 `ThirdParty\ffmpeg\ffmpeg.exe`（LGPL 动态链接）转封装产出 H.264/VP8 双成品。

## E. 文件与编码规范（高频）

### E1. C++ 源文件必须 UTF-8 with BOM + CRLF
- Windows MSVC 对无 BOM UTF-8 中文注释会误判编码，出现 C2001/C2226 等怪错或警告。**每轮改动后统一转换**：
  - PowerShell/Python 脚本统一 `utf-8-sig` + `\r\n`；
- **例外**：`.md` 文件 YAML frontmatter（`---`）之前不要加 BOM。

### E2. write_file 不能覆盖已有文件
- 工具会自动重命名（产生副本）而非覆盖；改已有文件必须用 `edit_file`。

### E3. 同一文件多处 edit_file 必须串行
- 同文件编辑互相冲突，必须一次一轮串行执行；不同文件才可并行。

### E4. 中间产物
- 临时脚本/清单写入当前会话/工作区约定的中间产物目录（如 `temp\`）；最终产物写结果目录（如 `output\`）；禁止写桌面、系统临时目录等任意位置。

## F. Git 与仓库

### F1. 推送通道与凭据（2026-10-18 更新）
- **优先直连推送**：`git -c http.proxy= -c https.proxy= push`（2026-10-18 实测直连成功）；
- 若网络无法直连 GitHub，需配置本地代理——**代理地址不在交接范围内，请向用户索要**；历史代理曾出现 MITM 证书异常（schannel 报 SEC_E_WRONG_PRINCIPAL、openssl 报证书主机名不匹配），遇同类证书错误先排查代理；
- remote URL 可能内嵌 PAT token（来自需求原件 §9，用户应尽快轮换），注意凭据保护。

### F2. Git LFS 策略（当前）
- `Packaged/**` 走 LFS；
- **`*.uasset/umap` 不走 LFS**（曾 track 后回退，防止超过 1GB 免费额度），全部 Content 资产直接 git 存储；
- `.gitignore`：`Build\`、`Intermediate\`、`Saved\`、`DerivedDataCache\` 排除；Content 资产**全量入库**（Characters/PMB/UEFirstPerson/Levels，"Epic 官方资产不入 git"的早期决策已被"全量入库"新决策取代）。

### F3. 提交节奏
- 每个阶段/功能块完成即 commit（中文或 Conventional Commits 均可），push 至少每阶段一次；见 06_WORKFLOW.md。

## G. 业务决策坑

### G1. 坐标系红线
- 世界/本地坐标切换数学必须严格正确（UE 左手系 Z-up）；旧 web 版坐标系错误被用户全盘否定。改 gizmo/变换代码后必须实测轴向。

### G2. 禁止静默砍功能
- 无法实现→写 `UNIMPLEMENTED.md`（功能名/原因/替代方案）→继续；不许粉饰验收结果。

### G3. Unity 导入实测
- 历史开发机 Unity 2021.3.8f1 许可证未激活（batchmode 报 "Unity has not been activated with a valid License"），**不绕过验证**；接手后自行准备 Unity（2022+ 亦可）并请用户激活许可证，用导出样例（OBJ/FBX/glTF/GLB，可用 `-AutoExportTest` 重新生成）补跑导入实测。
*（内容由AI生成，仅供参考）*
