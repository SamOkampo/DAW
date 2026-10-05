#include "flowdaw/ContentAudit.hpp"
#include "flowdaw/ContentLibrary.hpp"
#include "flowdaw/NativePresets.hpp"
#include "flowdaw/Wav.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>

namespace flowdaw {
namespace {
std::string readText(const std::filesystem::path& path){
    std::ifstream in(path);
    std::ostringstream out;
    out<<in.rdbuf();
    return out.str();
}
void issue(ContentAuditReport& report,std::string message){
    report.issues.push_back(std::move(message));
}
}

ContentAuditReport auditFlowCoreLibrary(const std::filesystem::path& root){
    ContentAuditReport report;
    try{
        if(root.empty()){issue(report,"FLOW Core root is empty");return report;}
        const auto manifestPath=root/"flow-core.manifest";
        const auto provenancePath=root/"PROVENANCE.txt";
        const auto rightsPath=root/"CONTENT_RIGHTS.txt";
        std::error_code ec;
        if(!std::filesystem::is_regular_file(manifestPath,ec)){issue(report,"Missing flow-core.manifest");return report;}
        ec.clear();
        if(!std::filesystem::is_regular_file(provenancePath,ec))issue(report,"Missing PROVENANCE.txt");
        ec.clear();
        if(!std::filesystem::is_regular_file(rightsPath,ec))issue(report,"Missing CONTENT_RIGHTS.txt");

        const auto manifest=loadContentManifest(manifestPath);
        if(manifest.libraryId!="flow.core")issue(report,"Unexpected library id: "+manifest.libraryId);

        const auto provenance=std::filesystem::is_regular_file(provenancePath)?readText(provenancePath):std::string{};
        const auto rights=std::filesystem::is_regular_file(rightsPath)?readText(rightsPath):std::string{};
        if(provenance.find("No third-party recordings") == std::string::npos)
            issue(report,"Provenance is missing the no-third-party-recordings statement");
        if(rights.find("No third-party audio") == std::string::npos)
            issue(report,"Rights notice is missing the first-party audio statement");
        if(rights.find("does not grant a public license") == std::string::npos)
            issue(report,"Rights notice must state that it is not a public license grant");

        std::set<std::string> declaredAssetPaths;
        for(const auto& entry:manifest.entries){
            const auto resolved=resolveContentPath(root,entry);
            const auto relative=entry.relativePath.generic_string();
            declaredAssetPaths.insert(relative);
            ec.clear();
            if(std::filesystem::is_symlink(resolved,ec)){
                issue(report,"Symlinked content is not allowed: "+relative);
                continue;
            }
            ec.clear();
            if(!std::filesystem::is_regular_file(resolved,ec)){
                issue(report,"Missing manifest asset: "+relative);
                continue;
            }
            if(provenance.find(entry.id)==std::string::npos)
                issue(report,"Missing provenance entry for id: "+entry.id);

            if(entry.kind==ContentKind::Sample){
                ++report.sampleCount;
                try{
                    const auto audio=WavFile::read(resolved);
                    if(audio.sampleRate!=48000)issue(report,"FLOW Core sample is not 48 kHz: "+entry.id);
                    if(audio.channels!=1)issue(report,"FLOW Core sample is not mono: "+entry.id);
                    if(audio.frames()<=1000)issue(report,"FLOW Core sample is unexpectedly short: "+entry.id);
                    float peak=0.0f;
                    for(const float x:audio.interleaved){
                        if(!std::isfinite(x)){issue(report,"Non-finite sample data: "+entry.id);break;}
                        peak=std::max(peak,std::abs(x));
                    }
                    if(peak<=0.01f||peak>1.0f)issue(report,"FLOW Core sample peak is invalid: "+entry.id);
                }catch(const std::exception& e){
                    issue(report,"WAV decode failed for "+entry.id+": "+e.what());
                }
            }else if(entry.kind==ContentKind::InstrumentPreset){
                ++report.presetCount;
                try{
                    const auto preset=loadNativeInstrumentPreset(resolved);
                    if(preset.id!=entry.id)issue(report,"Preset id does not match manifest: "+entry.id);
                    if(preset.category!=entry.category)issue(report,"Preset category does not match manifest: "+entry.id);
                }catch(const std::exception& e){
                    issue(report,"Preset parse failed for "+entry.id+": "+e.what());
                }
            }
        }

        for(std::filesystem::recursive_directory_iterator it(root,std::filesystem::directory_options::skip_permission_denied,ec),end;it!=end;it.increment(ec)){
            if(ec){ec.clear();continue;}
            if(!it->is_regular_file(ec))continue;
            const auto ext=it->path().extension().string();
            if(ext!=".wav"&&ext!=".flowpreset")continue;
            const auto relative=std::filesystem::relative(it->path(),root,ec).generic_string();
            if(ec){ec.clear();continue;}
            if(!declaredAssetPaths.contains(relative))issue(report,"Undeclared content asset: "+relative);
        }
    }catch(const std::exception& e){
        issue(report,std::string("Content audit exception: ")+e.what());
    }
    report.passed=report.issues.empty();
    return report;
}

}
