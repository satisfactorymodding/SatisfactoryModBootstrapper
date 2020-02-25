#ifndef XINPUT1_3_DLLLOADER_H
#define XINPUT1_3_DLLLOADER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unordered_map>
#include "SymbolResolver.h"

class DllLoader {
public:
    SymbolResolver* resolver;
public:
    explicit DllLoader(SymbolResolver* importResolver);

    HMODULE LoadModule(const wchar_t* filePath);
};


#endif //XINPUT1_3_DLLLOADER_H
