#pragma once

#include <string>

void InitModsSystem();
void UnloadMods();
bool HasImportedO2RMod();
bool IsImportedO2RModLoaded();
bool IsImportedO2RModEnabled();
void RefreshImportedO2RModStatus();
std::string GetImportedO2RModStatusText(bool requestedEnabled);
