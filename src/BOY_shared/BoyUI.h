// BOY series UI: the whole editor is ONE painted component (no slider / label / combo child widgets), so it costs
// almost no RAM: no images, no cached layers, two small embedded pixel fonts, a 15 Hz timer that repaints only when something
// changed. It stays simple but not monotone: every kind of setting has its own control -
//   big knob (the 1-2 main sounds), knob, vertical fader (levels), amount bar (0..1 amounts), segment buttons
//   (short choices, optionally with little pictures), stepper (long lists), LED switch (on/off), number box (ints)
// - plus one "hero" picture per plugin that is also a control (kick pitch curve, bass swell, melody roll).
// Right-click on any control: host automation items / Create automation clip / Freeze (RANDOMIZE skips it) / reset.
#pragma once
#include "BoyCommon.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "BoyFonts.h"

namespace boy
{
class LnF : public juce::LookAndFeel_V4
{
public:
    LnF()
    {
        pix  = juce::Typeface::createSystemTypefaceFor (BoyFonts::SilkscreenSub_ttf, BoyFonts::SilkscreenSub_ttfSize);
        mono = juce::Typeface::createSystemTypefaceFor (BoyFonts::VT323Sub_ttf, BoyFonts::VT323Sub_ttfSize);
        applyColours();
    }
    // menus and dialogs follow the theme (called by the editor whenever the theme changes)
    void applyColours()
    {
        setColour (juce::ResizableWindow::backgroundColourId, col::ground);
        setColour (juce::PopupMenu::backgroundColourId, col::ground);
        setColour (juce::PopupMenu::textColourId, col::ink);
        setColour (juce::PopupMenu::headerTextColourId, col::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::ink);
        setColour (juce::PopupMenu::highlightedTextColourId, col::inkInverse);
        setColour (juce::AlertWindow::backgroundColourId, col::ground);
        setColour (juce::AlertWindow::textColourId, col::ink);
        setColour (juce::AlertWindow::outlineColourId, col::ink);
        setColour (juce::TextEditor::backgroundColourId, col::well);
        setColour (juce::TextEditor::textColourId, col::ink);
        setColour (juce::TextEditor::outlineColourId, col::ink);
        setColour (juce::TextEditor::focusedOutlineColourId, col::ink);
        setColour (juce::TextEditor::highlightColourId, col::accent);
        setColour (juce::TextEditor::highlightedTextColourId, col::inkInverse);
        setColour (juce::CaretComponent::caretColourId, col::ink);
        setColour (juce::TextButton::buttonColourId, col::ground);
        setColour (juce::TextButton::textColourOffId, col::ink);
        setColour (juce::TextButton::textColourOnId, col::ink);
    }
    // Silkscreen = labels and titles, VT323 = readouts and text (design system: pixel fonts at 16 px and up)
    juce::Font font (float px) const
    { return pix != nullptr ? juce::Font (juce::FontOptions (pix).withHeight (px)) : juce::Font (juce::FontOptions (px)); }
    juce::Font monoFont (float px) const
    { return mono != nullptr ? juce::Font (juce::FontOptions (mono).withHeight (px)) : juce::Font (juce::FontOptions (px)); }
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& f) override
    { return pix != nullptr ? pix : juce::LookAndFeel_V4::getTypefaceForFont (f); }
    juce::Font getPopupMenuFont() override { return monoFont (24.0f); }
    juce::Font getAlertWindowTitleFont() override { return font (16.0f); }
    juce::Font getAlertWindowMessageFont() override { return monoFont (24.0f); }
    juce::Font getAlertWindowFont() override { return monoFont (24.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (16.0f); }
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (down || over ? col::ink : col::ground);
        g.fillRect (r);
        g.setColour (col::ink);
        g.drawRect (r, 2.0f);
    }
    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down) override
    {
        g.setFont (font (16.0f));
        g.setColour (down || over ? col::inkInverse : col::ink);
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
    }
    void drawCornerResizer (juce::Graphics& g, int w, int h, bool over, bool dragging) override
    {
        g.setColour (over || dragging ? col::inkInverse : col::inkInverse.withAlpha (0.8f));
        for (int i = 0; i < 3; ++i)   // three pixel steps
        {
            const float step = (float) (4 + i * 5);
            g.fillRect ((float) w - step - 2.0f, (float) h - 6.0f, 3.0f, 3.0f);
            g.fillRect ((float) w - 5.0f, (float) h - step - 2.0f, 3.0f, 3.0f);
        }
        juce::ignoreUnused (h);
    }
    juce::Typeface::Ptr pix, mono;
};

// FL's typing keyboard: a click on the plugin must not keep the keyboard in the plugin window. Two layers:
// 1) the outer JUCE wrapper component (the window's top component, not our editor) is told not to grab focus on a
//    click (JUCE answers WM_MOUSEACTIVATE with MA_NOACTIVATE only when THAT component says so);
// 2) if the keyboard still ends up in the plugin window (menus, dialogs, hosts), it is handed back to the host's
//    window (the plugin window's root) as soon as no mouse button is down and no menu / dialog is open.
namespace hostkeys
{
    inline void dontGrab (juce::Component& c)
    {
        for (auto* p = &c; p != nullptr; p = p->getParentComponent())
        {
            p->setWantsKeyboardFocus (false);
            p->setMouseClickGrabsKeyboardFocus (false);
        }
    }
   #if JUCE_WINDOWS
    inline void giveBack (juce::Component& c)
    {
        auto* peer = c.getPeer();
        if (peer == nullptr) return;
        // user32 functions by name: no <windows.h> in a header that JUCE code includes
        static juce::DynamicLibrary user32 ("user32.dll");
        using GetFocusFn = void* (__stdcall*) ();
        using SetFocusFn = void* (__stdcall*) (void*);
        using GetAncestorFn = void* (__stdcall*) (void*, unsigned int);
        using IsChildFn = int (__stdcall*) (void*, void*);
        static auto getFocus = (GetFocusFn) user32.getFunction ("GetFocus");
        static auto setFocus = (SetFocusFn) user32.getFunction ("SetFocus");
        static auto getAncestor = (GetAncestorFn) user32.getFunction ("GetAncestor");
        static auto isChild = (IsChildFn) user32.getFunction ("IsChild");
        if (getFocus == nullptr || setFocus == nullptr || getAncestor == nullptr || isChild == nullptr) return;
        void* own = peer->getNativeHandle();
        void* focused = getFocus();
        if (own == nullptr || focused == nullptr || ! (focused == own || isChild (own, focused) != 0)) return;
        void* root = getAncestor (own, 2 /* GA_ROOT: the host's plugin window */);
        if (root != nullptr && root != own) setFocus (root);
    }
   #else
    inline void giveBack (juce::Component&) {}
   #endif
    inline bool safeToGiveBack()
    {
        return ! juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown()
            && juce::ModalComponentManager::getInstance()->getNumModalComponents() == 0;
    }
}

