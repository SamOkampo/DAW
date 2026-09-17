#include "flowdaw/PluginSafety.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace flowdaw {
PluginSafetyRecord* PluginSafetyRegistry::find(const std::string&id){for(auto&r:records_)if(r.identifier==id)return&r;return nullptr;}
const PluginSafetyRecord* PluginSafetyRegistry::find(const std::string&id)const{for(auto const&r:records_)if(r.identifier==id)return&r;return nullptr;}
void PluginSafetyRegistry::noteFailure(const std::string&id,const std::string&error){auto*r=find(id);if(!r){records_.push_back({id,0,false,{}});r=&records_.back();}r->failures++;r->lastError=error;if(r->failures>=std::max(1,threshold_))r->quarantined=true;}
void PluginSafetyRegistry::noteSuccess(const std::string&id){auto*r=find(id);if(!r)return;r->failures=0;if(!r->quarantined)r->lastError.clear();}
bool PluginSafetyRegistry::isQuarantined(const std::string&id)const{auto*r=find(id);return r&&r->quarantined;}
void PluginSafetyRegistry::clearQuarantine(const std::string&id){auto*r=find(id);if(!r)return;r->quarantined=false;r->failures=0;r->lastError.clear();}
void PluginSafetyRegistry::save(const std::filesystem::path&path)const{std::filesystem::create_directories(path.parent_path());auto tmp=path;tmp+=".tmp";{std::ofstream f(tmp,std::ios::trunc);if(!f)throw std::runtime_error("Could not write plugin safety registry");f<<"FLOWDAW_PLUGIN_SAFETY 1\n";for(auto const&r:records_)f<<"PLUGIN "<<std::quoted(r.identifier)<<" "<<r.failures<<" "<<(r.quarantined?1:0)<<" "<<std::quoted(r.lastError)<<"\n";f<<"END\n";}std::error_code ec;std::filesystem::rename(tmp,path,ec);if(ec){std::filesystem::remove(path,ec);ec.clear();std::filesystem::rename(tmp,path,ec);if(ec)throw std::runtime_error("Could not replace plugin safety registry");}}
void PluginSafetyRegistry::load(const std::filesystem::path&path){records_.clear();std::ifstream f(path);if(!f)return;std::string tag;int version=0;f>>tag>>version;if(tag!="FLOWDAW_PLUGIN_SAFETY"||version!=1)return;while(f>>tag){if(tag=="PLUGIN"){PluginSafetyRecord r;int q=0;f>>std::quoted(r.identifier)>>r.failures>>q>>std::quoted(r.lastError);r.quarantined=q!=0;records_.push_back(std::move(r));}else if(tag=="END")break;else{std::string ignored;std::getline(f,ignored);}}}
}
