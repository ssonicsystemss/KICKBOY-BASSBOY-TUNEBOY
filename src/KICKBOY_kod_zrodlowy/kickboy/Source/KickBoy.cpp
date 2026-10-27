// KICKBOY by sonicsystems - hardstyle kick synthesizer (MIDI instrument), BOY series.
// Play the key of your track: G (any octave) = a G kick (tail folded into E1..D#2). Punch/click, body with a pitch
// envelope, 5 distortions (optionally 2x oversampled), delayed rumble, tone, low cut, tilt, limiter.
// Note End: FULL = one-shot (Length), CUT = the tail also ends with the piano-roll note. OWN SOUND: your own sample
// layered on the kick or replacing it. The editor's picture is the pitch curve: drag it (Pitch Start / Decay).
#include "BoyUI.h"
#include "OwnSound.h"

using namespace boy;

namespace
{
enum { tuneI, pstartI, pdecayI, punchI, driveI, dtypeI, toneI, rumbleI, lengthI, releaseI, lowcutI, outI,
       pcurveI, shapeI, ctoneI, cdecayI, dmixI, rdecayI, rdriveI, rtoneI, rdelayI, attackI, tiltI, velI, ktrackI, ceilI, osI,
       noteendI, ownA, ownB, ownC, ownD, rfollowI };   // own* = the 4 OWN SOUND params, then the newest

std::vector<Spec> kickSpecs()
{
    std::vector<Spec> s {
        // MAIN (12)
        P ("tune",   "Tune",        -12, 12, 0, true, -2, 2, "st"),
        P ("pstart", "Pitch Start",  80, 3000, 700, true, 300, 1800, "Hz", 500).fr (900, 2200),
        P ("pdecay", "Pitch Decay",  5, 400, 60, true, 25, 160, "ms", 80).fr (18, 50),
        P ("punch",  "Punch",        0, 1, 0.5f, true, 0.2f, 0.9f).fr (0.5f, 0.9f),
        P ("drive",  "Drive",        0, 48, 24, true, 10, 40, "dB").as (Ui::bigKnob).fr (30, 44),
        C ("dtype",  "Distortion",   { "Soft", "Hard", "Fold", "Crush", "Raw" }, 1, true).withIcons (distortionIcons).fr (1, 4),
        P ("tone",   "Tone",         300, 16000, 6000, true, 1500, 12000, "Hz", 3000).fr (6000, 12000),
        P ("rumble", "Rumble",       0, 1, 0.2f, true, 0, 0.6f).fr (0.1f, 0.7f),
        P ("length", "Length",       40, 2000, 450, true, 200, 900, "ms", 400).fr (130, 260),
        P ("release","Release",      2, 800, 60, true, 5, 200, "ms", 100).fr (5, 50),
        P ("lowcut", "Low Cut",      10, 200, 25, true, 18, 40, "Hz", 50).fr (20, 40),
        P ("out",    "Output",       -24, 6, -3, true, -3, -3, "dB"),
        // ADVANCED
        P ("pcurve", "Pitch Curve",  0.5f, 8, 3, false, 1.5f, 5).fr (2.5f, 6),
        P ("shape",  "Body Shape",   0, 1, 0, false, 0, 0.5f).fr (0, 0.5f),
        P ("ctone",  "Click Tone",   500, 12000, 4000, false, 1500, 9000, "Hz", 3000).fr (3000, 10000),
        P ("cdecay", "Click Decay",  0.5f, 30, 4, false, 1, 10, "ms", 5).fr (1, 6),
        P ("dmix",   "Dist Mix",     0, 1, 1, false, 0.7f, 1).fr (0.8f, 1),
        P ("rdecay", "Rumble Decay", 50, 2000, 600, false, 200, 1200, "ms", 500).fr (250, 700),
        P ("rdrive", "Rumble Drive", 0, 30, 12, false, 3, 24, "dB").fr (10, 30),
        P ("rtone",  "Rumble Tone",  60, 800, 200, false, 100, 400, "Hz", 200).fr (100, 300),
        P ("rdelay", "Rumble Delay", 0, 200, 40, false, 10, 90, "ms").fr (5, 40),
        P ("attack", "Attack",       0, 20, 0.3f, false, 0, 1, "ms").fr (0, 1),
        P ("tilt",   "EQ Tilt",      -12, 12, 0, false, -4, 4, "dB").fr (-2, 5),
        P ("vel",    "Velocity",     0, 1, 0.3f, false, 0.3f, 0.3f),
        C ("ktrack", "Key Track",    { "Off", "On" }, 1, false, false),
        P ("ceil",   "Ceiling",      -12, 0, -0.3f, false, -0.3f, -0.3f, "dB"),
        C ("os",     "Oversampling", { "Off", "2x" }, 1, false, false),
        C ("noteend","Note End",     { "Full", "Cut" }, 0, false, false),
    };
    addOwnSoundSpecs (s, false);
    s.push_back (C ("rfollow", "Rumble Tail", { "Free", "Follow" }, 1, false, false));   // Follow: the rumble ends with Length + Release
    return s;
}

std::vector<Preset> kickPresets()
{
    return {
        { "Euphoric Classic",   { {"pstart",650},{"pdecay",70},{"punch",0.45f},{"drive",20},{"dtype",0},{"tone",5000},{"rumble",0.15f},{"length",480},{"release",90} } },
        { "Euphoric Punchy",    { {"pstart",900},{"pdecay",50},{"punch",0.7f},{"drive",24},{"dtype",1},{"tone",7000},{"rumble",0.1f},{"length",420} } },
        { "Euphoric Long Tail", { {"pstart",600},{"pdecay",90},{"drive",22},{"dtype",0},{"tone",4500},{"rumble",0.35f},{"length",900},{"release",200},{"rdecay",1000} } },
        { "Raw Classic",        { {"pstart",1200},{"pdecay",45},{"punch",0.6f},{"drive",34},{"dtype",4},{"tone",9000},{"rumble",0.25f},{"length",520},{"pcurve",4} } },
        { "Raw Screech",        { {"pstart",1800},{"pdecay",30},{"punch",0.8f},{"drive",40},{"dtype",4},{"tone",12000},{"shape",0.35f},{"length",500},{"tilt",3} } },
        { "Rawstyle Gated",     { {"pstart",1400},{"pdecay",40},{"punch",0.7f},{"drive",38},{"dtype",1},{"tone",10000},{"length",300},{"release",5},{"rumble",0.0f} } },
        { "Rawstyle Distorted Tail", { {"pstart",1100},{"pdecay",55},{"drive",42},{"dtype",4},{"tone",8000},{"rumble",0.5f},{"rdrive",24},{"length",700},{"release",40} } },
        { "Gated Hard",         { {"pstart",1000},{"pdecay",50},{"drive",30},{"dtype",1},{"tone",8000},{"length",250},{"release",3},{"rumble",0} } },
        { "Gated Short Stab",   { {"pstart",900},{"pdecay",35},{"punch",0.8f},{"drive",28},{"dtype",1},{"length",160},{"release",3},{"rumble",0} } },
        { "Zaag Saw",           { {"pstart",1500},{"pdecay",40},{"drive",36},{"dtype",4},{"shape",0.8f},{"tone",11000},{"tilt",4},{"length",520} } },
        { "Zaag Folded",        { {"pstart",1300},{"pdecay",45},{"drive",30},{"dtype",2},{"shape",0.6f},{"tone",9000},{"length",500} } },
        { "Crushed Grit",       { {"pstart",1000},{"pdecay",60},{"drive",26},{"dtype",3},{"tone",7000},{"rumble",0.2f},{"length",480} } },
        { "Early Hardstyle",    { {"pstart",500},{"pdecay",90},{"punch",0.35f},{"drive",16},{"dtype",0},{"tone",3500},{"rumble",0.1f},{"length",380},{"release",100} } },
        { "Uptempo Tight",      { {"pstart",1600},{"pdecay",25},{"punch",0.9f},{"drive",40},{"dtype",1},{"tone",11000},{"length",220},{"release",10},{"rumble",0} } },
        { "Hardcore Fold",      { {"pstart",2000},{"pdecay",30},{"drive",44},{"dtype",2},{"tone",12000},{"length",350},{"release",20} } },
        { "Deep Rumble",        { {"pstart",500},{"pdecay",80},{"drive",18},{"dtype",0},{"tone",3000},{"rumble",0.7f},{"rdecay",1500},{"rtone",150},{"length",1000},{"release",300} } },
        { "Punch Only",         { {"pstart",800},{"pdecay",40},{"punch",1.0f},{"drive",14},{"dtype",0},{"length",150},{"release",30},{"rumble",0} } },
        { "Festival Mainstage", { {"pstart",950},{"pdecay",55},{"punch",0.6f},{"drive",30},{"dtype",4},{"tone",8000},{"rumble",0.3f},{"length",600},{"tilt",2} } },
        { "Dark Tunnel",        { {"pstart",700},{"pdecay",75},{"drive",32},{"dtype",3},{"tone",2500},{"tilt",-4},{"rumble",0.4f},{"length",650} } },
        { "Clean Sub",          { {"pstart",300},{"pdecay",100},{"punch",0.2f},{"drive",4},{"dtype",0},{"tone",2000},{"rumble",0},{"length",700},{"release",150} } },
    };
}
}

