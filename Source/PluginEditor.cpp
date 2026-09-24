#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour bgTop       { 0xff2b1714 };
    const juce::Colour bgBottom    { 0xff0b0a09 };
    const juce::Colour panel       { 0xee17120f };
    const juce::Colour panel2      { 0xee231914 };
    const juce::Colour gold        { 0xffe7ad58 };
    const juce::Colour goldBright  { 0xffffd489 };
    const juce::Colour bronze      { 0xff8b542e };
    const juce::Colour cream       { 0xfff2e5cf };
    const juce::Colour wood        { 0xff5c2d1b };
    const juce::Colour darkWood    { 0xff26130e };

    void drawSection(juce::Graphics& g, juce::Rectangle<float> r,
                     const juce::String& title)
    {
        g.setColour(panel);
        g.fillRoundedRectangle(r, 8.0f);
        g.setColour(bronze.withAlpha(0.8f));
        g.drawRoundedRectangle(r, 8.0f, 1.0f);
        g.setColour(gold);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(title, r.removeFromTop(22.0f).reduced(8, 0),
                   juce::Justification::centredLeft);
    }
}

AcousticGuitarByJGKAudioProcessorEditor::AcousticGuitarByJGKAudioProcessorEditor(
    AcousticGuitarByJGKAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setResizable(false, false);
    setSize(1200, 800);

    const char* roots[12] =
        { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    for (int i = 0; i < 12; ++i)
    {
        rootButtons[(size_t) i] = std::make_unique<juce::TextButton>(roots[i]);
        styleButton(*rootButtons[(size_t) i]);
        rootButtons[(size_t) i]->onClick = [this, i]
        {
            processor.setSelectedChord(i, getIntParam("chordType"), true);
            refreshSelections();
        };
        addAndMakeVisible(*rootButtons[(size_t) i]);
    }

    const char* types[8] =
        { "Major", "Minor", "7", "Maj7", "Min7", "Sus2", "Sus4", "Add9" };
    for (int i = 0; i < 8; ++i)
    {
        typeButtons[(size_t) i] = std::make_unique<juce::TextButton>(types[i]);
        styleButton(*typeButtons[(size_t) i]);
        typeButtons[(size_t) i]->onClick = [this, i]
        {
            processor.setSelectedChord(getIntParam("root"), i, true);
            refreshSelections();
        };
        addAndMakeVisible(*typeButtons[(size_t) i]);
    }

    voicingBox.addItem("Open / Standard", 1);
    voicingBox.addItem("Higher", 2);
    voicingBox.addItem("Wide", 3);
    inversionBox.addItem("Root position", 1);
    inversionBox.addItem("1st inversion", 2);
    inversionBox.addItem("2nd inversion", 3);

    for (auto* box : { &voicingBox, &inversionBox, &presetBox })
    {
        box->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff16110f));
        box->setColour(juce::ComboBox::textColourId, cream);
        box->setColour(juce::ComboBox::outlineColourId, bronze);
        box->setColour(juce::ComboBox::arrowColourId, gold);
        addAndMakeVisible(*box);
    }

    comboAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.getAPVTS(), "voicing", voicingBox));
    comboAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.getAPVTS(), "inversion", inversionBox));

    const char* presets[8] =
        { "Straight 8ths", "Pop Acoustic", "Singer-Songwriter", "Slow Ballad",
          "Driving Acoustic", "Palm Muted", "Folk", "16th Strum" };
    for (int i = 0; i < 8; ++i)
        presetBox.addItem(presets[i], i + 1);
    presetBox.setSelectedId(3, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedId() > 0)
            processor.setPatternPreset(presetBox.getSelectedId() - 1);
        refreshSelections();
    };

    styleButton(capoMinus);
    styleButton(capoPlus);
    addAndMakeVisible(capoMinus);
    addAndMakeVisible(capoPlus);

    capoLabel.setJustificationType(juce::Justification::centred);
    capoLabel.setColour(juce::Label::textColourId, goldBright);
    capoLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff15110f));
    capoLabel.setColour(juce::Label::outlineColourId, bronze);
    capoLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    addAndMakeVisible(capoLabel);

    capoMinus.onClick = [this]
    {
        setIntParam("capo", juce::jmax(0, getIntParam("capo") - 1));
        refreshSelections();
    };
    capoPlus.onClick = [this]
    {
        setIntParam("capo", juce::jmin(12, getIntParam("capo") + 1));
        refreshSelections();
    };

    configureKnob(strumKnob, "strum", false, " ms");
    configureKnob(humanizeKnob, "humanize", true);
    configureKnob(palmKnob, "mute", true);
    configureKnob(dynamicsKnob, "dynamics", true);
    configureKnob(toneKnob, "tone", true);
    configureKnob(roomKnob, "room", true);
    configureKnob(tightLooseKnob, "tightLoose", true);
    configureKnob(swingKnob, "swing", true);
    configureKnob(outputKnob, "output", false, " dB");

    configureKnob(scratchVolKnob, "scratchVolume", true);
    configureKnob(scratchLengthKnob, "scratchLength", false, " s");
    configureKnob(scratchTimingKnob, "scratchTiming", true);
    configureKnob(percVolKnob, "percVolume", true);
    configureKnob(percToneKnob, "percTone", true);
    configureKnob(percMixKnob, "percMix", true);

    const char* modes[3] = { "STRUM", "FINGERPICK", "PERCUSSION" };
    for (int i = 0; i < 3; ++i)
    {
        modeButtons[(size_t) i] = std::make_unique<juce::TextButton>(modes[i]);
        styleButton(*modeButtons[(size_t) i]);
        modeButtons[(size_t) i]->onClick = [this, i]
        {
            setIntParam("performanceMode", i);
            refreshSelections();
        };
        addAndMakeVisible(*modeButtons[(size_t) i]);
    }

    const char* speeds[3] = { "1/4", "1/8", "1/16" };
    for (int i = 0; i < 3; ++i)
    {
        speedButtons[(size_t) i] = std::make_unique<juce::TextButton>(speeds[i]);
        styleButton(*speedButtons[(size_t) i]);
        speedButtons[(size_t) i]->onClick = [this, i]
        {
            setIntParam("patternSpeed", i);
            refreshSelections();
        };
        addAndMakeVisible(*speedButtons[(size_t) i]);
    }

    const char* playModes[3] = { "LATCH", "HOLD", "MIDI" };
    for (int i = 0; i < 3; ++i)
    {
        playModeButtons[(size_t) i] = std::make_unique<juce::TextButton>(playModes[i]);
        styleButton(*playModeButtons[(size_t) i]);
        playModeButtons[(size_t) i]->onClick = [this, i]
        {
            setIntParam("playMode", i);
            if (i == 2) processor.clearUIChord();
            refreshSelections();
        };
        addAndMakeVisible(*playModeButtons[(size_t) i]);
    }

    for (int i = 0; i < 5; ++i)
    {
        variationButtons[(size_t) i] =
            std::make_unique<juce::TextButton>(i == 0 ? "OFF" : juce::String(i));
        styleButton(*variationButtons[(size_t) i]);
        variationButtons[(size_t) i]->onClick = [this, i]
        {
            setIntParam("variation", i);
            refreshSelections();
        };
        addAndMakeVisible(*variationButtons[(size_t) i]);
    }

    for (int i = 0; i < 8; ++i)
    {
        patternButtons[(size_t) i] = std::make_unique<juce::TextButton>();
        styleButton(*patternButtons[(size_t) i]);
        patternButtons[(size_t) i]->onClick = [this, i]
        {
            int next = selectedEvent;
            if (next < 0)
                next = (getIntParam("step" + juce::String(i + 1)) + 1) % 7;
            processor.setPatternStep(i, next);
            refreshSelections();
        };
        addAndMakeVisible(*patternButtons[(size_t) i]);
    }

    const char* eventNames[7] = { "REST", "DOWN", "UP", "MUTE", "CHOKE", "SCR", "PERC" };
    for (int i = 0; i < 7; ++i)
    {
        eventButtons[(size_t) i] = std::make_unique<juce::TextButton>(eventNames[i]);
        styleButton(*eventButtons[(size_t) i]);
        eventButtons[(size_t) i]->onClick = [this, i]
        {
            selectedEvent = selectedEvent == i ? -1 : i;
            refreshSelections();
        };
        addAndMakeVisible(*eventButtons[(size_t) i]);
    }

    const char* padNames[12] =
        { "C", "G", "Am", "F", "Dm", "Em", "D", "A", "Bm", "E", "Bb", "F#" };
    const std::array<int, 12> padRoots { 0,7,9,5,2,4,2,9,11,4,10,6 };
    const std::array<int, 12> padTypes { 0,0,1,0,1,1,0,0,1,0,0,0 };

    for (int i = 0; i < 12; ++i)
    {
        chordPads[(size_t) i] = std::make_unique<juce::TextButton>(padNames[i]);
        styleButton(*chordPads[(size_t) i]);

        auto* button = chordPads[(size_t) i].get();
        button->onStateChange = [this, i, padRoots, padTypes, button]
        {
            if (button->isDown())
                processor.pressChordFromUI(padRoots[(size_t) i], padTypes[(size_t) i]);
            else
                processor.releaseChordFromUI();
        };
        addAndMakeVisible(*button);
    }

    const char* scratchNames[4] = { "DOWN", "UP", "RAKE", "SLAP" };
    const char* percNames[4] = { "BODY", "THUMB", "KNUCK", "SLAP" };
    for (int i = 0; i < 4; ++i)
    {
        scratchButtons[(size_t) i] = std::make_unique<juce::TextButton>(scratchNames[i]);
        percussionButtons[(size_t) i] = std::make_unique<juce::TextButton>(percNames[i]);
        styleButton(*scratchButtons[(size_t) i]);
        styleButton(*percussionButtons[(size_t) i]);

        scratchButtons[(size_t) i]->onClick = [this, i]
        {
            setIntParam("scratchType", i);
            refreshSelections();
        };
        percussionButtons[(size_t) i]->onClick = [this, i]
        {
            setIntParam("percType", i);
            refreshSelections();
        };
        addAndMakeVisible(*scratchButtons[(size_t) i]);
        addAndMakeVisible(*percussionButtons[(size_t) i]);
    }

    refreshSelections();
    startTimerHz(30);
}

