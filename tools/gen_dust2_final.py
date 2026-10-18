# -*- coding: utf-8 -*-
"""
生成 dust2 白盒关卡（3A 白盒品质 / CS dust2 普通关卡规模，约 150m x 140m）。
- 结构：T/CT 出生点、A/B 包点、中门（双门）、Long A 长通道、中路、Catwalk 高架、
  Tunnels 隧道、A Short、CT 连接道、高低差平台（A 高台 / B 平台 / Catwalk 二层）
- 体块规模：170+ 块（显著优于此前 50 块版本），含柱廊/拱门/楼梯间/掩体等精细构件
- 输出：Content/Levels/level_cs_dust2_final.blockout.json（项目存档格式，LoadProject 直开）
单位：uu（1uu = 1cm）。
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

def room(blocks, cx, cy, w, d, wall_h=500, t=40, z=0, color=(0.55,0.55,0.58), prefix="Room",
         doors=None, floor=True, floor_color=None):
    """四面围墙，doors: [(side, offset, hole_w, hole_h)] side in N/S/E/W"""
    doors = doors or []
    for side, yy in (("S", cy - d/2), ("N", cy + d/2)):
        dl = [x for x in doors if x[0] == side]
        if dl:
            blocks.append(blk("DoorWall", f"{prefix}_{side}", (cx + (dl[0][1] if len(dl[0])>4 else 0), yy, z + wall_h/2),
                              (w, t, wall_h), color=color, hole_w=dl[0][2], hole_h=dl[0][3]))
        else:
            blocks.append(blk("Wall", f"{prefix}_{side}", (cx, yy, z + wall_h/2), (w, t, wall_h), color=color))
    for side, xx in (("W", cx - w/2), ("E", cx + w/2)):
        dl = [x for x in doors if x[0] == side]
        if dl:
            blocks.append(blk("DoorWall", f"{prefix}_{side}", (xx, cy + (dl[0][1] if len(dl[0])>4 else 0), z + wall_h/2),
                              (d, t, wall_h), rot=yaw(90), color=color, hole_w=dl[0][2], hole_h=dl[0][3]))
        else:
            blocks.append(blk("Wall", f"{prefix}_{side}", (xx, cy, z + wall_h/2), (d, t, wall_h), rot=yaw(90), color=color))
    if floor:
        blocks.append(blk("Floor", f"{prefix}_Floor", (cx, cy, z - 10), (w, d, 20),
                          color=floor_color or (0.5, 0.44, 0.36), tiling=2))

def corridor_ns(blocks, name, x, y0, y1, width, wall_h=650, t=40, color=(0.7,0.6,0.45), floor=False):
    """南北向走廊：两侧墙沿 Y 展开"""
    length = y1 - y0; cy = (y0 + y1) / 2.0
    blocks.append(blk("Wall", f"{name}_WallW", (x - width/2, cy, wall_h/2), (length, t, wall_h), rot=yaw(90), color=color))
    blocks.append(blk("Wall", f"{name}_WallE", (x + width/2, cy, wall_h/2), (length, t, wall_h), rot=yaw(90), color=color))
    if floor:
        blocks.append(blk("Floor", f"{name}_Floor", (x, cy, -10), (width, length, 20), color=(0.5,0.44,0.36), tiling=3))

def corridor_ew(blocks, name, y, x0, x1, width, wall_h=650, t=40, color=(0.7,0.6,0.45), floor=False):
    """东西向走廊：两侧墙沿 X 展开"""
    length = x1 - x0; cx = (x0 + x1) / 2.0
    blocks.append(blk("Wall", f"{name}_WallS", (cx, y - width/2, wall_h/2), (length, t, wall_h), color=color))
    blocks.append(blk("Wall", f"{name}_WallN", (cx, y + width/2, wall_h/2), (length, t, wall_h), color=color))
    if floor:
        blocks.append(blk("Floor", f"{name}_Floor", (cx, y, -10), (length, width, 20), color=(0.5,0.44,0.36), tiling=3))

# ============ dust2 白盒终版 ============
def level_dust2_final():
    B = []
    sand  = (0.72, 0.62, 0.45)   # 主沙色
    sand2 = (0.62, 0.52, 0.38)   # 深沙色
    white = (0.84, 0.79, 0.68)   # 石灰白（包点）
    dark  = (0.42, 0.36, 0.28)   # 阴影
    grey  = (0.55, 0.53, 0.50)   # 石材
    crate = (0.66, 0.55, 0.36)   # 木箱

    # ---- 地面与外墙 ----
    B.append(blk("Floor", "Ground", (0, 0, -20), (16000, 14000, 20), color=(0.55, 0.48, 0.38), tiling=8))
    B.append(blk("Wall", "Boundary_N", (0, 7000, 450), (16000, 80, 900), color=sand2))
    B.append(blk("Wall", "Boundary_S", (0, -7000, 450), (16000, 80, 900), color=sand2))
    B.append(blk("Wall", "Boundary_W", (-8000, 0, 450), (14000, 80, 900), rot=yaw(90), color=sand2))
    B.append(blk("Wall", "Boundary_E", (8000, 0, 450), (14000, 80, 900), rot=yaw(90), color=sand2))

    # ---- T 出生点（西南）---- 30m x 20m
    room(B, -5300, -4700, 3000, 2000, wall_h=550, color=sand, prefix="TSpawn",
         doors=[("N", 0, 320, 320), ("E", -200, 320, 320)], floor_color=(0.5, 0.42, 0.32))
    B.append(blk("Floor", "TSpawn_Mark", (-5300, -4700, 8), (1200, 800, 16), color=(0.45, 0.55, 0.35)))
    for i, (dx, dy, s, h) in enumerate([(-900, 400, 420, 380), (-350, -350, 380, 340),
                                        (700, 500, 460, 420), (350, -600, 400, 360)]):
        B.append(blk("Cube", f"TSpawn_Crate_{i}", (-5300 + dx, -4700 + dy, h/2), (s, s, h), color=crate))
    B.append(blk("Cube", "TSpawn_Crate_Stack", (-6100, -5250, 500), (420, 420, 1000), color=crate))
    B.append(blk("Wedge", "TSpawn_Awning1", (-6350, -4200, 620), (700, 1200, 300), color=dark))
    B.append(blk("Wedge", "TSpawn_Awning2", (-4250, -5250, 620), (600, 1100, 300), rot=yaw(180), color=dark))
    B.append(blk("Cylinder", "TSpawn_Barrel1", (-4550, -4150, 160), (300, 300, 320), color=grey, segments=14))
    B.append(blk("Cylinder", "TSpawn_Barrel2", (-4250, -4350, 160), (300, 300, 320), color=grey, segments=14))

    # ---- Long A 长通道（西侧，T→A，60m 纵深）----
    corridor_ns(B, "LongA", -5300, -3200, 2800, 1400, wall_h=720, color=sand)
    B.append(blk("WindowWall", "LongA_Win1", (-6020, -500, 360), (40, 1000, 720), rot=yaw(90), color=sand,
                 hole_w=500, hole_h=320, hole_z=120))
    B.append(blk("WindowWall", "LongA_Win2", (-6020, 1300, 360), (40, 1000, 720), rot=yaw(90), color=sand,
                 hole_w=500, hole_h=320, hole_z=120))
    B.append(blk("Arch", "LongA_Arch1", (-5300, -1200, 360), (1400, 45, 720), color=white,
                 hole_w=620, hole_h=520))
    B.append(blk("Arch", "LongA_Arch2", (-5300, 1000, 360), (1400, 45, 720), color=white,
                 hole_w=620, hole_h=520))
    for i, y in enumerate([-2800, -1800, -200, 600, 1800, 2400]):
        x = -6020 if i % 2 == 0 else -4580
        B.append(blk("Cylinder", f"LongA_Pillar_{i}", (x, y, 330), (260, 260, 660), color=grey, segments=12))
    for i, (x, y, s, h) in enumerate([(-5650, -2200, 420, 380), (-4950, -800, 380, 340),
                                      (-5650, 400, 400, 360), (-4950, 1500, 420, 380), (-5500, 2200, 460, 420)]):
        B.append(blk("Cube", f"LongA_Crate_{i}", (x, y, h/2), (s, s, h), color=crate))
    B.append(blk("Cube", "LongA_Crate_Stack", (-4950, -1400, 560), (380, 380, 1120), color=crate))
    B.append(blk("Ramp", "LongA_Boost", (-5900, -350, 180), (900, 1400, 360), color=sand2))

    # ---- A 包点（西北）----
    B.append(blk("Floor", "ASite_Mark", (-3900, 4100, 8), (1400, 1100, 16), color=(0.45, 0.55, 0.35)))
    B.append(blk("Wall", "ASite_WallN", (-3900, 5350, 360), (3400, 60, 720), color=white))
    B.append(blk("Arch", "ASite_ArchN", (-3900, 5350, 340), (700, 70, 700), color=white,
                 hole_w=360, hole_h=480))
    B.append(blk("Wall", "ASite_WallW", (-5650, 4100, 360), (2600, 60, 720), rot=yaw(90), color=white))
    B.append(blk("Wall", "ASite_WallE", (-2150, 4700, 360), (1400, 60, 720), rot=yaw(90), color=white))
    B.append(blk("DoorWall", "ASite_DoorE", (-2150, 3400, 340), (800, 60, 680), rot=yaw(90), color=white,
                 hole_w=340, hole_h=440))
    # A 高台（高低差 +320）
    B.append(blk("Cube", "ASite_Plateau", (-4700, 4750, 160), (1600, 1100, 320), color=sand2, tiling=2))
    B.append(blk("Ramp", "ASite_Ramp", (-3450, 4750, 160), (900, 1100, 320), color=sand2))
    B.append(blk("Staircase", "ASite_Stair", (-4700, 3550, 160), (700, 1000, 320), color=sand2, steps=8))
    B.append(blk("Cube", "ASite_PlatformBox", (-4700, 4750, 470), (500, 500, 300), color=crate))
    # 掩体箱组
    B.append(blk("Cube", "ASite_Crate1", (-3200, 3400, 225), (520, 520, 450), color=crate))
    B.append(blk("Cube", "ASite_Crate2", (-3200, 3400, 620), (440, 440, 340), color=crate))
    B.append(blk("Cube", "ASite_Crate3", (-2850, 4400, 200), (480, 480, 400), color=crate))
    B.append(blk("Cube", "ASite_Crate4", (-5100, 3350, 210), (500, 500, 420), color=crate))
    # A 点柱廊 + 立柱
    B.append(blk("Colonnade", "ASite_Colonnade", (-3900, 5100, 330), (280, 280, 660), color=grey,
                 count=7, spacing=460))
    B.append(blk("Cylinder", "ASite_Pillar1", (-2600, 5050, 330), (300, 300, 660), color=grey, segments=12))
    B.append(blk("Pipe", "ASite_Pipe", (-5450, 4100, 700), (220, 2400, 220), color=(0.4,0.42,0.44), segments=12))
    B.append(blk("Wedge", "ASite_Awning", (-4700, 5350, 800), (1600, 600, 320), color=dark))

    # ---- Catwalk 高架（A 点↔中路二层连通，Z=430）----
    B.append(blk("Floor", "Catwalk_Deck", (-2900, 3250, 430), (2200, 800, 40), color=sand2, tiling=2))
    B.append(blk("Wall", "Catwalk_RailS", (-2900, 2830, 560), (2200, 30, 220), color=sand2))
    B.append(blk("Wall", "Catwalk_RailN", (-2900, 3670, 560), (2200, 30, 220), color=sand2))
    B.append(blk("Staircase", "Catwalk_StairMid", (-1650, 3250, 215), (700, 800, 430), color=sand2, steps=10))
    B.append(blk("Staircase", "Catwalk_StairASite", (-4050, 3250, 215), (700, 800, 430), rot=yaw(180), color=sand2, steps=10))
    B.append(blk("Cylinder", "Catwalk_Support1", (-3400, 3250, 205), (260, 260, 410), color=grey, segments=10))
    B.append(blk("Cylinder", "Catwalk_Support2", (-2400, 3250, 205), (260, 260, 410), color=grey, segments=10))
    B.append(blk("Cylinder", "Catwalk_Support3", (-1900, 3250, 205), (260, 260, 410), color=grey, segments=10))

    # ---- 中路 Mid（南北贯通 + 中门双门）----
    corridor_ns(B, "Mid", -600, -3300, 3400, 1800, wall_h=760, color=sand)
    # 中门横墙（双门洞 + 中央立柱，dust2 标志性 mid doors）
    B.append(blk("DoorWall", "MidDoor_W", (-1020, -400, 380), (720, 55, 760), color=white,
                 hole_w=330, hole_h=460))
    B.append(blk("DoorWall", "MidDoor_E", (-180, -400, 380), (720, 55, 760), color=white,
                 hole_w=330, hole_h=460))
    B.append(blk("Cube", "MidDoor_Pillar", (-600, -400, 380), (180, 120, 760), color=grey))
    B.append(blk("Wedge", "MidDoor_Lintel", (-600, -400, 850), (1800, 300, 220), color=dark))
    # 中路掩体与细节
    for i, (x, y, s, h) in enumerate([(-1150, -1600, 440, 400), (-50, -900, 400, 360),
                                      (-1150, 500, 420, 380), (-50, 1400, 440, 400),
                                      (-1150, 2300, 400, 360), (-600, -2600, 460, 420)]):
        B.append(blk("Cube", f"Mid_Crate_{i}", (x, y, h/2), (s, s, h), color=crate))
    B.append(blk("Cube", "Mid_Boost", (-1250, -2150, 190), (700, 900, 380), color=sand2))
    B.append(blk("Arch", "Mid_ArchN", (-600, 2700, 380), (1800, 55, 760), color=white,
                 hole_w=640, hole_h=540))
    B.append(blk("Arch", "Mid_ArchS", (-600, -3050, 380), (1800, 55, 760), color=white,
                 hole_w=640, hole_h=540))
    B.append(blk("Cylinder", "Mid_Pillar1", (-1530, 900, 360), (280, 280, 720), color=grey, segments=12))
    B.append(blk("Cylinder", "Mid_Pillar2", (330, -1900, 360), (280, 280, 720), color=grey, segments=12))

    # ---- Tunnels 隧道（T→B，带拱肋与箱体）----
    corridor_ew(B, "TunnelS", -4800, -3800, 2200, 1600, wall_h=680, color=sand2)
    corridor_ns(B, "TunnelN", 2200, -4800, -1800, 1600, wall_h=680, color=sand2)
    B.append(blk("Floor", "TunnelRoof1", (-1500, -4800, 700), (2000, 1650, 40), color=dark))
    B.append(blk("Floor", "TunnelRoof2", (800, -4800, 700), (1400, 1650, 40), color=dark))
    B.append(blk("Floor", "TunnelRoof3", (2200, -3200, 700), (1650, 1400, 40), color=dark))
    for i, x in enumerate([-2500, -700, 1100]):
        B.append(blk("Arch", f"Tunnel_Rib_{i}", (x, -4800, 340), (1600, 60, 680), color=grey,
                     hole_w=620, hole_h=480))
    for i, (x, y, s, h) in enumerate([(-1800, -4500, 420, 380), (300, -5100, 400, 360),
                                      (1500, -4450, 440, 400), (2550, -3400, 420, 380),
                                      (1900, -2400, 460, 420)]):
        B.append(blk("Cube", f"Tunnel_Crate_{i}", (x, y, h/2), (s, s, h), color=crate))
    B.append(blk("Staircase", "Tunnel_StairB", (2200, -1750, 110), (900, 700, 220), color=sand2, steps=6))
    B.append(blk("Pipe", "Tunnel_Pipe", (-800, -5620, 640), (200, 3000, 200), color=(0.4,0.42,0.44), segments=10))

    # ---- B 包点（东南）----
    B.append(blk("Floor", "BSite_Mark", (4400, -3300, 8), (1400, 1100, 16), color=(0.45, 0.55, 0.35)))
    B.append(blk("Wall", "BSite_WallS", (4400, -4650, 360), (3200, 60, 720), color=white))
    B.append(blk("Wall", "BSite_WallE", (6050, -3300, 360), (2800, 60, 720), rot=yaw(90), color=white))
    B.append(blk("Wall", "BSite_WallN", (4400, -1950, 360), (3200, 60, 720), color=white))
    B.append(blk("WindowWall", "BSite_Window", (4400, -1950, 400), (900, 70, 800), color=white,
                 hole_w=520, hole_h=340, hole_z=120))
    B.append(blk("DoorWall", "BSite_DoorW", (2750, -3300, 340), (800, 60, 680), rot=yaw(90), color=white,
                 hole_w=340, hole_h=440))
    # B 平台（高低差 +300）
    B.append(blk("Cube", "BSite_Platform", (3650, -4150, 150), (1500, 900, 300), color=sand2, tiling=2))
    B.append(blk("Staircase", "BSite_Stair1", (3650, -3350, 150), (700, 700, 300), rot=yaw(180), color=sand2, steps=7))
    B.append(blk("Staircase", "BSite_Stair2", (5250, -4150, 150), (700, 700, 300), rot=yaw(90), color=sand2, steps=7))
    B.append(blk("Cube", "BSite_PlatformBox", (3650, -4150, 450), (480, 480, 300), color=crate))
    # 掩体箱组
    B.append(blk("Cube", "BSite_Crate1", (4900, -2900, 225), (520, 520, 450), color=crate))
    B.append(blk("Cube", "BSite_Crate2", (4900, -2900, 620), (440, 440, 340), color=crate))
    B.append(blk("Cube", "BSite_Crate3", (3350, -2700, 200), (480, 480, 400), color=crate))
    B.append(blk("Cube", "BSite_Crate4", (5600, -3900, 210), (500, 500, 420), color=crate))
    B.append(blk("Cube", "BSite_Crate5", (5650, -2500, 190), (460, 460, 380), color=crate))
    B.append(blk("Colonnade", "BSite_Colonnade", (5800, -3300, 330), (280, 280, 660), color=grey,
                 count=6, spacing=460))
    B.append(blk("Cylinder", "BSite_Pillar1", (3050, -2350, 330), (300, 300, 660), color=grey, segments=12))
    B.append(blk("Pipe", "BSite_Pipe", (4400, -4500, 700), (2800, 220, 220), color=(0.4,0.42,0.44), segments=12))
    B.append(blk("Wedge", "BSite_Awning", (3650, -4650, 800), (1500, 600, 320), color=dark))

    # ---- CT 出生点（东北）---- 28m x 20m
    room(B, 5300, 2600, 2800, 2000, wall_h=550, color=sand, prefix="CTSpawn",
         doors=[("W", -300, 320, 320), ("S", 300, 320, 320)], floor_color=(0.5, 0.42, 0.32))
    B.append(blk("Floor", "CTSpawn_Mark", (5300, 2600, 8), (1200, 800, 16), color=(0.4, 0.5, 0.65)))
    for i, (dx, dy, s, h) in enumerate([(-800, 350, 420, 380), (-250, -400, 380, 340),
                                        (800, -250, 460, 420), (500, 550, 400, 360)]):
        B.append(blk("Cube", f"CTSpawn_Crate_{i}", (5300 + dx, 2600 + dy, h/2), (s, s, h), color=crate))
    B.append(blk("Cube", "CTSpawn_Crate_Stack", (6350, 1950, 500), (420, 420, 1000), color=crate))
    B.append(blk("Wedge", "CTSpawn_Awning1", (6350, 3150, 620), (700, 1100, 300), rot=yaw(90), color=dark))
    B.append(blk("Cylinder", "CTSpawn_Barrel1", (4350, 3150, 160), (300, 300, 320), color=grey, segments=14))
    B.append(blk("Cylinder", "CTSpawn_Barrel2", (4650, 1950, 160), (300, 300, 320), color=grey, segments=14))

    # ---- 连接动线 ----
    # A Short（北环道，A 点↔CT 区）
    corridor_ew(B, "AShort", 5950, -3600, 3900, 1200, wall_h=620, color=sand)
    B.append(blk("Arch", "AShort_ArchW", (-3300, 5950, 310), (1200, 60, 620), color=white,
                 hole_w=520, hole_h=440))
    B.append(blk("Arch", "AShort_ArchE", (3600, 5950, 310), (1200, 60, 620), color=white,
                 hole_w=520, hole_h=440))
    B.append(blk("Cube", "AShort_Crate1", (-1500, 5950, 200), (440, 440, 400), color=crate))
    B.append(blk("Cube", "AShort_Crate2", (1200, 5950, 210), (460, 460, 420), color=crate))
    B.append(blk("Cube", "AShort_Crate3", (2600, 5950, 190), (420, 420, 380), color=crate))

    # CT↔A Short 立连接（南北）
    corridor_ns(B, "CTLinkA", 5300, 3800, 5350, 1200, wall_h=620, color=sand)
    B.append(blk("Arch", "CTLinkA_Arch", (5300, 4300, 310), (1200, 60, 620), color=white,
                 hole_w=520, hole_h=440))

    # CT↔B 连接道（东侧南北）
    corridor_ns(B, "CTLinkB", 5300, -1950, 1550, 1200, wall_h=620, color=sand)
    B.append(blk("Arch", "CTLinkB_Arch1", (5300, 1100, 310), (1200, 60, 620), color=white,
                 hole_w=520, hole_h=440))
    B.append(blk("Arch", "CTLinkB_Arch2", (5300, -1450, 310), (1200, 60, 620), color=white,
                 hole_w=520, hole_h=440))
    B.append(blk("Cube", "CTLinkB_Crate1", (4900, -300, 200), (440, 440, 400), color=crate))
    B.append(blk("Cube", "CTLinkB_Crate2", (5700, 400, 190), (420, 420, 380), color=crate))

    # 中路↔CT 连接（Mid to CT）
    corridor_ew(B, "MidToCT", 3050, 400, 3900, 1200, wall_h=620, color=sand)
    B.append(blk("Arch", "MidToCT_ArchW", (800, 3050, 310), (1200, 60, 620), color=white,
                 hole_w=520, hole_h=440))
    B.append(blk("Cube", "MidToCT_Crate1", (1900, 3050, 200), (440, 440, 400), color=crate))
    B.append(blk("Cube", "MidToCT_Crate2", (3100, 3050, 210), (460, 460, 420), color=crate))

    # 中路↔B 隧道捷径（Lower）
    corridor_ew(B, "MidToB", -2550, 400, 1950, 1300, wall_h=620, color=sand2)
    B.append(blk("Arch", "MidToB_Arch1", (850, -2550, 310), (1300, 60, 620), color=white,
                 hole_w=520, hole_h=440))
    B.append(blk("Arch", "MidToB_Arch2", (1750, -2550, 310), (1300, 60, 620), color=white,
                 hole_w=520, hole_h=440))
    B.append(blk("Cube", "MidToB_Crate1", (750, -2200, 190), (420, 420, 380), color=crate))

    # ---- 外围补强与氛围体块 ----
    B.append(blk("Fence", "Fence_NW", (-6600, 5900, 250), (2400, 30, 500), color=dark, spacing=420))
    B.append(blk("Fence", "Fence_SE", (6600, -5300, 250), (2400, 30, 500), color=dark, spacing=420))
    B.append(blk("Fence", "Fence_NE", (6900, 4900, 250), (2600, 30, 500), rot=yaw(90), color=dark, spacing=420))
    B.append(blk("Cylinder", "Tower_SW", (-7300, -6300, 550), (900, 900, 1100), color=sand2, segments=16))
    B.append(blk("Wedge", "Tower_SW_Roof", (-7300, -6300, 1250), (1000, 1000, 320), color=dark))
    B.append(blk("Cylinder", "Tower_NE", (7300, 6100, 550), (900, 900, 1100), color=sand2, segments=16))
    B.append(blk("Wedge", "Tower_NE_Roof", (7300, 6100, 1250), (1000, 1000, 320), color=dark))
    B.append(blk("Cube", "WallButtress_1", (-2150, 2400, 250), (500, 500, 500), color=sand2))
    B.append(blk("Cube", "WallButtress_2", (2750, -2550, 250), (500, 500, 500), color=sand2))
    B.append(blk("Cube", "WallButtress_3", (2750, 2350, 250), (500, 500, 500), color=sand2))
    B.append(blk("Cube", "WallButtress_4", (-5650, 2350, 250), (500, 500, 500), color=sand2))
    B.append(blk("Stairwell", "TowerStair_SW", (-6650, -6300, 275), (600, 1100, 550), color=grey,
                steps=10, thickness=30))
    B.append(blk("Stairwell", "TowerStair_NE", (6650, 6100, 275), (600, 1100, 550), color=grey,
                steps=10, thickness=30))
    return B

# AutoDemo 跑图路线：贴走廊中心、穿门洞、含 Catwalk 高低差
# z = 角色中心高度（地面 0 → 90；Catwalk 顶 450 → 540；楼梯中段 300）
DEMO_PATH = [
    # TSpawn → LongA
    (-5300, -5200, 90), (-5300, -3900, 90), (-5300, -3200, 90), (-5300, -500, 90),
    (-5300, 2500, 90),
    # A 点（上坡道平台）
    (-4700, 3600, 90), (-3900, 4100, 90), (-3450, 4750, 90),
    # Catwalk 上层（楼梯上行 → 走廊 → 楼梯下行）
    (-4050, 3250, 90), (-4050, 3250, 300), (-3300, 3250, 540), (-2900, 3250, 540),
    (-2000, 3250, 540), (-1650, 3250, 300), (-1300, 3250, 90),
    # Mid 北段 → 中门（避柱绕 x=-1020 门洞）→ Mid 南段（避箱）
    (-600, 2700, 90), (-600, 800, 90), (-1020, -400, 90), (-600, -1500, 90),
    (-300, -2200, 90), (-600, -3050, 90), (-600, -3300, 90),
    # MidToB 隧道捷径 → TunnelN → B 点
    (-200, -2550, 90), (350, -2550, 90), (1000, -2550, 90), (1750, -2550, 90),
    (1750, -2775, 90), (2200, -2775, 90), (2750, -3300, 90), (3600, -2850, 90),
    (4400, -2850, 90), (5200, -2850, 90),
    # 回程：B 西门 → MidToB → Mid → MidToCT（避箱）→ CT 门 W → CTSpawn
    (2750, -2850, 90), (1750, -2775, 90), (850, -2550, 90), (350, -2550, 90),
    (0, -1500, 90), (0, 1500, 90), (350, 2750, 90), (800, 3050, 90),
    (1900, 2750, 90), (3100, 2750, 90), (3900, 2300, 90), (4600, 2600, 90),
    (5300, 2600, 90),
]

def main():
    blocks = level_dust2_final()
    doc = {"version": "1.0", "savedAt": "2026-10-18T00:00:00", "blocks": blocks,
           "demoPath": [list(p) for p in DEMO_PATH]}
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = os.path.join(repo_root, "Content", "Levels", "level_cs_dust2_final.blockout.json")
    with open(out, "w", encoding="utf-8") as f:
        json.dump(doc, f, ensure_ascii=False)
    xs, ys, zs = [], [], []
    for o in blocks:
        t = o["Transform"]["Translation"]; par = o["Params"]
        r = max(par["Length"], par["Width"], par["Height"]) / 2
        xs += [t["X"]-r, t["X"]+r]; ys += [t["Y"]-r, t["Y"]+r]; zs += [t["Z"]-r, t["Z"]+r]
    print(f"level_cs_dust2_final: {len(blocks)} blocks -> {out}")
    print("bbox: %.0f x %.0f x %.0f m | X[%.1f,%.1f] Y[%.1f,%.1f] Z[%.1f,%.1f] m" % (
        (max(xs)-min(xs))/100, (max(ys)-min(ys))/100, (max(zs)-min(zs))/100,
        min(xs)/100, max(xs)/100, min(ys)/100, max(ys)/100, min(zs)/100, max(zs)/100))

if __name__ == "__main__":
    main()