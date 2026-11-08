"""Builds the probe material of the cat sweep: an opaque unlit flat magenta, saved as /Game/Gen/CatProbe/MI_Cat_01, so that a test cat with
CoatFolder=/Game/Gen/CatProbe/ and Coat=1 is a pure magenta shape that cat_sweep.py can count pixel by pixel.
Usage: python make_cat_probe.py"""
import ue

M = "editor_toolset.toolsets.material.MaterialTools"
SOURCE = "/Game/Amnesia/Materials/M_Weather"
PATH = "/Game/Gen/CatProbe/MI_Cat_01"
mat = {"refPath": PATH + ".MI_Cat_01"}

if ue.call(ue.AS, "exists", path=PATH):
    ue.call(ue.AS, "delete", path=PATH)

ue.call(ue.AS, "duplicate", path=SOURCE, new_path=PATH)
ue.props(mat, BlendMode="BLEND_Opaque")
exprs = ue.call(M, "get_expressions", material_or_function=mat)
color = [e for e in exprs if "VectorParameter" in e["refPath"]][0]
print(ue.get_props(color, "ParameterName", "DefaultValue"))
ue.props(color, DefaultValue={"R": 1.0, "G": 0.0, "B": 1.0, "A": 1.0})
ue.call(M, "recompile", material_or_function=mat)
print(ue.get_props(mat, "BlendMode", "ShadingModel", "bUsedWithSkeletalMesh"))
print(ue.get_props(color, "ParameterName", "DefaultValue"))
ue.call(ue.AS, "save", path=PATH)
