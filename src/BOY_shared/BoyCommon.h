// BOY series by sonicsystems (KICKBOY / BASSBOY / TUNEBOY): the shared processor side.
// Parameter specs (+ which control draws them), factory presets (editor-only), MY PRESETS files, RANDOMIZE with undo
// and FREEZE, a hardened processBlock, safe state loading, and the small DSP helpers. The UI is in BoyUI.h.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <random>

#ifndef BOY_NAME
 #error "BOY_NAME must be defined before including BoyCommon.h"
#endif

namespace boy
{
// how a parameter is drawn (automatic = picked from its range / unit / choices, see BoyUI.h)
enum class Ui { automatic, knob, bigKnob, fader, bar, segments, stepper, toggle, number, hidden };
// picture sets for segment buttons
enum Icons { noIcons = 0, distortionIcons, patternIcons };

struct Spec
{
    juce::String id, name;
    float min = 0, max = 1, def = 0;
    float skewCentre = 0;            // 0 = linear
    juce::StringArray choices;       // non-empty => choice param
    bool main = false;               // shown on the MAIN page
    float rndMin = 0, rndMax = 0;    // RANDOMIZE range (rndMin == rndMax => never randomized)
    juce::String unit;
    bool isInt = false;
    Ui ui = Ui::automatic;
    int icons = noIcons;
    float frLo = 0, frHi = 0; bool frSet = false;   // RANDOMIZE window in FRENCHCORE mode (180+ BPM); not set = the normal range
    Spec& fr (float lo, float hi) { frLo = lo; frHi = hi; frSet = true; return *this; }
    Spec& as (Ui u) { ui = u; return *this; }
    Spec& withIcons (int set) { icons = set; return *this; }
};

inline Spec P (juce::String id, juce::String name, float mn, float mx, float df, bool main,
               float rmn, float rmx, juce::String unit = {}, float skew = 0)
{ Spec s; s.id = id; s.name = name; s.min = mn; s.max = mx; s.def = df; s.main = main;
  s.rndMin = rmn; s.rndMax = rmx; s.unit = unit; s.skewCentre = skew; return s; }
inline Spec C (juce::String id, juce::String name, juce::StringArray ch, int df, bool main, bool rnd = true)
{ Spec s; s.id = id; s.name = name; s.choices = ch; s.min = 0; s.max = (float) ch.size() - 1; s.def = (float) df;
  s.main = main; s.rndMin = 0; s.rndMax = rnd ? s.max : 0; return s; }
inline Spec I (juce::String id, juce::String name, int mn, int mx, int df, bool main, int rmn, int rmx, juce::String unit = {})
{ auto s = P (id, name, (float) mn, (float) mx, (float) df, main, (float) rmn, (float) rmx, unit); s.isInt = true; return s; }

// OWN SOUND parameters (same IDs / meaning as src/SONICSYSTEMS_shared/OwnSound.h reads them)
inline void addOwnSoundSpecs (std::vector<Spec>& s, bool followNotes)
{
    s.push_back (P ("ownLevel", "Sample Level", -40, 6, 0, false, 0, 0, "dB"));
    s.push_back (P ("ownTune", "Sample Tune", -24, 24, 0, false, 0, 0, "st"));
    s.push_back (C ("ownKey", "Sample Pitch", { "Fixed", "Notes" }, followNotes ? 1 : 0, false, false));
    s.push_back (C ("ownMode", "Sample Mode", { "Layer", "Replace" }, 0, false, false));
}

struct Preset { juce::String name; std::vector<std::pair<const char*, float>> values; };

// Colours: four themes (BOY design system v2), each with the same roles. The old names stay so the heroes keep compiling:
//   bg = recessed LCD well, bg2 / panel = ground, panel2 / accentDim = line, text / textDim / warm = ink.
// Text is ink only (accent and line are non-text, >= 3:1). The theme is a state property ("theme"), not a parameter.
namespace col
{
    struct Theme { const char* id; const char* name; juce::uint32 lightest, light, dark, darkest; bool dark_ground; };
    inline constexpr Theme themes[] = {
        { "screen",     "Screen",      0xff9bbc0f, 0xff8bac0f, 0xff306230, 0xff0f380f, false },
        { "night",      "Night",       0xff9bbc0f, 0xff8bac0f, 0xff306230, 0xff0f380f, true  },
        { "pocket",     "Pocket",      0xffc4c4b0, 0xff8e8f7c, 0xff4f5044, 0xff1c1d18, false },
        { "virtualboy", "Virtual Boy", 0xffff3030, 0xffd42c2c, 0xff6a1010, 0xff120000, true  } };
    inline constexpr int numThemes = (int) (sizeof (themes) / sizeof (themes[0]));

