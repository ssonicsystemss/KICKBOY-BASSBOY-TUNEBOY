// TUNEBOY by sonicsystems - hardstyle intro / euphoric melody generator with a built-in supersaw lead, BOY series.
// Key, scale, chord progression, bars, rhythm, density, seed -> a melody + chord layer, played while you hold a key
// (C5 = as generated, other keys transpose) or following the song position. NEW MELODY / VARIATION, lock the bars you
// like (click the bar numbers), EXPORT / DRAG MIDI. Changes made while it plays start at the next bar.
#include "BoyUI.h"

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
};
// chord roots as scale degrees (0-based), 4 chords
const int progs[][4] = {
    { 0, 5, 2, 6 },   // i  VI III VII  (the euphoric classic)
    { 5, 6, 0, 0 },   // VI VII i  i
    { 0, 6, 5, 6 },   // i  VII VI VII
    { 0, 3, 5, 4 },   // i  iv VI v
    { 5, 3, 0, 6 },   // VI iv i  VII
    { 0, 5, 3, 4 },   // i  VI iv v
    { 3, 5, 6, 0 },   // iv VI VII i
    { 0, 2, 6, 5 },   // i  III VII VI
};
const juce::StringArray progNames { "i-VI-III-VII", "VI-VII-i-i", "i-VII-VI-VII", "i-iv-VI-v", "VI-iv-i-VII", "i-VI-iv-v", "iv-VI-VII-i", "i-III-VII-VI" };
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
};
const juce::StringArray rhythmNames { "Straight 1/8", "Syncopated", "Euphoric 1/16", "Long Notes", "Dotted",
                                      "Offbeat Stabs", "Gallop", "Call & Response", "Rolling Arp", "Half-time" };
constexpr int numRhythms = 10;

struct GenParams { int key, scale, prog, bars, rhythm, octave, range, seed, variation; float density; std::array<int, 16> locks; };
struct Note { double start, len; int pitch; float vel; bool chord; };
constexpr int maxNotes = 1024;

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
int generate (const GenParams& g, Note* out)
{
    const int* sc = scales[juce::jlimit (0, 5, g.scale)];
    const int* pr = progs[juce::jlimit (0, 7, g.prog)];
    const int root = 12 * (g.octave + 1) + g.key;
    auto degToNote = [&] (int d) { const int o = (d >= 0 ? d / 7 : (d - 6) / 7); const int i = d - o * 7; return root + o * 12 + sc[i]; };
    const Motif free = makeMotif (g, g.seed, g.variation);
    int n = 0;
    const int bars = g.bars, barsPerChord = bars >= 16 ? 2 : 1;
    for (int b = 0; b < bars && n < maxNotes - 8; ++b)
    {
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
        if (b % barsPerChord == 0)   // chord layer (one entry per chord span)
            for (int k = 0; k < 3 && n < maxNotes; ++k)
                out[n++] = { b * 4.0, barsPerChord * 4.0 - 0.05, degToNote (chordDeg + k * 2 - 7), 0.55f, true };
    }
    return n;
}

enum { keyI, scaleI, progI, barsI, rhythmI, densI, octI, rangeI, seedI, chordsI, reverbI, outI,
       detuneI, widthI, cutoffI, resoI, fenvI, attackI, decayI, sustainI, releaseI, rsizeI, midioutI, playI, syncI, variationI, modeI };

