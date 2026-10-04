#pragma once
#include <string>

namespace flowdaw::ui {

enum class StartupProjectSource { freshProject, lastSession, recoveredAutosave };

struct StartupPresentation {
    StartupProjectSource source=StartupProjectSource::freshProject;
    std::string identity;

    std::string projectLabel() const {
        const auto name=identity.empty()?std::string("Untitled Beat"):identity;
        switch(source){
            case StartupProjectSource::recoveredAutosave:return "Recovered autosave: "+name;
            case StartupProjectSource::lastSession:return "Restored last session: "+name;
            case StartupProjectSource::freshProject:return "Fresh project: "+name;
        }
        return name;
    }

    std::string statusText() const {
        switch(source){
            case StartupProjectSource::recoveredAutosave:return "Recovered autosave • Save to confirm, or choose New/Open intentionally";
            case StartupProjectSource::lastSession:return "Restored last session • recovery snapshot armed";
            case StartupProjectSource::freshProject:return "Fresh project • recovery snapshot armed";
        }
        return {};
    }
};

inline std::string pluginEmptyState(bool scanHasRun,bool filterActive){
    if(!scanHasRun)return "No plugins scanned • open Plugin Maintenance";
    if(filterActive)return "No plugin matches • change search or filter";
    return "No validated plugins found • open Plugin Maintenance";
}

inline std::string sampleEmptyState(){
    return "No audio sample • Import WAV or use Browser";
}

} // namespace flowdaw::ui
