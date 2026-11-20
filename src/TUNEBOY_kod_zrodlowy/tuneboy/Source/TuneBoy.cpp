// TUNEBOY by sonicsystems - hardstyle intro / euphoric melody generator with a built-in supersaw lead, BOY series.
// Key, scale, chord progression, bars, rhythm, density, seed -> a melody + chord layer, played while you hold a key
// (C5 = as generated, other keys transpose) or following the song position. NEW MELODY / VARIATION, lock the bars you
// like (click the bar numbers), EXPORT / DRAG MIDI. Changes made while it plays start at the next bar.
// 2.1: drag the notes in the roll (move / length / add / delete: a bar you touch becomes yours), 16 + Custom progressions,
// OWN SOUND (your own sample plays the notes), a bank of synth sounds and a random sound.
// 2.2: DRAW mode (paint a melody with the mouse), edit undo, copy / transpose a bar, 3 more scales, 10 more progressions,
// 4 more rhythms, 12 more sounds, CHORD STYLE (held / offbeat stabs / arp / gate) and PUMP (a built-in sidechain duck).
// RANDOMIZE no longer touches the sound (synth params + reverb): only the melody side; NEW SOUND is the sound dice.
// Phrase engine (param "engine", appended): 4-bar phrases, voice leading, one climax per 8 bars. Classic is the
// previous generator, bit for bit, and is what a project saved before this parameter loads. FRENCHCORE on Phrase
// writes short offbeat stabs instead of a sparse loop.
#include "BoyUI.h"
#include "OwnSound.h"
#include <cstring>
#include <limits>
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

struct GenParams { int key, scale, prog, bars, rhythm, octave, range, seed, variation; float density; std::array<int, 16> locks; std::array<int, 4> cust; int chordStyle; int engine; int french; int variety; int epic; bool epicSeeded = false; };
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
using GenKey = std::array<int, 40>;

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

// Store seeded Epic in a spare bit; the numeric part still decodes states saved by older versions.
constexpr int seededEpicLock = 1 << 21;
int lockNumber (int code) { return code & (seededEpicLock - 1); }
int lockCode (int seed, int variation, bool seeded) { return (1 + seed + 10000 * variation) | (seeded ? seededEpicLock : 0); }

// ---- Phrase engine. Stack only (it runs on the audio thread).
// One 4-bar phrase: call, answer, sequence, cadence with a rising 3-note pickup. Bars 5-8 develop it
// (they are not a copy), bars 9-16 develop it again. Pitches walk toward a bar anchor with voice leading
// and chord tones on accents. FRENCHCORE keeps the same phrase but forces short gates and offbeat weight.
// A locked bar is taken from the phrase its own seed would have written, so it does not follow the new seed.
uint32_t phraseMix (int seed, int variation, int bar, int salt)
{
    return (uint32_t) seed * 2654435761u + (uint32_t) bar * 97531u + (uint32_t) variation * 104729u + (uint32_t) salt * 17u + 11u;
}
int popOff (uint16_t m, int& tot)
{
    int off = 0; tot = 0;
    for (int s = 0; s < 16; ++s) if (m & (1u << s)) { ++tot; if (s % 4 != 0) ++off; }
    return off;
}
uint16_t makePhraseMask (const GenParams& g, int seed, int variation, int bar)
{
    const int rhythm = juce::jlimit (0, numRhythms - 1, g.rhythm);
    const float* t = rhythmTpl[rhythm];
    const int role = bar % 4, half = (bar / 4) & 1, part = (bar / 8) & 1;
    const bool french = g.french != 0;
    Rng r { phraseMix (seed, variation, bar, 1) };
    uint16_t m = 0;
    for (int s = 0; s < 16; ++s)
    {
        const float p = t[s];
        if (p >= 0.85f) { m = (uint16_t) (m | (1u << s)); continue; }   // the template's skeleton. Downbeats are NOT forced.
        if (p <= 0.02f) continue;
        float bias = 0.35f + 0.65f * g.density;
        if (role == 1 && (s % 4) == 3) bias += 0.15f;
        if (role == 2 && (s == 3 || s == 6 || s == 11 || s == 14)) bias += 0.20f;
        if (half == 1 && (s % 8) == 6) bias += 0.12f;
        if (part == 1 && (s % 4) == 1) bias *= 0.45f;                   // second 8 bars: thin the "e"s so it is not a copy
        if (r.next() < p * juce::jmin (1.15f, bias)) m = (uint16_t) (m | (1u << s));
    }
    if (role == 1) m = (uint16_t) (m | (1u << 7));                       // answer: an extra "a"
    if (role == 2) m = (uint16_t) (m | (1u << 3) | (1u << 11));          // sequence: 3-3-2 colour
    if (role == 3)
    {
        m = (uint16_t) (m & ~((1u << 9) | (1u << 10) | (1u << 11) | (1u << 12)));
        m = (uint16_t) (m | (1u << 13) | (1u << 14) | (1u << 15));       // cadence: a rising 3-note pickup
        if ((m & 0x1FFFu) == 0) m = (uint16_t) (m | (1u << ((french || t[0] < 0.5f) ? 2 : 0)));
    }
    if (french)
    {
        int tot = 0; const int off = popOff (m, tot);
        if (tot == 0 || off < 0.4f * (float) juce::jmax (1, tot))
            m = (uint16_t) (m | (1u << 2) | (1u << 6) | (1u << 10) | (1u << (role == 3 ? 15 : 14)));
    }
    if (half == 1) { m = (uint16_t) (m ^ (1u << 14)); if (m == 0) m = (uint16_t) (1u << 14); }   // bars 5-8 are not bars 1-4
    if (part == 1) { m = (uint16_t) (m ^ (1u << 6)); if (m == 0) m = (uint16_t) (1u << 6); }     // bars 9-16 are not bars 1-8
    if (m == 0) m = (uint16_t) (1u << ((french || t[0] < 0.5f) ? 2 : 0));
    return m;
}
int lowestStep (uint16_t m) { for (int s = 0; s < 16; ++s) if (m & (1u << s)) return s; return 0; }
bool maskSeen (const uint16_t* m, int n, int skip, uint16_t v)
{
    for (int i = 0; i < n; ++i) if (i != skip && m[i] == v) return true;
    return false;
}
int uniqueMasks (const uint16_t* m, int n)
{
    int u = 0;
    for (int i = 0; i < n; ++i) if (! maskSeen (m, i, -1, m[i])) ++u;
    return u;
}
float offRatio (const uint16_t* m, int n)
{
    int off = 0, tot = 0;
    for (int b = 0; b < n; ++b) { int t = 0; off += popOff (m[b], t); tot += t; }
    return tot > 0 ? (float) off / (float) tot : 0.0f;
}
void repairPhraseMasks (uint16_t* m, int bars, bool french)
{
    static const int offSteps[] = { 14, 6, 11, 3, 2, 10, 15, 7, 13, 1, 9, 5 };
    const int n = juce::jmin (bars, 8);
    auto halvesDiffer = [&] {
        if (bars < 8) return true;
        for (int i = 0; i < 4; ++i) if (m[i] != m[i + 4]) return true;
        return false;
    };
    if (! halvesDiffer())
    {
        for (int s : offSteps) if ((m[4] | (1u << s)) != m[4]) { m[4] = (uint16_t) (m[4] | (1u << s)); break; }
        if (! halvesDiffer()) { m[4] = (uint16_t) (m[4] ^ (1u << 14)); if (m[4] == 0) m[4] = (uint16_t) (1u << 14); }
    }
    for (int guard = 0; uniqueMasks (m, n) < juce::jmin (4, n) && guard < 40; ++guard)
    {
        bool changed = false;
        for (int b = 1; b < n && ! changed; ++b)
        {
            if (! maskSeen (m, b, -1, m[b])) continue;
            for (int s : offSteps)
            {
                if (m[b] & (1u << s)) continue;
                const uint16_t trial = (uint16_t) (m[b] | (1u << s));
                if (trial != 0 && ! maskSeen (m, n, b, trial)) { m[b] = trial; changed = true; break; }
            }
            if (changed) break;
            for (int s = 0; s < 16; ++s)
            {
                if (s % 4 != 0 || (m[b] & (1u << s)) == 0) continue;
                const uint16_t trial = (uint16_t) (m[b] & ~(1u << s));
                if (trial != 0 && ! maskSeen (m, n, b, trial)) { m[b] = trial; changed = true; break; }
            }
        }
        if (! changed) break;
    }
    if (french)
    {
        for (int guard = 0; offRatio (m, bars) < 0.40f && guard < 48; ++guard)
        {
            bool added = false;
            for (int s : offSteps)
            {
                for (int b = 0; b < bars; ++b)
                {
                    if (m[b] & (1u << s)) continue;
                    if (b == 0 && s < lowestStep (m[0])) continue;   // bar 1's first onset stays put (VARIATION keeps it)
                    m[b] = (uint16_t) (m[b] | (1u << s));
                    added = true; break;
                }
                if (added) break;
            }
            if (added) continue;
            for (int b = 0; b < bars && ! added; ++b)
                for (int s : { 0, 4, 8, 12 })
                {
                    if (b == 0 && s == lowestStep (m[0])) continue;
                    if ((m[b] & (1u << s)) == 0) continue;
                    const uint16_t trial = (uint16_t) (m[b] & ~(1u << s));
                    if (trial == 0) continue;
                    m[b] = trial; added = true; break;
                }
            if (! added) break;
        }
        for (int guard = 0; uniqueMasks (m, n) < juce::jmin (4, n) && guard < 20; ++guard)
        {
            bool changed = false;
            for (int b = 1; b < n && ! changed; ++b)
            {
                if (! maskSeen (m, b, -1, m[b])) continue;
                for (int s : offSteps)
                {
                    if (m[b] & (1u << s)) continue;
                    const uint16_t trial = (uint16_t) (m[b] | (1u << s));
                    if (! maskSeen (m, n, b, trial)) { m[b] = trial; changed = true; break; }
                }
            }
            if (! changed) break;
        }
    }
    if (bars >= 16)
    {
        bool copy = true;
        for (int i = 0; i < 8; ++i) if (m[i] != m[i + 8]) { copy = false; break; }
        if (copy) { m[8] = (uint16_t) (m[8] ^ (1u << 6)); if (m[8] == 0) m[8] = (uint16_t) (1u << 6); }
    }
}