// FRENCHCORE mode (180+ BPM, on with the Virtual Boy theme): short, hard, heavily clipped kicks with a pitched tail. At 190 BPM a beat
// is ~315 ms, so Length stays under ~260 ms and the rumble tail is the melodic part. These presets + the window in each spec's .fr().
std::vector<Preset> kickFrenchPresets()
{
    return {
        { "FC Classic 190",       { {"pstart",1400},{"pdecay",35},{"punch",0.7f},{"drive",40},{"dtype",4},{"tone",9000},{"rumble",0.35f},{"length",240},{"release",30},{"rdecay",400},{"rdrive",24},{"rdelay",20},{"pcurve",4} } },
        { "FC Gabber Crunch",     { {"pstart",1800},{"pdecay",28},{"punch",0.8f},{"drive",44},{"dtype",1},{"tone",11000},{"rumble",0.2f},{"length",200},{"release",15},{"lowcut",30},{"pcurve",5} } },
        { "FC Melodic Tail",      { {"pstart",1100},{"pdecay",45},{"punch",0.6f},{"drive",36},{"dtype",4},{"tone",8000},{"rumble",0.6f},{"length",260},{"release",50},{"rdecay",650},{"rdrive",20},{"rtone",180},{"rdelay",25} } },
        { "FC Fold Hardcore",     { {"pstart",1600},{"pdecay",32},{"punch",0.75f},{"drive",38},{"dtype",2},{"tone",10000},{"rumble",0.3f},{"length",220},{"release",25},{"shape",0.3f} } },
        { "FC Tight 200",         { {"pstart",1500},{"pdecay",26},{"punch",0.85f},{"drive",42},{"dtype",4},{"tone",11000},{"rumble",0.15f},{"length",170},{"release",10},{"lowcut",32} } },
        { "FC Industrial Crush",  { {"pstart",1300},{"pdecay",38},{"punch",0.7f},{"drive",34},{"dtype",3},{"tone",9000},{"rumble",0.4f},{"length",230},{"release",35},{"rdrive",22} } },
        { "FC Rolling 1/16",      { {"pstart",1500},{"pdecay",22},{"punch",0.8f},{"drive",40},{"dtype",1},{"tone",10000},{"rumble",0.0f},{"length",110},{"release",5},{"lowcut",35} } },
        { "FC Dark Rumble",       { {"pstart",1000},{"pdecay",42},{"punch",0.55f},{"drive",38},{"dtype",4},{"tone",6500},{"rumble",0.8f},{"length",250},{"release",45},{"rdecay",600},{"rtone",140},{"rdrive",26},{"tilt",-2} } },
        { "FC Euphoric Frenchcore", { {"pstart",900},{"pdecay",50},{"punch",0.5f},{"drive",30},{"dtype",0},{"tone",7000},{"rumble",0.5f},{"length",280},{"release",60},{"rdecay",700},{"rtone",170} } },
        { "FC Hard Stab",         { {"pstart",2000},{"pdecay",20},{"punch",0.9f},{"drive",44},{"dtype",4},{"tone",12000},{"rumble",0.1f},{"length",140},{"release",8},{"lowcut",38},{"pcurve",6} } },
    };
}