std::vector<Spec> tuneSpecs()
{
    return {
        C ("key",    "Key",       keyNames, 9, true, false),
        C ("scale",  "Scale",     { "Minor", "Harmonic Minor", "Phrygian", "Dorian", "Melodic Minor", "Major" }, 0, true),
        C ("prog",   "Progression", progNames, 0, true),
        C ("bars",   "Bars",      { "4", "8", "16" }, 1, true, false),
        C ("rhythm", "Rhythm",    rhythmNames, 1, true),
        P ("density","Density",   0, 1, 0.6f, true, 0.35f, 0.9f),
        I ("octave", "Octave",    3, 6, 5, true, 4, 5),
        I ("range",  "Range",     3, 12, 7, true, 4, 9, "deg"),
        I ("seed",   "Seed",      1, 9999, 1, true, 1, 9999),
        P ("chords", "Chords",    0, 1, 0.35f, true, 0.2f, 0.5f),
        P ("reverb", "Reverb",    0, 1, 0.35f, true, 0.2f, 0.5f),
        P ("out",    "Output",    -24, 6, -6, true, -6, -6, "dB"),
        // ADVANCED (synth)
        P ("detune", "Detune",    0, 60, 22, false, 10, 35, "ct").as (Ui::bigKnob),
        P ("width",  "Width",     0, 1, 0.7f, false, 0.5f, 0.9f),
        P ("cutoff", "Cutoff",    300, 16000, 6000, false, 3000, 10000, "Hz", 3000),
        P ("reso",   "Resonance", 0.5f, 4, 0.8f, false, 0.6f, 1.4f),
        P ("fenv",   "Filter Env",0, 4, 1.2f, false, 0.5f, 2, "oct"),
        P ("attack", "Attack",    0, 300, 3, false, 1, 20, "ms", 30),
        P ("decay",  "Decay",     10, 2000, 400, false, 200, 900, "ms", 300),
        P ("sustain","Sustain",   0, 1, 0.7f, false, 0.5f, 0.9f),
        P ("release","Release",   5, 2000, 250, false, 120, 500, "ms", 300),
        P ("rsize",  "Reverb Size",0, 0.9f, 0.8f, false, 0.6f, 0.9f),
        C ("midiout","MIDI Out",  { "Melody", "Melody+Chords", "Off" }, 0, false, false),
        C ("play",   "Play",      { "Both", "Melody", "Chords" }, 0, false, false),
        C ("sync",   "Start",     { "Key (restart)", "Song position" }, 0, false, false),
        I ("variation", "Variation", 0, 99, 0, false, 0, 0).as (Ui::hidden),
        C ("mode",   "Mode",      { "Melody", "Single Note" }, 0, true, false),
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
    };
}
}

