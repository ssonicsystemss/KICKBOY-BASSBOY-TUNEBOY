// TUNEBOY by sonicsystems - hardstyle intro / euphoric melody generator with a built-in supersaw lead, BOY series.
// Key, scale, chord progression, bars, rhythm, density, seed -> a melody + chord layer, played while you hold a key
// (C5 = as generated, other keys transpose) or following the song position. NEW MELODY / VARIATION, lock the bars you
// like (click the bar numbers), EXPORT / DRAG MIDI. Changes made while it plays start at the next bar.
// 2.1: drag the notes in the roll (move / length / add / delete: a bar you touch becomes yours), 16 + Custom progressions,
// OWN SOUND (your own sample plays the notes), a bank of synth sounds and a random sound.
// 2.2: DRAW mode (paint a melody with the mouse), edit undo, copy / transpose a bar, 3 more scales, 10 more progressions,
// 4 more rhythms, 12 more sounds, CHORD STYLE (held / offbeat stabs / arp / gate) and PUMP (a built-in sidechain duck).
// RANDOMIZE no longer touches the sound (synth params + reverb): only the melody side; NEW SOUND is the sound dice.
#include "BoyUI.h"
#include "OwnSound.h"
#include <map>

using namespace boy;

namespace
{
const juce::StringArray keyNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const int scales[][7] = {
    { 0, 2, 3, 5, 7, 8, 10 },   // Natural minor
    { 0, 2, 3, 5, 7, 8, 11 },   // Harmonic minor
    { 0, 1, 3, 5, 7, 8, 10 },   // Phrygian
    { 0, 2, 3, 5, 7, 9, 10 },   // Dorian
    { 0, 2, 3, 5, 7, 9, 11 },   // Melodic minor
    { 0, 2, 4, 5, 7, 9, 11 },   // Major
    { 0, 1, 4, 5, 7, 8, 10 },   // Phrygian Dominant (appended in 2.2)
    { 0, 2, 3, 6, 7, 8, 11 },   // Hungarian Minor
    { 0, 2, 4, 6, 7, 9, 11 },   // Lydian
};
constexpr int numScales = 9;
// chord roots as scale degrees (0-based), 4 chords
constexpr int numProgs = 16, customProg = 16;   // 0..15 = the list, 16 = Custom (your own 4 chords, params c1..c4)
const int progs[][4] = {
    { 0, 5, 2, 6 },   // i  VI III VII  (the euphoric classic)
    { 5, 6, 0, 0 },   // VI VII i  i
    { 0, 6, 5, 6 },   // i  VII VI VII
    { 0, 3, 5, 4 },   // i  iv VI v
    { 5, 3, 0, 6 },   // VI iv i  VII
    { 0, 5, 3, 4 },   // i  VI iv v
    { 3, 5, 6, 0 },   // iv VI VII i
    { 0, 2, 6, 5 },   // i  III VII VI
    { 0, 4, 5, 3 },   // i  v  VI  iv
    { 5, 2, 6, 0 },   // VI III VII i
    { 0, 2, 5, 6 },   // i  III VI VII
    { 0, 6, 3, 4 },   // i  VII iv v
    { 3, 0, 6, 5 },   // iv i  VII VI
    { 0, 5, 6, 4 },   // i  VI VII v
    { 5, 6, 4, 0 },   // VI VII v  i
    { 0, 3, 6, 2 },   // i  iv VII III
};
constexpr int numExtraProgs = 10, numAllProgs = 17 + numExtraProgs;   // 17..26 = appended in 2.2 (after Custom, so saved projects keep their index)
const int extraProgs[][4] = {
    { 0, 6, 5, 4 },   // i  VII VI v
    { 0, 5, 6, 2 },   // i  VI VII III
    { 5, 0, 2, 6 },   // VI i  III VII
    { 0, 3, 0, 4 },   // i  iv i   v
    { 0, 2, 3, 6 },   // i  III iv VII
    { 6, 5, 4, 0 },   // VII VI v  i
    { 0, 4, 3, 6 },   // i  v  iv VII
    { 2, 6, 0, 5 },   // III VII i VI
    { 0, 1, 6, 0 },   // i  II VII i  (phrygian flavour)
    { 3, 6, 2, 5 },   // iv VII III VI
};
const juce::StringArray progNames { "i-VI-III-VII", "VI-VII-i-i", "i-VII-VI-VII", "i-iv-VI-v", "VI-iv-i-VII", "i-VI-iv-v", "iv-VI-VII-i", "i-III-VII-VI",
                                     "i-v-VI-iv", "VI-III-VII-i", "i-III-VI-VII", "i-VII-iv-v", "iv-i-VII-VI", "i-VI-VII-v", "VI-VII-v-i", "i-iv-VII-III", "Custom",
                                     "i-VII-VI-v", "i-VI-VII-III", "VI-i-III-VII", "i-iv-i-v", "i-III-iv-VII", "VII-VI-v-i", "i-v-iv-VII", "III-VII-i-VI", "i-II-VII-i", "iv-VII-III-VI" };
// rhythm templates: probability per 16th step for one bar
const float rhythmTpl[][16] = {
    { 1,0,.9f,0, .9f,0,.9f,0, 1,0,.9f,0, .9f,0,.9f,0 },                 // Straight 1/8
    { 1,0,0,.9f, 0,0,.9f,0, 1,0,0,.9f, 0,.6f,.9f,0 },                   // Syncopated (3-3-2)
    { 1,.6f,.8f,.6f, .9f,.6f,.8f,.6f, 1,.6f,.8f,.6f, .9f,.6f,.8f,.6f }, // Euphoric 1/16
    { 1,0,0,0, 0,0,0,0, .9f,0,0,0, .5f,0,0,0 },                         // Long notes
    { 1,0,0,.9f, 0,0,.9f,0, 0,.9f,0,0, .9f,0,0,0 },                     // Dotted
    { .3f,0,1,0, 0,0,1,0, .3f,0,1,0, 0,0,1,0 },                          // Offbeat stabs
    { 1,0,.9f,.9f, 1,0,.9f,.9f, 1,0,.9f,.9f, 1,0,.9f,.9f },             // Gallop
    { 1,.8f,.9f,.8f, .9f,.8f,.9f,.8f, 1,0,0,0, .4f,0,0,0 },             // Call & response
    { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 },                              // Rolling arp 1/16
    { 1,0,0,0, 0,0,.5f,0, 0,0,0,0, .6f,0,0,0 },                          // Half-time
    { 1,0,.9f,.9f, 0,.9f,.9f,0, 1,0,.9f,.9f, 0,.9f,.9f,0 },             // Reverse gallop (appended in 2.2)
    { 0,0,.9f,0, 0,0,1,0, 0,0,.9f,0, 0,.7f,1,0 },                       // Sparse offbeat
    { 1,0,0,0, .9f,0,.9f,0, 1,0,0,0, .6f,.9f,.9f,.9f },                 // Pickup anthem
    { 1,0,0,.9f, 0,0,.9f,0, 0,.9f,0,0, 1,0,.9f,0 },                     // Hoover stabs
};
const juce::StringArray rhythmNames { "Straight 1/8", "Syncopated", "Euphoric 1/16", "Long Notes", "Dotted",
                                      "Offbeat Stabs", "Gallop", "Call & Response", "Rolling Arp", "Half-time",
                                      "Reverse Gallop", "Sparse Offbeat", "Pickup Anthem", "Hoover Stabs" };
constexpr int numRhythms = 14;

struct GenParams { int key, scale, prog, bars, rhythm, octave, range, seed, variation; float density; std::array<int, 16> locks; std::array<int, 4> cust; int chordStyle; };
struct Note { double start, len; int pitch; float vel; bool chord; };
constexpr int maxNotes = 1024;

// ---- your own note edits: per bar, up to 32 notes on a 16th grid. Lock-free (atomics), so the audio thread reads
//      them while the editor writes. A bar with count >= 0 plays these notes instead of the generated melody.
struct EditNote { int step, len, pitch, vel; };   // step 0..15, len 1..16 (16ths), MIDI pitch, velocity 1..127
constexpr int maxEditPerBar = 32;
struct EditStore
{
    EditStore() { for (auto& c : count) c.store (-1); for (auto& p : packed) p.store (0); }
    static uint32_t pack (const EditNote& n) { return (uint32_t) ((n.step & 15) | (((n.len - 1) & 15) << 4) | ((n.pitch & 127) << 8) | ((n.vel & 127) << 15)); }
    static EditNote unpack (uint32_t v) { return { (int) (v & 15), (int) ((v >> 4) & 15) + 1, (int) ((v >> 8) & 127), (int) ((v >> 15) & 127) }; }
    bool edited (int bar) const { return count[(size_t) juce::jlimit (0, 15, bar)].load (std::memory_order_relaxed) >= 0; }
    int get (int bar, EditNote* out) const
    {
        bar = juce::jlimit (0, 15, bar);
        const int n = juce::jmin (maxEditPerBar, count[(size_t) bar].load (std::memory_order_acquire));
        for (int i = 0; i < n; ++i) out[i] = unpack (packed[(size_t) (bar * maxEditPerBar + i)].load (std::memory_order_relaxed));
        return n;
    }
    void set (int bar, const EditNote* in, int n)
    {
        bar = juce::jlimit (0, 15, bar); n = juce::jlimit (0, maxEditPerBar, n);
        for (int i = 0; i < n; ++i) packed[(size_t) (bar * maxEditPerBar + i)].store (pack (in[i]), std::memory_order_relaxed);
        count[(size_t) bar].store (n, std::memory_order_release);
        rev.fetch_add (1);
    }
    void clearBar (int bar) { count[(size_t) juce::jlimit (0, 15, bar)].store (-1); rev.fetch_add (1); }
    void clearAll() { for (auto& c : count) c.store (-1); rev.fetch_add (1); }
    bool any() const { for (auto& c : count) if (c.load() >= 0) return true; return false; }
    std::array<std::atomic<int>, 16> count;
    std::array<std::atomic<uint32_t>, 16 * maxEditPerBar> packed;
    std::atomic<int> rev { 0 };
};
using GenKey = std::array<int, 32>;

// ---- drawing helpers (pure, so the render test can check them)
// one 16th step is painted: notes that start there are replaced, an earlier long note is cut so it ends here
void drawStep (std::vector<EditNote>& cur, int step, int pitch)
{
    for (size_t i = 0; i < cur.size();)
    {
        auto& e = cur[i];
        if (e.step == step) { cur.erase (cur.begin() + (long) i); continue; }
        if (e.step < step && e.step + e.len > step) e.len = step - e.step;
        ++i;
    }
    if ((int) cur.size() < maxEditPerBar) cur.push_back ({ step, 1, pitch, 100 });
}
void eraseStep (std::vector<EditNote>& cur, int step)
{
    for (size_t i = 0; i < cur.size();)
    {
        auto& e = cur[i];
        if (e.step == step) { cur.erase (cur.begin() + (long) i); continue; }
        if (e.step < step && e.step + e.len > step) e.len = step - e.step;
        ++i;
    }
}
// a run of painted steps on the same pitch becomes one longer note (only steps you painted, `drawn` = bit per step)
void mergeRuns (std::vector<EditNote>& cur, unsigned drawn)
{
    std::sort (cur.begin(), cur.end(), [] (const EditNote& a, const EditNote& b) { return a.step < b.step; });
    for (size_t i = 0; i + 1 < cur.size();)
    {
        auto& a = cur[i]; const auto& b = cur[i + 1];
        const bool both = ((drawn >> a.step) & 1u) && ((drawn >> b.step) & 1u);
        if (both && a.pitch == b.pitch && a.step + a.len == b.step && a.step + a.len + b.len <= 16) { a.len += b.len; cur.erase (cur.begin() + (long) i + 1); }
        else ++i;
    }
}

struct Rng { uint32_t s; float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffff) / 16777216.0f; }
             int range (int a, int b) { return a + (int) (next() * (float) (b - a + 1) * 0.9999f); } };

