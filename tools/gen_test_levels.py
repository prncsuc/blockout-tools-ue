# -*- coding: utf-8 -*-
"""
生成三个测试关卡 .blockout.json（项目存档格式，可被 BlockoutStudio LoadProject 直接打开）。
- 类魂关卡：灰堡要塞（史东威尔城风格），外庭 + 城门 + 塔楼 + 回廊 + 地牢坡道
- 类 CS 双边对抗：沙尘仓库（dust2 风格），T/CT 出生点 + 中门 + 双通道 + 包点
- 类神海动作冒险：遗迹攀岩场，攀岩墙 + 跑酷平台 + 绳索横梁 + 洞穴
单位：uu（1uu = 1cm）。关卡尺寸对应普通关卡（60m~120m 级）。
"""
import json, uuid, math, os

def blk(shape, name, loc, size, rot=(0,0,0), color=(0.6,0.6,0.6,1), pattern="Grid", tiling=1.0, **params):
    b = {
        "Id": str(uuid.uuid4()).upper(),
        "Name": name,
        "Shape": shape,
        "Params": {
            "Length": size[0], "Width": size[1], "Height": size[2],
            "Thickness": params.get("thickness", 20),
            "AngleDeg": params.get("angle", 90),
            "Steps": params.get("steps", 5),
            "Segments": params.get("segments", 24),
            "HoleWidth": params.get("hole_w", 100),
            "HoleHeight": params.get("hole_h", 200),
            "HoleOffsetZ": params.get("hole_z", 0),
            "ArrayCount": params.get("count", 4),
            "ArraySpacing": params.get("spacing", 300),
            "bThroughHole": params.get("through", True),
        },
        "Transform": {
            "Translation": {"X": loc[0], "Y": loc[1], "Z": loc[2]},
            "Rotation": {"X": rot[0], "Y": rot[1], "Z": rot[2], "W": rot[3] if len(rot) > 3 else 1.0},
            "Scale3D": {"X": 1, "Y": 1, "Z": 1},
        },
        "SurfaceType": "Wall",
        "Color": {"R": color[0], "G": color[1], "B": color[2], "A": 1.0},
        "TexturePattern": pattern,
        "Tiling": tiling,
        "bHidden": False,
        "bLocked": False,
        "GroupId": "00000000-0000-0000-0000-000000000000",
        "bFreeform": False,
        "Vertices": [],
        "Triangles": [],
    }
    return b

def yaw(deg):
    h = math.radians(deg) * 0.5
    return (0.0, 0.0, math.sin(h), math.cos(h))

def room(blocks, cx, cy, w, d, wall_h=400, t=30, z=0, color=(0.55,0.55,0.58), prefix="Wall", doors=None):
    """四面围墙，doors: [(side, offset, hole_w, hole_h)] side in N/S/E/W"""
    doors = doors or []
    # 南北墙（沿 X）
    for side, yy, rot in (("S", cy - d/2, 0), ("N", cy + d/2, 0)):
        dl = [t for t in doors if t[0] == side]
        if dl:
            blocks.append(blk("DoorWall", f"{prefix}_{side}", (cx, yy, z + wall_h/2), (w, t, wall_h),
                              color=color, hole_w=dl[0][2], hole_h=dl[0][3]))
        else:
            blocks.append(blk("Wall", f"{prefix}_{side}", (cx, yy, z + wall_h/2), (w, t, wall_h), color=color))
    for side, xx, rot in (("W", cx - w/2, yaw(90)), ("E", cx + w/2, yaw(90))):
        dl = [t for t in doors if t[0] == side]
        if dl:
            blocks.append(blk("DoorWall", f"{prefix}_{side}", (xx, cy, z + wall_h/2), (d, t, wall_h), rot=rot,
                              color=color, hole_w=dl[0][2], hole_h=dl[0][3]))
        else:
            blocks.append(blk("Wall", f"{prefix}_{side}", (xx, cy, z + wall_h/2), (d, t, wall_h), rot=rot, color=color))
    blocks.append(blk("Floor", f"{prefix}_Floor", (cx, cy, z - 10), (w, d, 20), color=(0.35,0.35,0.38), tiling=2))