class TuneProcessor : public BaseProcessor
{
public:
    TuneProcessor() : BaseProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true), tuneSpecs(), tunePresets())
    { for (auto& l : locks) l.store (0); }
    const juce::String getName() const override { return BOY_NAME; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 3.0; }
    juce::AudioProcessorEditor* createEditor() override;

    GenParams genParams() const
    {
        static const int barsV[] = { 4, 8, 16 };
        GenParams g { (int) get (keyI), (int) get (scaleI), (int) get (progI), barsV[juce::jlimit (0, 2, (int) get (barsI))],
                      (int) get (rhythmI), (int) get (octI), (int) get (rangeI), (int) get (seedI), (int) get (variationI), get (densI), {} };
        for (int b = 0; b < 16; ++b) g.locks[(size_t) b] = locks[(size_t) b].load (std::memory_order_relaxed);
        return g;
    }
    int generateNow (Note* dst) const { return generate (genParams(), dst); }
    void randomizeExtra (std::mt19937&) override { setParam ("variation", 0.0f); }   // a new seed starts without variation
    // everything the melody depends on, to notice a change
    static std::array<int, 26> keyOf (const GenParams& g)
    {
        std::array<int, 26> k { g.key, g.scale, g.prog, g.bars, g.rhythm, g.octave, g.range, g.seed, g.variation, (int) (g.density * 1000.0f) };
        for (int b = 0; b < 16; ++b) k[(size_t) (10 + b)] = g.locks[(size_t) b];
        return k;
    }

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
        const auto s = juce::StringArray::fromTokens (apvts.state.getProperty ("locks").toString(), ",", "");
        for (int b = 0; b < 16; ++b) locks[(size_t) b].store (b < s.size() ? juce::jmax (0, s[b].getIntValue()) : 0);
    }

    void prepareDsp (double sampleRate, int) override
    {
        sr = (float) sampleRate; rev.setSampleRate (sampleRate); lastKey = {}; outMidi.ensureSize (8192); outSm.prepare (sr); resetDsp();
    }
    void resetDsp() override
    {
        for (auto& v : voices) v = Voice{};
        rev.reset(); lim = Limiter{}; nHeld = 0; running = false; wasHostPlaying = false; pendingRegen = false; guiBeat.store (-1.0f);
        for (auto& c : outOn) for (auto& o : c) o = false;
    }
    void stopAll (juce::MidiBuffer& out) override { sendOffs (out, 0); resetDsp(); }

    bool writeMidiFile (const juce::File& f) const
    {
        std::vector<Note> tmp (maxNotes); const int n = generateNow (tmp.data());
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
        numNotes = generate (gp, notes); lastKey = keyOf (gp); pendingRegen = false;
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
        for (int k = 0; k < 5; ++k) best->ph[k] = (float) ((p * 7 + k * 13) % 17) / 17.0f;
    }
    void releaseVoices (int at)
    {
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
    { for (auto& vo : voices) if (vo.active && vo.note == p && vo.chord == chord && vo.stopAt < 0 && (vo.gate || vo.startAt >= 0)) { vo.stopAt = at; break; } }

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
        juce::Reverb::Parameters rp; rp.roomSize = get (rsizeI); rp.damping = 0.5f; rp.width = 1; rp.wetLevel = get (reverbI) * 0.5f; rp.dryLevel = 1.0f - get (reverbI) * 0.35f;
        rev.setParameters (rp); rev.processStereo (L, R, n);
        const float outG = juce::Decibels::decibelsToGain (get (outI)); lim.set (sr, -0.3f);
        for (int s = 0; s < n; ++s) { const float og = outSm.next (outG); float l = L[s] * og, r = R[s] * og; lim.process (l, r); L[s] = l; R[s] = r; }
    }

    float sr = 44100; Voice voices[16]; juce::Reverb rev; Limiter lim; Smooth outSm;
    Note notes[maxNotes]; int numNotes = 0; std::array<int, 26> lastKey {};
    bool outOn[2][128] {}; bool wasHostPlaying = false, running = false, wasSingle = false, songMode = false, justStarted = false, pendingRegen = false;
    int held[16] {}; int nHeld = 0, transpose = 0;
    double pos = 0, loopLen = 32, ppqInc = 0.001, blockHostPpq = 0;
    juce::MidiBuffer outMidi;
    std::array<std::atomic<int>, 16> locks;
};

// ---------------------------------------------------------------- editor
class TuneEditor : public Editor
{
public:
    explicit TuneEditor (TuneProcessor& p)
      : Editor (p, BOY_NAME, "melody  |  hold a note", 880, 560), tp (p)
    {
        notes.resize (maxNotes);
        addButton ("NEW MELODY", [this] { tp.setParam ("variation", 0.0f); tp.setParam ("seed", (float) juce::Random::getSystemRandom().nextInt ({ 1, 10000 })); },
                   [] { return juce::String ("a new seed: a whole new melody (locked bars stay)"); });
        addButton ("VARIATION", [this] { tp.setParam ("variation", (float) juce::Random::getSystemRandom().nextInt ({ 1, 100 })); },
                   [] { return juce::String ("the same melody, a few notes changed"); });
        addButton ("EXPORT MIDI", [this] { exportMidi(); }, [] { return juce::String ("save the melody as a .mid file"); });
        addButton ("DRAG MIDI", {}, [] { return juce::String ("hold and drag onto FL's playlist or piano roll"); }, true);
        refreshRoll();   // the roll is filled right away (not only on the first timer tick)
        init();
    }

protected:
    int heroHeight() const override { return 150; }
    bool animating() const override { return tp.guiBeat.load() >= 0.0f || wasPlaying; }
    void tick() override
    {
        wasPlaying = tp.guiBeat.load() >= 0.0f;
        if (refreshRoll()) repaint();
    }
    bool refreshRoll()
    {
        const auto gp = tp.genParams();
        const auto k = TuneProcessor::keyOf (gp);
        if (k == shownKey) return false;
        shownKey = k; count = generate (gp, notes.data()); bars = gp.bars;
        return true;
    }

