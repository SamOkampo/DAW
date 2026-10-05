#include "flowdaw/PluginHost.hpp"
#include "flowdaw/BuiltinPluginDSP.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>

namespace flowdaw {
namespace {
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}
}

float pluginParameterValue(const PluginInstance&p,const std::string&id,float fallback){for(auto const&x:p.parameters)if(x.id==id)return x.value;return fallback;}
void setPluginParameter(PluginInstance&p,const std::string&id,float value){for(auto&x:p.parameters)if(x.id==id){x.value=value;return;}p.parameters.push_back({id,value});}

std::vector<BuiltinPluginParameterDescriptor> builtinPluginParameterDescriptors(const std::string&id){
    if(id=="flow.gain")return{{"gain","Gain",0.0f,4.0f,0.01f,1.0f," gain"}};
    if(id=="flow.softclip")return{{"drive","Drive",0.0f,1.0f,0.01f,0.25f," drive"}};
    if(id=="flow.width")return{{"width","Width",0.0f,2.0f,0.01f,1.0f," width"}};
    if(id=="flow.compressor")return{
        {"threshold_db","Threshold",-60.0f,0.0f,0.1f,-18.0f," dB"},
        {"ratio","Ratio",1.0f,20.0f,0.1f,4.0f,":1"},
        {"attack_ms","Attack",0.1f,200.0f,0.1f,10.0f," ms"},
        {"release_ms","Release",5.0f,2000.0f,1.0f,120.0f," ms"},
        {"knee_db","Knee",0.0f,24.0f,0.1f,6.0f," dB"},
        {"makeup_db","Makeup",-12.0f,24.0f,0.1f,0.0f," dB"}
    };
    if(id=="flow.eq")return{
        {"band1_freq","Band 1 Frequency",20.0f,20000.0f,1.0f,80.0f," Hz"},
        {"band1_gain_db","Band 1 Gain",-18.0f,18.0f,0.1f,0.0f," dB"},
        {"band1_q","Band 1 Q",0.10f,12.0f,0.01f,0.707f," Q"},
        {"band2_freq","Band 2 Frequency",20.0f,20000.0f,1.0f,200.0f," Hz"},
        {"band2_gain_db","Band 2 Gain",-18.0f,18.0f,0.1f,0.0f," dB"},
        {"band2_q","Band 2 Q",0.10f,12.0f,0.01f,0.707f," Q"},
        {"band3_freq","Band 3 Frequency",20.0f,20000.0f,1.0f,500.0f," Hz"},
        {"band3_gain_db","Band 3 Gain",-18.0f,18.0f,0.1f,0.0f," dB"},
        {"band3_q","Band 3 Q",0.10f,12.0f,0.01f,0.707f," Q"},
        {"band4_freq","Band 4 Frequency",20.0f,20000.0f,1.0f,1500.0f," Hz"},
        {"band4_gain_db","Band 4 Gain",-18.0f,18.0f,0.1f,0.0f," dB"},
        {"band4_q","Band 4 Q",0.10f,12.0f,0.01f,0.707f," Q"},
        {"band5_freq","Band 5 Frequency",20.0f,20000.0f,1.0f,5000.0f," Hz"},
        {"band5_gain_db","Band 5 Gain",-18.0f,18.0f,0.1f,0.0f," dB"},
        {"band5_q","Band 5 Q",0.10f,12.0f,0.01f,0.707f," Q"},
        {"band6_freq","Band 6 Frequency",20.0f,20000.0f,1.0f,12000.0f," Hz"},
        {"band6_gain_db","Band 6 Gain",-18.0f,18.0f,0.1f,0.0f," dB"},
        {"band6_q","Band 6 Q",0.10f,12.0f,0.01f,0.707f," Q"},
        {"output_gain_db","Output Gain",-18.0f,18.0f,0.1f,0.0f," dB"}
    };
    return{};
}

PluginInstance makeBuiltinPlugin(const std::string&requestedId){
    std::string id=requestedId;
    std::string name;
    if(id=="flow.gain")name="FLOW Gain";
    else if(id=="flow.softclip")name="FLOW Soft Clip";
    else if(id=="flow.width")name="FLOW Width";
    else if(id=="flow.eq")name="FLOW EQ";
    else if(id=="flow.compressor")name="FLOW Compressor";
    else{id="flow.gain";name="FLOW Gain";}
    PluginInstance p;p.format="builtin";p.identifier=id;p.name=name;
    for(const auto&parameter:builtinPluginParameterDescriptors(id))setPluginParameter(p,parameter.id,parameter.defaultValue);
    return p;
}

std::vector<PluginDescriptor> builtinPluginDescriptors(){
    return{
        {"builtin","flow.gain","FLOW Gain","FLOWDAW","effect",{},true},
        {"builtin","flow.softclip","FLOW Soft Clip","FLOWDAW","effect",{},true},
        {"builtin","flow.width","FLOW Width","FLOWDAW","effect",{},true},
        {"builtin","flow.eq","FLOW EQ","FLOWDAW","effect",{},true},
        {"builtin","flow.compressor","FLOW Compressor","FLOWDAW","effect",{},true}
    };
}

std::vector<PluginDescriptor> scanPluginPaths(const std::vector<std::filesystem::path>&roots){
 std::vector<PluginDescriptor> out;std::set<std::string> seen;
 auto add=[&](const std::filesystem::path&p){auto ext=lower(p.extension().string());std::string format;if(ext==".vst3")format="vst3";else if(ext==".component")format="au";else return;std::error_code ec;auto abs=std::filesystem::weakly_canonical(p,ec);if(ec)abs=std::filesystem::absolute(p,ec);auto key=abs.generic_string();if(!seen.insert(key).second)return;PluginDescriptor d;d.format=format;d.identifier=key;d.name=p.stem().string();d.vendor="External";d.path=abs;out.push_back(std::move(d));};
 for(auto const&root:roots){std::error_code ec;if(!std::filesystem::exists(root,ec))continue;if(!std::filesystem::is_directory(root,ec)){add(root);continue;}std::filesystem::recursive_directory_iterator it(root,std::filesystem::directory_options::skip_permission_denied,ec),end;for(;it!=end;it.increment(ec)){if(ec){ec.clear();continue;}auto const&p=it->path();auto ext=lower(p.extension().string());if(ext==".vst3"||ext==".component"){add(p);if(it->is_directory(ec))it.disable_recursion_pending();}}}
 std::sort(out.begin(),out.end(),[](auto const&a,auto const&b){if(a.format!=b.format)return a.format<b.format;return a.name<b.name;});return out;
}

void PluginHost::registerBackend(std::shared_ptr<IExternalPluginBackend>b){if(b)backends_.push_back(std::move(b));}
std::unique_ptr<IPluginProcessor> PluginHost::createProcessor(const PluginInstance&p,std::string&error)const{
 error.clear();if(!p.enabled||p.bypass)return{};if(p.format=="builtin")return createBuiltinPluginProcessor(p,error);
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