AcousticGuitarByJGKAudioProcessorEditor::~AcousticGuitarByJGKAudioProcessorEditor()
{
    stopTimer();
}

void AcousticGuitarByJGKAudioProcessorEditor::styleButton(juce::TextButton& b)
{
    b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff241914));
    b.setColour(juce::TextButton::buttonOnColourId, gold);
    b.setColour(juce::TextButton::textColourOffId, cream);
    b.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff1a1009));
}

void AcousticGuitarByJGKAudioProcessorEditor::configureKnob(
    juce::Slider& s, const juce::String& id, bool percent, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                          juce::MathConstants<float>::pi * 2.75f, true);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 16);
    s.setScrollWheelEnabled(false);
    s.setColour(juce::Slider::rotarySliderFillColourId, gold);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, bronze.darker(0.6f));
    s.setColour(juce::Slider::thumbColourId, goldBright);
    s.setColour(juce::Slider::textBoxTextColourId, cream);
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff15110f));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    if (percent)
        s.textFromValueFunction = [] (double v)
        {
            return juce::String((int) std::round(v * 100.0)) + "%";
        };
    else if (suffix.isNotEmpty())
        s.textFromValueFunction = [suffix] (double v)
        {
            const int decimals = suffix == " s" ? 2 : suffix == " dB" ? 1 : 0;
            return juce::String(v, decimals) + suffix;
        };

    addAndMakeVisible(s);
    sliderAttachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getAPVTS(), id, s));
}

