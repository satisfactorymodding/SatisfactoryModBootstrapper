#ifndef XINPUT1_3_IMPORTRESOVER_H
#define XINPUT1_3_IMPORTRESOVER_H

#include <cstdint>
#include <string>

typedef void* HMODULE;
typedef void* HANDLE;
class CTypeInfoDump;
class CTypeInfoText;

class ImportResolver {
private:
    HMODULE hGamePrimaryModule;
    HANDLE hProcess;
    uint64_t gameDllBase;
    CTypeInfoDump* typeInfoDump;
    CTypeInfoText* infoText;

public:
    explicit ImportResolver(const char* gameModuleName);
    ~ImportResolver();

    void* ResolveSymbol(std::string symbolName);
};

#endif //XINPUT1_3_IMPORTRESOVER_H
