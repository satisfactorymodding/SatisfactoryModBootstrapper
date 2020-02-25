#ifndef XINPUT1_3_DLLLOADER_H
#define XINPUT1_3_DLLLOADER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <unordered_set>
#include "SymbolResolver.h"
#include <filesystem>
using path = std::filesystem::path;

class DllLoader {
public:
    SymbolResolver* resolver;
private:
    std::unordered_set<std::string> alreadyLoadedLibraries;
    std::vector<HMODULE> delayedModulePDBs;
    HMODULE dbgHelpModule;
public:
    explicit DllLoader(SymbolResolver* importResolver);

    HMODULE LoadModule(const path& filePath);

    /**
     * Flushes all delay loaded PDBs into the
     * symbol storage for the loaded DbgHelp.dll instance
     * Note that DbgHelp should be initialized at this point,
     * otherwise it will cause undefined behavior
     */
    void FlushDebugSymbols();
private:
    void LoadModulePDBInternal(HMODULE module);
    void TryToLoadModulePDB(HMODULE module);
};


#endif //XINPUT1_3_DLLLOADER_H
