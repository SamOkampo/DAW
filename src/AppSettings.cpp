#include "flowdaw/AppSettings.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace flowdaw {
unsigned long sanitizeBufferSize(unsigned long value){
    static constexpr unsigned long allowed[]{64,128,256,512,1024,2048};
    auto best=allowed[0];auto distance=value>best?value-best:best-value;
    for(auto v:allowed){auto d=value>v?value-v:v-value;if(d<distance){distance=d;best=v;}}
    return best;
}

std::filesystem::path defaultSettingsDirectory(){
#ifdef _WIN32
    if(const char* p=std::getenv("APPDATA")) return std::filesystem::path(p)/"FLOWDAW";
#elif defined(__APPLE__)
    if(const char* p=std::getenv("HOME")) return std::filesystem::path(p)/"Library"/"Application Support"/"FLOWDAW";
#else
    if(const char* p=std::getenv("XDG_CONFIG_HOME")) return std::filesystem::path(p)/"flowdaw";
    if(const char* p=std::getenv("HOME")) return std::filesystem::path(p)/".config"/"flowdaw";
#endif
    return std::filesystem::temp_directory_path()/"flowdaw";
}

AppSettings defaultAppSettings(){
    AppSettings s;
#ifdef _WIN32
    if(const char* p=std::getenv("COMMONPROGRAMFILES")) s.pluginRoots.emplace_back(std::filesystem::path(p)/"VST3");
#elif defined(__APPLE__)
    s.pluginRoots.emplace_back("/Library/Audio/Plug-Ins/VST3");
    s.pluginRoots.emplace_back("/Library/Audio/Plug-Ins/Components");
    if(const char* p=std::getenv("HOME")){s.pluginRoots.emplace_back(std::filesystem::path(p)/"Library/Audio/Plug-Ins/VST3");s.pluginRoots.emplace_back(std::filesystem::path(p)/"Library/Audio/Plug-Ins/Components");}
#else
    s.pluginRoots.emplace_back("/usr/lib/vst3");
    s.pluginRoots.emplace_back("/usr/local/lib/vst3");
    if(const char* p=std::getenv("HOME")) s.pluginRoots.emplace_back(std::filesystem::path(p)/".vst3");
#endif
    return s;
}

void saveAppSettings(const AppSettings&s,const std::filesystem::path&path){
    std::filesystem::create_directories(path.parent_path());auto tmp=path;tmp+=".tmp";
    {std::ofstream f(tmp,std::ios::trunc);if(!f)throw std::runtime_error("Could not write FLOWDAW settings");
    f<<"FLOWDAW_SETTINGS 1\n";
    f<<"SAMPLE_RATE "<<std::clamp(s.audio.preferredSampleRate,8000,384000)<<"\n";
    f<<"BUFFER "<<sanitizeBufferSize(s.audio.bufferSize)<<"\n";
    f<<"INPUT "<<std::quoted(s.audio.inputDevice)<<"\nOUTPUT "<<std::quoted(s.audio.outputDevice)<<"\n";
    f<<"MONITOR "<<(s.audio.monitorByDefault?1:0)<<"\nAUTOSAVE "<<std::clamp(s.autosaveSeconds,15,3600)<<"\nRESTORE "<<(s.restoreLastSession?1:0)<<"\n";
    f<<"LAST_PROJECT "<<std::quoted(s.lastProjectPath.string())<<"\nPLUGIN_ROOTS "<<s.pluginRoots.size()<<"\n";
    for(auto const&r:s.pluginRoots)f<<"PLUGIN_ROOT "<<std::quoted(r.string())<<"\n";
    f<<"END\n";}
    std::error_code ec;std::filesystem::rename(tmp,path,ec);if(ec){std::filesystem::remove(path,ec);ec.clear();std::filesystem::rename(tmp,path,ec);if(ec)throw std::runtime_error("Could not replace FLOWDAW settings");}
}

AppSettings loadAppSettings(const std::filesystem::path&path){
    auto s=defaultAppSettings();std::ifstream f(path);if(!f)return s;std::string tag;int version=0;f>>tag>>version;if(tag!="FLOWDAW_SETTINGS"||version!=1)return s;
    s.pluginRoots.clear();while(f>>tag){if(tag=="SAMPLE_RATE")f>>s.audio.preferredSampleRate;else if(tag=="BUFFER")f>>s.audio.bufferSize;else if(tag=="INPUT")f>>std::quoted(s.audio.inputDevice);else if(tag=="OUTPUT")f>>std::quoted(s.audio.outputDevice);else if(tag=="MONITOR"){int v;f>>v;s.audio.monitorByDefault=v!=0;}else if(tag=="AUTOSAVE")f>>s.autosaveSeconds;else if(tag=="RESTORE"){int v;f>>v;s.restoreLastSession=v!=0;}else if(tag=="LAST_PROJECT"){std::string v;f>>std::quoted(v);s.lastProjectPath=v;}else if(tag=="PLUGIN_ROOTS"){std::size_t n;f>>n;}else if(tag=="PLUGIN_ROOT"){std::string v;f>>std::quoted(v);s.pluginRoots.emplace_back(v);}else if(tag=="END")break;else{std::string ignored;std::getline(f,ignored);}}
    s.audio.preferredSampleRate=std::clamp(s.audio.preferredSampleRate,8000,384000);s.audio.bufferSize=sanitizeBufferSize(s.audio.bufferSize);s.autosaveSeconds=std::clamp(s.autosaveSeconds,15,3600);return s;
}
}
