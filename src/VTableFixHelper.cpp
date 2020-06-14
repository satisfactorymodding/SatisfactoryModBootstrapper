#include "VTableFixHelper.h"
#include <unordered_map>

static std::unordered_map<uint8_t*, uint8_t*> CachedVirtualTables;

uint32_t DetermineVirtualTableSize(void* VirtualTablePtr) {
    //DIA seems to be unable to give exact virtual table size
    //tried get_length and get_staticSize - both return 0 at SymTagPublicSymbol
    //Fallback to 4kB virtual table size for now - it's big enough for most classes
    //without bloating memory too much with hooked virtual tables
    return 4096;
}

uint8_t* CreateAndCacheVirtualTable(uint8_t* OriginalTable, VTableDefinition& Definition) {
    auto RemoveIterator = CachedVirtualTables.find(OriginalTable);
    if (Definition.bFlushCaches && RemoveIterator != CachedVirtualTables.end()) {
        //Remove old value and free memory it occupied
        CachedVirtualTables.erase(RemoveIterator);
        free(RemoveIterator->second);
    }
    Definition.bFlushCaches = false;
    //Allocate enough memory, copy values, apply overrides
    size_t TableSize = DetermineVirtualTableSize(OriginalTable);
    auto* NewVirtualTable = (uint8_t*) malloc(TableSize);
    memcpy(NewVirtualTable, OriginalTable, TableSize);
    //Apply overrides now
    for (VTableFixEntry& FixEntry : Definition.FixEntries) {
        void** FunctionPointer = (void**) (NewVirtualTable + FixEntry.FunctionEntryOffset);
        *FixEntry.OutOriginalFunctionPtr = *FunctionPointer;
        *FunctionPointer = FixEntry.FunctionToCallInstead;
    }
    //Add our entry to the caches map and return it
    CachedVirtualTables.insert({OriginalTable, NewVirtualTable});
    return NewVirtualTable;
}

void ApplyConstructorFixes(void* ThisPointer, ConstructorFixInfo* FixInfo) {
    uint8_t* OffsetPointer = (uint8_t*) ThisPointer;
    for (VTableDefinition& Definition : FixInfo->Fixes) {
        uint32_t OriginOffset = Definition.VirtualTableOriginOffset;
        uint8_t** VirtualTableField = (uint8_t**) (OffsetPointer + OriginOffset);
        uint8_t* VirtualTablePointer = *VirtualTableField;
        uint8_t* NewVirtualTablePointer = CreateAndCacheVirtualTable(VirtualTablePointer, Definition);
        *VirtualTableField = NewVirtualTablePointer;
    }
}
