/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "foleys_gui_magic/General/foleys_MagicProcessorState.h"

//==============================================================================
/**
*/
class SandysRhythmGeneratorAudioProcessor : public juce::AudioProcessor, Timer
{
public:
    //==============================================================================
    SandysRhythmGeneratorAudioProcessor();
    ~SandysRhythmGeneratorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

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
        int currentStep;

    };

    OwnedArray<Rhythm> rhythms;

    static StringArray getParameterIDs(int rhythmIndex);

	//Euclidean algorithm
    std::string euclidean(int pulses, int steps)
    {
        //We can only have as many beats as we have steps (0 <= beats <= steps)
        if (pulses > steps)
            pulses = steps;

        //Each iteration is a process of pairing strings X and Y and the remainder from the pairings
        //X will hold the "dominant" pair (the pair that there are more of)
        std::string x = "1";
        int x_amount = pulses;

        std::string y = "0";
        int y_amount = steps - pulses;

        do
        {
            //Placeholder variables
            int x_temp = x_amount;
            int y_temp = y_amount;
            std::string y_copy = y;

            //Check which is the dominant pair 
            if (x_temp >= y_temp)
            {
                //Set the new number of pairs for X and Y
                x_amount = y_temp;
                y_amount = x_temp - y_temp;

                //The previous dominant pair becomes the new non dominant pair
                y = x;
            }
            else
            {
                x_amount = x_temp;
                y_amount = y_temp - x_temp;
            }

            //Create the new dominant pair by combining the previous pairs
            x = x + y_copy;
        } while (x_amount > 1 && y_amount > 1);//iterate as long as the non dominant pair can be paired (if there is 1 Y left, all we can do is pair it with however many Xs are left, so we're done)

                                               //By this point, we have strings X and Y formed through a series of pairings of the initial strings "1" and "0"
                                               //X is the final dominant pair and Y is the second to last dominant pair
        std::string rhythm;
        for (int i = 1; i <= x_amount; i++)
            rhythm += x;
        for (int i = 1; i <= y_amount; i++)
            rhythm += y;
        return rhythm;
    }

    double fs;
    int time;

    bool noteIsOn;
	
    int timerPeriodMs;

    int getTimerInterval() const noexcept { return timerPeriodMs; }

    void timerCallback() override;

    MidiBuffer processedBuffer;
	
    AudioProcessorValueTreeState::ParameterLayout createParameterLayout(int rhythmCount) const;

    static int getRhythmCount();

    AudioPlayHead::CurrentPositionInfo posInfo;

    foleys::MagicProcessorState magicState{ *this, parameters };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SandysRhythmGeneratorAudioProcessor)
};