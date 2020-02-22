#ifndef XINPUT1_3_DLLLOADER_H
#define XINPUT1_3_DLLLOADER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unordered_map>
#include <MemoryModule.h>
#include "SymbolResolver.h"

class DllLoader {
public:
    SymbolResolver* resolver;
    std::unordered_map<std::string, HLOADEDMODULE> loadedModules;

public:
    DllLoader(SymbolResolver* importResolver) : resolver(importResolver) {};

    HLOADEDMODULE LoadModule(const char* moduleName, const wchar_t* filePath);

    HLOADEDMODULE LoadModule(const char* moduleName, const void * addr, size_t size);
};


#endif //XINPUT1_3_DLLLOADER_H
