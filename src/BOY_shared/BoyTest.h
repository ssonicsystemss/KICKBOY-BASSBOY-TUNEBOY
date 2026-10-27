// BOY series offline test (each plugin's Tools/RenderDemo.cpp includes its Source/*.cpp and then this file).
//   <X>Render demo.wav gui.png   ->  prints "RESULT: PASS" (exit 0) or "RESULT: FAIL"
// Checks: every factory preset renders clean audio; RANDOMIZE (20x) + undo; FREEZE survives RANDOMIZE; state and
// MY PRESETS round trips; hanging notes / odd block sizes / sample rates / no prepare / editor open-close; RAM + CPU;
// plus the plugin's own checks (extraChecks). Writes the demo WAV and screenshots of MAIN + ADVANCED.
#pragma once
#include <chrono>
#include <cstdio>
#include <functional>

namespace boytest
{
using namespace juce;

struct Head : AudioPlayHead
{
    double ppq = 0, bpm = 150; bool playing = true;
    Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (playing); p.setTimeSignature (AudioPlayHead::TimeSignature{}); return p; }
};

struct RenderResult { double rms = 0, peak = 0; bool bad = false; int midiOut = 0; double cpuRatio = 0; };

// kick: a note every beat; bass / tune: one long note
inline RenderResult render (AudioProcessor& p, const String& kind, double seconds, AudioBuffer<float>* keep, int note = 60)
{
    const double sr = 44100; const int bs = 256; Head head; p.setPlayHead (&head);
    p.setPlayConfigDetails (0, 2, sr, bs);
    p.prepareToPlay (sr, bs);
    const int total = (int) (seconds * sr); AudioBuffer<float> buf (2, bs); if (keep) keep->setSize (2, total);
    RenderResult r; double sum = 0; const double spb = sr * 60.0 / head.bpm; double cpuT = 0;
    for (int pos = 0; pos < total; pos += bs)
    {
        const int n = jmin (bs, total - pos); MidiBuffer m;
        if (kind == "kick")
            for (int i = 0; i < n; ++i)
            {
                if ((pos + i) % (int) spb == 0) m.addEvent (MidiMessage::noteOn (1, note, (uint8) 110), i);
                if ((pos + i) % (int) spb == (int) (spb * 0.9)) m.addEvent (MidiMessage::noteOff (1, note), i);
            }
        else if (pos == 0) m.addEvent (MidiMessage::noteOn (1, note, (uint8) 110), 0);
        AudioBuffer<float> b (buf.getArrayOfWritePointers(), 2, n);
        const auto t0 = std::chrono::high_resolution_clock::now();
        p.processBlock (b, m);
        cpuT += std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
        for (const auto ev : m) if (ev.getMessage().isNoteOn()) ++r.midiOut;
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { const float v = b.getSample (c, i); if (! std::isfinite (v)) r.bad = true; sum += v * v; r.peak = jmax (r.peak, (double) std::abs (v)); }
        if (keep) for (int c = 0; c < 2; ++c) keep->copyFrom (c, pos, b, c, 0, n);
        head.ppq += n / spb;
    }
    r.rms = std::sqrt (sum / (2.0 * total)); r.cpuRatio = cpuT / seconds; p.setPlayHead (nullptr);
    return r;
}

