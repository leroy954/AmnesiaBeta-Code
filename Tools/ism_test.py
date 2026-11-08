"""Smoke test of the ism job op: three grass clumps far from the level, then they are removed again."""
import ue

G = "/Game/Stylized_Spruce_Forest/Meshes/Plants/SM_Grass_02.SM_Grass_02"
inst = [[300000 + i * 100, 0, 0, 0, i * 30, 0, 2, 2, 2] for i in range(3)]
r = ue.job([{"op": "delete", "folder": "Temp/IsmTest"},
            {"op": "ism", "mesh": G, "label": "IsmTest", "folder": "Temp/IsmTest", "tags": ["Decoration"], "shadow": False, "instances": inst}])
print(r)
acts = ue.call(ue.S, "find_actors", name="IsmTest", collision_channels=[], tag="")
print([a["refPath"][-40:] for a in acts])
if acts:
    print(ue.call(ue.A, "get_actor_bounds", actor=acts[0]))
print(ue.job([{"op": "delete", "folder": "Temp/IsmTest"}]))