// 2 bars of rhythm + degree offsets relative to the chord root, from one seed (+ a variation that nudges it)
struct Motif { bool on[32]; int off[32]; float acc[32]; };
Motif makeMotif (const GenParams& g, int seed, int variation)
{
    Motif m {};
    Rng r { (uint32_t) seed * 2654435761u + 7u }; for (int i = 0; i < 4; ++i) r.next();
    const float* t = rhythmTpl[juce::jlimit (0, numRhythms - 1, g.rhythm)];
    int cur = 2 + r.range (0, 2);
    for (int s = 0; s < 32; ++s)
    {
        const float p = t[s % 16]; const bool strong = (s % 8) == 0;
        m.on[s] = strong || r.next() < p * (0.35f + 0.65f * g.density);
        if (s == 0) m.on[s] = true;
        const int stepMove = r.next() < 0.7f ? (r.next() < 0.5f ? -1 : 1) : r.range (-3, 3);
        if (strong) { const int ct[] = { 0, 2, 4, 7 }; cur = ct[r.range (0, 3)]; } else cur += stepMove;
        cur = juce::jlimit (-2, juce::jmax (2, g.range), cur);
        m.off[s] = cur; m.acc[s] = strong ? 1.0f : (s % 4 == 0 ? 0.85f : 0.7f);
    }
    if (variation > 0)   // VARIATION: same feel, some weak notes move a step or two, a few appear / disappear
    {
        Rng v { (uint32_t) seed * 7919u + (uint32_t) variation * 104729u + 3u }; for (int i = 0; i < 3; ++i) v.next();
        for (int s = 1; s < 32; ++s)
        {
            if (s % 8 == 0) continue;
            if (v.next() < 0.35f) m.off[s] = juce::jlimit (-2, juce::jmax (2, g.range), m.off[s] + (v.next() < 0.5f ? -1 : 1) * (v.next() < 0.7f ? 1 : 2));
            if (v.next() < 0.15f) m.on[s] = ! m.on[s];
        }
    }
    return m;
}

// lock code of a bar: 0 = free, else 1 + seed + 10000 * variation (the melody it had when you locked it)
int lockCode (int seed, int variation) { return 1 + seed + 10000 * variation; }

// pure, allocation-free melody generator (used by the audio thread AND the editor)
int generate (const GenParams& g, Note* out, const EditStore* ed = nullptr)
{
    const int* sc = scales[juce::jlimit (0, numScales - 1, g.scale)];
    const int* pr = g.prog == customProg ? g.cust.data() : g.prog > customProg ? extraProgs[juce::jlimit (0, numExtraProgs - 1, g.prog - customProg - 1)]
                                                                             : progs[juce::jlimit (0, numProgs - 1, g.prog)];
    const int root = 12 * (g.octave + 1) + g.key;
    auto degToNote = [&] (int d) { const int o = (d >= 0 ? d / 7 : (d - 6) / 7); const int i = d - o * 7; return root + o * 12 + sc[i]; };
    const Motif free = makeMotif (g, g.seed, g.variation);
    int n = 0;
    const int bars = g.bars, barsPerChord = bars >= 16 ? 2 : 1;
    // the chord layer: Held (one entry per chord span), Offbeat stabs, Arp 1/16, Gate 1/8
    auto emitChords = [&] (int b, int deg)
    {
        const double base = b * 4.0;
        switch (g.chordStyle)
        {
            case 1: for (int beat = 0; beat < 4; ++beat) for (int k = 0; k < 3; ++k) out[n++] = { base + beat + 0.5, 0.4, degToNote (deg + k * 2 - 7), 0.55f, true }; break;
            case 2: { static const int order[4] = { 0, 1, 2, 1 }; for (int st = 0; st < 16; ++st) out[n++] = { base + st * 0.25, 0.2, degToNote (deg + order[st % 4] * 2 - 7), 0.55f, true }; break; }
            case 3: for (int e = 0; e < 8; ++e) for (int k = 0; k < 3; ++k) out[n++] = { base + e * 0.5, 0.4, degToNote (deg + k * 2 - 7), 0.55f, true }; break;
            default: if (b % barsPerChord == 0) for (int k = 0; k < 3; ++k) out[n++] = { base, barsPerChord * 4.0 - 0.05, degToNote (deg + k * 2 - 7), 0.55f, true };
        }
    };
    for (int b = 0; b < bars && n < maxNotes - 64; ++b)
    {
        const int chordDeg0 = pr[(b / barsPerChord) % 4];
        if (ed != nullptr && ed->edited (b))   // this bar is yours: your notes instead of the generated melody
        {
            EditNote en[maxEditPerBar];
            const int ne = ed->get (b, en);
            for (int i = 0; i < ne; ++i)
                out[n++] = { b * 4.0 + en[i].step * 0.25, (double) juce::jmin (en[i].len, 16 - en[i].step) * 0.25, en[i].pitch, (float) en[i].vel / 127.0f, false };
            emitChords (b, chordDeg0);
            continue;
        }
        const int code = g.locks[(size_t) juce::jlimit (0, 15, b)];
        const int bSeed = code > 0 ? (code - 1) % 10000 : g.seed, bVar = code > 0 ? (code - 1) / 10000 : g.variation;
        const Motif mo = code > 0 ? makeMotif (g, bSeed, bVar) : free;
        Rng rb { (uint32_t) bSeed * 31u + (uint32_t) b * 977u + (uint32_t) bVar * 13u + 1u };
        const int chordDeg = pr[(b / barsPerChord) % 4];
        const bool phraseEnd = (b % 4) == 3, variation = (b % 8) >= 4;
        int lastOn = -1;
        for (int s = 0; s < 16; ++s)
        {
            const int ms = (b % 2) * 16 + s;
            bool o = mo.on[ms];
            if (phraseEnd && s >= 8) o = (s == 8);
            if (! o) continue;
            int d = chordDeg + mo.off[ms];
            if (variation && (ms % 8) == 4) d += 2;                                   // answer phrase lifts
            if (phraseEnd && s == 8) d = (chordDeg % 7 == 0 ? 7 : chordDeg + 2);      // resolve to a chord tone
            const Note nt { b * 4.0 + s * 0.25, 0.25, degToNote (d), mo.acc[ms] * (0.85f + 0.15f * rb.next()), false };
            if (lastOn >= 0) out[lastOn].len = juce::jmax (0.125, nt.start - out[lastOn].start - 0.02);
            out[n] = nt; lastOn = n++;
        }
        if (lastOn >= 0) out[lastOn].len = juce::jmax (0.125, (b + 1) * 4.0 - out[lastOn].start - 0.02);
        emitChords (b, chordDeg);
    }
    return n;
}

enum { keyI, scaleI, progI, barsI, rhythmI, densI, octI, rangeI, seedI, chordsI, reverbI, outI,
       detuneI, widthI, cutoffI, resoI, fenvI, attackI, decayI, sustainI, releaseI, rsizeI, midioutI, playI, syncI, variationI, modeI,
       c1I, c2I, c3I, c4I, ownLevelI_, ownTuneI_, ownKeyI_, ownModeI_,   // (the four OWN SOUND params, see addOwnSoundSpecs)
       chordstyleI, pumpI };

std::vector<Spec> tuneSpecs()
{
    std::vector<Spec> s {
        C ("key",    "Key",       keyNames, 9, true, false),
        C ("scale",  "Scale",     { "Minor", "Harmonic Minor", "Phrygian", "Dorian", "Melodic Minor", "Major", "Phrygian Dominant", "Hungarian Minor", "Lydian" }, 0, true),
        C ("prog",   "Progression", progNames, 0, true),
        C ("bars",   "Bars",      { "4", "8", "16" }, 1, true, false),
        C ("rhythm", "Rhythm",    rhythmNames, 1, true),
        P ("density","Density",   0, 1, 0.6f, true, 0.35f, 0.9f),
        I ("octave", "Octave",    3, 6, 5, true, 4, 5),
        I ("range",  "Range",     3, 12, 7, true, 4, 9, "deg"),
        I ("seed",   "Seed",      1, 9999, 1, true, 1, 9999),
        P ("chords", "Chords",    0, 1, 0.35f, true, 0.2f, 0.5f),
        P ("reverb", "Reverb",    0, 1, 0.35f, true, 0.35f, 0.35f),
        P ("out",    "Output",    -24, 6, -6, true, -6, -6, "dB"),
        // ADVANCED (synth)
        P ("detune", "Detune",    0, 60, 22, false, 22, 22, "ct").as (Ui::bigKnob),
        P ("width",  "Width",     0, 1, 0.7f, false, 0.7f, 0.7f),
        P ("cutoff", "Cutoff",    300, 16000, 6000, false, 6000, 6000, "Hz", 3000),
        P ("reso",   "Resonance", 0.5f, 4, 0.8f, false, 0.8f, 0.8f),
        P ("fenv",   "Filter Env",0, 4, 1.2f, false, 1.2f, 1.2f, "oct"),
        P ("attack", "Attack",    0, 300, 3, false, 3, 3, "ms", 30),
        P ("decay",  "Decay",     10, 2000, 400, false, 400, 400, "ms", 300),
        P ("sustain","Sustain",   0, 1, 0.7f, false, 0.7f, 0.7f),
        P ("release","Release",   5, 2000, 250, false, 250, 250, "ms", 300),
        P ("rsize",  "Reverb Size",0, 0.9f, 0.8f, false, 0.8f, 0.8f),
        C ("midiout","MIDI Out",  { "Melody", "Melody+Chords", "Off" }, 0, false, false),
        C ("play",   "Play",      { "Both", "Melody", "Chords" }, 0, false, false),
        C ("sync",   "Start",     { "Key (restart)", "Song position" }, 0, false, false),
        I ("variation", "Variation", 0, 99, 0, false, 0, 0).as (Ui::hidden),
        C ("mode",   "Mode",      { "Melody", "Single Note" }, 0, true, false),
        // appended in 2.1: your own progression (used when Progression = Custom), degrees 1..7 of the scale
        I ("c1", "Custom Chord 1", 1, 7, 1, false, 1, 1),
        I ("c2", "Custom Chord 2", 1, 7, 6, false, 6, 6),
        I ("c3", "Custom Chord 3", 1, 7, 3, false, 3, 3),
        I ("c4", "Custom Chord 4", 1, 7, 7, false, 7, 7),
    };
    addOwnSoundSpecs (s, true);   // OWN SOUND: your own sample plays the notes (Layer / Replace), pitch follows the notes
    // appended in 2.2 (after OWN SOUND, so nothing that was saved moves)
    s.push_back (C ("chordstyle", "Chord Style", { "Held", "Offbeat", "Arp", "Gate" }, 0, true, false));
    s.push_back (P ("pump", "Pump", 0, 1, 0, true, 0, 0));   // a built-in sidechain duck on every beat (no kick plugin needed)
    return s;
}

