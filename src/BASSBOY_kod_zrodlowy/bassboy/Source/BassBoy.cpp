// BASSBOY by sonicsystems - tempo-synced hardstyle reverse bass (MIDI instrument), BOY series.
// Hold a note, press play: the bass swells after every "kick" of the pattern and leaves a gap for the kick.
// Play the key of your track (G = a G bass, folded into E1..D#2, same map as KICKBOY). Mono (glide) or Chord mode,
// saw/sine mix, detune + width, 5 distortions (2x oversampled), resonant filter with envelope, pitch rise.
// 14 patterns + Custom: click the steps in the picture to make your own; drag the curve for Kick Gap / Swell.
#include "BoyUI.h"

using namespace boy;

namespace
{
enum { gapI, swellI, riseI, mixI, detuneI, driveI, cutoffI, resoI, fenvI, glideI, patI, outI,
       dtypeI, subI, octI, widthI, fadeI, lowcutI, tiltI, velI, syncI, bpmI, ceilI, voicesI, osI, customI };

const char* const patternSteps[] = {
    "X---X---X---X---",   // Classic offbeat reverse (one swell per beat)
    "X-X-X-X-X-X-X-X-",   // Rolling 1/8
    "X--XX--XX--XX--X",   // Gallop
    "X---X-X-X---X-X-",   // Bounce
    "X-------X-------",   // Half-time
    "XXXXXXXXXXXXXXXX",   // Build 1/16
    "X--X--X-X--X--X-",   // Shuffle 3-3-2
    "X---X---X---X-X-",   // Push (an extra 1/8 at the end)
    "X---X---X-------",   // Skip beat 4 (room for a fill)
    "X---X---X---XXXX",   // Fill 1/16 on beat 4
    "X---X---X-X-XXXX",   // Rolling fill
    "X---X--X--X-X---",   // Breakbeat
    "XX--XX--XX--XX--",   // Stutter
    "X-----X-X-----X-",   // Long + short
};
constexpr int numFixedPatterns = (int) (sizeof (patternSteps) / sizeof (patternSteps[0]));
const juce::StringArray patternNames { "Classic", "Rolling 1/8", "Gallop", "Bounce", "Half-time", "Build 1/16", "Shuffle 3-3-2",
                                       "Push", "Skip Beat 4", "Fill 1/16", "Rolling Fill", "Breakbeat", "Stutter", "Long + Short", "Custom" };

// the 16 steps of a pattern (index numFixedPatterns = Custom, from the bit mask)
std::array<bool, 16> stepsOf (int pattern, int custom)
{
    std::array<bool, 16> s {};
    if (pattern >= 0 && pattern < numFixedPatterns) for (int k = 0; k < 16; ++k) s[(size_t) k] = patternSteps[pattern][k] == 'X';
    else for (int k = 0; k < 16; ++k) s[(size_t) k] = ((custom >> k) & 1) != 0;
    return s;
}

std::vector<Spec> bassSpecs()
{
    return {
        P ("gap",    "Kick Gap",    0, 0.8f, 0.3f, true, 0.2f, 0.45f).fr (0.25f, 0.5f),
        P ("swell",  "Swell",       0.3f, 4, 1.6f, true, 0.8f, 2.8f).fr (0.9f, 1.8f),
        P ("rise",   "Pitch Rise",  0, 24, 5, true, 0, 12, "st").fr (0, 7),
        P ("mix",    "Saw/Sine",    0, 1, 0.5f, true, 0.2f, 0.8f).fr (0.1f, 0.6f),
        P ("detune", "Detune",      0, 50, 12, true, 4, 30, "ct").fr (4, 22),
        P ("drive",  "Drive",       0, 36, 12, true, 4, 26, "dB").as (Ui::bigKnob).fr (14, 34),
        P ("cutoff", "Cutoff",      40, 12000, 900, true, 300, 4000, "Hz", 1000).fr (800, 3500),
        P ("reso",   "Resonance",   0.5f, 8, 1.2f, true, 0.6f, 3.5f).fr (0.8f, 3),
        P ("fenv",   "Filter Env",  0, 6, 2, true, 0.5f, 4, "oct").fr (0.5f, 3),
        P ("glide",  "Glide",       0, 500, 40, true, 0, 150, "ms", 100).fr (0, 60),
        C ("pattern","Pattern",     patternNames, 0, true).withIcons (patternIcons).fr (0, 3),
        P ("out",    "Output",      -24, 6, -4, true, -4, -4, "dB"),
        // ADVANCED
        C ("dtype",  "Distortion",  { "Soft", "Hard", "Fold", "Crush", "Raw" }, 0, false).withIcons (distortionIcons).fr (1, 4),
        C ("sub",    "Sine Octave", { "0", "-1" }, 0, false),
        I ("oct",    "Octave",      -2, 2, 0, false, -1, 0),
        P ("width",  "Width",       0, 1, 0.3f, false, 0, 0.6f).fr (0, 0.4f),
        P ("fade",   "End Fade",    0.5f, 20, 3, false, 1, 6, "ms").fr (1, 4),
        P ("lowcut", "Low Cut",     10, 200, 30, false, 20, 45, "Hz", 50).fr (25, 45),
        P ("tilt",   "EQ Tilt",     -12, 12, 0, false, -3, 3, "dB").fr (-2, 3),
        P ("vel",    "Velocity",    0, 1, 0.2f, false, 0.2f, 0.2f),
        C ("sync",   "Sync",        { "Host", "Free" }, 0, false, false),
        P ("bpm",    "Free BPM",    100, 220, 150, false, 150, 150),
        P ("ceil",   "Ceiling",     -12, 0, -0.3f, false, -0.3f, -0.3f, "dB"),
        C ("voices", "Voices",      { "Mono", "Chord" }, 0, false, false),
        C ("os",     "Oversampling",{ "Off", "2x" }, 1, false, false),
        I ("custom", "Custom Steps", 0, 65535, 0x1111, false, 0, 0).as (Ui::hidden),
    };
}

std::vector<Preset> bassPresets()
{
    return {
        { "Euphoric Reverse",   { {"gap",0.3f},{"swell",1.6f},{"rise",5},{"mix",0.5f},{"cutoff",900},{"fenv",2} } },
        { "Classic Hardstyle 2010", { {"gap",0.35f},{"swell",2.0f},{"rise",7},{"mix",0.4f},{"drive",16},{"cutoff",1200} } },
        { "Deep Sine Swell",    { {"gap",0.3f},{"swell",1.2f},{"rise",3},{"mix",0.9f},{"drive",6},{"cutoff",500},{"fenv",1} } },
        { "Raw Saw Reverse",    { {"gap",0.3f},{"swell",2.2f},{"rise",9},{"mix",0.15f},{"drive",24},{"dtype",4},{"cutoff",2500},{"reso",2} } },
        { "Rolling Eighths",    { {"pattern",1},{"gap",0.25f},{"swell",1.4f},{"rise",4},{"cutoff",1100} } },
        { "Gallop Rider",       { {"pattern",2},{"gap",0.2f},{"swell",1.5f},{"rise",6},{"drive",14} } },
        { "Bounce Groove",      { {"pattern",3},{"gap",0.25f},{"swell",1.3f},{"cutoff",1500},{"reso",2.5f} } },
        { "Half-time Breakdown",{ {"pattern",4},{"gap",0.2f},{"swell",1.0f},{"rise",12},{"cutoff",700},{"fenv",3} } },
        { "Build-up Riser",     { {"pattern",5},{"gap",0.15f},{"swell",1.0f},{"rise",3},{"cutoff",2000} } },
        { "Acid Squelch",       { {"gap",0.3f},{"mix",0.1f},{"cutoff",400},{"reso",6},{"fenv",4},{"drive",18} } },
        { "Folded Grit",        { {"dtype",2},{"drive",20},{"mix",0.4f},{"cutoff",1800},{"rise",5} } },
        { "Crushed Low",        { {"dtype",3},{"drive",14},{"mix",0.6f},{"cutoff",900} } },
        { "Wide Supersaw Bass", { {"mix",0.2f},{"detune",28},{"width",0.8f},{"cutoff",1800},{"drive",10} } },
        { "Sub Octave Monster", { {"sub",1},{"mix",0.7f},{"drive",16},{"cutoff",800},{"rise",4} } },
        { "Tight Short Pump",   { {"gap",0.5f},{"swell",2.8f},{"rise",2},{"cutoff",1300} } },
        { "Long Swell",         { {"gap",0.12f},{"swell",0.9f},{"rise",7},{"cutoff",1000} } },
        { "Hard Clip Punch",    { {"dtype",1},{"drive",22},{"mix",0.35f},{"cutoff",2200},{"reso",1.5f} } },
        { "Gliding Legato",     { {"glide",180},{"rise",2},{"mix",0.5f},{"cutoff",1000} } },
        { "Dark Reverse",       { {"cutoff",350},{"fenv",1.5f},{"mix",0.6f},{"drive",10},{"tilt",-4} } },
        { "Screaming Raw",      { {"dtype",4},{"drive",30},{"mix",0.05f},{"cutoff",4000},{"reso",3},{"rise",12},{"tilt",3} } },
        { "Shuffle Rider",      { {"pattern",6},{"gap",0.2f},{"swell",1.4f},{"rise",4},{"cutoff",1300},{"drive",14} } },
        { "Breakbeat Bass",     { {"pattern",11},{"gap",0.22f},{"swell",1.2f},{"mix",0.35f},{"cutoff",1600},{"reso",2.0f} } },
        { "Stutter Chords",     { {"pattern",12},{"voices",1},{"gap",0.15f},{"swell",1.1f},{"mix",0.4f},{"cutoff",1400},{"width",0.6f} } },
        { "Fill Into Drop",     { {"pattern",9},{"gap",0.25f},{"swell",1.5f},{"rise",7},{"drive",18},{"dtype",4} } },
    };
}
}

