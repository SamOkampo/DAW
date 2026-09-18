#include <juce_audio_processors/juce_audio_processors.h>

class FlowdawTestProcessor final:public juce::AudioProcessor{
public:
    FlowdawTestProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)){}
    const juce::String getName()const override{return"FLOWDAW Test Plugin";}
    void prepareToPlay(double,int)override{}
    void releaseResources()override{}
    bool isBusesLayoutSupported(const BusesLayout&l)const override{return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainInputChannelSet()==juce::AudioChannelSet::stereo();}
    void processBlock(juce::AudioBuffer<float>&b,juce::MidiBuffer&)override{b.applyGain(gain_);}
    juce::AudioProcessorEditor* createEditor()override{return new juce::GenericAudioProcessorEditor(*this);}
    bool hasEditor()const override{return true;}
    double getTailLengthSeconds()const override{return 0.0;}
    bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}
    int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return{};}void changeProgramName(int,const juce::String&)override{}
    void getStateInformation(juce::MemoryBlock&dest)override{juce::MemoryOutputStream out(dest,false);out.writeFloat(gain_);}
    void setStateInformation(const void*data,int bytes)override{juce::MemoryInputStream in(data,static_cast<std::size_t>(bytes),false);if(bytes>=4)gain_=in.readFloat();}
private:float gain_=0.5f;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new FlowdawTestProcessor();}
