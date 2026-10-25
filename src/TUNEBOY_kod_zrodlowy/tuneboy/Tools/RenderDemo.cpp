// TUNEBOY offline test: TuneboyRender demo.wav gui.png -> RESULT: PASS
#include "../Source/TuneBoy.cpp"
#include "OwnSoundTest.h"
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
    // 6) your own note edits: your notes replace the generated ones, they survive NEW MELODY, the state and a note-on in the loop
    {
        tp.applyPreset (0); tp.edits.clearAll();
        const int n1 = tp.generateNow (a.data());
        const EditNote e[2] = { { 0, 4, 72, 100 }, { 8, 2, 75, 90 } };
        tp.edits.set (2, e, 2);
        const int n2 = tp.generateNow (b.data());
        const std::vector<int> want { 72000 + 32, 75000 + 40 };
        const bool mine = melody (b, n2, 2) == want;
        tp.setParam ("seed", 4321.0f);
        std::vector<Note> c (maxNotes);
        const int n3 = tp.generateNow (c.data());
        int others = 0; for (int bar : { 0, 1, 3 }) others += melody (a, n1, bar) != melody (c, n3, bar) ? 1 : 0;
        const bool kept = melody (c, n3, 2) == want && others > 0;
        tp.saveEdits();
        MemoryBlock st; tp.getStateInformation (st);
        TuneProcessor q; q.setStateInformation (st.getData(), (int) st.getSize());
        std::vector<Note> d (maxNotes);
        const int n4 = q.generateNow (d.data());
        const bool roundTrip = melody (d, n4, 2) == want;
        tp.edits.clearAll(); tp.saveEdits(); tp.setParam ("seed", 101.0f);
        const int n5 = tp.generateNow (a.data());
        const bool back = n5 == n1;
        std::printf ("  [%s] note edits: mine %s, survive NEW MELODY %s (%d other bars changed), state %s, reset %s\n", mine && kept && roundTrip && back ? "OK" : "FAIL",
                     mine ? "yes" : "NO", kept ? "yes" : "NO", others, roundTrip ? "kept" : "LOST", back ? "ok" : "BROKEN");
        fails += mine && kept && roundTrip && back ? 0 : 1;
    }
    // 7) progressions: all 16 make chords; Custom with 1-6-3-7 plays exactly like the first one
    {
        int ok = 0;
        for (int pr = 0; pr < numProgs; ++pr) { tp.applyPreset (0); tp.setParam ("prog", (float) pr); ok += tp.generateNow (a.data()) > 12 ? 1 : 0; }
        auto chordsOf = [] (const std::vector<Note>& v, int n) { std::vector<int> out; for (int i = 0; i < n; ++i) if (v[(size_t) i].chord) out.push_back (v[(size_t) i].pitch + (int) v[(size_t) i].start * 100); return out; };
        tp.applyPreset (0);
        const int n1 = tp.generateNow (a.data());
        tp.setParam ("prog", (float) customProg);
        tp.setParam ("c1", 1.0f); tp.setParam ("c2", 6.0f); tp.setParam ("c3", 3.0f); tp.setParam ("c4", 7.0f);
        const int n2 = tp.generateNow (b.data());
        const bool same = chordsOf (a, n1) == chordsOf (b, n2);
        tp.setParam ("c1", 4.0f);
        const int n3 = tp.generateNow (b.data());
        const bool differs = chordsOf (a, n1) != chordsOf (b, n3);
        const bool pass = ok == numProgs && same && differs;
        std::printf ("  [%s] progressions: %d/%d make music, Custom 1-6-3-7 = the first one: %s, changing a chord changes it: %s\n", pass ? "OK" : "FAIL", ok, numProgs, same ? "yes" : "NO", differs ? "yes" : "NO");
        fails += pass ? 0 : 1;
    }
    // 8) sound bank + random sound: only the ten sound params move, frozen ones stay, the melody stays
    {
        tp.applyPreset (0);
        const int n1 = tp.generateNow (a.data());
        tp.applySound (1);   // Raw Screech: cutoff 9500
        const bool bank = std::abs (tp.get (cutoffI) - 9500.0f) < 1.0f && tp.soundName() == "Raw Screech";
        tp.toggleFrozen ("cutoff");
        const float c0 = tp.get (cutoffI);
        bool moved = false, inRange = true;
        for (int k = 0; k < 20; ++k)
        {
            tp.randomSound();
            moved = moved || std::abs (tp.get (detuneI) - 35.0f) > 0.01f;
            inRange = inRange && tp.get (cutoffI) == c0 && tp.get (detuneI) >= 0.0f && tp.get (detuneI) <= 60.0f && tp.get (attackI) >= 0.0f && tp.get (attackI) <= 300.0f;
        }
        tp.toggleFrozen ("cutoff");
        const int n2 = tp.generateNow (b.data());
        const bool same = n1 == n2 && a[0].pitch == b[0].pitch;
        const bool pass = bank && moved && inRange && same;
        std::printf ("  [%s] sounds: bank applies %s, random moves %s, frozen + ranges kept %s, melody untouched %s\n", pass ? "OK" : "FAIL", bank ? "yes" : "NO", moved ? "yes" : "NO", inRange ? "yes" : "NO", same ? "yes" : "NO");
        fails += pass ? 0 : 1;
    }
    // 10) 2.2: scales / progressions / rhythms, chord styles, PUMP, DRAW helpers, edit undo
    {
        int made = 0, expected = 0;
        for (int sc = 0; sc < numScales; ++sc) { tp.applyPreset (0); tp.setParam ("scale", (float) sc); ++expected; made += tp.generateNow (a.data()) > 12 ? 1 : 0; }
        for (int pr = customProg + 1; pr < numAllProgs; ++pr) { tp.applyPreset (0); tp.setParam ("prog", (float) pr); ++expected; made += tp.generateNow (a.data()) > 12 ? 1 : 0; }
        const bool lists = made == expected;
        // chord styles: notes of chords in bar 1 = 3 (held), 12 (offbeat stabs), 16 (arp), 24 (gate)
        const int want[4] = { 3, 12, 16, 24 };
        bool styles = true; int got[4] {};
        for (int st = 0; st < 4; ++st)
        {
            tp.applyPreset (0); tp.setParam ("chordstyle", (float) st);
            const int n = tp.generateNow (a.data());
            for (int i = 0; i < n; ++i) if (a[(size_t) i].chord && a[(size_t) i].start < 4.0) ++got[st];
            styles = styles && got[st] == want[st];
        }
        tp.setParam ("chordstyle", 0.0f);
        // PUMP: with a key held the sound ducks on every beat, so it is quieter than without, and stays clean
        auto rms = [&] (float pump)
        {
            tp.applyPreset (0); tp.setParam ("reverb", 0.0f); tp.setParam ("pump", pump);
            const double sr = 44100; const int bs = 256;
            tp.setPlayConfigDetails (0, 2, sr, bs); tp.prepareToPlay (sr, bs);
            AudioBuffer<float> buf (2, bs);
            double sum = 0; long cnt = 0; bool clean = true;
            for (int blk = 0; blk < 700; ++blk)
            {
                MidiBuffer m;
                if (blk == 0) m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 0);
                tp.processBlock (buf, m);
                if (blk >= 40) for (int i = 0; i < bs; ++i) { const float v = buf.getSample (0, i); if (! std::isfinite (v)) clean = false; sum += v * v; ++cnt; }
            }
            return clean ? std::sqrt (sum / (double) cnt) : -1.0;
        };
        const double r0 = rms (0.0f), r1 = rms (1.0f);
        const bool pump = r0 > 0.001 && r1 > 0.0 && r1 < r0 * 0.97;
        tp.setParam ("pump", 0.0f);
        // DRAW: painting replaces notes, cuts a longer one, same-pitch painted steps merge, erase cuts again
        std::vector<EditNote> cur { { 0, 4, 60, 100 } };
        drawStep (cur, 2, 64); drawStep (cur, 3, 64); drawStep (cur, 4, 64);
        mergeRuns (cur, (1u << 2) | (1u << 3) | (1u << 4));
        bool drawOk = cur.size() == 2;
        for (auto& e : cur) { if (e.step == 0) drawOk = drawOk && e.len == 2 && e.pitch == 60; if (e.step == 2) drawOk = drawOk && e.len == 3 && e.pitch == 64; }
        eraseStep (cur, 3);
        bool eraseOk = cur.size() == 2;
        for (auto& e : cur) if (e.step == 2) eraseOk = eraseOk && e.len == 1;
        // edit undo: an edit made after a snapshot is taken back
        tp.edits.clearAll(); tp.pushEditUndo();
        const EditNote one[1] = { { 0, 2, 70, 100 } };
        tp.edits.set (3, one, 1);
        const bool had = tp.edits.edited (3);
        const bool undone = tp.undoEdit() && ! tp.edits.edited (3) && ! tp.canUndoEdit();
        const bool pass = lists && styles && pump && drawOk && eraseOk && had && undone;
        std::printf ("  [%s] 2.2: lists %s, chord styles %d/%d/%d/%d notes %s, pump rms %.3f -> %.3f %s, draw %s, erase %s, edit undo %s\n", pass ? "OK" : "FAIL",
                     lists ? "ok" : "BAD", got[0], got[1], got[2], got[3], styles ? "ok" : "BAD", r0, r1, pump ? "ok" : "BAD", drawOk ? "ok" : "BAD", eraseOk ? "ok" : "BAD", had && undone ? "ok" : "BAD");
        fails += pass ? 0 : 1;
    }
    // 11) RANDOMIZE changes the melody side but never the sound (synth params + reverb); NEW SOUND is what changes them
    {
        tp.applyPreset (0);
        const float before[] = { tp.get (detuneI), tp.get (widthI), tp.get (cutoffI), tp.get (resoI), tp.get (fenvI), tp.get (attackI), tp.get (decayI),
                                 tp.get (sustainI), tp.get (releaseI), tp.get (rsizeI), tp.get (reverbI) };
        bool soundSame = true, melodyMoved = false;
        const int seed0 = (int) tp.get (seedI);
        for (int k = 0; k < 30; ++k)
        {
            tp.randomize();
            const float now[] = { tp.get (detuneI), tp.get (widthI), tp.get (cutoffI), tp.get (resoI), tp.get (fenvI), tp.get (attackI), tp.get (decayI),
                                  tp.get (sustainI), tp.get (releaseI), tp.get (rsizeI), tp.get (reverbI) };
            for (int i = 0; i < 11; ++i) soundSame = soundSame && std::abs (now[i] - before[i]) < 1.0e-4f;
            melodyMoved = melodyMoved || (int) tp.get (seedI) != seed0;
        }
        tp.randomSound();
        const bool newSoundMoves = std::abs (tp.get (cutoffI) - before[2]) > 1.0f || std::abs (tp.get (detuneI) - before[0]) > 0.01f;
        std::printf ("  [%s] RANDOMIZE: sound untouched %s, melody settings change %s; NEW SOUND changes the sound %s\n", soundSame && melodyMoved && newSoundMoves ? "OK" : "FAIL",
                     soundSame ? "yes" : "NO", melodyMoved ? "yes" : "NO", newSoundMoves ? "yes" : "NO");
        fails += soundSame && melodyMoved && newSoundMoves ? 0 : 1;
        tp.applyPreset (0);
    }
    // 9) OWN SOUND: your sample plays the generated notes (Replace), state keeps it, removing it brings the synth back
    {
        tp.applyPreset (0);
        if (! Sonic::ownSoundSelfTest<TuneProcessor> (44100.0, 256, 60, 1.2f)) ++fails;
    }
    return fails;
}

int main (int argc, char** argv) { return boytest::testMain (argc, argv, "tune", tuneChecks); }
