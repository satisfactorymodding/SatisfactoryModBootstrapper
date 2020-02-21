#include "logging.h"
#include "DllLoader.h"
#include <MemoryModule.h>
#include <filesystem>
#include <mutex>
#include <map>
#include "exports.h"

#define GAME_MODULE_NAME "FactoryGame-Win64-Shipping.exe"

static DllLoader* dllLoader;

bool EXPORTS_IsLoaderModuleLoaded(const char* moduleName) {
    return dllLoader->loadedModules.find(moduleName) != dllLoader->loadedModules.end();
}

HLOADEDMODULE EXPORTS_LoadModule(const char* moduleName, const wchar_t* filePath) {
    if (dllLoader == nullptr) {
        Logging::logFile << "WARNING: loadPatchModule called before dllLoader was initialized!" << std::endl;
        return nullptr;
    }
    Logging::logFile << "Attempting to load module: " << moduleName << " from " << filePath << std::endl;
    return dllLoader->LoadModule(moduleName, filePath);
}

FUNCTION_PTR EXPORTS_GetModuleProcAddress(HLOADEDMODULE module, const char* symbolName) {
    return MemoryGetProcAddress(module, symbolName);
}

FUNCTION_PTR EXPORTS_ResolveModuleSymbol(const char* symbolName) {
    return reinterpret_cast<FUNCTION_PTR>(dllLoader->resolver->ResolveSymbol(symbolName));
}

void discoverLoaderMods(std::map<std::string, HLOADEDMODULE>& discoveredModules, const std::filesystem::path& rootGameDirectory) {
    std::filesystem::path directoryPath = rootGameDirectory / "loaders";
    std::filesystem::create_directories(directoryPath);
    for (auto& file : std::filesystem::directory_iterator(directoryPath)) {
        if (file.is_regular_file() && file.path().extension() == ".dll") {
            Logging::logFile << "Discovering loader module candidate " << file.path().filename() << std::endl;
            HLOADEDMODULE loadedModule = nullptr;
            try {
                loadedModule = dllLoader->LoadModule(nullptr, file.path().generic_wstring().c_str());
            } catch (std::exception& ex) {
                Logging::logFile << "[FATAL] Failed to load module " << file.path().filename() << ": " << ex.what() << std::endl;
                exit(1);
            }
            if (loadedModule != nullptr) {
                Logging::logFile << "Successfully loaded module " << file.path().filename() << std::endl;
                discoveredModules.insert({file.path().filename().string(), loadedModule});
            }
        }
    }
}

void bootstrapLoaderMods(const std::map<std::string, HLOADEDMODULE>& discoveredModules, const std::wstring& gameRootDirectory) {
    for (auto& loaderModule : discoveredModules) {
        FUNCTION_PTR bootstrapFunc = MemoryGetProcAddress(loaderModule.second, "BootstrapModule");
        if (bootstrapFunc == nullptr) {
            Logging::logFile << "[WARNING]: BootstrapModule() not found in loader module " << loaderModule.first << "!" << std::endl;
            return;
        }
        BootstrapAccessors accessors{
            gameRootDirectory.c_str(),
            &EXPORTS_LoadModule,
            &EXPORTS_GetModuleProcAddress,
            &EXPORTS_IsLoaderModuleLoaded,
            &EXPORTS_ResolveModuleSymbol
        };
        Logging::logFile << "Bootstrapping module " << loaderModule.first << std::endl;
        try {
            ((BootstrapModuleFunc) bootstrapFunc)(accessors);
        } catch (std::exception& ex) {
            Logging::logFile << "[FATAL] Module " << loaderModule.first << " thrown exception during initialization: " << ex.what() << std::endl;
            exit(1);
        }
    }
}

static std::mutex setupHookMutex;
static volatile bool hookAlreadySetup = false;

std::filesystem::path resolveGameRootDir() {
    wchar_t pathBuffer[2048]; //just to be sure it will always fit
    GetModuleFileNameW(GetModuleHandleA(GAME_MODULE_NAME), pathBuffer, 2048);
    std::filesystem::path rootDirPath{pathBuffer};
    std::string gameFolderName(GAME_MODULE_NAME);
    gameFolderName.erase(gameFolderName.find('-'));
    //we go up the directory tree until we find the folder called
    //FactoryGame, which denotes the root of the game directory
    while (!std::filesystem::exists(rootDirPath / gameFolderName)) {
        rootDirPath = rootDirPath.parent_path();
    }
    Logging::logFile << "Game Root Directory: " << rootDirPath.generic_string() << std::endl;
    return rootDirPath;
}

void setupExecutableHook() {
    //fast route to exit before locking on mutex
    if (hookAlreadySetup) return;
    std::lock_guard guard(setupHookMutex);
    //check if we have loaded while we loaded on mutex
    if (hookAlreadySetup) return;
    hookAlreadySetup = true; //mark as loaded
    //initialize systems, load symbols, call bootstrapper modules
    Logging::initializeLogging();
    Logging::logFile << "Setting up hooking" << std::endl;
    std::filesystem::path rootGameDirectory = resolveGameRootDir();

    try {
        auto* resolver = new ImportResolver(GAME_MODULE_NAME);
        HANDLE moduleHandle = GetModuleHandleA(GAME_MODULE_NAME);
        void* agsDeInit = resolver->ResolveSymbol("void agsDeInit()");
        Logging::logFile << "AGSDeInit: " << agsDeInit << std::endl;
        auto offsetPtr = reinterpret_cast<ULONG64>(agsDeInit) - reinterpret_cast<ULONG64>(moduleHandle);
        Logging::logFile << "Offset ptr: " << offsetPtr << std::endl;

        dllLoader = new DllLoader(resolver);
        Logging::logFile << "Discovering loader modules..." << std::endl;
        std::map<std::string, HLOADEDMODULE> discoveredMods;
        discoverLoaderMods(discoveredMods, rootGameDirectory);

        Logging::logFile << "Bootstrapping loader modules..." << std::endl;
        bootstrapLoaderMods(discoveredMods, rootGameDirectory.generic_wstring());

        Logging::logFile << "Successfully performed bootstrapping." << std::endl;
    } catch (std::exception& ex) {
        Logging::logFile << "[FATAL] Failed to initialize import resolver: " << ex.what() << std::endl;
        exit(1);
    }
}