    inline juce::Colour ground, well, ink, inkInverse, accent, line;
    inline juce::Colour bg, bg2, panel, panel2, accentDim, text, textDim, warm;   // legacy names (see above)
    inline int current = -1;

    inline int indexOf (const juce::String& id)
    { for (int i = 0; i < numThemes; ++i) if (id == themes[i].id) return i; return 0; }
    inline void set (int idx)
    {
        idx = juce::jlimit (0, numThemes - 1, idx);
        if (idx == current) return;
        current = idx;
        const auto& t = themes[idx];
        const juce::Colour lightest (t.lightest), light (t.light), dark (t.dark), darkest (t.darkest);
        ground    = t.dark_ground ? darkest : lightest;
        well      = t.dark_ground ? darkest : light;
        ink       = t.dark_ground ? lightest : darkest;
        inkInverse = t.dark_ground ? darkest : lightest;
        accent    = t.dark_ground ? light : dark;
        line      = accent;
        bg = well; bg2 = panel = ground; panel2 = accentDim = line; text = textDim = warm = ink;
    }
    struct Init { Init() { set (0); } };
    inline Init initDefault;
}

class BaseProcessor : public juce::AudioProcessor
{
public:
    BaseProcessor (const BusesProperties& b, std::vector<Spec> s, std::vector<Preset> pr)
      : juce::AudioProcessor (b), specs (std::move (s)), presets (std::move (pr)),
        apvts (*this, nullptr, "STATE", makeLayout (specs))
    {
        for (auto& sp : specs) raw.push_back (apvts.getRawParameterValue (sp.id));
        if (! presets.empty()) applyPreset (0);
    }

    // ---- presets: editor-only on purpose (getNumPrograms() = 1), so a host program change can never overwrite the
    //      sound saved in a project. The current name is kept in the state.
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return presetName(); }
    void changeProgramName (int, const juce::String&) override {}

    // ---- FRENCHCORE mode (180+ BPM): its own preset bank + a tighter RANDOMIZE window (Spec::fr). It follows the Virtual Boy
    //      theme (choosing that theme switches it on, any other theme off) and can also be switched by hand in the theme menu.
    //      A state property like the theme (not a parameter), so no parameter IDs change.
    std::vector<Preset> frenchPresets;
    void setFrenchPresets (std::vector<Preset> p) { frenchPresets = std::move (p); }
    bool frenchcore() const { return (int) apvts.state.getProperty ("frenchcore", 0) != 0 && ! frenchPresets.empty(); }
    void setFrenchcore (bool on)
    {
        if (on == frenchcore()) return;
        apvts.state.setProperty ("frenchcore", on ? 1 : 0, nullptr);
        apvts.state.setProperty ("presetIndex", -1, nullptr);   // the sound stays; the preset list and RANDOMIZE window change
    }
    const std::vector<Preset>& bank() const { return frenchcore() ? frenchPresets : presets; }   // the factory presets on offer now

    juce::String presetName() const { return apvts.state.getProperty ("presetName", presets.empty() ? "" : presets[0].name).toString(); }
    int themeIndex() const { return col::indexOf (apvts.state.getProperty ("theme", "screen").toString()); }
    void setTheme (int idx)
    {
        idx = juce::jlimit (0, col::numThemes - 1, idx);
        apvts.state.setProperty ("theme", col::themes[idx].id, nullptr);
        setFrenchcore (juce::String (col::themes[idx].id) == "virtualboy");   // Virtual Boy = FRENCHCORE mode
    }
    int presetIndex() const { return (int) apvts.state.getProperty ("presetIndex", 0); }   // -1 = user / random

    void applyPreset (int idx)
    {
        const auto& list = bank();
        if (! juce::isPositiveAndBelow (idx, (int) list.size())) return;
        for (auto& sp : specs)
            if (! sp.id.startsWith ("own"))   // OWN SOUND belongs to the user, not to the preset
                setParam (sp.id, sp.def);
        for (auto& kv : list[(size_t) idx].values) setParam (kv.first, kv.second);
        apvts.state.setProperty ("presetIndex", idx, nullptr);
        apvts.state.setProperty ("presetName", list[(size_t) idx].name, nullptr);
        presetChanged();
    }
    void stepPreset (int d)
    {
        const int n = (int) bank().size();
        if (n > 0) applyPreset (((presetIndex() < 0 ? 0 : presetIndex()) + d + n) % n);
    }

