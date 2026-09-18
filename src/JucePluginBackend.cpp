#include "flowdaw/JucePluginBackend.hpp"
#include "flowdaw/PluginSafety.hpp"
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>

namespace flowdaw {
namespace {
constexpr int kMaxPluginBlock=16384;
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}

std::string hexEncode(const void* data,std::size_t size){static constexpr char kHex[]="0123456789ABCDEF";auto*p=static_cast<const unsigned char*>(data);std::string out;out.resize(size*2);for(std::size_t i=0;i<size;++i){out[i*2]=kHex[p[i]>>4];out[i*2+1]=kHex[p[i]&15];}return out;}
int nibble(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return-1;}
juce::MemoryBlock hexDecode(const std::string&s){juce::MemoryBlock b;if(s.size()%2)return b;b.setSize(s.size()/2,false);auto*out=static_cast<unsigned char*>(b.getData());for(std::size_t i=0;i<s.size()/2;++i){int hi=nibble(s[i*2]),lo=nibble(s[i*2+1]);if(hi<0||lo<0){b.reset();return b;}out[i]=static_cast<unsigned char>((hi<<4)|lo);}return b;}

void addFormats(juce::AudioPluginFormatManager&m){
#if JUCE_PLUGINHOST_VST3
    m.addFormat(std::make_unique<juce::VST3PluginFormat>());
#endif
#if JUCE_PLUGINHOST_AU && JUCE_MAC
    m.addFormat(std::make_unique<juce::AudioUnitPluginFormat>());
#endif
}
juce::AudioPluginFormat* formatFor(juce::AudioPluginFormatManager&m,const std::string&format){const auto want=lower(format);for(auto*f:m.getFormats()){auto name=lower(f->getName().toStdString());if((want=="vst3"&&name.find("vst3")!=std::string::npos)||(want=="au"&&name.find("audio")!=std::string::npos))return f;}return nullptr;}

class JuceProcessor final:public IPluginProcessor{
public:
    explicit JuceProcessor(std::unique_ptr<juce::AudioPluginInstance> instance):instance_(std::move(instance)){}
    bool prepare(int sampleRate,int channels,std::string&error)override{
        if(!instance_){error="JUCE plugin instance is null";return false;}if(channels<1||channels>2){error="FLOWDAW JUCE bridge currently supports mono/stereo buffers";return false;}
        sampleRate_=sampleRate;channels_=channels;block_.setSize(channels_,kMaxPluginBlock,false,true,false);midi_.ensureSize(16384);instance_->setRateAndBufferSizeDetails(static_cast<double>(sampleRate_),kMaxPluginBlock);instance_->prepareToPlay(static_cast<double>(sampleRate_),kMaxPluginBlock);prepared_=true;return true;
    }
    void setState(const std::string&state)override{if(!instance_||state.empty())return;auto b=hexDecode(state);if(b.getSize())instance_->setStateInformation(b.getData(),static_cast<int>(b.getSize()));}
    std::string state()const override{if(!instance_)return{};juce::MemoryBlock b;instance_->getStateInformation(b);return hexEncode(b.getData(),b.getSize());}
    void process(AudioBuffer&buffer)override{(void)processRealtime(buffer.interleaved.data(),buffer.frames(),buffer.channels);}
    bool supportsRealtimeProcessing()const noexcept override{return true;}
    int latencySamples()const noexcept override{return instance_?std::max(0,instance_->getLatencySamples()):0;}
    bool processRealtime(float*interleaved,SampleIndex frameCount,int channels)noexcept override{
        return processRealtimeMidi(interleaved,frameCount,channels,nullptr,0);
    }
    bool supportsRealtimeMidiInput()const noexcept override{return instance_&&instance_->acceptsMidi();}
    bool processRealtimeMidi(float*interleaved,SampleIndex frameCount,int channels,const PluginMidiEvent*events,std::size_t eventCount)noexcept override{
        if(!instance_||!prepared_||!interleaved||frameCount<=0||channels!=channels_)return false;
        try{
            SampleIndex offset=0;while(offset<frameCount){const int frames=static_cast<int>(std::min<SampleIndex>(kMaxPluginBlock,frameCount-offset));block_.setSize(channels_,frames,false,false,true);midi_.clear();
                for(int c=0;c<channels_;++c)for(int f=0;f<frames;++f)block_.setSample(c,f,interleaved[static_cast<std::size_t>((offset+f)*channels_+c)]);
                if(events){for(std::size_t i=0;i<eventCount;++i){const int pos=events[i].sampleOffset-static_cast<int>(offset);if(pos<0||pos>=frames)continue;const std::uint8_t bytes[3]{events[i].status,events[i].data1,events[i].data2};midi_.addEvent(bytes,3,pos);}}
                instance_->processBlock(block_,midi_);
                for(int c=0;c<channels_;++c)for(int f=0;f<frames;++f)interleaved[static_cast<std::size_t>((offset+f)*channels_+c)]=block_.getSample(c,f);offset+=frames;}
            return true;
        }catch(...){return false;}
    }
    void resetRealtime()noexcept override{if(!instance_)return;try{instance_->reset();midi_.clear();}catch(...){} }
private:std::unique_ptr<juce::AudioPluginInstance> instance_;juce::AudioBuffer<float>block_;juce::MidiBuffer midi_;int sampleRate_=48000,channels_=2;bool prepared_=false;
};

class JuceBackend final:public IExternalPluginBackend{
public:JuceBackend(){addFormats(manager_);}bool supports(const std::string&format)const override{return format=="vst3"||format=="au";}
    std::unique_ptr<IPluginProcessor> create(const PluginInstance&plugin,std::string&error)override{error.clear();auto*fmt=formatFor(manager_,plugin.format);if(!fmt){error="JUCE backend does not provide format: "+plugin.format;return{};}juce::OwnedArray<juce::PluginDescription> found;fmt->findAllTypesForFile(found,juce::String(plugin.identifier));if(found.isEmpty()){error="JUCE could not describe plugin: "+plugin.identifier;return{};}juce::String juceError;auto instance=manager_.createPluginInstance(*found[0],48000.0,512,juceError);if(!instance){error=juceError.toStdString();if(error.empty())error="JUCE failed to instantiate plugin";return{};}return std::make_unique<JuceProcessor>(std::move(instance));}
private:juce::AudioPluginFormatManager manager_;
};
}