struct PhraseEv { int bar, step, pitch; bool accent, climax, pickup, pad; float gate; }; // gate 0 = the engine's default length; pad = counter line (played with the chords)

int nearestScalePitch (int target, const int* sc, int root)
{
    int best = target, bd = 99;
    for (int oct = -3; oct <= 5; ++oct)
        for (int k = 0; k < 7; ++k)
        {
            const int p = root + oct * 12 + sc[k];
            const int d = std::abs (p - target);
            if (d < bd) { bd = d; best = p; }
        }
    return best;
}
int nearestChordPitch (int target, int chordDeg, const int* sc, int root)
{
    auto deg = [&] (int d) { const int o = d >= 0 ? d / 7 : (d - 6) / 7; return root + o * 12 + sc[d - o * 7]; };
    int best = target, bd = 99;
    for (int ct : { 0, 2, 4 })
        for (int oct : { -7, 0, 7, 14 })
        {
            const int p = deg (chordDeg + ct + oct);
            const int d = std::abs (p - target);
            if (d < bd) { bd = d; best = p; }
        }
    return best;
}
int anchorSemi (int bar, int range, bool french, int seed, int& top, bool& drop)
{
    top = juce::jlimit (7, 12, range + 4);
    drop = french && (seed % 5 == 0);
    const int b = bar % 8;
    auto sca = [&] (int s) { return (s * top) / 12; };
    if (! drop)
    {
        static const int a[8] = { 0, 2, 4, 3, 7, 8, 10, 12 };
        static const int a2[8] = { 2, 5, 7, 4, 8, 10, 7, 12 };   // the second 8 bars climb a different path
        return sca (bar >= 8 ? a2[b] : a[b]);
    }
    static const int d[8] = { 7, 9, 12, 10, 0, 2, 4, 5 };         // high call, then one drop into bar 5
    return sca (d[b]);
}

