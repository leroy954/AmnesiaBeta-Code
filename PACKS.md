# Külső csomagok (nincsenek a repóban)

Ezek a Fab/Marketplace csomagok a `Content/` mappában vannak, de a git nem követi őket, mert a licencük nem engedi a továbbadást, és együtt kb. 10 GB-ot foglalnak.
Klónozás után a szerkesztőben a Fab könyvtárból újra hozzá kell adni őket a projekthez, hogy az alábbi mappák meglegyenek:

| Mappa | Mire használjuk |
|---|---|
| AncientTempleRuins | a romok zónája, kőkör, oszlopok, falak |
| Stylized_Spruce_Forest | fák, fű, sziklák, hegyek a háttérben |
| GreenwoodFantasyVillage | a falu házai és kellékei |
| PackOfTreeEnts | az Ent ellenségek |
| MysteryCave | a barlang díszítése |
| StoneGolem, Stone_Golem | a páncélos gólem és az óriás |
| Wood_Monster | fa szörny |
| LittleDragons | a sárkány társ |
| Skeleton_Guard | a csontváz ellenségek |
| MeleeWeaponAura_VFX | a kard aurája |
| ElfArden | a hős |
| SuperStorm | viharfelhők (a motor Marketplace mappájában lévő plugin be van kapcsolva a .uproject-ben) |
| Dynamic_Skill_Tree_System | csak mintának: egy Blueprint képességfa, ennek a kinézete volt a minta a képességek képernyőjéhez (a játék saját C++ képességrendszert használ) |
| KiroshoSkIconsPk0 | a 16 képesség ikonja (néhányat a `Tools/skill_icons.py` átszínez a `Content/Amnesia/UI/Skills` mappába) |
| RPGClassIcons | 500 tolvaj képesség ikon, még nincs használva |
| Vefects | Stylized Fire VFX: a képességek lángjai (égés, lánglöket, fagyhullám) |
| Knife_light | vágás effektek (Niagara), még nincs használva |
| FantasyPackFREE | négy zeneszám (harc, kazamata, felfedezés, város), még nincs használva |
| ParagonGrux | boss karakter animációkkal (kb. 3 GB), még nincs használva |
| Fab/Bear_Trap_-_Dungeon_Interactables | medvecsapda részekben (talp, két állkapocs, lánc), még nincs használva |
| Animal-Cat-FreeBundle | a gyűjthető cicák (modell, 11 bunda, pózok): az `AAmnesiaCat` a játék indulásakor tölti be őket, a csomag nélkül nincsenek cicák |
| PurePoly/FreeLowPolyFantasyRPGWeapons | 26 low poly fegyver modell: a boltban kapható Holdkorong (csakram), bárd és pálca modellje (`AmnesiaWeapons.cpp`), a többi még nincs használva |
| FreeAnimationLibrary | a védekező póz (`anim_Counter_Victim_01`); a vázát a `Tools/skeleton_compat.py` jelöli kompatibilisnek a hősével. A lándzsás animációk már nincsenek használva (a lándzsa helyére a Holdkorong került) |
| MageBundleSamples | a pálca varázsló animációi és a Holdkorong dobása (`AS_Mage_Ability_Disk_Throw`), MANNY változatok; ezt is a `Tools/skeleton_compat.py` jelöli kompatibilisnek |
| MC_Sample | mocap minta animációk (varázsló, varázskönyv, kalapácsos ork ...), még nincs használva |
| Low_Poly_Orc_Warrior | az ork ellenségek (`BP_Orc_Warrior`, `_2`, `_3`): 3 modell, bárd, pajzs, 20 animáció |
| Whisper | magas horror lény, ellenség jelölt (16 animáció: 3 támadás, séta, futás, sebződés, halál), még nincs használva |
| DemonLord2 | páncélos, kalapácsos szörny, ellenség vagy boss jelölt (14 animáció, UE4-es váz), még nincs használva |
| ParagonSevarog | Paragon boss karakter (kb. 1,4 GB, Paragon váz), még nincs használva |
| CavesAndDungeons | 14 hurkolható sötét ambiens és zene (mindegyikhez egy Cue és Loop A/B cue, kb. 830 MB), a barlang és a romok zónájához, még nincs használva |
| MonsterRoarsAndGrowls | 25 üvöltés és morgás (wav és cue): az orkok hangjai (`Tools/orc_sounds.py`) |
| Chest | fa láda nyitó és záró animációval, még nincs használva |
| StylizedGravestones | 5 stilizált sírkő, még nincs használva |
| HebeFountain | animált szökőkút (víz, fröccsenés, kifolyó, szobor), még nincs használva |
| FreeParticle_SoftTofu | szikra, levél és toll Niagara részecskék (CC BY 4.0, a szerzőt fel kell tüntetni), még nincs használva |
| Mixamo | Adobe Mixamo mocap animációk (X Bot váz; Sword and Shield, Pro Melee Axe, Great Sword, Pro Magic csomag, mutáns támadások, frizbidobás). Nem a Fabról jön: a mixamo.com-ról kell letölteni (FBX, Without Skin), az `Import/Mixamo` mappába kicsomagolni, és a `Tools/mixamo_import.py` importálja. A hősre, az orkokra és a csontvázőrökre retargetelt, helyben maradó másolatok a `Content/Amnesia/Animation/Combat` mappában vannak (`Tools/retarget_anims.py`, `Tools/anim_in_place.py`) |

Saját tartalom (Git LFS-sel a gitben): `Content/Amnesia`, `Audio`, `UI`, `Input`, `Slate`, a `Variant_*` sablon mappák és a pálya külső actor fájljai.

A Cinzel betűtípus (SIL Open Font License 1.1, Natanael Gama munkája) a repó része: a betűkészletek a `Content/UI/Fonts` mappában vannak, mellettük a licenc szövege.
