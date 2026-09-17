#include "flowdaw/SessionRecovery.hpp"
#include "flowdaw/Serialization.hpp"
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace flowdaw {
SessionRecovery::SessionRecovery(std::filesystem::path root):root_(std::move(root)),marker_(root_/"session.lock"),autosave_(root_/"autosave.flow"){}

RecoveryInfo SessionRecovery::inspect() const{
    RecoveryInfo r;r.autosaveProject=autosave_;r.available=std::filesystem::exists(marker_)&&std::filesystem::exists(autosave_);if(!std::filesystem::exists(marker_))return r;
    std::ifstream f(marker_);std::string tag;while(f>>tag){if(tag=="ORIGINAL"){std::string v;f>>std::quoted(v);r.originalProject=v;}else if(tag=="GENERATION")f>>r.generation;else{std::string ignored;std::getline(f,ignored);}}return r;
}

void SessionRecovery::writeMarker(const std::filesystem::path& originalProject){
    std::filesystem::create_directories(root_);auto tmp=marker_;tmp+=".tmp";{std::ofstream f(tmp,std::ios::trunc);if(!f)throw std::runtime_error("Could not write recovery marker");f<<"FLOWDAW_SESSION 1\nORIGINAL "<<std::quoted(originalProject.string())<<"\nGENERATION "<<generation_<<"\nDIRTY 1\nEND\n";}
    std::error_code ec;std::filesystem::rename(tmp,marker_,ec);if(ec){std::filesystem::remove(marker_,ec);ec.clear();std::filesystem::rename(tmp,marker_,ec);if(ec)throw std::runtime_error("Could not replace recovery marker");}
}

void SessionRecovery::beginSession(const std::filesystem::path& originalProject){auto previous=inspect();generation_=previous.generation;writeMarker(originalProject);}
void SessionRecovery::autosave(const Project& project,const std::filesystem::path& originalProject){std::filesystem::create_directories(root_);ProjectSerializer::save(project,autosave_);++generation_;writeMarker(originalProject);}
Project SessionRecovery::loadRecovered(bool loadAudio) const{if(!std::filesystem::exists(autosave_))throw std::runtime_error("No FLOWDAW recovery project available");return ProjectSerializer::load(autosave_,loadAudio);}
void SessionRecovery::markCleanExit(){std::error_code ec;std::filesystem::remove(marker_,ec);std::filesystem::remove(autosave_,ec);auto bak=autosave_;bak+=".bak";std::filesystem::remove(bak,ec);}
}
