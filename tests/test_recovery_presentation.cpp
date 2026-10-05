#include "flowdaw/RecoveryPresentation.hpp"
#include <cassert>
#include <string>

using flowdaw::ui::StartupPresentation;
using flowdaw::ui::StartupProjectSource;

int main(){
    StartupPresentation fresh{StartupProjectSource::freshProject,"Untitled Beat"};
    assert(fresh.projectLabel()=="Fresh project: Untitled Beat");
    assert(fresh.statusText().find("recovery snapshot armed")!=std::string::npos);

    StartupPresentation last{StartupProjectSource::lastSession,"song.flow"};
    assert(last.projectLabel()=="Restored last session: song.flow");
    assert(last.statusText().find("Restored last session")!=std::string::npos);

    StartupPresentation recovered{StartupProjectSource::recoveredAutosave,"song.flow"};
    assert(recovered.projectLabel()=="Recovered autosave: song.flow");
    assert(recovered.statusText().find("Save to confirm")!=std::string::npos);

    assert(flowdaw::ui::pluginEmptyState(false,false).find("Plugin Maintenance")!=std::string::npos);
    assert(flowdaw::ui::pluginEmptyState(true,true).find("search or filter")!=std::string::npos);
    assert(flowdaw::ui::sampleEmptyState().find("Import WAV")!=std::string::npos);
}
