#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "logging.h"
#include "DllLoader.h"
#include <MemoryModule.h>
#include <filesystem>
#include <mutex>
#include <map>
#include "exports.h"
#include "util.h"
using namespace std::filesystem;

#define GAME_MODULE_NAME "FactoryGame-Win64-Shipping.exe"

static DllLoader* dllLoader;

extern "C" __declspec(dllexport) const wchar_t* bootstrapperVersion = L"2.0.2";

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

std::string GetLastErrorAsString();

void discoverLoaderMods(std::map<std::string, HLOADEDMODULE>& discoveredModules, const std::filesystem::path& rootGameDirectory) {
    std::filesystem::path directoryPath = rootGameDirectory / "loaders";
    std::filesystem::create_directories(directoryPath);
    for (auto& file : std::filesystem::directory_iterator(directoryPath)) {
        if (file.is_regular_file() && file.path().extension() == ".dll") {
            Logging::logFile << "Discovering loader module candidate " << file.path().filename() << std::endl;
            HLOADEDMODULE loadedModule = nullptr;
            loadedModule = dllLoader->LoadModule(nullptr, file.path().wstring().c_str());
            if (loadedModule != nullptr) {
                Logging::logFile << "Successfully loaded module " << file.path().filename() << std::endl;
                discoveredModules.insert({file.path().filename().string(), loadedModule});
            } else {
                Logging::logFile << "Failed to load module " << file.path().filename() << ": " << std::endl;
                Logging::logFile << "Last Error Message: " << GetLastErrorAsString() << std::endl;
                exit(1);
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
            &EXPORTS_ResolveModuleSymbol,
            bootstrapperVersion
        };
        Logging::logFile << "Bootstrapping module " << loaderModule.first << std::endl;
        ((BootstrapModuleFunc) bootstrapFunc)(accessors);
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
    return rootDirPath;
}

void setupExecutableHook(HMODULE selfModuleHandle) {
    //fast route to exit before locking on mutex
    if (hookAlreadySetup) return;
    std::lock_guard guard(setupHookMutex);
    //check if we have loaded while we loaded on mutex
    if (hookAlreadySetup) return;
    hookAlreadySetup = true; //mark as loaded

    //initialize systems, load symbols, call bootstrapper modules
    Logging::initializeLogging();
    Logging::logFile << "Setting up hooking" << std::endl;

    path rootGameDirectory = resolveGameRootDir();
    path bootstrapperDirectory = path(getModuleFileName(selfModuleHandle)).parent_path();
    Logging::logFile << "Game Root Directory: " << rootGameDirectory << std::endl;
    Logging::logFile << "Bootstrapper Directory: " << bootstrapperDirectory << std::endl;

    HMODULE gameModule = GetModuleHandleA(GAME_MODULE_NAME);
    if (gameModule == nullptr) {
        Logging::logFile << "Failed to find primary game module with name: " << GAME_MODULE_NAME << std::endl;
        exit(1);
    }
    path diaDllPath = bootstrapperDirectory / "msdia140.dll";
    HMODULE diaDllHandle = LoadLibraryW(diaDllPath.wstring().c_str());
    if (diaDllHandle == nullptr) {
        Logging::logFile << "Failed to load DIA SDK implementation DLL." << std::endl;
        Logging::logFile << "Expected to find it at: " << diaDllPath.string() << std::endl;
        Logging::logFile << "Make sure it is here and restart. Exiting now." << std::endl;
        exit(1);
    }
    //TODO strict mode where missing symbols result in aborting?
    auto *resolver = new SymbolResolver(gameModule, diaDllHandle, false);
    dllLoader = new DllLoader(resolver);
    Logging::logFile << "Discovering loader modules..." << std::endl;
    std::map<std::string, HLOADEDMODULE> discoveredMods;
    discoverLoaderMods(discoveredMods, rootGameDirectory);

    Logging::logFile << "Bootstrapping loader modules..." << std::endl;
    bootstrapLoaderMods(discoveredMods, rootGameDirectory.wstring());

    Logging::logFile << "Successfully performed bootstrapping." << std::endl;
}

std::string GetLastErrorAsString() {
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, nullptr);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}
