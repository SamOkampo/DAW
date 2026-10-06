#include "flowdaw/NativePresets.hpp"
#include "flowdaw/PluginHost.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace flowdaw {
namespace {
void validatePreset(const NativeInstrumentPreset& preset){
    if(preset.id.empty())throw std::runtime_error("Native preset id is empty");
    if(preset.name.empty())throw std::runtime_error("Native preset name is empty");
    if(preset.category.empty())throw std::runtime_error("Native preset category is empty");
    if(!isSupportedNativeInstrumentType(preset.instrument.type))throw std::runtime_error("Unsupported native instrument type: "+preset.instrument.type);
    if(preset.instrument.gain<0.0f||preset.instrument.gain>2.0f)throw std::runtime_error("Native preset gain out of range");
    if(preset.instrument.pan<-1.0f||preset.instrument.pan>1.0f)throw std::runtime_error("Native preset pan out of range");
    if(preset.instrument.attackMs<0.0f||preset.instrument.attackMs>5000.0f)throw std::runtime_error("Native preset attack out of range");
    if(preset.instrument.releaseMs<0.0f||preset.instrument.releaseMs>10000.0f)throw std::runtime_error("Native preset release out of range");
    if(preset.instrument.tone<0.0f||preset.instrument.tone>1.0f)throw std::runtime_error("Native preset tone out of range");
    if(preset.instrument.drive<0.0f||preset.instrument.drive>1.0f)throw std::runtime_error("Native preset drive out of range");
    if(preset.instrument.delayMix<0.0f||preset.instrument.delayMix>0.85f)throw std::runtime_error("Native preset delay mix out of range");
    if(preset.instrument.delayTicks<1||preset.instrument.delayTicks>kPPQ*16)throw std::runtime_error("Native preset delay ticks out of range");
}
}

bool isSupportedNativeInstrumentType(const std::string& type){
    return type=="flow_keys"||type=="flow_808"||type=="flow_bass"||type=="flow_lead";
}

NativeInstrumentPreset loadNativeInstrumentPreset(const std::filesystem::path& path){
    std::ifstream input(path);
    if(!input)throw std::runtime_error("Could not open FLOWDAW native preset");

    NativeInstrumentPreset preset;
    std::string magic;
    input>>magic>>preset.schemaVersion;
    if(magic!="FLOWDAW_INSTRUMENT_PRESET"||preset.schemaVersion!=1)throw std::runtime_error("Unsupported FLOWDAW native preset");

    std::string tag;
    while(input>>tag){
        if(tag=="ID")input>>std::quoted(preset.id);
        else if(tag=="NAME")input>>std::quoted(preset.name);
        else if(tag=="CATEGORY")input>>std::quoted(preset.category);
        else if(tag=="TYPE")input>>std::quoted(preset.instrument.type);
        else if(tag=="GAIN")input>>preset.instrument.gain;
        else if(tag=="PAN")input>>preset.instrument.pan;
        else if(tag=="ATTACK_MS")input>>preset.instrument.attackMs;
        else if(tag=="RELEASE_MS")input>>preset.instrument.releaseMs;
        else if(tag=="TONE")input>>preset.instrument.tone;
        else if(tag=="DRIVE")input>>preset.instrument.drive;
        else if(tag=="DELAY_MIX")input>>preset.instrument.delayMix;
        else if(tag=="DELAY_TICKS")input>>preset.instrument.delayTicks;
        else if(tag=="END")break;
        else throw std::runtime_error("Unknown FLOWDAW native preset field: "+tag);
        if(!input)throw std::runtime_error("Malformed FLOWDAW native preset");
    }
    preset.instrument.enabled=true;
    validatePreset(preset);
    return preset;
}

void saveNativeInstrumentPreset(const NativeInstrumentPreset& preset,const std::filesystem::path& path){
    validatePreset(preset);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path,std::ios::trunc);
    if(!output)throw std::runtime_error("Could not write FLOWDAW native preset");

    output<<"FLOWDAW_INSTRUMENT_PRESET 1\n";
    output<<"ID "<<std::quoted(preset.id)<<"\n";
    output<<"NAME "<<std::quoted(preset.name)<<"\n";
    output<<"CATEGORY "<<std::quoted(preset.category)<<"\n";
    output<<"TYPE "<<std::quoted(preset.instrument.type)<<"\n";
    output<<"GAIN "<<preset.instrument.gain<<"\n";
    output<<"PAN "<<preset.instrument.pan<<"\n";
    output<<"ATTACK_MS "<<preset.instrument.attackMs<<"\n";
    output<<"RELEASE_MS "<<preset.instrument.releaseMs<<"\n";
    output<<"TONE "<<preset.instrument.tone<<"\n";
    output<<"DRIVE "<<preset.instrument.drive<<"\n";
    output<<"DELAY_MIX "<<preset.instrument.delayMix<<"\n";
    output<<"DELAY_TICKS "<<preset.instrument.delayTicks<<"\n";
    output<<"END\n";
}


