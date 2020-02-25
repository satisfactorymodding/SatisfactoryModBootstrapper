#ifndef XINPUT1_3_EXPORTS_H
#define XINPUT1_3_EXPORTS_H

#include <string>

typedef __int64 (__stdcall *FUNCTION_PTR)();
typedef void* MODULE_PTR;
typedef MODULE_PTR (*LoadModuleFunc)(const char* moduleName, const wchar_t* filePath);
typedef FUNCTION_PTR (*GetModuleProcAddressFunc)(MODULE_PTR module, const char* symbolName);
typedef bool (*IsLoaderModuleLoadedFunc)(const char* moduleName);
typedef FUNCTION_PTR(*ResolveGameSymbolFunc)(const char* symbolName);

struct BootstrapAccessors {
	const wchar_t* gameRootDirectory;
    LoadModuleFunc LoadModule;
    GetModuleProcAddressFunc GetModuleProcAddress;
    IsLoaderModuleLoadedFunc IsLoaderModuleLoaded;
    ResolveGameSymbolFunc ResolveGameSymbol;
    const wchar_t* version;
};

typedef void (*BootstrapModuleFunc)(BootstrapAccessors& accessors);

#endif //XINPUT1_3_EXPORTS_H
