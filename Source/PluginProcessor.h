#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include <set>
#include <random>
#include "GuitarEngine.h"

class AcousticGuitarByJGKAudioProcessor : public juce::AudioProcessor
{
public:
    enum PatternEvent { Rest = 0, Down, Up, Palm, Choke, Scratch, Percussion };

    AcousticGuitarByJGKAudioProcessor();
    ~AcousticGuitarByJGKAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Acoustic Guitar by JGK V3.3"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const { return apvts; }

    void pressChordFromUI(int root, int type);
    void releaseChordFromUI();
    void clearUIChord();
    void setSelectedChord(int root, int type, bool retriggerIfPlaying = true);
    void setPatternPreset(int presetIndex);
    void setPatternStep(int step, int eventType);

    int getCurrentPatternStep() const { return displayPatternStep.load(); }
    int getDisplayRoot() const { return displayRoot.load(); }
    int getDisplayType() const { return displayType.load(); }
    bool isChordActive() const { return displayActive.load(); }
    uint32_t getStrumSerial() const { return strumSerial.load(); }
    bool samplesLoaded() const { return engine.hasSamples(); }
    int sampleCount() const { return engine.getSampleCount(); }

    juce::String getDisplayChordName() const;
    juce::String getActualPitchName() const;
    std::array<int, 6> getDisplayFrets() const;

    static juce::String rootName(int root);
    static juce::String typeName(int type);
    static juce::String chordName(int root, int type);

private:
    struct ChordVoicing
    {
        std::array<int, 6> notes { -1, -1, -1, -1, -1, -1 };
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static std::vector<int> chordIntervals(int type);
    static int positiveMod12(int v);

    ChordVoicing buildVoicing(int root, int type, int inversion, int voicingStyle) const;
    std::pair<int, int> detectMidiChord() const;
    void triggerPatternStep(const ChordVoicing&, int step, int eventOffset,
                            float velocity, int performanceMode);
    void setIntParameter(const juce::String& id, int value);

    GuitarEngine engine;
    juce::AudioProcessorValueTreeState apvts;

    std::set<int> heldMidiNotes;
    bool midiChordActive = false;
    int midiChordRoot = 0;
    int midiChordType = 0;

    std::atomic<int> uiRoot { 0 };
    std::atomic<int> uiType { 0 };
    std::atomic<bool> uiActive { false };
    std::atomic<uint32_t> uiSerial { 0 };
    uint32_t seenUiSerial = 0;

    int currentRoot = 0;
    int currentType = 0;
    bool wasPatternActive = false;
    double samplesUntilNextStep = 0.0;
    int patternStep = 0;

    std::atomic<int> displayPatternStep { -1 };
    std::atomic<int> displayRoot { 0 };
    std::atomic<int> displayType { 0 };
    std::atomic<bool> displayActive { false };
    std::atomic<uint32_t> strumSerial { 0 };

    std::mt19937 timingRng { 0x4A474B56u };
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AcousticGuitarByJGKAudioProcessor)
};
