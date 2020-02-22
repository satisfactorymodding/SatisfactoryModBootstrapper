#ifndef XINPUT1_3_SYMBOLRESOLVER_H
#define XINPUT1_3_SYMBOLRESOLVER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xstring>
#include <atlbase.h>
#include <dia2.h>

class SymbolResolver {
private:
    CComPtr<IDiaSession> diaSession;
    CComPtr<IDiaSymbol> globalSymbol;
    bool exitOnUnresolvedSymbol;
public:
    explicit SymbolResolver(HMODULE gameModuleHandle, HMODULE diaDllHandle, bool exitOnUnresolvedSymbol);
    ~SymbolResolver();

    void* ResolveSymbol(const char* mangledSymbolName);
};

#endif //XINPUT1_3_SYMBOLRESOLVER_H
