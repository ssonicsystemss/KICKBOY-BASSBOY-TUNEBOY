// BASSBOY offline test: BassboyRender demo.wav gui.png -> RESULT: PASS
#include "../Source/BassBoy.cpp"
#include "BoyTest.h"

using namespace juce;

// hold some keys with the transport stopped (free-running pattern), return the left channel
static std::vector<float> hold (BassProcessor& p, double seconds, std::initializer_list<int> notes)
{
    const double sr = 44100; const int bs = 256;
    p.setPlayConfigDetails (0, 2, sr, bs); p.prepareToPlay (sr, bs);
    const int total = (int) (seconds * sr);
    std::vector<float> out ((size_t) total);
    AudioBuffer<float> b (2, bs);
    for (int pos = 0; pos < total; pos += bs)
    {
        const int n = jmin (bs, total - pos);
        MidiBuffer m;
        if (pos == 0) for (int nt : notes) m.addEvent (MidiMessage::noteOn (1, nt, (uint8) 100), 0);
        AudioBuffer<float> v (b.getArrayOfWritePointers(), 2, n);
        p.processBlock (v, m);
        for (int i = 0; i < n; ++i) out[(size_t) (pos + i)] = v.getSample (0, i);
    }
    return out;
}
static double rmsOf (const std::vector<float>& y) { double s = 0; for (float v : y) s += (double) v * v; return std::sqrt (s / (double) jmax ((size_t) 1, y.size())); }

static int bassChecks (boy::BaseProcessor& base)
{
    int fails = 0;
    auto& bp = dynamic_cast<BassProcessor&> (base);
    // 1) every pattern (14 + Custom) plays clean, audible audio
    {
        int ok = 0;
        for (int k = 0; k < (int) patternNames.size(); ++k)
        {
            bp.applyPreset (0); bp.setParam ("pattern", (float) k);
            const auto y = hold (bp, 1.7, { 60 });
            bool good = rmsOf (y) > 0.01;
            for (float v : y) good = good && std::isfinite (v) && std::abs (v) <= 1.0001f;
            ok += good ? 1 : 0;
        }
        const bool pass = ok == patternNames.size();
        std::printf ("  [%s] patterns: %d/%d play clean audio\n", pass ? "OK" : "FAIL", ok, patternNames.size()); fails += pass ? 0 : 1;
    }
    // 2) Custom with no step = silence; one step = sound
    {
        bp.applyPreset (0); bp.setParam ("pattern", (float) numFixedPatterns);
        bp.setParam ("custom", 0.0f);   const double none = rmsOf (hold (bp, 1.0, { 60 }));
        bp.setParam ("custom", 1.0f);   const double one = rmsOf (hold (bp, 1.7, { 60 }));
        const bool ok = none < 1.0e-4 && one > 0.005;
        std::printf ("  [%s] custom steps: none = %.5f, one step = %.4f\n", ok ? "OK" : "FAIL", none, one); fails += ok ? 0 : 1;
    }
    // 3) play the key: G (any octave) = G1 49 Hz (pure sine, no drive / rise / filter sweep)
    for (int note : { 67, 43 })
    {
        bp.applyPreset (2);
        for (auto [id, v] : std::initializer_list<std::pair<const char*, float>> { { "mix", 1.0f }, { "drive", 0.0f }, { "rise", 0.0f }, { "gap", 0.0f },
                { "swell", 0.3f }, { "pattern", 4.0f }, { "cutoff", 12000.0f }, { "fenv", 0.0f }, { "os", 0.0f }, { "sync", 1.0f }, { "bpm", 120.0f } })
            bp.setParam (id, v);
        const auto y = hold (bp, 1.6, { note });
        int ups = 0, first = -1, last = -1;
        for (int i = (int) (0.3 * 44100); i < (int) (1.4 * 44100); ++i)
            if (y[(size_t) i - 1] < 0.0f && y[(size_t) i] >= 0.0f) { if (first < 0) first = i; last = i; ++ups; }
        const double hz = ups > 1 ? (ups - 1) * 44100.0 / (last - first) : 0.0;
        const bool ok = std::abs (hz - 49.0) < 1.5;
        std::printf ("  [%s] note %d -> %.2f Hz (G1 = 49.0, same map as KICKBOY)\n", ok ? "OK" : "FAIL", note, hz); fails += ok ? 0 : 1;
    }
    // 4) Chord mode: three keys sound (louder / fuller than one, still clean)
    {
        bp.applyPreset (0); bp.setParam ("voices", 1.0f);
        const auto one = hold (bp, 1.7, { 60 });
        const auto three = hold (bp, 1.7, { 60, 63, 67 });
        bool clean = true; for (float v : three) clean = clean && std::isfinite (v) && std::abs (v) <= 1.0001f;
        const bool ok = clean && rmsOf (three) > rmsOf (one) * 0.8 && rmsOf (three) > 0.01;
        std::printf ("  [%s] chord mode: 1 key rms %.3f, 3 keys rms %.3f\n", ok ? "OK" : "FAIL", rmsOf (one), rmsOf (three)); fails += ok ? 0 : 1;
    }
    return fails;
}

int main (int argc, char** argv) { return boytest::testMain (argc, argv, "bass", bassChecks); }