// FRENCHCORE mode (180+ BPM, on with the Virtual Boy theme): at 190 BPM a beat is ~315 ms, so the swells are faster (Swell ~1-1.8),
// the kick gap is wider, the patterns are the straight / rolling / gallop ones and the distortion is hard. The presets also set Free BPM 190.
std::vector<Preset> bassFrenchPresets()
{
    return {
        { "FC Rolling Bass 190",    { {"bpm",190},{"pattern",1},{"gap",0.3f},{"swell",1.2f},{"rise",3},{"mix",0.3f},{"drive",28},{"dtype",4},{"cutoff",1800},{"reso",1.8f},{"fenv",2.5f} } },
        { "FC Gallop Raw",          { {"bpm",190},{"pattern",2},{"gap",0.3f},{"swell",1.1f},{"rise",5},{"mix",0.15f},{"drive",32},{"dtype",4},{"cutoff",2500},{"reso",2},{"detune",18} } },
        { "FC Classic Offbeat 200", { {"bpm",200},{"pattern",0},{"gap",0.4f},{"swell",1.4f},{"rise",4},{"mix",0.35f},{"drive",24},{"dtype",1},{"cutoff",1400} } },
        { "FC Bounce Hard",         { {"bpm",190},{"pattern",3},{"gap",0.35f},{"swell",1.3f},{"drive",26},{"dtype",1},{"cutoff",2000},{"reso",2.5f} } },
        { "FC Stutter Bass",        { {"bpm",190},{"pattern",12},{"gap",0.25f},{"swell",1.0f},{"mix",0.25f},{"cutoff",1700},{"drive",30},{"dtype",2} } },
        { "FC Dark Rumble Bass",    { {"bpm",185},{"pattern",1},{"gap",0.35f},{"swell",1.5f},{"mix",0.7f},{"drive",20},{"cutoff",700},{"fenv",1.5f},{"tilt",-3},{"sub",1} } },
        { "FC Screamer",            { {"bpm",195},{"pattern",1},{"gap",0.3f},{"swell",1.0f},{"dtype",4},{"drive",36},{"mix",0.05f},{"cutoff",3800},{"reso",3},{"rise",9},{"tilt",3} } },
        { "FC Pumping Chords",      { {"bpm",190},{"pattern",12},{"voices",1},{"gap",0.3f},{"swell",1.1f},{"mix",0.4f},{"cutoff",1500},{"width",0.5f},{"drive",18} } },
        { "FC Tight Short",         { {"bpm",190},{"gap",0.5f},{"swell",1.8f},{"rise",2},{"cutoff",1500},{"drive",22},{"dtype",4} } },
        { "FC Crushed Industrial",  { {"bpm",200},{"pattern",1},{"gap",0.3f},{"swell",1.1f},{"dtype",3},{"drive",22},{"cutoff",1400} } },
    };
}

