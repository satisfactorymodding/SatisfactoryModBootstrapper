#include "DllLoader.h"
#include "logging.h"
#include "naming_util.h"
#include <stdexcept>

struct ModuleInfo {
    HMODULE module;
    HLOADEDMODULE memoryModule;
};

FARPROC MemoryGetProcAddressResolver(HCUSTOMMODULE module, LPCSTR name, void *userdata) {
    auto* moduleInfo = static_cast<ModuleInfo *>(module);
    if (moduleInfo->module != nullptr) {
        return (FARPROC) GetProcAddress(moduleInfo->module, name);
    } else if (moduleInfo->memoryModule != nullptr) {
        return (FARPROC) MemoryGetProcAddress(moduleInfo->memoryModule, name);
    } else {
        auto* loader = reinterpret_cast<DllLoader*>(userdata);
        void* symbolAddress = loader->resolver->ResolveSymbol(name);
        if (symbolAddress == nullptr) {
            Logging::logFile << "Unresolved game import for module: " << name << std::endl;
        }
        return (FARPROC) symbolAddress;
    }
}

HCUSTOMMODULE MemoryLoadLibraryWithPlaceholder(LPCSTR filename, void *userdata) {
    auto* loader = reinterpret_cast<DllLoader*>(userdata);
    auto memoryModule = loader->loadedModules.find(filename);
    auto* moduleInfo = new ModuleInfo();
    if (memoryModule != loader->loadedModules.end()) {
        moduleInfo->memoryModule = memoryModule->second;
    } else {
        HMODULE module = LoadLibraryA(filename);
        if (module != nullptr) {
            moduleInfo->module = module;
        }
    }
    return moduleInfo;
}

void MemoryFreeLibrarySafe(HCUSTOMMODULE module, void *userdata) {
    auto* loader = reinterpret_cast<DllLoader*>(userdata);
    auto* moduleInfo = static_cast<ModuleInfo *>(module);
    if (moduleInfo->module != nullptr) {
        FreeLibrary(moduleInfo->module);
    } else if (moduleInfo->memoryModule != nullptr) {
        MemoryFreeLibrary(moduleInfo->memoryModule);
        for(const auto& pair : loader->loadedModules) {
            if (pair.second == moduleInfo->memoryModule) {
                loader->loadedModules.erase(pair.first);
            }
        }
    }
    delete moduleInfo;
}

HLOADEDMODULE DllLoader::LoadModule(const char* moduleName, const void* addr, size_t size) {
    HLOADEDMODULE handle = MemoryLoadLibraryEx(addr, size,
                                               MemoryDefaultAlloc,
                                               MemoryDefaultFree,
                                               MemoryLoadLibraryWithPlaceholder,
                                               MemoryGetProcAddressResolver,
                                               MemoryFreeLibrarySafe,
                                               this);
    if (handle == nullptr) {
        Logging::logFile << "Failed to load module from memory: " << GetLastErrorAsString() << std::endl;
    }
    if (handle != nullptr) {
        this->loadedModules.insert({moduleName, handle});
    }
    return static_cast<HMODULE>(handle);
}

HLOADEDMODULE DllLoader::LoadModule(const char *filePath) {
    std::string fileName(filePath);
    size_t lastSlashIndex = fileName.find_last_of('\\');
    if (lastSlashIndex == std::string::npos)
        lastSlashIndex = fileName.find_last_of('/');
    if (lastSlashIndex != std::string::npos) {
        fileName.erase(lastSlashIndex + 1);
    }
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
        HLOADEDMODULE result = LoadModule(fileName.c_str(), readBuffer, fileSize);
        free(readBuffer);
        return result;
    } catch (std::exception& ex) {
        free(readBuffer);
        throw ex;
    }
}
