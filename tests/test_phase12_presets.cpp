#include "flowdaw/ContentLibrary.hpp"
#include "flowdaw/CoreLibrary.hpp"
#include "flowdaw/NativeInstruments.hpp"
#include "flowdaw/NativePresets.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace flowdaw;

static void require(bool value,const char* message){
    if(!value)throw std::runtime_error(message);
}

int main(){
    try{
        const auto root=std::filesystem::temp_directory_path()/"flowdaw_phase12_presets";
        std::filesystem::remove_all(root);
        const auto summary=writeFlowCoreLibrary(root,48000);
        require(summary.sampleCount==12,"12.3 preserves the Phase 12.2 sample catalog");
        require(summary.presetCount==10,"12.3 ships ten native instrument presets");

        const auto manifest=loadContentManifest(root/"flow-core.manifest");
        require(manifest.libraryVersion==2,"preset catalog advances FLOW Core library version");
        std::size_t presetEntries=0;
        std::set<std::string> categories;
        std::set<std::string> ids;
        std::set<std::string> instrumentTypes;

        for(const auto& entry:manifest.entries){
            require(ids.insert(entry.id).second,"manifest IDs stay unique across samples and presets");
            if(entry.kind!=ContentKind::InstrumentPreset)continue;
            ++presetEntries;
            categories.insert(entry.category);
            const auto path=resolveContentPath(root,entry);
            require(std::filesystem::is_regular_file(path),"native preset file exists");
            const auto preset=loadNativeInstrumentPreset(path);
            require(preset.id==entry.id,"preset stable ID matches manifest");
            require(preset.category==entry.category,"preset category matches manifest");
            require(preset.instrument.enabled,"loaded native preset is enabled");
            instrumentTypes.insert(preset.instrument.type);

            const auto audio=renderNativeInstrumentNote(preset.instrument,60,0.9f,12000,48000,90.0);
            require(audio.frames()>12000,"native preset render contains release/tail");
            float peak=0.0f;
            for(const float x:audio.interleaved){
                require(std::isfinite(x),"native preset render is finite");
                peak=std::max(peak,std::abs(x));
            }
            require(peak>0.01f&&peak<=1.2f,"native preset produces bounded audible output");
        }

        require(presetEntries==10,"manifest contains ten native preset entries");
        require(categories==std::set<std::string>({"Instruments/808","Instruments/Bass","Instruments/Keys","Instruments/Lead","Instruments/Pad"}),"preset role categories");
        require(instrumentTypes==std::set<std::string>({"flow_808","flow_bass","flow_keys","flow_lead"}),"presets use only supported native engines");

        NativeInstrumentPreset roundtrip;
        roundtrip.id="flow.test.roundtrip";
        roundtrip.name="Roundtrip";
        roundtrip.category="Instruments/Keys";
        roundtrip.instrument.enabled=true;
        roundtrip.instrument.type="flow_keys";
        roundtrip.instrument.gain=0.73f;
        roundtrip.instrument.pan=-0.2f;
        roundtrip.instrument.attackMs=12.0f;
        roundtrip.instrument.releaseMs=430.0f;
        roundtrip.instrument.tone=0.36f;
        roundtrip.instrument.drive=0.11f;
        roundtrip.instrument.delayMix=0.17f;
        roundtrip.instrument.delayTicks=kPPQ/4;
        const auto roundtripPath=root/"roundtrip.flowpreset";
        saveNativeInstrumentPreset(roundtrip,roundtripPath);
        const auto loaded=loadNativeInstrumentPreset(roundtripPath);
        require(loaded.id==roundtrip.id&&loaded.instrument.type==roundtrip.instrument.type,"native preset roundtrip identity");
        require(std::abs(loaded.instrument.gain-roundtrip.instrument.gain)<0.0001f,"native preset roundtrip parameters");

        bool unsupportedRejected=false;
        try{
            NativeInstrumentPreset bad=roundtrip;
            bad.id="flow.test.bad";
            bad.instrument.type="flow_unknown";
            saveNativeInstrumentPreset(bad,root/"bad.flowpreset");
        }catch(const std::exception&){unsupportedRejected=true;}
        require(unsupportedRejected,"unsupported native instrument type rejected");

        bool rangeRejected=false;
        try{
            NativeInstrumentPreset bad=roundtrip;
            bad.id="flow.test.range";
            bad.instrument.delayMix=0.99f;
            saveNativeInstrumentPreset(bad,root/"range.flowpreset");
        }catch(const std::exception&){rangeRejected=true;}
        require(rangeRejected,"out-of-range preset parameter rejected");

        std::filesystem::remove_all(root);
        std::cout<<"FLOWDAW Phase 12.3 native preset tests: PASS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FLOWDAW Phase 12.3 native preset tests: FAIL: "<<e.what()<<"\n";
        return 1;
    }
}
