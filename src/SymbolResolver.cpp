#include "DestructorGenerator.h"
#include "SymbolResolver.h"
#include "logging.h"
#include "atlbase.h"
#include "comdef.h"
#include "util.h"
#include <psapi.h>
#include "provided_symbols.h"

// Implemented in VC CRT (msvcVERSION.dll or vcruntimeVERSION.dll or UCRT (Windows 10 only))
extern "C" char * __unDName(char* outputString, const char* name, int maxStringLength, void* (*pAlloc)(size_t), void(*pFree)(void*), unsigned short disableFlags);

#define CHECK_FAILED(hr, message) \
    if (FAILED(hr)) { \
        _com_error err(hr); \
        LPCTSTR errMsg = err.ErrorMessage(); \
        Logging::logFile << message << errMsg << std::endl; \
        exit(1); \
    }

HRESULT CoCreateDiaDataSource(HMODULE diaDllHandle, CComPtr<IDiaDataSource>& data_source);

SymbolResolver::SymbolResolver(HMODULE gameModuleHandle, HMODULE diaDllHandle, bool exitOnUnresolvedSymbol) {
    this->exitOnUnresolvedSymbol = exitOnUnresolvedSymbol;
    CComPtr<IDiaDataSource> dataSource;
    HRESULT hr = CoCreateDiaDataSource(diaDllHandle, dataSource);
    CHECK_FAILED(hr, "Failed to create IDiaDatSource: ");
    std::wstring executablePath = getModuleFileName(gameModuleHandle);
    (*dataSource).loadDataForExe(executablePath.c_str(), nullptr, nullptr);
    CHECK_FAILED(hr, "Failed to load DIA data from executable file: ");
    hr = (*dataSource).openSession(&diaSession);
    CHECK_FAILED(hr, "Failed to open DIA session: ");
    //because HMODULE value is the same as the DLL load base address, according to the MS documentation
    hr = (*diaSession).put_loadAddress(reinterpret_cast<ULONGLONG>(gameModuleHandle));
    CHECK_FAILED(hr, "Failed to update DLL load address on IDiaSession: ");
    hr = (*diaSession).get_globalScope(&globalSymbol);
    CHECK_FAILED(hr, "Failed to retrieve global DLL scope");
    dllBaseAddress = (LPVOID) gameModuleHandle;
    destructorGenerator = new DestructorGenerator(dllBaseAddress, globalSymbol);
    hookRequiredSymbols(*this);
}

void DummyUnresolvedSymbolHandler(const char* symbolName) {
    Logging::logFile << "Attempt to call unresolved symbol from a loaded module code!!!" << std::endl;
    Logging::logFile << "This is a dummy symbol and it cannot be called. Aborting." << std::endl;
    Logging::logFile << "Symbol Name: " << symbolName << std::endl;
    exit(1);
}

void* SymbolResolver::ResolveSymbol(const char* mangledSymbolName) {
    USES_CONVERSION;
    const wchar_t* resultName = A2CW(mangledSymbolName);
    CComPtr<IDiaEnumSymbols> enumSymbols;
    HRESULT hr = (*globalSymbol).findChildren(SymTagNull, resultName, nsfCaseSensitive, &enumSymbols);
    CHECK_FAILED(hr, "Failed find symbol in executable: ");
    LONG symbolCount = 0L;
    enumSymbols->get_Count(&symbolCount);
    if (symbolCount == 0L) {
        void* providedSymbolPointer = provideSymbolImplementation(mangledSymbolName);
        if (providedSymbolPointer != nullptr) {
            return providedSymbolPointer; //fallback to provided symbol
        }
        Logging::logFile << "[FATAL] Executable missing symbol with mangled name: " << mangledSymbolName << std::endl;
        char* demangledName = __unDName(nullptr, mangledSymbolName, 0, malloc, free, 0);
        Logging::logFile << "[FATAL] De-mangled symbol name (for reference): " << demangledName << std::endl;
        free(demangledName);
        if (exitOnUnresolvedSymbol) {
            Logging::logFile << "[FATAL] Strict mode enabled. Aborting on missing symbol." << std::endl;
            exit(1);
        }
        Logging::logFile << "[FATAL] Overriding it with dummy symbol. Bad things will happen if it is going to be actually called!" << std::endl;
        return generateDummySymbol(demangledName, &DummyUnresolvedSymbolHandler);
    }
    CComPtr<IDiaSymbol> resolvedSymbol;
    hr = enumSymbols->Item(0, &resolvedSymbol);
    CHECK_FAILED(hr, "Failed to retrieve first matching symbol: ");
    DWORD resultAddress;
    hr = resolvedSymbol->get_relativeVirtualAddress(&resultAddress);
    CHECK_FAILED(hr, "Failed to retrieve symbol relative virtual address: ");
    return reinterpret_cast<void *>((unsigned long long)dllBaseAddress + resultAddress);
}

