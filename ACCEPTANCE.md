# BlockoutStudio P5 验收清单

UE5 白盒关卡工具（E:\BlockoutStudio），P5 验收测试（需求规格 v1.1 §8）。验收日期：2026-10-17。验收环境：UE 5.6.1 / Windows 11 / Shipping 打包跑图。

## P5 验收清单

### 1. 鸟瞰图（三测试关卡）

- [x] 类魂关卡（level_souls_castle）输出 3 张鸟瞰图（aerial_1/2/3.png）
- [x] 类 CS 关卡（level_cs_dustyard）输出 3 张鸟瞰图（aerial_1/2/3.png）
- [x] 类神海关卡输出 3 张鸟瞰图（aerial_1/2/3.png）
- [x] 同一关卡 3 张视角互异（机位/朝向均不同，无重复视角）
- [x] 画面质量合格（无黑屏、无遮挡、构图完整）

**验收结论：通过。** 三关卡共 9 张鸟瞰图，各关卡 3 张视角互异，画面质量合格。

### 2. 跑图视频（v2 修复版，抽帧验证）

对三关卡跑图视频各抽取 f0/f1/f2 三帧逐项检查：

- [x] 画面无遮挡（相机不被 Pawn mesh 遮挡）
- [x] 画面无穿模
- [x] 亮度正常（非黑屏/非全零帧）
- [x] 相机明显移动（帧间视角有实际变化）

| 关卡 | 抽帧序列 | f0->f1 mean_diff / pct_changed | f1->f2 mean_diff / pct_changed | 结论 |
|---|---|---|---|---|
| demo_souls（类魂） | f0/f1/f2 | 51.53 / 64.5% | 92.22 / 80.3% | 通过 |
| demo_dust2（类 CS，level_cs_dustyard） | f0/f1/f2 | 47.59 / 26.8% | 22.43 / 14.5% | 通过（附注） |
| demo_uncharted（类神海） | f0/f1/f2 | 86.79 / 82.4% | 78.33 / 75.6% | 通过 |

附注：level_cs_dustyard 场景开阔，帧间透视变化较小，但像素差 pct_changed 达 14-27%，确认相机在移动，判定通过。

**验收结论：通过。** 三关卡跑图视频 v2 画面无遮挡无穿模、亮度正常、相机明显移动，全部达标。

## 总体验收结论

P5 验收全部通过：鸟瞰图三关卡各 3 张视角互异；跑图视频 v2 抽帧验证画面无遮挡无穿模、亮度正常、相机明显移动。遗留仅 P6 打包发布。

## 证据文件

| 证据 | 路径 |
|---|---|
| 鸟瞰图（类魂） | C:\Users\mail\AppData\Roaming\Tencent\Marvis\User\49669A1552B1EA7C8AB5DFD758A55155\workspace\conv_8fdc8b8718904d11afccd6f0b4798006\output\demo_souls\aerial_1.png ～ aerial_3.png |
| 鸟瞰图（类 CS） | ...\output\demo_dust2\aerial_1.png ～ aerial_3.png |
| 鸟瞰图（类神海） | ...\output\demo_uncharted\aerial_1.png ～ aerial_3.png |
| 跑图视频 v2（类魂） | ...\output\demo_souls\runthrough.avi |
| 跑图视频 v2（类 CS） | ...\output\demo_dust2\runthrough.avi |
| 跑图视频 v2（类神海） | ...\output\demo_uncharted\runthrough.avi |
| 抽帧验证帧（v2） | ...\temp\demo_souls_v2_f0/f1/f2.png、demo_dust2_v2_f0/f1/f2.png、demo_uncharted_v2_f0/f1/f2.png |

（路径前缀均为 C:\Users\mail\AppData\Roaming\Tencent\Marvis\User\49669A1552B1EA7C8AB5DFD758A55155\workspace\conv_8fdc8b8718904d11afccd6f0b4798006）
