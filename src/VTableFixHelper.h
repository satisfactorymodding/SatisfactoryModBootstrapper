#ifndef XINPUT_1_3_VTABLEFIXHELPER_H
#define XINPUT_1_3_VTABLEFIXHELPER_H
#include "SymbolResolver.h"
#include <vector>

struct VTableFixEntry {
    unsigned int FunctionEntryOffset;
    void* FunctionToCallInstead;
    void** OutOriginalFunctionPtr;
};

struct VTableDefinition {
    unsigned int VirtualTableOriginOffset;
    bool bFlushCaches;
    std::vector<VTableFixEntry> FixEntries;
};

struct ConstructorFixInfo {
    std::vector<VTableDefinition> Fixes;
};

void ApplyConstructorFixes(void* ThisPointer, ConstructorFixInfo* FixInfo);

#endif //XINPUT_1_3_VTABLEFIXHELPER_H
