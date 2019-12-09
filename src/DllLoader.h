#ifndef XINPUT1_3_DLLLOADER_H
#define XINPUT1_3_DLLLOADER_H

#include <Windows.h>
#include <unordered_map>
#include <MemoryModule.h>
#include "ImportResover.h"

class DllLoader {
public:
    ImportResolver* resolver;
    std::unordered_map<std::string, HLOADEDMODULE> loadedModules;

public:
    DllLoader(ImportResolver* importResolver) : resolver(importResolver) {};

    HLOADEDMODULE LoadModule(const wchar_t* filePath);

    HLOADEDMODULE LoadModule(const char* moduleName, const void * addr, size_t size);
};


#endif //XINPUT1_3_DLLLOADER_H
