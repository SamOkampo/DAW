#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>

class FlowdawTestInstrument final:public juce::AudioProcessor{
public:
    FlowdawTestInstrument():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)){}
    const juce::String getName()const override{return"FLOWDAW Test Instrument";}
    void prepareToPlay(double,int)override{activeNotes_=0;}
    void releaseResources()override{}
    bool isBusesLayoutSupported(const BusesLayout&l)const override{
        return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainInputChannelSet().isDisabled();
    }
    void processBlock(juce::AudioBuffer<float>&buffer,juce::MidiBuffer&midi)override{
        buffer.clear();
        int cursor=0;
        auto fill=[&](int from,int to){
            if(activeNotes_<=0||to<=from)return;
            for(int c=0;c<buffer.getNumChannels();++c)
                for(int i=from;i<to;++i)buffer.setSample(c,i,0.2f);
        };
        for(const auto metadata:midi){
            const int pos=std::clamp(metadata.samplePosition,0,buffer.getNumSamples());
            fill(cursor,pos);
            const auto msg=metadata.getMessage();
            if(msg.isNoteOn())++activeNotes_;
            else if(msg.isNoteOff())activeNotes_=std::max(0,activeNotes_-1);
            cursor=pos;
        }
        fill(cursor,buffer.getNumSamples());
    }
    juce::AudioProcessorEditor* createEditor()override{return nullptr;}
    bool hasEditor()const override{return false;}
    double getTailLengthSeconds()const override{return 0.0;}
    bool acceptsMidi()const override{return true;}
    bool producesMidi()const override{return false;}
    bool isMidiEffect()const override{return false;}
    int getNumPrograms()override{return 1;}
    int getCurrentProgram()override{return 0;}
    void setCurrentProgram(int)override{}
    const juce::String getProgramName(int)override{return{};}
    void changeProgramName(int,const juce::String&)override{}
    void getStateInformation(juce::MemoryBlock&)override{}
    void setStateInformation(const void*,int)override{}
private:
    int activeNotes_=0;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new FlowdawTestInstrument();}
