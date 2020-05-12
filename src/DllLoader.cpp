#include "DllLoader.h"
#include "logging.h"
#include <Psapi.h>
#include "util.h"
#define MAX_NAME_LENGTH 2048

DllLoader::DllLoader(SymbolResolver* importResolver) : resolver(importResolver), dbgHelpModule(nullptr) {}

typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

typedef DWORD64 (__stdcall *SymLoadModuleExW)(HANDLE hProcess, HANDLE hFile, PCWSTR ImageName, PCWSTR ModuleName, DWORD64 BaseOfDll, DWORD DllSize, void* Data, DWORD Flags);
typedef bool (__stdcall *SymSetSearchPathW)(HANDLE hProcess, PCWSTR SearchPathStr);
typedef bool (__stdcall *SymUnloadModule64)(HANDLE hProcess, DWORD64 BaseOfDll);

void UnprotectPageIfNeeded(void* pagePointer, DWORD pageRangeSize);

bool ResolveDllImportsInternal(std::unordered_set<std::string>& alreadyLoadedLibraries, SymbolResolver* resolver, unsigned char* codeBase, PIMAGE_IMPORT_DESCRIPTOR importDesc);

HMODULE DllLoader::LoadModule(const path& filePath) {
    HMODULE preloadedModule = GetModuleHandleW(filePath.filename().c_str());
    if (preloadedModule != nullptr) {
        //don't try to load the same module twice.
        return preloadedModule;
    }
    auto* baseDll = reinterpret_cast<unsigned char *>(LoadLibraryExW(filePath.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
    Logging::logFile << "Loaded Raw DLL module: " << baseDll << std::endl;
    auto dosHeader = (PIMAGE_DOS_HEADER) baseDll;
    Logging::logFile << "Casted module to dos header " << dosHeader << std::endl;
    Logging::logFile << "Magic: " << dosHeader->e_magic << " Expected Magic: " << IMAGE_DOS_SIGNATURE << std::endl;
    auto pNTHeader = (PIMAGE_NT_HEADERS) ((LONGLONG) dosHeader + dosHeader->e_lfanew);
    auto importsDir = (PIMAGE_DATA_DIRECTORY) &(pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    if (importsDir->Size) {
        Logging::logFile << "Import Directory Size: " << importsDir->Size << std::endl;
        auto baseImp = (PIMAGE_IMPORT_DESCRIPTOR) (baseDll + importsDir->VirtualAddress);
        UnprotectPageIfNeeded(reinterpret_cast<void*>(baseImp), importsDir->Size);
        if (!ResolveDllImportsInternal(alreadyLoadedLibraries, resolver, baseDll, baseImp)) {
            Logging::logFile << "LoadModule failed: Cannot resolve imports of the library" << std::endl;
            return nullptr;
        }
    }
    Logging::logFile << "Resolved imports successfully; Calling DllMain" << std::endl;
    if (pNTHeader->OptionalHeader.AddressOfEntryPoint != 0) {
        auto DllEntry = (DllEntryProc)(LPVOID)(baseDll + pNTHeader->OptionalHeader.AddressOfEntryPoint);
        // notify library about attaching to process
        BOOL successful = (*DllEntry)((HINSTANCE) baseDll, DLL_PROCESS_ATTACH, nullptr);
        if (!successful) {
            Logging::logFile << "LoadModule failed: DllEntry returned false for library" << std::endl;
            return nullptr;
        }
    }
    Logging::logFile << "Called DllMain successfully on DLL. Loading finished." << std::endl;
    TryToLoadModulePDB((HMODULE) baseDll);
    return (HINSTANCE) baseDll;
}

/** @return path to directory containing DLL and PDB files */
std::wstring loadModuleDbgInfo(HMODULE dbgHelpModule, HMODULE dllModule) {
    HANDLE currentProcess = GetCurrentProcess();
    auto loadFunc = reinterpret_cast<SymLoadModuleExW>(GetProcAddress(dbgHelpModule, "SymLoadModuleExW"));
    auto unloadFunc = reinterpret_cast<SymUnloadModule64>(GetProcAddress(dbgHelpModule, "SymUnloadModule64"));
    auto setSearchPathFunc = reinterpret_cast<SymSetSearchPathW>(GetProcAddress(dbgHelpModule, "SymSetSearchPathW"));
    MODULEINFO moduleInfo{};
    wchar_t moduleName[MAX_NAME_LENGTH] = {0};
    wchar_t imageName[MAX_NAME_LENGTH] = {0};
    GetModuleInformation(currentProcess, dllModule, &moduleInfo, sizeof(moduleInfo));
    GetModuleFileNameExW(currentProcess, dllModule, imageName, MAX_NAME_LENGTH);
    GetModuleBaseNameW(currentProcess, dllModule, moduleName, MAX_NAME_LENGTH);
    const wchar_t* symbolSearchPath = path(imageName).parent_path().wstring().c_str();
    setSearchPathFunc(currentProcess, symbolSearchPath);

    //unload old module symbols if they were loaded via UE4's invasive load with invalid search path
    unloadFunc(currentProcess, (DWORD64) moduleInfo.lpBaseOfDll);
    DWORD64 resultAddr = loadFunc(currentProcess, dllModule, imageName, moduleName, (DWORD64) moduleInfo.lpBaseOfDll, (UINT32) moduleInfo.SizeOfImage, nullptr, 0);
    if (!resultAddr) {
        Logging::logFile << "Failed to load debug information for module " << path(imageName).string().c_str() << std::endl;
        Logging::logFile << "Failure Reason: " << GetLastErrorAsString() << std::endl;
    }
    return symbolSearchPath;
}

void DllLoader::FlushDebugSymbols() {
    //only perform flushing if we don't have dbghelp initialized
    if (dbgHelpModule == nullptr) {
        dbgHelpModule = GetModuleHandleA("dbghelp.dll");
        if (dbgHelpModule == nullptr) {
            Logging::logFile << "[WARNING] FlushDebugSymbols called too early dbghelp.dll is not loaded yet." << std::endl;
            return;
        }
        Logging::logFile << "Flushing debug symbols" << std::endl;
        //DbgHelp is loaded now, so perform registering of all earlier entries
        for (const auto& modulePath : delayedModulePDBs) {
            LoadModulePDBInternal(modulePath);
        }
        delayedModulePDBs.clear();
    }
}

void DllLoader::LoadModulePDBInternal(HMODULE dllModule) {
    const std::wstring SymbolDirectory = loadModuleDbgInfo(dbgHelpModule, dllModule);
    pdbRootDirectories.insert(SymbolDirectory);
}

void DllLoader::TryToLoadModulePDB(HMODULE module) {
    if (dbgHelpModule == nullptr) {
        //DbgHelp module is not initialized yet, delay loading
        delayedModulePDBs.push_back(module);
    } else {
        //DbgHelp is ready to accept symbol initializations, so perform it
        LoadModulePDBInternal(module);
    }
}

bool ResolveDllImportsInternal(std::unordered_set<std::string>& alreadyLoadedLibraries, SymbolResolver* resolver, unsigned char* codeBase, PIMAGE_IMPORT_DESCRIPTOR importDesc) {
    for (; importDesc->Name; importDesc++) {
        uintptr_t *thunkRef;
        FARPROC *funcRef;
        if (importDesc->OriginalFirstThunk) {
            thunkRef = (uintptr_t *) (codeBase + importDesc->OriginalFirstThunk);
            funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
        } else {
            // no hint table
            thunkRef = (uintptr_t *) (codeBase + importDesc->FirstThunk);
            funcRef = (FARPROC *) (codeBase + importDesc->FirstThunk);
        }
        const char* libraryName = (LPCSTR) (codeBase + importDesc->Name);

        HMODULE libraryHandle = GetModuleHandleA(libraryName);
        if (libraryHandle == nullptr) {
            std::string libraryNameString = libraryName;
            //try to load library only once
            if (alreadyLoadedLibraries.count(libraryNameString) == 0) {
                //load library if it exists, fallback to symbol resolver
                libraryHandle = LoadLibraryA(libraryName);
                alreadyLoadedLibraries.insert(std::move(libraryNameString));
            }
        }
        //iterate all thunk import entries and resolve them
        for (; *thunkRef; thunkRef++, funcRef++) {
            const char* importDescriptor;
            if (IMAGE_SNAP_BY_ORDINAL(*thunkRef)) {
                importDescriptor = (LPCSTR)IMAGE_ORDINAL(*thunkRef);
            } else {
                auto thunkData = (PIMAGE_IMPORT_BY_NAME) (codeBase + (*thunkRef));
                importDescriptor = (LPCSTR)&thunkData->Name;
            }
            UnprotectPageIfNeeded(reinterpret_cast<void*>(funcRef), 1);
            if (libraryHandle == nullptr) {
                //library handle is empty, attempt symbol resolution
                *funcRef = reinterpret_cast<FARPROC>(resolver->ResolveSymbol(importDescriptor));
            } else {
                //we have a library reference for a given import, so use GetProcAddress
                *funcRef = GetProcAddress(libraryHandle, importDescriptor);
            }
            if (*funcRef == nullptr) {
                Logging::logFile << "Failed to resolve import of symbol " << importDescriptor << " from " << libraryName << std::endl;
                return false;
            }
        }
    }
    return true;
}

//checks if memory page is protected and changes access to PAGE_READWRITE if it is protected
void UnprotectPageIfNeeded(void* pagePointer, DWORD pageRangeSize) {
    MEMORY_BASIC_INFORMATION memoryInfo;
    auto result = VirtualQuery(pagePointer, &memoryInfo, sizeof(memoryInfo));
    if ((memoryInfo.Protect & PAGE_READONLY) > 0) {
        Logging::logFile << "Removing protection from page " << pagePointer << std::endl;
        VirtualProtect(pagePointer, 1, PAGE_READWRITE, &memoryInfo.Protect);
    }
}