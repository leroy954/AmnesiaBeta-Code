"""Rebuilds the grass along the walking path at a lower height (knee height next to the hero instead of towering over it).
The seed and lanes are the ones of levelgen.step_grass, only the size changed."""
import random
import ue
import levelgen as lg

rnd = random.Random(55)
ops = [{"op": "delete", "folder": "Gen/PathGrass"}]
lg.path_grass(ops, -2000, 64400, "Gen/PathGrass", rnd, spacing=90, flowers=0.08, scale=(1.5, 2.2),
              skip=lambda x: lg.MOUND[0] <= x <= lg.MOUND[1] or lg.in_cave(x, 0))
print("clumps:", len(ops) - 1)
print(ue.job(ops + [{"op": "save"}], timeout=900))
