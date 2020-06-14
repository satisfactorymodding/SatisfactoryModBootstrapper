#ifndef XINPUT1_3_ASSEMBLY_ANALYZER_H
#define XINPUT1_3_ASSEMBLY_ANALYZER_H
#include <cstdint>

template<typename FunctionPtrType>
struct PointerContainer {
    FunctionPtrType MyPointer;
};

struct MemberFunctionInfo {
    //Code to the real implementation of the function
    //Will be null pointer if function is virtual!
    void* OriginalCodePointer;
    uint32_t ThisAdjustment;
    bool bIsVirtualFunctionThunk;
    uint32_t VirtualTableOffset;
};

MemberFunctionInfo DigestMemberFunctionPointer(void* FunctionPointerContainer, size_t ContainerSize);

#endif //XINPUT1_3_ASSEMBLY_ANALYZER_H