def save(name, blocks, outdir):
    doc = {"version": "1.0", "savedAt": "2026-10-17T00:00:00", "blocks": blocks}
    path = os.path.join(outdir, name)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(doc, f, ensure_ascii=False)
    print(f"{name}: {len(blocks)} blocks -> {path}")

# ============ 关卡 1：类魂 - 灰堡要塞（约 90m x 70m，含高低差 20m） ============
def level_souls():
    B = []
    gray = (0.42, 0.40, 0.38)
    dark = (0.30, 0.29, 0.28)
    # 外庭地面（90m x 50m）
    B.append(blk("Floor", "Courtyard", (0, 0, -10), (9000, 5000, 20), color=(0.32,0.31,0.30), tiling=4))
    # 外围墙（带垛口感：围墙）
    B.append(blk("Fence", "Fence_S", (0, -2500, 150), (9000, 20, 300), color=dark, spacing=400))
    B.append(blk("Fence", "Fence_N", (0, 2500, 150), (9000, 20, 300), color=dark, spacing=400))
    B.append(blk("Fence", "Fence_W", (-4500, 0, 150), (5000, 20, 300), rot=yaw(90), color=dark, spacing=400))
    B.append(blk("Fence", "Fence_E", (4500, 0, 150), (5000, 20, 300), rot=yaw(90), color=dark, spacing=400))
    # 城门（南侧门楼 + 门洞墙 + 门楼塔）
    B.append(blk("DoorWall", "GateWall", (0, -2500, 350), (1600, 60, 700), color=gray, hole_w=350, hole_h=450))
    B.append(blk("Cylinder", "Tower_L", (-1100, -2500, 450), (700, 700, 900), color=gray, segments=16))
    B.append(blk("Cylinder", "Tower_R", (1100, -2500, 450), (700, 700, 900), color=gray, segments=16))
    B.append(blk("Wedge", "GateRoof", (0, -2500, 800), (1800, 500, 400), color=dark))
    # 主堡（北侧，两层 + 拱门入口 + 塔楼）
    B.append(blk("Wall", "Keep_S", (0, 1800, 500), (1200, 40, 1000), color=gray))
    B.append(blk("Arch", "KeepArch", (0, 1800, 250), (600, 300, 500), color=gray, hole_w=300, hole_h=420))
    B.append(blk("Wall", "Keep_N", (0, 3600, 500), (1200, 40, 1000), color=gray))
    B.append(blk("Wall", "Keep_W", (-600, 2700, 500), (1800, 40, 1000), rot=yaw(90), color=gray))
    B.append(blk("Wall", "Keep_E", (600, 2700, 500), (1800, 40, 1000), rot=yaw(90), color=gray))
    B.append(blk("Floor", "Keep_F1", (0, 2700, 300), (1200, 1800, 30), color=dark))
    B.append(blk("Cylinder", "Keep_Tower", (0, 2700, 1400), (800, 800, 1200), color=gray, segments=20))
    B.append(blk("Wedge", "Keep_TowerRoof", (0, 2700, 2150), (900, 900, 400), color=dark))
    # 大楼梯（进入主堡高台）
    B.append(blk("Staircase", "GrandStair", (0, 1300, 150), (800, 1000, 300), color=gray, steps=12))
    # 东侧回廊（柱廊 + 顶棚，高差 4m 平台）
    B.append(blk("Floor", "Gallery_Floor", (2600, 500, 390), (3000, 2200, 30), color=dark))
    B.append(blk("Colonnade", "Gallery_Columns", (2600, -450, 200), (300, 300, 400), color=gray,
                 count=8, spacing=400))
    B.append(blk("Floor", "Gallery_Roof", (2600, 500, 600), (3000, 2400, 30), color=dark))
    # 楼梯间连接平台（M5 快速生成）
    B.append(blk("Stairwell", "GalleryStairwell", (2600, -1400, 200), (600, 1200, 400), color=gray, steps=10, thickness=30))
    # 地牢入口（西侧坡道 + 门洞墙 + 下沉庭院）
    B.append(blk("Ramp", "DungeonRamp", (-2600, 500, -200), (1200, 2400, 400), color=dark))
    B.append(blk("DoorWall", "DungeonDoor", (-3600, 500, 200), (600, 40, 400), rot=yaw(90), color=dark,
                 hole_w=250, hole_h=320))
    B.append(blk("Floor", "DungeonFloor", (-4100, 500, -410), (1000, 2400, 20), color=dark))
    B.append(blk("Wall", "DungeonWall_N", (-4100, 1700, -100), (1000, 30, 600), color=dark))
    B.append(blk("Wall", "DungeonWall_S", (-4100, -700, -100), (1000, 30, 600), color=dark))
    # 西塔楼
    B.append(blk("Cylinder", "WestTower", (-3800, -1800, 500), (700, 700, 1000), color=gray, segments=16))
    B.append(blk("Staircase", "WestTowerStair", (-3000, -1800, 150), (600, 900, 300), color=gray, steps=10))
    # 破墙/掩体（魂系战斗节奏点）
    for i, (x, y) in enumerate([(-1500, -1200), (1800, -1000), (-800, 300), (1500, 1200)]):
        B.append(blk("Cube", f"Cover_{i}", (x, y, 90), (400, 300, 180), color=(0.45,0.43,0.41)))
    return B