inline void writeWav (const AudioBuffer<float>& b, const File& f)
{
    f.deleteFile(); WavAudioFormat w;
    std::unique_ptr<AudioFormatWriter> wr (w.createWriterFor (new FileOutputStream (f), 44100, 2, 24, {}, 0));
    if (wr) wr->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

// ---------- hanging-note / robustness tests ----------
struct Script { std::function<void (int pos, int n, MidiBuffer&, Head&)> fn; };
inline double tailPeak (AudioProcessor& p, double sr, double seconds, double tailFrom, Script sc, int midiBal[2][128], std::function<int()> blockSize = {})
{
    Head head; head.playing = false; p.setPlayHead (&head); p.setPlayConfigDetails (0, 2, sr, 4096); p.prepareToPlay (sr, 4096);
    std::vector<float> L (4096), R (4096); float* ch[2] = { L.data(), R.data() };
    const int total = (int) (seconds * sr); double peak = 0; const double spb = sr * 60.0 / head.bpm;
    for (int pos = 0; pos < total;)
    {
        int n = blockSize ? blockSize() : 256; n = jmin (n, total - pos);
        AudioBuffer<float> b (ch, 2, n); MidiBuffer m;
        sc.fn (pos, n, m, head);
        p.processBlock (b, m);
        for (const auto ev : m) { const auto mm = ev.getMessage(); const int c = jlimit (0, 1, mm.getChannel() - 1);
            if (mm.isNoteOn()) midiBal[c][mm.getNoteNumber()]++; else if (mm.isNoteOff()) midiBal[c][mm.getNoteNumber()] = jmax (0, midiBal[c][mm.getNoteNumber()] - 1); }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { const float v = b.getSample (c, i); if (! std::isfinite (v)) return 99; if (pos + i >= tailFrom * sr) peak = jmax (peak, (double) std::abs (v)); }
        if (head.playing) head.ppq += n / spb;
        pos += jmax (1, n);
    }
    p.setPlayHead (nullptr); return peak;
}
inline bool evAt (int pos, int n, int at) { return at >= pos && at < pos + n; }

inline int robustness (const String& kind)
{
    int fails = 0; const double sil = 1.0e-3;   // -60 dBFS
    std::unique_ptr<AudioProcessor> p (createPluginFilter());
    auto* bp = dynamic_cast<boy::BaseProcessor*> (p.get());
    auto check = [&] (const char* what, double pk, int bal[2][128]) {
        int hang = 0; for (int c = 0; c < 2; ++c) for (int i = 0; i < 128; ++i) hang += bal[c][i];
        const bool ok = pk < sil && hang == 0; fails += ok ? 0 : 1;
        std::printf ("  [%s] %-46s tailPeak=%.2e hangingMidiOut=%d\n", ok ? "OK" : "FAIL", what, pk, hang); return ok; };
    int worstPreset = 0; double worst = 0; int bal[2][128] {};
    for (int i = 0; i < (int) bp->presets.size(); ++i)   // FL preview click: 10 ms note, transport stopped, every preset
    {
        bp->applyPreset (i);
        const double pk = tailPeak (*p, 44100, 14, 12, { [] (int pos, int n, MidiBuffer& m, Head&) {
            if (evAt (pos, n, 1000)) m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 1000 - pos);
            if (evAt (pos, n, 1441)) m.addEvent (MidiMessage::noteOff (1, 60), 1441 - pos); } }, bal);
        if (pk > worst) { worst = pk; worstPreset = i; }
    }
    check (("short click 10ms, all presets (worst #" + String (worstPreset + 1) + ")").toRawUTF8(), worst, bal);
    bp->applyPreset (0);
    { int b2[2][128] {}; check ("on/off same block + 30 rapid clicks", tailPeak (*p, 44100, 14, 12, { [] (int pos, int n, MidiBuffer& m, Head&) {
        for (int k = 0; k < 30; ++k) { const int at = 500 + k * 3000; if (evAt (pos, n, at)) m.addEvent (MidiMessage::noteOn (1, 55 + k % 7, (uint8) 100), at - pos);
                                       if (evAt (pos, n, at + 20)) m.addEvent (MidiMessage::noteOff (1, 55 + k % 7), at + 20 - pos); } } }, b2), b2); }
    { int b3[2][128] {}; check ("double note-on + single note-off", tailPeak (*p, 44100, 14, 12, { [] (int pos, int n, MidiBuffer& m, Head&) {
        if (evAt (pos, n, 100)) m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 100 - pos);
        if (evAt (pos, n, 5000)) m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 5000 - pos);
        if (evAt (pos, n, 40000)) m.addEvent (MidiMessage::noteOff (1, 60), 40000 - pos); } }, b3), b3); }
    { int b4[2][128] {}; check ("transport stop while key held", tailPeak (*p, 44100, 16, 14, { [] (int pos, int, MidiBuffer& m, Head& h) {
        if (pos == 0) { h.playing = true; m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 0); }
        if (pos >= 44100 * 3) h.playing = false; } }, b4), b4); }
    for (int cc : { 123, 120 })
    { int b5[2][128] {}; check (("CC" + String (cc) + " while key held").toRawUTF8(), tailPeak (*p, 44100, 14, 12, { [cc] (int pos, int n, MidiBuffer& m, Head&) {
        if (pos == 0) m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 0);
        if (evAt (pos, n, 88200)) m.addEvent (MidiMessage::controllerEvent (1, cc, 0), 88200 - pos); } }, b5), b5); }
    if (kind == "tune")
    { int b6[2][128] {};
      check ("regenerate while playing, then note-off", tailPeak (*p, 44100, 16, 14, { [bp] (int pos, int n, MidiBuffer& m, Head&) {
        if (pos == 0) m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 0);
        if (evAt (pos, n, 50000)) bp->setParam ("seed", 777); if (evAt (pos, n, 70000)) bp->setParam ("bars", 0);
        if (evAt (pos, n, 100000)) m.addEvent (MidiMessage::noteOff (1, 60), 100000 - pos); } }, b6), b6); }
    {
        Random r (42); bool okAll = true; int b7[2][128] {};
        for (double sr : { 44100.0, 48000.0, 96000.0, 22050.0, 192000.0 })
        {
            const double pk = tailPeak (*p, sr, 14, 12, { [sr] (int pos, int n, MidiBuffer& m, Head& h) {
                if (pos == 0) { h.playing = true; m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 0); }
                const int off = (int) (sr * 2); if (evAt (pos, n, off)) m.addEvent (MidiMessage::noteOff (1, 60), off - pos);
                if (pos > sr * 3) h.playing = false; } }, b7, [&r] { const int k = r.nextInt (10); return k == 0 ? 0 : (k < 3 ? 1 + r.nextInt (7) : 1 + r.nextInt (4096)); });
            okAll &= pk < sil;
        }
        check ("random block sizes 0..4096 @22k..192k", okAll ? 0.0 : 1.0, b7);
    }
    {
        bool ok = true;
        for (int k = 0; k < 8; ++k)
        {
            std::unique_ptr<AudioProcessor> q (createPluginFilter());
            AudioBuffer<float> b (2, 8192); MidiBuffer m; m.addEvent (MidiMessage::noteOn (1, 60, (uint8) 100), 0);
            q->processBlock (b, m);   // no prepare!
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 8192; ++i) ok &= std::isfinite (b.getSample (c, i));
            for (int e = 0; e < 3; ++e) { std::unique_ptr<AudioProcessorEditor> ed (q->createEditor()); ed->setVisible (true); dynamic_cast<boy::BaseProcessor*> (q.get())->applyPreset (e); ed->resized(); }
        }
        std::printf ("  [%s] no-prepare 8192 block, 8 instances x 3 editor open/close\n", ok ? "OK" : "FAIL"); fails += ok ? 0 : 1;
    }
    return fails;
}

