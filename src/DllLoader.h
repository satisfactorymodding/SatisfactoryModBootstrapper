#ifndef XINPUT1_3_DLLLOADER_H
#define XINPUT1_3_DLLLOADER_H

#include <Windows.h>
#include "ImportResover.h"

class DllLoader {
private:
    ImportResolver* resolver;

public:
    DllLoader(ImportResolver* importResolver) : resolver(importResolver) {};

    HMODULE LoadModule(const char* filePath);

    HMODULE LoadModule(const void * addr, size_t size);
};


#endif //XINPUT1_3_DLLLOADER_H