class Editor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    Editor (BaseProcessor& p, juce::String title, juce::String subtitle, int w, int h)
      : juce::AudioProcessorEditor (p), proc (p), name (std::move (title)), sub (std::move (subtitle))
    {
        setLookAndFeel (&lnf.get());
        setOpaque (true);
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);   // knobs never steal FL's keyboard (space bar)
        seen.assign (proc.specs.size(), -1.0f);
        initialSize = { juce::jmax (w, 960), juce::jmax (h, 580) };
        useTheme();
        startTimerHz (15);
    }
    // the plugin's editor calls this at the end of its constructor (the layout asks its virtual hooks)
    void init()
    {
        float sc = juce::jlimit (minScale, maxScale, (float) (double) proc.apvts.state.getProperty ("uiScale", 1.0));
        if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())   // never bigger than the screen
            sc = juce::jmin (sc, 0.95f * (float) d->userArea.getWidth() / (float) initialSize.x, 0.9f * (float) d->userArea.getHeight() / (float) initialSize.y);
        sc = juce::jmax (minScale, sc);
        setResizable (true, true);
        setResizeLimits (juce::roundToInt ((float) initialSize.x * minScale), juce::roundToInt ((float) initialSize.y * minScale),
                         juce::roundToInt ((float) initialSize.x * maxScale), juce::roundToInt ((float) initialSize.y * maxScale));
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) initialSize.x / (double) initialSize.y);
        setSize (juce::roundToInt ((float) initialSize.x * sc), juce::roundToInt ((float) initialSize.y * sc));
        layout();
        sizeReady = true;
    }
    float scale() const { return uiScale; }   // 1 = design size (960 x 580 / 600)
    ~Editor() override { stopTimer(); setLookAndFeel (nullptr); }

    // render tests: the ADVANCED page
    void showAdvanced (bool a) { advanced = a; layout(); repaint(); }

    // once the host has put us into its window: the outer wrapper must not take the keyboard on a click either
    void parentHierarchyChanged() override { hostkeys::dontGrab (*this); }

    void resized() override
    {
        if (initialSize.x > 0 && getWidth() > 0) uiScale = (float) getWidth() / (float) initialSize.x;
        layout();
        if (sizeReady) proc.apvts.state.setProperty ("uiScale", juce::String (uiScale, 3), nullptr);   // the window size is remembered
    }

    // the theme lives in the state; colours are global per plugin, so each editor re-selects its own before painting
    void useTheme()
    {
        const int t = proc.themeIndex();
        if (t != col::current) { col::set (t); lnf->applyColours(); }
    }

    void paint (juce::Graphics& g) override
    {
        useTheme();
        g.fillAll (col::ground);
        g.addTransform (juce::AffineTransform::scale (uiScale));   // everything below is drawn at the design size
        const auto b = logicalBounds();
        paintTop (g);
        if (! advanced && heroArea.getHeight() > 0)
        {
            g.setColour (col::well);
            g.fillRect (heroArea);
            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (heroArea.toNearestInt());
            paintHero (g, heroArea);
            g.setColour (col::ink);   // a control or picture sitting on the well is outlined with ink
            g.drawRect (heroArea, 2.0f);
        }
        for (size_t i = 0; i < buttons.size(); ++i) paintButton (g, buttons[i], (int) i == hoverButton);
        for (size_t i = 0; i < ctls.size(); ++i) paintCtl (g, ctls[i], (int) i == hover || (int) i == active);
        // footer: what the mouse points at / what just happened (ink bar, inverse text)
        auto f = b.withTop (b.getBottom() - footerH);
        g.setColour (col::ink);
        g.fillRect (f);
        g.setColour (col::inkInverse);
        g.setFont (monoFont (20.0f));
        const bool recent = juce::Time::getMillisecondCounter() - statusAt < 4000;
        g.drawText (recent ? statusText : hintText.isNotEmpty() ? hintText
                           : juce::String ("drag = change  |  double-click = reset  |  right-click = automation / freeze  |  shift = fine"),
                    f.reduced (12, 0), juce::Justification::centredLeft, true);
    }

    // ---------------------------------------------------------------- mouse
    void mouseMove (const juce::MouseEvent& e0) override
    {
        const auto e = scaled (e0);
        const int h = ctlAt (e.position), hb = buttonAt (e.position);
        juce::String hint;
        if (h >= 0) hint = describe (h) + "   |   right-click: automation clip / freeze";
        else if (hb >= 0 && buttons[(size_t) hb].hint) hint = buttons[(size_t) hb].hint();
        else if (! advanced && heroArea.contains (e.position)) hint = heroHint (e.position);
        else hint = zoneHint (zoneAt (e.position));
        if (h != hover || hb != hoverButton || hint != hintText) { hover = h; hoverButton = hb; hintText = hint; repaint(); }
    }
    void mouseExit (const juce::MouseEvent&) override { hover = hoverButton = -1; hintText = {}; repaint(); }

    void mouseDown (const juce::MouseEvent& e0) override
    {
        const auto e = scaled (e0);
        const auto pos = e.position;
        if (const int i = ctlAt (pos); i >= 0)
        {
            if (e.mods.isPopupMenu()) { ctlMenu (i); return; }
            auto& c = ctls[(size_t) i];
            auto* p = param (c.idx);
            switch (c.kind)
            {
                case Ui::segments: { const int k = segmentAt (c, pos); if (k >= 0) setChoice (c.idx, k); return; }
                case Ui::toggle:   setNorm (c.idx, p->getValue() > 0.5f ? 0.0f : 1.0f); return;
                case Ui::stepper:  stepperClick (c, pos); return;
                default: break;
            }
            active = i;
            dragNorm = p->getValue();
            lastPos = pos;
            p->beginChangeGesture();
            if (c.kind == Ui::bar) dragBarTo (c, pos);
            return;
        }
        if (const int b = buttonAt (pos); b >= 0) { pressedButton = b; dragStarted = false; return; }
        if (! advanced && heroArea.contains (pos))
        {
            if (e.mods.isPopupMenu()) heroContext (pos, e);
            else heroDragging = heroDown (pos, e);
        }
    }
    void mouseDrag (const juce::MouseEvent& e0) override
    {
        const auto e = scaled (e0);
        if (heroDragging) { heroDrag (e.position, e); return; }
        if (pressedButton >= 0)
        {
            auto& b = buttons[(size_t) pressedButton];
            if (b.drag && ! dragStarted && e0.getDistanceFromDragStart() > 4) { dragStarted = true; dragOut (b); }
            return;
        }
        if (active < 0) return;
        auto& c = ctls[(size_t) active];
        if (c.kind == Ui::bar) { dragBarTo (c, e.position); return; }
        const float range = c.kind == Ui::fader ? juce::jmax (40.0f, c.r.getHeight() - 40.0f) : c.kind == Ui::number ? 140.0f : 170.0f;
        const float fine = e.mods.isShiftDown() || e.mods.isCtrlDown() ? 0.15f : 1.0f;
        dragNorm = juce::jlimit (0.0f, 1.0f, dragNorm + (lastPos.y - e.position.y) / range * fine);
        lastPos = e.position;
        param (c.idx)->setValueNotifyingHost (snap (c.idx, dragNorm));
    }
    void mouseUp (const juce::MouseEvent& e0) override
    {
        const auto e = scaled (e0);
        if (heroDragging) { heroDragging = false; heroUp(); return; }
        if (pressedButton >= 0)
        {
            const int b = pressedButton;
            pressedButton = -1;
            if (! dragStarted && buttons[(size_t) b].r.contains (e.position) && buttons[(size_t) b].click) buttons[(size_t) b].click();
            return;
        }
        if (active >= 0) { param (ctls[(size_t) active].idx)->endChangeGesture(); active = -1; hostkeys::giveBack (*this); return; }
        if (e.mouseWasClicked() && ! e.mods.isPopupMenu()) zoneClick (zoneAt (e.position));
        if (hostkeys::safeToGiveBack()) hostkeys::giveBack (*this);   // FL's keyboard plays notes again right away
    }
    void mouseDoubleClick (const juce::MouseEvent& e0) override
    {
        const auto e = scaled (e0);
        if (! advanced && ctlAt (e.position) < 0 && buttonAt (e.position) < 0 && heroArea.contains (e.position)) { heroDoubleClick (e.position, e); return; }
        if (const int i = ctlAt (e.position); i >= 0)
        {
            const auto k = ctls[(size_t) i].kind;
            if (k != Ui::segments && k != Ui::toggle && k != Ui::stepper)
                setNorm (ctls[(size_t) i].idx, param (ctls[(size_t) i].idx)->getDefaultValue());
        }
    }
    void mouseWheelMove (const juce::MouseEvent& e0, const juce::MouseWheelDetails& w) override
    {
        const auto e = scaled (e0);
        const int i = ctlAt (e.position);
        if (i < 0 || std::abs (w.deltaY) < 1.0e-4f) return;
        auto* p = param (ctls[(size_t) i].idx);
        const int steps = p->getNumSteps();
        const bool discrete = steps > 1 && steps < 2000;
        const float step = discrete ? 1.0f / (float) (steps - 1) : (e.mods.isShiftDown() ? 0.004f : 0.02f);
        setNorm (ctls[(size_t) i].idx, p->getValue() + (w.deltaY > 0 ? step : -step));
    }

    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        if (! wantsFiles()) return false;
        for (auto& f : files) if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true;
        return false;
    }
    void filesDropped (const juce::StringArray& files, int, int) override
    {
        for (auto& f : files)
            if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) { filesDroppedOnPlugin (juce::File (f)); return; }
    }