// the synth sound params: a sound = these ten (the melody params are not touched)
const char* const soundIds[] = { "detune", "width", "cutoff", "reso", "fenv", "attack", "decay", "sustain", "release", "rsize" };
const float soundLo[] = { 10, 0.5f, 3000, 0.6f, 0.5f, 1, 200, 0.5f, 120, 0.6f };   // NEW SOUND picks inside these ranges
const float soundHi[] = { 35, 0.9f, 10000, 1.4f, 2.0f, 20, 900, 0.9f, 500, 0.9f };
std::vector<Preset> soundBank()
{
    return {
        { "Euphoric Supersaw", { {"detune",22},{"width",0.7f},{"cutoff",6000},{"reso",0.8f},{"fenv",1.2f},{"attack",3},{"decay",400},{"sustain",0.7f},{"release",250},{"rsize",0.8f} } },
        { "Raw Screech",       { {"detune",35},{"width",0.8f},{"cutoff",9500},{"reso",2.2f},{"fenv",2.0f},{"attack",1},{"decay",300},{"sustain",0.6f},{"release",150},{"rsize",0.5f} } },
        { "Soft Pluck",        { {"detune",14},{"width",0.6f},{"cutoff",3500},{"reso",1.0f},{"fenv",2.5f},{"attack",1},{"decay",180},{"sustain",0.1f},{"release",160},{"rsize",0.7f} } },
        { "Epic Pad",          { {"detune",40},{"width",1.0f},{"cutoff",2500},{"reso",0.7f},{"fenv",0.4f},{"attack",220},{"decay",900},{"sustain",0.9f},{"release",1400},{"rsize",0.9f} } },
        { "Bright Hoover",     { {"detune",50},{"width",0.9f},{"cutoff",12000},{"reso",1.4f},{"fenv",1.5f},{"attack",2},{"decay",500},{"sustain",0.8f},{"release",220},{"rsize",0.6f} } },
        { "Dark Lead",         { {"detune",18},{"width",0.5f},{"cutoff",2200},{"reso",1.8f},{"fenv",1.8f},{"attack",4},{"decay",350},{"sustain",0.6f},{"release",300},{"rsize",0.8f} } },
        { "Glass Bell",        { {"detune",8},{"width",0.9f},{"cutoff",7000},{"reso",0.6f},{"fenv",0.8f},{"attack",1},{"decay",1200},{"sustain",0.05f},{"release",900},{"rsize",0.9f} } },
        { "Wide Anthem",       { {"detune",45},{"width",1.0f},{"cutoff",7500},{"reso",0.9f},{"fenv",1.0f},{"attack",6},{"decay",600},{"sustain",0.8f},{"release",450},{"rsize",0.85f} } },
        { "Rave Stab",         { {"detune",25},{"width",0.7f},{"cutoff",5000},{"reso",1.5f},{"fenv",3.0f},{"attack",1},{"decay",140},{"sustain",0.0f},{"release",100},{"rsize",0.4f} } },
        { "Sunrise Strings",   { {"detune",30},{"width",0.95f},{"cutoff",4000},{"reso",0.6f},{"fenv",0.3f},{"attack",280},{"decay",1500},{"sustain",0.85f},{"release",1100},{"rsize",0.9f} } },
        { "Mono Hero",         { {"detune",10},{"width",0.3f},{"cutoff",5500},{"reso",1.2f},{"fenv",1.4f},{"attack",2},{"decay",450},{"sustain",0.7f},{"release",200},{"rsize",0.6f} } },
        { "Rave Lead",         { {"detune",28},{"width",0.8f},{"cutoff",8000},{"reso",1.6f},{"fenv",2.2f},{"attack",1},{"decay",260},{"sustain",0.5f},{"release",180},{"rsize",0.55f} } },
        { "Warm Keys",         { {"detune",6},{"width",0.5f},{"cutoff",3000},{"reso",0.7f},{"fenv",0.8f},{"attack",3},{"decay",800},{"sustain",0.3f},{"release",500},{"rsize",0.7f} } },
        { "Airy Choir",        { {"detune",20},{"width",1.0f},{"cutoff",3200},{"reso",0.8f},{"fenv",0.5f},{"attack",120},{"decay",1000},{"sustain",0.8f},{"release",900},{"rsize",0.9f} } },
        { "Acid Squelch",      { {"detune",4},{"width",0.3f},{"cutoff",1800},{"reso",3.6f},{"fenv",3.5f},{"attack",1},{"decay",250},{"sustain",0.3f},{"release",120},{"rsize",0.3f} } },
        { "Punch Brass",       { {"detune",12},{"width",0.6f},{"cutoff",4200},{"reso",0.9f},{"fenv",2.2f},{"attack",18},{"decay",350},{"sustain",0.65f},{"release",200},{"rsize",0.5f} } },
        { "Euphoric Pluck Lead", { {"detune",24},{"width",0.8f},{"cutoff",5500},{"reso",1.1f},{"fenv",2.8f},{"attack",1},{"decay",320},{"sustain",0.35f},{"release",260},{"rsize",0.75f} } },
        { "Huge Detuned Wall", { {"detune",58},{"width",1.0f},{"cutoff",9000},{"reso",0.7f},{"fenv",0.6f},{"attack",4},{"decay",700},{"sustain",0.9f},{"release",500},{"rsize",0.8f} } },
        { "Bass Lead",         { {"detune",15},{"width",0.4f},{"cutoff",1500},{"reso",2.0f},{"fenv",2.0f},{"attack",2},{"decay",300},{"sustain",0.7f},{"release",150},{"rsize",0.3f} } },
        { "Crystal Arp",       { {"detune",10},{"width",0.9f},{"cutoff",8000},{"reso",0.9f},{"fenv",1.2f},{"attack",1},{"decay",150},{"sustain",0.0f},{"release",200},{"rsize",0.85f} } },
        { "Dreamy Lead",       { {"detune",26},{"width",0.85f},{"cutoff",4800},{"reso",0.9f},{"fenv",0.9f},{"attack",40},{"decay",800},{"sustain",0.75f},{"release",700},{"rsize",0.9f} } },
        { "Festival Lead",     { {"detune",32},{"width",0.9f},{"cutoff",10000},{"reso",1.3f},{"fenv",1.8f},{"attack",2},{"decay",400},{"sustain",0.75f},{"release",280},{"rsize",0.7f} } },
        { "Soft Pad",          { {"detune",3},{"width",0.7f},{"cutoff",1200},{"reso",0.5f},{"fenv",0.2f},{"attack",200},{"decay",1200},{"sustain",0.9f},{"release",1300},{"rsize",0.9f} } },
        { "Reese Growl",       { {"detune",55},{"width",0.5f},{"cutoff",2000},{"reso",2.5f},{"fenv",1.0f},{"attack",2},{"decay",500},{"sustain",0.8f},{"release",200},{"rsize",0.35f} } },
    };
}

