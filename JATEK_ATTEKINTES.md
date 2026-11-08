# AmnesiaBeta: a játék felépítése

Ez az összefoglaló egy másik AI-nak (vagy fejlesztőnek) mutatja be a projektet: mi a játék, hogyan épül fel a kód és a tartalom, mi kész és mi van folyamatban. Állapot: 2026-11-08.

## 1. A játék röviden

- **Műfaj:** 2.5D akció-platformer, az Ori-játékok stílusában. 3D modellek, a mozgás egy síkra (X–Z) van korlátozva, a kamera oldalról néz.
- **Téma:** amnézia. A hős névtelen, a történet még nincs megírva, és nem köthető a mostani hősmodellhez.
- **Fejlesztő:** egy ember, Unreal Engine 5.8, C++ (a Third Person sablonból indult, a Combat / SideScrolling / Platforming változat-mappák maradtak meg mintának).
- **Nyelv:** a játék szövegei magyarok (menü, bolt, képességek, feliratok).

## 2. A pálya

Egyetlen hosszú pálya: `/Game/Amnesia/Maps/Lvl_Forest01`, X kb. -2100-tól 64500-ig. Az út az Y=0 vonalon fut. Hat zónából áll, mindegyiknek saját hangulata van (`AAmnesiaMoodVolume`):

| # | Zóna | Jellemző |
|---|---|---|
| 1 | Erdő | nyári fények, folyó a háttérben, Entek és orkok |
| 2 | Falu | őszi arany fények, házak, ugróplatformok ládákkal |
| 3 | Havas erdő | tél, hóesés |
| 4 | Romok (X 32000–42000) | romvárosi templom: kapu, oszlopsor, oltár, köd és eső |
| 5 | Barlang (X 42000–54000) | sötét, cseppkövek, világító gombák |
| 6 | Viharerdő | vihar, villámok, kőkör, a háttérben egy óriás (kb. 69 m magas kőgólem) |

- **Ellenőrzőpontok:** 11 tűztál (`AAmnesiaCheckpoint`). Gyógyítanak, mentenek, és innen éled újra a hős.
- **Gyógygömbök:** `AAmnesiaPickup`, az ellenségek is dobhatják.
- **Légkör:** `AAmnesiaAtmosphere` (nap, égbolt, köd, színkorrekció, hangulatok keverése), `AAmnesiaWeather` (eső, hó, villám), `AAmnesiaGlowLight` (pislákoló fények).
- A díszletnek (kb. 6000 actor, „Decoration” címkével) futás közben nincs ütközése. Ezt az `UAmnesiaDecorationSubsystem` kapcsolja ki.

## 3. A hős

- **Osztály:** `AAmnesiaCharacter` (`ACombatCharacter` leszármazott), Blueprint: `BP_AmnesiaCharacter`.
- **Modell:** Elf Arden (Fab). **Csak ideiglenes**, később jön egy saját főhős. A váz Manny-kompatibilis.
- **Animáció:** `UAmnesiaHeroAnimInstance` (az `ABP_Arden` szülője). A csapások a felsőtestre kerülnek, a lábak közben futhatnak.
- **Mozgás:** futás, Shift sprint, dupla ugrás, coyote time, levegőben vágás (a találat után a hős visszapattan).
- **Harc:**
  - bal gomb röviden: kombó, a vágás felengedéskor indul;
  - bal gomb 0,3 s-nál tovább nyomva: erős támadás, vagy a fegyver különleges képessége;
  - jobb gomb nyomva tartva: védekezés (felsőtestes póz, fele sebzés, lassabb járás, a hős a legközelebbi ellenfél felé fordul).
- **Szintek:** 1–10, XP az ölésekből. Szintenként több HP, +15% sebzés és teljes gyógyulás, a felirat „SZINTLÉPÉS!”.
- **Visszajelzések:** hit-stop, kameraremegés, villanó ellenség, sebzés- és gyógyulásszámok.

### Irányítás (billentyűzet)

A/D mozgás • Space ugrás • Shift sprint • bal gomb támadás • jobb gomb védekezés • Q lángcsapás • E fagynova • R elem váltása • X fegyverváltás • T képességek • B bolt • F cica simogatása • K admin panel • Esc szünet menü

## 4. Fegyverek

Táblázat a kódban: `FAmnesiaWeapons::All()` (`AmnesiaWeapons.h/.cpp`). Minden fegyverhez tartozik modell, kéztartás, aura (Niagara), szorzók, kombó és erős támadás. A fegyver megvétele új képességágat nyit.