    juce::Rectangle<float> header (juce::Rectangle<float> a) const { return a.reduced (10.0f, 6.0f).removeFromTop (22.0f); }
    juce::Rectangle<float> rollArea (juce::Rectangle<float> a) const { auto r = a.reduced (10.0f, 6.0f); r.removeFromTop (28.0f); return r; }

    void paintHero (juce::Graphics& g, juce::Rectangle<float> area) override
    {
        const auto hd = header (area), r = rollArea (area);
        const double end = bars * 4.0;
        const float bw = r.getWidth() / (float) bars;
        // bar numbers: click = lock / unlock that bar (it keeps its notes through NEW MELODY / VARIATION)
        for (int b = 0; b < bars; ++b)
        {
            const auto cell = juce::Rectangle<float> (hd.getX() + bw * (float) b, hd.getY(), bw, hd.getHeight()).reduced (1.0f, 0.0f);
            const bool locked = tp.lockOf (b) > 0;
            g.setColour (locked ? col::ink : col::well);
            g.fillRect (cell);
            g.setColour (col::ink);
            g.drawRect (cell, locked ? 2.0f : 1.0f);
            g.setColour (locked ? col::inkInverse : col::ink);
            g.setFont (monoFont (20.0f));
            g.drawText ((locked ? juce::String ("#") : juce::String()) + juce::String (b + 1), cell, juce::Justification::centred, false);
            if (locked) { g.setColour (col::accent.withAlpha (0.18f)); g.fillRect (r.getX() + bw * (float) b, r.getY(), bw, r.getHeight()); }
        }
        if (count > 0)
        {
            int lo = 127, hi = 0;
            for (int i = 0; i < count; ++i) { lo = juce::jmin (lo, notes[(size_t) i].pitch); hi = juce::jmax (hi, notes[(size_t) i].pitch); }
            hi += 1;
            const float rowH = r.getHeight() / (float) juce::jmax (1, hi - lo + 1);
            g.setColour (col::line.withAlpha (0.6f));
            for (int b = 0; b <= bars; ++b) g.drawVerticalLine ((int) (r.getX() + bw * (float) b), r.getY(), r.getBottom());
            for (int i = 0; i < count; ++i)
            {
                const auto& nt = notes[(size_t) i];
                const float x = r.getX() + r.getWidth() * (float) (nt.start / end), w = juce::jmax (2.0f, r.getWidth() * (float) (nt.len / end));
                const float y = r.getBottom() - (float) (nt.pitch - lo + 1) * rowH;
                g.setColour (nt.chord ? col::accent : col::ink);   // chords accent, melody ink
                g.fillRect (x, y, w - 1, juce::jmax (2.0f, rowH - 1));
            }
        }
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
    bool heroDown (juce::Point<float> p, const juce::MouseEvent&) override
    {
        const auto hd = header (heroArea);
        if (hd.expanded (0, 3).contains (p))
        {
            const int b = juce::jlimit (0, bars - 1, (int) ((p.x - hd.getX()) / (hd.getWidth() / (float) bars)));
            tp.toggleLock (b);
            status (tp.lockOf (b) > 0 ? "bar " + juce::String (b + 1) + " locked: it keeps these notes through NEW MELODY / VARIATION"
                                      : "bar " + juce::String (b + 1) + " unlocked");
            repaint();
        }
        return false;
    }
    juce::String heroHint (juce::Point<float> p) override
    {
        if (real ("mode") > 0.5f) return "Single Note: the keys you play sound as they are (Mode: Melody brings it back)";
        return header (heroArea).expanded (0, 3).contains (p) ? "click a bar number: lock / unlock that bar"
                                                              : "the melody (bright) and the chords (dim); changes start at the next bar while it plays";
    }
    void dragOut (Button&) override
    {
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (juce::String (BOY_NAME) + "_melody.mid");
        if (tp.writeMidiFile (f)) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }

private:
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
    TuneProcessor& tp;
    std::vector<Note> notes;
    int count = 0, bars = 8;
    std::array<int, 26> shownKey {};
    bool wasPlaying = false;
    std::unique_ptr<juce::FileChooser> chooser;
};

juce::AudioProcessorEditor* TuneProcessor::createEditor() { return new TuneEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TuneProcessor(); }
