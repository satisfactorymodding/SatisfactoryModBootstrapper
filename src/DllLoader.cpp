#include "DllLoader.h"
#include "logging.h"

DllLoader::DllLoader(SymbolResolver* importResolver) : resolver(importResolver) {}

typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

void UnprotectPageIfNeeded(void* pagePointer, DWORD pageRangeSize);

bool ResolveDllImportsInternal(SymbolResolver* resolver, unsigned char* codeBase, PIMAGE_IMPORT_DESCRIPTOR importDesc);

inline bool startsWith(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

HMODULE DllLoader::LoadModule(const wchar_t * filePath) {
    auto* baseDll = reinterpret_cast<unsigned char *>(LoadLibraryExW(filePath, nullptr, DONT_RESOLVE_DLL_REFERENCES));
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
        if (!ResolveDllImportsInternal(resolver, baseDll, baseImp)) {
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
    return (HINSTANCE) baseDll;
}

bool ResolveDllImportsInternal(SymbolResolver* resolver, unsigned char* codeBase, PIMAGE_IMPORT_DESCRIPTOR importDesc) {
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
            //load library if it doesn't exist, fallback to symbol resolver
            libraryHandle = LoadLibraryA(libraryName);
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