class BassProcessor : public BaseProcessor
{
public:
    BassProcessor() : BaseProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true), bassSpecs(), bassPresets()) { setFrenchPresets (bassFrenchPresets()); }
    const juce::String getName() const override { return BOY_NAME; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override;

    void prepareDsp (double sampleRate, int) override
    {
        sr = (float) sampleRate;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);
        os->initProcessing (chunk);
        pre.setSize (2, chunk); envBuf.assign (chunk, 0.0f); gBuf.assign (chunk, 0.0f);
        outSm.prepare (sr);
        resetDsp();
    }
    void resetDsp() override
    {
        fl = fr = SVF{}; hp1 = hp2 = hp3 = hp4 = OnePole{}; tl = tr = Tilt{}; lim = Limiter{};
        nHeld = 0; gateEnv = 0; wasPlaying = false; guiPos.store (-1.0f);
        if (os) os->reset();
    }

    void renderBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) override
    {
        buf.clear();
        const int n = buf.getNumSamples();
        double bpm = get (bpmI), ppq = freePpq; bool hostPlaying = false;
        if (get (syncI) < 0.5f)
            if (auto* ph = getPlayHead())
                if (auto pos = ph->getPosition())
                {
                    if (auto b = pos->getBpm()) if (std::isfinite (*b) && *b > 20.0) bpm = *b;
                    if (pos->getIsPlaying()) if (auto q = pos->getPpqPosition()) if (std::isfinite (*q)) { ppq = *q; hostPlaying = true; }
                }
        if (wasPlaying && ! hostPlaying) nHeld = 0;      // transport stopped -> release everything
        wasPlaying = hostPlaying;
        const double ppqInc = bpm / 60.0 / sr;
        const float gap = get (gapI), swell = get (swellI), rise = get (riseI), mix = get (mixI);
        const float detUp = centsRatio (get (detuneI)), detDn = 1.0f / detUp, drive = juce::Decibels::decibelsToGain (get (driveI));
        const float cutoff = get (cutoffI), reso = get (resoI), fenv = get (fenvI);
        const float glideCoef = std::exp (-1.0f / (juce::jmax (0.5f, get (glideI)) * 0.001f * sr));
        const auto steps = stepsOf ((int) get (patI), (int) get (customI));
        const int dtype = (int) get (dtypeI); const bool sub = get (subI) > 0.5f; const float octv = get (octI) * 12.0f;
        const float width = get (widthI), fadeBeats = (float) (get (fadeI) * 0.001 * bpm / 60.0);
        const float outG = juce::Decibels::decibelsToGain (get (outI)), velAmt = get (velI);
        const bool chord = get (voicesI) > 0.5f, useOs = get (osI) > 0.5f;
        hp1.set (get (lowcutI), sr); hp2.set (get (lowcutI), sr); hp3.set (get (lowcutI), sr); hp4.set (get (lowcutI), sr);
        tl.set (get (tiltI), sr); tr.set (get (tiltI), sr); lim.set (sr, get (ceilI));
        const float gateCoef = std::exp (-1.0f / (0.004f * sr));

        auto* L = buf.getWritePointer (0); auto* R = buf.getWritePointer (1);
        auto it = midi.cbegin();
        float lastPos = -1.0f;
        for (int start = 0; start < n; start += chunk)
        {
            const int m = juce::jmin (chunk, n - start);
            auto* xl = pre.getWritePointer (0); auto* xr = pre.getWritePointer (1);
            for (int i = 0; i < m; ++i)
            {
                const int s = start + i;
                while (it != midi.cend() && (*it).samplePosition <= s)
                {
                    const auto msg = (*it).getMessage();
                    if (msg.isNoteOn())
                    {
                        removeHeld (msg.getNoteNumber());   // FL can resend note-on: never double-count
                        if (nHeld == 0 && ! hostPlaying) freePpq = ppq = 0;
                        if (nHeld == 0) curNote = (float) foldBass (msg.getNoteNumber());
                        if (nHeld < 16) held[nHeld++] = msg.getNoteNumber();
                        velG = 1.0f - velAmt + velAmt * msg.getFloatVelocity();
                    }
                    else if (msg.isNoteOff()) removeHeld (msg.getNoteNumber());
                    else if (msg.isAllNotesOff() || msg.isAllSoundOff()) nHeld = 0;
                    ++it;
                }
                gateEnv = (nHeld > 0 ? 1.0f : 0.0f) + gateCoef * (gateEnv - (nHeld > 0 ? 1.0f : 0.0f));
                if (gateEnv < 1e-4f && nHeld == 0) { xl[i] = xr[i] = 0; envBuf[(size_t) i] = 0; gBuf[(size_t) i] = 0; ppq += ppqInc; continue; }

                // the voices: Mono = the last key (with glide), Chord = every held key (up to 4), all moved by the
                // same fold as the lowest key so the chord keeps its shape
                int lowest = 128; for (int k = 0; k < nHeld; ++k) lowest = juce::jmin (lowest, held[k]);
                const int shift = lowest < 128 ? foldBass (lowest) - lowest : 0;
                if (nHeld > 0) target = (float) foldBass (held[nHeld - 1]);
                curNote = target + glideCoef * (curNote - target);
                int nv = 1;
                float notes[4] { curNote };
                if (chord && nHeld > 1)
                {
                    nv = juce::jmin (4, nHeld);
                    for (int k = 0; k < nv; ++k) notes[k] = (float) (held[nHeld - nv + k] + shift);
                }

                // pattern position (16 steps per bar of 4 beats)
                double barPos = std::fmod (ppq, 4.0); if (barPos < 0) barPos += 4.0;
                const int step = juce::jlimit (0, 15, (int) (barPos * 4.0));
                int st = step; while (st > 0 && ! steps[(size_t) st]) --st;
                float env = 0, pr = 0;
                if (steps[(size_t) st])
                {
                    int en = step + 1; while (en < 16 && ! steps[(size_t) en]) ++en;
                    const double hs = st * 0.25, hl = (en - st) * 0.25;
                    const float p = (float) ((barPos - hs) / hl);
                    if (p > gap) { pr = (p - gap) / (1 - gap); env = std::pow (pr, swell); }
                    const float remain = (float) (hs + hl - barPos);
                    if (remain < fadeBeats) env *= remain / fadeBeats;
                }
                lastPos = (float) (barPos / 4.0);
                const float riseSt = rise * std::pow (pr, 4.0f);
                float a = 0, b = 0;
                for (int k = 0; k < nv; ++k)
                {
                    auto& v = vo[(size_t) k];
                    const float f = noteHz (notes[k] + octv + riseSt);
                    const float dt1 = f * detUp / sr, dt2 = f * detDn / sr;
                    v.ph1 += dt1; if (v.ph1 >= 1) v.ph1 -= 1;
                    v.ph2 += dt2; if (v.ph2 >= 1) v.ph2 -= 1;
                    v.phS += (sub ? f * 0.5f : f) / sr; if (v.phS >= 1) v.phS -= 1;
                    const float s1 = 2 * v.ph1 - 1 - polyBlep (v.ph1, dt1), s2 = 2 * v.ph2 - 1 - polyBlep (v.ph2, dt2);
                    const float sine = std::sin (juce::MathConstants<float>::twoPi * v.phS);
                    a += s1 * (1 - mix) + sine * mix;
                    b += s2 * (1 - mix) + sine * mix;
                }
                const float comp = 1.0f / std::sqrt ((float) nv);   // a chord hits the distortion like one note
                a *= comp; b *= comp;
                const float mid = 0.5f * (a + b);
                xl[i] = mid + 0.5f * width * (a - b);
                xr[i] = mid - 0.5f * width * (a - b);
                envBuf[(size_t) i] = env;
                gBuf[(size_t) i] = env * gateEnv * velG;
                ppq += ppqInc;
            }
            // distortion, 2x oversampled (the saw at high drive no longer aliases)
            if (useOs && os != nullptr)
            {
                juce::dsp::AudioBlock<float> blk (pre); auto sb = blk.getSubBlock (0, (size_t) m);
                auto up = os->processSamplesUp (sb);
                for (size_t c = 0; c < 2; ++c) { auto* u = up.getChannelPointer (c); for (size_t i = 0; i < up.getNumSamples(); ++i) u[i] = distort (dtype, u[i], drive); }
                os->processSamplesDown (sb);
            }
            else
                for (int i = 0; i < m; ++i) { xl[i] = distort (dtype, xl[i], drive); xr[i] = distort (dtype, xr[i], drive); }

            for (int i = 0; i < m; ++i)
            {
                if ((ctr++ & 15) == 0) { const float fc = cutoff * std::pow (2.0f, fenv * envBuf[(size_t) i]); fl.set (fc, reso, sr); fr.set (fc, reso, sr); }
                const float g = gBuf[(size_t) i] * outSm.next (outG) * 0.8f;
                float l = fl.lp (xl[i]), r = fr.lp (xr[i]);
                l = tl.process (hp2.hp (hp1.hp (l * g))); r = tr.process (hp4.hp (hp3.hp (r * g)));
                lim.process (l, r);
                L[start + i] = l; R[start + i] = r;
            }
        }
        if (! hostPlaying) freePpq = ppq;
        guiPos.store (nHeld > 0 || gateEnv > 1e-3f ? lastPos : -1.0f, std::memory_order_relaxed);
    }

    std::atomic<float> guiPos { -1.0f };   // 0..1 within the bar while it plays, -1 = silent (for the picture)