// masks + pitches for every bar of this seed (repairs included). `ne` events, in time order.
void fillPhrase (const GenParams& g, int seed, int variation, uint16_t* masks, PhraseEv* ev, int& ne)
{
    const int bars = g.bars;
    const bool french = g.french != 0;
    const int rhythm = juce::jlimit (0, numRhythms - 1, g.rhythm);
    const float* tplt = rhythmTpl[rhythm];
    const bool offbeatTpl = tplt[0] < 0.5f && tplt[8] < 0.5f;
    const int* sc = scales[juce::jlimit (0, numScales - 1, g.scale)];
    const int* pr = g.prog == customProg ? g.cust.data() : g.prog > customProg ? extraProgs[juce::jlimit (0, numExtraProgs - 1, g.prog - customProg - 1)]
                                                                                 : progs[juce::jlimit (0, numProgs - 1, g.prog)];
    const int root = 12 * (g.octave + 1) + g.key;
    const int barsPerChord = bars >= 16 ? 2 : 1;
    for (int b = 0; b < bars; ++b)
        masks[b] = makePhraseMask (g, seed, b == 0 ? 0 : variation, b);
    if (variation > 0 && bars > 0)   // bar 1 keeps the un-varied first onset; later steps may move
    {
        const uint16_t plain0 = makePhraseMask (g, seed, 0, 0);
        const int first = lowestStep (plain0);
        uint16_t varied = makePhraseMask (g, seed, variation, 0);
        varied = (uint16_t) ((varied & ~((1u << first) - 1u)) | (1u << first));
        masks[0] = varied;
    }
    repairPhraseMasks (masks, bars, french);
    if (variation > 0 && bars > 1)
    {
        const uint16_t plain = makePhraseMask (g, seed, 0, 1);
        if (masks[1] == plain)
        {
            static const int offSteps[] = { 14, 6, 11, 3, 2, 10, 15, 7, 13, 1, 9, 5 };
            for (int s : offSteps)
            {
                const uint16_t trial = (uint16_t) (masks[1] ^ (1u << s));
                if (trial != 0 && trial != plain) { masks[1] = trial; break; }
            }
        }
    }
    ne = 0;
    int prev = -1;
    int top = 7; bool drop = false;
    anchorSemi (0, g.range, french, seed, top, drop);   // fills top + drop for this seed
    for (int b = 0; b < bars; ++b)
    {
        int steps[16], ns = 0;
        for (int s = 0; s < 16; ++s) if (masks[b] & (1u << s)) steps[ns++] = s;
        if (ns == 0 || ne >= 256) continue;
        int topB = top; bool dropB = drop;
        const int anch = anchorSemi (b, g.range, french, seed, topB, dropB);
        const int end = nearestScalePitch (root + anch, sc, root);
        const int chord = pr[(b / barsPerChord) % 4];
        const int role = b % 4;
        int run = prev < 0 ? root : prev;
        for (int i = 0; i < ns && ne < 256; ++i)
        {
            const int s = steps[i];
            const bool plannedDrop = drop && (b % 8) == 4 && i == 0;
            int desired = end;
            if (i != ns - 1)
                desired = run + juce::roundToInt ((float) (end - run) * ((float) (i + 1) / (float) ns));
            const int limit = plannedDrop ? 12 : 5;
            int delta = juce::jlimit (-limit, limit, desired - run);
            if (delta == 0 && desired != run && ! plannedDrop) delta = desired > run ? 1 : -1;
            int pitch = nearestScalePitch (run + delta, sc, root);
            if (! plannedDrop && std::abs (pitch - run) > 7)
                pitch = nearestScalePitch (run + (pitch > run ? 5 : -5), sc, root);
            const bool accent = tplt[s] >= 0.85f || (offbeatTpl ? (s % 4 == 2) : (s % 4 == 0 && tplt[s] >= 0.5f));
            if (accent)
            {
                const int ct = nearestChordPitch (pitch, chord, sc, root);
                if (std::abs (ct - pitch) <= 4 && std::abs (ct - run) <= (plannedDrop ? 12 : 7)) pitch = ct;
            }
            if (prev >= 0 && pitch == prev && i > 0 && ne > 0 && ev[ne - 1].pitch == pitch)
            {
                const int nudged = nearestScalePitch (pitch + (end >= pitch ? 2 : -2), sc, root);
                if (std::abs (nudged - run) <= 7) pitch = nudged;
            }
            ev[ne++] = { b, s, pitch, accent, false, role == 3 && s >= 13 };
            run = pitch; prev = pitch;
        }
    }
    for (int b = 0; b < bars; ++b)   // cadence pickup: the last three 16ths rise
    {
        if (b % 4 != 3) continue;
        int idx[8], ni = 0;
        for (int i = 0; i < ne && ni < 8; ++i) if (ev[i].bar == b && ev[i].step >= 13) idx[ni++] = i;
        for (int k = 1; k < ni; ++k)
        {
            int want = nearestScalePitch (ev[idx[k - 1]].pitch + 2, sc, root);
            if (want <= ev[idx[k - 1]].pitch) want = nearestScalePitch (ev[idx[k - 1]].pitch + 1, sc, root);
            if (std::abs (want - ev[idx[k - 1]].pitch) > 5) want = ev[idx[k - 1]].pitch + 2;
            ev[idx[k]].pitch = want; ev[idx[k]].pickup = true;
        }
    }
    if (ne > 0)   // one climax per 8 bars, and it is the only top note of that phrase
    {
        for (int section = 0; section < bars; section += 8)
        {
            int ci = -1;
            const int wantRole = drop ? 2 : 7;
            for (int i = 0; i < ne; ++i)
                if (ev[i].bar >= section && ev[i].bar < section + 8 && (ev[i].bar % 8) == wantRole) ci = i;
            if (ci < 0) ci = juce::jmin (ne - 1, section);
            int target = nearestScalePitch (root + top, sc, root);
            if (ci > 0)
            {
                const int pp = ev[ci - 1].pitch;
                if (std::abs (target - pp) > 7 && ! (drop && (ev[ci].bar % 8) == 4))
                    target = nearestScalePitch (pp + (target > pp ? 5 : -5), sc, root);
            }
            ev[ci].pitch = target; ev[ci].climax = true;
            const int peak = ev[ci].pitch;
            for (int i = 0; i < ne; ++i)
            {
                if (i == ci || ev[i].bar < section || ev[i].bar >= section + 8) continue;
                if (ev[i].pitch >= peak)
                {
                    ev[i].pitch = nearestScalePitch (peak - 2, sc, root);
                    if (ev[i].pitch >= peak) ev[i].pitch = peak - 1;
                }
            }
        }
    }
    auto leapFix = [&] {
        int budget[4] = { 2, 2, 2, 2 };
        for (int i = 1; i < ne; ++i)
        {
            const int diff = ev[i].pitch - ev[i - 1].pitch;
            const int grp = juce::jmin (3, ev[i].bar / 4);
            if (std::abs (diff) <= 7) continue;
            if (budget[grp] > 0) { --budget[grp]; continue; }
            const int sign = diff > 0 ? 1 : -1;
            ev[i].pitch = nearestScalePitch (ev[i - 1].pitch + sign * 5, sc, root);
            if (std::abs (ev[i].pitch - ev[i - 1].pitch) > 7) ev[i].pitch = ev[i - 1].pitch + sign * 5;
        }
    };
    leapFix();
    if (ne > 0)
    {
        auto span = [&] (int& mn, int& mx) {
            mn = ev[0].pitch; mx = ev[0].pitch;
            for (int i = 1; i < ne; ++i) { mn = juce::jmin (mn, ev[i].pitch); mx = juce::jmax (mx, ev[i].pitch); }
        };
        int mn, mx; span (mn, mx);
        if (mx - mn < 7)
        {
            int ci = ne - 1;
            for (int i = 0; i < ne; ++i) if (ev[i].climax) ci = i;
            ev[ci].pitch = nearestScalePitch (juce::jmax (ev[ci].pitch, mn + 7), sc, root);
            if (ev[ci].pitch - mn < 7) ev[ci].pitch = mn + 7;
            leapFix();
            span (mn, mx);
            if (mx - mn < 7) ev[ci].pitch = mn + 8;
        }
        span (mn, mx);
        if (mx - mn > 19)
        {
            for (int i = 0; i < ne; ++i) if (ev[i].pitch > mn + 19) ev[i].pitch -= 12;
            leapFix();
        }
    }
}