namespace {
void validateEffectPreset(const NativeEffectPreset& preset){
    if(preset.schemaVersion!=1)throw std::runtime_error("Unsupported FLOWDAW native effect preset schema");
    if(preset.id.empty())throw std::runtime_error("Native effect preset id is empty");
    if(preset.name.empty())throw std::runtime_error("Native effect preset name is empty");
    if(preset.category.empty())throw std::runtime_error("Native effect preset category is empty");
    if(!isSupportedNativeEffectIdentifier(preset.pluginIdentifier))throw std::runtime_error("Unsupported native effect identifier: "+preset.pluginIdentifier);
    if(preset.wet<0.0f||preset.wet>1.0f)throw std::runtime_error("Native effect preset wet out of range");
    for(const auto&p:preset.parameters){
        const auto descriptors=builtinPluginParameterDescriptors(preset.pluginIdentifier);
        auto it=std::find_if(descriptors.begin(),descriptors.end(),[&](const auto&d){return d.id==p.id;});
        if(it==descriptors.end())throw std::runtime_error("Unknown native effect preset parameter: "+p.id);
        if(p.value<it->minValue||p.value>it->maxValue)throw std::runtime_error("Native effect preset parameter out of range: "+p.id);
    }
}

NativeEffectPreset fx(std::string id,std::string name,std::string category,std::string plugin,float wet,std::initializer_list<PluginParameter> parameters){
    NativeEffectPreset p;p.id=std::move(id);p.name=std::move(name);p.category=std::move(category);p.pluginIdentifier=std::move(plugin);p.wet=wet;p.parameters.assign(parameters.begin(),parameters.end());return p;
}
}

bool isSupportedNativeEffectIdentifier(const std::string& identifier){
    const auto descriptors=builtinPluginDescriptors();
    return std::any_of(descriptors.begin(),descriptors.end(),[&](const auto&d){return d.builtin&&!d.instrument&&d.identifier==identifier;});
}

NativeEffectPreset loadNativeEffectPreset(const std::filesystem::path& path){
    std::ifstream input(path);
    if(!input)throw std::runtime_error("Could not open FLOWDAW native effect preset");
    NativeEffectPreset preset;
    std::string magic;
    input>>magic>>preset.schemaVersion;
    if(magic!="FLOWDAW_EFFECT_PRESET"||preset.schemaVersion!=1)throw std::runtime_error("Unsupported FLOWDAW native effect preset");
    std::string tag;
    while(input>>tag){
        if(tag=="ID")input>>std::quoted(preset.id);
        else if(tag=="NAME")input>>std::quoted(preset.name);
        else if(tag=="CATEGORY")input>>std::quoted(preset.category);
        else if(tag=="PLUGIN")input>>std::quoted(preset.pluginIdentifier);
        else if(tag=="WET")input>>preset.wet;
        else if(tag=="PARAM"){PluginParameter parameter;input>>std::quoted(parameter.id)>>parameter.value;preset.parameters.push_back(std::move(parameter));}
        else if(tag=="END")break;
        else throw std::runtime_error("Unknown FLOWDAW native effect preset field: "+tag);
        if(!input)throw std::runtime_error("Malformed FLOWDAW native effect preset");
    }
    validateEffectPreset(preset);
    return preset;
}

void saveNativeEffectPreset(const NativeEffectPreset& preset,const std::filesystem::path& path){
    validateEffectPreset(preset);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path,std::ios::trunc);
    if(!output)throw std::runtime_error("Could not write FLOWDAW native effect preset");
    output<<"FLOWDAW_EFFECT_PRESET 1\n";
    output<<"ID "<<std::quoted(preset.id)<<"\n";
    output<<"NAME "<<std::quoted(preset.name)<<"\n";
    output<<"CATEGORY "<<std::quoted(preset.category)<<"\n";
    output<<"PLUGIN "<<std::quoted(preset.pluginIdentifier)<<"\n";
    output<<"WET "<<preset.wet<<"\n";
    for(const auto&parameter:preset.parameters)output<<"PARAM "<<std::quoted(parameter.id)<<" "<<parameter.value<<"\n";
    output<<"END\n";
}

