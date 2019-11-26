#include <Windows.h>
#include <iostream>
#include "logging.h"
#include "DllLoader.h"

#define MAX_MODULE_PATH_SIZE 500

void setupExecutableHook() {
    Logging::initializeLogging();
    Logging::logFile << "Setting up hooking" << std::endl;


    HMODULE hGamePrimaryModule = GetModuleHandleA("FactoryGame-Win64-Shipping.exe");
    try {
        ImportResolver* resolver = new ImportResolver("FactoryGame-Win64-Shipping.exe");
        DllLoader* dllLoader = new DllLoader(resolver);
        dllLoader->LoadModule("UE4-ExampleMod-Win64-Shipping.dll");
    } catch (std::exception ex) {
        Logging::logFile << "Failed to initialize import resolver: " << ex.what() << std::endl;
        exit(1);
    }

   // std::string exampleSymbol("agsDeInit");
    //void* agsDeInitAddress = resolver->ResolveSymbol(exampleSymbol);
  //  std::cout << "agsDeInit Address: << " << agsDeInitAddress << std::endl;
   // delete resolver;
}