| Fegyver | Ár | Működés | Különleges (4. szintű képesség) |
|---|---|---|---|
| Kard | alap | a Blueprint saját vágásai | – |
| Holdkorong (csakram) | 250 | eldobható korong (`AAmnesiaChakram`): átrepül az ellenfeleken, visszatér a kézbe, és amíg nem tért vissza, nem dobható újra | Örvény (Korongzápor: max. 4 korong) |
| Kőtörő bárd | 350 | lassú, széles, nagy sebzés, vérzés | Földrengés (lökéshullám, kábítás) |
| Ködjáró pálca | 450 | minden kattintásra tűzlövedék (`AAmnesiaFireball`), nyomva tartva erősebb | Varázsrobbanás |

## 5. Képességek

- **Fájlok:** `AmnesiaSkills.h/.cpp` (definíciók és `UAmnesiaSkillComponent`), Slate képernyő: `AmnesiaSkillScreen`.
- **Pontok:** szintlépésenként 2, plusz a cicákért.
- **Ágak:** Acél, Tűz, Jég, Sárkány, valamint fegyverenként egy-egy ág (Holdkorong, Balta, Pálca). Egy ágban 4 szint van, a 3. szintű képességek 2 pontba kerülnek.
- **Aktív képességek:** Q lángcsapás, E fagynova (valódi fagyasztás jégkristályokkal: `AAmnesiaFrost`). Az R a kard elemét váltja (semmi / tűz / jég), ettől változik az aura és a sárkány színe is.
- **Állapotok az ellenségen:** égés, lassítás, fagyás, vérzés, kábítás.

## 6. A sárkány társ

- **Osztály:** `AAmnesiaCompanion`, a Little Dragons csomag tigrissárkánya.
- **Megszerzés:** csak a boltban vehető meg, addig a Sárkány ág zárolva van.
- **Viselkedés:** követi a hőst, és tűzgolyót lő a közeli ellenségre. Reagál is: szintlépéskor bukfencet vet, kitér, ha a hőst találat éri, és a kamera felé néz, ha a hős tétlen.

## 7. Ellenségek

- **Osztály:** `AAmnesiaEnemy` (`ACombatEnemy` leszármazott). Saját tick-alapú agya van: őrjárat, üldözés, támadás. Az animációkat a gráf nélküli `UAmnesiaCreatureAnimInstance` játssza le (idle/walk/run, és egyszeri akciók).
- **Támadások:** `Attacks` tömb (animáció, találati idő, csont, sebesség). Akinek nincs támadóanimációja, az procedurális lökéssel támad.
- **Típusok:**
  - Entek: tölgy, kőris, nyír, lucfenyő; ez a fő ellenség;
  - orkok: 3 kinézet, 6 darab az 1–3. zónában;
  - csontvázőrök;
  - páncélos gólem (kalapács és pajzs), kőgólem;
  - a 6. zóna óriása, amely inkább díszlet-boss.
- **Nehézség:** szint zónánként emelkedik (EnemyLevel 1–10), HP- és sebzésszorzók.
- **Teljesítmény:** az 5500 cm-nél távolabbi ellenségek „alszanak” (nem tickelnek).

## 8. Gazdaság, bolt, gyűjtögetés

- **Arany:** a döntés szerint valódi pénzért vehető, kizárólag webshopból. Játékban nem adható jutalomként (ölésért, cicáért, ládából sem). A webshop és a fizetés még nincs kész. A bolt (`SAmnesiaShopScreen`, B gomb) aranyért adja a sárkányt és a fegyvereket, minden tételhez részletes oldallal.
- **Cicák:** zónánként egy alvó cica (`AAmnesiaCat`), F-fel simogatható. XP-t ad, és mind a hat megtalálásáért +1 képességpont jár.
- **Ötlet, még nincs eldöntve:** ládák a pályán, amelyekből vas jön. A vasért a kovácsnál (a faluban) rúnákat lehet a fegyverekbe illeszteni. A vas lenne a játékban szerezhető pénz. Ehhez a pályát bővíteni kell.

## 9. Felület és mentés

- **HUD:** `AAmnesiaHUD`, a legnagyobb fájl. Életcsík, XP-csík, szint, képességsáv, ellenségcsíkok, felugró szövegek, halálképernyő; innen nyílnak a képernyők.
- **Menük:**
  - `SAmnesiaMenu`: főmenü és szünet menü; a beállításokban hangerő, grafikai minőség, kameraremegés és sebzésszámok;
  - `SAmnesiaAdminPanel` (K): sebezhetetlenség, repülés, teleport a zónákba, +1000 arany, végtelen képességpont, szintállítás.
