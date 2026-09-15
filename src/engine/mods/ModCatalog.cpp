#include "ModCatalog.h"
#include "ModMetadata.h"
#include "ship/Context.h"
#include "libultraship/bridge/consolevariablebridge.h"
#include <zip.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <cctype>
#include <stdexcept>

namespace {
std::vector<ImportedMod> catalog;
std::set<std::string> activePaths;
std::map<std::string, ModMetadata> metadata;
std::string ReadManifest(const std::filesystem::path& path, SpaghettiPadMods::Resources* resources) {
    if (std::filesystem::is_directory(path)) {
        std::ifstream file(path / "mods.toml");
        std::string text((std::istreambuf_iterator<char>(file)), {});
        if (text.size() > 65536) return {};
        if (resources) for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file() && !entry.is_symlink())
                resources->Add(entry.path().lexically_relative(path).generic_string());
        }
        return text;
    }
    int error = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_RDONLY | ZIP_CHECKCONS, &error);
    if (!zip) return {};
    std::string text;
    zip_stat_t stat;
    if (zip_stat(zip, "mods.toml", 0, &stat) == 0 && stat.size <= 65536) {
        if (zip_file_t* file = zip_fopen(zip, "mods.toml", 0)) {
            text.resize(stat.size);
            if (zip_fread(file, text.data(), text.size()) != static_cast<zip_int64_t>(text.size())) text.clear();
            zip_fclose(file);
        }
    }
    if (resources) for (zip_int64_t i = 0; i < zip_get_num_entries(zip, 0); ++i) {
        const char* name = zip_get_name(zip, i, 0);
        if (name && *name && name[std::char_traits<char>::length(name) - 1] != '/') resources->Add(name);
    }
    zip_close(zip);
    return text;
}
}

const std::vector<ImportedMod>& GetImportedMods() { return catalog; }

