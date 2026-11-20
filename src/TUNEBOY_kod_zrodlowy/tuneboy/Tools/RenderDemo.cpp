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
    // Candidate selection changes free bars and is undoable.
    {
        TuneProcessor p;
        p.setParam ("seed", 6102.0f);
        p.toggleLock (1);
        const EditNote custom { 0, 8, 72, 100 };
        p.edits.set (2, &custom, 1);
        const int before = p.generateNow (a.data());
        std::mt19937 rng (338);
        const bool moved = p.chooseMelody (false, rng);
        const int after = p.generateNow (b.data());
        const bool protectedBars = melody (a, before, 1) == melody (b, after, 1)
                                && melody (a, before, 2) == melody (b, after, 2);
        const bool distinct = melody (a, before, 0) != melody (b, after, 0)
                           || melody (a, before, 3) != melody (b, after, 3);
        p.undo();
        const bool undone = (int) p.get (seedI) == 6102;
        p.toggleFrozen ("seed"); p.toggleFrozen ("rhythm"); p.toggleFrozen ("scale");
        p.setParam ("rhythm", 3.0f); p.setParam ("scale", 5.0f);
        p.setFrenchcore (true);
        p.randomize();
        const bool frozen = p.get (seedI) == 6102 && p.get (rhythmI) == 3 && p.get (scaleI) == 5;
        TuneProcessor epic;
        epic.setParam ("epic", 1.0f); epic.setParam ("bars", 2.0f);
        epic.toggleLock (1);
        const int epicBefore = epic.generateNow (a.data());
        const bool epicMoved = epic.chooseMelody (false, rng);
        const int epicAfter = epic.generateNow (b.data());
        const bool epicLock = melody (a, epicBefore, 1) == melody (b, epicAfter, 1);
        MemoryBlock saved;
        epic.getStateInformation (saved);
        TuneProcessor loaded;
        loaded.setStateInformation (saved.getData(), (int) saved.getSize());
        const int restored = loaded.generateNow (a.data());
        bool epicKept = restored == epicAfter;
        for (int bar = 0; bar < 16; ++bar) epicKept = epicKept && melody (a, restored, bar) == melody (b, epicAfter, bar);
        const bool ok = moved && distinct && protectedBars && undone && frozen && epicMoved && epicLock && epicKept;
        std::printf ("  [%s] melody choice changes unlocked bars, supports undo and respects Freeze\n", ok ? "OK" : "FAIL");
        fails += ok ? 0 : 1;
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
    {
        bool ok = true;
        for (int transition = 0; transition < 3; ++transition)
        {
            TuneProcessor p;
            p.setParam ("midiout", 1.0f);
            const EditNote first { 0, 8, 72, 100 };
            p.edits.set (0, &first, 1);
            p.setPlayConfigDetails (0, 2, 44100, 256); p.prepareToPlay (44100, 256);
            AudioBuffer<float> buf (2, 256);
            MidiBuffer midi;
            midi.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 0);
            p.processBlock (buf, midi);
            std::array<std::array<bool, 128>, 2> active {};
            for (const auto ev : midi)
            {
                const auto msg = ev.getMessage();
                if (msg.isNoteOn()) active[(size_t) (msg.getChannel() - 1)][(size_t) msg.getNoteNumber()] = true;
            }
            for (const auto& channel : active) ok = ok && std::any_of (channel.begin(), channel.end(), [] (bool on) { return on; });
            if (transition == 0) p.setParam ("mode", 1.0f);
            else p.setParam ("midiout", transition == 1 ? 2.0f : 0.0f);
            midi.clear();
            p.processBlock (buf, midi);
            for (const auto ev : midi)
            {
                const auto msg = ev.getMessage();
                ok = ok && msg.isNoteOff() && ev.samplePosition == 0;
                if (msg.isNoteOff()) active[(size_t) (msg.getChannel() - 1)][(size_t) msg.getNoteNumber()] = false;
            }
            const bool leadRemains = std::any_of (active[0].begin(), active[0].end(), [] (bool on) { return on; });
            const bool chordsRemain = std::any_of (active[1].begin(), active[1].end(), [] (bool on) { return on; });
            ok = ok && ! chordsRemain && leadRemains == (transition == 2);
        }
        std::printf ("  [%s] mode and MIDI routing changes release the affected notes\n", ok ? "OK" : "FAIL");
        fails += ok ? 0 : 1;
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
    // 12) Phrase engine: 8 bars are a phrase, not one motif copied across. Classic stays what a legacy project loads.
    {
        tp.edits.clearAll();
        for (int bar = 0; bar < 16; ++bar) if (tp.lockOf (bar) > 0) tp.toggleLock (bar);
        tp.setFrenchcore (false);
        const int seeds[] = { 1, 17, 101, 202, 777, 4101, 8888 };
        const int rhythmsN[] = { 1, 0, 5, 6, 2, 4, 12 };
        const int rhythmsF[] = { 5, 13, 6, 1, 12, 11, 10 };
        int okN = 0, okF = 0;
        auto analyse = [&] (bool fc, int rhythm, float density, int seed) {
            tp.setFrenchcore (false);
            tp.applyPreset (0);
            tp.edits.clearAll();
            tp.setParam ("bars", 1.0f);
            tp.setParam ("engine", 1.0f);
            tp.setParam ("scale", 0.0f);
            tp.setParam ("prog", 0.0f);
            tp.setParam ("octave", 5.0f);
            tp.setParam ("range", 8.0f);
            tp.setParam ("density", density);
            tp.setParam ("rhythm", (float) rhythm);
            tp.setParam ("seed", (float) seed);
            tp.setParam ("variation", 0.0f);
            tp.setParam ("chordstyle", 0.0f);
            if (fc) tp.setFrenchcore (true);
            const int n = tp.generateNow (a.data());
            struct Ev { double st, len; int pitch, bar, step; };
            std::vector<Ev> m;
            for (int i = 0; i < n; ++i)
            {
                if (a[(size_t) i].chord || a[(size_t) i].start >= 32.0) continue;
                const int bar = (int) (a[(size_t) i].start / 4.0);
                const int step = (int) std::floor ((a[(size_t) i].start - bar * 4.0) / 0.25 + 0.5);
                if (bar >= 0 && bar < 8) m.push_back ({ a[(size_t) i].start, a[(size_t) i].len, a[(size_t) i].pitch, bar, step });
            }
            uint16_t mask[8] = {};
            for (auto& e : m) if (e.step >= 0 && e.step < 16) mask[e.bar] = (uint16_t) (mask[e.bar] | (1u << e.step));
            int distinct = 0;
            for (int i = 0; i < 8; ++i)
            {
                bool seen = false;
                for (int j = 0; j < i; ++j) if (mask[j] == mask[i]) seen = true;
                distinct += seen ? 0 : 1;
            }
            bool halves = false;
            for (int i = 0; i < 4; ++i) if (mask[i] != mask[i + 4]) halves = true;
            int mn = 127, mx = 0;
            for (auto& e : m) { mn = std::min (mn, e.pitch); mx = std::max (mx, e.pitch); }
            const int span = m.empty() ? 0 : mx - mn;
            int leaps[2] = {};
            for (size_t i = 1; i < m.size(); ++i)
                if (std::abs (m[i].pitch - m[i - 1].pitch) > 7)
                {
                    const int g = m[i].bar / 4;
                    if (g < 2) ++leaps[g];
                }
            int off = 0;
            double gateSum = 0;
            for (size_t i = 0; i < m.size(); ++i)
            {
                double gap = (i + 1 < m.size()) ? m[i + 1].st - m[i].st : 0.5;
                if (gap < 1.0e-6) gap = 0.5;
                gateSum += m[i].len / gap;
                if (m[i].step % 4 != 0) ++off;
            }
            const double meanGate = m.empty() ? 1.0 : gateSum / (double) m.size();
            const double offFrac = m.empty() ? 0.0 : (double) off / (double) m.size();
            bool ok = (int) m.size() >= 8 && distinct >= 4 && halves && span >= 7 && span <= 19 && leaps[0] <= 2 && leaps[1] <= 2;
            if (fc) ok = ok && offFrac >= 0.35 && meanGate < 0.8;
            if (! ok)
                std::printf ("    seed %d %s rhythm %d: notes %d distinct %d halves %d span %d leaps %d/%d off %.2f gate %.2f\n",
                             seed, fc ? "FC" : "normal", rhythm, (int) m.size(), distinct, halves ? 1 : 0, span, leaps[0], leaps[1], offFrac, meanGate);
            return ok;
        };
        for (int i = 0; i < 7; ++i)
        {
            okN += analyse (false, rhythmsN[i], 0.65f, seeds[i]) ? 1 : 0;
            okF += analyse (true, rhythmsF[i], 0.75f, seeds[i]) ? 1 : 0;
        }
        // legacy project: no engine parameter and no marker -> Classic, and that matches engine 0 note for note
        tp.setFrenchcore (false);
        tp.applyPreset (0);
        tp.setParam ("seed", 202.0f);
        tp.setParam ("variation", 0.0f);
        tp.setParam ("engine", 0.0f);
        const int nClassic = tp.generateNow (a.data());
        tp.setParam ("engine", 1.0f);
        tp.reloadAsLegacyProject();
        const int nLegacy = tp.generateNow (b.data());
        bool same = std::abs (tp.get (engineI)) < 0.01f && nClassic == nLegacy;
        for (int i = 0; i < nClassic && same; ++i)
            same = a[(size_t) i].pitch == b[(size_t) i].pitch && a[(size_t) i].start == b[(size_t) i].start
                && std::abs (a[(size_t) i].len - b[(size_t) i].len) < 1.0e-9 && std::abs (a[(size_t) i].vel - b[(size_t) i].vel) < 1.0e-6f
                && a[(size_t) i].chord == b[(size_t) i].chord;
        TuneProcessor fresh;
        const bool freshPhrase = std::abs (fresh.get (engineI) - 1.0f) < 0.01f;
        tp.setFrenchcore (true);
        tp.applyPreset (0);
        const bool fcPhrase = std::abs (tp.get (engineI) - 1.0f) < 0.01f;
        tp.setFrenchcore (false);
        tp.applyPreset (0);
        tp.setParam ("engine", 1.0f);
        tp.setParam ("bars", 1.0f);
        tp.setParam ("seed", 101.0f);
        tp.hostBpm.store (150.0);
        const auto midN = File::getCurrentWorkingDirectory().getChildFile ("phrase_normal.mid");
        const bool wroteN = tp.writeMidiFile (midN);
        { AudioBuffer<float> wav; boytest::render (tp, "tune", 12.0, &wav); boytest::writeWav (wav, File::getCurrentWorkingDirectory().getChildFile ("phrase_normal.wav")); }
        tp.setFrenchcore (true);
        tp.setParam ("rhythm", 13.0f);
        tp.setParam ("density", 0.75f);
        tp.setParam ("scale", 1.0f);
        tp.setParam ("seed", 4404.0f);
        tp.hostBpm.store (190.0);
        const auto midF = File::getCurrentWorkingDirectory().getChildFile ("phrase_frenchcore.mid");
        const bool wroteF = tp.writeMidiFile (midF);
        { AudioBuffer<float> wav; boytest::render (tp, "tune", 12.0, &wav); boytest::writeWav (wav, File::getCurrentWorkingDirectory().getChildFile ("phrase_frenchcore.wav")); }
        tp.setFrenchcore (false);
        tp.applyPreset (0);
        const bool freshVb = fresh.themeIndex() == 3 && fresh.frenchcore();
        // Tight (the shipped Phrase) vs Wide, same 7 seeds: distinct bars, interval entropy, contour distance
        auto metrics = [&] (int variety, double& distinct, double& entropy, double& contour) {
            distinct = entropy = contour = 0;
            std::vector<std::vector<int>> deltas (7);
            for (int s = 0; s < 7; ++s)
            {
                tp.setParam ("engine", 1.0f); tp.setParam ("epic", 0.0f); tp.setParam ("variety", (float) variety);
                tp.setParam ("frenchcore", 0.0f);
                tp.setFrenchcore (false);
                tp.setParam ("bars", 1.0f); tp.setParam ("rhythm", 1.0f); tp.setParam ("scale", 0.0f); tp.setParam ("density", 0.65f);
                tp.setParam ("seed", (float) seeds[s]); tp.setParam ("variation", 0.0f);
                const int n = tp.generateNow (a.data());
                uint16_t mask[8] = {};
                std::vector<int> pitches;
                for (int i = 0; i < n; ++i)
                {
                    if (a[(size_t) i].chord || a[(size_t) i].start >= 32.0) continue;
                    const int bar = (int) (a[(size_t) i].start / 4.0);
                    const int step = (int) std::floor ((a[(size_t) i].start - bar * 4.0) / 0.25 + 0.5);
                    if (bar >= 0 && bar < 8 && step >= 0 && step < 16) { mask[bar] = (uint16_t) (mask[bar] | (1u << step)); pitches.push_back (a[(size_t) i].pitch); }
                }
                int uniq = 0;
                for (int i = 0; i < 8; ++i) { bool seen = false; for (int j = 0; j < i; ++j) if (mask[j] == mask[i]) seen = true; uniq += seen ? 0 : 1; }
                distinct += uniq;
                int hist[25] = {};
                int niv = 0;
                deltas[s].clear();
                for (size_t i = 1; i < pitches.size(); ++i)
                {
                    const int d = pitches[i] - pitches[i - 1];
                    deltas[s].push_back (d);
                    ++hist[juce::jlimit (0, 24, d + 12)];
                    ++niv;
                }
                double h = 0;
                for (int c : hist) if (c > 0 && niv > 0) { const double p = (double) c / (double) niv; h -= p * std::log (p) / std::log (2.0); }
                entropy += h;
            }
            distinct /= 7.0; entropy /= 7.0;
            double acc = 0; int pairs = 0;
            for (int i = 0; i < 7; ++i) for (int j = i + 1; j < 7; ++j)
            {
                const int m = (int) std::max (deltas[i].size(), deltas[j].size());
                if (m == 0) continue;
                double l1 = 0;
                for (int k = 0; k < m; ++k)
                {
                    const int a = k < (int) deltas[i].size() ? deltas[i][(size_t) k] : 0;
                    const int b = k < (int) deltas[j].size() ? deltas[j][(size_t) k] : 0;
                    l1 += std::abs (a - b);
                }
                acc += l1 / (double) m; ++pairs;
            }
            contour = pairs > 0 ? acc / (double) pairs : 0;
        };
        double d0, e0, c0, d1, e1, c1;
        metrics (0, d0, e0, c0);
        metrics (1, d1, e1, c1);
        const bool richer = e1 > e0 + 0.05 && c1 > c0 + 0.2;
        std::printf ("  variety Tight -> Wide: distinct %.2f -> %.2f, interval entropy %.2f -> %.2f bits, contour distance %.2f -> %.2f\n",
                     d0, d1, e0, e1, c0, c1);
        // two epic themes, MIDI + a short render
        auto exportEpic = [&] (const char* name, float presetSeed, float scale, float prog) {
            tp.setFrenchcore (false);
            tp.setParam ("engine", 1.0f); tp.setParam ("epic", 1.0f); tp.setParam ("bars", 2.0f);
            tp.setParam ("scale", scale); tp.setParam ("prog", prog); tp.setParam ("rhythm", 4.0f);
            tp.setParam ("seed", presetSeed); tp.setParam ("reverb", 0.55f); tp.setParam ("chordstyle", 0.0f); tp.setParam ("midiout", 1.0f);
            tp.hostBpm.store (140.0);
            const auto mid = File::getCurrentWorkingDirectory().getChildFile (name).withFileExtension ("mid");
            const bool midiOk = tp.writeMidiFile (mid);
            AudioBuffer<float> wav;
            const auto rr = boytest::render (tp, "tune", 20.0, &wav);
            const auto wavF = File::getCurrentWorkingDirectory().getChildFile (name).withFileExtension ("wav");
            boytest::writeWav (wav, wavF);
            std::printf ("  epic %s: midi %s, wav rms %.3f %s\n", name, midiOk ? "yes" : "NO", rr.rms, rr.bad ? "BAD" : "clean");
            return midiOk && ! rr.bad && rr.rms > 0.005 && rr.midiOut > 0;
        };
        const bool epicOk = exportEpic ("epic_hymn", 5101.0f, 0.0f, 0.0f) && exportEpic ("epic_lift", 5202.0f, 1.0f, 3.0f);
        tp.setParam ("epic", 0.0f);
        tp.setTheme (3);   // the screenshot after this check is the startup theme
        const bool pass = okN == 7 && okF == 7 && same && freshPhrase && fcPhrase && freshVb && richer && epicOk;
        std::printf ("  [%s] phrase engine: normal %d/7, FRENCHCORE %d/7, legacy->Classic %s, new instance Phrase %s, FC preset Phrase %s, Virtual Boy %s, variety up %s, epic %s\n",
                     pass ? "OK" : "FAIL", okN, okF, same ? "yes" : "NO", freshPhrase ? "yes" : "NO", fcPhrase ? "yes" : "NO", freshVb ? "yes" : "NO", richer ? "yes" : "NO", epicOk ? "yes" : "NO");
        if (wroteN || wroteF)
            std::printf ("  MIDI examples: %s %s, %s %s\n", wroteN ? "wrote" : "FAILED", midN.getFullPathName().toRawUTF8(), wroteF ? "wrote" : "FAILED", midF.getFullPathName().toRawUTF8());
        fails += pass ? 0 : 1;
    }
    return fails;
}

int main (int argc, char** argv) { return boytest::testMain (argc, argv, "tune", tuneChecks); }
