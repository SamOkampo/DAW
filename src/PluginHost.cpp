#include "flowdaw/PluginHost.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>

namespace flowdaw {
namespace {
class GainProcessor final:public IPluginProcessor{
public: explicit GainProcessor(float gain):gain_(gain){}
 bool prepare(int,int,std::string&)override{return true;} void setState(const std::string&)override{} std::string state()const override{return{};}
 void process(AudioBuffer&b)override{for(auto&x:b.interleaved)x*=gain_;}
private:float gain_=1.0f;
};
class SoftClipProcessor final:public IPluginProcessor{
public: explicit SoftClipProcessor(float drive):drive_(std::clamp(drive,0.0f,1.0f)){}
 bool prepare(int,int,std::string&)override{return true;} void setState(const std::string&)override{} std::string state()const override{return{};}
 void process(AudioBuffer&b)override{const float d=1.0f+drive_*9.0f;const float norm=std::max(0.0001f,std::tanh(d));for(auto&x:b.interleaved)x=std::tanh(x*d)/norm;}
private:float drive_=0.25f;
};
class WidthProcessor final:public IPluginProcessor{
public: explicit WidthProcessor(float width):width_(std::clamp(width,0.0f,2.0f)){}
 bool prepare(int,int channels,std::string&error)override{if(channels<2){error="FLOW Width requires stereo audio";return false;}return true;} void setState(const std::string&)override{} std::string state()const override{return{};}
 void process(AudioBuffer&b)override{if(b.channels<2)return;for(SampleIndex f=0;f<b.frames();++f){auto i=static_cast<std::size_t>(f*b.channels);float l=b.interleaved[i],r=b.interleaved[i+1],m=(l+r)*0.5f,s=(l-r)*0.5f*width_;b.interleaved[i]=m+s;b.interleaved[i+1]=m-s;}}
private:float width_=1.0f;
};
std::unique_ptr<IPluginProcessor> builtinProcessor(const PluginInstance&p){
 if(p.identifier=="flow.gain")return std::make_unique<GainProcessor>(std::clamp(pluginParameterValue(p,"gain",1.0f),0.0f,4.0f));
 if(p.identifier=="flow.softclip")return std::make_unique<SoftClipProcessor>(pluginParameterValue(p,"drive",0.25f));
 if(p.identifier=="flow.width")return std::make_unique<WidthProcessor>(pluginParameterValue(p,"width",1.0f));
 return{};
}
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}
}

float pluginParameterValue(const PluginInstance&p,const std::string&id,float fallback){for(auto const&x:p.parameters)if(x.id==id)return x.value;return fallback;}
void setPluginParameter(PluginInstance&p,const std::string&id,float value){for(auto&x:p.parameters)if(x.id==id){x.value=value;return;}p.parameters.push_back({id,value});}
PluginInstance makeBuiltinPlugin(const std::string&id){PluginInstance p;p.format="builtin";p.identifier=id;if(id=="flow.softclip"){p.name="FLOW Soft Clip";setPluginParameter(p,"drive",0.25f);}else if(id=="flow.width"){p.name="FLOW Width";setPluginParameter(p,"width",1.0f);}else{p.identifier="flow.gain";p.name="FLOW Gain";setPluginParameter(p,"gain",1.0f);}return p;}
std::vector<PluginDescriptor> builtinPluginDescriptors(){return{{"builtin","flow.gain","FLOW Gain","FLOWDAW","effect",{},true},{"builtin","flow.softclip","FLOW Soft Clip","FLOWDAW","effect",{},true},{"builtin","flow.width","FLOW Width","FLOWDAW","effect",{},true}};}