std::vector<NativeEffectPreset> builtinNativeEffectPresets(){
    return{
        fx("eq-clean","EQ • Clean","EQ","flow.eq",1.0f,{{"output_gain_db",0.0f}}),
        fx("eq-vocal-presence","EQ • Vocal Presence","EQ","flow.eq",1.0f,{{"band2_freq",220.0f},{"band2_gain_db",-2.5f},{"band2_q",0.9f},{"band4_freq",3200.0f},{"band4_gain_db",3.0f},{"band4_q",1.1f},{"band6_freq",12000.0f},{"band6_gain_db",1.5f}}),
        fx("compressor-glue","Compressor • Glue","Dynamics","flow.compressor",1.0f,{{"threshold_db",-18.0f},{"ratio",2.5f},{"attack_ms",18.0f},{"release_ms",180.0f},{"knee_db",6.0f},{"makeup_db",1.0f}}),
        fx("compressor-punch","Compressor • Punch","Dynamics","flow.compressor",1.0f,{{"threshold_db",-14.0f},{"ratio",4.0f},{"attack_ms",28.0f},{"release_ms",95.0f},{"knee_db",3.0f},{"makeup_db",1.5f}}),
        fx("limiter-safe","Limiter • Safe","Dynamics","flow.limiter",1.0f,{{"ceiling_db",-1.0f},{"input_gain_db",0.0f},{"lookahead_ms",3.0f},{"release_ms",90.0f}}),
        fx("saturator-warm","Saturator • Warm","Colour","flow.saturator",1.0f,{{"drive_db",5.0f},{"tone",-0.15f},{"mode",0.0f}}),
        fx("saturator-edge","Saturator • Edge","Colour","flow.saturator",0.8f,{{"drive_db",11.0f},{"tone",0.25f},{"mode",1.0f}}),
        fx("reverb-small-room","Reverb • Small Room","Ambience","flow.reverb",0.18f,{{"room",0.28f},{"decay_s",0.85f},{"damping",0.48f},{"predelay_ms",8.0f},{"width",0.85f}}),
        fx("reverb-wide-hall","Reverb • Wide Hall","Ambience","flow.reverb",0.28f,{{"room",0.82f},{"decay_s",3.8f},{"damping",0.38f},{"predelay_ms",24.0f},{"width",1.0f}}),
        fx("delay-eighth","Delay • Eighth","Ambience","flow.delay",0.25f,{{"sync_bpm",120.0f},{"sync_beats",0.5f},{"feedback",0.30f},{"filter_hz",7600.0f},{"ping_pong",0.0f}}),
        fx("delay-ping-pong","Delay • Ping Pong","Ambience","flow.delay",0.28f,{{"sync_bpm",120.0f},{"sync_beats",0.75f},{"feedback",0.42f},{"filter_hz",6200.0f},{"ping_pong",1.0f}}),
        fx("chorus-wide","Chorus • Wide","Modulation","flow.chorus",0.32f,{{"rate_hz",0.65f},{"depth_ms",7.0f},{"base_ms",11.0f},{"feedback",0.05f},{"width",1.0f}}),
        fx("gate-tight","Gate • Tight","Dynamics","flow.gate",1.0f,{{"threshold_db",-32.0f},{"range_db",-70.0f},{"attack_ms",0.8f},{"hold_ms",18.0f},{"release_ms",65.0f}}),
        fx("utility-mono","Utility • Mono","Utility","flow.utility",1.0f,{{"mono",1.0f},{"gain_db",0.0f},{"width",1.0f}}),
        fx("utility-wide","Utility • Wide","Utility","flow.utility",1.0f,{{"width",1.35f},{"gain_db",0.0f},{"balance",0.0f}})
    };
}

PluginInstance instantiateNativeEffectPreset(const NativeEffectPreset& preset){
    validateEffectPreset(preset);
    auto plugin=makeBuiltinPlugin(preset.pluginIdentifier);
    plugin.wet=preset.wet;
    for(const auto&parameter:preset.parameters)setPluginParameter(plugin,parameter.id,parameter.value);
    return plugin;
}

}
