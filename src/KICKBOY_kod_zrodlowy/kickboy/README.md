# KICKBOY (seria BOY, sonicsystems) – generator kicka hardstyle

Instrument VST3. Wpisz nuty w piano rollu – każda nuta to kick.

## Najważniejsze
- **Graj w tonacji utworu:** nuta G (w dowolnej oktawie) = kick w G (~49 Hz). F = kick w F itd. **Tune** przestraja całość.
  (Key Track = Off: zawsze C + Tune.)
- **Obrazek u góry** to krzywa wysokości kicka. Złap białą kropkę: w górę = wyższy start, w prawo = wolniejszy spadek.
  Zielone tło = głośność w czasie (Length / Release). Kropka błyska przy każdym kicku.
- **DRIVE** (duże pokrętło) + **DISTORTION** (obrazki = kształt przesteru: Soft, Hard, Fold, Crush, Raw).
- **PUNCH**, **RUMBLE** – paski: przeciągnij w lewo / w prawo.
- **OUTPUT** – suwak głośności.

## Nowe w 2.0
- **Note End** (ADVANCED): FULL = kick gra całą długość (Length), CUT = ogon kończy się razem z nutą w piano rollu.
- **LOAD SAMPLE** – twój własny dźwięk (WAV / AIFF / FLAC / MP3 / OGG; możesz też upuścić plik na wtyczkę).
  W ADVANCED: Sample Level, Sample Tune, Pitch (Fixed / Notes), Mode (**Layer** = razem z kickiem, **Replace** = tylko twój).
  Sampel zapisuje się w projekcie.
- Szybkie kicki (rolle) nie trzeszczą przy kolejnym uderzeniu.
- Wspólne dla serii BOY: **MY PRESETS**, **RANDOMIZE** z cofaniem (strzałka obok), **Freeze** i **Create automation clip**
  pod prawym klikiem – patrz niżej.

## Tryb FRENCHCORE (180+ BPM)
Wybierz motyw **Virtual Boy** (klik w nazwę presetu → THEME) – tryb włącza się sam (znaczek FRENCHCORE u góry). Można go też włączyć / wyłączyć
ręcznie w tym samym menu („FRENCHCORE mode”). Brzmienie, które masz, **nie zmienia się** przy przełączeniu.
- **Presety:** w trybie FRENCHCORE lista presetów to osobna paczka pod 180–200 BPM (nazwy zaczynają się od „FC”).
- **RANDOMIZE:** losuje w węższym oknie wartości, które dobrze działają przy 180+ BPM: krótsze kicki (Length 130–260 ms, Release 5–50 ms), mocny przester (Hard / Fold / Crush / Raw, Drive 30–44 dB), wyższy Pitch Start (900–2200 Hz), ogon rumble jako część melodyczna.

## Wspólne dla KICKBOY / BASSBOY / TUNEBOY
- **Nazwa presetu** (u góry): kliknij – presety fabryczne, **MY PRESETS** (twoje), „Save current sound…” (zapisz swój).
  Pliki: `Dokumenty\sonicsystems Presets\KICKBOY\`. Strzałki < > = poprzedni / następny preset.
- **RANDOMIZE** losuje brzmienie w sensownych zakresach, **↶** cofa ostatnie losowanie / wczytanie presetu.
- **Prawy klik na kontrolce:** *Create automation clip* (potem w FL: Tools → Last tweaked → Create automation clip),
  *Freeze* (RANDOMIZE jej nie ruszy, pojawia się żółta śnieżynka), *Reset to default*.
- **ADVANCED** = wszystkie parametry. Podwójny klik = wartość domyślna, Shift = precyzyjnie, kółko myszy = mały krok.
- Wtyczka zużywa bardzo mało pamięci: cały wygląd jest rysowany na jednej powierzchni, bez obrazków.

## Uwaga (zmiana z wersji 1.x)
Od 2.0 producent to **sonicsystems** (wcześniej „Sergio Parowka”). FL traktuje to jak nowe wtyczki: stare projekty
z KICKBOY 1.x trzeba otworzyć ze starą wersją (kopia zapasowa jest w `C:\smms\backup`). Nuta C5 daje teraz kick w C
(wcześniej G) – po prostu graj nutę w tonacji utworu.