class KickProcessor : public BaseProcessor
{
public:
    KickProcessor() : BaseProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true), kickSpecs(), kickPresets())
    { setFrenchPresets (kickFrenchPresets()); own.attach (apvts); }
    const juce::String getName() const override { return BOY_NAME; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override;
    double getTailLengthSeconds() const override
    {
        const double body = (get (lengthI) + get (releaseI) * 6.0f) * 0.001, rumbleSec = get (rumbleI) > 0.0f ? (get (rdelayI) + get (rdecayI) * 6.0f) * 0.001 : 0.0;
        return juce::jlimit (0.5, 8.0, juce::jmax (body, rumbleSec) + 0.1);
    }
    void stateLoaded() override { own.restore(); }

    // the tail frequency for a note (shared with the editor's picture)
    float tailHz (int n) const { return noteHz ((float) (get (ktrackI) > 0.5f ? foldBass (n) : 36) + get (tuneI)); }

    void prepareDsp (double sampleRate, int) override
    {
        sr = (float) sampleRate;
        os = std::make_unique<juce::dsp::Oversampling<float>> (1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);
        os->initProcessing (chunk);
        pre.setSize (1, chunk); env.assign (chunk, 0.0f); rum.assign (chunk, 0.0f); retrig.assign (chunk, 0);
        outSm.prepare (sr);
        own.prepare (sampleRate);
        setLatencySamples (0);
        resetDsp();
    }
    void resetDsp() override
    {
        active = false; hp1 = hp2 = clickHp = OnePole{}; tone = SVF{}; rtone = SVF{}; clickF = SVF{}; tilt = Tilt{}; lim = Limiter{};
        declick = 0; lastOut = 0;
        if (os) os->reset();
    }

    void renderBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) override
    {
        buf.clear();
        const int n = buf.getNumSamples();
        auto it = midi.cbegin();
        const float pStart = get (pstartI), pDec = get (pdecayI) * 0.001f, pCurve = get (pcurveI), shape = get (shapeI);
        const float punch = get (punchI), drive = juce::Decibels::decibelsToGain (get (driveI)); const int dtype = (int) get (dtypeI);
        const float cDec = get (cdecayI) * 0.001f, dmix = get (dmixI), rumbleL = get (rumbleI), rDec = get (rdecayI) * 0.001f;
        const float rDrive = juce::Decibels::decibelsToGain (get (rdriveI)), rDelay = get (rdelayI) * 0.001f;
        const float att = juce::jmax (0.0001f, get (attackI) * 0.001f), len = get (lengthI) * 0.001f, rel = juce::jmax (0.001f, get (releaseI) * 0.001f);
        const float velAmt = get (velI), outG = juce::Decibels::decibelsToGain (get (outI));
        const bool useOs = get (osI) > 0.5f, cut = get (noteendI) > 0.5f, rFollow = get (rfollowI) > 0.5f;
        tone.set (get (toneI), 0.707f, sr); rtone.set (get (rtoneI), 0.9f, sr);
        clickF.set (get (ctoneI), 0.8f, sr); clickHp.set (300.0f, sr);
        hp1.set (get (lowcutI), sr); hp2.set (get (lowcutI), sr); tilt.set (get (tiltI), sr); lim.set (sr, get (ceilI));
        const float dcMul = std::exp (-1.0f / (0.003f * sr));   // retrigger declick: the jump fades out in ~3 ms

        auto* L = buf.getWritePointer (0); auto* R = buf.getWritePointer (1);
        for (int start = 0; start < n; start += chunk)
        {
            const int m = juce::jmin (chunk, n - start);
            auto* x = pre.getWritePointer (0);
            for (int i = 0; i < m; ++i)
            {
                const int s = start + i;
                retrig[(size_t) i] = 0;
                while (it != midi.cend() && (*it).samplePosition <= s)
                {
                    const auto msg = (*it).getMessage();
                    if (msg.isNoteOn())
                    {
                        if (active) retrig[(size_t) i] = 1;
                        active = true; t = 0; phase = 0; rphase = 0; relAt = -1.0f; note = msg.getNoteNumber();
                        velG = 1.0f - velAmt + velAmt * msg.getFloatVelocity();
                        fEnd = tailHz (note);
                        hits.fetch_add (1, std::memory_order_relaxed);
                        lastNote.store (note, std::memory_order_relaxed);
                        own.trigger (s, note, msg.getFloatVelocity());
                    }
                    else if (msg.isNoteOff())
                    {
                        if (cut && active && msg.getNoteNumber() == note && t > 0.002f && relAt < 0.0f) relAt = t;   // Note End = CUT
                        own.release (s, msg.getNoteNumber());
                    }
                    else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { active = false; own.release (s, -1); }
                    ++it;
                }
                if (! active) { x[i] = 0; env[(size_t) i] = 0; rum[(size_t) i] = 0; continue; }
                const float u = juce::jlimit (0.0f, 1.0f, t / juce::jmax (0.001f, pDec));
                const float e = std::pow (1.0f - u, pCurve);
                const float f = fEnd + (juce::jmax (pStart, fEnd) - fEnd) * e;
                phase += f / sr; if (phase >= 1) phase -= 1;
                const float ph = juce::MathConstants<float>::twoPi * phase;
                const float body = std::sin (ph + shape * 2.5f * std::sin (ph));
                const float click = clickHp.hp (clickF.lp (noise())) * std::exp (-t / cDec) * 2.5f;
                x[i] = (body + click * punch) * velG;
                // amp env: attack -> hold (Length, or until the note ends with CUT) -> release; continuous at every turn
                const float holdEnd = relAt >= 0.0f ? juce::jmin (len, relAt) : len;
                const float envHold = holdEnd < att ? holdEnd / att : 1.0f;
                const float a = t < holdEnd ? (t < att ? t / att : 1.0f) : envHold * std::exp (-(t - holdEnd) / rel * 3.0f);
                env[(size_t) i] = a;
                // rumble: delayed, distorted low sine following the tail (CUT fades it with the note too)
                rphase += fEnd / sr; if (rphase >= 1) rphase -= 1;
                const float rt = t - rDelay;
                float renv = rt > 0 ? (1 - std::exp (-rt / 0.015f)) * std::exp (-rt / rDec) : 0.0f;
                if (relAt >= 0.0f && t > relAt) renv *= std::exp (-(t - relAt) / rel * 3.0f);
                if (rFollow && t > holdEnd) renv *= std::exp (-(t - holdEnd) / rel * 3.0f);   // no ringing after the kick is over
                rum[(size_t) i] = rumbleL * renv * velG * rtone.lp (softClip (std::sin (juce::MathConstants<float>::twoPi * rphase) * rDrive) * 0.8f);
                t += 1.0f / sr;
                const bool bodyDone = t > holdEnd + rel * 6 + 0.05f;
                const bool rumbleDone = rumbleL <= 0.0f || rFollow || t > rDelay + rDec * 6 || (relAt >= 0.0f && t > relAt + rel * 6 + 0.05f);
                if (bodyDone && rumbleDone) active = false;
            }
            // distortion (optionally 2x oversampled)
            if (useOs && os != nullptr)
            {
                juce::dsp::AudioBlock<float> blk (pre); auto sb = blk.getSubBlock (0, (size_t) m);
                auto up = os->processSamplesUp (sb);
                auto* uu = up.getChannelPointer (0);
                for (size_t i = 0; i < up.getNumSamples(); ++i) uu[i] = uu[i] * (1 - dmix) + distort (dtype, uu[i], drive) * dmix;
                os->processSamplesDown (sb);
            }
            else
                for (int i = 0; i < m; ++i) x[i] = x[i] * (1 - dmix) + distort (dtype, x[i], drive) * dmix;

            for (int i = 0; i < m; ++i)
            {
                float y = tone.lp (x[i]) * env[(size_t) i] + rum[(size_t) i];
                y = hp2.hp (hp1.hp (y)); y = tilt.process (y) * outSm.next (outG) * 0.9f;
                if (retrig[(size_t) i]) declick += lastOut - y;   // a new kick starts where the old one was: no click
                y += declick;
                declick *= dcMul;
                lastOut = y;
                L[start + i] = y; R[start + i] = y;
            }
        }
        own.gate = cut;
        own.render (buf);                       // OWN SOUND: layer or replace, before the limiter
        for (int i = 0; i < n; ++i) { float l = L[i], r = R[i]; lim.process (l, r); L[i] = l; R[i] = r; }
    }

    Sonic::OwnSound own;                         // { mono = true, root C5 }: one hit per note, a new hit cuts the last
    std::atomic<int> hits { 0 }, lastNote { 55 };   // for the editor's picture

