/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin processor.
  ==============================================================================
*/

#include "PluginProcessor.h"

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
        jassert(paramIDs.size() == 3);

        auto activeParam = dynamic_cast<AudioParameterBool*>(parameters.getParameter(paramIDs[0]));
        jassert(activeParam != nullptr);

        auto octParam = dynamic_cast<AudioParameterInt*>(parameters.getParameter(paramIDs[1]));
        jassert(octParam != nullptr);

        auto noteParam = dynamic_cast<AudioParameterInt*>(parameters.getParameter(paramIDs[2]));
        jassert(noteParam != nullptr);

        rhythms.add(new Rhythm(activeParam, octParam, noteParam));
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
        jassert(paramIDs.size() == 3);

        params.add(std::make_unique<AudioParameterBool>(paramIDs[0], paramIDs[0], false));
        params.add(std::make_unique<AudioParameterInt>(paramIDs[1], paramIDs[1], 1, 8, 4));
        params.add(std::make_unique<AudioParameterInt>(paramIDs[2], paramIDs[2], 1, 12, 1));
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
    //No channels in audio buffer for midi effect, but we use it to get timing info
    //jassert(buffer.getNumChannels() == 0);

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto numSamples = buffer.getNumSamples();
	
    //Get timing info from host
    auto* playHead = getPlayHead();

    if (playHead != nullptr)
    {
        playHead->getCurrentPosition(posInfo); 
    }
	
    auto bpm = posInfo.bpm;
    auto ppqPosition = posInfo.ppqPosition;
    auto barPosPpq = ppqPosition - posInfo.ppqPositionOfLastBarStart;
    auto barLengthInQuarterNotes = posInfo.timeSigNumerator * 4. / posInfo.timeSigDenominator;
    auto samplesPerBeat = (60 / bpm) * fs;
    auto sampleCounter = barPosPpq * samplesPerBeat;
    auto currentSamplePos = (ppqPosition * 60.0 / bpm) * fs;

    float beatsPerSample = bpm / (fs * 60.0f);
	
    auto targetSample = barPosPpq;

	//MIDI

	/*
	 *    DBG("Beats per sample: " << beatsPerSample);

	 *
	 *
	 *
	 *
	 *
	 *    auto barPosPpq = ppqPosition - posInfo.ppqPositionOfLastBarStart;
    auto beatsPerSample = bpm / (fs * 60.0f);
	 *    auto sampleCounter = barPosPpq * samplesPerBeat;
    int samplesPerStep = (60 / bpm) * fs * 4;

    auto targetSample = barPosPpq;
	DBG("Samplecount: " << sampleCounter);
    DBG("SamlePerStep: " << samplesPerStep);
	 *    int samplePos = 0;
    MidiBuffer::Iterator it(midiMessages);
    while (it.getNextEvent(message, samplePos))
    {
        double beats = ppqPosition + (samplePos / samplesPerBeat);
        DBG("Beats: " << String::formatted("%4f", beats) << " " << message.getDescription());
    }
    */

	//Seed for random number generator
    srand(getTimerInterval());

    for (auto rhythm : rhythms)
    {
    	int selectedNote = rhythm->note->get();
        int selectedOctave = rhythm->octave->get();

        //Translate octave and note choice into Midi note value - (24 is midi value for C1)
        int note = 24 + (selectedNote * selectedOctave) - 1;
        int midiChannel = rhythms.indexOf(rhythm);

        if (rhythm->activated->get() == true && posInfo.isPlaying == true)
        {
        	/*
            DBG("Rhythm: " << rhythms.indexOf(rhythm));
            DBG("Note: " << note);
            DBG("Octave: " << selectedOctave);
            */
        
            //Generate random step and pulse lengths uniformly
            const int step_min = 4;
            const int step_max = 32;
            const int pulse_min = 1;
            int range = step_min - step_max + 1;

            int steps = rand() % range + step_min;
            int pulses = rand() % steps + pulse_min;

            //Call Euclidean algorithm and store pulses and steps into string
            std::string rhythmSeq = euclidean(pulses, steps);

        	/*
            DBG("Rhythm sequence: " << rhythmSeq);
            DBG("Steps: " << rhythmSeq.length());
            DBG("Pulses:" << pulses);
            */
    		
            int stepIndex = -1;
            int rhythmLength = rhythmSeq.length();
            DBG("Steps: " << rhythmSeq.length());

            for (int sample = 0; sample < numSamples; sample++)
            {
                targetSample += beatsPerSample;
                DBG(targetSample);
            	
                if (targetSample > 2.0)
                {                	
                    stepIndex++;
                    rhythm->currentStep = stepIndex;
                    DBG("Current step: " << stepIndex);

                    if (rhythmSeq[stepIndex] == 1)
                    {
                        midiMessages.addEvent(MidiMessage::noteOn(midiChannel, note, (juce::uint8) 100), midiMessages.getLastEventTime() + 1);
                    }
                    else if (rhythmSeq[stepIndex] == 0)
                    {
                        midiMessages.addEvent(MidiMessage::noteOff(midiChannel, note, (juce::uint8) 0), midiMessages.getLastEventTime() + 1);
                    }

                    if (stepIndex > rhythmLength)
                    {
                        stepIndex = 0;
                    }
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

SandysRhythmGeneratorAudioProcessor::Rhythm::Rhythm(AudioParameterBool* isActive, AudioParameterInt* octaveNumber, AudioParameterInt* noteNumber)
    :
    activated(isActive), octave(octaveNumber), note(noteNumber)
{
    reset();
}

void SandysRhythmGeneratorAudioProcessor::Rhythm::reset()
{
    cachedMidiNote = 24 + (note->get() * octave->get()) - 1;
    currentStep = 0;
}

StringArray SandysRhythmGeneratorAudioProcessor::getParameterIDs(const int rhythmIndex)
{
    String activated = "Activated";
    String note = "NoteNumber";
    String octave = "Octave";

    StringArray paramIDs = { activated, octave, note };

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