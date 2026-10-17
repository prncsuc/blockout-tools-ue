# -*- coding: utf-8 -*-
"""
create_materials.py — 生成 M_BlockoutBase 主材质资产（P3 + 立体感增强版）
运行方式：
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<本文件>"

参数约定（与 BlockoutActor::EnsureMID / ApplyAppearance 对齐）：
  BaseColor        VectorParameter  基础颜色
  Tiling           ScalarParameter  纹理平铺密度
  Pattern          TextureSampleParameter2D  图案纹理（运行时由 BlockoutTextureLib 注入）
  PatternStrength  ScalarParameter  图案叠加强度（0=纯色）
  Highlight        ScalarParameter  选中高亮强度
  HighlightColor   VectorParameter  高亮颜色
  Roughness/Metallic ScalarParameter 粗糙度/金属度
立体感增强参数（新增）：
  WorldAligned     ScalarParameter  1=世界对齐三平面 UV（防拉伸/闪烁），0=模型 UV
  TopColor         VectorParameter  顶面提亮色
  TopStrength      ScalarParameter  顶面提亮强度（体积感）
"""
import unreal

MAT_PATH = '/Game/BlockoutStudio/Materials/M_BlockoutBase'
MAT_PACKAGE = '/Game/BlockoutStudio/Materials'

L = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty

def make_material():
    if unreal.EditorAssetLibrary.does_asset_exist(MAT_PATH):
        unreal.EditorAssetLibrary.delete_asset(MAT_PATH)
    unreal.EditorAssetLibrary.make_directory(MAT_PACKAGE)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = asset_tools.create_asset('M_BlockoutBase', MAT_PACKAGE,
                                  unreal.Material, unreal.MaterialFactoryNew())
    assert mat, 'create material failed'
    return mat

def scalar(mat, name, value, x, y):
    e = L.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property('parameter_name', name)
    e.set_editor_property('default_value', float(value))
    return e

def vector(mat, name, rgba, x, y):
    e = L.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property('parameter_name', name)
    e.set_editor_property('default_value', unreal.LinearColor(*rgba))
    return e

def expr(mat, cls, x, y):
    return L.create_material_expression(mat, cls, x, y)

def swizzle(mat, src, out_name, x, y):
    """ComponentMask 提取通道"""
    e = expr(mat, unreal.MaterialExpressionComponentMask, x, y)
    e.set_editor_property('r', 'R' in out_name)
    e.set_editor_property('g', 'G' in out_name)
    e.set_editor_property('b', 'B' in out_name)
    e.set_editor_property('a', 'A' in out_name)
    L.connect_material_expressions(src, '', e, '')
    return e

def append2(mat, a, b, x, y):
    e = expr(mat, unreal.MaterialExpressionAppendVector, x, y)
    L.connect_material_expressions(a, '', e, 'A')
    L.connect_material_expressions(b, '', e, 'B')
    return e

def mul(mat, a, b, x, y):
    e = expr(mat, unreal.MaterialExpressionMultiply, x, y)
    L.connect_material_expressions(a, '', e, 'A')
    L.connect_material_expressions(b, '', e, 'B')
    return e

