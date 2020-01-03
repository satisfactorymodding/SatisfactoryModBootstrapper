#include "ImportResover.h"
#include "logging.h"

#include <vector>
#include <iostream>
#include "naming_util.h"
#include <psapi.h>

using namespace std;

static const wchar_t* StaticConfigName_UObject() {
    return L"UObject";
}
static const wchar_t* StaticConfigName_AActor() {
    return L"AActor";
}
static const wchar_t* StaticConfigName_UActorComponent() {
    return L"UActorComponent";
}
static const wchar_t* StaticConfigName_Default() {
    return L"UnknownConfigName";
}

static void UnimplementedSymbol() {
    Logging::logFile << "UnimplementedSymbol called!" << std::endl;
    Logging::logFile << "That means one of the symbols your module DLL referenced is not there, and it was called" << std::endl;
    Logging::logFile << "Read this log from the start to pinpoint the issue" << std::endl;
    exit(0xBADCA11);
}

struct SymbolLookupInfo {
    const std::string& functionSignature;
    CTypeInfoText& infoText;
    bool foundTarget = false;
    SymbolInfo resultInfo;
    int symbolsFound = 0;
    std::vector<SymbolInfo> allFoundSymbols;
};

BOOL CALLBACK ProcessFunctionCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, void* UserContext) {
    auto *lookupInfo = static_cast<SymbolLookupInfo *>(UserContext);
    SymbolInfo symbolInfo{};
    symbolInfo.Name = pSymInfo->Name;
    symbolInfo.Address = pSymInfo->Address;
    symbolInfo.TypeId = pSymInfo->TypeIndex;
    if (pSymInfo->Tag == SymTagFunction) {
        //this is function, try fast lookup
        if (createFunctionName(symbolInfo, lookupInfo->infoText) == lookupInfo->functionSignature) {
            lookupInfo->resultInfo = symbolInfo;
            lookupInfo->foundTarget = true;
            return false; //stop enumeration
        }
    } else {
        //this is something else, first match is fine
        lookupInfo->resultInfo = symbolInfo;
        lookupInfo->foundTarget = true;
        return false; //stop enumeration
    }
    lookupInfo->allFoundSymbols.push_back(symbolInfo);
    lookupInfo->symbolsFound++;
    return true; //continue enumeration
}

void* findBuiltinSymbolAddress(const std::string& functionName, const std::string& functionSignature) {
    if (functionName == "UObject::StaticConfigName") return reinterpret_cast<void*>(&StaticConfigName_UObject);
    if (functionName == "AActor::StaticConfigName") return reinterpret_cast<void*>(&StaticConfigName_AActor);
    if (functionName == "UActorComponent::StaticConfigName") return reinterpret_cast<void*>(&StaticConfigName_UActorComponent);
    if (functionName.find("::StaticConfigName") != std::string::npos) return reinterpret_cast<void*>(&StaticConfigName_Default);
    return nullptr;
}

ULONG64 findSymbolLocation(HANDLE hProcess, ULONG64 dllBase, CTypeInfoText& infoText, const std::string& functionSignature) {
    std::string functionName = getFunctionName(functionSignature);
    //try builtin symbol first
    void* builtInFunctionAddress = findBuiltinSymbolAddress(functionName, functionSignature);
    if (builtInFunctionAddress != nullptr) {
        return reinterpret_cast<ULONG64>(builtInFunctionAddress);
    }
    //try to fast-find required symbol
    SymbolLookupInfo lookupInfo{functionSignature, infoText};
    SymEnumSymbols(hProcess, dllBase, functionName.c_str(), ProcessFunctionCallback, (void*) &lookupInfo);
    if (lookupInfo.foundTarget) {
        return lookupInfo.resultInfo.Address;
    }
    //fast lookup failed; try first function found instead
    if (lookupInfo.symbolsFound > 0) {
        bool listCandidates = true;
        if (functionSignature.find("@GENERIC_TYPE@") != std::string::npos) {
            //do not list candidates on generic functions, they're just broken.
            listCandidates = false;
            Logging::logFile << "Using levenshtein distance for generic function " << functionName << std::endl;
        }
        if (listCandidates) {
            Logging::logFile << "Fast symbol lookup failed for symbol " << functionSignature << ", using levenshtein distance" << std::endl;
            Logging::logFile << "Expected signature: " << functionSignature << std::endl;
            Logging::logFile << "All possible candidates: " << std::endl;
        }
        SymbolInfo mostSimilarSymbol;
        double maxSimilarity = 0.0;
        for (auto& symbol : lookupInfo.allFoundSymbols) {
            std::string symbolSignature = createFunctionName(symbol, infoText);
            double similarity = computeStringSimilarity(functionSignature, symbolSignature);
            if (similarity > maxSimilarity) {
                maxSimilarity = similarity;
                mostSimilarSymbol = symbol;
            }
            if (listCandidates) {
                Logging::logFile << symbolSignature << std::endl;
            }
        }
        if (listCandidates) {
            Logging::logFile << "Picking most similar symbol " << mostSimilarSymbol.Name << std::endl;
        }
        return mostSimilarSymbol.Address;
    }
    //Lookup failed; print warning and use unimplemented symbol
    Logging::logFile << "Symbol not found in executable for: " << functionName << std::endl;
    return reinterpret_cast<ULONG64>(reinterpret_cast<void*>(&UnimplementedSymbol));
}

ULONG64 findSymbolLocationDecorated(HANDLE hProcess, ULONG64 dllBase, CTypeInfoText& infoText, std::string& functionName) {
    char undecoratedName[MAX_SYM_NAME];
    UnDecorateSymbolName(functionName.c_str(), undecoratedName, MAX_SYM_NAME, 0);
    functionName.assign(undecoratedName);
    Logging::logFile << "UnDecorated name: " << functionName << std::endl;
    formatUndecoratedName(functionName);
    Logging::logFile << "Formatted name: " << functionName << std::endl;
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