- **Kinézet:** minden UI Slate-ben készült, Cinzel és Roboto betűkkel.
- **Mentés:** `UAmnesiaSaveGame` és `UAmnesiaProgressSubsystem` (AmnesiaProgress.h/.cpp), egy mentési hely. Elmenti a szintet, az XP-t, az ellenőrzőpontot, a beállításokat, a képességeket, az elemet, az aranyat, a megvett tételeket, a felszerelt fegyvert és a cicákat.

## 10. Kódszerkezet

Minden játékkód a `Source/AmnesiaBeta/Amnesia/` mappában van (kb. 21 000 sor):

| Fájl | Feladat |
|---|---|
| AmnesiaCharacter | a hős: mozgás, harc, védekezés, fegyverek, szintek, debug kapcsolók |
| AmnesiaHeroAnimInstance | a hős animációs rétegei (felsőtest-overlay) |
| AmnesiaEnemy | ellenség-agy, támadások, állapotok, nehézség |
| AmnesiaCreatureAnimInstance | gráf nélküli animáció az ellenségekhez és a sárkányhoz |
| AmnesiaWeapons | fegyvertáblázat |
| AmnesiaChakram, AmnesiaFireball, AmnesiaFrost | lövedékek és effektek |
| AmnesiaSkills, AmnesiaSkillScreen | képességrendszer és képernyője |
| AmnesiaCompanion | a sárkány |
| AmnesiaHUD, AmnesiaMenu, AmnesiaShopScreen, AmnesiaAdminPanel | felület |
| AmnesiaProgress | mentés, arany, megvett tételek |
| AmnesiaCheckpoint, AmnesiaPickup, AmnesiaCat, AmnesiaGiant | pályaelemek |
| AmnesiaAtmosphere, AmnesiaWeather, AmnesiaGlowLight | légkör |
| AmnesiaSound.h | `FAmnesiaSoundSet`: véletlen változatok, hangmagasság, távolsággal halkuló hang |
| `Source/AmnesiaBeta/AmnesiaLevelJobs.cpp` | csak szerkesztőben fut: a `Saved/LevelJobs/job.json` fájlból tömegesen helyez el és módosít actorokat, és szerkesztőbeli Pythont is futtat (`exec`) |

## 11. Tartalom

- **Saját tartalom (gitben, LFS-sel):** `Content/Amnesia` (blueprintek, pálya, anyagok, UI, animációk), `Audio`, `UI`, `Input`, `Slate`.
- **Külső csomagok:** nincsenek a gitben (licenc és méret miatt), a listájuk a `PACKS.md`-ben van. Ilyenek például: ElfArden, PackOfTreeEnts, Low_Poly_Orc_Warrior, Skeleton_Guard, StoneGolem, LittleDragons, Stylized_Spruce_Forest, AncientTempleRuins, GreenwoodFantasyVillage, MysteryCave, MeleeWeaponAura_VFX, Vefects, MageBundleSamples, FreeAnimationLibrary, ParagonGrux/Sevarog, Chest, Blacksmith_Setup.
- **Mixamo:** a nyers FBX fájlok az `Import/Mixamo` mappában vannak, nem kerülnek gitbe.

## 12. Fejlesztési munkamenet

- **Szerkesztő vezérlése:** az unreal-mcp szerveren keresztül (`.mcp.json`, 8000-es port) vagy a `Tools/ue.py` Python klienssel. A `Tools/` mappában sok teszt- és építőszkript van (pálya-generálás, PIE-tesztek képernyőképekkel, hangok, ikonok, retarget).
- **Fordítás:** `Tools/rebuild.ps1`: ment, bezárja a szerkesztőt, fordít, és újraindítja. Futó szerkesztő mellett a Live Coding blokkolja a fordítást.
- **Git:** privát GitHub repó (`origin`). Magyar commit-üzenetek, force-push nincs.

## 13. Folyamatban és nyitott kérdések

- **Élethű harci animációk (folyamatban, 2026-11-08):**
  - Mixamo mocap csomagok: kard-pajzs, balta, kétkezes kard, varázslat, mutáns-szörny, frizbidobás;
  - IK Retargeterrel kerülnek át a hős vázára (`Tools/retarget_anims.py`); a vágások helyben maradnak, az előrelépést a játék adja;
  - következő lépések: fegyverenként kiválasztani az oldalnézetben jól mutató mozdulatokat, bekötni a kombókba, aztán az ellenségekre (orkok) is átvinni.
- **Új főhős:** a fegyverek kéztartását, a csapások időzítését és az aurákat újra kell hangolni (a retarget ezt megkönnyíti).
- **További tervek:** részletesebb menüdizájn, ládák, vas, kovács és rúnák, webshop az aranyhoz, a történet megírása.