# ============ 关卡 2：类 CS 双边对抗 - 沙尘仓库（约 110m x 90m） ============
def level_cs():
    B = []
    sand = (0.72, 0.62, 0.45)
    sand2 = (0.65, 0.55, 0.40)
    white = (0.82, 0.78, 0.70)
    # 主地面
    B.append(blk("Floor", "Ground", (0, 0, -10), (11000, 9000, 20), color=(0.55,0.48,0.38), tiling=6))
    # T 出生点（西南）
    room(B, -4000, -3200, 2400, 1800, wall_h=450, color=sand, prefix="TSpawn",
         doors=[("E", 0, 300, 300)])
    B.append(blk("Floor", "TSpawn_RoofMark", (-4000, -3200, 460), (2400, 1800, 20), color=(0.5,0.42,0.33)))
    # CT 出生点（东北）
    room(B, 4000, 3200, 2400, 1800, wall_h=450, color=sand, prefix="CTSpawn",
         doors=[("W", 0, 300, 300)])
    # A 包点（西北）
    room(B, -3600, 2600, 2600, 2200, wall_h=420, color=white, prefix="ASite",
         doors=[("S", -600, 300, 300), ("E", 0, 300, 300)])
    B.append(blk("Cube", "A_Box1", (-3900, 2500, 250), (500, 500, 500), color=sand2))
    B.append(blk("Cube", "A_Box2", (-3300, 2900, 200), (600, 400, 400), color=sand2))
    B.append(blk("Cube", "A_Platform", (-4200, 3200, 225), (700, 900, 450), color=sand2))
    # B 包点（东南，带高台）
    room(B, 3600, -2600, 2600, 2200, wall_h=420, color=white, prefix="BSite",
         doors=[("N", 600, 300, 300), ("W", 0, 300, 300)])
    B.append(blk("Cube", "B_Box1", (3900, -2500, 250), (500, 500, 500), color=sand2))
    B.append(blk("Staircase", "B_Stair", (3000, -3100, 200), (700, 900, 400), color=sand2, steps=8))
    B.append(blk("Floor", "B_HighGround", (3000, -3600, 410), (1600, 700, 30), color=sand2))
    # 中路长廊（Mid）：两侧高墙 + 中门（dust2 中门）
    B.append(blk("Wall", "MidWall_N", (0, 1200, 350), (6000, 40, 700), color=sand))
    B.append(blk("Wall", "MidWall_S", (0, -1200, 350), (6000, 40, 700), color=sand))
    B.append(blk("DoorWall", "MidDoor", (0, 0, 300), (1200, 40, 600), rot=yaw(90), color=white,
                 hole_w=320, hole_h=380))
    B.append(blk("Wall", "MidDoor_N", (0, 700, 350), (1200, 40, 700), rot=yaw(90), color=sand))
    B.append(blk("Wall", "MidDoor_S", (0, -700, 350), (1200, 40, 700), rot=yaw(90), color=sand))
    # 中路箱子掩体
    for i, (x, y) in enumerate([(-800, -300), (600, 400), (0, -600), (-200, 700)]):
        B.append(blk("Cube", f"MidBox_{i}", (x, y, 225), (450, 450, 450), color=sand2))
    # 长通道 Long（西侧，T→A）
    B.append(blk("Wall", "LongWall_E", (-2000, -800, 350), (3600, 40, 700), rot=yaw(90), color=sand))
    B.append(blk("Wall", "LongWall_W", (-3400, -800, 350), (3600, 40, 700), rot=yaw(90), color=sand))
    B.append(blk("Arch", "LongArch", (-2700, 800, 300), (500, 300, 600), rot=yaw(90), color=white,
                 hole_w=280, hole_h=400))
    # 短通道 Short（东侧，CT→B）
    B.append(blk("Wall", "ShortWall_E", (2200, 800, 350), (3200, 40, 700), rot=yaw(90), color=sand))
    B.append(blk("Wall", "ShortWall_W", (1100, 800, 350), (3200, 40, 700), rot=yaw(90), color=sand))
    # 洞/隧道（连接 mid 与 B）
    B.append(blk("Arch", "TunnelArch", (1200, -800, 250), (500, 300, 500), rot=yaw(90), color=white,
                 hole_w=260, hole_h=350))
    B.append(blk("Wall", "TunnelWall", (1700, -800, 350), (3200, 40, 700), rot=yaw(90), color=sand))
    # 沙丘地形（斜坡 + 高台，丰富垂直玩法）
    B.append(blk("Ramp", "Ramp_North", (-1200, 1800, 150), (1200, 1600, 300), color=sand2))
    B.append(blk("Ramp", "Ramp_South", (1200, -1800, 150), (1200, 1600, 300), rot=yaw(180), color=sand2))
    # 外围封闭墙
    B.append(blk("Wall", "Boundary_N", (0, 4500, 400), (11000, 60, 800), color=(0.5,0.44,0.35)))
    B.append(blk("Wall", "Boundary_S", (0, -4500, 400), (11000, 60, 800), color=(0.5,0.44,0.35)))
    B.append(blk("Wall", "Boundary_W", (-5500, 0, 400), (9000, 60, 800), rot=yaw(90), color=(0.5,0.44,0.35)))
    B.append(blk("Wall", "Boundary_E", (5500, 0, 400), (9000, 60, 800), rot=yaw(90), color=(0.5,0.44,0.35)))
    return B