private:
    float noise() { seed = seed * 1664525u + 1013904223u; return (float) (int32_t) seed * (1.0f / 2147483648.0f); }
    static constexpr int chunk = 256;
    float sr = 44100, t = 0, phase = 0, rphase = 0, velG = 1, relAt = -1, fEnd = 49, declick = 0, lastOut = 0;
    int note = 55; bool active = false; uint32_t seed = 12345;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::AudioBuffer<float> pre; std::vector<float> env, rum; std::vector<uint8_t> retrig;
    OnePole hp1, hp2, clickHp; SVF tone, rtone, clickF; Tilt tilt; Limiter lim; Smooth outSm;
};

// ---------------------------------------------------------------- editor
class KickEditor : public Editor
{
public:
    explicit KickEditor (KickProcessor& p)
      : Editor (p, BOY_NAME, "kick  |  play the key", 880, 500), kp (p)
    {
        addButton ("LOAD SAMPLE", [this] { sampleClick(); },
                   [this] { return kp.own.hasSound() ? "OWN SOUND: " + kp.own.name() + "  |  click: another sample / remove  |  level + mode on ADVANCED"
                                                     : juce::String ("OWN SOUND: load your own kick / top sample (or drop a WAV on the plugin)"); });
        init();
    }

protected:
    int heroHeight() const override { return 150; }
    bool wantsFiles() const override { return true; }
    void filesDroppedOnPlugin (const juce::File& f) override { load (f); }
    bool animating() const override { return juce::Time::getMillisecondCounter() - lastHitAt < 400; }
    void tick() override
    {
        const int h = kp.hits.load();
        if (h != seenHits) { seenHits = h; lastHitAt = juce::Time::getMillisecondCounter(); }
        const auto t = kp.own.hasSound() ? "SAMPLE: " + kp.own.name() : juce::String ("LOAD SAMPLE");
        if (buttons[0].text != t) { buttons[0].text = t; repaint(); }
    }