private:
    struct Voice { float ph1 = 0, ph2 = 0.37f, phS = 0; };
    void removeHeld (int note) { int w = 0; for (int k = 0; k < nHeld; ++k) if (held[k] != note) held[w++] = held[k]; nHeld = w; }
    static constexpr int chunk = 256;
    bool wasPlaying = false;
    float sr = 44100, curNote = 36, target = 36, velG = 1, gateEnv = 0;
    double freePpq = 0; int held[16] {}; int nHeld = 0; unsigned ctr = 0;
    std::array<Voice, 4> vo {};
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::AudioBuffer<float> pre; std::vector<float> envBuf, gBuf;
    SVF fl, fr; OnePole hp1, hp2, hp3, hp4; Tilt tl, tr; Limiter lim; Smooth outSm;
};

// ---------------------------------------------------------------- editor
class BassEditor : public Editor
{
public:
    explicit BassEditor (BassProcessor& p)
      : Editor (p, BOY_NAME, "reverse bass  |  hold the key", 880, 520), bp (p)
    { init(); }

protected:
    int heroHeight() const override { return 140; }
    bool animating() const override { return bp.guiPos.load() >= 0.0f || wasMoving; }
    void tick() override { wasMoving = bp.guiPos.load() >= 0.0f; }

    juce::Rectangle<float> stepRow (juce::Rectangle<float> a) const { return a.reduced (14.0f, 10.0f).removeFromTop (22.0f); }
    juce::Rectangle<float> curveArea (juce::Rectangle<float> a) const { auto r = a.reduced (14.0f, 10.0f); r.removeFromTop (30.0f); return r; }
    std::array<bool, 16> currentSteps() const { return stepsOf (juce::roundToInt (real ("pattern")), juce::roundToInt (real ("custom"))); }