// Wider Phrase (variety > 0). The original fillPhrase is left alone so a saved project with Variety = Tight
// (or a state from before this parameter) stays note-for-note. Form over 8 bars is A A' B A'', not one 4-bar loop.
// The seed picks the cell, the transform and the contour, so two RANDOMIZE results do not share a shape.
void fillRich (const GenParams& g, int seed, int variation, uint16_t* masks, PhraseEv* ev, int& ne)
{
    const int bars = g.bars;
    const bool french = g.french != 0;
    const int rhythm = juce::jlimit (0, numRhythms - 1, g.rhythm);
    const float* tplt = rhythmTpl[rhythm];
    const int* sc = scales[juce::jlimit (0, numScales - 1, g.scale)];
    const int* pr = g.prog == customProg ? g.cust.data() : g.prog > customProg ? extraProgs[juce::jlimit (0, numExtraProgs - 1, g.prog - customProg - 1)]
                                                                                 : progs[juce::jlimit (0, numProgs - 1, g.prog)];
    const int root = 12 * (g.octave + 1) + g.key;
    const int barsPerChord = bars >= 16 ? 2 : 1;
    const int plan = (seed / 3) % 5;
    static const uint16_t cells[6] = { 0xAAAA, 0x9249, 0x4924, 0xA492, 0x9999, 0x8421 }; // 8ths, 3-3-2, offbeat, dotted, gallop, sparse
    uint16_t motif = 0;
    for (int s = 0; s < 16; ++s) if (tplt[s] >= 0.85f) motif = (uint16_t) (motif | (1u << s));
    motif = (uint16_t) (motif | (cells[(seed * 3 + 1) % 6] & 0x6666u));
    if (french) motif = (uint16_t) (motif | 0x4444u);   // the "ands", so frenchcore stays off the beat
    if (motif == 0) motif = french ? 0x4444u : 0x1111u;
    auto xform = [] (uint16_t m, int kind) -> uint16_t
    {
        if (kind == 1) return (uint16_t) (((m << 1) | (m >> 15)) & 0xFFFFu);          // displaced (sequence)
        if (kind == 2) { uint16_t r = 0; for (int s = 0; s < 16; ++s) if (m & (1u << s)) r = (uint16_t) (r | (1u << (15 - s))); return r; } // retrograde
        if (kind == 3) return (uint16_t) ((m ^ 0x2222u) | (m & 0x1111u));              // flip the offbeats, keep the beats
        if (kind == 4) return (uint16_t) ((m & 0x1111u) | 0x0101u);                    // augmentation: quarters
        return m;
    };
    // A A' B A''  (and a developed second 8 that is not the same transforms)
    static const int formA[5][8] = {
        { 0, 0, 1, 1, 2, 2, 3, 4 },
        { 0, 0, 1, 3, 2, 4, 1, 4 },
        { 0, 1, 0, 3, 2, 2, 4, 4 },
        { 0, 0, 3, 1, 4, 2, 1, 4 },
        { 0, 3, 1, 1, 2, 4, 3, 4 },
    };
    const int* form = formA[plan];
    for (int b = 0; b < bars; ++b)
    {
        int kind = form[b % 8];
        if (b >= 8) kind = (kind + 2 + (seed % 2)) % 5;
        masks[b] = xform (motif, kind);
        if ((b % 8) == 7) masks[b] = (uint16_t) ((masks[b] & ~0x1F00u) | 0xE000u);   // cadence pickup
        if (((seed + b) % 3) == 0)                                                     // grace into the next beat
        {
            if (masks[b] & (1u << 4)) masks[b] = (uint16_t) (masks[b] | (1u << 3));
            if (masks[b] & (1u << 8)) masks[b] = (uint16_t) (masks[b] | (1u << 7));
        }
        if (masks[b] == 0) masks[b] = motif;
    }
    if (variation > 0 && bars > 0)
    {
        const int first = lowestStep (masks[0]);
        masks[0] = (uint16_t) ((masks[0] & ~((1u << first) - 1u)) | (1u << first));
    }
    repairPhraseMasks (masks, bars, french);
    if (variation > 0 && bars > 1) { masks[1] = (uint16_t) (masks[1] ^ (1u << 7)); if (masks[1] == 0) masks[1] = (uint16_t) (1u << 7); }
    static const int contour[5][8] = {
        { 0, 2, 4, 5,  7, 9, 8, 12 },
        { 0, 3, 7, 4,  2, 5, 10, 12 },
        { 5, 7, 4, 2,  0, 3, 8, 12 },
        { 0, 0, 3, 5,  8, 10, 7, 12 },
        { 2, 4, 7, 5,  9, 12, 8, 11 },
    };
    const int* shape = contour[plan];
    const int color = (g.scale == 2 || g.scale == 6) ? 1 : (g.scale == 1 ? 11 : 0); // phrygian leans on b2; harmonic minor cadences via the leading tone
    ne = 0;
    int motifPitch[16] = {}, motifN = 0;
    for (int b = 0; b < bars; ++b)
    {
        int steps[16], ns = 0;
        for (int s = 0; s < 16; ++s) if (masks[b] & (1u << s)) steps[ns++] = s;
        if (ns == 0 || ne >= 240) continue;
        int anch = shape[b % 8];
        if (b >= 8) anch = juce::jlimit (0, 12, anch + ((seed % 2) ? 3 : -2));
        if (color == 1 && anch == 0) anch = 1;
        if (color == 11 && (b % 8) == 6) anch = 11;
        const int section = b % 8;
        const int chord = pr[(b / barsPerChord) % 4];
        const bool offbeatTpl = tplt[0] < 0.5f && tplt[8] < 0.5f;
        for (int i = 0; i < ns && ne < 256; ++i)
        {
            const int s = steps[i];
            int target = root + anch;
            if (b % 8 == 0)
            {
                const float t = ns == 1 ? 0.0f : (float) i / (float) (ns - 1);
                const int arch = (i == 0 || i == ns - 1) ? 0 : (int) std::round (std::sin (t * 3.14159f) * 3.0f);
                target = root + arch + (color == 1 && i == 0 ? 1 : 0);
            }
            else if (motifN > 0)
            {
                const int src = juce::jmin (motifN - 1, i);
                int off = motifPitch[src] - motifPitch[0];
                if (section >= 2 && section <= 3) off += 2 + (plan % 3);             // A' sequence up
                if (section >= 4 && section <= 5 && (plan % 2) == 0) off = -off;    // B inversion
                if (section >= 4 && section <= 5 && (plan % 2) == 1) off = motifPitch[motifN - 1 - src] - motifPitch[0]; // B retrograde
                if (section == 6) off += (i % 2 == 0 ? 0 : 2);                        // neighbor / turn
                if (b >= 8) off += (seed % 2) ? 3 : -2;
                target = motifPitch[0] + off + (anch - shape[0]);
            }
            const bool jump = plan == 2 && b % 8 == 4 && i == 0;
            if (! jump) target = juce::jlimit (root - 2, root + 16, target);
            int pitch = nearestScalePitch (target, sc, root);
            if (ne > 0 && ! jump && std::abs (pitch - ev[ne - 1].pitch) > 5)
                pitch = nearestScalePitch (ev[ne - 1].pitch + (pitch > ev[ne - 1].pitch ? 4 : -4), sc, root);
            if (b % 8 == 0 && i == 0) pitch = nearestScalePitch (root + (color == 1 ? 1 : 0), sc, root);
            const bool accent = tplt[s] >= 0.85f || (offbeatTpl ? (s % 4 == 2) : (s % 4 == 0));
            if (accent)
            {
                const int ct = nearestChordPitch (pitch, chord, sc, root);
                if (std::abs (ct - pitch) <= 4 && (ne == 0 || std::abs (ct - ev[ne - 1].pitch) <= (jump ? 12 : 7))) pitch = ct;
            }
            const bool grace = (s == 3 || s == 7) && ((seed + b) % 3) == 0;
            float gate = grace ? 0.38f : (accent ? 0.92f : (s % 3 == 0 ? 0.78f : 0.58f));
            ev[ne++] = { b, s, pitch, accent, false, (b % 8) == 7 && s >= 13, false, gate };
            if (b % 8 == 0 && motifN < 16) motifPitch[motifN++] = pitch;
        }
    }
    if (ne > 0)
    {
        int ci = ne - 1;
        for (int i = 0; i < ne; ++i) if ((ev[i].bar % 8) == 7) ci = i;
        ev[ci].pitch = nearestScalePitch (root + 12, sc, root);
        ev[ci].climax = true;
        const int peak = ev[ci].pitch;
        for (int i = 0; i < ne; ++i) if (i != ci && ev[i].pitch >= peak) ev[i].pitch = nearestScalePitch (peak - 2, sc, root);
        int budget[4] = { 2, 2, 2, 2 };
        for (int i = 1; i < ne; ++i)
        {
            const int diff = ev[i].pitch - ev[i - 1].pitch, grp = juce::jmin (3, ev[i].bar / 4);
            if (std::abs (diff) <= 7) continue;
            if (budget[grp] > 0) { --budget[grp]; continue; }
            const int sign = diff > 0 ? 1 : -1;
            ev[i].pitch = nearestScalePitch (ev[i - 1].pitch + sign * 5, sc, root);
            if (std::abs (ev[i].pitch - ev[i - 1].pitch) > 7) ev[i].pitch = ev[i - 1].pitch + sign * 5;
        }
        int mn = ev[0].pitch, mx = ev[0].pitch;
        for (int i = 1; i < ne; ++i) { mn = juce::jmin (mn, ev[i].pitch); mx = juce::jmax (mx, ev[i].pitch); }
        if (mx - mn < 7) ev[ci].pitch = mn + 9;
        if (ev[ci].pitch - mn > 19) ev[ci].pitch = mn + 16;
        for (int i = 0; i < ne; ++i) if (ev[i].pitch > mn + 19) ev[i].pitch -= 12;
    }
}