    // ---- RANDOMIZE (+ undo), FREEZE
    void randomize()
    {
        std::random_device rd; std::mt19937 rng (rd());
        pushUndo();
        for (auto& sp : specs)
        {
            const bool fc = frenchcore() && sp.frSet;   // FRENCHCORE window when the mode is on and the parameter has one
            const float lo = fc ? sp.frLo : sp.rndMin, hi = fc ? sp.frHi : sp.rndMax;
            if (lo == hi || isFrozen (sp.id)) continue;
            const bool discrete = ! sp.choices.isEmpty() || sp.isInt;
            std::uniform_real_distribution<float> d (discrete ? lo - 0.499f : lo, discrete ? hi + 0.499f : hi);
            float v = d (rng);
            if (discrete) v = std::round (v);
            setParam (sp.id, v);
        }
        randomizeExtra (rng);
        apvts.state.setProperty ("presetIndex", -1, nullptr);
        apvts.state.setProperty ("presetName", "Random", nullptr);
        presetChanged();
    }
    void rememberForUndo() { pushUndo(); }   // plugin-specific actions (e.g. a new synth sound) can be undone with the arrow
    bool canUndo() const { return ! undoStack.empty(); }
    void undo()
    {
        if (undoStack.empty()) return;
        auto snap = std::move (undoStack.back());
        undoStack.pop_back();
        auto& ps = getParameters();
        for (int i = 0; i < ps.size() && i < (int) snap.vals.size(); ++i)
            if (! juce::approximatelyEqual (ps[i]->getValue(), snap.vals[(size_t) i])) setNorm (*ps[i], snap.vals[(size_t) i]);
        apvts.state.setProperty ("presetName", snap.name, nullptr);
        apvts.state.setProperty ("presetIndex", snap.index, nullptr);
        presetChanged();
    }
    bool isFrozen (const juce::String& id) const
    { return juce::StringArray::fromTokens (apvts.state.getProperty ("frozen").toString(), " ", "").contains (id); }
    void toggleFrozen (const juce::String& id)
    {
        auto f = juce::StringArray::fromTokens (apvts.state.getProperty ("frozen").toString(), " ", "");
        if (f.contains (id)) f.removeString (id); else f.add (id);
        f.removeEmptyStrings();
        apvts.state.setProperty ("frozen", f.joinIntoString (" "), nullptr);
    }
    virtual void randomizeExtra (std::mt19937&) {}
    virtual void presetChanged() {}