std::vector<Preset> tunePresets()
{
    return {
        { "Euphoric Anthem A-",       { {"key",9},{"scale",0},{"prog",0},{"rhythm",1},{"seed",101} } },
        { "Harmonic Minor Epic",      { {"key",4},{"scale",1},{"prog",0},{"rhythm",1},{"seed",202} } },
        { "Phrygian Dark Intro",      { {"key",2},{"scale",2},{"prog",2},{"rhythm",4},{"seed",303},{"cutoff",4000} } },
        { "Festival Sunrise",         { {"key",7},{"scale",0},{"prog",1},{"rhythm",3},{"seed",404},{"reverb",0.5f},{"attack",30} } },
        { "Rolling 16ths",            { {"key",11},{"scale",0},{"prog",3},{"rhythm",2},{"seed",505},{"density",0.7f} } },
        { "Emotional Breakdown",      { {"key",5},{"scale",0},{"prog",4},{"rhythm",3},{"seed",606},{"reverb",0.6f},{"attack",60},{"release",800} } },
        { "Raw Intro Lead",           { {"key",1},{"scale",1},{"prog",5},{"rhythm",4},{"seed",707},{"cutoff",9000},{"chords",0.2f} } },
        { "Dorian Hope",              { {"key",2},{"scale",3},{"prog",6},{"rhythm",1},{"seed",808} } },
        { "Melodic Minor Shine",      { {"key",0},{"scale",4},{"prog",0},{"rhythm",0},{"seed",909} } },
        { "Major Uplift",             { {"key",7},{"scale",5},{"prog",7},{"rhythm",1},{"seed",111} } },
        { "Q-Style Intro",            { {"key",8},{"scale",1},{"prog",2},{"rhythm",1},{"seed",1212},{"bars",2} } },
        { "Supersaw Wall",            { {"key",9},{"scale",0},{"prog",0},{"rhythm",0},{"seed",1313},{"detune",40},{"chords",0.5f} } },
        { "Pluck Melody",             { {"key",4},{"scale",0},{"prog",3},{"rhythm",2},{"seed",1414},{"decay",180},{"sustain",0.2f},{"release",150} } },
        { "Anthem Long 16 Bars",      { {"key",3},{"scale",0},{"prog",0},{"rhythm",1},{"seed",1515},{"bars",2} } },
        { "Sparse Mystic",            { {"key",6},{"scale",2},{"prog",4},{"rhythm",3},{"seed",1616},{"density",0.3f},{"reverb",0.6f} } },
        { "Dotted Hero",              { {"key",10},{"scale",1},{"prog",5},{"rhythm",4},{"seed",1717} } },
        { "Uptempo Rush",             { {"key",1},{"scale",0},{"prog",2},{"rhythm",2},{"seed",1818},{"density",0.85f},{"range",9} } },
        { "Low Octave Anthem",        { {"key",9},{"scale",1},{"prog",0},{"rhythm",1},{"seed",1919},{"octave",4} } },
        { "Hymn Chords Only",         { {"key",0},{"scale",0},{"prog",1},{"rhythm",3},{"seed",2020},{"play",2},{"chords",0.7f},{"attack",120} } },
        { "Euphoric Climax",          { {"key",11},{"scale",1},{"prog",0},{"rhythm",2},{"seed",2121},{"range",10},{"detune",30} } },
        { "Offbeat Stab Lead",        { {"key",5},{"scale",0},{"prog",0},{"rhythm",5},{"seed",2222},{"decay",220},{"sustain",0.3f},{"density",0.8f} } },
        { "Gallop Anthem",            { {"key",7},{"scale",1},{"prog",3},{"rhythm",6},{"seed",2323},{"density",0.7f} } },
        { "Call And Response",        { {"key",9},{"scale",0},{"prog",4},{"rhythm",7},{"seed",2424},{"reverb",0.45f} } },
        { "Rolling Arp Climax",       { {"key",2},{"scale",1},{"prog",0},{"rhythm",8},{"seed",2525},{"decay",160},{"sustain",0.25f},{"range",9} } },
        { "Half-time Breakdown",      { {"key",4},{"scale",0},{"prog",1},{"rhythm",9},{"seed",2626},{"attack",40},{"reverb",0.6f},{"release",900} } },
        { "Hungarian Rawstyle",       { {"key",5},{"scale",7},{"prog",17},{"rhythm",6},{"seed",3131},{"density",0.75f},{"cutoff",9000},{"detune",35} } },
        { "Phrygian Dominant Rave",   { {"key",9},{"scale",6},{"prog",18},{"rhythm",10},{"seed",3232},{"cutoff",8000} } },
        { "Offbeat Chords + Pump",    { {"key",7},{"scale",0},{"prog",0},{"rhythm",1},{"seed",3333},{"chordstyle",1},{"pump",0.55f},{"chords",0.5f} } },
        { "Arp Chords Trance",        { {"key",4},{"scale",0},{"prog",20},{"rhythm",3},{"seed",3434},{"chordstyle",2},{"chords",0.45f},{"density",0.5f} } },
        { "Gated Hymn",               { {"key",0},{"scale",8},{"prog",19},{"rhythm",11},{"seed",3535},{"chordstyle",3},{"pump",0.4f},{"reverb",0.5f} } },
        { "Pickup Anthem Lead",       { {"key",11},{"scale",1},{"prog",21},{"rhythm",12},{"seed",3636},{"cutoff",8000} } },
        { "Hoover Stab Raw",          { {"key",1},{"scale",7},{"prog",22},{"rhythm",13},{"seed",3737},{"attack",1},{"decay",300},{"sustain",0.4f},{"detune",45} } },
        { "Crystal Dream",            { {"key",6},{"scale",8},{"prog",23},{"rhythm",3},{"seed",3838},{"reverb",0.6f},{"attack",30} } },
    };
}
}

class TuneProcessor : public BaseProcessor
{
public:
    TuneProcessor() : BaseProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true), tuneSpecs(), tunePresets())
    { for (auto& l : locks) l.store (0); own.attach (apvts); }
    const juce::String getName() const override { return BOY_NAME; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 3.0; }
    juce::AudioProcessorEditor* createEditor() override;

    GenParams genParams() const
    {
        static const int barsV[] = { 4, 8, 16 };
        GenParams g { (int) get (keyI), (int) get (scaleI), (int) get (progI), barsV[juce::jlimit (0, 2, (int) get (barsI))],
                      (int) get (rhythmI), (int) get (octI), (int) get (rangeI), (int) get (seedI), (int) get (variationI), get (densI), {}, {}, (int) get (chordstyleI) };
        for (int b = 0; b < 16; ++b) g.locks[(size_t) b] = locks[(size_t) b].load (std::memory_order_relaxed);
        for (int k = 0; k < 4; ++k) g.cust[(size_t) k] = juce::jlimit (0, 6, (int) get (c1I + k) - 1);
        return g;
    }
    int generateNow (Note* dst) const { return generate (genParams(), dst, &edits); }
    void randomizeExtra (std::mt19937&) override { setParam ("variation", 0.0f); }   // a new seed starts without variation
    // everything the melody depends on, to notice a change
    GenKey keyOf (const GenParams& g) const
    {
        GenKey k { g.key, g.scale, g.prog, g.bars, g.rhythm, g.octave, g.range, g.seed, g.variation, (int) (g.density * 1000.0f) };
        for (int b = 0; b < 16; ++b) k[(size_t) (10 + b)] = g.locks[(size_t) b];
        k[26] = edits.rev.load();
        k[31] = g.chordStyle;
        for (int c = 0; c < 4; ++c) k[(size_t) (27 + c)] = g.cust[(size_t) c];
        return k;
    }

    // ---- your note edits (kept in the state as "edits": bar=step.len.pitch.vel,...;bar=...)
    void saveEdits()
    {
        juce::StringArray bars;
        for (int b = 0; b < 16; ++b)
        {
            if (! edits.edited (b)) continue;
            EditNote en[maxEditPerBar]; const int n = edits.get (b, en);
            juce::StringArray ns;
            for (int i = 0; i < n; ++i) ns.add (juce::String (en[i].step) + "." + juce::String (en[i].len) + "." + juce::String (en[i].pitch) + "." + juce::String (en[i].vel));
            bars.add (juce::String (b) + "=" + ns.joinIntoString (","));
        }
        if (bars.isEmpty()) apvts.state.removeProperty ("edits", nullptr);
        else apvts.state.setProperty ("edits", bars.joinIntoString (";"), nullptr);
    }
    void loadEdits()
    {
        edits.clearAll();
        const auto text = apvts.state.getProperty ("edits").toString();
        for (auto& part : juce::StringArray::fromTokens (text, ";", ""))
        {
            const int eq = part.indexOfChar ('=');
            if (eq < 1) continue;
            const int bar = part.substring (0, eq).getIntValue();
            if (bar < 0 || bar > 15) continue;
            EditNote en[maxEditPerBar]; int n = 0;
            for (auto& tok : juce::StringArray::fromTokens (part.substring (eq + 1), ",", ""))
            {
                const auto f = juce::StringArray::fromTokens (tok, ".", "");
                if (f.size() != 4 || n >= maxEditPerBar) continue;
                en[n++] = { juce::jlimit (0, 15, f[0].getIntValue()), juce::jlimit (1, 16, f[1].getIntValue()),
                            juce::jlimit (0, 127, f[2].getIntValue()), juce::jlimit (1, 127, f[3].getIntValue()) };
            }
            edits.set (bar, en, n);
        }
    }
    EditStore edits;

    // ---- undo for your note edits (message thread only): a snapshot before every edit gesture
    struct EditSnap { std::array<int, 16> count; std::array<uint32_t, 16 * maxEditPerBar> packed; };
    void pushEditUndo()
    {
        EditSnap sn;
        for (int b = 0; b < 16; ++b) sn.count[(size_t) b] = edits.count[(size_t) b].load();
        for (size_t i = 0; i < sn.packed.size(); ++i) sn.packed[i] = edits.packed[i].load();
        editUndo.push_back (sn);
        if (editUndo.size() > 30) editUndo.erase (editUndo.begin());
    }
    bool canUndoEdit() const { return ! editUndo.empty(); }
    bool undoEdit()
    {
        if (editUndo.empty()) return false;
        const auto sn = editUndo.back(); editUndo.pop_back();
        for (int b = 0; b < 16; ++b)
        {
            const int c = sn.count[(size_t) b];
            if (c < 0) { edits.clearBar (b); continue; }
            EditNote en[maxEditPerBar];
            for (int i = 0; i < juce::jmin (c, maxEditPerBar); ++i) en[i] = EditStore::unpack (sn.packed[(size_t) (b * maxEditPerBar + i)]);
            edits.set (b, en, c);
        }
        saveEdits();
        return true;
    }
    std::vector<EditSnap> editUndo;

    // ---- sounds: a bank of synth sounds + a random sound (only the ten sound params, never the melody)
    juce::String soundName() const { return apvts.state.getProperty ("soundName", "Euphoric Supersaw").toString(); }
    void applySound (int idx)
    {
        static const auto bank = soundBank();
        if (! juce::isPositiveAndBelow (idx, (int) bank.size())) return;
        rememberForUndo();
        for (auto* id : soundIds) if (const int i = indexOf (id); i >= 0 && ! isFrozen (id)) setParam (id, specs[(size_t) i].def);
        for (auto& kv : bank[(size_t) idx].values) if (! isFrozen (kv.first)) setParam (kv.first, kv.second);
        apvts.state.setProperty ("soundName", bank[(size_t) idx].name, nullptr);
    }
    void randomSound()
    {
        std::random_device rd; std::mt19937 rng (rd());
        rememberForUndo();
        for (int k = 0; k < (int) (sizeof (soundIds) / sizeof (soundIds[0])); ++k)
        {
            const auto* id = soundIds[k];
            if (isFrozen (id)) continue;
            std::uniform_real_distribution<float> d (soundLo[k], soundHi[k]);
            setParam (id, d (rng));
        }
        apvts.state.setProperty ("soundName", "Random", nullptr);
    }
    Sonic::OwnSound own { Sonic::OwnSound::Config { false, 60, false } };   // up to 16 sample voices; C5 plays the sample as recorded

    // ---- bar locks (the editor's bar numbers); kept in the state as "locks"
    int lockOf (int bar) const { return locks[(size_t) juce::jlimit (0, 15, bar)].load(); }
    void toggleLock (int bar)
    {
        bar = juce::jlimit (0, 15, bar);
        locks[(size_t) bar].store (lockOf (bar) > 0 ? 0 : lockCode ((int) get (seedI), (int) get (variationI)));
        juce::StringArray s; for (auto& l : locks) s.add (juce::String (l.load()));
        apvts.state.setProperty ("locks", s.joinIntoString (","), nullptr);
    }
    void stateLoaded() override
    {
        own.restore();
        loadEdits();
        const auto s = juce::StringArray::fromTokens (apvts.state.getProperty ("locks").toString(), ",", "");
        for (int b = 0; b < 16; ++b) locks[(size_t) b].store (b < s.size() ? juce::jmax (0, s[b].getIntValue()) : 0);
    }

    void prepareDsp (double sampleRate, int) override
    {
        sr = (float) sampleRate; rev.setSampleRate (sampleRate); lastKey = {}; outMidi.ensureSize (8192); outSm.prepare (sr); own.prepare (sampleRate); resetDsp();
    }
    void resetDsp() override
    {
        for (auto& v : voices) v = Voice{};
        own.release (0, -1);
        rev.reset(); lim = Limiter{}; nHeld = 0; running = false; wasHostPlaying = false; pendingRegen = false; guiBeat.store (-1.0f);
        for (auto& c : outOn) for (auto& o : c) o = false;
    }
    void stopAll (juce::MidiBuffer& out) override { sendOffs (out, 0); resetDsp(); }

    bool writeMidiFile (const juce::File& f) const
    {
        std::vector<Note> tmp (maxNotes); const int n = generateNow (tmp.data());   // includes your edits
        juce::MidiMessageSequence mel, ch; const int tpq = 960;
        const bool chords = (int) get (midioutI) == 1;
        mel.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / juce::jlimit (20.0, 999.0, hostBpm.load()))), 0);
        mel.addEvent (juce::MidiMessage::textMetaEvent (3, "TUNEBOY melody"), 0);
        ch.addEvent (juce::MidiMessage::textMetaEvent (3, "TUNEBOY chords"), 0);
        for (int i = 0; i < n; ++i)
        {
            const auto& nt = tmp[(size_t) i];
            if (nt.chord && ! chords) continue;
            auto& seq = nt.chord ? ch : mel;
            seq.addEvent (juce::MidiMessage::noteOn (1, nt.pitch, nt.vel), nt.start * tpq);
            seq.addEvent (juce::MidiMessage::noteOff (1, nt.pitch), (nt.start + nt.len) * tpq);
        }
        mel.updateMatchedPairs(); ch.updateMatchedPairs();
        juce::MidiFile mf; mf.setTicksPerQuarterNote (tpq);
        mf.addTrack (mel); if (chords) mf.addTrack (ch);
        f.deleteFile(); juce::FileOutputStream os (f);
        return os.openedOk() && mf.writeTo (os);
    }

    // Melody plays ONLY while a key is held (transposed: key 60 = as generated).
    // "Key (restart)": starts at bar 1 on every new key press.  "Song position": follows the host position.
    void renderBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) override
    {
        buf.clear();
        const int n = buf.getNumSamples();
        juce::MidiBuffer& out = outMidi; out.clear();

        if (get (modeI) > 0.5f) { renderSingle (buf, midi, n, out); return; }
        if (wasSingle) { wasSingle = false; releaseVoices (0); nHeld = 0; running = false; justStarted = true; }

        const auto gp = genParams();
        const auto key = keyOf (gp);
        own.gate = true;
        if (key != lastKey)
        {
            if (! running) { regenerate (gp, out, 0); }
            else pendingRegen = true;          // while it plays: the new melody starts at the next bar
        }
        loopLen = gp.bars * 4.0;

        double bpm = 150, hostPpq = 0; bool hostPlaying = false;
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition())
        { if (auto b = pos->getBpm()) if (std::isfinite (*b)) bpm = juce::jlimit (20.0, 999.0, *b);
          if (pos->getIsPlaying()) if (auto q = pos->getPpqPosition()) if (std::isfinite (*q)) { hostPlaying = true; hostPpq = *q; } }
        hostBpm.store (bpm);
        ppqInc = bpm / 60.0 / sr;
        songMode = (int) get (syncI) == 1 && hostPlaying;
        blockHostPpq = hostPpq;
        blockBeat0 = running ? (songMode ? hostPpq : pos) : -1.0;   // where the beat is, for PUMP
        if (wasHostPlaying && ! hostPlaying) { nHeld = 0; running = false; releaseVoices (0); sendOffs (out, 0); }   // transport stop
        wasHostPlaying = hostPlaying;

        int cursor = 0;
        for (const auto m : midi)
        {
            const int e = juce::jlimit (0, n - 1, m.samplePosition);
            scheduleWithRegen (cursor, e, out); cursor = e;
            const auto msg = m.getMessage();
            if (msg.isNoteOn())
            {
                removeHeld (msg.getNoteNumber());
                if (nHeld < 16) held[nHeld++] = msg.getNoteNumber();
                releaseVoices (e); sendOffs (out, e);
                if (pendingRegen) regenerate (genParams(), out, e);
                running = true; pos = 0; transpose = msg.getNoteNumber() - 60; justStarted = true;
            }
            else if (msg.isNoteOff())
            {
                removeHeld (msg.getNoteNumber());
                releaseVoices (e); sendOffs (out, e);
                if (nHeld == 0) running = false;
                else { transpose = held[nHeld - 1] - 60; justStarted = true; }   // the notes already sounding come back
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff() || (msg.isController() && (msg.getControllerNumber() == 120 || msg.getControllerNumber() == 123)))
            {
                nHeld = 0; running = false; sendOffs (out, e);
                if (msg.isAllSoundOff() || msg.getControllerNumber() == 120) { for (auto& v : voices) v.active = false; rev.reset(); }
                else releaseVoices (e);
            }
        }
        scheduleWithRegen (cursor, n, out);
        if (! running && pendingRegen) regenerate (genParams(), out, n - 1);
        midi.swapWith (out);
        renderSynth (buf, n);
        guiBeat.store (running ? (float) std::fmod (songMode ? blockHostPpq + n * ppqInc : pos, loopLen) : -1.0f, std::memory_order_relaxed);
    }

    // SINGLE NOTE mode: no generator, the keys you play sound as they are (polyphonic, same supersaw)
    void renderSingle (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi, int n, juce::MidiBuffer& out)
    {
        own.gate = true;
        blockBeat0 = -1.0;   // Single Note: no pump
        if (! wasSingle) { wasSingle = true; releaseVoices (0); sendOffs (out, 0); nHeld = 0; running = false; pendingRegen = false; }
        for (const auto m : midi)
        {
            const int e = juce::jlimit (0, n - 1, m.samplePosition);
            const auto msg = m.getMessage();
            if (msg.isNoteOn())
            {
                noteOff (msg.getNoteNumber(), false, e);   // the same key again: the old note ends, the new one starts
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), false, e);
            }
            else if (msg.isNoteOff()) noteOff (msg.getNoteNumber(), false, e);
            else if (msg.isAllNotesOff() || (msg.isController() && msg.getControllerNumber() == 123)) releaseVoices (e);
            else if (msg.isAllSoundOff() || (msg.isController() && msg.getControllerNumber() == 120)) { for (auto& v : voices) v.active = false; rev.reset(); }
        }
        midi.clear();
        renderSynth (buf, n);
        guiBeat.store (-1.0f, std::memory_order_relaxed);
    }

    std::atomic<float> guiBeat { -1.0f };   // where the melody is (beats in the loop), -1 = silent
    std::atomic<double> hostBpm { 150.0 };