void AcousticGuitarByJGKAudioProcessorEditor::setIntParam(
    const juce::String& id, int value)
{
    if (auto* p = processor.getAPVTS().getParameter(id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1((float) value));
        p->endChangeGesture();
    }
}

int AcousticGuitarByJGKAudioProcessorEditor::getIntParam(const juce::String& id) const
{
    if (auto* v = processor.getAPVTS().getRawParameterValue(id))
        return (int) *v;
    return 0;
}

juce::String AcousticGuitarByJGKAudioProcessorEditor::eventText(int type)
{
    switch (type)
    {
        case AcousticGuitarByJGKAudioProcessor::Down:       return "D";
        case AcousticGuitarByJGKAudioProcessor::Up:         return "U";
        case AcousticGuitarByJGKAudioProcessor::Palm:       return "M";
        case AcousticGuitarByJGKAudioProcessor::Choke:      return "X";
        case AcousticGuitarByJGKAudioProcessor::Scratch:    return "SCR";
        case AcousticGuitarByJGKAudioProcessor::Percussion: return "P";
        default: return "-";
    }
}

void AcousticGuitarByJGKAudioProcessorEditor::refreshSelections()
{
    const int root = getIntParam("root");
    const int type = getIntParam("chordType");
    const int mode = getIntParam("performanceMode");
    const int speed = getIntParam("patternSpeed");
    const int play = getIntParam("playMode");
    const int variation = getIntParam("variation");
    const int scratchType = getIntParam("scratchType");
    const int percType = getIntParam("percType");
    const int capo = getIntParam("capo");

    for (int i = 0; i < 12; ++i)
        rootButtons[(size_t) i]->setToggleState(i == root, juce::dontSendNotification);
    for (int i = 0; i < 8; ++i)
        typeButtons[(size_t) i]->setToggleState(i == type, juce::dontSendNotification);
    for (int i = 0; i < 3; ++i)
    {
        modeButtons[(size_t) i]->setToggleState(i == mode, juce::dontSendNotification);
        speedButtons[(size_t) i]->setToggleState(i == speed, juce::dontSendNotification);
        playModeButtons[(size_t) i]->setToggleState(i == play, juce::dontSendNotification);
    }
    for (int i = 0; i < 5; ++i)
        variationButtons[(size_t) i]->setToggleState(i == variation, juce::dontSendNotification);
    for (int i = 0; i < 4; ++i)
    {
        scratchButtons[(size_t) i]->setToggleState(i == scratchType, juce::dontSendNotification);
        percussionButtons[(size_t) i]->setToggleState(i == percType, juce::dontSendNotification);
    }
    for (int i = 0; i < 7; ++i)
        eventButtons[(size_t) i]->setToggleState(i == selectedEvent, juce::dontSendNotification);
    for (int i = 0; i < 8; ++i)
        patternButtons[(size_t) i]->setButtonText(
            eventText(getIntParam("step" + juce::String(i + 1))));

    capoLabel.setText(capo == 0 ? "OFF" : juce::String(capo), juce::dontSendNotification);
    repaint();
}

