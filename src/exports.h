#ifndef XINPUT1_3_EXPORTS_H
#define XINPUT1_3_EXPORTS_H

#include <string>

typedef __int64 (far __stdcall *FARPROC)();
typedef void* HLOADEDMODULE;
typedef HLOADEDMODULE (*LoadModuleFunc)(const wchar_t* filePath);
typedef FARPROC (*GetModuleProcAddress)(HLOADEDMODULE module, const char* symbolName);

typedef void (*ThreadAttachHandler)();
typedef void (*AddThreadAttachHandler)(ThreadAttachHandler attachHandler);

typedef bool (*IsLoaderModuleLoaded)(const char* moduleName);

struct BootstrapAccessors {
	std::wstring executablePath;
    LoadModuleFunc LoadModule;
    GetModuleProcAddress GetModuleProcAddress;
    IsLoaderModuleLoaded IsLoaderModuleLoaded;
    AddThreadAttachHandler AddThreadAttachHandler;
};

typedef void (*BootstrapModuleFunc)(BootstrapAccessors& accessors);

#endif //XINPUT1_3_EXPORTS_H
