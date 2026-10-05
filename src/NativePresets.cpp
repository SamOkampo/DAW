#include "flowdaw/NativePresets.hpp"
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

}