void AcousticGuitarByJGKAudioProcessorEditor::resized()
{
    const int W = getWidth();

    for (int i = 0; i < 12; ++i)
    {
        const int col = i % 6;
        const int row = i / 6;
        rootButtons[(size_t) i]->setBounds(28 + col * 47, 92 + row * 37, 43, 32);
    }

    for (int i = 0; i < 8; ++i)
    {
        const int col = i % 4;
        const int row = i / 4;
        typeButtons[(size_t) i]->setBounds(325 + col * 69, 92 + row * 37, 65, 32);
    }

    voicingBox.setBounds(624, 96, 152, 30);
    inversionBox.setBounds(624, 134, 152, 30);

    capoMinus.setBounds(1000, 98, 32, 30);
    capoLabel.setBounds(1038, 98, 52, 30);
    capoPlus.setBounds(1096, 98, 32, 30);

    juce::Slider* knobs[9] =
        { &strumKnob, &humanizeKnob, &palmKnob, &dynamicsKnob, &toneKnob,
          &roomKnob, &tightLooseKnob, &swingKnob, &outputKnob };
    for (int i = 0; i < 9; ++i)
        knobs[i]->setBounds(25 + i * 104, 382, 82, 78);

    for (int i = 0; i < 3; ++i)
        modeButtons[(size_t) i]->setBounds(30 + i * 123, 483, 115, 32);

    presetBox.setBounds(414, 484, 174, 30);

    for (int i = 0; i < 3; ++i)
        speedButtons[(size_t) i]->setBounds(616 + i * 58, 484, 54, 30);

    for (int i = 0; i < 5; ++i)
        variationButtons[(size_t) i]->setBounds(813 + i * 52, 484, 48, 30);

    for (int i = 0; i < 8; ++i)
        patternButtons[(size_t) i]->setBounds(32 + i * 66, 555, 58, 44);

    for (int i = 0; i < 7; ++i)
        eventButtons[(size_t) i]->setBounds(586 + i * 77, 555, 71, 32);

    scratchVolKnob.setBounds(32, 636, 74, 70);
    scratchLengthKnob.setBounds(112, 636, 74, 70);
    scratchTimingKnob.setBounds(192, 636, 74, 70);

    for (int i = 0; i < 4; ++i)
        scratchButtons[(size_t) i]->setBounds(286 + i * 61, 654, 57, 30);

    percVolKnob.setBounds(558, 636, 74, 70);
    percToneKnob.setBounds(638, 636, 74, 70);
    percMixKnob.setBounds(718, 636, 74, 70);

    for (int i = 0; i < 4; ++i)
        percussionButtons[(size_t) i]->setBounds(812 + i * 67, 654, 63, 30);

    for (int i = 0; i < 12; ++i)
        chordPads[(size_t) i]->setBounds(30 + i * 65, 742, 59, 34);

    for (int i = 0; i < 3; ++i)
        playModeButtons[(size_t) i]->setBounds(835 + i * 105, 742, 99, 34);
}

void AcousticGuitarByJGKAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient background(bgTop, 0.0f, 0.0f,
                                    bgBottom, 0.0f, (float) getHeight(), false);
    background.addColour(0.35, juce::Colour(0xff41231a));
    background.addColour(0.60, juce::Colour(0xff17110e));
    g.setGradientFill(background);
    g.fillAll();

    // Soft sunset glow / bokeh, all procedural so there are no image assets to decode.
    g.setColour(juce::Colour(0xffffb45d).withAlpha(0.09f));
    g.fillEllipse(730.0f, 20.0f, 350.0f, 260.0f);
    g.setColour(juce::Colour(0xffffd38b).withAlpha(0.06f));
    for (int i = 0; i < 7; ++i)
        g.fillEllipse(80.0f + i * 165.0f, 26.0f + (i % 3) * 16.0f, 44.0f, 44.0f);

    // Header.
    g.setColour(juce::Colour(0xdd100d0b));
    g.fillRect(0, 0, getWidth(), 62);
    g.setColour(goldBright);
    g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    g.drawText("ACOUSTIC GUITAR", 28, 10, 310, 32, juce::Justification::centredLeft);
    g.setColour(cream.withAlpha(0.8f));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("by JGK  •  V3.3", 30, 39, 240, 18, juce::Justification::centredLeft);

    const char* tabs[] = { "PLAY", "CHORDS", "PATTERNS", "MIX", "SETUP" };
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    for (int i = 0; i < 5; ++i)
    {
        g.setColour(i == 0 ? gold : cream.withAlpha(0.6f));
        g.drawText(tabs[i], 650 + i * 92, 18, 82, 26, juce::Justification::centred);
    }

    drawSection(g, { 18.0f, 72.0f, 585.0f, 105.0f }, "CHORD BUILDER");
    drawSection(g, { 610.0f, 72.0f, 575.0f, 105.0f }, "VOICING / CAPO");

    // Central performance display.
    auto guitarArea = juce::Rectangle<float>(28.0f, 187.0f, 820.0f, 178.0f);
    drawGuitar(g, guitarArea);

    drawSection(g, { 866.0f, 187.0f, 319.0f, 178.0f }, "NOW PLAYING");
    g.setColour(goldBright);
    g.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    g.drawFittedText(processor.getDisplayChordName().toUpperCase(),
                     886, 219, 200, 42, juce::Justification::centredLeft, 1);
    g.setColour(cream.withAlpha(0.8f));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("ACTUAL: " + processor.getActualPitchName().toUpperCase(),
               886, 260, 220, 22, juce::Justification::centredLeft);
    drawChordDiagram(g, { 1080.0f, 216.0f, 88.0f, 118.0f });
    drawPaperAndCapo(g, { 888.0f, 292.0f, 180.0f, 58.0f }, getIntParam("capo"));

    drawSection(g, { 18.0f, 372.0f, 1167.0f, 96.0f }, "PERFORMANCE");
    const char* knobLabels[9] =
        { "STRUM", "HUMAN", "PALM", "DYNAMICS", "TONE",
          "ROOM", "LOOSE", "SWING", "OUTPUT" };
    g.setColour(cream.withAlpha(0.75f));
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    for (int i = 0; i < 9; ++i)
        g.drawText(knobLabels[i], 25 + i * 104, 368 + 22, 82, 18, juce::Justification::centred);

    drawSection(g, { 18.0f, 475.0f, 1167.0f, 51.0f }, "PLAY MODE / PRESET / RHYTHM");

    drawSection(g, { 18.0f, 537.0f, 555.0f, 72.0f }, "PATTERN");
    drawSection(g, { 578.0f, 537.0f, 607.0f, 72.0f }, "EDIT TOOL");

    drawSection(g, { 18.0f, 620.0f, 531.0f, 98.0f }, "SCRATCH");
    drawSection(g, { 555.0f, 620.0f, 630.0f, 98.0f }, "GUITAR PERCUSSION");

    drawSection(g, { 18.0f, 728.0f, 800.0f, 59.0f }, "CHORD PADS");
    drawSection(g, { 825.0f, 728.0f, 360.0f, 59.0f }, "PAD BEHAVIOUR");

    // Sample health indicator.
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(processor.samplesLoaded()
        ? juce::Colour(0xff65d68a) : juce::Colour(0xffff8b75));
    g.drawText(processor.samplesLoaded()
        ? "SAMPLES READY • " + juce::String(processor.sampleCount()) + " roots"
        : "WAITING FOR SAMPLES",
        1000, 44, 178, 14, juce::Justification::centredRight);
}

