"""Sizes and a contact sheet of the cave pack pieces that could build walls, ceiling and floor."""
import ue

CV = "/Game/MysteryCave/Meshes/"
names = (["SM_Scene", "SM_Water", "SM_Plane"] + ["SM_stal_StalactiteCeil_%d" % i for i in (1, 2, 3)] + ["SM_stal_StalagmiteWall_%d" % i for i in (1, 2, 3, 4)] +
         ["SM_stal_StalagmiteFloor_%d" % i for i in (1, 2, 3, 4)] + ["SM_stal_StalactitesM_%d" % i for i in (1, 2, 3)] +
         ["SM_stal_StalactitesW_%d" % i for i in (1, 2, 3)] + ["SM_stal_StalactitesWG_1"] + ["SM_stal_StalactiteStrip_%d" % i for i in (4, 5, 6, 7)] +
         ["SM_stal_Pillar_%d" % i for i in (1, 2, 3)])
paths = [CV + n + "." + n for n in names]
r = ue.job([{"op": "mesh_bounds", "meshes": paths}])
for line in r["report"]:
    print(line)
ue.sheet(paths[:30], "shots/cave_assets.png")
print("saved")
