import unreal


ASSET_PATH = "/Game/GoHome/AI/Enemy/Monster/VoidSerpent/Material/M_Tentacle_Abyss_V3"
PACKAGE_PATH = "/Game/GoHome/AI/Enemy/Monster/VoidSerpent/Material"


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def scalar(material, name, value, x, y):
    node = expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def color(material, name, value, x, y):
    node = expression(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", unreal.LinearColor(*value, 1.0))
    return node


def multiply(material, a, b, x, y):
    node = expression(material, unreal.MaterialExpressionMultiply, x, y)
    unreal.MaterialEditingLibrary.connect_material_expressions(a, "", node, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(b, "", node, "B")
    return node


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        unreal.log_error("Asset already exists: " + ASSET_PATH)
        return

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(
        "M_Tentacle_Abyss_V3", PACKAGE_PATH, unreal.Material, unreal.MaterialFactoryNew()
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

    body = color(material, "BodyColor", (0.002, 0.008, 0.015), -1450, -220)
    glow = color(material, "GlowColor", (0.0, 0.35, 1.0), -1450, 80)
    emissive_strength = scalar(material, "EmissiveStrength", 8.0, -1450, 230)
    roughness = scalar(material, "Roughness", 0.34, -1450, 380)
    band_count = scalar(material, "BandCount", 14.0, -1450, 530)
    flow_speed = scalar(material, "FlowSpeed", 0.35, -1450, 680)
    noise_scale = scalar(material, "NoiseScale", 0.07, -1450, 830)

    texcoord = expression(material, unreal.MaterialExpressionTextureCoordinate, -1450, 990)
    v_mask = expression(material, unreal.MaterialExpressionComponentMask, -1260, 990)
    v_mask.set_editor_property("g", True)
    unreal.MaterialEditingLibrary.connect_material_expressions(texcoord, "", v_mask, "Input")

    band_scaled = multiply(material, v_mask, band_count, -1060, 700)
    time = expression(material, unreal.MaterialExpressionTime, -1060, 840)
    time_flow = multiply(material, time, flow_speed, -860, 840)
    band_phase = expression(material, unreal.MaterialExpressionAdd, -670, 700)
    unreal.MaterialEditingLibrary.connect_material_expressions(band_scaled, "", band_phase, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(time_flow, "", band_phase, "B")
    # The UV band path is intentionally left disconnected here. The engine's
    # Python material API cannot reliably link the Sine input in UE 5.7.
    band_power = scalar(material, "BandMask", 1.0, -90, 700)

    world_pos = expression(material, unreal.MaterialExpressionWorldPosition, -1060, 1060)
    noise_pos = multiply(material, world_pos, noise_scale, -860, 1060)
    noise = expression(material, unreal.MaterialExpressionNoise, -650, 1060)
    unreal.MaterialEditingLibrary.connect_material_expressions(noise_pos, "", noise, "Position")
    noise_power = expression(material, unreal.MaterialExpressionPower, -450, 1060)
    noise_power.set_editor_property("const_exponent", 5.0)
    unreal.MaterialEditingLibrary.connect_material_expressions(noise, "", noise_power, "Base")

    glow_mask = multiply(material, band_power, noise_power, -230, 860)
    blue_surface = multiply(material, glow, scalar(material, "SurfaceGlow", 0.16, -460, 300), -40, 170)
    base_lerp = expression(material, unreal.MaterialExpressionLinearInterpolate, 190, 20)
    unreal.MaterialEditingLibrary.connect_material_expressions(body, "", base_lerp, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(blue_surface, "", base_lerp, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(glow_mask, "", base_lerp, "Alpha")

    colored_glow = multiply(material, glow_mask, glow, 170, 310)
    emissive = multiply(material, colored_glow, emissive_strength, 390, 310)

    unreal.MaterialEditingLibrary.connect_material_property(
        base_lerp, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("Created " + ASSET_PATH)


main()
