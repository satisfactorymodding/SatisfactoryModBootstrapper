#ifndef XINPUT1_3_EXPORTS_H
#define XINPUT1_3_EXPORTS_H

typedef __int64(__stdcall *FUNCTION_PTR)();
typedef void* HLOADEDMODULE;
typedef HLOADEDMODULE(*LoadModuleFunc)(const char*, const wchar_t* filePath);
typedef FUNCTION_PTR(*GetModuleProcAddress)(HLOADEDMODULE module, const char* symbolName);
typedef bool(*IsLoaderModuleLoaded)(const char* moduleName);
/**
 * @deprecated Use DigestGameSymbol instead
 */
typedef FUNCTION_PTR(*ResolveGameSymbolPtr)(const char* symbolName);
typedef void(*FlushDebugSymbolsFunc)();

/**
 * @param symbolName name of the symbol to search for. Can be both decorated and undecorated name
 * @note When multiple symbols with same name are found, only bMultipleSymbolsMatch is set to true
 * @note You have to manually free SymbolName with provided free to avoid memory leaks!
 * @return information about symbol
 */
typedef struct SymbolDigestInfo(*DigestGameSymbolFunc)(const wchar_t* symbolName);

/**
 * @return list of symbol root directories joined by ';' symbol;
 * @note Memory will be allocated by using provided malloc and **should be freed manually by the caller**
 */
#define SYMBOL_ROOT_SEPARATOR L";"
typedef wchar_t* (*GetSymbolFileRootsFunc)(void*(*malloc)(unsigned long long));

/**
 * Creates function thunk usable for overriding virtual table of the constructor function
 * Before using it, set OutTrampolineAddress to the correct pointer to the original constructor
 * You can add hooked functions via
 */
typedef struct ConstructorHookThunk(*CreateConstructorHookThunkFunc)();

/**
 * Adds virtual function hook to the given constructor thunk
 * @return true when successfully hooked, false otherwise
 */
typedef bool(*AddConstructorHookFunc)(struct ConstructorHookThunk ConstructorThunk, struct VirtualFunctionHookInfo HookInfo);

typedef void(*FreeStringFunc)(wchar_t* String);

struct SymbolDigestInfo {
    bool bSymbolNotFound;
    bool bSymbolOptimizedAway;
    bool bSymbolVirtual;
    bool bMultipleSymbolsMatch;
    void* SymbolImplementationPointer;
    wchar_t* SymbolName;
    FreeStringFunc SymbolNameFree;
};

struct ConstructorHookThunk {
    void* OpaquePointer;
    void* GeneratedThunkAddress;
    //set this address after hooking to point to the trampoline function
    void** OutTrampolineAddress;
};

struct VirtualFunctionHookInfo {
    void* MemberFunctionPointer;
    int MemberFunctionPointerSize;
    void* FunctionToCallInstead;
    void** OutOriginalFunctionPtr;
};

struct BootstrapAccessors {
    const wchar_t* gameRootDirectory;
    LoadModuleFunc LoadModule;
    GetModuleProcAddress GetModuleProcAddress;
    IsLoaderModuleLoaded IsLoaderModuleLoaded;
    ResolveGameSymbolPtr ResolveGameSymbol;
    const wchar_t* version;
    FlushDebugSymbolsFunc FlushDebugSymbols;
    GetSymbolFileRootsFunc GetSymbolFileRoots;
    DigestGameSymbolFunc DigestGameSymbol;
    CreateConstructorHookThunkFunc CreateConstructorHookThunk;
    AddConstructorHookFunc AddConstructorHook;
};

typedef void(*BootstrapModuleFunc)(BootstrapAccessors& accessors);

#endif //XINPUT1_3_EXPORTS_H