// Epic / symphonic: long soaring line, a 16-bar build → climax → lift, a slower counter-melody under it.
// The chord pad follows (generatePhrase). A Picardy third and a 4th-up lift are applied there on the last section.
void fillEpic (const GenParams& g, int seed, int variation, bool seeded, uint16_t* masks, PhraseEv* ev, int& ne)
{
    const int bars = g.bars;
    const int* sc = scales[juce::jlimit (0, numScales - 1, g.scale)];
    const int* pr = g.prog == customProg ? g.cust.data() : g.prog > customProg ? extraProgs[juce::jlimit (0, numExtraProgs - 1, g.prog - customProg - 1)]
                                                                                 : progs[juce::jlimit (0, numProgs - 1, g.prog)];
    const int root = 12 * (g.octave + 1) + g.key;
    const int barsPerChord = bars >= 16 ? 2 : 1;
    const int liftAt = bars >= 16 ? 12 : (bars >= 8 ? 6 : bars - 1);
    static const uint16_t longCell[4] = { 0x1111u, 0x0141u, 0x1041u, 0x4101u }; // quarters, dotted, held + pickup, half notes
    for (int b = 0; b < bars; ++b)
    {
        const int part = b < bars / 2 ? 0 : (b < liftAt ? 1 : 2);
        masks[b] = longCell[part == 2 ? 3 : (part == 1 ? 2 : (b % 2))];
        if ((b % 4) == 3) masks[b] = (uint16_t) (masks[b] | 0xE000u);             // rising pickup into the next phrase
        if (b == bars - 1) masks[b] = (uint16_t) (0x0001u | 0xE000u);             // climax note, then a cadence
        if (seeded && b > 0 && b < bars - 1)
        {
            Rng rhythm { phraseMix (seed, variation, b, 24) };
            if (rhythm.next() < 0.40f) masks[b] |= (uint16_t) (1u << (rhythm.next() < 0.5f ? 6 : 10));
        }
        else if (! seeded && variation > 0 && b == 1) masks[b] = (uint16_t) (masks[b] | (1u << 6));
        if (masks[b] == 0) masks[b] = 0x0101u;
    }
    // statement, sequence, wide B, lift. Semitones above the (local) tonic.
    static const int arc16[16] = { 0, 2, 4, 3,  7, 8, 10, 7,  12, 10, 14, 12,  7, 10, 12, 0 };
    static const int arc8[8] = { 0, 3, 7, 5,  10, 12, 15, 7 };
    ne = 0;
    int prev = root;
    for (int b = 0; b < bars && ne < 200; ++b)
    {
        const bool lift = b >= liftAt;
        const int localRoot = root + (lift ? 5 : 0);   // key-change lift of a 4th
        int anch = bars >= 16 ? arc16[b] : arc8[b % 8];
        if (bars < 8) anch = (b % 4) * 3;
        if (g.scale == 2 || g.scale == 6) anch += (b % 4 == 0 ? 1 : 0);           // phrygian b2 on the phrase starts
        if (seeded && b > 0 && b < bars - 1)
        {
            Rng contour { phraseMix (seed, 0, b / 2, 25) };
            const int offset = (int) (contour.next() * 5.0f) - 2;
            anch += b % 2 == 0 ? offset : -offset;
        }
        int steps[16], ns = 0;
        for (int s = 0; s < 16; ++s) if (masks[b] & (1u << s)) steps[ns++] = s;
        const int chord = pr[(b / barsPerChord) % 4];
        for (int i = 0; i < ns && ne < 240; ++i)
        {
            const int s = steps[i];
            const bool climax = b == bars - 1 && i == 0;
            int target = localRoot + (climax ? 12 : anch);
            if (! climax && i > 0) target = prev + juce::jlimit (-7, 9, (localRoot + anch) - prev);
            if (seeded && b > 0 && b < bars - 1 && (s % 4) != 0)
            {
                Rng passing { phraseMix (seed, variation, b, 30 + s) };
                target += (int) (passing.next() * 5.0f) - 2;
            }
            int pitch = nearestScalePitch (target, sc, localRoot);
            if (s % 4 == 0)
            {
                const int ct = nearestChordPitch (pitch, chord, sc, localRoot);
                if (std::abs (ct - pitch) <= 5) pitch = ct;
            }
            const float gate = climax ? 0.98f : (s % 4 == 0 ? 0.94f : 0.72f);
            ev[ne++] = { b, s, pitch, s % 4 == 0, climax, (b % 4) == 3 && s >= 13, false, gate };
            prev = pitch;
            // counter-melody: a held 6th below the strong beats, under the lead (pad flag)
            if ((s % 8) == 0 && ne < 250)
            {
                const int cp = nearestScalePitch (pitch - 8, sc, localRoot);
                ev[ne++] = { b, s, cp, false, false, false, true, 0.96f };
            }
        }
    }
}

int generatePhrase (const GenParams& g, Note* out, const EditStore* ed)
{
    const int* sc = scales[juce::jlimit (0, numScales - 1, g.scale)];
    const int* pr = g.prog == customProg ? g.cust.data() : g.prog > customProg ? extraProgs[juce::jlimit (0, numExtraProgs - 1, g.prog - customProg - 1)]
                                                                             : progs[juce::jlimit (0, numProgs - 1, g.prog)];
    const int root = 12 * (g.octave + 1) + g.key;
    auto degToNote = [&] (int d) { const int o = (d >= 0 ? d / 7 : (d - 6) / 7); const int i = d - o * 7; return root + o * 12 + sc[i]; };
    const int bars = g.bars, barsPerChord = bars >= 16 ? 2 : 1;
    const bool french = g.french != 0;
    bool edited[16] = {}, seeded[16] = {};
    int bSeed[16], bVar[16];
    for (int b = 0; b < bars; ++b)
    {
        edited[b] = ed != nullptr && ed->edited (b);
        const int code = g.locks[(size_t) juce::jlimit (0, 15, b)];
        const int number = lockNumber (code);
        bSeed[b] = number > 0 ? (number - 1) % 10000 : g.seed;
        bVar[b] = number > 0 ? (number - 1) / 10000 : g.variation;
        seeded[b] = g.epic > 0 && (number > 0 ? (code & seededEpicLock) != 0 : g.epicSeeded);
    }
    // one phrase per (seed, variation). A locked bar reads the phrase of the seed it was locked with.
    PhraseEv kept[16][24];
    int keptN[16] = {};
    bool filled[16] = {};
    for (int b = 0; b < bars; ++b)
    {
        if (edited[b] || filled[b]) continue;
        uint16_t masks[16] = {};
        PhraseEv ev[256];
        int ne = 0;
        if (g.epic > 0) fillEpic (g, bSeed[b], bVar[b], seeded[b], masks, ev, ne);
        else if (g.variety > 0) fillRich (g, bSeed[b], bVar[b], masks, ev, ne);
        else fillPhrase (g, bSeed[b], bVar[b], masks, ev, ne);
        for (int c = b; c < bars; ++c)
        {
            if (edited[c] || bSeed[c] != bSeed[b] || bVar[c] != bVar[b] || seeded[c] != seeded[b]) continue;
            keptN[c] = 0;
            for (int i = 0; i < ne && keptN[c] < 24; ++i)
                if (ev[i].bar == c) kept[c][keptN[c]++] = ev[i];
            filled[c] = true;
        }
    }
    // lengths from the gap to the next melody note (chords are not part of the gate)
    struct Mel { int bar; double start, len; int pitch; float vel; };
    Mel mel[256];
    int nm = 0;
    for (int b = 0; b < bars; ++b)
    {
        if (edited[b])
        {
            EditNote en[maxEditPerBar];
            const int n = ed->get (b, en);
            for (int i = 0; i < n && nm < 256; ++i)
                mel[nm++] = { b, b * 4.0 + en[i].step * 0.25, (double) juce::jmin (en[i].len, 16 - en[i].step) * 0.25, en[i].pitch, (float) en[i].vel / 127.0f };
            continue;
        }
        Rng rb { phraseMix (bSeed[b], bVar[b], b, 9) };
        for (int i = 0; i < keptN[b] && nm < 256; ++i)
        {
            const auto& e = kept[b][i];
            if (e.pad) continue;   // counter-melody is emitted with the chords
            float vel = e.climax ? 1.0f : (e.accent ? 0.94f : 0.76f);
            if (e.pickup) vel = 0.70f + 0.08f * (float) (e.step - 13);
            vel *= 0.94f + 0.06f * rb.next();
            mel[nm++] = { b, b * 4.0 + e.step * 0.25, e.gate, e.pitch, juce::jlimit (0.15f, 1.0f, vel) };
        }
    }
    for (int i = 0; i < nm; ++i)
    {
        if (edited[mel[i].bar]) continue;
        int nxt = -1;
        for (int k = i + 1; k < nm; ++k) if (! edited[mel[k].bar]) { nxt = k; break; }
        double gap = nxt >= 0 ? mel[nxt].start - mel[i].start : 0.5;
        if (gap < 1.0e-6) gap = 0.25;
        const bool strong = mel[i].vel >= 0.9f;
        const float asked = (float) mel[i].len;   // per-note gate ratio, 0 = default
        double len;
        if (g.epic > 0) len = gap * (asked > 0.2f ? asked : (strong ? 0.97f : 0.9f));
        else if (asked > 0.2f && ! french) len = gap * asked;
        else if (french) len = juce::jmin (gap * (strong ? 0.68 : 0.56), strong ? 0.60 : 0.45);
        else { len = gap * (strong ? 0.90 : 0.80); if (gap >= 0.75) len = juce::jmin (len, 0.48); }
        if (nxt >= 0) len = juce::jmin (len, gap - 0.02);
        mel[i].len = juce::jmax (0.05, len);
    }
    int n = 0;
    const int liftAt = bars >= 16 ? 12 : (bars >= 8 ? 6 : bars - 1);
    auto tone = [&] (int b, int deg, int k, bool finalChord) {
        int p = degToNote (deg + k * 2 - 7);
        if (g.epic > 0 && b >= liftAt) p += 5;                 // the pad lifts with the melody
        if (g.epic > 0 && finalChord && k == 1)
        {
            const int lower = degToNote (deg - 7) + (b >= liftAt ? 5 : 0);
            if (((p - lower) % 12 + 12) % 12 == 3) ++p;       // Picardy / borrowed major third
        }
        return juce::jlimit (0, 127, p);
    };
    auto emitChords = [&] (int b, int deg)
    {
        const double base = b * 4.0;
        const bool finalChord = b >= bars - barsPerChord;
        switch (g.chordStyle)
        {
            case 1: for (int beat = 0; beat < 4; ++beat) for (int k = 0; k < 3; ++k) out[n++] = { base + beat + 0.5, 0.4, tone (b, deg, k, finalChord), 0.55f, true }; break;
            case 2: { static const int order[4] = { 0, 1, 2, 1 }; for (int st = 0; st < 16; ++st) out[n++] = { base + st * 0.25, 0.2, tone (b, deg, order[st % 4], finalChord), 0.55f, true }; break; }
            case 3: for (int e = 0; e < 8; ++e) for (int k = 0; k < 3; ++k) out[n++] = { base + e * 0.5, 0.4, tone (b, deg, k, finalChord), 0.55f, true }; break;
            default: if (b % barsPerChord == 0) for (int k = 0; k < 3; ++k) out[n++] = { base, barsPerChord * 4.0 - 0.05, tone (b, deg, k, finalChord), 0.55f, true };
        }
    };
    for (int b = 0; b < bars && n < maxNotes - 64; ++b)
    {
        const int chordDeg = pr[(b / barsPerChord) % 4];
        for (int i = 0; i < nm && n < maxNotes - 64; ++i)
            if (mel[i].bar == b) out[n++] = { mel[i].start, mel[i].len, juce::jlimit (0, 127, mel[i].pitch), mel[i].vel, false };
        if (! edited[b])
            for (int i = 0; i < keptN[b] && n < maxNotes - 64; ++i)
                if (kept[b][i].pad)
                    out[n++] = { b * 4.0 + kept[b][i].step * 0.25, 1.6, juce::jlimit (0, 127, kept[b][i].pitch), 0.42f, true };
        emitChords (b, chordDeg);
    }
    return n;
}

