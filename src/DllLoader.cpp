#include "DllLoader.h"
#include "logging.h"
#include "naming_util.h"
#include <stdexcept>
#include <MemoryModule.h>

ULONG64 placeholderLibrary = 0xDEADBEEF;

FARPROC MemoryGetProcAddressResolver(HCUSTOMMODULE module, LPCSTR name, void *userdata) {
    UNREFERENCED_PARAMETER(userdata);
    if (module == &placeholderLibrary) {
        //handle placeholder library here - resolve symbols using userdata
        ImportResolver* resolver = reinterpret_cast<ImportResolver*>(userdata);
        void* symbolAddress = resolver->ResolveSymbol(name);
        if (symbolAddress == nullptr) {
            Logging::logFile << "Unresolved import for module: " << name << std::endl;
        }
        return (FARPROC) symbolAddress;
    }
    return (FARPROC) GetProcAddress((HMODULE) module, name);
}

HCUSTOMMODULE MemoryLoadLibraryWithPlaceholder(LPCSTR filename, void *userdata) {
    UNREFERENCED_PARAMETER(userdata);
    HMODULE result;
    result = LoadLibraryA(filename);
    if (result == NULL) {
        //redirect not found libraries to placeholder address
        return &placeholderLibrary;
    }
    return (HCUSTOMMODULE) result;
}

void MemoryFreeLibrarySafe(HCUSTOMMODULE module, void *userdata) {
    UNREFERENCED_PARAMETER(userdata);
    if (module != &placeholderLibrary) {
        FreeLibrary((HMODULE) module);
    }
}

HMODULE DllLoader::LoadModule(const void* addr, size_t size) {
    HMEMORYMODULE handle = MemoryLoadLibraryEx(addr, size,
            MemoryDefaultAlloc,
            MemoryDefaultFree,
            MemoryLoadLibraryWithPlaceholder,
            MemoryGetProcAddressResolver,
            MemoryFreeLibrarySafe,
            this->resolver);
    if (handle == nullptr) {
        Logging::logFile << "Failed to load module from memory: " << GetLastErrorAsString() << std::endl;
    }
    return static_cast<HMODULE>(handle);
}

HMODULE DllLoader::LoadModule(const char *filePath) {
    HANDLE fileHandle = CreateFileA(filePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (fileHandle == nullptr) {
        std::string str("Failed to open module file: ");
        str.append(GetLastErrorAsString());
        throw std::invalid_argument(str.c_str());
    }
    DWORD fileSize = GetFileSize(fileHandle, nullptr);
    void* readBuffer = malloc(fileSize);
    bool success = ReadFile(fileHandle, readBuffer, fileSize, nullptr, nullptr);
    std::string lastErrorString = GetLastErrorAsString();
    CloseHandle(fileHandle);
    if (!success) {
        free(readBuffer);
        std::string str("Failed to read module file: ");
        str.append(lastErrorString);
        throw std::invalid_argument(str.c_str());
    }
    try {
        HMODULE result = LoadModule(readBuffer, fileSize);
        free(readBuffer);
        return result;
    } catch (std::exception ex) {
        free(readBuffer);
        throw ex;
    }
}
