#include "ImportResover.h"
#include "logging.h"

#include <vector>
#include <iostream>
#include "naming_util.h"
#include <psapi.h>

using namespace std;

BOOL CALLBACK ProcessFunctionCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, void* UserContext) {
    auto* resultVec = static_cast<vector<SymbolInfo>*>(UserContext);
    SymbolInfo symbolInfo{};
    symbolInfo.Name = pSymInfo->Name;
    symbolInfo.Address = pSymInfo->Address;
    symbolInfo.TypeId = pSymInfo->TypeIndex;
    resultVec->push_back(symbolInfo);
    return false;
}

ULONG64 findSymbolLocation(HANDLE hProcess, ULONG64 dllBase, CTypeInfoText& infoText, std::string& functionSignature) {
    std::vector<SymbolInfo> symbolInfo;
    std::string functionName = getFunctionName(functionSignature);
    SymEnumSymbols(hProcess, dllBase, functionName.c_str(), ProcessFunctionCallback, (void*) &symbolInfo);
    if (symbolInfo.empty()) {
        Logging::logFile << "Symbol not found in executable for: " << functionName << std::endl;
        return NULL;
    }
    if (symbolInfo.size() == 1) {
        //no need to iterate symbols and resolve types if there is only 1 symbol
        return symbolInfo[0].Address;
    }
    Logging::logFile << "Multiple symbols (" << symbolInfo.size() << ") found for " << functionName << ", attempting signature match" << std::endl;
    for (auto &symbolInfo1 : symbolInfo) {
        std::string symbolSignature = createFunctionName(symbolInfo1, infoText);
        if (symbolSignature == functionSignature) {
            return symbolInfo1.Address;
        }
    }
    Logging::logFile << "No matching symbol found in executable for: " << functionName << ". None of below matches: " << functionSignature << std::endl;
    for (auto &symbolInfo1 : symbolInfo) {
        std::string symbolSignature = createFunctionName(symbolInfo1, infoText);
        Logging::logFile << "  " << symbolSignature << std::endl;
    }
    return NULL;
}

ULONG64 findSymbolLocationDecorated(HANDLE hProcess, ULONG64 dllBase, CTypeInfoText& infoText, std::string& functionName) {
    char undecoratedName[MAX_SYM_NAME];
    UnDecorateSymbolName(functionName.c_str(), undecoratedName, MAX_SYM_NAME, 0);
    functionName.assign(undecoratedName);
    formatUndecoratedName(functionName);
    return findSymbolLocation(hProcess, dllBase, infoText, functionName);
}

void* ImportResolver::ResolveSymbol(std::string symbolName) {
    return reinterpret_cast<void *>(findSymbolLocationDecorated(hProcess, gameDllBase, *infoText, symbolName));
}

ImportResolver::ImportResolver(const char* gameModuleName) {
    this->hProcess = GetCurrentProcess();
    Logging::logFile << "Game Module Name: " << gameModuleName << std::endl;
    this->hGamePrimaryModule = GetModuleHandleA(gameModuleName);
    if (hGamePrimaryModule == nullptr) {
        throw std::invalid_argument("Game module with name specified cannot be found");
    }
    SymInitialize(hProcess, nullptr, FALSE);
    char moduleFileNameRaw[2048];
    GetModuleFileNameA(hGamePrimaryModule, moduleFileNameRaw, 2048);
    std::string moduleFileName(moduleFileNameRaw);
    moduleFileName.replace(moduleFileName.find_last_of('.'), 4, ".pdb");
    Logging::logFile << "PDB file path: " << moduleFileName << std::endl;
    //have to use PDB file path here because for some reason for executable it cannot find static symbols
    MODULEINFO moduleInfo;
    GetModuleInformation(hProcess, hGamePrimaryModule, &moduleInfo, sizeof(moduleInfo));
    Logging::logFile << "Game Module Size: " << moduleInfo.SizeOfImage << "; DllBase = " << moduleInfo.lpBaseOfDll << std::endl;
    this->gameDllBase = SymLoadModuleEx(hProcess, nullptr, moduleFileName.c_str(), gameModuleName,
            (DWORD64) moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage, nullptr, 0);
    if (gameDllBase == NULL) {
        throw std::invalid_argument("Cannot load debug symbols for specified game module");
    }
    this->typeInfoDump = new CTypeInfoDump(hProcess, gameDllBase);
    this->infoText = new CTypeInfoText(typeInfoDump);
}

ImportResolver::~ImportResolver() {
    if (gameDllBase != NULL) {
        SymUnloadModule64(hProcess, gameDllBase);
    }
    delete typeInfoDump;
    delete infoText;
}