void ScanImportedMods(bool startup) {
    catalog.clear();
    metadata.clear();
    const auto root = std::filesystem::path(Ship::Context::GetPathRelativeToAppDirectory("mods"));
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    std::vector<std::filesystem::path> paths;
    for (std::filesystem::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_symlink()) continue;
        auto ext = it->path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
        if (it->is_directory() || ext == ".o2r" || ext == ".zip") paths.push_back(it->path());
    }
    std::sort(paths.begin(), paths.end());
    for (const auto& path : paths) {
        ImportedMod mod;
        mod.path = path.lexically_normal().string();
        mod.filename = path.filename().string();
        mod.name = mod.filename;
        mod.active = activePaths.count(mod.path) != 0;
        try {
            const auto text = ReadManifest(path, &mod.resources);
            if (text.empty()) throw std::runtime_error("Missing or unreadable mods.toml. Needs a compatible SpaghettiKart pack.");
            const auto table = toml::parse(text);
            const auto name = table["mod"]["name"].value<std::string>();
            const auto version = table["mod"]["version"].value<std::string>();
            if (!name || name->empty() || !version || version->empty())
                throw std::runtime_error("Invalid pack name or version in mods.toml.");
            mod.name = *name;
            if (mod.name == "mk64-assets" || mod.name == "extended-assets" || mod.name == "spaghettikart-core")
                throw std::runtime_error("Optional packs cannot replace core archive metadata.");
            // Pack labels may use calendar versions (the existing HD pack does).
            // Only dependency constraints need strict semantic version parsing.
            if (auto* deps = table["dependencies"].as_table()) for (const auto& [key, value] : *deps) {
                const auto constraint = value.value<std::string>();
                semver::range_set<int, int, int> parsed;
                if (!constraint || !semver::parse(*constraint, parsed))
                    throw std::runtime_error("Invalid dependency version constraint.");
            }
            metadata[mod.path] = ModMetadata::LoadFromTOML(text);
            if (mod.resources.keys.empty()) throw std::runtime_error("No supported replacement resources found.");
            for (const auto& key : mod.resources.keys) {
                if (key.find("_kart/") != std::string::npos && key.rfind("textures/karts/", 0) != 0)
                    throw std::runtime_error("Older character format. Convert this pack for SpaghettiKart 1.0 first.");
            }
        } catch (const std::exception& error) { mod.error = error.what(); }
        const int saved = CVarGetInteger(SpaghettiPadMods::SettingKey(mod.filename).c_str(), -1);
        // Preserve the existing HD choice, but leave newly imported character
        // packs off until explicitly selected. Persist this migration once.
        mod.requested = saved < 0 ? (mod.name == "MK64-Reloaded-SK" &&
            CVarGetInteger("gSettings.SpaghettiPad.ImportedTexturePack", 1)) : saved != 0;
        catalog.push_back(std::move(mod));
    }
    // Stable base-before-character order. This is also the mount priority.
    std::stable_sort(catalog.begin(), catalog.end(), [](const auto& a, const auto& b) {
        return a.resources.trackTextures > b.resources.trackTextures;
    });
    // Validate declared core dependencies before allowing the user to select.
    std::map<std::string, ModMetadata> core;
    for (const auto& path : {Ship::Context::GetPathRelativeToAppDirectory("mk64.o2r"),
                             Ship::Context::LocateFileAcrossAppDirs("spaghetti.o2r")}) {
        const auto meta = ModMetadata::LoadFromTOML(ReadManifest(path, nullptr));
        core[meta.name] = meta;
    }
    for (auto& mod : catalog) {
        for (auto& [name, range] : metadata[mod.path].dependencies) {
            auto found = core.find(name);
            if (found != core.end() && !range.first.contains(found->second.version))
                mod.error = "Requires a different version of " + name + ".";
            else if (found == core.end())
                mod.error = "Requires " + name + "; dependent packs are not supported by this selector yet.";
        }
    }
    if (startup) {
        // Reconcile persisted selections deterministically, so adding or
        // replacing a conflicting archive cannot prevent the app from opening.
        for (size_t i = 0; i < catalog.size(); ++i) {
            auto& mod = catalog[i];
            if (!mod.error.empty()) mod.requested = false;
            for (size_t j = 0; mod.requested && j < i; ++j)
                if (catalog[j].requested && SpaghettiPadMods::Conflict(mod.resources, catalog[j].resources)) mod.requested = false;
            CVarSetInteger(SpaghettiPadMods::SettingKey(mod.filename).c_str(), mod.requested);
        }
        CVarSave();
    }
}

std::string ImportedModBlockReason(size_t index) {
    const auto& mod = catalog.at(index);
    if (!mod.error.empty()) return mod.error;
    for (size_t i = 0; i < catalog.size(); ++i) {
        if (i != index && catalog[i].requested && SpaghettiPadMods::Conflict(mod.resources, catalog[i].resources))
            return "Turn off " + catalog[i].name + " first: these packs replace the same racers or assets.";
    }
    return {};
}
bool SetImportedModEnabled(size_t index, bool enabled) {
    if (index >= catalog.size() || (enabled && !ImportedModBlockReason(index).empty())) return false;
    auto& mod = catalog[index];
    mod.requested = enabled;
    CVarSetInteger(SpaghettiPadMods::SettingKey(mod.filename).c_str(), enabled);
    CVarSave();
    return true;
}
std::vector<std::string> SelectedImportedModPaths() {
    std::vector<std::string> paths;
    for (const auto& mod : catalog) if (mod.requested && mod.error.empty()) paths.push_back(mod.path);
    return paths;
}
void MarkImportedModsActive() {
    activePaths.clear();
    for (auto& mod : catalog) { mod.active = mod.requested && mod.error.empty(); if (mod.active) activePaths.insert(mod.path); }
}
bool ImportedModsNeedRestart() {
    const auto paths = SelectedImportedModPaths();
    return std::set<std::string>(paths.begin(), paths.end()) != activePaths;
}
