#pragma once
#include <algorithm>
#include <set>
#include <string>

namespace SpaghettiPadMods {
inline std::string ResourceKey(std::string path) {
    if (path.rfind("textures/", 0) != 0 && path.rfind("sound/", 0) != 0 &&
        path.rfind("models/", 0) != 0 && path.rfind("tracks/", 0) != 0) return {};
    for (const auto* suffix : {".meta", ".json", ".png", ".ogg", ".mp3", ".wav", ".flac"}) {
        const std::string ext(suffix);
        if (path.size() > ext.size() && path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
            path.resize(path.size() - ext.size());
    }
    return path;
}
struct Resources {
    std::set<std::string> keys;
    std::set<std::string> racers;
    bool trackTextures = false;
    void Add(const std::string& path) {
        const auto key = ResourceKey(path);
        if (key.empty()) return;
        keys.insert(key);
        const std::string prefix = "textures/karts/";
        if (key.rfind(prefix, 0) == 0) {
            const auto end = key.find('/', prefix.size());
            if (end != std::string::npos) racers.insert(key.substr(prefix.size(), end - prefix.size()));
        }
        if (key.rfind("textures/tracks/", 0) == 0) trackTextures = true;
    }
    bool CharacterLayer() const { return !racers.empty() && !trackTextures; }
};
inline bool Overlap(const std::set<std::string>& a, const std::set<std::string>& b) {
    auto x = a.begin(), y = b.begin();
    while (x != a.end() && y != b.end()) {
        if (*x == *y) return true;
        if (*x < *y) ++x; else ++y;
    }
    return false;
}
inline bool Conflict(const Resources& a, const Resources& b) {
    // A character pack intentionally overrides the character art in a broad
    // track/HD texture pack. Mount the broad pack first, then the character art.
    if ((a.CharacterLayer() && b.trackTextures) || (b.CharacterLayer() && a.trackTextures)) {
        for (const auto& key : a.keys)
            if (key.rfind("textures/", 0) != 0 && b.keys.count(key)) return true;
        return false;
    }
    if (a.CharacterLayer() && b.CharacterLayer() && Overlap(a.racers, b.racers)) return true;
    return Overlap(a.keys, b.keys);
}
inline std::string SettingKey(const std::string& filename) {
    // Encode the complete filename so punctuation cannot create CVar subkeys.
    static const char hex[] = "0123456789abcdef";
    std::string key = "gSettings.SpaghettiPad.Mods.";
    for (unsigned char c : filename) { key += hex[c >> 4]; key += hex[c & 15]; }
    return key;
}
}
