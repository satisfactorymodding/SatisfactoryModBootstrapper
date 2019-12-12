#include "DllLoader.h"
#include "logging.h"
#include "naming_util.h"
#include <stdexcept>
#include <fstream>

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
    Logging::logFile << "Loading module " << moduleName << std::endl;
    HLOADEDMODULE handle = MemoryLoadLibraryEx(addr, size,
                                               MemoryDefaultAlloc,
                                               MemoryDefaultFree,
                                               MemoryLoadLibraryWithPlaceholder,
                                               MemoryGetProcAddressResolver,
                                               MemoryFreeLibrarySafe,
                                               this);
    if (handle == nullptr) {
        Logging::logFile << "Failed to load module " << moduleName << ": " << GetLastErrorAsString() << std::endl;
        throw std::invalid_argument(GetLastErrorAsString().c_str());
    }
    Logging::logFile << "Successfully loaded module " << moduleName << std::endl;
    this->loadedModules.insert({moduleName, handle});
    return static_cast<HMODULE>(handle);
}

std::string getModuleNameFromFilePath(const wchar_t* filePath) {
    std::wstring moduleNameStr(filePath);
    size_t lastSlashIndex = moduleNameStr.find_last_of('\\');
    if (lastSlashIndex == std::string::npos)
        lastSlashIndex = moduleNameStr.find_last_of('/');
    if (lastSlashIndex != std::string::npos) {
        moduleNameStr.erase(0, lastSlashIndex + 1);
    }
    char convertedName[500];
    wcstombs_s(nullptr, convertedName, moduleNameStr.c_str(), 500);
    return std::string(convertedName);
}

HLOADEDMODULE DllLoader::LoadModule(const char* moduleName, const wchar_t* filePath) {
    std::string moduleNameResult;
    if (moduleName == nullptr) {
        moduleNameResult = getModuleNameFromFilePath(filePath);
    } else {
        moduleNameResult = std::string(moduleName);
    }

    Logging::logFile << "Loading module " << moduleNameResult << " from " << filePath << std::endl;
    HANDLE fileHandle = CreateFileW(filePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
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
        HLOADEDMODULE result = LoadModule(moduleNameResult.c_str(), readBuffer, fileSize);
        free(readBuffer);
        return result;
    } catch (std::exception& ex) {
        free(readBuffer);
        throw ex;
    }
}
