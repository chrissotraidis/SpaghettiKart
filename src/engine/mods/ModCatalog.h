#pragma once
#include "ModSelection.h"
#include <string>
#include <vector>
struct ImportedMod {
    std::string path, filename, name, error;
    SpaghettiPadMods::Resources resources;
    bool requested = false;
    bool active = false;
};
void ScanImportedMods(bool startup = false);
const std::vector<ImportedMod>& GetImportedMods();
std::string ImportedModBlockReason(size_t index);
bool SetImportedModEnabled(size_t index, bool enabled);
std::vector<std::string> SelectedImportedModPaths();
void MarkImportedModsActive();
bool ImportedModsNeedRestart();