# ============ 关卡 3：类神海 - 遗迹攀岩场（约 100m x 80m，高差 35m） ============
def level_uncharted():
    B = []
    rock = (0.48, 0.42, 0.35)
    ruin = (0.62, 0.58, 0.50)
    moss = (0.35, 0.42, 0.32)
    # 起始平台（南侧峡谷口）
    B.append(blk("Floor", "StartPlateau", (0, -3000, -10), (10000, 4000, 20), color=(0.40,0.36,0.31), tiling=5))
    # 峡谷两侧高岩
    B.append(blk("Cube", "Cliff_W", (-5000, -2000, 1200), (1400, 8000, 2400), color=rock, tiling=3))
    B.append(blk("Cube", "Cliff_E", (5000, -2000, 1200), (1400, 8000, 2400), color=rock, tiling=3))
    # 攀岩墙 A（西侧，带楔形岩点）
    B.append(blk("Wall", "ClimbWall_A", (-3200, -1000, 1200), (40, 1600, 2400), color=rock))
    for i in range(6):
        B.append(blk("Wedge", f"ClimbHold_A{i}", (-3150, -1700 + i*280, 500 + i*380), (300, 260, 220),
                     rot=yaw(90), color=ruin))
    # 跑酷平台链（Z 字上升，高差 8m）
    plats = [(-2200, -800, 400), (-1200, -200, 800), (-200, -800, 1200), (800, -150, 1600), (1800, -800, 2000)]
    for i, (x, y, z) in enumerate(plats):
        B.append(blk("Cube", f"Parkour_{i}", (x, y, z), (900, 700, 80), color=ruin))
        if i % 2 == 0:
            B.append(blk("Cube", f"ParkourRail_{i}", (x, y - 320, z + 200), (900, 40, 320), color=moss))
    # 绳索横梁（窄梁跨越峡谷）
    B.append(blk("Cube", "Beam1", (2600, -200, 2100), (2200, 220, 120), color=ruin))
    B.append(blk("Cube", "Beam2", (3600, 400, 2300), (1800, 200, 110), rot=yaw(25), color=ruin))
    # 中央遗迹庭院（高台，柱廊 + 半塌墙 + 拱门）
    B.append(blk("Floor", "RuinCourt", (2200, 1200, 2390), (6000, 5000, 30), color=(0.55,0.51,0.44), tiling=3))
    B.append(blk("Colonnade", "RuinCols_N", (2200, 3400, 2700), (340, 340, 600), color=ruin, count=9, spacing=600))
    B.append(blk("Colonnade", "RuinCols_S", (2200, -1000, 2700), (340, 340, 600), color=ruin, count=9, spacing=600))
    B.append(blk("Arch", "RuinArch_W", (-500, 1200, 2750), (700, 400, 700), rot=yaw(90), color=ruin,
                 hole_w=380, hole_h=560))
    B.append(blk("Arch", "RuinArch_E", (4900, 1200, 2750), (700, 400, 700), rot=yaw(90), color=ruin,
                 hole_w=380, hole_h=560))
    # 半塌墙体（楔形 + 墙体组合）
    B.append(blk("Wall", "RuinWall_Frag1", (600, 2000, 2700), (1400, 40, 600), color=ruin))
    B.append(blk("Wedge", "RuinWall_Frag2", (600, 2600, 2620), (1400, 400, 300), color=moss))
    B.append(blk("Wall", "RuinWall_Frag3", (3900, 2200, 2700), (1200, 40, 600), color=ruin))
    # 神庙高塔（最终目标，带外螺旋楼梯 + 楼梯间）
    B.append(blk("Cylinder", "Temple", (4200, 2600, 3400), (1600, 1600, 2000), color=ruin, segments=24))
    B.append(blk("Staircase", "TempleStair", (4200, 900, 2550), (900, 1800, 450), color=ruin, steps=14))
    B.append(blk("Stairwell", "TempleStairwell", (2900, 2600, 3000), (700, 1500, 900), color=ruin, steps=12, thickness=35))
    B.append(blk("Floor", "TempleTop", (4200, 2600, 4450), (1700, 1700, 40), color=moss))
    B.append(blk("Wedge", "TempleRoof", (4200, 2600, 4750), (1500, 1500, 550), color=ruin))
    # 洞穴捷径（北侧，门洞墙模拟洞口 + 下行坡道）
    B.append(blk("Wall", "CaveWall", (-500, 3400, 2800), (2400, 60, 800), color=rock))
    B.append(blk("DoorWall", "CaveMouth", (-500, 3400, 2680), (1200, 80, 600), color=rock,
                 hole_w=500, hole_h=450))
    B.append(blk("Ramp", "CaveRamp", (-500, 2400, 2200), (1000, 1800, 800), color=rock))
    B.append(blk("Floor", "CaveFloor", (-500, 1200, 1790), (1200, 2000, 20), color=(0.25,0.23,0.20)))
    # 峡谷底部河流面（视觉锚点）
    B.append(blk("Floor", "Riverbed", (0, 2200, -10), (9000, 6000, 20), color=(0.22,0.28,0.32), tiling=6))
    # 起点吊桥桥板
    for i in range(8):
        B.append(blk("Cube", f"Bridge_{i}", (-2500 + i*280, -2600 + i*160, 120), (240, 620, 60), color=ruin))
    return B

if __name__ == "__main__":
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Content", "Levels")
    os.makedirs(outdir, exist_ok=True)
    save("level_souls_castle.blockout.json", level_souls(), outdir)
    save("level_cs_dustyard.blockout.json", level_cs(), outdir)
    save("level_uncharted_ruins.blockout.json", level_uncharted(), outdir)
