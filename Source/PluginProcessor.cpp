/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin processor.
  ==============================================================================
*/

#include "PluginProcessor.h"

#include <random>

#include "foleys_gui_magic/General/foleys_MagicPluginEditor.h"

//==============================================================================
SandysRhythmGeneratorAudioProcessor::SandysRhythmGeneratorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::mono(), true)//Need audio input to get sample size
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ), parameters(*this, nullptr, Identifier("RhythmGeneratorPlugin"), createParameterLayout(getRhythmCount()))
#endif
{
    for (int i = 0; i < getRhythmCount(); ++i)
    {
        auto paramIDs = getParameterIDs(i);
        jassert(paramIDs.size() == 4);

        auto activeParam = dynamic_cast<AudioParameterBool*>(parameters.getParameter(paramIDs[0]));
        jassert(activeParam != nullptr);

        auto noteParam = dynamic_cast<AudioParameterInt*>(parameters.getParameter(paramIDs[1]));
        jassert(noteParam != nullptr);

        auto stepsParam = dynamic_cast<AudioParameterInt*>(parameters.getParameter(paramIDs[2]));
        jassert(stepsParam != nullptr);

        auto pulseParam = dynamic_cast<AudioParameterInt*>(parameters.getParameter(paramIDs[3]));
        jassert(pulseParam != nullptr);

        rhythms.add(new Rhythm(activeParam, noteParam, stepsParam, pulseParam));
    }
    startTimer(20);
}

SandysRhythmGeneratorAudioProcessor::~SandysRhythmGeneratorAudioProcessor()
{
    stopTimer();
}

AudioProcessorValueTreeState::ParameterLayout SandysRhythmGeneratorAudioProcessor::createParameterLayout(const int rhythmCount) const
{

    AudioProcessorValueTreeState::ParameterLayout params;

    for (int i = 0; i < rhythmCount; ++i)
    {
        auto paramIDs = getParameterIDs(i);
        jassert(paramIDs.size() == 4);

        params.add(std::make_unique<AudioParameterBool>(paramIDs[0], paramIDs[0], false));
        params.add(std::make_unique<AudioParameterInt>(paramIDs[1], paramIDs[1], 24, 119, 24));
        params.add(std::make_unique<AudioParameterInt>(paramIDs[2], paramIDs[2], 1, 32, 8));
        params.add(std::make_unique<AudioParameterInt>(paramIDs[3], paramIDs[3], 1, 32, 4));
    }

    return params;
}
int SandysRhythmGeneratorAudioProcessor::getRhythmCount()
{
    return 4;
}


//==============================================================================
const juce::String SandysRhythmGeneratorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SandysRhythmGeneratorAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SandysRhythmGeneratorAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SandysRhythmGeneratorAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SandysRhythmGeneratorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SandysRhythmGeneratorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SandysRhythmGeneratorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SandysRhythmGeneratorAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SandysRhythmGeneratorAudioProcessor::getProgramName(int index)
{
    return {};
}

void SandysRhythmGeneratorAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void SandysRhythmGeneratorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    time = 0;
    stepIndex = -1;
}

void SandysRhythmGeneratorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SandysRhythmGeneratorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void SandysRhythmGeneratorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    buffer.clear();

    auto numSamples = buffer.getNumSamples();
	
    //Get timing info from host
    auto* playHead = getPlayHead();

    if (playHead != nullptr)
    {
        playHead->getCurrentPosition(posInfo); 
    }

    auto bpm = posInfo.bpm;
    auto bps = bpm / 60.0f;
    auto samplesPerBeat = (fs / bps) / 2;
    auto counter = static_cast<int>(posInfo.timeInSamples % (int)(samplesPerBeat));
	
    auto eventTime = counter % numSamples;
    
    for (auto rhythm : rhythms)
    {
        if (rhythm->activated->get() == true && posInfo.isPlaying == true)
        {

            int steps = rhythm->steps->get();
            int pulses = rhythm->pulses->get();

        	
            int note = rhythm->note->get();

            if (pulses > steps)
            {
                rhythm->pulses->setValueNotifyingHost(steps);
            }

            //Call Euclidean algorithm and store pulses and steps into string
            std::string rhythmSeq = euclidean(pulses, steps);

            if ((counter + numSamples) >= samplesPerBeat)
            {
                //Reset indexing when reaching end of rhythm 
                if (stepIndex >= steps)
                {
                    stepIndex = -1;
                }

                stepIndex++;
            	
                if (rhythmSeq[stepIndex] == '1')
                {
                    midiMessages.addEvent(MidiMessage::noteOn(1, note, (juce::uint8) 100), midiMessages.getLastEventTime() + 1);
                }
            	else if (rhythmSeq[stepIndex] == '0')
                {
                    midiMessages.addEvent(MidiMessage::noteOff(1, note, (juce::uint8) 0), midiMessages.getLastEventTime() + 1);

                }
            }

               
        }

    }
}

//==============================================================================
bool SandysRhythmGeneratorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SandysRhythmGeneratorAudioProcessor::createEditor()
{
    // MAGIC GUI: create the generated editor, load your GUI from magic.xml in the binary resources
    // if you haven't created one yet, just give it a magicState and remove the last two arguments
    return new foleys::MagicPluginEditor(magicState);
}

//==============================================================================
void SandysRhythmGeneratorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    magicState.getStateInformation(destData);
}

void SandysRhythmGeneratorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    magicState.setStateInformation(data, sizeInBytes, getActiveEditor());
}

SandysRhythmGeneratorAudioProcessor::Rhythm::Rhythm(AudioParameterBool* isActive, AudioParameterInt* noteNumber, AudioParameterInt* stepsNumber, AudioParameterInt* pulseNumber)
    :
    activated(isActive), note(noteNumber), steps(stepsNumber), pulses(pulseNumber)
{
    reset();
}

void SandysRhythmGeneratorAudioProcessor::Rhythm::reset()
{
    cachedMidiNote = note->get();
}

StringArray SandysRhythmGeneratorAudioProcessor::getParameterIDs(const int rhythmIndex)
{
    String activated = "Activated";
    String note = "NoteNumber";
    String steps = "Steps";
    String pulses = "Pulses";

    StringArray paramIDs = { activated, note, steps, pulses };

    //Append Rhythms index to parameter IDs
    for (int i = 0; i < paramIDs.size(); ++i)
        paramIDs.getReference(i) += rhythmIndex;
    return paramIDs;
}

void SandysRhythmGeneratorAudioProcessor::timerCallback()
{
	
    for (auto rhythm : rhythms)
    {
        if (rhythm->activated->get() != true)
            return;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SandysRhythmGeneratorAudioProcessor();
}