private:
    struct Voice { bool active = false, gate = false, chord = false; int note = 0; float vel = 0, env = 0, fenv = 0; int stage = 0; int startAt = -1, stopAt = -1;
                   float ph[5] {}; SVF fl, fr; };

    void regenerate (const GenParams& gp, juce::MidiBuffer& out, int at)
    {
        releaseVoices (at); sendOffs (out, at);
        numNotes = generate (gp, notes, &edits); lastKey = keyOf (gp); pendingRegen = false;
        justStarted = true;   // the new melody's notes that are already "sounding" at this point come in
    }

    // schedule [a, b); a pending change is applied exactly on the next bar line inside the range
    void scheduleWithRegen (int a, int b, juce::MidiBuffer& out)
    {
        if (pendingRegen && running && b > a)
        {
            const double start = songMode ? blockHostPpq + a * ppqInc : pos;
            double toBar = 4.0 - std::fmod (start, 4.0); if (toBar >= 4.0 - 1.0e-9) toBar = 0.0;
            const int k = (int) std::ceil (toBar / ppqInc);
            if (a + k < b)
            {
                schedule (a, a + k, out);
                regenerate (genParams(), out, a + k);
                schedule (a + k, b, out);
                return;
            }
        }
        schedule (a, b, out);
    }

    void noteOn (int p, float v, bool chord, int at)
    {
        Voice* best = nullptr;
        for (auto& vo : voices) if (! vo.active) { best = &vo; break; }
        if (! best)   // steal the quietest voice that is already sounding (never one that starts in this block)
            for (auto& vo : voices) if (vo.startAt < 0 && (best == nullptr || vo.env < best->env)) best = &vo;
        if (! best) best = &voices[0];
        *best = Voice{}; best->active = true; best->note = p; best->vel = v; best->chord = chord; best->startAt = at;
        own.trigger (at, p, v);   // OWN SOUND: the same note on your own sample
        for (int k = 0; k < 5; ++k) best->ph[k] = (float) ((p * 7 + k * 13) % 17) / 17.0f;
    }
    void releaseVoices (int at)
    {
        own.release (at, -1);
        for (auto& v : voices)
        {
            if (! v.active) continue;
            if (v.startAt >= at) { v.active = false; continue; }          // not started yet in this block
            if (v.stopAt < 0 || v.stopAt > at) v.stopAt = at;
        }
    }
    void sendOffs (juce::MidiBuffer& out, int at)
    {
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 128; ++i)
            if (outOn[c][i]) { out.addEvent (juce::MidiMessage::noteOff (c + 1, i), at); outOn[c][i] = false; }
    }
    void removeHeld (int note) { int w = 0; for (int k = 0; k < nHeld; ++k) if (held[k] != note) held[w++] = held[k]; nHeld = w; }

    // schedule generated notes for samples [a, b) of this block
    void schedule (int a, int b, juce::MidiBuffer& out)
    {
        if (! running || b <= a) return;
        const double start = songMode ? blockHostPpq + a * ppqInc : pos;
        const double len = (b - a) * ppqInc;
        pos += len;
        double s0 = std::fmod (start, loopLen); if (s0 < 0) s0 += loopLen;
        const double s1 = s0 + len;
        const int midiMode = (int) get (midioutI), playMode = (int) get (playI);
        const float chordsL = get (chordsI);
        const bool first = justStarted; justStarted = false;
        for (int i = 0; i < numNotes; ++i)
        {
            const auto& nt = notes[i];
            const int p = juce::jlimit (0, 127, nt.pitch + transpose);
            const bool toMidi = midiMode == 0 ? ! nt.chord : midiMode == 1;
            const int ch = nt.chord ? 1 : 0;
            for (double base : { 0.0, loopLen })
            {
                const double st = nt.start + base, en = nt.start + nt.len + base;
                const bool startsHere = st >= s0 && st < s1;
                const bool resume = first && st < s0 && en > s0;   // pick up notes that should already be sounding
                if (startsHere || resume)
                {
                    const int at = a + (resume ? 0 : juce::jlimit (0, b - a - 1, (int) ((st - s0) / ppqInc)));
                    const bool audible = nt.chord ? playMode != 1 : playMode != 2;
                    if (audible) noteOn (p, nt.chord ? nt.vel * chordsL * 1.5f : nt.vel, nt.chord, at);
                    if (toMidi) { if (outOn[ch][p]) out.addEvent (juce::MidiMessage::noteOff (ch + 1, p), at);
                                  out.addEvent (juce::MidiMessage::noteOn (ch + 1, p, nt.vel), at); outOn[ch][p] = true; }
                }
                if (en >= s0 && en < s1)
                {
                    const int at = a + juce::jlimit (0, b - a - 1, (int) ((en - s0) / ppqInc));
                    noteOff (p, nt.chord, at);
                    if (toMidi && outOn[ch][p]) { out.addEvent (juce::MidiMessage::noteOff (ch + 1, p), at); outOn[ch][p] = false; }
                }
            }
        }
    }

    void noteOff (int p, bool chord, int at)
    { own.release (at, p); for (auto& vo : voices) if (vo.active && vo.note == p && vo.chord == chord && vo.stopAt < 0 && (vo.gate || vo.startAt >= 0)) { vo.stopAt = at; break; } }

    void renderSynth (juce::AudioBuffer<float>& buf, int n)
    {
        const float detCents = get (detuneI), width = get (widthI), cutoff = get (cutoffI), reso = get (resoI), fenvA = get (fenvI);
        const float aT = juce::jmax (0.0005f, get (attackI) * 0.001f), dT = get (decayI) * 0.001f, sus = get (sustainI), rT = get (releaseI) * 0.001f;
        const float aInc = 1.0f / (aT * sr), dC = std::exp (-1.0f / (dT * sr * 0.3f)), rC = std::exp (-1.0f / (rT * sr * 0.3f)), fC = std::exp (-1.0f / (0.25f * sr));
        static const float spread[5] = { 0, -1, 1, -0.45f, 0.45f }, pan[5] = { 0, -1, 1, -0.5f, 0.5f };
        float ratio[5]; for (int k = 0; k < 5; ++k) ratio[k] = centsRatio (spread[k] * detCents);   // real cents (was 1 + cents/1200)
        auto* L = buf.getWritePointer (0); auto* R = buf.getWritePointer (1);
        for (auto& v : voices)
        {
            if (! v.active) continue;
            const float f0 = noteHz ((float) v.note);
            float dts[5]; for (int k = 0; k < 5; ++k) dts[k] = f0 * ratio[k] / sr;
            const float g = v.vel * (v.chord ? 0.12f : 0.22f);
            for (int s = 0; s < n; ++s)
            {
                if (s == v.startAt) { v.gate = true; v.stage = 0; v.startAt = -1; }
                if (s == v.stopAt) { v.gate = false; v.stage = 3; v.stopAt = -1; }
                if (v.startAt >= 0) continue;
                if (v.stage == 0) { v.env += aInc; if (v.env >= 1) { v.env = 1; v.stage = 1; } }
                else if (v.stage == 1) { v.env = sus + dC * (v.env - sus); }
                else if (v.stage == 3) { v.env *= rC; if (v.env < 1e-4f) { v.active = false; break; } }
                v.fenv = v.stage == 0 ? 1.0f : v.fenv * fC;
                float l = 0, r = 0;
                for (int k = 0; k < 5; ++k)
                {
                    v.ph[k] += dts[k]; if (v.ph[k] >= 1) v.ph[k] -= 1;
                    const float sw = 2 * v.ph[k] - 1 - polyBlep (v.ph[k], dts[k]);
                    const float pl = 0.5f - 0.5f * pan[k] * width; l += sw * pl; r += sw * (1 - pl);
                }
                if ((s & 15) == 0) { const float fc = cutoff * std::pow (2.0f, fenvA * v.fenv) * (v.chord ? 0.5f : 1.0f); v.fl.set (fc, reso, sr); v.fr.set (fc, reso, sr); }
                const float e = v.env * g;
                L[s] += v.fl.lp (l) * e; R[s] += v.fr.lp (r) * e;
            }
        }
        own.render (buf);   // OWN SOUND (Replace clears the synth first), then it shares the reverb and the limiter
        if (const float pumpAmt = get (pumpI); pumpAmt > 0.001f && blockBeat0 >= 0.0)   // PUMP: duck on every beat, back up in ~half a beat
            for (int s = 0; s < n; ++s)
            {
                const double beat = blockBeat0 + s * ppqInc;
                const float gain = 1.0f - pumpAmt * std::exp (-(float) (beat - std::floor (beat)) * 7.0f);
                L[s] *= gain; R[s] *= gain;
            }
        juce::Reverb::Parameters rp; rp.roomSize = get (rsizeI); rp.damping = 0.5f; rp.width = 1; rp.wetLevel = get (reverbI) * 0.5f; rp.dryLevel = 1.0f - get (reverbI) * 0.35f;
        rev.setParameters (rp); rev.processStereo (L, R, n);
        const float outG = juce::Decibels::decibelsToGain (get (outI)); lim.set (sr, -0.3f);
        for (int s = 0; s < n; ++s) { const float og = outSm.next (outG); float l = L[s] * og, r = R[s] * og; lim.process (l, r); L[s] = l; R[s] = r; }
    }

    float sr = 44100; Voice voices[16]; juce::Reverb rev; Limiter lim; Smooth outSm;
    Note notes[maxNotes]; int numNotes = 0; GenKey lastKey {};
    bool outOn[2][128] {}; bool wasHostPlaying = false, running = false, wasSingle = false, songMode = false, justStarted = false, pendingRegen = false;
    int held[16] {}; int nHeld = 0, transpose = 0;
    double pos = 0, loopLen = 32, ppqInc = 0.001, blockHostPpq = 0, blockBeat0 = -1.0;
    juce::MidiBuffer outMidi;
    std::array<std::atomic<int>, 16> locks;
};

