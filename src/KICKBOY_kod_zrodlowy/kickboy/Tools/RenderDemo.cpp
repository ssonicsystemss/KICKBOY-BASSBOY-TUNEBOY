// KICKBOY offline test: KickboyRender demo.wav gui.png -> RESULT: PASS
#include "../Source/KickBoy.cpp"
#include "OwnSoundTest.h"
#include "BoyTest.h"

using namespace juce;

// render one or two notes into a mono buffer
static std::vector<float> play (KickProcessor& p, double seconds, std::vector<std::pair<double, int>> notes, double offAfter = 10.0)
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
        for (auto& [at, note] : notes)
        {
            const int s = (int) (at * sr), e = (int) ((at + offAfter) * sr);
            if (s >= pos && s < pos + n) m.addEvent (MidiMessage::noteOn (1, note, (uint8) 100), s - pos);
            if (e >= pos && e < pos + n) m.addEvent (MidiMessage::noteOff (1, note), e - pos);
        }
        AudioBuffer<float> v (b.getArrayOfWritePointers(), 2, n);
        p.processBlock (v, m);
        for (int i = 0; i < n; ++i) out[(size_t) (pos + i)] = v.getSample (0, i);
    }
    return out;
}

static int kickChecks (boy::BaseProcessor& base)
{
    int fails = 0;
    auto& kp = dynamic_cast<KickProcessor&> (base);
    // 1) play the key: G (any octave) = a G1 kick, 49 Hz (Clean Sub, almost a sine)
    for (int note : { 55, 67, 43 })
    {
        kp.applyPreset (19); kp.setParam ("punch", 0.0f);
        const auto y = play (kp, 0.6, { { 0.0, note } });
        int ups = 0, first = -1, last = -1;
        for (int i = (int) (0.25 * 44100); i < (int) (0.5 * 44100); ++i)
            if (y[(size_t) i - 1] < 0.0f && y[(size_t) i] >= 0.0f) { if (first < 0) first = i; last = i; ++ups; }
        const double hz = ups > 1 ? (ups - 1) * 44100.0 / (last - first) : 0.0;
        const bool ok = std::abs (hz - 49.0) < 2.0;
        std::printf ("  [%s] note %d -> tail %.2f Hz (G1 = 49.0)\n", ok ? "OK" : "FAIL", note, hz); fails += ok ? 0 : 1;
    }
    // 2) a retrigger in the middle of the tail must not click: the jump at the new note is no bigger than the wave's
    //    own steepest step before it
    {
        kp.applyPreset (19); kp.setParam ("punch", 0.0f); kp.setParam ("os", 0.0f);
        const double atSec = (6615.0 + 37.0) / 44100.0;
        const int at = (int) (atSec * 44100.0);   // exactly the sample play() puts the note on
        const auto y = play (kp, 0.3, { { 0.0, 55 }, { atSec, 55 } });
        float steep = 0.0f;
        for (int i = (int) (0.05 * 44100); i < at - 4; ++i) steep = jmax (steep, std::abs (y[(size_t) i] - y[(size_t) i - 1]));
        // the step AT the new note (after it the new kick's own fast start begins, which is fine)
        const float jump = std::abs (y[(size_t) at] - y[(size_t) at - 1]), before = std::abs (y[(size_t) at - 1]);
        const bool ok = jump <= steep * 1.5f + 1.0e-4f;
        std::printf ("  [%s] retrigger: step at the new note %.4f (old wave there %.3f, its steepest step %.4f) - no click\n",
                     ok ? "OK" : "FAIL", jump, before, steep); fails += ok ? 0 : 1;
    }
    // 3) Note End = CUT: a 100 ms note is (almost) silent 200 ms later; FULL keeps ringing
    {
        float rest[2] {};
        for (int mode = 0; mode < 2; ++mode)
        {
            kp.applyPreset (2); kp.setParam ("noteend", (float) mode); kp.setParam ("release", 30.0f);
            const auto y = play (kp, 0.5, { { 0.0, 55 } }, 0.1);
            for (int i = (int) (0.3 * 44100); i < (int) (0.45 * 44100); ++i) rest[mode] = jmax (rest[mode], std::abs (y[(size_t) i]));
        }
        const bool ok = rest[1] < 0.01f && rest[0] > 0.1f;
        std::printf ("  [%s] Note End: FULL rings %.3f, CUT %.4f after a 100 ms note\n", ok ? "OK" : "FAIL", rest[0], rest[1]); fails += ok ? 0 : 1;
        kp.setParam ("noteend", 0.0f);
    }
    // 4) OWN SOUND (shared test: test tone in REPLACE, state round trip, removal)
    if (! Sonic::ownSoundSelfTest<KickProcessor> (44100.0, 256, 55, 1.2f)) ++fails;
    return fails;
}

int main (int argc, char** argv) { return boytest::testMain (argc, argv, "kick", kickChecks); }
