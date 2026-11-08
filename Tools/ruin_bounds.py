"""Prints size and pivot of the ruin pieces used by zone4.py."""
import ue

RU = "/Game/AncientTempleRuins/Environment/"
NAMES = [
    "Construction/Pillar/SM_Pillar_01_128_128_512_01", "Construction/Pillar/SM_Pillar_02_256_512_01",
    "Construction/Pillar/SM_Pillar_01_64_64_256_02", "Construction/Pillar/SM_Pillar_01_64_64_256_D", "Construction/Pillar/SM_Pillar_64_64_256_01_C",
    "Construction/Pillar/SM_PillarBase_01_128_192_01", "Construction/Pillar/SM_PillarBase_01_128_96_01_A", "Construction/Pillar/SM_Pillar_Cube_01_A",
    "Construction/Wall/SM_Wall_01_Linear_256_256_01A", "Construction/Wall/SM_Wall_01_Linear_256_256_02", "Construction/Wall/SM_Wall_01_Linear_256_128_02",
    "Construction/Wall/SM_Wall_01_Linear_512_512_01_A", "Construction/Wall/SM_Wall_01_Open_192_256_01_A", "Construction/Wall/SM_Wall_01_Linear_192_256_03",
    "Construction/Door/SM_Door_01_A", "Construction/Door/SM_Door_01_B",
    "Construction/Floor/SM_Floor_02_Linear_512_512_01_A", "Construction/Floor/SM_Floor_01_Linear_768_512_01_A", "Construction/Floor/SM_Floor_Engraved_800_800_01_A",
    "Construction/Floor/SM_FloorTrim_01_Linear_256_64_01_A",
    "Construction/Stair/SM_Stairs_02_Linear_800_500_01", "Construction/Stair/SM_Floor_01_Stairs_Linear_256_256_01",
    "Construction/Ceil/SM_TopTrim_01_Linear_128_256_A", "Construction/Ceil/SM_Roof_02_Linear_256_512_01_A", "Construction/Ornament/SM_Ornament_01_A",
    "Construction/Buildings/SM_Facade_Custom_01",
    "Prop/SM_Torch01_Holder", "Prop/SM_Torch_01", "Prop/Pots/SM_Pot_01_A", "Prop/Pots/SM_Pot_01_Broken_A_01", "Prop/Pots/SM_Pot_03_Broken_A_01",
    "Prop/StoneHead/SM_StoneHead_01", "Prop/StoneHead/SM_StoneHead_04", "Prop/StoneHead/SM_StoneHead_07",
    "Rock/Stone01/SM_Stones01_a", "Rock/Stone02/SM_Stones_02_a", "Rock/StoneTiles02/SM_StoneTiles_01_A", "Rock/StairStep02/SM_StairStep_02a",
    "Rock/Rock01/SM_Rock01", "GroundScatter/SM_GS_DirtStony_01c",
    "Foliage/Ivy_01/SM_Ivy_01_A", "Foliage/Ivy_01/SM_Ivy_01_D", "Foliage/Moss_01/SM_Moss_Cluster_01_A", "Foliage/CeibaRoots_01/SM_Ceiba_Roots_03B",
    "Foliage/Fern_01/SM_Fern_01_A", "Foliage/Monstera_01/SM_Monstera_01_Cluster_01", "Foliage/Palm_Giant_01/SM_PalmGiant_01_A",
    "Foliage/Ceiba_Tree_01/SM_Ceiba_Tree_01_A", "Foliage/CelyonCreeper_01/SM_CeylonCreeper_01_A", "Foliage/Heliconia_01/SM_Heliconia_01",
]
paths = [RU + n + "." + n.split("/")[-1] for n in NAMES]
r = ue.job([{"op": "mesh_bounds", "meshes": paths}])
print(r["errors"])
for line in r["report"]:
    p, *v = line.split(" ")
    mn, mx = list(map(float, v[:3])), list(map(float, v[3:]))
    size = [mx[i] - mn[i] for i in range(3)]
    print("%-46s size %5.0f %5.0f %5.0f   min %6.0f %6.0f %6.0f" % (p.split(".")[-1], *size, *mn))