// ---------------------------------------------------------------- editor
class TuneEditor : public Editor
{
public:
    explicit TuneEditor (TuneProcessor& p)
      : Editor (p, BOY_NAME, "melody  |  hold a note", 960, 660), tp (p)
    {
        notes.resize (maxNotes);
        addButton ("NEW MELODY", [this] { tp.setParam ("variation", 0.0f); tp.setParam ("seed", (float) juce::Random::getSystemRandom().nextInt ({ 1, 10000 })); },
                   [] { return juce::String ("a new seed: a whole new melody (locked bars and bars you edited stay)"); });
        addButton ("VARIATION", [this] { tp.setParam ("variation", (float) juce::Random::getSystemRandom().nextInt ({ 1, 100 })); },
                   [] { return juce::String ("the same melody, a few notes changed"); });
        addButton ("SAVE MIDI", [this] { exportMidi(); }, [] { return juce::String ("save the melody (with your edits) as a .mid file"); });
        addButton ("DRAG MIDI", {}, [] { return juce::String ("hold and drag onto FL's playlist or piano roll"); }, true);
        addButton ("DRAW", [this] { drawMode = ! drawMode; status (drawMode ? "DRAW on: drag over the roll to paint notes  |  Shift = erase  |  Ctrl = any semitone" : "DRAW off: drag the notes instead"); },
                   [] { return juce::String ("paint a melody: drag over the roll (notes snap to the scale; Shift = erase, Ctrl = any semitone)"); });
        addButton ("LOAD SAMPLE", [this] { sampleClick(); },
                   [this] { return tp.own.hasSound() ? "OWN SOUND: " + tp.own.name() + "  |  click: another sample / remove  |  level + mode on ADVANCED"
                                                     : juce::String ("OWN SOUND: your own sample plays the notes (or drop a WAV on the plugin)"); });
        addButton ("SOUNDS", [this] { soundsMenu(); }, [this] { return "SOUND BANK: " + tp.soundName() + "  |  click: pick a synth sound or a random one"; });
        addButton ("NEW SOUND", [this] { tp.randomSound(); status ("NEW SOUND: a random synth sound (the melody stays)  |  undo with the arrow"); },
                   [] { return juce::String ("a random synth sound in musical ranges (frozen controls stay)"); });
        addButton ("UNDO EDIT", [this] { undoEdit(); }, [this] { return tp.canUndoEdit() ? juce::String ("take back your last note edit") : juce::String ("nothing to undo"); });
        addButton ("RESET EDITS", [this] { tp.pushEditUndo(); tp.edits.clearAll(); tp.saveEdits(); status ("your note edits are gone: the generated melody is back  |  UNDO EDIT brings them back"); },
                   [this] { return tp.edits.any() ? juce::String ("throw away all notes you moved / added / deleted") : juce::String ("no edits yet: drag the notes in the roll"); });
        refreshRoll();   // the roll is filled right away (not only on the first timer tick)
        init();
    }

protected:
    int heroHeight() const override { return 190; }
    int buttonColumns() const override { return 2; }
    bool wantsFiles() const override { return true; }
    void filesDroppedOnPlugin (const juce::File& f) override { loadSample (f); }
    bool animating() const override { return tp.guiBeat.load() >= 0.0f || wasPlaying; }
    void tick() override
    {
        wasPlaying = tp.guiBeat.load() >= 0.0f;
        const auto t = tp.own.hasSound() ? "SAMPLE: " + tp.own.name() : juce::String ("LOAD SAMPLE");
        if (buttons[5].text != t) { buttons[5].text = t; repaint(); }
        const auto d = drawMode ? juce::String ("DRAW: ON") : juce::String ("DRAW");
        if (buttons[4].text != d) { buttons[4].text = d; repaint(); }
        if (refreshRoll()) repaint();
    }
    bool refreshRoll()
    {
        const auto gp = tp.genParams();
        const auto k = tp.keyOf (gp);
        if (k == shownKey) return false;
        shownKey = k; count = generate (gp, notes.data(), &tp.edits); bars = gp.bars;
        return true;
    }

