#ifndef XINPUT1_3_IMPORTRESOVER_H
#define XINPUT1_3_IMPORTRESOVER_H

#include <Windows.h>
#include <xstring>
#include <CTypeInfoText.h>

class ImportResolver {
private:
    HMODULE hGamePrimaryModule;
    HANDLE hProcess;
    ULONG64 gameDllBase;
    CTypeInfoDump* typeInfoDump;
    CTypeInfoText* infoText;

public:
    ImportResolver(const char* gameModuleName);
    ~ImportResolver();

    void* ResolveSymbol(std::string symbolName);
};

#endif //XINPUT1_3_IMPORTRESOVER_H
