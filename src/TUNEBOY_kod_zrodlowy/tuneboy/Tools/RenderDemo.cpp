// TUNEBOY offline test: TuneboyRender demo.wav gui.png -> RESULT: PASS
#include "../Source/TuneBoy.cpp"
#include "BoyTest.h"

using namespace juce;

static int tuneChecks (boy::BaseProcessor& base)
{
    int fails = 0;
    auto& tp = dynamic_cast<TuneProcessor&> (base);
    std::vector<Note> a (maxNotes), b (maxNotes);
    auto melody = [] (const std::vector<Note>& v, int n, int bar) { std::vector<int> out; for (int i = 0; i < n; ++i) if (! v[(size_t) i].chord && (int) (v[(size_t) i].start / 4.0) == bar) out.push_back (v[(size_t) i].pitch * 1000 + (int) (v[(size_t) i].start * 4.0)); return out; };
    // 1) every rhythm makes a melody
    {
        int ok = 0;
        for (int r = 0; r < numRhythms; ++r) { tp.applyPreset (0); tp.setParam ("rhythm", (float) r); ok += tp.generateNow (a.data()) > 12 ? 1 : 0; }
        const bool pass = ok == numRhythms;
        std::printf ("  [%s] rhythms: %d/%d make a melody\n", pass ? "OK" : "FAIL", ok, numRhythms); fails += pass ? 0 : 1;
    }
    // 2) VARIATION changes some notes but keeps the first one (same feel)
    {
        tp.applyPreset (0);
        const int n1 = tp.generateNow (a.data());
        tp.setParam ("variation", 37.0f);
        const int n2 = tp.generateNow (b.data());
        int diff = 0; for (int bar = 0; bar < 8; ++bar) diff += melody (a, n1, bar) != melody (b, n2, bar) ? 1 : 0;
        const bool ok = diff > 0 && a[0].pitch == b[0].pitch && a[0].start == b[0].start;
        std::printf ("  [%s] VARIATION: %d of 8 bars changed, first note kept\n", ok ? "OK" : "FAIL", diff); fails += ok ? 0 : 1;
        tp.setParam ("variation", 0.0f);
    }
    // 3) a locked bar keeps its notes through NEW MELODY (seed change); the others change
    {
        tp.applyPreset (0);
        const int n1 = tp.generateNow (a.data());
        tp.toggleLock (2);
        tp.setParam ("seed", 4321.0f);
        const int n2 = tp.generateNow (b.data());
        int others = 0; for (int bar : { 0, 1, 3, 4 }) others += melody (a, n1, bar) != melody (b, n2, bar) ? 1 : 0;
        const bool ok = melody (a, n1, 2) == melody (b, n2, 2) && others > 0 && tp.lockOf (2) > 0;
        // the lock survives a state round trip
        MemoryBlock st; tp.getStateInformation (st);
        TuneProcessor q; q.setStateInformation (st.getData(), (int) st.getSize());
        const bool kept = q.lockOf (2) == tp.lockOf (2);
        tp.toggleLock (2);
        std::printf ("  [%s] bar lock: bar 3 kept, %d other bars changed, state %s\n", ok && kept ? "OK" : "FAIL", others, kept ? "kept" : "LOST");
        fails += ok && kept ? 0 : 1;
    }
    // 4) a change while it plays starts at the next bar: no new note before the bar line
    {
        tp.applyPreset (0);
        const double sr = 44100; const int bs = 256;
        tp.setPlayConfigDetails (0, 2, sr, bs); tp.prepareToPlay (sr, bs);
        AudioBuffer<float> buf (2, bs);
        const double spb = sr * 60.0 / 150.0;   // samples per beat at 150 BPM (no playhead -> 150)
        const int changeAt = (int) (spb * 4.5), barLine = (int) (spb * 8.0);
        int earlyOn = 0;
        const int nOld = tp.generateNow (a.data());   // the melody before the change (seed 101)
        for (int pos = 0; pos < (int) (spb * 9); pos += bs)
        {
            MidiBuffer m;
            if (pos == 0) m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 0);
            if (changeAt >= pos && changeAt < pos + bs) tp.setParam ("seed", 999.0f);
            tp.processBlock (buf, m);
            for (const auto ev : m)
                if (ev.getMessage().isNoteOn() && pos >= changeAt && pos + ev.samplePosition < barLine - 2)
                {   // a note-on between the change and the bar line must still belong to the old melody (seed 101)
                    bool old = false; const double beat = (pos + ev.samplePosition) / spb;
                    for (int i = 0; i < nOld; ++i) if (std::abs (a[(size_t) i].start - beat) < 0.02 && a[(size_t) i].pitch == ev.getMessage().getNoteNumber()) old = true;
                    earlyOn += old ? 0 : 1;
                }
        }
        const bool ok = earlyOn == 0;
        std::printf ("  [%s] change while playing waits for the bar line (%d early notes)\n", ok ? "OK" : "FAIL", earlyOn); fails += ok ? 0 : 1;
    }
    // 5) SINGLE NOTE: a held key sounds right away as a plain note (no generated melody, no MIDI out), a chord is polyphonic,
    //    and after the key is released it fades out
    {
        tp.applyPreset (0); tp.setParam ("mode", 1.0f); tp.setParam ("reverb", 0.0f);
        const double sr = 44100; const int bs = 256;
        tp.setPlayConfigDetails (0, 2, sr, bs); tp.prepareToPlay (sr, bs);
        AudioBuffer<float> buf (2, bs);
        float held = 0.0f, after = 0.0f; int midiOut = 0;
        for (int blk = 0; blk < 400; ++blk)
        {
            MidiBuffer m;
            if (blk == 0) { m.addEvent (MidiMessage::noteOn (1, 57, (uint8) 100), 0); m.addEvent (MidiMessage::noteOn (1, 64, (uint8) 100), 0); }
            if (blk == 200) { m.addEvent (MidiMessage::noteOff (1, 57), 0); m.addEvent (MidiMessage::noteOff (1, 64), 0); }
            tp.processBlock (buf, m);
            midiOut += m.getNumEvents();
            const float pk = buf.getMagnitude (0, bs);
            if (blk >= 50 && blk < 200) held = jmax (held, pk);
            if (blk >= 380) after = jmax (after, pk);
        }
        const bool ok = held > 0.02f && after < 0.001f && midiOut == 0;
        std::printf ("  [%s] single note: level while held %.3f, after release %.5f, %d MIDI events out\n", ok ? "OK" : "FAIL", held, after, midiOut);
        fails += ok ? 0 : 1;
        tp.setParam ("mode", 0.0f);
    }
    return fails;
}

int main (int argc, char** argv) { return boytest::testMain (argc, argv, "tune", tuneChecks); }
