#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

class AcousticGuitarByJGKAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit AcousticGuitarByJGKAudioProcessorEditor(AcousticGuitarByJGKAudioProcessor&);
    ~AcousticGuitarByJGKAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureKnob(juce::Slider&, const juce::String&, bool percent = false,
                       const juce::String& suffix = {});
    void setIntParam(const juce::String&, int);
    int getIntParam(const juce::String&) const;
    void refreshSelections();
    void styleButton(juce::TextButton&);
    void drawGuitar(juce::Graphics&, juce::Rectangle<float>);
    void drawChordDiagram(juce::Graphics&, juce::Rectangle<float>);
    void drawPaperAndCapo(juce::Graphics&, juce::Rectangle<float>, int capo);
    static juce::String eventText(int type);

    AcousticGuitarByJGKAudioProcessor& processor;

    std::array<std::unique_ptr<juce::TextButton>, 12> rootButtons;
    std::array<std::unique_ptr<juce::TextButton>, 8> typeButtons;
    std::array<std::unique_ptr<juce::TextButton>, 8> patternButtons;
    std::array<std::unique_ptr<juce::TextButton>, 7> eventButtons;
    std::array<std::unique_ptr<juce::TextButton>, 12> chordPads;
    std::array<std::unique_ptr<juce::TextButton>, 3> modeButtons;
    std::array<std::unique_ptr<juce::TextButton>, 3> speedButtons;
    std::array<std::unique_ptr<juce::TextButton>, 3> playModeButtons;
    std::array<std::unique_ptr<juce::TextButton>, 5> variationButtons;
    std::array<std::unique_ptr<juce::TextButton>, 4> scratchButtons;
    std::array<std::unique_ptr<juce::TextButton>, 4> percussionButtons;

    juce::ComboBox presetBox;
    juce::ComboBox voicingBox;
    juce::ComboBox inversionBox;

    juce::TextButton capoMinus { "-" };
    juce::TextButton capoPlus { "+" };
    juce::Label capoLabel;

    juce::Slider strumKnob, humanizeKnob, palmKnob, dynamicsKnob, toneKnob;
    juce::Slider roomKnob, tightLooseKnob, swingKnob, outputKnob;
    juce::Slider scratchVolKnob, scratchLengthKnob, scratchTimingKnob;
    juce::Slider percVolKnob, percToneKnob, percMixKnob;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    int selectedEvent = -1;
    int lastStep = -2;
    uint32_t lastStrumSerial = 0;
    float stringGlow = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AcousticGuitarByJGKAudioProcessorEditor)
};