// pure, allocation-free melody generator (used by the audio thread AND the editor).
// engine == 0 is the original generator, unchanged, so a saved project stays bit-identical.
int generate (const GenParams& g, Note* out, const EditStore* ed = nullptr)
{
    if (g.engine != 0)
        return generatePhrase (g, out, ed);
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
        const int code = lockNumber (g.locks[(size_t) juce::jlimit (0, 15, b)]);
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
       chordstyleI, pumpI, engineI, varietyI, epicI, epicSeededI };

std::vector<Spec> tuneSpecs()
{
    std::vector<Spec> s {
        C ("key",    "Key",       keyNames, 9, true, false),
        C ("scale",  "Scale",     { "Minor", "Harmonic Minor", "Phrygian", "Dorian", "Melodic Minor", "Major", "Phrygian Dominant", "Hungarian Minor", "Lydian" }, 0, true).fr (0, 7),
        C ("prog",   "Progression", progNames, 0, true).fr (0, 7),
        C ("bars",   "Bars",      { "4", "8", "16" }, 1, true, false),
        C ("rhythm", "Rhythm",    rhythmNames, 1, true).fr (1, 13),
        P ("density","Density",   0, 1, 0.6f, true, 0.35f, 0.9f).fr (0.58f, 0.9f),
        I ("octave", "Octave",    3, 6, 5, true, 4, 5).fr (4, 5),
        I ("range",  "Range",     3, 12, 7, true, 4, 9, "deg").fr (5, 10),
        I ("seed",   "Seed",      1, 9999, 1, true, 1, 1),
        P ("chords", "Chords",    0, 1, 0.35f, true, 0.2f, 0.5f).fr (0.25f, 0.5f),
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
    // appended last: parameter IDs stay frozen. Default is Phrase so a new instance writes the new melodies.
    // A project saved before this parameter has no "melodyEngine" marker; stateLoaded puts it back on Classic.
    s.push_back (C ("engine", "Engine", { "Classic", "Phrase" }, 1, false, false));
    // appended after Engine. Tight = the Phrase engine as first shipped (saved projects land here). Wide = the
    // varied phrases. Default Wide for a new instance; stateLoaded puts a project that never had this parameter on Tight.
    s.push_back (C ("variety", "Variety", { "Tight", "Wide" }, 1, false, false));
    s.push_back (C ("epic", "Epic", { "Off", "Epic" }, 0, false, false));   // symphonic themes; Classic ignores it
    s.push_back (C ("epicseeded", "Epic Seeded", { "Original", "Seeded" }, 0, false, false).as (Ui::hidden));
    return s;
}

// the synth sound params: a sound = these ten (the melody params are not touched)
const char* const soundIds[] = { "detune", "width", "cutoff", "reso", "fenv", "attack", "decay", "sustain", "release", "rsize" };
const float soundLo[] = { 10, 0.5f, 3000, 0.6f, 0.5f, 1, 200, 0.5f, 120, 0.6f };   // NEW SOUND picks inside these ranges
const float soundHi[] = { 35, 0.9f, 10000, 1.4f, 2.0f, 20, 900, 0.9f, 500, 0.9f };
// FRENCHCORE mode (180+ BPM, on with the Virtual Boy theme): minor-key stab leads (offbeat / gallop / hoover / pickup,
// not long hypnotic notes), short gates, fast attacks, short releases, a small room, PUMP on. Phrase engine writes the
// melody; Classic keeps older projects bit-identical. A beat at 190 BPM is ~315 ms, so nothing rings across the next kick.
const float soundLoFc[] = { 20, 0.5f, 5000, 0.8f, 1.0f, 1, 150, 0.4f, 80, 0.3f };   // NEW SOUND in FRENCHCORE mode
const float soundHiFc[] = { 45, 0.9f, 12000, 2.2f, 3.0f, 6, 500, 0.8f, 250, 0.6f };
std::vector<Preset> tuneFrenchPresets()
{
    return {
        { "FC Hypnotic Lead",   { {"key",6},{"scale",1},{"prog",0},{"rhythm",5},{"seed",4101},{"density",0.72f},{"cutoff",9000},{"attack",2},{"decay",250},{"sustain",0.5f},{"release",120},{"detune",32},{"reverb",0.25f},{"rsize",0.5f},{"pump",0.5f},{"chordstyle",1},{"engine",1} } },
        { "FC Epic Anthem 190", { {"key",7},{"scale",7},{"prog",17},{"rhythm",1},{"seed",4202},{"density",0.70f},{"cutoff",8000},{"release",160},{"pump",0.45f},{"chords",0.4f},{"reverb",0.25f},{"rsize",0.55f},{"engine",1} } },
        { "FC Dark Phrygian",   { {"key",9},{"scale",2},{"prog",2},{"rhythm",13},{"seed",4303},{"density",0.68f},{"cutoff",5000},{"reso",1.6f},{"release",110},{"pump",0.55f},{"chordstyle",3},{"reverb",0.2f},{"rsize",0.4f},{"engine",1} } },
        { "FC Rave Stabs",      { {"key",4},{"scale",6},{"prog",18},{"rhythm",13},{"seed",4404},{"density",0.78f},{"decay",140},{"sustain",0.0f},{"release",90},{"cutoff",7000},{"chordstyle",1},{"pump",0.6f},{"reverb",0.2f},{"rsize",0.4f},{"engine",1} } },
        { "FC Arp Machine",     { {"key",9},{"scale",0},{"prog",20},{"rhythm",6},{"seed",4505},{"density",0.80f},{"chordstyle",2},{"decay",120},{"sustain",0.1f},{"release",80},{"cutoff",8500},{"pump",0.4f},{"chords",0.4f},{"reverb",0.2f},{"rsize",0.4f},{"engine",1} } },
        { "FC Emotional Break", { {"key",5},{"scale",1},{"prog",4},{"rhythm",12},{"seed",4606},{"density",0.62f},{"attack",60},{"release",300},{"reverb",0.45f},{"rsize",0.7f},{"pump",0.0f},{"cutoff",5500},{"engine",1} } },
        { "FC Raw Scream Lead", { {"key",1},{"scale",7},{"prog",22},{"rhythm",6},{"seed",4707},{"density",0.82f},{"cutoff",12000},{"reso",2.2f},{"detune",45},{"attack",1},{"release",100},{"pump",0.5f},{"reverb",0.2f},{"rsize",0.4f},{"engine",1} } },
        { "FC Kick-Lock Gate",  { {"key",11},{"scale",0},{"prog",0},{"rhythm",5},{"seed",4808},{"chordstyle",3},{"pump",0.65f},{"density",0.70f},{"release",90},{"reverb",0.2f},{"rsize",0.4f},{"engine",1} } },
        { "FC Wide Hymn",       { {"key",2},{"scale",1},{"prog",19},{"rhythm",1},{"seed",4909},{"density",0.66f},{"attack",40},{"release",400},{"reverb",0.5f},{"rsize",0.75f},{"pump",0.35f},{"detune",38},{"width",0.95f},{"engine",1} } },
    };
}

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
        { "Epic Hymn",                { {"key",9},{"scale",0},{"prog",0},{"bars",2},{"rhythm",4},{"seed",5101},{"engine",1},{"epic",1},{"range",10},{"reverb",0.55f},{"attack",50},{"release",700},{"chordstyle",0},{"chords",0.45f} } },
        { "Epic Harmonic Lift",       { {"key",4},{"scale",1},{"prog",3},{"bars",2},{"rhythm",3},{"seed",5202},{"engine",1},{"epic",1},{"range",11},{"reverb",0.6f},{"attack",80},{"release",800},{"detune",28} } },
        { "Epic Phrygian",            { {"key",2},{"scale",2},{"prog",2},{"bars",2},{"rhythm",4},{"seed",5303},{"engine",1},{"epic",1},{"reverb",0.5f},{"attack",40},{"release",600},{"cutoff",5000} } },
        { "Epic Dorian Hymn",         { {"key",7},{"scale",3},{"prog",6},{"bars",2},{"rhythm",3},{"seed",5404},{"engine",1},{"epic",1},{"reverb",0.58f},{"attack",60},{"release",750},{"chords",0.5f} } },
        { "Epic Dominant Theme",      { {"key",11},{"scale",6},{"prog",18},{"bars",2},{"rhythm",4},{"seed",5505},{"engine",1},{"epic",1},{"range",10},{"reverb",0.52f},{"attack",30},{"release",650},{"cutoff",7000} } },
    };
}
}

