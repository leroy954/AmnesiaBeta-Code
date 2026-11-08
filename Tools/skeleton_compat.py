"""Marks the skeletons of the animation packs as compatible with the hero's skeleton (SKEL_ElfArden), both ways, so the free animations of the packs play
on the hero. The packs are not in git, so run this after adding them again from the Fab library, with the editor running.
Usage: python skeleton_compat.py"""
import json

import ue

AS, O = ue.AS, ue.O
ARDEN = "/Game/ElfArden/BaseMesh/SKEL_ElfArden.SKEL_ElfArden"
SKELETONS = [
    "/Game/FreeAnimationLibrary/Demo/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin",
    "/Game/MageBundleSamples/Demo/Manny/SKM_Manny_Skeleton.SKM_Manny_Skeleton",
]

arden = ue.call(AS, "load_asset", asset_path=ARDEN)
arden_list = ue.get_props(arden, "CompatibleSkeletons")["CompatibleSkeletons"]
saved = [ARDEN]

for path in SKELETONS:
    try:
        skeleton = ue.call(AS, "load_asset", asset_path=path)
    except RuntimeError:
        print("not in the project:", path)
        continue

    listed = ue.get_props(skeleton, "CompatibleSkeletons")["CompatibleSkeletons"]

    if not any(x["refPath"] == ARDEN for x in listed):
        ue.call(O, "set_properties", instance=skeleton, values=json.dumps({"CompatibleSkeletons": listed + [{"refPath": ARDEN}]}))

    if not any(x["refPath"] == path for x in arden_list):
        arden_list = arden_list + [{"refPath": path}]

    saved.append(path)
    print("compatible:", path.split("/")[-1])

ue.call(O, "set_properties", instance=arden, values=json.dumps({"CompatibleSkeletons": arden_list}))
print("saved:", ue.call(AS, "save_assets", asset_paths=saved))