    // ---- the roll: geometry shared by painting and editing
    juce::Rectangle<float> header (juce::Rectangle<float> a) const { return a.reduced (10.0f, 6.0f).removeFromTop (22.0f); }
    juce::Rectangle<float> rollArea (juce::Rectangle<float> a) const { auto r = a.reduced (10.0f, 6.0f); r.removeFromTop (28.0f); return r; }
    struct Range { int lo = 48, hi = 72; };
    Range pitchRange() const
    {
        if (dragging || stroking) return dragRange;   // frozen while you drag / paint, so the picture does not jump
        Range g;
        if (count > 0)
        {
            int lo = 127, hi = 0;
            for (int i = 0; i < count; ++i) { lo = juce::jmin (lo, notes[(size_t) i].pitch); hi = juce::jmax (hi, notes[(size_t) i].pitch); }
            g.lo = lo - 2; g.hi = hi + 2;
        }
        if (g.hi - g.lo < 12) g.hi = g.lo + 12;
        return g;
    }
    static float rowHeight (juce::Rectangle<float> r, Range g) { return r.getHeight() / (float) (g.hi - g.lo + 1); }
    juce::Rectangle<float> noteRect (const Note& nt, juce::Rectangle<float> r, Range g) const
    {
        const double end = bars * 4.0;
        const float rh = rowHeight (r, g);
        return { r.getX() + r.getWidth() * (float) (nt.start / end), r.getBottom() - (float) (nt.pitch - g.lo + 1) * rh,
                 juce::jmax (2.0f, r.getWidth() * (float) (nt.len / end)) - 1.0f, juce::jmax (2.0f, rh - 1.0f) };
    }
    int pitchAt (float y, juce::Rectangle<float> r, Range g) const { return g.lo + (int) std::floor ((r.getBottom() - y) / rowHeight (r, g)); }
    int hitNote (juce::Point<float> p) const
    {
        const auto r = rollArea (heroArea);
        const auto g = pitchRange();
        for (int i = count - 1; i >= 0; --i)
            if (! notes[(size_t) i].chord && noteRect (notes[(size_t) i], r, g).expanded (1.0f, 3.0f).contains (p)) return i;
        return -1;
    }
    // the pitch you drop a note on: the nearest note of the song's scale (Shift / Ctrl = any semitone)
    int snapPitch (int p, bool chromatic) const
    {
        if (chromatic) return juce::jlimit (0, 127, p);
        const int key = juce::roundToInt (real ("key")), scale = juce::jlimit (0, numScales - 1, juce::roundToInt (real ("scale")));
        auto in = [&] (int q) { const int pc = ((q - key) % 12 + 12) % 12; for (int k = 0; k < 7; ++k) if (scales[scale][k] == pc) return true; return false; };
        for (int d = 0; d <= 6; ++d)
        {
            if (in (p + d)) return juce::jlimit (0, 127, p + d);
            if (in (p - d)) return juce::jlimit (0, 127, p - d);
        }
        return juce::jlimit (0, 127, p);
    }

    // a bar becomes yours: its current notes (generated or already edited) are copied into the edit store
    int captureBar (int bar, std::vector<EditNote>& cur) const
    {
        cur.clear();
        if (tp.edits.edited (bar)) { EditNote en[maxEditPerBar]; const int n = tp.edits.get (bar, en); cur.assign (en, en + n); }
        else
            for (int i = 0; i < count; ++i)
            {
                const auto& nt = notes[(size_t) i];
                if (nt.chord || (int) (nt.start / 4.0) != bar) continue;
                const int step = juce::jlimit (0, 15, juce::roundToInt ((nt.start - bar * 4.0) / 0.25));
                cur.push_back ({ step, juce::jlimit (1, 16 - step, juce::roundToInt (nt.len / 0.25)), nt.pitch, juce::jlimit (1, 127, juce::roundToInt (nt.vel * 127.0f)) });
            }
        if ((int) cur.size() > maxEditPerBar) cur.resize (maxEditPerBar);
        return (int) cur.size();
    }
    void storeBar (int bar, const std::vector<EditNote>& cur) { tp.edits.set (bar, cur.data(), (int) cur.size()); refreshRoll(); repaint(); }
    void undoEdit()
    {
        if (tp.undoEdit()) { refreshRoll(); status ("edit undone"); }
        else status ("nothing to undo");
        repaint();
    }

    void paintHero (juce::Graphics& g, juce::Rectangle<float> area) override
    {
        const auto hd = header (area), r = rollArea (area);
        const double end = bars * 4.0;
        const float bw = r.getWidth() / (float) bars;
        // bar numbers: click = lock / unlock that bar (it keeps its notes through NEW MELODY / VARIATION); * = you edited it
        for (int b = 0; b < bars; ++b)
        {
            const auto cell = juce::Rectangle<float> (hd.getX() + bw * (float) b, hd.getY(), bw, hd.getHeight()).reduced (1.0f, 0.0f);
            const bool locked = tp.lockOf (b) > 0, mine = tp.edits.edited (b);
            g.setColour (locked ? col::ink : mine ? col::accent : col::well);
            g.fillRect (cell);
            g.setColour (col::ink);
            g.drawRect (cell, locked || mine ? 2.0f : 1.0f);
            g.setColour (locked ? col::inkInverse : col::ink);
            g.setFont (monoFont (20.0f));
            g.drawText ((locked ? juce::String ("#") : juce::String()) + (mine ? juce::String ("*") : juce::String()) + juce::String (b + 1), cell, juce::Justification::centred, false);
            if (locked) { g.setColour (col::accent.withAlpha (0.18f)); g.fillRect (r.getX() + bw * (float) b, r.getY(), bw, r.getHeight()); }
        }
        if (count > 0)
        {
            const auto rg = pitchRange();
            g.setColour (col::line.withAlpha (0.6f));
            for (int b = 0; b <= bars; ++b) g.drawVerticalLine ((int) (r.getX() + bw * (float) b), r.getY(), r.getBottom());
            for (int i = 0; i < count; ++i)
            {
                const auto& nt = notes[(size_t) i];
                const auto nr = noteRect (nt, r, rg);
                g.setColour (nt.chord ? col::accent : col::ink);   // chords accent, melody ink
                g.fillRect (nr);
            }
            if (dragging)   // the note you hold gets a frame
            {
                const int step = drag.cur[(size_t) drag.idx].step;
                Note nt { drag.bar * 4.0 + step * 0.25, drag.cur[(size_t) drag.idx].len * 0.25, drag.cur[(size_t) drag.idx].pitch, 1.0f, false };
                g.setColour (col::inkInverse);
                g.drawRect (noteRect (nt, r, rg).expanded (1.0f), 1.0f);
            }
        }
        if (drawMode)
            heroLabel (g, monoFont (22.0f), r, "DRAW  |  Shift = erase", col::ink, juce::Justification::topRight);
        if (real ("mode") > 0.5f)
            heroLabel (g, monoFont (22.0f), r, "SINGLE NOTE  |  the melody is paused", col::ink, juce::Justification::bottomRight);
        const float beat = tp.guiBeat.load();
        if (beat >= 0.0f)
        {
            const float x = r.getX() + r.getWidth() * beat / (float) end;
            g.setColour (col::ink);
            g.fillRect (x - 1.5f, hd.getY(), 4.0f, r.getBottom() - hd.getY());
        }
    }