    // one complete host gesture per change, so FL records it (automation / undo)
    void setParam (const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id)) setNorm (*p, p->convertTo0to1 (v));
    }
    static void setNorm (juce::AudioProcessorParameter& p, float n)
    {
        p.beginChangeGesture();
        p.setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, n));
        p.endChangeGesture();
    }
    float get (int i) const { return raw[(size_t) i]->load(); }

    // ---- MY PRESETS: Documents/sonicsystems Presets/<PLUGIN>/<name>.boypreset (the plugin state as XML)
    static juce::File userFolder()
    { return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("sonicsystems Presets").getChildFile (BOY_NAME); }
    juce::Array<juce::File> userPresets() const
    {
        auto files = userFolder().findChildFiles (juce::File::findFiles, false, "*.boypreset");
        files.sort();
        return files;
    }
    bool saveUserPreset (const juce::String& name)
    {
        const auto clean = juce::File::createLegalFileName (name.trim());
        if (clean.isEmpty()) return false;
        userFolder().createDirectory();
        apvts.state.setProperty ("presetIndex", -1, nullptr);
        apvts.state.setProperty ("presetName", clean, nullptr);
        auto st = apvts.copyState();
        st.removeProperty ("frozen", nullptr);
        st.removeProperty ("theme", nullptr);   // the look is yours, not part of a sound
        st.removeProperty ("frenchcore", nullptr);
        st.removeProperty ("uiScale", nullptr);
        auto x = st.createXml();
        return x != nullptr && x->writeTo (userFolder().getChildFile (clean + ".boypreset"));
    }
    bool loadUserPreset (const juce::File& f)
    {
        auto x = juce::parseXML (f);
        if (x == nullptr || ! x->hasTagName (apvts.state.getType())) return false;
        pushUndo();
        const auto frozen = apvts.state.getProperty ("frozen");   // your freeze marks stay
        const auto theme = apvts.state.getProperty ("theme");
        const auto frenchKept = apvts.state.getProperty ("frenchcore");
        const auto uiScaleKept = apvts.state.getProperty ("uiScale");
        loadXml (*x);
        apvts.state.setProperty ("frozen", frozen, nullptr);
        apvts.state.setProperty ("theme", theme, nullptr);
        apvts.state.setProperty ("frenchcore", frenchKept, nullptr);
        apvts.state.setProperty ("uiScale", uiScaleKept, nullptr);
        apvts.state.setProperty ("presetIndex", -1, nullptr);
        apvts.state.setProperty ("presetName", f.getFileNameWithoutExtension(), nullptr);
        return true;
    }

    // ---- state
    void getStateInformation (juce::MemoryBlock& dest) override
    {
        if (auto x = apvts.copyState().createXml()) copyXmlToBinary (*x, dest);
    }
    void setStateInformation (const void* d, int n) override
    {
        if (auto x = getXmlFromBinary (d, n))
            if (x->hasTagName (apvts.state.getType())) loadXml (*x);
    }
    virtual void stateLoaded() {}   // e.g. OWN SOUND brings its sample back

    // ---- hardened audio entry points (shared by all plugins)
    void prepareToPlay (double sampleRate, int maxBlock) override
    {
        sampleRateUsed = sampleRate > 0 ? sampleRate : 44100.0;
        pendingMidi.ensureSize (2048);
        prepareDsp (sampleRateUsed, juce::jmax (1, maxBlock));
        prepared = true;
    }
    void processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) override
    {
        juce::ScopedNoDenormals nd;
        const int n = buf.getNumSamples();
        if (n <= 0) { pendingMidi.addEvents (midi, 0, -1, 0); midi.clear(); return; }   // keep events for the next block
        if (! pendingMidi.isEmpty()) { midi.addEvents (pendingMidi, 0, -1, 0); pendingMidi.clear(); }
        if (buf.getNumChannels() < 2) { buf.clear(); midi.clear(); return; }
        if (! prepared) prepareToPlay (getSampleRate() > 0 ? getSampleRate() : 44100.0, juce::jmax (n, 512));   // host skipped prepare
        renderBlock (buf, midi);
        if (! producesMidi()) midi.clear();
        // NaN / Inf guard: never pass garbage to the host
        bool bad = false;
        for (int c = 0; c < 2 && ! bad; ++c) { auto* dd = buf.getReadPointer (c); for (int i = 0; i < n; ++i) if (! std::isfinite (dd[i])) { bad = true; break; } }
        if (bad) { buf.clear(); resetDsp(); }
    }
    void processBlockBypassed (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) override
    { buf.clear(); midi.clear(); stopAll (midi); }
    void reset() override { resetDsp(); }
    void releaseResources() override { resetDsp(); }

    virtual void prepareDsp (double sr, int maxBlock) = 0;
    virtual void renderBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) = 0;
    virtual void resetDsp() = 0;                   // silence everything, no allocation
    virtual void stopAll (juce::MidiBuffer&) { resetDsp(); }
    bool prepared = false; double sampleRateUsed = 44100.0; juce::MidiBuffer pendingMidi;

    bool hasEditor() const override { return true; }
    double getTailLengthSeconds() const override { return 0.5; }
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }

    int indexOf (const juce::String& id) const
    { for (size_t i = 0; i < specs.size(); ++i) if (specs[i].id == id) return (int) i; return -1; }

    std::vector<Spec> specs;
    std::vector<Preset> presets;
    juce::AudioProcessorValueTreeState apvts;
    std::vector<std::atomic<float>*> raw;

private:
    struct Snapshot { std::vector<float> vals; juce::String name; int index = 0; };
    std::vector<Snapshot> undoStack;
    void pushUndo()
    {
        Snapshot s;
        for (auto* p : getParameters()) s.vals.push_back (p->getValue());
        s.name = presetName(); s.index = presetIndex();
        undoStack.push_back (std::move (s));
        if (undoStack.size() > 16) undoStack.erase (undoStack.begin());
    }
    void loadXml (const juce::XmlElement& xml)
    {
        auto copy = xml;   // a broken / hand-edited state must not inject NaN: such values fall back to the default
        for (int i = copy.getNumChildElements(); --i >= 0;)
            if (auto* c = copy.getChildElement (i))
                if (c->hasAttribute ("value") && ! std::isfinite (c->getDoubleAttribute ("value"))) copy.removeChildElement (c, true);
        apvts.replaceState (juce::ValueTree::fromXml (copy));
        stateLoaded();
        presetChanged();
    }
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout (const std::vector<Spec>& specs)
    {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        for (auto& s : specs)
        {
            if (! s.choices.isEmpty())
                l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { s.id, 1 }, s.name, s.choices, (int) s.def));
            else if (s.isInt)
                l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { s.id, 1 }, s.name, (int) s.min, (int) s.max, (int) s.def,
                        juce::AudioParameterIntAttributes().withLabel (s.unit)));
            else
            {
                juce::NormalisableRange<float> r (s.min, s.max);
                if (s.skewCentre > s.min && s.skewCentre < s.max) r.setSkewForCentre (s.skewCentre);
                l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { s.id, 1 }, s.name, r, s.def,
                        juce::AudioParameterFloatAttributes().withLabel (s.unit)
                        .withStringFromValueFunction ([span = s.max - s.min, unit = s.unit] (float v, int) {
                            if (unit == "dB" && v <= -39.95f) return juce::String ("-inf dB");
                            if (unit == "Hz" && v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " kHz";
                            const int dp = span >= 100 ? 0 : (span >= 10 ? 1 : 2);
                            return juce::String (v, dp) + (unit.isEmpty() ? juce::String() : " " + unit); })));
            }
        }
        return l;
    }
};

