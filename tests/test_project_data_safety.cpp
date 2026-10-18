#include "flowdaw/Serialization.hpp"
#include "flowdaw/Wav.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace flowdaw;

static void require(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
}
static std::string contents(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    require(static_cast<bool>(in), "test input must open");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
static void put(const fs::path& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    require(static_cast<bool>(out), "test fixture write");
}
static bool refused(const fs::path& path) {
    try { (void)ProjectSerializer::load(path, false); }
    catch (const std::exception&) { return true; }
    return false;
}

int main() {
    const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto root = fs::temp_directory_path() / ("flowdaw-project-safety-" + suffix);
    fs::create_directories(root);
    struct Cleanup { fs::path root; ~Cleanup() { std::error_code ec; fs::remove_all(root, ec); } } cleanup{root};

    Project first; first.name = "Last Known Good"; first.transport.bpm = 110.0;
    Track track; track.name = "Audio"; first.tracks.push_back(track);
    const auto path = root / "project.flow";
    ProjectSerializer::save(first, path);
    require(ProjectSerializer::load(path, false).name == first.name, "original project loads");

    Project second = first; second.name = "Updated Project";
    ProjectSerializer::save(second, path);
    require(ProjectSerializer::load(path, false).name == second.name, "replacement project loads");
    const auto backup = fs::path(path.string() + ".bak");
    require(fs::exists(backup), "save must preserve previous project backup");
    require(ProjectSerializer::load(backup, false).name == first.name, "backup must load independently");

    const std::string all = contents(path);
    const auto end = all.rfind("END\n");
    require(end != std::string::npos, "serialized project has END");
    const auto noEnd = root / "missing-end.flow";
    put(noEnd, all.substr(0, end));
    require(refused(noEnd), "missing END marker must be rejected");
    const auto midRecord = root / "mid-record.flow";
    put(midRecord, all.substr(0, all.find("TRACKS ") + 9));
    require(refused(midRecord), "truncated record must be rejected");

    // Make the next temp destination a directory, simulating an I/O failure
    // before promotion. A failure must never remove the last good project.
    const auto tempPath = fs::path(path.string() + ".tmp");
    require(!fs::exists(tempPath), "temporary project path should not exist");
    fs::create_directory(tempPath);
    bool failed = false;
    try { ProjectSerializer::save(first, path); }
    catch (const std::exception&) { failed = true; }
    require(failed, "blocked temporary write should report failure");
    require(ProjectSerializer::load(path, false).name == second.name,
            "failed save must preserve the last valid project");
    fs::remove(tempPath);

    AudioBuffer audio; audio.sampleRate = 48000; audio.channels = 1;
    audio.interleaved.assign(64, 0.25f);
    const auto wavPath = root / "audio.wav";
    WavFile::writeFloat32(wavPath, audio);
    require(WavFile::read(wavPath).frames() == audio.frames(), "float WAV roundtrip");
    failed = false;
    try { WavFile::writeFloat32(root, audio); }
    catch (const std::exception&) { failed = true; }
    require(failed, "WAV writing to a directory must report failure");
    return 0;
}