SymbolDigestInfo SymbolResolver::DigestGameSymbol(const wchar_t* SymbolName) {
    CComPtr<IDiaEnumSymbols> enumSymbols;
    HRESULT hr = (*globalSymbol).findChildren(SymTagNull, SymbolName, nsfCaseSensitive, &enumSymbols);
    CHECK_FAILED(hr, "findChildren failed in executable");
    LONG SymbolCount = 0L;
    enumSymbols->get_Count(&SymbolCount);
    SymbolDigestInfo ResultDigestInfo{};
    if (SymbolCount == 0) {
        ResultDigestInfo.bSymbolNotFound = true;
        return ResultDigestInfo;
    }
    if (SymbolCount > 1) {
        ResultDigestInfo.bMultipleSymbolsMatch = true;
        return ResultDigestInfo;
    }
    CComPtr<IDiaSymbol> ResultSymbol;
    enumSymbols->Item(0, &ResultSymbol);
    ResultSymbol->get_undecoratedName(&ResultDigestInfo.SymbolName.String);
    ResultDigestInfo.SymbolName.StringFree = &SysFreeString;
    DWORD LocationType = 0;
    ResultSymbol->get_locationType(&LocationType);
    if (LocationType == LocIsNull) {
        ResultDigestInfo.bSymbolOptimizedAway = LocationType == LocIsNull;
        return ResultDigestInfo;
    }
    BOOL bIsVirtual = false;
    ResultSymbol->get_virtual(&bIsVirtual);
    ResultDigestInfo.bSymbolVirtual = bIsVirtual;

    if (LocationType == LocIsStatic) {
        DWORD RelativeVirtualAddress;
        ResultSymbol->get_relativeVirtualAddress(&RelativeVirtualAddress);
        void* SymbolPointer = reinterpret_cast<void*>((uint64_t)dllBaseAddress + RelativeVirtualAddress);
        ResultDigestInfo.SymbolImplementationPointer = SymbolPointer;
    }
    return ResultDigestInfo;
}

SymbolResolver::~SymbolResolver() = default;


HRESULT CoCreateDiaDataSource(HMODULE diaDllHandle, CComPtr<IDiaDataSource>& data_source) {
    auto DllGetClassObject = (BOOL (WINAPI*)(REFCLSID, REFIID, LPVOID *)) GetProcAddress(diaDllHandle, "DllGetClassObject");
    if (!DllGetClassObject) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    CComPtr<IClassFactory> pClassFactory;
    HRESULT hr = DllGetClassObject(CLSID_DiaSource, IID_IClassFactory, reinterpret_cast<LPVOID *>(&pClassFactory));
    if (FAILED(hr)) {
        return hr;
    }
    hr = pClassFactory->CreateInstance(nullptr, IID_IDiaDataSource, (void **) &data_source);
    if (FAILED(hr)) {
        return hr;
    }
    return S_OK;
}
