# KICKBOY / BASSBOY / TUNEBOY 2.0 – kod źródłowy (sonicsystems)

To jest dokładnie ten kod, z którego zbudowane są działające wtyczki
(wersja 2.0 + poprawka klawiatury z 18.10.2026).

## Co jest w środku
- `src/KICKBOY_kod_zrodlowy/kickboy/` – KICKBOY (`Source/KickBoy.cpp` = cała wtyczka, `Tools/RenderDemo.cpp` = test)
- `src/BASSBOY_kod_zrodlowy/bassboy/` – BASSBOY
- `src/TUNEBOY_kod_zrodlowy/tuneboy/` – TUNEBOY
- `src/BOY_shared/` – wspólny kod całej serii: wygląd (`BoyUI.h`), presety / RANDOMIZE / Freeze (`BoyCommon.h`),
  test (`BoyTest.h`), czcionka, `boy.cmake`
- `src/SONICSYSTEMS_shared/OwnSound.h` – „własny sampel” (LOAD SAMPLE w KICKBOY)
- każdy folder ma `README.md` (instrukcja po polsku) i `CLAUDE.md` (notatki dla Claude'a)

**Nie zmieniaj układu folderów** – projekty szukają wspólnego kodu przez `../../BOY_shared`.

## Budowanie (jak będziesz chciał sam)
Potrzebne: Visual Studio z C++ i CMake. JUCE 8.0.4 pobiera się samo przy pierwszym budowaniu (internet).
W folderze np. `src/KICKBOY_kod_zrodlowy/kickboy`:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel 2

Gotowa wtyczka: `build/BoyKickboy_artefacts/Release/VST3/KICKBOY.vst3` -> skopiuj do
`C:\Program Files\Common Files\VST3`. Test: `KickboyRender demo.wav gui.png` musi wypisać `RESULT: PASS`.
(Na Twoim laptopie najprościej poprosić Claude'a – ma do tego gotowy skrypt w repo SINGULARITY.)

Oryginał tego kodu jest w repo `Desktop\danger\SINGULARITY` (GitHub `ssonicsystemss/SINGULARITY`).