void AcousticGuitarByJGKAudioProcessorEditor::drawGuitar(
    juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colour(0xbb0f0c0a));
    g.fillRoundedRectangle(area, 10.0f);
    g.setColour(bronze.withAlpha(0.65f));
    g.drawRoundedRectangle(area, 10.0f, 1.0f);

    auto body = juce::Rectangle<float>(area.getX() + 80.0f, area.getY() + 35.0f, 180.0f, 115.0f);
    g.setColour(darkWood);
    g.fillEllipse(body);
    g.setColour(wood);
    g.drawEllipse(body, 3.0f);

    auto upper = body.withSizeKeepingCentre(118.0f, 100.0f).translated(56.0f, -6.0f);
    g.setColour(juce::Colour(0xff6d351f));
    g.fillEllipse(upper);

    g.setColour(juce::Colour(0xff100c09));
    g.fillEllipse(body.getCentreX() + 17.0f, body.getCentreY() - 25.0f, 48.0f, 48.0f);
    g.setColour(gold.withAlpha(0.55f));
    g.drawEllipse(body.getCentreX() + 12.0f, body.getCentreY() - 30.0f, 58.0f, 58.0f, 2.0f);

    const float neckY = area.getCentreY() - 14.0f;
    g.setColour(juce::Colour(0xff2a1710));
    g.fillRoundedRectangle(area.getX() + 245.0f, neckY, 450.0f, 28.0f, 5.0f);
    g.setColour(juce::Colour(0xff3b2117));
    g.fillRoundedRectangle(area.getX() + 680.0f, neckY - 6.0f, 90.0f, 40.0f, 9.0f);

    const float glow = 0.25f + 0.75f * stringGlow;
    for (int s = 0; s < 6; ++s)
    {
        const float y = neckY + 4.0f + s * 4.0f;
        g.setColour(goldBright.withAlpha(0.25f + 0.55f * glow));
        g.drawLine(area.getX() + 205.0f, y, area.getX() + 757.0f, y,
                   0.7f + s * 0.08f);
    }

    const int capo = getIntParam("capo");
    if (capo > 0)
    {
        const float x = area.getX() + 296.0f + (float) juce::jmin(capo, 12) * 23.0f;
        g.setColour(juce::Colour(0xffd1a153));
        g.fillRoundedRectangle(x, neckY - 7.0f, 10.0f, 42.0f, 4.0f);
        g.setColour(juce::Colour(0xff1c1612));
        g.fillEllipse(x - 3.0f, neckY + 28.0f, 16.0f, 16.0f);
    }

    g.setColour(cream.withAlpha(0.65f));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("REAL GUITAR VOICING • SIX-STRING STRUM",
               (int) area.getX() + 300, (int) area.getBottom() - 28,
               420, 20, juce::Justification::centred);
}