std::shared_ptr<IExternalPluginBackend> makeJucePluginBackend(){return std::make_shared<JuceBackend>();}

std::vector<PluginDescriptor> scanPluginsWithJuce(const std::vector<std::filesystem::path>&roots,std::string&error,PluginSafetyRegistry*safety){
    error.clear();juce::AudioPluginFormatManager manager;addFormats(manager);std::vector<PluginDescriptor> out;auto candidates=scanPluginPaths(roots);int failures=0;
    for(auto const&candidate:candidates){if(safety&&safety->isQuarantined(candidate.identifier))continue;auto*fmt=formatFor(manager,candidate.format);if(!fmt)continue;try{juce::OwnedArray<juce::PluginDescription> found;fmt->findAllTypesForFile(found,juce::String(candidate.identifier));if(found.isEmpty()){++failures;if(safety)safety->noteFailure(candidate.identifier,"JUCE scan returned no plugin descriptions");continue;}if(safety)safety->noteSuccess(candidate.identifier);for(auto*d:found){PluginDescriptor p;p.format=candidate.format;p.identifier=candidate.identifier;p.name=d->name.toStdString();p.vendor=d->manufacturerName.toStdString();p.category=d->category.toStdString();p.path=candidate.path;p.builtin=false;out.push_back(std::move(p));}}catch(const std::exception&e){++failures;if(safety)safety->noteFailure(candidate.identifier,e.what());}catch(...){++failures;if(safety)safety->noteFailure(candidate.identifier,"Unknown JUCE scan failure");}}
    if(failures>0)error=std::to_string(failures)+" plugin bundle(s) failed validation; repeated failures are quarantined";std::sort(out.begin(),out.end(),[](auto const&a,auto const&b){if(a.format!=b.format)return a.format<b.format;return a.name<b.name;});return out;
}

} // namespace flowdaw
