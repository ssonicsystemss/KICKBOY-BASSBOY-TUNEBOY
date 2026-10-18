# TUNEBOY (seria BOY, sonicsystems) – generator melodii do intro i euforycznych części

Instrument VST3 z wbudowanym supersaw leadem. Melodia gra, **dopóki trzymasz klawisz** (C5 = tak jak na obrazku,
inne nuty = transpozycja). Najprościej: narysuj jedną długą nutę C5 na 4 / 8 / 16 taktów.

## Najważniejsze
- **KEY, SCALE, PROGRESSION, BARS, RHYTHM, DENSITY, OCTAVE, RANGE, SEED** – z tego powstaje melodia + akordy.
- **Obrazek u góry** = mini piano roll (jasne = melodia, ciemne = akordy). Żółta linia = gdzie teraz gra.
  - **Kliknij numer taktu** = zablokuj go (żółty): ten takt zostaje taki sam przy NEW MELODY / VARIATION / zmianie Seed.
- **NEW MELODY** – zupełnie nowa melodia (nowy seed). **VARIATION** – ta sama melodia, kilka nut inaczej.
- **EXPORT MIDI** – zapis pliku .mid (z tempem). **DRAG MIDI** – złap i przeciągnij do Playlist / Piano roll w FL.
- Zmiany robione w trakcie grania wchodzą **od następnego taktu** (nie urywają dźwięku).

## Nowe w 2.0
- 5 nowych rytmów (razem 10): Offbeat Stabs, Gallop, Call & Response, Rolling Arp, Half-time; 5 nowych presetów.
- VARIATION, blokowanie taktów, playhead, zmiany od następnego taktu.
- Przełączanie klawiszy w trakcie (puszczasz jedną z dwóch nut) nie robi już ciszy – nuty wracają od razu.
- Detune supersawa liczy prawdziwe centy.

## Wspólne dla serii BOY
Presety + **MY PRESETS** (klik w nazwę), **RANDOMIZE** + **↶** cofnij, prawy klik: *Create automation clip* / *Freeze* /
*Reset* – opis w README KICKBOY-a.

## Ograniczenie FL
FL nie zawsze przekazuje MIDI z wtyczki VST3 dalej do innych syntezatorów – najpewniej przenosisz melodię przez
DRAG MIDI albo EXPORT MIDI.

## Uwaga (zmiana z wersji 1.x)
Od 2.0 producent to **sonicsystems** – stare projekty z TUNEBOY 1.x potrzebują starej wersji (kopia w `C:\smms\backup`).