    // the pitch curve: frequency (log) over time (square-root axis: the fast start stays readable)
    float span() const { return juce::jlimit (0.15f, 1.6f, (real ("length") + real ("release") * 2.0f) * 0.001f); }
    float xOf (float sec, juce::Rectangle<float> r) const { return r.getX() + r.getWidth() * std::sqrt (juce::jlimit (0.0f, 1.0f, sec / span())); }
    float yOf (float hz, juce::Rectangle<float> r) const
    { return r.getBottom() - r.getHeight() * (std::log (juce::jlimit (25.0f, 4000.0f, hz) / 25.0f) / std::log (4000.0f / 25.0f)); }

    void paintHero (juce::Graphics& g, juce::Rectangle<float> area) override
    {
        auto r = area.reduced (14.0f, 12.0f);
        const float fEnd = kp.tailHz (kp.lastNote.load()), pStart = real ("pstart"), pDec = real ("pdecay") * 0.001f, curve = real ("pcurve");
        const float len = real ("length") * 0.001f, rel = juce::jmax (0.001f, real ("release") * 0.001f), att = juce::jmax (0.0001f, real ("attack") * 0.001f);
        // grid
        g.setFont (monoFont (16.0f));
        for (float hz : { 50.0f, 100.0f, 500.0f, 1000.0f, 2000.0f })
        {
            const float y = yOf (hz, r);
            g.setColour (col::line.withAlpha (0.6f));
            g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
            g.setColour (col::ink);
            g.drawText (hz >= 1000 ? juce::String (hz / 1000.0f, 0) + "k" : juce::String ((int) hz), (int) r.getX() - 10, (int) y - 16, 44, 16, juce::Justification::left);
        }
        // amplitude envelope as a soft hill
        juce::Path amp;
        amp.startNewSubPath (r.getX(), r.getBottom());
        for (int k = 0; k <= 120; ++k)
        {
            const float sec = span() * (float) (k * k) / (120.0f * 120.0f);
            const float a = sec < len ? (sec < att ? sec / att : 1.0f) : std::exp (-(sec - len) / rel * 3.0f);
            amp.lineTo (xOf (sec, r), r.getBottom() - r.getHeight() * 0.85f * a);
        }
        amp.lineTo (r.getRight(), r.getBottom());
        amp.closeSubPath();
        g.setColour (col::accent.withAlpha (0.35f));
        g.fillPath (amp);
        // pitch curve
        juce::Path pc;
        float prevY = 0;
        for (int k = 0; k <= 44; ++k)   // a staircase: the pitch curve in pixels
        {
            const float sec = span() * (float) (k * k) / (44.0f * 44.0f);
            const float u = juce::jlimit (0.0f, 1.0f, sec / juce::jmax (0.001f, pDec));
            const float f = fEnd + (juce::jmax (pStart, fEnd) - fEnd) * std::pow (1.0f - u, curve);
            const float px = xOf (sec, r), py = std::round (yOf (f, r) / 4.0f) * 4.0f;
            if (k == 0) pc.startNewSubPath (px, py); else { pc.lineTo (px, prevY); pc.lineTo (px, py); }
            prevY = py;
        }
        pc.lineTo (r.getRight(), prevY);
        g.setColour (col::ink);
        g.strokePath (pc, juce::PathStrokeType (3.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
        // the handle sits on the curve halfway through the drop; flashes when a kick fires
        const float hf = fEnd + (juce::jmax (pStart, fEnd) - fEnd) * std::pow (0.5f, curve);
        const juce::Point<float> hnd (xOf (pDec * 0.5f, r), yOf (hf, r));
        const float fl = juce::jlimit (0.0f, 1.0f, 1.0f - (float) (juce::Time::getMillisecondCounter() - lastHitAt) / 400.0f);
        if (fl > 0) { g.setColour (col::accent.withAlpha (0.6f * fl)); g.fillRect (hnd.x - 15, hnd.y - 15, 30.0f, 30.0f); }
        g.setColour (col::ink);
        g.fillRect (hnd.x - 8, hnd.y - 8, 16.0f, 16.0f);
        g.setColour (col::inkInverse);
        g.fillRect (hnd.x - 3, hnd.y - 3, 6.0f, 6.0f);
        // tail note
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int tn = kp.get (ktrackI) > 0.5f ? foldBass (kp.lastNote.load()) : 36;
        heroLabel (g, monoFont (24.0f), area.reduced (14, 8), juce::String ("tail ") + names[tn % 12] + juce::String (tn / 12 - 1) + "  "
                   + juce::String (fEnd, 1) + " Hz", col::text, juce::Justification::topRight);
        heroLabel (g, monoFont (20.0f), area.reduced (14, 8), "drag: Pitch Start (up) / Pitch Decay (right)", col::ink, juce::Justification::bottomRight);
    }
    bool heroDown (juce::Point<float> p, const juce::MouseEvent&) override { editBegin ({ "pstart", "pdecay" }); dragTo (p); return true; }
    void heroDrag (juce::Point<float> p, const juce::MouseEvent&) override { dragTo (p); }
    void dragTo (juce::Point<float> p)
    {
        const auto r = heroArea.reduced (14.0f, 12.0f);
        const float xn = juce::jlimit (0.0f, 1.0f, (p.x - r.getX()) / r.getWidth());
        const float yn = juce::jlimit (0.0f, 1.0f, (r.getBottom() - p.y) / r.getHeight());
        // the dot is the curve at half the decay: x -> Pitch Decay, y -> the Pitch Start that puts the curve there
        editSet ("pdecay", juce::jlimit (5.0f, 400.0f, 2.0f * xn * xn * span() * 1000.0f));
        const float fEnd = kp.tailHz (kp.lastNote.load()), hf = 25.0f * std::pow (4000.0f / 25.0f, yn);
        editSet ("pstart", juce::jlimit (80.0f, 3000.0f, fEnd + juce::jmax (0.0f, hf - fEnd) / std::pow (0.5f, real ("pcurve"))));
    }
    void heroUp() override { editEnd(); }
    juce::String heroHint (juce::Point<float>) override { return "the kick's pitch: drag the dot - up = higher start, right = slower drop"; }

    void paintSegmentIcon (juce::Graphics& g, const Spec& s, int k, juce::Rectangle<float> r, juce::Colour ink) override
    {
        if (s.icons != distortionIcons) return;
        juce::Path p;   // the distortion's transfer curve
        for (int i = 0; i <= 24; ++i)
        {
            const float xin = -1.0f + 2.0f * (float) i / 24.0f;
            const float y = juce::jlimit (-1.0f, 1.0f, distort (k, xin, 2.5f));
            const juce::Point<float> pt (r.getX() + r.getWidth() * (float) i / 24.0f, r.getCentreY() - y * r.getHeight() * 0.5f);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (ink);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }

private:
    void sampleClick()
    {
        if (! kp.own.hasSound()) { choose(); return; }
        juce::PopupMenu m;
        m.addSectionHeader ("OWN SOUND: " + kp.own.name());
        m.addItem (1, "Load another sample...");
        m.addItem (2, "Remove the sample");
        juce::Component::SafePointer<KickEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe] (int r)
        {
            if (safe == nullptr) return;
            if (r == 1) safe->choose();
            if (r == 2) { safe->kp.own.clear(); safe->status ("sample removed: the plugin plays its own kick again"); }
        });
    }
    void choose()
    {
        chooser = std::make_unique<juce::FileChooser> ("Load your own sound", juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                       Sonic::OwnSound::fileTypes());
        juce::Component::SafePointer<KickEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe] (const juce::FileChooser& fc)
        { if (safe != nullptr && fc.getResult().existsAsFile()) safe->load (fc.getResult()); });
    }
    void load (const juce::File& f)
    {
        const auto note = kp.own.loadFile (f);
        status (kp.own.hasSound() && kp.own.name() == f.getFileNameWithoutExtension()
                    ? "OWN SOUND: " + kp.own.name() + " loaded" + (note.isEmpty() ? juce::String() : "  |  " + note) + "  |  Sample Level / Mode on ADVANCED"
                    : note);
    }
    KickProcessor& kp;
    std::unique_ptr<juce::FileChooser> chooser;
    int seenHits = 0;
    juce::uint32 lastHitAt = 0;
};

juce::AudioProcessorEditor* KickProcessor::createEditor() { return new KickEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KickProcessor(); }
