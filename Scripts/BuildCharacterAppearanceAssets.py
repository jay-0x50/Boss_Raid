import unreal


DEST = "/Game/Characters/Exception/Materials"


def log(message):
    unreal.log(f"[BuildCharacterAppearanceAssets] {message}")


def load(path, expected):
    asset = unreal.load_asset(path)
    if not isinstance(asset, expected):
        raise RuntimeError(f"Missing {expected.__name__}: {path}")
    return asset


def material(name, translucent=False):
    unreal.EditorAssetLibrary.make_directory(DEST)
    path = f"{DEST}/{name}"
    result = unreal.load_asset(path)
    if not isinstance(result, unreal.Material):
        result = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, DEST, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(result, unreal.Material):
        raise RuntimeError(f"Could not create {path}")
    result.set_editor_property(
        "blend_mode",
        unreal.BlendMode.BLEND_TRANSLUCENT if translucent else unreal.BlendMode.BLEND_OPAQUE,
    )
    result.set_editor_property("two_sided", translucent)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(result)
    return result


def expr(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def vector_param(mat, name, value, x, y):
    node = expr(mat, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", unreal.LinearColor(*value))
    return node


def scalar_param(mat, name, value, x, y):
    node = expr(mat, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def texture_node(mat, path, x, y, normal=False):
    node = expr(mat, unreal.MaterialExpressionTextureSample, x, y)
    node.set_editor_property("texture", load(path, unreal.Texture2D))
    if normal:
        node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    return node


def connect(source, output_name, target, input_name):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(source, output_name, target, input_name):
        raise RuntimeError(f"Could not connect {source} to {target}.{input_name}")


def output(mat, source, output_name, prop):
    if not unreal.MaterialEditingLibrary.connect_material_property(source, output_name, prop):
        raise RuntimeError(f"Could not connect {source} to {prop}")


def add_code_edge(mat, code_color, glow_default, x=-60, y=180):
    fresnel = expr(mat, unreal.MaterialExpressionFresnel, x - 440, y)
    color = vector_param(mat, "CodeColor", code_color, x - 440, y + 120)
    glow = scalar_param(mat, "GlowStrength", glow_default, x - 440, y + 240)
    edge_color = expr(mat, unreal.MaterialExpressionMultiply, x - 220, y + 60)
    edge_glow = expr(mat, unreal.MaterialExpressionMultiply, x, y + 80)
    connect(fresnel, "", edge_color, "A")
    connect(color, "", edge_color, "B")
    connect(edge_color, "", edge_glow, "A")
    connect(glow, "", edge_glow, "B")
    return edge_glow


def build_hendel(name, base_path, normal_path, mra_path):
    mat = material(name)
    base = texture_node(mat, base_path, -720, -180)
    tint = vector_param(mat, "ArmorTint", (0.025, 0.045, 0.075, 1.0), -720, -30)
    tinted = expr(mat, unreal.MaterialExpressionMultiply, -440, -110)
    connect(base, "RGB", tinted, "A")
    connect(tint, "", tinted, "B")
    output(mat, tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    normal = texture_node(mat, normal_path, -720, 360, normal=True)
    output(mat, normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    mra = texture_node(mat, mra_path, -720, 520)
    output(mat, mra, "R", unreal.MaterialProperty.MP_METALLIC)
    output(mat, mra, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    output(mat, mra, "B", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    edge = add_code_edge(mat, (0.0, 0.72, 1.0, 1.0), 1.8, -80, 130)
    output(mat, edge, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    save(mat)


def build_nel(name, base_path, normal_path):
    mat = material(name, translucent=True)
    base = texture_node(mat, base_path, -760, -180)
    tint = vector_param(mat, "ArmorTint", (0.72, 0.86, 1.0, 1.0), -760, -20)
    tinted = expr(mat, unreal.MaterialExpressionMultiply, -500, -100)
    connect(base, "RGB", tinted, "A")
    connect(tint, "", tinted, "B")
    output(mat, tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    normal = texture_node(mat, normal_path, -760, 410, normal=True)
    output(mat, normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    edge = add_code_edge(mat, (0.58, 0.82, 1.0, 1.0), 3.2, -80, 100)
    output(mat, edge, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = scalar_param(mat, "Opacity", 0.78, -180, 430)
    output(mat, opacity, "", unreal.MaterialProperty.MP_OPACITY)
    roughness = scalar_param(mat, "Roughness", 0.32, -180, 520)
    output(mat, roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    save(mat)


def build_robe():
    mat = material("M_Nel_Robe", translucent=True)
    tint = vector_param(mat, "ArmorTint", (0.68, 0.84, 1.0, 1.0), -520, -80)
    output(mat, tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    edge = add_code_edge(mat, (0.72, 0.90, 1.0, 1.0), 3.2, -60, 80)
    output(mat, edge, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = scalar_param(mat, "Opacity", 0.70, -180, 400)
    output(mat, opacity, "", unreal.MaterialProperty.MP_OPACITY)
    roughness = scalar_param(mat, "Roughness", 0.42, -180, 500)
    output(mat, roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    save(mat)


def save(mat):
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)


def main():
    base = "/Game/Characters/Mannequins/Textures"
    build_hendel(
        "M_Hendel_Armor01",
        f"{base}/Manny/T_Manny_01_D",
        f"{base}/Manny/T_Manny_01_BN",
        f"{base}/Manny/T_Manny_01_MRA",
    )
    build_hendel(
        "M_Hendel_Armor02",
        f"{base}/Manny/T_Manny_02_D",
        f"{base}/Manny/T_Manny_02_N",
        f"{base}/Manny/T_Manny_02_MRA",
    )
    build_nel(
        "M_Nel_Ghost01",
        f"{base}/Quinn/T_Quinn_01_D",
        f"{base}/Quinn/T_Quinn_01_N",
    )
    build_nel(
        "M_Nel_Ghost02",
        f"{base}/Quinn/T_Quinn_02_D",
        f"{base}/Quinn/T_Quinn_02_N",
    )
    build_robe()
    log("Created Handel dark-cyan armor and Nel translucent apparition materials.")


main()