def build(mat):
    # ================= 基础参数 =================
    p_base = vector(mat, 'BaseColor', (0.75, 0.75, 0.75, 1.0), -1600, -300)
    p_tiling = scalar(mat, 'Tiling', 4.0, -1600, -100)
    p_pattern = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 100)
    p_pattern.set_editor_property('parameter_name', 'Pattern')
    p_pattern_xy = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 350)
    p_pattern_xy.set_editor_property('parameter_name', 'Pattern')
    p_pattern_xz = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 600)
    p_pattern_xz.set_editor_property('parameter_name', 'Pattern')
    p_strength = scalar(mat, 'PatternStrength', 0.0, -1600, 100)
    p_hl = scalar(mat, 'Highlight', 0.0, -1600, 900)
    p_hlc = vector(mat, 'HighlightColor', (1.0, 0.85, 0.2, 1.0), -1600, 1050)
    p_rough = scalar(mat, 'Roughness', 0.7, -1600, 1200)
    p_metal = scalar(mat, 'Metallic', 0.0, -1600, 1350)
    p_world = scalar(mat, 'WorldAligned', 1.0, -1600, -600)
    p_topcolor = vector(mat, 'TopColor', (1.25, 1.25, 1.25, 1.0), -1600, -800)
    p_topstr = scalar(mat, 'TopStrength', 0.35, -1600, -950)

    # ================= 世界对齐三平面 UV =================
    wp = expr(mat, unreal.MaterialExpressionWorldPosition, -1600, 400)
    wp_scaled = mul(mat, wp, p_tiling, -1400, 400)   # WorldPos * Tiling
    wx = swizzle(mat, wp_scaled, 'R', -1250, 300)
    wy = swizzle(mat, wp_scaled, 'G', -1250, 450)
    wz = swizzle(mat, wp_scaled, 'B', -1250, 600)
    uv_yz = append2(mat, wy, wz, -1100, 300)   # 侧面投影
    uv_xz = append2(mat, wx, wz, -1100, 450)   # 顶/底面投影
    uv_xy = append2(mat, wx, wy, -1100, 600)   # 前后投影
    L.connect_material_expressions(uv_yz, '', p_pattern, 'UVs')
    L.connect_material_expressions(uv_xz, '', p_pattern_xz, 'UVs')
    L.connect_material_expressions(uv_xy, '', p_pattern_xy, 'UVs')

    # 三平面权重：|WorldNormal| 归一化
    nrm = expr(mat, unreal.MaterialExpressionVertexNormalWS, -1600, 700)
    nabs = expr(mat, unreal.MaterialExpressionAbs, -1450, 700)
    L.connect_material_expressions(nrm, '', nabs, '')
    nx = swizzle(mat, nabs, 'R', -1300, 700)
    ny = swizzle(mat, nabs, 'G', -1300, 800)
    nz = swizzle(mat, nabs, 'B', -1300, 900)
    add_n = expr(mat, unreal.MaterialExpressionAdd, -1150, 750)
    L.connect_material_expressions(nx, '', add_n, 'A')
    L.connect_material_expressions(ny, '', add_n, 'B')
    add_n2 = expr(mat, unreal.MaterialExpressionAdd, -1050, 750)
    L.connect_material_expressions(add_n, '', add_n2, 'A')
    L.connect_material_expressions(nz, '', add_n2, 'B')
    inv_n = expr(mat, unreal.MaterialExpressionOneMinus, -950, 850)  # 占位防误连
    # 归一化：各权重 = n / (nx+ny+nz)
    def norm(comp, yv):
        d = expr(mat, unreal.MaterialExpressionDivide, -1050, yv)
        L.connect_material_expressions(comp, '', d, 'A')
        L.connect_material_expressions(add_n2, '', d, 'B')
        return d
    w_yz = norm(nx, 650)   # YZ 平面对应 X 法线
    w_xz = norm(ny, 850)   # XZ 平面对 Y
    w_xy = norm(nz, 1050)  # XY 平面对 Z

    # 三平面混合图案
    m1 = mul(mat, p_pattern, w_yz, -750, 200)
    m2 = mul(mat, p_pattern_xz, w_xz, -750, 400)
    m3 = mul(mat, p_pattern_xy, w_xy, -750, 600)
    s1 = expr(mat, unreal.MaterialExpressionAdd, -600, 300)
    L.connect_material_expressions(m1, '', s1, 'A')
    L.connect_material_expressions(m2, '', s1, 'B')
    s2 = expr(mat, unreal.MaterialExpressionAdd, -500, 300)
    L.connect_material_expressions(s1, '', s2, 'A')
    L.connect_material_expressions(m3, '', s2, 'B')

    # 模型 UV 备用图案（WorldAligned=0 时）
    uv = expr(mat, unreal.MaterialExpressionTextureCoordinate, -1250, 100)
    mul_uv = mul(mat, uv, p_tiling, -1150, 100)
    L.connect_material_expressions(mul_uv, '', p_pattern_xy, 'UVs')  # 保持三平面一致，旧路径直接用 s2

    # 世界对齐 / 模型 UV 切换（默认世界对齐）
    pat_lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -400, 350)
    L.connect_material_expressions(s2, '', pat_lerp, 'A')
    L.connect_material_expressions(s2, '', pat_lerp, 'B')
    L.connect_material_expressions(p_world, '', pat_lerp, 'Alpha')

    # ================= 图案调制 =================
    mul_pat = mul(mat, p_base, pat_lerp, -250, -100)
    lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -100, -150)
    L.connect_material_expressions(p_base, '', lerp, 'A')
    L.connect_material_expressions(mul_pat, '', lerp, 'B')
    L.connect_material_expressions(p_strength, '', lerp, 'Alpha')

    # ================= 顶面提亮（体积感） =================
    # TopFactor = saturate(WorldNormal.Z) * TopStrength；Base * (1 + TopFactor*(TopColor-1))
    nz_sat = expr(mat, unreal.MaterialExpressionSaturate, -750, 1050)
    L.connect_material_expressions(nz, '', nz_sat, '')
    top_f = mul(mat, nz_sat, p_topstr, -600, 1050)
    top_off = expr(mat, unreal.MaterialExpressionAdd, -450, 1150)  # TopColor - 1
    L.connect_material_expressions(p_topcolor, '', top_off, 'A')
    one_c = expr(mat, unreal.MaterialExpressionConstant3Vector, -600, 1250)
    one_c.set_editor_property('constant', unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    neg = expr(mat, unreal.MaterialExpressionMultiply, -500, 1300)
    L.connect_material_expressions(one_c, '', neg, 'A')
    neg.set_editor_property('const_b', -1.0)
    L.connect_material_expressions(neg, '', top_off, 'B')
    top_scale = mul(mat, top_f, top_off, -300, 1150)
    base_lit = expr(mat, unreal.MaterialExpressionAdd, -100, -250)
    L.connect_material_expressions(lerp, '', base_lit, 'A')
    L.connect_material_expressions(top_scale, '', base_lit, 'B')
    L.connect_material_property(base_lit, '', MP.MP_BASE_COLOR)

    # ================= 高亮 Emissive =================
    mul_hl = mul(mat, p_hlc, p_hl, -100, 700)
    L.connect_material_property(mul_hl, '', MP.MP_EMISSIVE_COLOR)

    # ================= Roughness / Metallic =================
    L.connect_material_property(p_rough, '', MP.MP_ROUGHNESS)
    L.connect_material_property(p_metal, '', MP.MP_METALLIC)

    # 无光照影响干扰：默认 lit，双面关闭
    mat.set_editor_property('two_sided', False)

    L.recompile_material(mat)

def main():
    mat = make_material()
    build(mat)
    unreal.EditorAssetLibrary.save_asset(MAT_PATH)
    unreal.log('[create_materials] %s saved, expressions=%d' % (
        MAT_PATH, L.get_num_material_expressions(mat)))

if __name__ == '__main__':
    main()
