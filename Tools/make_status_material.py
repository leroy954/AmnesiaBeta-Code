"""Builds M_StatusOverlay: the look of a burning, chilled or frozen enemy. An unlit translucent overlay for the skeletal mesh that glows
at the rim (fresnel) and tints the body only a little, so the textures of the enemy stay visible. Parameters: Color (emissive, HDR) and Opacity
(how strong the whole effect is); the rim power and boost are set in the material.
Usage: python make_status_material.py"""
import json

import ue

M = "editor_toolset.toolsets.material.MaterialTools"
PATH = "/Game/Amnesia/Materials/M_StatusOverlay"
SOURCE = "/Game/Amnesia/Materials/M_Weather"
FULL = PATH + "." + PATH.split("/")[-1]

if ue.call(ue.AS, "exists", path=PATH):
    ue.call(ue.AS, "delete", path=PATH)

ue.call(ue.AS, "duplicate", path=SOURCE, new_path=PATH)
mat = {"refPath": FULL}


def cls(name):
    return {"refPath": "/Script/Engine." + name}


def add(name, x, y):
    return ue.call(M, "add_expression", material_or_function=mat, expression_class=cls(name), x=x, y=y)


def connect(src, out, dst, inp):
    ue.call(M, "connect_expressions", from_expression=src, from_output_name=out, to_expression=dst, to_input_name=inp)


# the parameters that already exist: Color and Opacity
exprs = ue.call(M, "get_expressions", material_or_function=mat)
color = [e for e in exprs if "VectorParameter" in e["refPath"]][0]
opacity = [e for e in exprs if "ScalarParameter" in e["refPath"]][0]

fresnel = add("MaterialExpressionFresnel", -700, 200)
rim_power = add("MaterialExpressionScalarParameter", -950, 250)
ue.props(rim_power, ParameterName="RimPower", DefaultValue=2.2)
rim_boost = add("MaterialExpressionScalarParameter", -950, 380)
ue.props(rim_boost, ParameterName="RimBoost", DefaultValue=3.2)
body_glow = add("MaterialExpressionConstant", -700, 420)
ue.props(body_glow, R=0.3)
rim_share = add("MaterialExpressionConstant", -700, 100)
ue.props(rim_share, R=1.5)
base_share = add("MaterialExpressionConstant", -700, 20)
ue.props(base_share, R=0.35)

# the emissive: Color * (Fresnel * RimBoost + BodyGlow)
fresnel_boost = add("MaterialExpressionMultiply", -450, 250)
connect(fresnel, "", fresnel_boost, "A")
connect(rim_boost, "", fresnel_boost, "B")
glow_sum = add("MaterialExpressionAdd", -250, 300)
connect(fresnel_boost, "", glow_sum, "A")
connect(body_glow, "", glow_sum, "B")
emissive = add("MaterialExpressionMultiply", -50, 200)
connect(color, "RGB", emissive, "A")
connect(glow_sum, "", emissive, "B")
ue.call(M, "connect_to_output", expression=emissive, output_name="", material_property="MP_EmissiveColor")

# the opacity: Saturate(Opacity * (BaseShare + Fresnel * RimShare))
fresnel_share = add("MaterialExpressionMultiply", -450, 90)
connect(fresnel, "", fresnel_share, "A")
connect(rim_share, "", fresnel_share, "B")
share_sum = add("MaterialExpressionAdd", -250, 60)
connect(fresnel_share, "", share_sum, "A")
connect(base_share, "", share_sum, "B")
strength = add("MaterialExpressionMultiply", -50, 40)
connect(opacity, "", strength, "A")
connect(share_sum, "", strength, "B")
clamp = add("MaterialExpressionSaturate", 150, 40)
connect(strength, "", clamp, "None")
ue.call(M, "connect_to_output", expression=clamp, output_name="", material_property="MP_Opacity")

# the exponent of the fresnel comes from RimPower
connect(rim_power, "", fresnel, "ExponentIn")

ue.call(M, "recompile", material_or_function=mat)
print(ue.get_props(mat, "BlendMode", "ShadingModel", "bUsedWithSkeletalMesh"))
ue.call(ue.AS, "save_assets", asset_paths=[PATH])
print("saved", PATH)