protected:
    // ---- plugin hooks
    virtual int heroHeight() const { return 0; }
    virtual void paintHero (juce::Graphics&, juce::Rectangle<float>) {}
    virtual bool heroDown (juce::Point<float>, const juce::MouseEvent&) { return false; }   // true = this click drags
    virtual void heroDrag (juce::Point<float>, const juce::MouseEvent&) {}
    virtual void heroUp() {}
    virtual void heroDoubleClick (juce::Point<float>, const juce::MouseEvent&) {}
    virtual void heroContext (juce::Point<float>, const juce::MouseEvent&) {}   // right click on the hero
    virtual int buttonColumns() const { return 1; }                              // side buttons: 1 column (default) or 2
    virtual juce::String heroHint (juce::Point<float>) { return {}; }
    virtual bool animating() const { return false; }   // true while the hero shows something moving (playhead)
    virtual bool wantsFiles() const { return false; }
    virtual void filesDroppedOnPlugin (const juce::File&) {}
    virtual void tick() {}   // 15x per second (e.g. a button text that follows the processor); call repaint() if needed
    virtual void paintSegmentIcon (juce::Graphics&, const Spec&, int /*index*/, juce::Rectangle<float>, juce::Colour) {}

    struct Button { juce::String text; std::function<void()> click; bool drag = false; std::function<juce::String()> hint; juce::Rectangle<float> r; };
    void addButton (juce::String text, std::function<void()> click, std::function<juce::String()> hint = {}, bool dragSource = false)
    { buttons.push_back ({ std::move (text), std::move (click), dragSource, std::move (hint), {} }); }
    virtual void dragOut (Button&) {}

    void status (const juce::String& s) { statusText = s; statusAt = juce::Time::getMillisecondCounter(); repaint(); }
    juce::Font font (float px) const { return lnf->font (px); }          // Silkscreen: labels, titles
    juce::Font monoFont (float px) const { return lnf->monoFont (px); }   // VT323: readouts, text
    juce::RangedAudioParameter* param (int idx) const { return proc.apvts.getParameter (proc.specs[(size_t) idx].id); }
    juce::RangedAudioParameter* param (const juce::String& id) const { return proc.apvts.getParameter (id); }
    float real (const juce::String& id) const { auto* p = param (id); return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f; }
    // hero edits: begin on mouse down, set while dragging, end on mouse up (one undo step in the host)
    void editBegin (std::initializer_list<const char*> ids) { for (auto* id : ids) if (auto* p = param (juce::String (id))) { p->beginChangeGesture(); editing.add (id); } }
    void editSet (const juce::String& id, float realValue) { if (auto* p = param (id)) p->setValueNotifyingHost (p->convertTo0to1 (realValue)); }
    void editEnd() { for (auto& id : editing) if (auto* p = param (id)) p->endChangeGesture(); editing.clear(); }

    BaseProcessor& proc;
    juce::SharedResourcePointer<LnF> lnf;
    juce::Rectangle<float> heroArea;
    std::vector<Button> buttons;
    bool advanced = false;