std::vector<PluginDescriptor> scanPluginPaths(const std::vector<std::filesystem::path>&roots){
 std::vector<PluginDescriptor> out;std::set<std::string> seen;
 auto add=[&](const std::filesystem::path&p){auto ext=lower(p.extension().string());std::string format;if(ext==".vst3")format="vst3";else if(ext==".component")format="au";else return;std::error_code ec;auto abs=std::filesystem::weakly_canonical(p,ec);if(ec)abs=std::filesystem::absolute(p,ec);auto key=abs.generic_string();if(!seen.insert(key).second)return;PluginDescriptor d;d.format=format;d.identifier=key;d.name=p.stem().string();d.vendor="External";d.path=abs;out.push_back(std::move(d));};
 for(auto const&root:roots){std::error_code ec;if(!std::filesystem::exists(root,ec))continue;if(!std::filesystem::is_directory(root,ec)){add(root);continue;}std::filesystem::recursive_directory_iterator it(root,std::filesystem::directory_options::skip_permission_denied,ec),end;for(;it!=end;it.increment(ec)){if(ec){ec.clear();continue;}auto const&p=it->path();auto ext=lower(p.extension().string());if(ext==".vst3"||ext==".component"){add(p);if(it->is_directory(ec))it.disable_recursion_pending();}}}
 std::sort(out.begin(),out.end(),[](auto const&a,auto const&b){if(a.format!=b.format)return a.format<b.format;return a.name<b.name;});return out;
}

void PluginHost::registerBackend(std::shared_ptr<IExternalPluginBackend>b){if(b)backends_.push_back(std::move(b));}
std::unique_ptr<IPluginProcessor> PluginHost::createProcessor(const PluginInstance&p,std::string&error)const{
 error.clear();if(!p.enabled||p.bypass)return{};if(p.format=="builtin"){auto proc=builtinProcessor(p);if(!proc)error="Unknown FLOWDAW builtin plugin: "+p.identifier;return proc;}
 if(p.format!="vst3"&&p.format!="au"){error="Unsupported plugin format: "+p.format;return{};}
 bool supportedBackend=false;std::string backendError;
 for(auto const&b:backends_)if(b&&b->supports(p.format)){supportedBackend=true;std::string attemptError;auto proc=b->create(p,attemptError);if(proc){error.clear();return proc;}if(!attemptError.empty())backendError=std::move(attemptError);}
 if(supportedBackend){error=backendError.empty()?"Compatible plugin backend could not instantiate: "+p.identifier:backendError;return{};}
 error="No "+p.format+" execution backend registered. Discovery/state are available; connect the JUCE/platform adapter to instantiate this plugin.";return{};
}
bool PluginHost::process(AudioBuffer&buffer,PluginInstance&p,std::string&error)const{
 if(!p.enabled||p.bypass)return true;auto proc=createProcessor(p,error);if(!proc)return false;if(!proc->prepare(buffer.sampleRate,buffer.channels,error))return false;proc->setState(p.opaqueState);AudioBuffer dry=buffer;proc->process(buffer);p.opaqueState=proc->state();const float wet=std::clamp(p.wet,0.0f,1.0f);if(wet<1.0f)for(std::size_t i=0;i<buffer.interleaved.size();++i)buffer.interleaved[i]=dry.interleaved[i]*(1.0f-wet)+buffer.interleaved[i]*wet;return true;
}
bool PluginHost::processChain(AudioBuffer&buffer,std::vector<PluginInstance>&plugins,std::string&error)const{for(auto&p:plugins)if(p.enabled&&!p.bypass&&!process(buffer,p,error))return false;return true;}

void processRealtimeBuiltinSample(const PluginInstance&p,float&l,float&r){
 if(!p.enabled||p.bypass||p.format!="builtin")return;const float wet=std::clamp(p.wet,0.0f,1.0f),dl=l,dr=r;
 if(p.identifier=="flow.gain"){const float g=std::clamp(pluginParameterValue(p,"gain",1.0f),0.0f,4.0f);l*=g;r*=g;}
 else if(p.identifier=="flow.softclip"){const float d=1.0f+std::clamp(pluginParameterValue(p,"drive",0.25f),0.0f,1.0f)*9.0f,n=std::max(0.0001f,std::tanh(d));l=std::tanh(l*d)/n;r=std::tanh(r*d)/n;}
 else if(p.identifier=="flow.width"){const float w=std::clamp(pluginParameterValue(p,"width",1.0f),0.0f,2.0f),m=(l+r)*0.5f,s=(l-r)*0.5f*w;l=m+s;r=m-s;}
 else return;l=dl*(1.0f-wet)+l*wet;r=dr*(1.0f-wet)+r*wet;
}
}