class TuneProcessor : public BaseProcessor
{
public:
    TuneProcessor() : BaseProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true), tuneSpecs(), tunePresets())
    {
        for (auto& l : locks) l.store (0);
        own.attach (apvts);
        setFrenchPresets (tuneFrenchPresets());
        apvts.state.setProperty ("melodyEngine", 1, nullptr);   // this state knows the Engine parameter (old projects do not)
        apvts.state.setProperty ("phraseVariety", 1, nullptr);  // Wide phrases; a project saved before Variety falls back to Tight
        apvts.state.setProperty ("epicSeedVersion", 1, nullptr);
        setTheme (3);                                           // Virtual Boy (and its FRENCHCORE link) is the startup theme
    }
    const juce::String getName() const override { return BOY_NAME; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 3.0; }
    juce::AudioProcessorEditor* createEditor() override;

    GenParams genParams() const
    {
        static const int barsV[] = { 4, 8, 16 };
        GenParams g { (int) get (keyI), (int) get (scaleI), (int) get (progI), barsV[juce::jlimit (0, 2, (int) get (barsI))],
                      (int) get (rhythmI), (int) get (octI), (int) get (rangeI), (int) get (seedI), (int) get (variationI), get (densI), {}, {}, (int) get (chordstyleI),
                      (int) get (engineI), frenchcore() ? 1 : 0, (int) get (varietyI), (int) get (epicI), get (epicSeededI) > 0.5f };
        for (int b = 0; b < 16; ++b) g.locks[(size_t) b] = locks[(size_t) b].load (std::memory_order_relaxed);
        for (int k = 0; k < 4; ++k) g.cust[(size_t) k] = juce::jlimit (0, 6, (int) get (c1I + k) - 1);
        return g;
    }
    int generateNow (Note* dst) const { return generate (genParams(), dst, &edits); }
    void randomizeExtra (std::mt19937& rng) override
    {
        if (frenchcore())
        {
            const int rhythm = (int) get (rhythmI), scale = (int) get (scaleI);
            if (! isFrozen ("rhythm") && (rhythm == 3 || rhythm == 9))
            {
                const int stabs[] = { 1, 5, 6, 10, 11, 12, 13 };
                setParam ("rhythm", (float) stabs[std::uniform_int_distribution<int> (0, 6) (rng)]);
            }
            if (! isFrozen ("scale") && (scale == 3 || scale == 4 || scale == 5))
            {
                const int minorScales[] = { 0, 1, 2, 6, 7 };
                setParam ("scale", (float) minorScales[std::uniform_int_distribution<int> (0, 4) (rng)]);
            }
        }
        chooseMelody (false, rng, false);
    }

    bool chooseMelody (bool variation, std::mt19937& rng, bool undoable = true)
    {
        const char* id = variation ? "variation" : "seed";
        if (isFrozen (id)) return false;
        const auto g = genParams();
        const bool seededEpic = g.engine != 0 && g.epic > 0;
        const bool resetVariation = ! variation && ! isFrozen ("variation");
        const int current = variation ? g.variation : g.seed;
        std::vector<Note> generated (maxNotes);
        using Grid = std::array<int, 16 * 16>;
        auto fillGrid = [&] (const GenParams& params, Grid& grid)
        {
            grid.fill (-1);
            const int count = generate (params, generated.data(), &edits);
            for (int i = 0; i < count; ++i)
            {
                const auto& note = generated[(size_t) i];
                const int step = juce::jlimit (0, (int) grid.size() - 1, (int) std::floor (note.start * 4.0 + 1.0e-6));
                const int bar = step / 16;
                if (! note.chord && g.locks[(size_t) bar] == 0 && ! edits.edited (bar))
                    grid[(size_t) step] = note.pitch;
            }
        };

        Grid previous, candidate;
        fillGrid (g, previous);
        int best = current;
        float bestScore = -std::numeric_limits<float>::infinity();
        for (int attempt = 0; attempt < 10; ++attempt)
        {
            GenParams trial = g;
            if (seededEpic) trial.epicSeeded = true;
            const int value = differentRandomInt (rng, 1, variation ? 99 : 9999, current);
            if (variation) trial.variation = value;
            else { trial.seed = value; if (resetVariation) trial.variation = 0; }
            fillGrid (trial, candidate);

            int changed = 0, occupied = 0, pitches = 0, excess = 0, last = -1, repeated = 0;
            for (size_t step = 0; step < candidate.size(); ++step)
            {
                const int pitch = candidate[step];
                if (pitch >= 0 || previous[step] >= 0) ++occupied;
                if (pitch != previous[step]) ++changed;
                if (step % 16 == 0) { last = -1; repeated = 0; }
                if (pitch < 0) continue;
                ++pitches;
                if (last >= 0) excess += juce::jmax (0, std::abs (pitch - last) - 9);
                repeated = pitch == last ? repeated + 1 : 0;
                if (repeated > 2) ++excess;
                last = pitch;
            }
            if (changed == 0 || pitches == 0) continue;
            const float distance = (float) changed / (float) occupied;
            const float contrast = variation ? 1.0f - std::abs (distance - 0.30f) : juce::jmin (0.80f, distance);
            const float score = contrast - 0.35f * (float) excess / (float) pitches;
            if (score > bestScore) { bestScore = score; best = value; }
        }
        if (best == current) return false;
        if (undoable) rememberForUndo();
        if (seededEpic && ! g.epicSeeded) setParam ("epicseeded", 1.0f);
        if (resetVariation) setParam ("variation", 0.0f);
        setParam (id, (float) best);
        return true;
    }
    // everything the melody depends on, to notice a change
    GenKey keyOf (const GenParams& g) const
    {
        GenKey k { g.key, g.scale, g.prog, g.bars, g.rhythm, g.octave, g.range, g.seed, g.variation, (int) (g.density * 1000.0f) };
        for (int b = 0; b < 16; ++b) k[(size_t) (10 + b)] = g.locks[(size_t) b];
        k[26] = edits.rev.load();
        k[31] = g.chordStyle;
        for (int c = 0; c < 4; ++c) k[(size_t) (27 + c)] = g.cust[(size_t) c];
        k[32] = g.engine;
        k[33] = g.engine == 0 ? 0 : g.french;   // Classic ignores FRENCHCORE, so toggling it does not rebuild that melody
        k[34] = g.engine == 0 ? 0 : g.variety;
        k[35] = g.engine == 0 ? 0 : g.epic;
        k[36] = g.engine != 0 && g.epic > 0 && g.epicSeeded ? 1 : 0;
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
        auto& rng = randomEngine();
        static const auto bank = soundBank();
        const auto& model = bank[(size_t) std::uniform_int_distribution<int> (0, (int) bank.size() - 1) (rng)];
        const bool fc = frenchcore();
        rememberForUndo();
        for (int k = 0; k < (int) (sizeof (soundIds) / sizeof (soundIds[0])); ++k)
        {
            const auto* id = soundIds[k];
            if (isFrozen (id)) continue;
            const float lo = fc ? soundLoFc[k] : soundLo[k], hi = fc ? soundHiFc[k] : soundHi[k];
            const auto hit = std::find_if (model.values.begin(), model.values.end(),
                                           [id] (const auto& item) { return std::strcmp (item.first, id) == 0; });
            const float center = hit != model.values.end() ? juce::jlimit (lo, hi, hit->second) : (lo + hi) * 0.5f;
            const float offset = (hi - lo) * 0.25f;
            std::uniform_real_distribution<float> draw (juce::jmax (lo, center - offset), juce::jmin (hi, center + offset));
            setParam (id, draw (rng));
        }
        apvts.state.setProperty ("soundName", "Random", nullptr);
    }
    Sonic::OwnSound own { Sonic::OwnSound::Config { false, 60, false } };   // up to 16 sample voices; C5 plays the sample as recorded

    // ---- bar locks (the editor's bar numbers); kept in the state as "locks"
    int lockOf (int bar) const { return locks[(size_t) juce::jlimit (0, 15, bar)].load(); }
    void toggleLock (int bar)
    {
        bar = juce::jlimit (0, 15, bar);
        const bool seeded = get (engineI) > 0.5f && get (epicI) > 0.5f && get (epicSeededI) > 0.5f;
        locks[(size_t) bar].store (lockOf (bar) > 0 ? 0 : lockCode ((int) get (seedI), (int) get (variationI), seeded));
        juce::StringArray s; for (auto& l : locks) s.add (juce::String (l.load()));
        apvts.state.setProperty ("locks", s.joinIntoString (","), nullptr);
    }
    void stateLoaded() override
    {
        own.restore();
        loadEdits();
        const auto s = juce::StringArray::fromTokens (apvts.state.getProperty ("locks").toString(), ",", "");
        for (int b = 0; b < 16; ++b) locks[(size_t) b].store (b < s.size() ? juce::jmax (0, s[b].getIntValue()) : 0);
        // JUCE fills a parameter the chunk does not mention with its default (Phrase). Projects saved before
        // Engine existed have no melodyEngine marker, so they go back to Classic and sound as they did.
        if (! apvts.state.hasProperty ("melodyEngine"))
        {
            if (auto* p = apvts.getParameter ("engine"))
                p->setValueNotifyingHost (p->convertTo0to1 (0.0f));
            apvts.state.setProperty ("melodyEngine", 1, nullptr);
        }
        // Variety defaults to Wide. A project from before that parameter has no marker, so it stays on Tight
        // (the Phrase engine as first shipped) and sounds the same.
        if (! apvts.state.hasProperty ("phraseVariety"))
        {
            if (auto* p = apvts.getParameter ("variety"))
                p->setValueNotifyingHost (p->convertTo0to1 (0.0f));
            apvts.state.setProperty ("phraseVariety", 1, nullptr);
        }
        if (! apvts.state.hasProperty ("epicSeedVersion"))
        {
            if (auto* p = apvts.getParameter ("epicseeded")) p->setValueNotifyingHost (0.0f);
            apvts.state.setProperty ("epicSeedVersion", 1, nullptr);
        }
        // No saved theme (a new insert, or a project from before themes were stored): open on Virtual Boy.
        // A theme the user picked is kept, and the theme menu still changes it for the rest of the session.
        if (! apvts.state.hasProperty ("theme"))
        {
            const bool hadFc = apvts.state.hasProperty ("frenchcore");
            const bool fc = frenchcore();
            setTheme (3);
            if (hadFc) setFrenchcore (fc);
        }
    }

    // Render test: reload this processor as if the project was saved before Engine existed.
    void reloadAsLegacyProject()
    {
        juce::MemoryBlock in;
        getStateInformation (in);
        auto xml = getXmlFromBinary (in.getData(), (int) in.getSize());
        if (xml == nullptr) return;
        xml->removeAttribute ("melodyEngine");
        xml->removeAttribute ("phraseVariety");
        for (int i = xml->getNumChildElements(); --i >= 0;)
            if (auto* c = xml->getChildElement (i))
                if (c->getStringAttribute ("id") == "engine" || c->getStringAttribute ("id") == "variety" || c->getStringAttribute ("id") == "epic")
                    xml->removeChildElement (c, true);
        juce::MemoryBlock out;
        copyXmlToBinary (*xml, out);
        setStateInformation (out.getData(), (int) out.getSize());
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
        juce::TemporaryFile temp (f);
        {
            juce::FileOutputStream os (temp.getFile());
            if (! os.openedOk() || ! mf.writeTo (os)) return false;
            os.flush();
            if (os.getStatus().failed()) return false;
        }
        return temp.overwriteTargetFileWithTemporary();
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
        blockMidiMode = (int) get (midioutI);
        if (blockMidiMode == 2) sendOffs (out, 0, 0);
        if (blockMidiMode != 1) sendOffs (out, 0, 1);

        const auto gp = genParams();
        const auto key = keyOf (gp);
        own.gate = true;
        if (key != lastKey)
        {
            if (! running) { regenerate (gp, out, 0); }
            else pendingRegen = true;          // while it plays: the new melody starts at the next bar
        }
        loopLen = gp.bars * 4.0;

        // no host tempo: 150 (FRENCHCORE mode: 190)
        double bpm = frenchcore() ? 190.0 : 150.0, hostPpq = 0; bool hostPlaying = false;
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
        midi.swapWith (out);
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
        sendOffs (out, at, 0);
        sendOffs (out, at, 1);
    }
    void sendOffs (juce::MidiBuffer& out, int at, int channel)
    {
        for (int pitch = 0; pitch < 128; ++pitch)
            if (outOn[channel][pitch])
            {
                out.addEvent (juce::MidiMessage::noteOff (channel + 1, pitch), at);
                outOn[channel][pitch] = false;
            }
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
        const int playMode = (int) get (playI);
        const float chordsL = get (chordsI);
        const bool first = justStarted; justStarted = false;
        for (int i = 0; i < numNotes; ++i)
        {
            const auto& nt = notes[i];
            const int p = juce::jlimit (0, 127, nt.pitch + transpose);
            const bool toMidi = blockMidiMode == 0 ? ! nt.chord : blockMidiMode == 1;
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
    int held[16] {}; int nHeld = 0, transpose = 0, blockMidiMode = 0;
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
        addButton ("NEW MELODY", [this] { tp.chooseMelody (false, tp.randomEngine()); },
                   [] { return juce::String ("a new seed: a whole new melody (locked bars and bars you edited stay)"); });
        addButton ("VARIATION", [this] { tp.chooseMelody (true, tp.randomEngine()); },
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
