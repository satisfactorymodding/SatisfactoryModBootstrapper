#ifndef XINPUT1_3_EXPORTS_H
#define XINPUT1_3_EXPORTS_H

#include <fstream>

typedef __int64 (far __stdcall *FARPROC)();

typedef void* HLOADEDMODULE;
typedef HLOADEDMODULE (*LoadModuleFunc)(const char* filePath);
typedef FARPROC (*GetModuleProcAddress)(HLOADEDMODULE module, const char* symbolName);

typedef void (*ThreadAttachHandler)();
typedef void (*AddThreadAttachHandler)(ThreadAttachHandler attachHandler);

typedef bool (*IsLoaderModuleLoaded)(const char* moduleName);

struct BootstrapAccessors {
    std::ofstream& logFile;
    LoadModuleFunc LoadModule;
    GetModuleProcAddress GetModuleProcAddress;
    IsLoaderModuleLoaded IsLoaderModuleLoaded;
    AddThreadAttachHandler AddThreadAttachHandler;
};

typedef FARPROC (*BootstrapModuleFunc)(BootstrapAccessors& accessors);

#endif //XINPUT1_3_EXPORTS_H
