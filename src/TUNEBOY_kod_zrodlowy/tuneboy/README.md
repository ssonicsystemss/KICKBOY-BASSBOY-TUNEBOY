# TUNEBOY (seria BOY, sonicsystems) – generator melodii do intro i euforycznych części

Instrument VST3 z wbudowanym supersaw leadem. Melodia gra, **dopóki trzymasz klawisz** (C5 = tak jak na obrazku,
inne nuty = transpozycja). Najprościej: narysuj jedną długą nutę C5 na 4 / 8 / 16 taktów.

## Najważniejsze
- **KEY, SCALE, PROGRESSION, BARS, RHYTHM, DENSITY, OCTAVE, RANGE, SEED** – z tego powstaje melodia + akordy.
- **Obrazek u góry** = mini piano roll (jasne = melodia, ciemne = akordy). Żółta linia = gdzie teraz gra.
  - **Kliknij numer taktu** = zablokuj go (żółty): ten takt zostaje taki sam przy NEW MELODY / VARIATION / zmianie Seed.
- **NEW MELODY** – zupełnie nowa melodia (nowy seed). **VARIATION** – ta sama melodia, kilka nut inaczej.
- **SAVE MIDI** – zapis pliku .mid (z tempem). **DRAG MIDI** – złap i przeciągnij do Playlist / Piano roll w FL.
- Zmiany robione w trakcie grania wchodzą **od następnego taktu** (nie urywają dźwięku).

## Nowe w 2.0
- 5 nowych rytmów (razem 10): Offbeat Stabs, Gallop, Call & Response, Rolling Arp, Half-time; 5 nowych presetów.
- VARIATION, blokowanie taktów, playhead, zmiany od następnego taktu.
- Przełączanie klawiszy w trakcie (puszczasz jedną z dwóch nut) nie robi już ciszy – nuty wracają od razu.
- Detune supersawa liczy prawdziwe centy.

## Nowe w 2.1
- **Edycja melodii w obrazku.** Złap nutę myszką i przeciągnij: w bok = przesunięcie w czasie (siatka 1/16),
  w górę / w dół = inna wysokość (zatrzaskuje się na dźwiękach skali; **Shift** = dowolny półton).
  Prawy brzeg nuty = długość. **Podwójny klik** w pustym miejscu = nowa nuta. **Prawy klik** na nucie = usuń.
  Takt, którego dotkniesz, staje się **twój** (gwiazdka * przy numerze): NEW MELODY / VARIATION / zmiana Seed go nie ruszą.
  **Prawy klik** → „Reset bar” oddaje wygenerowany takt, **RESET EDITS** kasuje wszystkie zmiany.
  Edycje są zapisane w projekcie i w MY PRESETS oraz idą do SAVE MIDI / DRAG MIDI.
- **Więcej progresji:** 16 gotowych + **Custom** (ostatnia na liście). Przy Custom ustawiasz 4 akordy sam:
  **Custom Chord 1–4** na stronie ADVANCED (1–7 = stopnie skali, np. 1-6-3-7 = i-VI-III-VII).
  Dotychczasowe progresje zostały bez zmian.
- **Własny dźwięk (OWN SOUND):** przycisk **LOAD SAMPLE** (albo upuść plik WAV / AIFF / FLAC / MP3 / OGG na wtyczkę).
  Twój sampel gra wszystkie nuty melodii i akordów (C5 = tak jak nagrany). Na ADVANCED: Sample Level, Tune,
  Pitch (Fixed / Notes), Mode (**Layer** = razem z syntezatorem, **Replace** = tylko twój). Sampel zapisuje się w projekcie.
- **Baza dźwięków i losowy dźwięk:** **SOUNDS** = 12 gotowych brzmień syntezatora (Euphoric Supersaw, Raw Screech,
  Soft Pluck, Epic Pad, Bright Hoover, Dark Lead, Glass Bell, Wide Anthem, Rave Stab, Sunrise Strings, Mono Hero, Rave Lead).
  **NEW SOUND** = losowe brzmienie w sensownych zakresach (melodia zostaje; zamrożone Freeze'm pokrętła też). Cofa strzałka ↶.

## Nowe w 2.2
- **DRAW (rysowanie melodii):** włącz przycisk **DRAW**, potem przeciągaj myszką po obrazku – każdy krok 1/16, przez który
  przejedziesz, dostaje nutę (zatrzaskuje się na dźwiękach skali; **Shift** = gumka, **Ctrl** = dowolny półton).
  Kilka kroków na tej samej wysokości łączy się w jedną dłuższą nutę. Potem możesz nuty dalej przeciągać.
- **UNDO EDIT** cofa ostatnią zmianę nut (także rysowanie), **RESET EDITS** też da się cofnąć.
- **Prawy klik na obrazku:** kopiuj takt do następnego / do wszystkich, takt oktawę w górę / w dół, cofnij.
- **Więcej:** 3 nowe skale (Phrygian Dominant, Hungarian Minor, Lydian), 10 nowych progresji (na końcu listy, po Custom),
  4 nowe rytmy (Reverse Gallop, Sparse Offbeat, Pickup Anthem, Hoover Stabs), 12 nowych brzmień w SOUNDS (razem 24),
  8 nowych presetów.
- **CHORD STYLE:** akordy jako Held (długie, jak dotąd) / Offbeat (staby na ósemkach „i”) / Arp (1/16) / Gate (1/8).
- **PUMP:** wbudowany „sidechain” – dźwięk przyciszany na każdą ćwierćnutę i wracający w pół uderzenia. Działa, gdy gra melodia.
- **RANDOMIZE nie zmienia już brzmienia** (syntezator + reverb) – losuje tylko melodię. Brzmienie losuje **NEW SOUND**.
- Okno jest wyższe (960 x 660), bo na ADVANCED jest więcej pokręteł; można je zmniejszyć / powiększyć (róg w prawym dolnym).

## Wspólne dla serii BOY
Presety + **MY PRESETS** (klik w nazwę), **RANDOMIZE** + **↶** cofnij, prawy klik: *Create automation clip* / *Freeze* /
*Reset* – opis w README KICKBOY-a.

## Ograniczenie FL
FL nie zawsze przekazuje MIDI z wtyczki VST3 dalej do innych syntezatorów – najpewniej przenosisz melodię przez
DRAG MIDI albo EXPORT MIDI.

## Uwaga (zmiana z wersji 1.x)
Od 2.0 producent to **sonicsystems** – stare projekty z TUNEBOY 1.x potrzebują starej wersji (kopia w `C:\smms\backup`).
