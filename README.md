# AmnesiaBeta (kód)

2.5D akció-platformer Unreal Engine 5.8-ban, C++-ban, az Ori-játékok stílusában. A hős névtelen, a téma az amnézia. Egy ember fejleszti.

*A 2.5D action platformer in Unreal Engine 5.8 (C++). This public repository holds the game code and the tools only, without any art, sound or animation assets.*

## Mi van ebben a repóban

- `Source/AmnesiaBeta/Amnesia/`: a játék kódja. Itt van a hős, a harc, a fegyverek, a képességek, az ellenségek, a sárkány társ, a felület (Slate), a mentés és a légkör.
- `Source/AmnesiaBeta/AmnesiaLevelJobs.cpp`: csak szerkesztőben fut. Tömegesen helyez el és módosít actorokat, és szerkesztőbeli Python-szkripteket futtat.
- `Tools/`: Python-szkriptek, amelyek az unreal-mcp szerveren át vezérlik a szerkesztőt: pályaépítés, tesztek képernyőképekkel, animáció-retarget, ikonok, hangok.
- `Config/`, `AmnesiaBeta.uproject`: a projekt beállításai.
- [JATEK_ATTEKINTES.md](JATEK_ATTEKINTES.md): részletes leírás a játék felépítéséről.

## Mi nincs benne

A `Content` mappa (pálya, blueprintek, modellek, textúrák, hangok, animációk) nincs ebben a repóban. Ezek nagy része Fab-, Mixamo- és más csomagokból származik, és a licencük nem engedi a továbbadást. A felhasznált csomagok listája a [PACKS.md](PACKS.md) fájlban van. Emiatt a projekt ebből a repóból önmagában nem nyitható meg játszható formában: ez a kód és az eszközök bemutatására szolgál.

A fejlesztés egy privát repóban folyik, ez annak a kódrésze.
