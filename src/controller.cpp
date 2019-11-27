#include "logging.h"
#include "DllLoader.h"
#include <MemoryModule.h>
#include <filesystem>
#include <mutex>
#include "exports.h"

#define GAME_MODULE_NAME "FactoryGame-Win64-Shipping.exe"

static DllLoader* dllLoader;
static std::mutex attachHandlersMutex;
static std::vector<ThreadAttachHandler> attachHandlers;
static bool performedThreadAttach;

void triggerThreadAttach() {
    if (!performedThreadAttach) {
        std::lock_guard<std::mutex> guard(attachHandlersMutex);
        performedThreadAttach = true;
        for (auto handler : attachHandlers) handler();
    }
}

bool EXPORTS_IsLoaderModuleLoaded(const char* moduleName) {
    return dllLoader->loadedModules.find(moduleName) != dllLoader->loadedModules.end();
}

void EXPORTS_AddThreadAttachHandler(ThreadAttachHandler attachHandler) {
    std::lock_guard<std::mutex> guard(attachHandlersMutex);
    attachHandlers.push_back(attachHandler);
    if (performedThreadAttach) attachHandler();
}

HLOADEDMODULE EXPORTS_LoadModule(const char* filePath) {
    if (dllLoader == nullptr) {
        Logging::logFile << "WARNING: loadPatchModule called before dllLoader was initialized!" << std::endl;
        return nullptr;
    }
    Logging::logFile << "Attempting to load module: " << filePath << std::endl;
    return dllLoader->LoadModule(filePath);
}

FARPROC EXPORTS_GetModuleProcAddress(HLOADEDMODULE module, const char* symbolName) {
    return MemoryGetProcAddress(module, symbolName);
}

void discoverLoaderMods(std::unordered_map<std::string, HLOADEDMODULE> discoveredModules) {
    for (auto& file : std::filesystem::directory_iterator("loaders")) {
        if (file.is_regular_file() && file.path().extension() == "dll") {
            Logging::logFile << "Discovering loader module candidate " << file.path().filename() << std::endl;
            HLOADEDMODULE loadedModule = dllLoader->LoadModule("UE4-ExampleMod-Win64-Shipping.dll");
            if (loadedModule != nullptr) {
                Logging::logFile << "Successfully loaded module " << file.path().filename() << std::endl;
                discoveredModules.insert({file.path().filename().string(), loadedModule});
            }
        }
    }
}

void bootstrapLoaderMods(const std::unordered_map<std::string, HLOADEDMODULE>& discoveredModules) {
    for (auto& loaderModule : discoveredModules) {
        FARPROC bootstrapFunc = MemoryGetProcAddress(loaderModule.second, "BootstrapModule");
        if (bootstrapFunc == nullptr) {
            Logging::logFile << "[WARNING]: BootstrapModule() not found in loader module " << loaderModule.first << "!" << std::endl;
            return;
        }
        BootstrapAccessors accessors{
            Logging::logFile,
            &EXPORTS_LoadModule,
            &EXPORTS_GetModuleProcAddress,
            &EXPORTS_IsLoaderModuleLoaded,
            &EXPORTS_AddThreadAttachHandler
        };
        try {
            ((BootstrapModuleFunc) bootstrapFunc)(accessors);
        } catch (std::exception& ex) {
            Logging::logFile << "[FATAL] Module " << loaderModule.first << " thrown exception during initialization: " << ex.what() << std::endl;
            exit(1);
        }
    }
}

void setupExecutableHook() {
    Logging::initializeLogging();
    Logging::logFile << "Setting up hooking" << std::endl;
    try {
        auto* resolver = new ImportResolver(GAME_MODULE_NAME);
        dllLoader = new DllLoader(resolver);
        Logging::logFile << "Discovering loader modules..." << std::endl;
        std::unordered_map<std::string, HLOADEDMODULE> discoveredMods;
        discoverLoaderMods(discoveredMods);
        Logging::logFile << "Bootstrapping loader modules..." << std::endl;
        bootstrapLoaderMods(discoveredMods);
    } catch (std::exception& ex) {
        Logging::logFile << "[FATAL] Failed to initialize import resolver: " << ex.what() << std::endl;
        exit(1);
    }
}

