#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace flowdaw {

enum class ContentKind {
    Sample,
    InstrumentPreset
};

struct ContentEntry {
    std::string id;
    ContentKind kind=ContentKind::Sample;
    std::filesystem::path relativePath;
    std::string category;
    std::vector<std::string> tags;
};

struct ContentManifest {
    int schemaVersion=1;
    std::string libraryId;
    std::string displayName;
    int libraryVersion=1;
    std::vector<ContentEntry> entries;
};

struct ResolvedContentEntry {
    ContentEntry entry;
    std::filesystem::path path;
};

ContentManifest loadContentManifest(const std::filesystem::path& path);
bool isSafeContentRelativePath(const std::filesystem::path& path);
std::filesystem::path resolveContentPath(const std::filesystem::path& root,const ContentEntry& entry);
std::vector<ResolvedContentEntry> queryContentEntries(
    const std::filesystem::path& root,
    const ContentManifest& manifest,
    ContentKind kind,
    const std::string& query);

}