void AcousticGuitarByJGKAudioProcessorEditor::drawChordDiagram(
    juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto frets = processor.getDisplayFrets();
    g.setColour(juce::Colour(0xff15110f));
    g.fillRoundedRectangle(area, 6.0f);
    g.setColour(bronze);
    g.drawRoundedRectangle(area, 6.0f, 1.0f);

    area = area.reduced(10.0f);
    const float top = area.getY() + 14.0f;
    const float bottom = area.getBottom() - 8.0f;
    const float gapX = area.getWidth() / 5.0f;
    const float gapY = (bottom - top) / 5.0f;

    g.setColour(cream.withAlpha(0.65f));
    for (int s = 0; s < 6; ++s)
        g.drawLine(area.getX() + s * gapX, top,
                   area.getX() + s * gapX, bottom, 1.0f);
    for (int f = 0; f <= 5; ++f)
        g.drawLine(area.getX(), top + f * gapY,
                   area.getRight(), top + f * gapY, f == 0 ? 2.0f : 1.0f);

    for (int s = 0; s < 6; ++s)
    {
        const int fret = frets[(size_t) s];
        const float x = area.getX() + s * gapX;

        if (fret < 0)
        {
            g.setColour(juce::Colour(0xffff8b75));
            g.drawText("x", (int) x - 5, (int) area.getY() - 2, 12, 14,
                       juce::Justification::centred);
        }
        else if (fret == 0)
        {
            g.setColour(goldBright);
            g.drawEllipse(x - 3.0f, area.getY() + 2.0f, 6.0f, 6.0f, 1.5f);
        }
        else
        {
            const int shown = juce::jlimit(1, 5, fret);
            const float y = top + ((float) shown - 0.5f) * gapY;
            g.setColour(goldBright);
            g.fillEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        }
    }
}

void AcousticGuitarByJGKAudioProcessorEditor::drawPaperAndCapo(
    juce::Graphics& g, juce::Rectangle<float> area, int capo)
{
    g.setColour(juce::Colour(0xffd9cdb7).withAlpha(0.92f));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(juce::Colour(0xff756957));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);

    g.setColour(juce::Colour(0xff51483d));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(capo == 0 ? "capo • pen • song notes" : "capo on guitar",
               area.reduced(8).removeFromTop(16), juce::Justification::centredLeft);

    // Pen.
    g.setColour(juce::Colour(0xff29211d));
    g.drawLine(area.getX() + 98.0f, area.getBottom() - 13.0f,
               area.getRight() - 9.0f, area.getBottom() - 28.0f, 3.0f);

    // Capo only rests on the paper when OFF.
    if (capo == 0)
    {
        g.setColour(juce::Colour(0xffbd8a45));
        g.fillRoundedRectangle(area.getX() + 20.0f, area.getBottom() - 24.0f,
                               58.0f, 9.0f, 4.0f);
        g.setColour(juce::Colour(0xff211913));
        g.fillEllipse(area.getX() + 63.0f, area.getBottom() - 28.0f, 16.0f, 16.0f);
    }
}

void AcousticGuitarByJGKAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    const int step = processor.getCurrentPatternStep();
    if (step >= 0 && step < 8)
    {
        auto r = patternButtons[(size_t) step]->getBounds().toFloat().expanded(2.0f);
        g.setColour(goldBright.withAlpha(0.95f));
        g.drawRoundedRectangle(r, 5.0f, 2.2f);
    }
}

void AcousticGuitarByJGKAudioProcessorEditor::timerCallback()
{
    const int step = processor.getCurrentPatternStep();
    const uint32_t serial = processor.getStrumSerial();

    if (serial != lastStrumSerial)
    {
        lastStrumSerial = serial;
        stringGlow = 1.0f;
    }
    else
    {
        stringGlow *= 0.82f;
    }

    if (step != lastStep)
    {
        lastStep = step;
        repaint();
    }
    else if (stringGlow > 0.02f)
    {
        repaint(28, 187, 820, 178);
    }

    capoLabel.setText(getIntParam("capo") == 0 ? "OFF" : juce::String(getIntParam("capo")),
                      juce::dontSendNotification);
}