    void paintHero (juce::Graphics& g, juce::Rectangle<float> area) override
    {
        const auto steps = currentSteps();
        const auto row = stepRow (area), cur = curveArea (area);
        const float sw = row.getWidth() / 16.0f;
        // the 16 steps (click = your own pattern); beats marked
        for (int k = 0; k < 16; ++k)
        {
            auto c = juce::Rectangle<float> (row.getX() + sw * (float) k, row.getY(), sw, row.getHeight()).reduced (2.0f, 0.0f);
            if (steps[(size_t) k]) { g.setColour (col::accent); g.fillRect (c); }
            g.setColour (col::ink);
            g.drawRect (c, k % 4 == 0 ? 2.0f : 1.0f);   // beats get the heavier frame
        }
        // the swell of every segment + the kick at its start
        const float gap = real ("gap"), swell = real ("swell"), rise = real ("rise");
        juce::Path p;
        p.startNewSubPath (cur.getX(), cur.getBottom());
        for (int x = 0; x <= (int) cur.getWidth(); x += 2)
        {
            const float pos16 = 16.0f * (float) x / cur.getWidth();
            const int step = juce::jlimit (0, 15, (int) pos16);
            int st = step; while (st > 0 && ! steps[(size_t) st]) --st;
            float env = 0;
            if (steps[(size_t) st])
            {
                int en = step + 1; while (en < 16 && ! steps[(size_t) en]) ++en;
                const float ph = (pos16 - (float) st) / (float) (en - st);
                if (ph > gap) env = std::pow ((ph - gap) / (1 - gap), swell);
            }
            p.lineTo (cur.getX() + (float) x, cur.getBottom() - cur.getHeight() * 0.92f * env);
        }
        p.lineTo (cur.getRight(), cur.getBottom());
        p.closeSubPath();
        g.setColour (col::accent.withAlpha (0.5f));
        g.fillPath (p);
        g.setColour (col::ink);
        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
        for (int k = 0; k < 16; ++k)
            if (steps[(size_t) k])
            {
                const float x = cur.getX() + cur.getWidth() * (float) k / 16.0f;
                g.setColour (col::ink);
                g.fillRect (x - 1.0f, cur.getY() + cur.getHeight() * 0.35f, 3.0f, cur.getHeight() * 0.65f);
            }
        // the playhead
        const float pos = bp.guiPos.load();
        if (pos >= 0.0f)
        {
            const float x = cur.getX() + cur.getWidth() * pos;
            g.setColour (col::ink);
            g.fillRect (x - 1.5f, row.getY(), 4.0f, cur.getBottom() - row.getY());
        }
        heroLabel (g, monoFont (20.0f), cur, "KICK", col::ink, juce::Justification::bottomLeft);
        heroLabel (g, monoFont (20.0f), cur.reduced (2.0f), patternNames[juce::roundToInt (real ("pattern"))] + "   rise +" + juce::String (rise, 0) + " st",
                   col::ink, juce::Justification::topRight);
    }
    bool heroDown (juce::Point<float> p, const juce::MouseEvent&) override
    {
        const auto row = stepRow (heroArea);
        if (row.expanded (0, 4).contains (p))   // a step: copy the pattern into Custom and toggle it
        {
            const int k = juce::jlimit (0, 15, (int) ((p.x - row.getX()) / (row.getWidth() / 16.0f)));
            auto s = currentSteps();
            s[(size_t) k] = ! s[(size_t) k];
            int mask = 0; for (int i = 0; i < 16; ++i) if (s[(size_t) i]) mask |= 1 << i;
            editBegin ({ "custom", "pattern" });
            editSet ("custom", (float) mask);
            editSet ("pattern", (float) numFixedPatterns);
            editEnd();
            return false;
        }
        editBegin ({ "gap", "swell" });
        startGap = real ("gap"); startSwell = real ("swell"); startPos = p;
        return true;
    }
    void heroDrag (juce::Point<float> p, const juce::MouseEvent&) override
    {
        const float segW = curveArea (heroArea).getWidth() / 4.0f;
        editSet ("gap", juce::jlimit (0.0f, 0.8f, startGap + (p.x - startPos.x) / segW));
        editSet ("swell", juce::jlimit (0.3f, 4.0f, startSwell * std::pow (2.0f, (p.y - startPos.y) / 60.0f)));
    }
    void heroUp() override { editEnd(); }
    juce::String heroHint (juce::Point<float> p) override
    {
        return stepRow (heroArea).expanded (0, 4).contains (p) ? "click the steps: your own pattern (Custom); a lit step = a kick + a new swell"
                                                               : "drag: left / right = Kick Gap, up / down = Swell shape";
    }
    void paintSegmentIcon (juce::Graphics& g, const Spec& s, int k, juce::Rectangle<float> r, juce::Colour ink) override
    {
        if (s.icons == distortionIcons)
        {
            juce::Path p;
            for (int i = 0; i <= 24; ++i)
            {
                const float xin = -1.0f + 2.0f * (float) i / 24.0f, y = juce::jlimit (-1.0f, 1.0f, distort (k, xin, 2.5f));
                const juce::Point<float> pt (r.getX() + r.getWidth() * (float) i / 24.0f, r.getCentreY() - y * r.getHeight() * 0.5f);
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            g.setColour (ink); g.strokePath (p, juce::PathStrokeType (1.6f));
            return;
        }
        // a pattern as a tiny 4x4 grid of its steps
        const auto st = stepsOf (k, juce::roundToInt (real ("custom")));
        const float cw = r.getWidth() / 4.0f, ch = r.getHeight() / 4.0f;
        for (int i = 0; i < 16; ++i)
        {
            g.setColour (st[(size_t) i] ? ink : ink.withAlpha (0.18f));
            g.fillRect (r.getX() + cw * (float) (i % 4) + 0.5f, r.getY() + ch * (float) (i / 4) + 0.5f, cw - 1.0f, ch - 1.0f);
        }
    }

private:
    BassProcessor& bp;
    bool wasMoving = false;
    float startGap = 0, startSwell = 1;
    juce::Point<float> startPos;
};

juce::AudioProcessorEditor* BassProcessor::createEditor() { return new BassEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BassProcessor(); }
