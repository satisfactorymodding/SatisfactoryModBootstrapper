#ifndef XINPUT1_3_SYMBOLRESOLVER_H
#define XINPUT1_3_SYMBOLRESOLVER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xstring>
#include <atlbase.h>
#include <dia2.h>
#include <vector>
#include <string>
#include "exports.h"

class SymbolResolver {
public:
    CComPtr<IDiaSession> diaSession;
    CComPtr<IDiaSymbol> globalSymbol;
    bool exitOnUnresolvedSymbol;
	LPVOID dllBaseAddress;
    class DestructorGenerator* destructorGenerator;
public:
    explicit SymbolResolver(HMODULE gameModuleHandle, HMODULE diaDllHandle, bool exitOnUnresolvedSymbol);
    ~SymbolResolver();

    SymbolDigestInfo DigestGameSymbol(const wchar_t* SymbolName);
    void* ResolveSymbol(const char* mangledSymbolName);
};

#endif //XINPUT1_3_SYMBOLRESOLVER_H
