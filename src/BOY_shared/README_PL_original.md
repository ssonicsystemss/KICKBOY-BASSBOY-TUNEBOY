# KICKBOY · BASSBOY · TUNEBOY — pluginy do hardstyle (VST3, Windows 64-bit)

Trzy lekkie instrumenty VST3 do FL Studio:

| Plugin | Co robi |
|---|---|
| **KICKBOY** | Generator kicka hardstyle: punch/click, body z opadającym pitchem, przester (Soft/Hard/Fold/Crush/Raw), rumble pod nutę MIDI, EQ, długość, limiter. |
| **BASSBOY** | Reverse bass zsynchronizowany z tempem projektu (offbeat), saw + sine, przester, filtr z envelope, glide, 6 patternów i „Kick Gap” (miejsce na kicka jak sidechain). |
| **TUNEBOY** | Generator melodii do intro / euphoric: tonacja, skala, progresja akordów, liczba taktów, rytm, gęstość, seed. Gra wbudowanym supersaw leadem z pogłosem, wysyła MIDI i eksportuje plik .mid. |

Każdy ma: widok **MAIN** (12 najważniejszych gałek), przycisk **ADVANCED** (wszystkie parametry), **RANDOMIZE** i **20 presetów** (lista u góry + strzałki < >; FL widzi je też jako „programs”).

## Instalacja
1. Rozpakuj ZIP.
2. Skopiuj trzy foldery `KICKBOY.vst3`, `BASSBOY.vst3`, `TUNEBOY.vst3` do:
   `C:\Program Files\Common Files\VST3`
   (cały folder `.vst3`, nie tylko plik w środku; Windows zapyta o uprawnienia admina — kliknij „Kontynuuj”).
3. FL Studio: **Options → Manage plugins → Find installed plugins** (albo „Find more plugins”/Rescan). Znajdź je na liście, zaznacz gwiazdkę (Favorite).
4. Dodaj przez **Channel rack → + → KICKBOY** (itp.).

Jeśli Windows Defender/SmartScreen marudzi: pluginy nie mają podpisu cyfrowego — to normalne dla własnych buildów.

## Jak używać
- **KICKBOY**: wpisz nuty w piano roll. Domyślnie **C5 = kick w G (~49 Hz)**; wyższa nuta = wyższy kick. TUNE przestraja całość.
- **BASSBOY**: trzymaj długą nutę (np. cały takt) i włącz play — bas sam gra offbeaty w tempie projektu. KICK GAP = ile miejsca zostaje na kicka, SWELL = kształt narastania. Bez playa też gra (tempo z „Free BPM” albo z projektu).
- **TUNEBOY**: melodia gra **tylko dopóki trzymasz nutę** (klawisz albo nuta w piano roll). Puszczasz = cisza (zostaje tylko krótki release i pogłos). C5 = oryginalna wysokość, inne nuty = transpozycja. Każde nowe naciśnięcie zaczyna od 1. taktu (w ADVANCED → START możesz przełączyć na „Song position”, wtedy melodia idzie równo z pozycją utworu). Najprościej: narysuj w piano roll jedną długą nutę C5 na 4/8/16 taktów. **NEW MELODY** = nowy seed, **RANDOMIZE** = nowa melodia + brzmienie.
  - **EXPORT MIDI** zapisuje plik `.mid`; **DRAG MIDI** — złap przycisk i przeciągnij na Playlist albo do Piano roll w FL.
  - Wyjście MIDI wtyczki: FL Studio nie zawsze przekazuje MIDI out z VST3 dalej — najpewniej jest przeciągnąć/eksportować MIDI do piano rolla innego syntezatora.

## Presety
**KICKBOY:** Euphoric Classic, Euphoric Punchy, Euphoric Long Tail, Raw Classic, Raw Screech, Rawstyle Gated, Rawstyle Distorted Tail, Gated Hard, Gated Short Stab, Zaag Saw, Zaag Folded, Crushed Grit, Early Hardstyle, Uptempo Tight, Hardcore Fold, Deep Rumble, Punch Only, Festival Mainstage, Dark Tunnel, Clean Sub

**BASSBOY:** Euphoric Reverse, Classic Hardstyle 2010, Deep Sine Swell, Raw Saw Reverse, Rolling Eighths, Gallop Rider, Bounce Groove, Half-time Breakdown, Build-up Riser, Acid Squelch, Folded Grit, Crushed Low, Wide Supersaw Bass, Sub Octave Monster, Tight Short Pump, Long Swell, Hard Clip Punch, Gliding Legato, Dark Reverse, Screaming Raw

**TUNEBOY:** Euphoric Anthem A-, Harmonic Minor Epic, Phrygian Dark Intro, Festival Sunrise, Rolling 16ths, Emotional Breakdown, Raw Intro Lead, Dorian Hope, Melodic Minor Shine, Major Uplift, Q-Style Intro, Supersaw Wall, Pluck Melody, Anthem Long 16 Bars, Sparse Mystic, Dotted Hero, Uptempo Rush, Low Octave Anthem, Hymn Chords Only, Euphoric Climax

## Wersja 1.1 (poprawki)
- TUNEBOY: koniec z „niekończącą się” melodią — zatrzymuje się po puszczeniu nuty, po zatrzymaniu transportu, po All Notes Off (CC123/CC120) i przy bypassie.
- BASSBOY: podwójne note-on nie zawiesza basu; stop transportu = cisza.
- Wszystkie: nowy kompilator Windows (zgodny z MSVC), wbudowana czcionka, zabezpieczenia przed nietypowymi rozmiarami bufora i brakiem inicjalizacji.

## Zasoby
Bez sampli i grafik (UI rysowane wektorowo; jedyny zasób to wbudowana czcionka Barlow, ok. 200 KB, licencja OFL). Zużycie CPU w teście: ok. 0,4–1,4% jednego rdzenia na instancję (44,1 kHz).
