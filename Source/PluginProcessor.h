/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

//==============================================================================
/**
*/
class SandysRhythmGeneratorAudioProcessor  : public juce::AudioProcessor, Timer
{
public:
    //==============================================================================
    SandysRhythmGeneratorAudioProcessor();
    ~SandysRhythmGeneratorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    AudioProcessorValueTreeState parameters;

    struct Rhythm
    {
        Rhythm(AudioParameterBool* isActive, AudioParameterInt* octaveNumber, AudioParameterInt* noteNumber);

        void reset();

        AudioParameterBool* activated;
        AudioParameterInt* octave;
        AudioParameterInt* note;

        int cachedMidiNote;

    };

    OwnedArray<Rhythm> rhythms;

    static StringArray getParameterIDs(int rhythmIndex);

    void timerCallback() override;

    AudioProcessorValueTreeState::ParameterLayout createParameterLayout(int rhythmCount) const;

    static int getRhythmCount();

    AudioPlayHead::CurrentPositionInfo posInfo;

    foleys::MagicProcessorState magicState{ *this, parameters };
	
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SandysRhythmGeneratorAudioProcessor)
};
