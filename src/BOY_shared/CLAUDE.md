# BOY series (KICKBOY / BASSBOY / TUNEBOY) by sonicsystems — read first

Three light VST3 instruments the user brought on 2026-10-18 (v1.x came from another tool as one CMake project with
manufacturer "Sergio Parowka" / `Sprw`; the original zip is unpacked in `Desktop\danger\HardstylePlugins`, review in
its `REVIEW.md`). Since 2.0 they live here, one project per plugin, built by `build_all.py` like the rest.

## Identity (frozen from 2.0 on)
| plugin | folder | target | PLUGIN_CODE | bundle / product |
|---|---|---|---|---|
| KICKBOY | `src/KICKBOY_kod_zrodlowy/kickboy` | `BoyKickboy` | `Kkby` | `com.sonicsystems.boy.kickboy` / "KICKBOY" |
| BASSBOY | `src/BASSBOY_kod_zrodlowy/bassboy` | `BoyBassboy` | `Bsby` | `com.sonicsystems.boy.bassboy` / "BASSBOY" |
| TUNEBOY | `src/TUNEBOY_kod_zrodlowy/tuneboy` | `BoyTuneboy` | `Tnby` | `com.sonicsystems.boy.tuneboy` / "TUNEBOY" |
Manufacturer `Sncs` "sonicsystems" (the user chose to switch from Sprw, knowing 1.x projects won't find 2.0).
VST3 only (no CLAP / Standalone: smallest footprint). Parameter IDs frozen; add new ones only at the END of the spec list.

## The user's rules for this series
- Keep the **green look** (`col::` in BoyCommon.h) and the UI **very simple**, but not monotone: different control
  types per kind of setting + one hero picture per plugin that is also a control.
- **Least RAM possible**: the editor is ONE painted component (BoyUI.h) — no Slider/Label/ComboBox children, no
  images, one embedded font (Barlow Semi Condensed Bold), repaint only on change (15 Hz check), JUCE software renderer
  (no Direct2D device per window). Don't add bitmap assets or per-control components.

## Code map
- `BOY_shared/BoyCommon.h` — `Spec` (+ `Ui` kind, icon set), `P/C/I` helpers, `addOwnSoundSpecs`, `BaseProcessor`
  (factory presets editor-only: getNumPrograms() = 1; MY PRESETS `Documents/sonicsystems Presets/<NAME>/*.boypreset`;
  RANDOMIZE with undo stack + FREEZE (state property `frozen`); every set is a complete host gesture; hardened
  processBlock; NaN-safe state load; `stateLoaded()` hook), DSP helpers (`distort`, `SVF`, `Smooth`, `foldBass`...).
- `BOY_shared/BoyUI.h` — `LnF` (font, menu colours), `Editor` (top bar: presets, RANDOMIZE, undo, ADVANCED; auto flow
  layout; controls: bigKnob, knob, fader, bar, segments (+ icons), stepper, toggle, number; right-click menu with host
  items / Create automation clip / Freeze / reset; hero hooks; side buttons; file drop).
- `BOY_shared/BoyTest.h` — the shared render test (`boytest::testMain`), included by each `Tools/RenderDemo.cpp`,
  which first `#include "../Source/<X>.cpp"` (the tool compiles the plugin source itself).
- `BOY_shared/boy.cmake` — `boy_use(target)`: include paths (+ `SONICSYSTEMS_shared` for OWN SOUND), MP3 decoder, font.

## Per plugin (2.0)
- KICKBOY: play the key (note folded into E1..D#2, Tune 0); retrigger declick (the jump at a new note fades in 3 ms);
  Note End FULL/CUT (`noteend`); OWN SOUND (`Sonic::OwnSound`, params own*); hero = pitch curve (dot on the curve at
  half the decay: drag = Pitch Decay / Pitch Start).
- BASSBOY: same note map as KICKBOY; Voices Mono/Chord (up to 4, shared fold); 2x oversampled distortion (`os`);
  real-cents detune; 14 patterns + Custom (`custom` = 16-bit step mask, edited by clicking the hero's steps);
  hero = one bar of swells, drag = Kick Gap / Swell, playhead.
- TUNEBOY: real-cents detune; resume sounding notes on key change (both modes); a change while playing applies at the
  next bar line; VARIATION (`variation`, hidden param) + bar locks (state property `locks`, lock code = 1 + seed +
  10000 * variation); 10 rhythms; tempo + track names in the .mid; hero = roll with playhead, click bar numbers = lock.

## Test
`<X>Render demo.wav gui.png` must print `RESULT: PASS` (build_all runs it). Also writes `gui_advanced.png`.
Local builds: never more than 2 parallel jobs.
