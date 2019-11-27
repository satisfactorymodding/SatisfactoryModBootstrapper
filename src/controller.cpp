#include "logging.h"
#include "DllLoader.h"

typedef HMODULE (*LoadModuleFunc)(const char* filePath);
typedef FARPROC (*BootstrapModuleFunc)(LoadModuleFunc loadModuleFuncPtr);

static DllLoader* dllLoader;

HMODULE LoadModuleInternal(const char* filePath) {
    if (dllLoader == nullptr) {
        Logging::logFile << "WARNING: loadPatchModule called before dllLoader was initialized!" << std::endl;
        return nullptr;
    }
    Logging::logFile << "Attempting to load module: " << filePath << std::endl;
    return dllLoader->LoadModule(filePath);
}

void setupExecutableHook() {
    Logging::initializeLogging();
    Logging::logFile << "Setting up hooking" << std::endl;

    try {
        auto* resolver = new ImportResolver("FactoryGame-Win64-Shipping.exe");
        dllLoader = new DllLoader(resolver);
        HMODULE loadedModule = dllLoader->LoadModule("UE4-ExampleMod-Win64-Shipping.dll");
        FARPROC bootstrapFunc = GetProcAddress(loadedModule, "BootstrapModule");
        if (bootstrapFunc == nullptr) {
            Logging::logFile << "BootstrapModule() not found in loaded module!";
            return;
        }
        Logging::logFile << "Calling BootstrapModule() on init module" << std::endl;
        ((BootstrapModuleFunc) bootstrapFunc)(&LoadModuleInternal);
    } catch (std::exception& ex) {
        Logging::logFile << "Failed to initialize import resolver: " << ex.what() << std::endl;
        exit(1);
    }
}

