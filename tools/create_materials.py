# -*- coding: utf-8 -*-
"""
create_materials.py — 生成 M_BlockoutBase 主材质资产（P3）
运行方式：
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<本文件>"

参数约定（与 BlockoutActor::EnsureMID / ApplyAppearance 对齐）：
  BaseColor        VectorParameter  基础颜色
  Tiling           ScalarParameter  纹理平铺密度
  Pattern          TextureSampleParameter2D  图案纹理（棋盘格/网格/砖墙等，运行时由 BlockoutTextureLib 注入）
  PatternStrength  ScalarParameter  图案叠加强度（0=纯色）
  Highlight        ScalarParameter  选中高亮强度
  HighlightColor   VectorParameter  高亮颜色
  Roughness/Metallic ScalarParameter 粗糙度/金属度
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

def build(mat):
    # ---- 参数 ----
    p_base = vector(mat, 'BaseColor', (0.75, 0.75, 0.75, 1.0), -900, -300)
    p_tiling = scalar(mat, 'Tiling', 4.0, -900, -100)
    p_pattern = L.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, 100)
    p_pattern.set_editor_property('parameter_name', 'Pattern')
    p_strength = scalar(mat, 'PatternStrength', 0.0, -900, 100)
    p_hl = scalar(mat, 'Highlight', 0.0, -900, 300)
    p_hlc = vector(mat, 'HighlightColor', (1.0, 0.85, 0.2, 1.0), -900, 450)
    p_rough = scalar(mat, 'Roughness', 0.7, -900, 600)
    p_metal = scalar(mat, 'Metallic', 0.0, -900, 750)

    # ---- UV: TexCoord * Tiling -> Pattern.UVs ----
    uv = L.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    mul_uv = L.create_material_expression(mat, unreal.MaterialExpressionMultiply, -750, 0)
    L.connect_material_expressions(uv, '', mul_uv, 'A')
    L.connect_material_expressions(p_tiling, '', mul_uv, 'B')
    L.connect_material_expressions(mul_uv, '', p_pattern, 'UVs')

    # ---- 图案调制：BaseColor * PatternRGB，按 PatternStrength Lerp 混合 ----
    mul_pat = L.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, -100)
    L.connect_material_expressions(p_base, '', mul_pat, 'A')
    L.connect_material_expressions(p_pattern, 'RGB', mul_pat, 'B')
    lerp = L.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -150, -150)
    L.connect_material_expressions(p_base, '', lerp, 'A')
    L.connect_material_expressions(mul_pat, '', lerp, 'B')
    L.connect_material_expressions(p_strength, '', lerp, 'Alpha')
    L.connect_material_property(lerp, '', MP.MP_BASE_COLOR)

    # ---- 高亮：Emissive = BaseColor? -> HighlightColor * Highlight ----
    mul_hl = L.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 300)
    L.connect_material_expressions(p_hlc, '', mul_hl, 'A')
    L.connect_material_expressions(p_hl, '', mul_hl, 'B')
    L.connect_material_property(mul_hl, '', MP.MP_EMISSIVE_COLOR)

    # ---- Roughness / Metallic ----
    L.connect_material_property(p_rough, '', MP.MP_ROUGHNESS)
    L.connect_material_property(p_metal, '', MP.MP_METALLIC)

    L.recompile_material(mat)

def main():
    mat = make_material()
    build(mat)
    unreal.EditorAssetLibrary.save_asset(MAT_PATH)
    unreal.log('[create_materials] %s saved, expressions=%d' % (
        MAT_PATH, L.get_num_material_expressions(mat)))

if __name__ == '__main__':
    main()