    // ---- editing: drag a note = move it (16th grid, scale notes), drag its right edge = length; DRAW mode = paint notes
    bool heroDown (juce::Point<float> p, const juce::MouseEvent& e) override
    {
        const auto hd = header (heroArea);
        if (hd.expanded (0, 3).contains (p))
        {
            const int b = juce::jlimit (0, bars - 1, (int) ((p.x - hd.getX()) / (hd.getWidth() / (float) bars)));
            tp.toggleLock (b);
            status (tp.lockOf (b) > 0 ? "bar " + juce::String (b + 1) + " locked: it keeps these notes through NEW MELODY / VARIATION"
                                      : "bar " + juce::String (b + 1) + " unlocked");
            repaint();
            return false;
        }
        const auto r = rollArea (heroArea);
        if (drawMode)
        {
            if (! r.contains (p) || count == 0) return false;
            tp.pushEditUndo();
            stroking = true; strokeErase = e.mods.isShiftDown(); strokeLastG = -1;
            strokeBars.clear(); strokeDrawn.clear();
            dragRange = pitchRange();
            applyStroke (p, e);
            return true;
        }
        const int hit = hitNote (p);
        if (hit < 0) return false;
        const auto rg = pitchRange();
        const auto& nt = notes[(size_t) hit];
        drag = {};
        drag.bar = juce::jlimit (0, bars - 1, (int) (nt.start / 4.0));
        captureBar (drag.bar, drag.cur);
        const int step = juce::jlimit (0, 15, juce::roundToInt ((nt.start - drag.bar * 4.0) / 0.25));
        drag.idx = -1;
        for (int i = 0; i < (int) drag.cur.size(); ++i) if (drag.cur[(size_t) i].step == step && drag.cur[(size_t) i].pitch == nt.pitch) { drag.idx = i; break; }
        if (drag.idx < 0) return false;
        const auto nr = noteRect (nt, r, rg);
        drag.resize = p.x > nr.getRight() - juce::jmin (8.0f, nr.getWidth() * 0.4f);
        drag.grabDx = p.x - nr.getX();
        drag.grabRows = pitchAt (p.y, r, rg) - nt.pitch;
        drag.origLen = drag.cur[(size_t) drag.idx].len;
        dragRange = rg;
        dragging = true;
        tp.pushEditUndo();
        storeBar (drag.bar, drag.cur);   // the bar is yours from now on
        return true;
    }
    void heroDrag (juce::Point<float> p, const juce::MouseEvent& e) override
    {
        if (stroking) { applyStroke (p, e); return; }
        if (! dragging || drag.idx < 0) return;
        const auto r = rollArea (heroArea);
        const float stepW = r.getWidth() / (float) (bars * 16);
        auto& n = drag.cur[(size_t) drag.idx];
        if (drag.resize)
        {
            const int endStep = juce::roundToInt ((p.x - r.getX()) / stepW) - drag.bar * 16;
            n.len = juce::jlimit (1, 16 - n.step, endStep - n.step);
            storeBar (drag.bar, drag.cur);
            return;
        }
        const int g = juce::jlimit (0, bars * 16 - 1, juce::roundToInt ((p.x - drag.grabDx - r.getX()) / stepW));
        const int nb = g / 16, ns = g % 16;
        EditNote en = n;
        en.step = ns; en.len = juce::jmin (drag.origLen, 16 - ns);
        en.pitch = snapPitch (pitchAt (p.y, r, dragRange) - drag.grabRows, e.mods.isShiftDown() || e.mods.isCtrlDown());
        if (nb != drag.bar)   // dragged into another bar: it moves over (that bar becomes yours too)
        {
            std::vector<EditNote> other;
            captureBar (nb, other);
            if ((int) other.size() >= maxEditPerBar) return;
            drag.cur.erase (drag.cur.begin() + drag.idx);
            storeBar (drag.bar, drag.cur);
            drag.cur = std::move (other);
            drag.bar = nb;
            drag.cur.push_back (en);
            drag.idx = (int) drag.cur.size() - 1;
        }
        else n = en;
        storeBar (drag.bar, drag.cur);
    }
    void heroUp() override
    {
        if (stroking)
        {
            for (auto& kv : strokeBars) { mergeRuns (kv.second, strokeDrawn[kv.first]); storeBar (kv.first, kv.second); }
            stroking = false;
            tp.saveEdits();
            status (strokeErase ? "notes erased  |  UNDO EDIT takes it back" : "melody painted  |  drag the notes to fine-tune, UNDO EDIT takes it back");
            refreshRoll(); repaint();
            return;
        }
        if (! dragging) return;
        dragging = false;
        tp.saveEdits();
        status ("note edited  |  bar " + juce::String (drag.bar + 1) + " is yours now (*)  |  RESET EDITS gives the generated melody back");
        refreshRoll();
        repaint();
    }
    void heroDoubleClick (juce::Point<float> p, const juce::MouseEvent& e) override
    {
        const auto r = rollArea (heroArea);
        if (drawMode || ! r.contains (p) || hitNote (p) >= 0 || count == 0) return;
        const float stepW = r.getWidth() / (float) (bars * 16);
        const int g = juce::jlimit (0, bars * 16 - 1, (int) std::floor ((p.x - r.getX()) / stepW));
        const int bar = g / 16, step = g % 16;
        std::vector<EditNote> cur;
        captureBar (bar, cur);
        if ((int) cur.size() >= maxEditPerBar) { status ("this bar is full (32 notes)"); return; }
        tp.pushEditUndo();
        cur.push_back ({ step, juce::jmin (2, 16 - step), snapPitch (pitchAt (p.y, r, pitchRange()), e.mods.isShiftDown() || e.mods.isCtrlDown()), 100 });
        storeBar (bar, cur);
        tp.saveEdits();
        status ("note added in bar " + juce::String (bar + 1) + "  |  drag it, right-click it to delete");
    }
    void heroContext (juce::Point<float> p, const juce::MouseEvent&) override
    {
        const int hit = hitNote (p);
        const auto r = rollArea (heroArea);
        const int bar = juce::jlimit (0, bars - 1, (int) ((p.x - r.getX()) / (r.getWidth() / (float) bars)));
        juce::PopupMenu m;
        m.addSectionHeader ("MELODY EDIT  |  bar " + juce::String (bar + 1));
        if (hit >= 0 && ! notes[(size_t) hit].chord) m.addItem (1, "Delete this note");
        m.addItem (2, "Reset bar " + juce::String (bar + 1) + " to the generated melody", tp.edits.edited (bar));
        m.addItem (3, "Reset all edits", tp.edits.any());
        m.addSeparator();
        m.addItem (4, "Copy bar " + juce::String (bar + 1) + " to the next bar", bar + 1 < bars);
        m.addItem (5, "Copy bar " + juce::String (bar + 1) + " to ALL bars");
        m.addItem (6, "Bar " + juce::String (bar + 1) + ": up an octave");
        m.addItem (7, "Bar " + juce::String (bar + 1) + ": down an octave");
        m.addSeparator();
        m.addItem (8, "Undo the last edit", tp.canUndoEdit());
        const int noteBar = hit >= 0 ? juce::jlimit (0, bars - 1, (int) (notes[(size_t) juce::jmax (0, hit)].start / 4.0)) : bar;
        const Note nt = hit >= 0 ? notes[(size_t) hit] : Note {};
        juce::Component::SafePointer<TuneEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, nt, noteBar, bar] (int id)
        {
            if (safe == nullptr || id <= 0) return;
            auto& t = *safe;
            if (id == 8) { t.undoEdit(); return; }
            t.tp.pushEditUndo();
            if (id == 1)
            {
                std::vector<EditNote> cur; t.captureBar (noteBar, cur);
                const int step = juce::jlimit (0, 15, juce::roundToInt ((nt.start - noteBar * 4.0) / 0.25));
                for (size_t i = 0; i < cur.size(); ++i) if (cur[i].step == step && cur[i].pitch == nt.pitch) { cur.erase (cur.begin() + (long) i); break; }
                t.storeBar (noteBar, cur); t.tp.saveEdits(); t.status ("note deleted");
            }
            else if (id == 2) { t.tp.edits.clearBar (bar); t.tp.saveEdits(); t.refreshRoll(); t.status ("bar " + juce::String (bar + 1) + " is generated again"); }
            else if (id == 3) { t.tp.edits.clearAll(); t.tp.saveEdits(); t.refreshRoll(); t.status ("all edits gone"); }
            else if (id == 4 || id == 5)
            {
                std::vector<EditNote> cur; t.captureBar (bar, cur);
                int copies = 0;
                for (int b = 0; b < t.bars; ++b)
                    if (b != bar && (id == 5 || b == bar + 1)) { t.tp.edits.set (b, cur.data(), (int) cur.size()); ++copies; }
                t.tp.saveEdits(); t.refreshRoll(); t.status ("bar " + juce::String (bar + 1) + " copied to " + juce::String (copies) + (copies == 1 ? " bar" : " bars"));
            }
            else if (id == 6 || id == 7)
            {
                std::vector<EditNote> cur; t.captureBar (bar, cur);
                for (auto& en : cur) en.pitch = juce::jlimit (0, 127, en.pitch + (id == 6 ? 12 : -12));
                t.storeBar (bar, cur); t.tp.saveEdits(); t.status ("bar " + juce::String (bar + 1) + (id == 6 ? " up an octave" : " down an octave"));
            }
            t.repaint();
        });
    }
    juce::String heroHint (juce::Point<float> p) override
    {
        if (real ("mode") > 0.5f) return "Single Note: the keys you play sound as they are (Mode: Melody brings it back)";
        if (header (heroArea).expanded (0, 3).contains (p)) return "click a bar number: lock / unlock that bar  |  * = you edited that bar";
        if (drawMode) return "DRAW: drag over the roll to paint notes  |  Shift = erase  |  Ctrl = any semitone  |  click DRAW again to stop";
        if (hitNote (p) >= 0) return "drag = move (Shift: any semitone)  |  drag the right edge = length  |  right-click = delete / copy bar / octave";
        return "double-click = add a note  |  DRAW = paint a melody  |  right-click = bar tools  |  melody (dark) and chords (accent)";
    }
    void dragOut (Button&) override
    {
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (juce::String (BOY_NAME) + "_melody.mid");
        if (tp.writeMidiFile (f)) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }

private:
    struct Drag { int bar = 0, idx = -1, grabRows = 0, origLen = 1; float grabDx = 0; bool resize = false; std::vector<EditNote> cur; };

    // DRAW: every 16th step the mouse passes gets a note (also the steps a fast mouse jumps over)
    void applyStroke (juce::Point<float> p, const juce::MouseEvent& e)
    {
        const auto r = rollArea (heroArea);
        const float stepW = r.getWidth() / (float) (bars * 16);
        const int g = juce::jlimit (0, bars * 16 - 1, (int) std::floor ((p.x - r.getX()) / stepW));
        const bool chromatic = e.mods.isCtrlDown();
        const int pitch = snapPitch (pitchAt (juce::jlimit (r.getY(), r.getBottom() - 0.5f, p.y), r, dragRange), chromatic);
        const int from = strokeLastG < 0 ? g : strokeLastG, fromP = strokeLastG < 0 ? pitch : strokeLastPitch, dir = g >= from ? 1 : -1;
        for (int k = from;; k += dir)
        {
            const float t = from == g ? 1.0f : (float) (k - from) / (float) (g - from);
            const int pk = snapPitch (juce::roundToInt ((float) fromP + (float) (pitch - fromP) * t), chromatic);
            const int bar = k / 16, step = k % 16;
            auto it = strokeBars.find (bar);
            if (it == strokeBars.end()) { std::vector<EditNote> cur; captureBar (bar, cur); it = strokeBars.emplace (bar, std::move (cur)).first; }
            if (strokeErase) eraseStep (it->second, step);
            else { drawStep (it->second, step, pk); strokeDrawn[bar] |= 1u << step; }
            if (k == g) break;
        }
        for (auto& kv : strokeBars) storeBar (kv.first, kv.second);
        strokeLastG = g; strokeLastPitch = pitch;
    }

    void exportMidi()
    {
        chooser = std::make_unique<juce::FileChooser> ("Save melody", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                                          .getChildFile (juce::String (BOY_NAME) + "_melody.mid"), "*.mid");
        juce::Component::SafePointer<TuneEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [safe] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (safe != nullptr && f != juce::File())
                    safe->status (safe->tp.writeMidiFile (f.withFileExtension ("mid")) ? "saved " + f.withFileExtension ("mid").getFileName() : juce::String ("could not save"));
            });
    }
    void soundsMenu()
    {
        static const auto bank = soundBank();
        juce::PopupMenu m;
        m.addSectionHeader ("SYNTH SOUNDS");
        for (int i = 0; i < (int) bank.size(); ++i) m.addItem (100 + i, bank[(size_t) i].name, true, bank[(size_t) i].name == tp.soundName());
        m.addSeparator();
        m.addItem (1, "NEW SOUND (random)");
        m.addSectionHeader ("OWN SOUND");
        m.addItem (2, tp.own.hasSound() ? "Load another sample..." : "Load your own sample...");
        if (tp.own.hasSound()) m.addItem (3, "Remove the sample (" + tp.own.name() + ")");
        juce::Component::SafePointer<TuneEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            if (r >= 100) { safe->tp.applySound (r - 100); safe->status ("sound: " + safe->tp.soundName() + "  |  undo with the arrow"); }
            else if (r == 1) { safe->tp.randomSound(); safe->status ("NEW SOUND: a random synth sound  |  undo with the arrow"); }
            else if (r == 2) safe->chooseSample();
            else if (r == 3) { safe->tp.own.clear(); safe->status ("sample removed: the synth plays again"); }
            safe->repaint();
        });
    }
    void sampleClick()
    {
        if (! tp.own.hasSound()) { chooseSample(); return; }
        juce::PopupMenu m;
        m.addSectionHeader ("OWN SOUND: " + tp.own.name());
        m.addItem (1, "Load another sample...");
        m.addItem (2, "Remove the sample");
        juce::Component::SafePointer<TuneEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe] (int r)
        {
            if (safe == nullptr) return;
            if (r == 1) safe->chooseSample();
            if (r == 2) { safe->tp.own.clear(); safe->status ("sample removed: the synth plays again"); }
        });
    }
    void chooseSample()
    {
        chooser = std::make_unique<juce::FileChooser> ("Load your own sound", juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                       Sonic::OwnSound::fileTypes());
        juce::Component::SafePointer<TuneEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe] (const juce::FileChooser& fc)
        { if (safe != nullptr && fc.getResult().existsAsFile()) safe->loadSample (fc.getResult()); });
    }
    void loadSample (const juce::File& f)
    {
        const auto note = tp.own.loadFile (f);
        status (tp.own.hasSound() && tp.own.name() == f.getFileNameWithoutExtension()
                    ? "OWN SOUND: " + tp.own.name() + " loaded (C5 = as recorded)" + (note.isEmpty() ? juce::String() : "  |  " + note) + "  |  Sample Level / Mode on ADVANCED"
                    : note);
    }

    TuneProcessor& tp;
    std::vector<Note> notes;
    int count = 0, bars = 8;
    GenKey shownKey {};
    bool wasPlaying = false, dragging = false, drawMode = false, stroking = false, strokeErase = false;
    int strokeLastG = -1, strokeLastPitch = 0;
    std::map<int, std::vector<EditNote>> strokeBars;
    std::map<int, unsigned> strokeDrawn;
    Drag drag;
    Range dragRange;
    std::unique_ptr<juce::FileChooser> chooser;
};

juce::AudioProcessorEditor* TuneProcessor::createEditor() { return new TuneEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TuneProcessor(); }