// ---------------- DSP helpers ----------------
inline float softClip (float x) { return std::tanh (x); }
inline float hardClip (float x) { return juce::jlimit (-1.0f, 1.0f, x); }
inline float fold (float x) { return std::sin (x * 1.2f); }
inline float crush (float x, float bits) { const float q = std::pow (2.0f, bits); return std::round (x * q) / q; }

inline float distort (int type, float x, float drive)
{
    x *= drive;
    switch (type)
    {
        case 0: return softClip (x);
        case 1: return hardClip (x);
        case 2: return fold (x);
        case 3: return crush (softClip (x), 5.0f);
        default: { const float s = softClip (x * 1.5f); return hardClip (s * 1.3f); }   // "Raw" stacked
    }
}

struct OnePole { float z = 0, a = 0; void set (float hz, float sr) { a = std::exp (-juce::MathConstants<float>::twoPi * hz / sr); }
                 float lp (float x) { z = x + a * (z - x); return z; } float hp (float x) { return x - lp (x); } };

// TPT state-variable filter (cheap, stable under modulation)
struct SVF {
    float ic1 = 0, ic2 = 0, g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
    void set (float hz, float q, float sr) { hz = juce::jlimit (20.0f, sr * 0.45f, hz); g = std::tan (juce::MathConstants<float>::pi * hz / sr);
        k = 1.0f / q; a1 = 1 / (1 + g * (g + k)); a2 = g * a1; a3 = g * a2; }
    float lp (float v0) { const float v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; return v2; }
    void reset() { ic1 = ic2 = 0; }
};

// tilt EQ via a one-pole split at 600 Hz
struct Tilt { OnePole sp; float gl = 1, gh = 1;
    void set (float db, float sr) { sp.set (600.0f, sr); gh = juce::Decibels::decibelsToGain (db * 0.5f); gl = juce::Decibels::decibelsToGain (-db * 0.5f); }
    float process (float x) { const float l = sp.lp (x); return l * gl + (x - l) * gh; } };

// lookahead-free peak limiter (fast attack, smooth release)
struct Limiter { float env = 0, rel = 0.9995f, ceil = 0.98f;
    void set (float sr, float ceilingDb) { rel = std::exp (-1.0f / (0.08f * sr)); ceil = juce::Decibels::decibelsToGain (ceilingDb); }
    void process (float& l, float& r) { const float pk = juce::jmax (std::abs (l), std::abs (r));
        env = pk > env ? pk : env * rel + pk * (1 - rel);
        const float g = env > ceil ? ceil / env : 1.0f; l *= g; r *= g;
        l = juce::jlimit (-ceil, ceil, l); r = juce::jlimit (-ceil, ceil, r); } };

// per-sample glide towards a block target (no zipper noise on automation); ~20 ms
struct Smooth { float v = 0, c = 0.999f; bool init = false;
    void prepare (float sr, float ms = 20.0f) { c = std::exp (-1.0f / (ms * 0.001f * sr)); init = false; }
    float next (float target) { if (! init) { v = target; init = true; } v = target + c * (v - target); return v; } };

inline float polyBlep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1; }
    if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; }
    return 0;
}

inline float noteHz (float n) { return 440.0f * std::pow (2.0f, (n - 69.0f) / 12.0f); }
// the note you play, moved into the kick / bass octave E1..D#2 (MIDI 28..39): play G = a G kick / bass in any octave
inline int foldBass (int note) { while (note > 39) note -= 12; while (note < 28) note += 12; return note; }
inline float centsRatio (float cents) { return std::pow (2.0f, cents / 1200.0f); }

} // namespace boy