private:
    struct Ctl { int idx; Ui kind; juce::Rectangle<float> r; };
    enum Zone { zNone, zPrev, zName, zNext, zRandom, zUndo, zPage };

    // ---------------------------------------------------------------- what control a parameter gets
    Ui kindOf (const Spec& s) const
    {
        if (s.ui != Ui::automatic) return s.ui;
        if (! s.choices.isEmpty())
        {
            if (s.icons != noIcons) return Ui::segments;
            if (s.choices.size() == 2 && (s.choices[0] == "Off" || s.choices[1] == "On")) return Ui::toggle;
            int chars = 0; for (auto& c : s.choices) chars += c.length();
            return s.choices.size() <= 5 && chars <= 26 ? Ui::segments : Ui::stepper;
        }
        if (s.isInt) return Ui::number;
        if (s.id == "out" || s.id.endsWith ("Level")) return Ui::fader;
        if (s.min == 0.0f && s.max == 1.0f && s.unit.isEmpty()) return Ui::bar;
        return Ui::knob;
    }
    juce::Point<float> sizeOf (const Ctl& c, float z) const
    {
        const auto& s = proc.specs[(size_t) c.idx];
        auto f = font (16.0f), mf = monoFont (24.0f);
        const float label = juce::GlyphArrangement::getStringWidth (f, s.name.toUpperCase()) + 8.0f;
        switch (c.kind)
        {
            case Ui::bigKnob:  return { juce::jmax (110.0f * z, label, 100.0f), 130.0f * z + lblH };
            case Ui::knob:     return { juce::jmax (80.0f * z, label, 100.0f), 96.0f * z + lblH };
            case Ui::fader:    return { juce::jmax (60.0f, label, 100.0f), 120.0f * z + lblH };
            case Ui::bar:      return { juce::jmax (170.0f * z, label + 70.0f), lblH + 28.0f };
            case Ui::toggle:   return { label + 40.0f, 32.0f };
            case Ui::number:   return { juce::jmax (84.0f, label), lblH + 36.0f };
            case Ui::stepper:
            {
                float w = 0; for (auto& ch : s.choices) w = juce::jmax (w, juce::GlyphArrangement::getStringWidth (mf, ch));
                return { juce::jmax (label, w + 64.0f), lblH + 36.0f };
            }
            case Ui::segments:
            {
                float w = 0;
                if (s.icons != noIcons) w = (float) s.choices.size() * 42.0f;
                else for (auto& ch : s.choices) w += juce::GlyphArrangement::getStringWidth (mf, ch) + 20.0f;
                return { juce::jmax (label, w), lblH + 36.0f };
            }
            case Ui::automatic: case Ui::hidden: default: return { 0, 0 };
        }
    }

    // ---------------------------------------------------------------- layout
    void layout()
    {
        auto r = logicalBounds();
        r.removeFromTop (topH);
        r.removeFromBottom (footerH);
        r = r.reduced (16.0f, 8.0f);
        heroArea = {};
        for (auto& b : buttons) b.r = {};
        if (! advanced && heroHeight() > 0)
        {
            auto h = r.removeFromTop ((float) heroHeight());
            if (! buttons.empty())
            {
                const int cols = juce::jmax (1, buttonColumns()), rows = ((int) buttons.size() + cols - 1) / cols;
                auto area = h.removeFromRight (150.0f * (float) cols + 8.0f * (float) (cols - 1));
                h.removeFromRight (10.0f);
                const float bh = juce::jmin (44.0f, (area.getHeight() - 6.0f * (float) (rows - 1)) / (float) rows);
                for (int c = 0; c < cols; ++c)
                {
                    auto colR = area.removeFromLeft (150.0f);
                    area.removeFromLeft (8.0f);
                    for (int i = c * rows; i < juce::jmin ((int) buttons.size(), (c + 1) * rows); ++i)
                    { buttons[(size_t) i].r = colR.removeFromTop (bh); colR.removeFromTop (6.0f); }
                }
            }
            heroArea = h;
            r.removeFromTop (10.0f);
        }
        ctls.clear();
        for (size_t i = 0; i < proc.specs.size(); ++i)
        {
            const auto& s = proc.specs[i];
            const auto k = kindOf (s);
            if (k == Ui::hidden || (! advanced && ! s.main)) continue;
            ctls.push_back ({ (int) i, k, {} });
        }
        // the biggest control size at which every row fits, rows spread evenly
        const float gap = 16.0f;
        std::vector<std::vector<int>> rows;
        float z = 1.0f;
        for (; z >= 0.4f; z -= 0.05f)
        {
            rows.clear();
            float x = 0, total = 0, rowH = 0;
            rows.push_back ({});
            for (int i = 0; i < (int) ctls.size(); ++i)
            {
                const auto sz = sizeOf (ctls[(size_t) i], z);
                if (! rows.back().empty() && x + sz.x > r.getWidth()) { rows.push_back ({}); total += rowH + 8.0f; x = 0; rowH = 0; }
                rows.back().push_back (i);
                x += sz.x + gap;
                rowH = juce::jmax (rowH, sz.y);
            }
            total += rowH;
            if (total <= r.getHeight()) break;
        }
        float used = 0;
        std::vector<float> heights;
        for (auto& row : rows) { float h = 0; for (int i : row) h = juce::jmax (h, sizeOf (ctls[(size_t) i], z).y); heights.push_back (h); used += h; }
        const float vgap = juce::jmax (8.0f, (r.getHeight() - used) / (float) (rows.size() + 1));
        float y = r.getY() + juce::jmin (vgap, 24.0f);
        for (size_t ri = 0; ri < rows.size(); ++ri)
        {
            float w = 0;
            for (int i : rows[ri]) w += sizeOf (ctls[(size_t) i], z).x;
            const float g2 = rows[ri].size() > 1 ? juce::jlimit (gap, 44.0f, (r.getWidth() - w) / (float) (rows[ri].size() - 1)) : 0.0f;
            float x = r.getX() + juce::jmax (0.0f, (r.getWidth() - w - g2 * (float) (rows[ri].size() - 1)) * 0.5f);
            for (int i : rows[ri])
            {
                const auto sz = sizeOf (ctls[(size_t) i], z);
                ctls[(size_t) i].r = { x, y + (heights[ri] - sz.y), sz.x, sz.y };   // bottom-aligned in the row
                x += sz.x + g2;
            }
            y += heights[ri] + vgap;
        }
    }

    // ---------------------------------------------------------------- painting
    static constexpr float topH = 64.0f, footerH = 28.0f;
    juce::Rectangle<float> zoneRect (Zone z) const
    {
        const float w = logicalBounds().getWidth(), y = 14.0f, h = 36.0f;
        switch (z)   // right to left: START (ADVANCED) | undo | A (RANDOMIZE) | next | SELECT (MY PRESETS) | prev
        {
            case zPage:   return { w - 160.0f, y, 144.0f, h };
            case zUndo:   return { w - 204.0f, y, 36.0f, h };
            case zRandom: return { w - 372.0f, y, 160.0f, h };
            case zNext:   return { w - 412.0f, y, 32.0f, h };
            case zName:   return { w - 652.0f, y, 232.0f, h };
            case zPrev:   return { w - 692.0f, y, 32.0f, h };
            case zNone: default: return {};
        }
    }
    Zone zoneAt (juce::Point<float> p) const
    { for (auto z : { zPrev, zName, zNext, zRandom, zUndo, zPage }) if (zoneRect (z).contains (p)) return z; return zNone; }
    juce::String zoneHint (Zone z) const
    {
        switch (z)
        {
            case zPrev: case zNext: return "previous / next factory preset";
            case zName:   return "SELECT: factory presets, MY PRESETS, save your sound, THEME";
            case zRandom: return "A: RANDOMIZE - a new sound in musical ranges (frozen controls stay)";
            case zUndo:   return proc.canUndo() ? "undo the last RANDOMIZE / preset load" : "nothing to undo";
            case zPage:   return advanced ? "START: back to the main controls" : "START: all parameters";
            case zNone: default: return {};
        }
    }
    void paintTop (juce::Graphics& g)
    {
        const float w = logicalBounds().getWidth();
        g.setColour (col::ink);
        g.fillRect (0.0f, topH - 4.0f, w, 4.0f);   // the console's lower edge
        g.setFont (font (24.0f));
        g.drawText (name, 16, 6, 260, 32, juce::Justification::centredLeft);
        g.setFont (monoFont (20.0f));
        g.drawText (sub, 17, 36, (int) zoneRect (zPrev).getX() - 30, 22, juce::Justification::centredLeft, true);
        auto box = [&] (Zone z, const juce::String& t, bool lit, bool pill, bool enabled = true)
        {
            const auto r = zoneRect (z);
            const bool over = zoneAt (mouse) == z && enabled;
            const bool fill = lit || over;
            juce::Path pth;
            pth.addRoundedRectangle (r.reduced (1.0f), pill ? r.getHeight() * 0.5f : 0.0f);
            g.setColour (fill ? col::ink : col::ground);
            g.fillPath (pth);
            g.setColour (col::ink.withAlpha (enabled ? 1.0f : 0.4f));
            g.strokePath (pth, juce::PathStrokeType (2.0f));
            g.setColour (fill ? col::inkInverse : col::ink.withAlpha (enabled ? 1.0f : 0.4f));
            g.setFont (z == zName ? monoFont (24.0f) : font (16.0f));
            g.drawText (t, r.reduced (10, 0), juce::Justification::centred, true);
        };
        box (zPrev, "<", false, false);
        box (zName, proc.presetName(), false, true);   // SELECT
        box (zNext, ">", false, false);
        box (zRandom, "RANDOMIZE", true, true);        // A: the main action
        box (zUndo, "<<", false, false, proc.canUndo());
        box (zPage, advanced ? "MAIN" : "ADVANCED", advanced, true);   // START
    }
    void paintButton (juce::Graphics& g, const Button& b, bool over)
    {
        if (b.r.isEmpty()) return;
        const auto r = b.r.reduced (1.0f);
        g.setColour (over ? col::ink : col::ground);
        g.fillRect (r);
        g.setColour (col::ink);
        g.drawRect (r, b.drag ? 3.0f : 2.0f);   // a drag source has the thicker frame
        g.setColour (over ? col::inkInverse : col::ink);
        g.setFont (font (16.0f));
        g.drawText (b.text, b.r.reduced (6, 0), juce::Justification::centred, true);
    }
    void paintCtl (juce::Graphics& g, const Ctl& c, bool hot)
    {
        const auto& s = proc.specs[(size_t) c.idx];
        auto* p = param (c.idx);
        const float v = p->getValue();
        auto r = c.r;
        auto label = [&] (juce::Rectangle<float> a, juce::Justification j)
        { g.setColour (col::ink); g.setFont (font (16.0f)); g.drawText (s.name.toUpperCase(), a, j, true); };
        auto value = [&] (juce::Rectangle<float> a, juce::Justification j, float px = 24.0f)
        { g.setColour (col::ink); g.setFont (monoFont (px)); g.drawText (p->getCurrentValueAsText(), a, j, true); };
        auto frame = [&] (juce::Rectangle<float> a, float t) { g.setColour (col::ink); g.drawRect (a, t); };
        juce::ignoreUnused (hot);

        switch (c.kind)
        {
            case Ui::knob:
            case Ui::bigKnob:
            {
                const bool big = c.kind == Ui::bigKnob;
                label (r.removeFromTop (lblH), juce::Justification::centred);
                auto valR = r.removeFromBottom (valH);
                const float d = juce::jmin (r.getWidth(), r.getHeight()) - 6.0f, rad = d * 0.5f;
                const auto ctr = r.getCentre();
                const float a0 = juce::MathConstants<float>::pi * 1.2f, a1 = juce::MathConstants<float>::pi * 2.8f, a = a0 + v * (a1 - a0);
                const int nTick = big ? 24 : 12;
                for (int t = 0; t <= nTick; ++t)   // square pixel ticks around the knob
                {
                    const float ta = a0 + (a1 - a0) * (float) t / (float) nTick;
                    const bool lit = (float) t / (float) nTick <= v + 1.0e-4f;
                    const auto q = ctr.getPointOnCircumference (rad - 2.0f, ta);
                    const float sz = big ? 6.0f : 5.0f;
                    if (lit) { g.setColour (col::accent); g.fillRect (q.x - sz * 0.5f, q.y - sz * 0.5f, sz, sz); }
                    g.setColour (col::ink);
                    g.drawRect (q.x - sz * 0.5f, q.y - sz * 0.5f, sz, sz, 1.0f);
                }
                const float kr = rad - (big ? 12.0f : 10.0f);
                g.setColour (big ? col::accent : col::ground);   // A button = the big knob
                g.fillEllipse (ctr.x - kr, ctr.y - kr, 2 * kr, 2 * kr);
                g.setColour (col::ink);
                g.drawEllipse (ctr.x - kr, ctr.y - kr, 2 * kr, 2 * kr, 2.0f);
                const auto tip = ctr.getPointOnCircumference (kr - 3.0f, a), mid = ctr.getPointOnCircumference (kr * 0.3f, a);
                g.setColour (big ? col::inkInverse : col::ink);
                g.drawLine ({ mid, tip }, 3.0f);
                value (valR, juce::Justification::centred);
                break;
            }
            case Ui::fader:
            {
                label (r.removeFromTop (lblH), juce::Justification::centred);
                value (r.removeFromBottom (valH), juce::Justification::centred);
                auto slot = r.reduced (0, 6).withSizeKeepingCentre (14.0f, r.getHeight() - 12.0f);
                g.setColour (col::well);
                g.fillRect (slot);
                const float fy = slot.getBottom() - slot.getHeight() * v;
                g.setColour (col::accent);
                g.fillRect (slot.withTop (fy));
                frame (slot, 2.0f);
                for (int t = 0; t <= 6; ++t)
                {
                    const float ty = slot.getBottom() - slot.getHeight() * (float) t / 6.0f;
                    g.setColour (col::ink);
                    g.drawLine (slot.getX() - 8, ty, slot.getX() - 3, ty, 1.0f);
                    g.drawLine (slot.getRight() + 3, ty, slot.getRight() + 8, ty, 1.0f);
                }
                const juce::Rectangle<float> knob (slot.getCentreX() - 15, fy - 5, 30, 10);
                g.setColour (col::ink);
                g.fillRect (knob);
                g.setColour (col::inkInverse);
                g.fillRect (knob.getX() + 3, knob.getCentreY() - 0.5f, knob.getWidth() - 6, 1.0f);
                break;
            }
            case Ui::bar:
            {
                auto top = r.removeFromTop (lblH);
                label (top, juce::Justification::centredLeft);
                value (top, juce::Justification::centredRight);
                auto track = r.reduced (0, 4).withHeight (18.0f);
                const int n = juce::jlimit (8, 24, juce::roundToInt (track.getWidth() / 12.0f));   // segmented bar
                const float cw = track.getWidth() / (float) n;
                const int lit = juce::roundToInt (v * (float) n);
                for (int k = 0; k < n; ++k)
                {
                    const auto cell = juce::Rectangle<float> (track.getX() + cw * (float) k, track.getY(), cw, track.getHeight()).reduced (1.0f, 0.0f);
                    if (k < lit) { g.setColour (col::accent); g.fillRect (cell); }
                    g.setColour (col::ink);
                    g.drawRect (cell, 1.0f);
                }
                break;
            }
            case Ui::toggle:
            {
                const bool on = v > 0.5f;
                auto sw = r.withSizeKeepingCentre (r.getWidth(), 24.0f).removeFromLeft (24.0f);
                g.setColour (on ? col::accent : col::well);
                g.fillRect (sw);
                frame (sw, 2.0f);
                if (on) { g.setColour (col::ink); g.fillRect (sw.reduced (7.0f)); }
                label (r.withTrimmedLeft (34.0f), juce::Justification::centredLeft);
                break;
            }
            case Ui::number:
            {
                label (r.removeFromTop (lblH), juce::Justification::centred);
                auto bx = r.reduced (2, 2);
                g.setColour (col::well);
                g.fillRect (bx);
                frame (bx, 2.0f);
                value (bx, juce::Justification::centred, 24.0f);
                break;
            }
            case Ui::stepper:
            {
                label (r.removeFromTop (lblH), juce::Justification::centredLeft);
                auto bx = r.reduced (0, 2);
                g.setColour (col::well);
                g.fillRect (bx);
                frame (bx, 2.0f);
                g.setColour (col::ink);
                g.setFont (font (16.0f));
                g.drawText ("<", bx.removeFromLeft (26.0f), juce::Justification::centred, false);
                g.drawText (">", bx.removeFromRight (26.0f), juce::Justification::centred, false);
                value (bx, juce::Justification::centred, 24.0f);
                break;
            }
            case Ui::segments:
            {
                label (r.removeFromTop (lblH), juce::Justification::centredLeft);
                const int n = s.choices.size(), sel = juce::roundToInt (v * (float) (n - 1));
                for (int k = 0; k < n; ++k)   // square tiles; the selected one is a full ink block
                {
                    const auto cell = segmentRect (c, k).reduced (1.0f, 0.0f);
                    const bool on = k == sel;
                    if (on) { g.setColour (col::ink); g.fillRect (cell); }
                    else { g.setColour (col::well); g.fillRect (cell); g.setColour (col::ink); g.drawRect (cell, 2.0f); }
                    const auto ink = on ? col::inkInverse : col::ink;
                    if (s.icons != noIcons) paintSegmentIcon (g, s, k, cell.reduced (5.0f, 6.0f), ink);
                    else { g.setColour (ink); g.setFont (monoFont (24.0f)); g.drawText (s.choices[k], cell, juce::Justification::centred, true); }
                }
                break;
            }
            case Ui::automatic: case Ui::hidden: default: break;
        }
        if (proc.isFrozen (s.id))   // a little snowflake: RANDOMIZE leaves it alone
        {
            const juce::Point<float> sc (c.r.getRight() - 6.0f, c.r.getY() + 8.0f);
            g.setColour (col::ink);
            for (int k = 0; k < 3; ++k)
            {
                const float a = juce::MathConstants<float>::pi * (float) k / 3.0f;
                g.drawLine (sc.x - 5.0f * std::cos (a), sc.y - 5.0f * std::sin (a), sc.x + 5.0f * std::cos (a), sc.y + 5.0f * std::sin (a), 2.0f);
            }
        }
    }
    static constexpr float lblH = 22.0f, valH = 22.0f;

    juce::Rectangle<float> segmentRect (const Ctl& c, int k) const
    {
        const auto& s = proc.specs[(size_t) c.idx];
        auto row = c.r.withTrimmedTop (lblH + 2.0f).withHeight (32.0f);
        if (s.icons != noIcons) { const float w = row.getWidth() / (float) s.choices.size(); return { row.getX() + w * (float) k, row.getY(), w, row.getHeight() }; }
        float total = 0; std::vector<float> ws;
        for (auto& ch : s.choices) { ws.push_back (juce::GlyphArrangement::getStringWidth (monoFont (24.0f), ch) + 20.0f); total += ws.back(); }
        const float scale = row.getWidth() / juce::jmax (1.0f, total);
        float x = row.getX();
        for (int i = 0; i < k; ++i) x += ws[(size_t) i] * scale;
        return { x, row.getY(), ws[(size_t) k] * scale, row.getHeight() };
    }
    int segmentAt (const Ctl& c, juce::Point<float> p) const
    { for (int k = 0; k < proc.specs[(size_t) c.idx].choices.size(); ++k) if (segmentRect (c, k).contains (p)) return k; return -1; }

    // ---------------------------------------------------------------- actions
    int ctlAt (juce::Point<float> p) const { for (size_t i = 0; i < ctls.size(); ++i) if (ctls[i].r.contains (p)) return (int) i; return -1; }
    int buttonAt (juce::Point<float> p) const { for (size_t i = 0; i < buttons.size(); ++i) if (buttons[i].r.contains (p)) return (int) i; return -1; }
    float snap (int idx, float n) const
    {
        const int steps = param (idx)->getNumSteps();
        if (steps > 1 && steps < 2000 && (proc.specs[(size_t) idx].isInt || ! proc.specs[(size_t) idx].choices.isEmpty()))
            return std::round (n * (float) (steps - 1)) / (float) (steps - 1);
        return n;
    }
    void setNorm (int idx, float n) { BaseProcessor::setNorm (*param (idx), snap (idx, juce::jlimit (0.0f, 1.0f, n))); }
    void setChoice (int idx, int k) { const int n = proc.specs[(size_t) idx].choices.size(); setNorm (idx, n > 1 ? (float) k / (float) (n - 1) : 0.0f); }
    void dragBarTo (const Ctl& c, juce::Point<float> p)
    { param (c.idx)->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (p.x - c.r.getX()) / c.r.getWidth())); }
    void stepperClick (const Ctl& c, juce::Point<float> p)
    {
        const auto& s = proc.specs[(size_t) c.idx];
        const int n = s.choices.size(), cur = juce::roundToInt (param (c.idx)->getValue() * (float) (n - 1));
        if (p.x < c.r.getX() + 24.0f) { setChoice (c.idx, (cur - 1 + n) % n); return; }
        if (p.x > c.r.getRight() - 24.0f) { setChoice (c.idx, (cur + 1) % n); return; }
        juce::PopupMenu m;
        for (int k = 0; k < n; ++k) m.addItem (k + 1, s.choices[k], true, k == cur);
        juce::Component::SafePointer<Editor> safe (this);
        const int idx = c.idx;
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, idx] (int r)
        { if (safe != nullptr && r > 0) safe->setChoice (idx, r - 1); });
    }
    juce::String describe (int ci) const
    {
        const auto& s = proc.specs[(size_t) ctls[(size_t) ci].idx];
        return s.name + ": " + param (ctls[(size_t) ci].idx)->getCurrentValueAsText() + (proc.isFrozen (s.id) ? "  (frozen)" : "");
    }
    void ctlMenu (int ci)
    {
        const int idx = ctls[(size_t) ci].idx;
        auto* p = param (idx);
        const auto id = proc.specs[(size_t) idx].id;
        juce::PopupMenu hostItems;
        if (auto* ctx = getHostContext())
            if (auto menu = ctx->getContextMenuForParameter (p))
                hostItems = menu->getEquivalentPopupMenu();
        bool hostAutomation = false;
        for (juce::PopupMenu::MenuItemIterator it (hostItems, true); it.next();)
            hostAutomation = hostAutomation || it.getItem().text.containsIgnoreCase ("automat");
        juce::Component::SafePointer<Editor> safe (this);
        juce::PopupMenu m;
        m.addSectionHeader (proc.specs[(size_t) idx].name.toUpperCase());
        if (! hostAutomation)   // FL offers none for plugins: make it FL's "last tweaked" control and say where to click
            m.addItem ("Create automation clip", [safe, p]
            {
                if (safe == nullptr) return;
                BaseProcessor::setNorm (*p, p->getValue());
                safe->status (p->getName (64) + " is FL's last tweaked control now  |  in FL: Tools > Last tweaked > Create automation clip");
            });
        if (hostItems.getNumItems() > 0) { m.addSeparator(); for (juce::PopupMenu::MenuItemIterator it (hostItems); it.next();) m.addItem (it.getItem()); }
        m.addSeparator();
        const bool frozen = proc.isFrozen (id);
        m.addItem (frozen ? "Unfreeze (RANDOMIZE may change it)" : "Freeze (RANDOMIZE leaves it alone)", true, frozen, [safe, id]
        { if (safe != nullptr) { safe->proc.toggleFrozen (id); safe->repaint(); } });
        m.addItem ("Reset to default", [safe, p] { if (safe != nullptr) BaseProcessor::setNorm (*p, p->getDefaultValue()); });
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition());
    }
    void zoneClick (Zone z)
    {
        switch (z)
        {
            case zPrev:   proc.stepPreset (-1); break;
            case zNext:   proc.stepPreset (1); break;
            case zName:   presetMenu(); break;
            case zRandom: proc.randomize(); status ("RANDOMIZE: new sound  |  undo with the arrow, freeze a control with right-click"); break;
            case zUndo:   if (proc.canUndo()) { proc.undo(); status ("back to the sound before"); } break;
            case zPage:   showAdvanced (! advanced); break;
            case zNone: default: break;
        }
        repaint();
    }
    void presetMenu()
    {
        juce::PopupMenu m;
        m.addSectionHeader ("FACTORY");
        for (int i = 0; i < (int) proc.presets.size(); ++i) m.addItem (i + 1, proc.presets[(size_t) i].name, true, i == proc.presetIndex());
        m.addSectionHeader ("MY PRESETS");
        const auto files = proc.userPresets();
        for (int i = 0; i < files.size(); ++i)
            m.addItem (1000 + i, files[i].getFileNameWithoutExtension(), true, proc.presetIndex() < 0 && files[i].getFileNameWithoutExtension() == proc.presetName());
        m.addSectionHeader ("THEME");
        for (int i = 0; i < col::numThemes; ++i) m.addItem (3000 + i, col::themes[i].name, true, i == proc.themeIndex());
        m.addSectionHeader ("MY SOUND");
        m.addItem (2000, "Save current sound...");
        m.addItem (2001, "Open the presets folder", true, false);
        juce::Component::SafePointer<Editor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, files] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            if (r < 1000) safe->proc.applyPreset (r - 1);
            else if (r < 2000) { if (safe->proc.loadUserPreset (files[r - 1000])) safe->status ("loaded " + files[r - 1000].getFileNameWithoutExtension()); }
            else if (r >= 3000) { safe->proc.setTheme (r - 3000); safe->useTheme(); safe->status ("theme: " + juce::String (col::themes[r - 3000].name)); }
            else if (r == 2000) safe->askNameAndSave();
            else { BaseProcessor::userFolder().createDirectory(); BaseProcessor::userFolder().startAsProcess(); }
            safe->repaint();
        });
    }
    void askNameAndSave()
    {
        juce::String suggestion;   // a free name, so OK alone works even when the host keeps the keyboard
        for (int n = 1; n < 1000; ++n)
            if (! BaseProcessor::userFolder().getChildFile ("My Sound " + juce::String (n) + ".boypreset").exists())
            { suggestion = "My Sound " + juce::String (n); break; }
        auto* aw = new juce::AlertWindow ("SAVE PRESET", "Name for this sound:", juce::MessageBoxIconType::NoIcon, this);
        aw->setLookAndFeel (&lnf.get());
        aw->addTextEditor ("name", suggestion);
        aw->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        juce::Component::SafePointer<Editor> safe (this);
        aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int r)
        {
            aw->setLookAndFeel (nullptr);
            if (safe == nullptr || r != 1) return;
            const auto n = aw->getTextEditorContents ("name").trim();
            safe->status (safe->proc.saveUserPreset (n) ? "saved to MY PRESETS: " + n : juce::String ("could not save"));
        }), true);
    }

    void timerCallback() override
    {
        tick();
        bool dirty = animating();
        for (size_t i = 0; i < proc.specs.size(); ++i)
        {
            const float v = proc.get ((int) i);
            if (v != seen[i]) { seen[i] = v; dirty = true; }
        }
        const auto key = proc.presetName() + "|" + proc.apvts.state.getProperty ("frozen").toString() + "|" + proc.apvts.state.getProperty ("theme").toString();
        if (key != seenKey) { seenKey = key; dirty = true; }
        if (statusText.isNotEmpty() && juce::Time::getMillisecondCounter() - statusAt > 4000) { statusText = {}; dirty = true; }
        mouse = getMouseXYRelative().toFloat() / uiScale;
        if (! rendererChosen)   // JUCE's software renderer: no GPU device / swap chain per window = less RAM
            if (auto* peer = getPeer())
            {
                rendererChosen = true;
                const auto engines = peer->getAvailableRenderingEngines();
                for (int i = 0; i < engines.size(); ++i)
                    if (engines[i].containsIgnoreCase ("software") && peer->getCurrentRenderingEngine() != i) { peer->setCurrentRenderingEngine (i); break; }
            }
        if (dirty) repaint();
        if (hostkeys::safeToGiveBack()) hostkeys::giveBack (*this);   // e.g. after a menu, dialog or file chooser
    }
    bool rendererChosen = false;

    juce::String name, sub, statusText, hintText, seenKey;
    juce::uint32 statusAt = 0;
    std::vector<Ctl> ctls;
    std::vector<float> seen;
    juce::StringArray editing;
    juce::Point<float> lastPos, mouse;
    float dragNorm = 0;
    int hover = -1, hoverButton = -1, active = -1, pressedButton = -1;
    bool heroDragging = false, dragStarted = false;
    juce::Point<int> initialSize;
    float uiScale = 1.0f;
    bool sizeReady = false;
    static constexpr float minScale = 0.75f, maxScale = 2.0f;
    juce::Rectangle<float> logicalBounds() const { return { 0.0f, 0.0f, (float) initialSize.x, (float) initialSize.y }; }
    juce::MouseEvent scaled (const juce::MouseEvent& e) const { return e.withNewPosition (e.position / uiScale); }
};

// a small helper for heroes: a value as text in the hero's corner
inline void heroLabel (juce::Graphics& g, const juce::Font& f, juce::Rectangle<float> r, const juce::String& t, juce::Colour c, juce::Justification j)
{ g.setColour (c); g.setFont (f); g.drawText (t, r, j, true); }

} // namespace boy