inline std::vector<float> values (AudioProcessor& p) { std::vector<float> v; for (auto* q : p.getParameters()) v.push_back (q->getValue()); return v; }
inline bool same (const std::vector<float>& a, const std::vector<float>& b)
{ if (a.size() != b.size()) return false; for (size_t i = 0; i < a.size(); ++i) if (std::abs (a[i] - b[i]) > 1.0e-5f) return false; return true; }

inline void snapshot (AudioProcessorEditor& ed, const File& f)
{
    auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
    f.deleteFile(); FileOutputStream os (f); PNGImageFormat png; png.writeImageToStream (img, os);
}

double residentMB();   // defined below (platform)

inline int testMain (int argc, char** argv, const String& kind, std::function<int (boy::BaseProcessor&)> extraChecks = {})
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    ScopedJuceInitialiser_GUI gui;
    const File wav = File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "demo.wav");
    const String pngPath = argc > 2 ? argv[2] : "";
    std::unique_ptr<AudioProcessor> p (createPluginFilter());
    auto* bp = dynamic_cast<boy::BaseProcessor*> (p.get());
    int fails = 0; double maxCpu = 0;
    std::printf ("== %s : %d presets, %d params\n", p->getName().toRawUTF8(), (int) bp->presets.size(), (int) p->getParameters().size());
    const double secs = kind == "tune" ? 12.8 : 3.2;
    for (int i = 0; i < (int) bp->presets.size(); ++i)
    {
        bp->applyPreset (i);
        AudioBuffer<float> keep;
        auto r = render (*p, kind, secs, i == 0 ? &keep : nullptr);
        bool ok = ! r.bad && r.rms > 0.005 && r.peak <= 1.0001 && bp->presetIndex() == i;
        if (kind == "tune") ok = ok && r.midiOut > 0;
        fails += ok ? 0 : 1; maxCpu = jmax (maxCpu, r.cpuRatio);
        std::printf ("  [%s] %2d %-26s rms=%.3f peak=%.3f cpu=%.3f%%%s\n", ok ? "OK" : "FAIL", i, bp->presets[(size_t) i].name.toRawUTF8(), r.rms, r.peak, r.cpuRatio * 100,
                     kind == "tune" ? (" midiOut=" + String (r.midiOut)).toRawUTF8() : "");
        if (i == 0) { writeWav (keep, wav); std::printf ("  WAV written: %s\n", wav.getFullPathName().toRawUTF8()); }
    }
    // RANDOMIZE 20x: changes the sound, stays clean; undo brings the sound back
    int rndOk = 0;
    for (int k = 0; k < 20; ++k)
    {
        const auto before = values (*p);
        bp->randomize();
        const auto after = values (*p);
        auto r = render (*p, kind, secs * 0.5, nullptr);
        const bool ok = ! same (before, after) && ! r.bad && r.rms > 0.002 && r.peak <= 1.0001;
        rndOk += ok ? 1 : 0; maxCpu = jmax (maxCpu, r.cpuRatio);
    }
    std::printf ("  [%s] RANDOMIZE: %d/20 changed the sound and stayed clean\n", rndOk == 20 ? "OK" : "FAIL", rndOk);
    fails += 20 - rndOk;
    {
        bp->applyPreset (2);
        const auto before = values (*p);
        bp->randomize(); bp->undo();
        const bool ok = same (before, values (*p)) && bp->presetIndex() == 2;
        std::printf ("  [%s] RANDOMIZE + undo restores the sound\n", ok ? "OK" : "FAIL"); fails += ok ? 0 : 1;
    }
    {   // FREEZE: a frozen control survives RANDOMIZE
        const auto& sp = *std::find_if (bp->specs.begin(), bp->specs.end(), [] (const boy::Spec& s) { return s.rndMin != s.rndMax && s.choices.isEmpty(); });
        bp->toggleFrozen (sp.id);
        const float v = p->getParameters()[bp->indexOf (sp.id)]->getValue();
        bool ok = bp->isFrozen (sp.id);
        for (int k = 0; k < 10; ++k) { bp->randomize(); ok = ok && std::abs (p->getParameters()[bp->indexOf (sp.id)]->getValue() - v) < 1.0e-6f; }
        bp->toggleFrozen (sp.id);
        std::printf ("  [%s] FREEZE: %s stays put through 10x RANDOMIZE\n", ok ? "OK" : "FAIL", sp.name.toRawUTF8()); fails += ok ? 0 : 1;
    }
    {   // state round trip (+ the preset name)
        bp->applyPreset (5); MemoryBlock st; p->getStateInformation (st);
        std::unique_ptr<AudioProcessor> q (createPluginFilter()); q->setStateInformation (st.getData(), (int) st.getSize());
        auto* bq = dynamic_cast<boy::BaseProcessor*> (q.get());
        const bool ok = same (values (*p), values (*q)) && bq->presetName() == bp->presetName();
        std::printf ("  [%s] state save/restore (%s)\n", ok ? "OK" : "FAIL", bq->presetName().toRawUTF8()); fails += ok ? 0 : 1;
    }
    {   // MY PRESETS: save -> change -> load restores; the file is removed again
        bp->applyPreset (3);
        const auto saved = values (*p);
        const String nm = "BOY test preset (delete me)";
        bool ok = bp->saveUserPreset (nm);
        bp->applyPreset (7);
        const auto f = boy::BaseProcessor::userFolder().getChildFile (nm + ".boypreset");
        ok = ok && bp->loadUserPreset (f) && same (saved, values (*p)) && bp->presetName() == nm;
        f.deleteFile();
        std::printf ("  [%s] MY PRESETS save / load\n", ok ? "OK" : "FAIL"); fails += ok ? 0 : 1;
    }
    {   // FRENCHCORE mode (180+ BPM): follows the Virtual Boy theme, own preset bank, RANDOMIZE inside its window, state kept
        bool linked = ! bp->frenchcore();
        bp->setTheme (3); linked = linked && bp->frenchcore();
        bp->setTheme (0); linked = linked && ! bp->frenchcore();
        bp->setTheme (3);
        int presetsOk = 0; const int nfp = (int) bp->bank().size();
        for (int i = 0; i < nfp; ++i)
        {
            bp->applyPreset (i);
            auto r = render (*p, kind, secs, nullptr);
            presetsOk += (! r.bad && r.rms > 0.005 && r.peak <= 1.0001 && bp->presetIndex() == i && (kind != "tune" || r.midiOut > 0)) ? 1 : 0;
        }
        bool inWindow = true, clean = true; int windows = 0;
        for (int k = 0; k < 40; ++k)
        {
            bp->randomize();
            for (auto& sp : bp->specs)
                if (sp.frSet && ! bp->isFrozen (sp.id))
                {
                    const float v = bp->get (bp->indexOf (sp.id));
                    inWindow = inWindow && v >= sp.frLo - 0.51f && v <= sp.frHi + 0.51f && (sp.frLo == sp.frHi || true);
                    if (k == 0) ++windows;
                }
            if (k % 8 == 0) { auto r = render (*p, kind, secs * 0.5, nullptr); clean = clean && ! r.bad && r.peak <= 1.0001; }
        }
        bp->applyPreset (0);
        MemoryBlock st; p->getStateInformation (st);
        std::unique_ptr<AudioProcessor> q (createPluginFilter()); q->setStateInformation (st.getData(), (int) st.getSize());
        const bool kept = dynamic_cast<boy::BaseProcessor*> (q.get())->frenchcore();
        bp->setFrenchcore (false);
        const bool ok = linked && nfp >= 6 && presetsOk == nfp && inWindow && windows >= 6 && clean && kept;
        std::printf ("  [%s] FRENCHCORE: follows the Virtual Boy theme %s, %d/%d presets clean, RANDOMIZE inside %d windows %s, state %s\n", ok ? "OK" : "FAIL",
                     linked ? "yes" : "NO", presetsOk, nfp, windows, inWindow ? "yes" : "NO", kept ? "kept" : "LOST");
        fails += ok ? 0 : 1;
        bp->setTheme (0); bp->applyPreset (0);
    }
    if (extraChecks) fails += extraChecks (*bp);
    fails += robustness (kind);
    {   // RAM: 10 prepared instances, then one editor
        const double r0 = residentMB();
        std::vector<std::unique_ptr<AudioProcessor>> v;
        for (int i = 0; i < 10; ++i) { v.emplace_back (createPluginFilter()); v.back()->setPlayConfigDetails (0, 2, 44100, 512); v.back()->prepareToPlay (44100, 512); }
        const double r1 = residentMB();
        { std::unique_ptr<AudioProcessorEditor> warm (v[1]->createEditor()); auto img = warm->createComponentSnapshot (warm->getLocalBounds(), true, 1.0f); }
        const double r2 = residentMB();   // the first editor also loads JUCE's GUI + the font once per process
        std::vector<std::unique_ptr<AudioProcessorEditor>> eds;
        for (int i = 2; i < 6; ++i) { eds.emplace_back (v[(size_t) i]->createEditor()); eds.back()->setBounds (eds.back()->getBounds()); }
        const double r3 = residentMB();
        std::printf ("  RAM per instance (DSP): %.2f MB   first editor (+ GUI + font, once): %.2f MB   each further editor: %.2f MB\n",
                     (r1 - r0) / 10.0, r2 - r1, (r3 - r2) / 4.0);
    }
    if (pngPath.isNotEmpty())
    {
        bp->applyPreset (0);
        std::unique_ptr<AudioProcessorEditor> ed (p->createEditor());
        const File png = File::getCurrentWorkingDirectory().getChildFile (pngPath);
        if (auto* be0 = dynamic_cast<boy::Editor*> (ed.get()))   // screenshots at the design size (the editor may have shrunk to fit this screen)
            if (be0->scale() < 0.999f) be0->setSize (roundToInt ((float) be0->getWidth() / be0->scale()), roundToInt ((float) be0->getHeight() / be0->scale()));
        snapshot (*ed, png);
        if (auto* be = dynamic_cast<boy::Editor*> (ed.get())) { be->showAdvanced (true); snapshot (*be, png.getSiblingFile (png.getFileNameWithoutExtension() + "_advanced.png")); }
        if (auto* be2 = dynamic_cast<boy::Editor*> (ed.get()))   // one screenshot per colour theme (main page)
        {
            be2->showAdvanced (false);
            for (int t = 0; t < boy::col::numThemes; ++t)
            {
                bp->setTheme (t);
                snapshot (*be2, png.getSiblingFile (png.getFileNameWithoutExtension() + "_" + boy::col::themes[t].id + ".png"));
            }
            bp->setTheme (0);
            // resizing: the whole picture scales with the window (aspect kept), the size is remembered in the state
            const int w0 = be2->getWidth(), h0 = be2->getHeight();
            const float s0 = be2->scale();
            be2->setSize ((int) (w0 * 1.5), (int) (h0 * 1.5));
            const bool scaled = std::abs (be2->scale() - 1.5f * s0) < 0.01f;
            snapshot (*be2, png.getSiblingFile (png.getFileNameWithoutExtension() + "_large.png"));
            const auto kept = bp->apvts.state.getProperty ("uiScale").toString().getFloatValue();
            be2->setSize (w0, h0);
            std::printf ("  [%s] resizing: scale %.2f at 1.5x, remembered %.2f\n", scaled && std::abs (kept - 1.5f * s0) < 0.01f ? "OK" : "FAIL", 1.5f * s0, kept);
            if (! (scaled && std::abs (kept - 1.5f * s0) < 0.01f)) ++fails;
        }
        std::printf ("  GUI screenshots: %s (+ _advanced)\n", png.getFullPathName().toRawUTF8());
    }
    std::printf ("  max CPU (1 instance, 44.1k, 256 buf): %.3f%% of one core\n", maxCpu * 100);
    std::printf ("RESULT: %s\n", fails == 0 ? "PASS" : "FAIL");
    return fails == 0 ? 0 : 1;
}
} // namespace boytest

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
 #include <psapi.h>
double boytest::residentMB()
{
    PROCESS_MEMORY_COUNTERS pmc {};
    return K32GetProcessMemoryInfo (GetCurrentProcess(), &pmc, sizeof (pmc)) ? (double) pmc.WorkingSetSize / (1024.0 * 1024.0) : 0.0;
}
#else
double boytest::residentMB()
{
    const auto t = juce::StringArray::fromTokens (juce::File ("/proc/self/statm").loadFileAsString(), " ", "");
    return (double) t[1].getLargeIntValue() * 4.0 / 1024.0;
}
#endif
