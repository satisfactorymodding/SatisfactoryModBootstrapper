#include "AssemblyAnalyzer.h"
#include <Zydis/Zydis.h>
#include <iostream>

struct FunctionPointerContainerImpl {
    void* FunctionAddress;
    uint32_t ThisAdjustment;
};

bool IsJumpThunkInstruction(const ZydisDecodedInstruction& Instruction) {
    return Instruction.mnemonic == ZydisMnemonic::ZYDIS_MNEMONIC_JMP && Instruction.operands[0].type == ZydisOperandType::ZYDIS_OPERAND_TYPE_IMMEDIATE;
}

bool IsVirtualTableJumpThunkInstruction(const ZydisDecodedInstruction& Instruction) {
    const ZydisDecodedOperand& FirstOp = Instruction.operands[0];
    return Instruction.mnemonic == ZydisMnemonic::ZYDIS_MNEMONIC_JMP &&
            FirstOp.type == ZydisOperandType::ZYDIS_OPERAND_TYPE_MEMORY &&
            FirstOp.mem.base == ZydisRegister::ZYDIS_REGISTER_RAX &&
            FirstOp.mem.index == ZydisRegister::ZYDIS_REGISTER_NONE;
}

//basically tests for assembly sequence: mov rax, [rcx]
bool IsFirstVirtualTableCallThunkInstruction(const ZydisDecodedInstruction& Instruction) {
    const ZydisDecodedOperand& FirstOp = Instruction.operands[0];
    const ZydisDecodedOperand& SecondOp = Instruction.operands[1];
    return Instruction.mnemonic == ZydisMnemonic::ZYDIS_MNEMONIC_MOV &&
        FirstOp.type == ZydisOperandType::ZYDIS_OPERAND_TYPE_REGISTER &&
        FirstOp.reg.value == ZydisRegister::ZYDIS_REGISTER_RAX &&
        SecondOp.type == ZydisOperandType ::ZYDIS_OPERAND_TYPE_MEMORY &&
        SecondOp.mem.type == ZydisMemoryOperandType::ZYDIS_MEMOP_TYPE_MEM &&
        SecondOp.mem.base == ZydisRegister::ZYDIS_REGISTER_RCX &&
        !SecondOp.mem.disp.has_displacement &&
        SecondOp.mem.index == ZydisRegister::ZYDIS_REGISTER_NONE;
}

void* DiscoverRealFunctionAddress(uint8_t* FunctionPtr, bool& bIsVirtualFunction, uint32_t& VirtualTableOffset) {
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);

    // Initialize formatter. Only required when you actually plan to do instruction
    // formatting ("disassembling"), like we do here
    ZydisFormatter formatter;
    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    // Loop over the instructions in our buffer.
    // The runtime-address (instruction pointer) is chosen arbitrary here in order to better
    // visualize relative addressing
    ZydisDecodedInstruction Instruction;
    bool bFirstInstruction = ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, FunctionPtr, 4096, &Instruction));
    if (!bFirstInstruction) {
        return nullptr; //Invalid sequence - not an instruction
    }
    //test for simple in-module jump thunk
    if (IsJumpThunkInstruction(Instruction)) {
        ZyanU64 ResultJumpAddress;
        ZydisCalcAbsoluteAddress(&Instruction, &Instruction.operands[0], (ZyanU64) FunctionPtr, &ResultJumpAddress);
        return DiscoverRealFunctionAddress((uint8_t*) ResultJumpAddress, bIsVirtualFunction, VirtualTableOffset);
    }
    //test for virtual table call thunk
    if (IsFirstVirtualTableCallThunkInstruction(Instruction)) {
        //second instruction should be jump by pointer with displacement,
        //which will be virtual table offset then
        FunctionPtr += Instruction.length;
        bool bSecondInstruction = ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, FunctionPtr, 4096, &Instruction));
        if (!bSecondInstruction) {
            return nullptr; //Invalid sequence - not an instruction
        }
        //Next instruction should be: jmp qword ptr [rax+Displacement]
        if (IsVirtualTableJumpThunkInstruction(Instruction)) {
            const auto& Displacement = Instruction.operands[0].mem.disp;
            bIsVirtualFunction = true;
            VirtualTableOffset = Displacement.has_displacement ? (uint32_t) Displacement.value : 0;
            return nullptr; //Doesn't have an actual address because it is virtual
        }
    }
    //We can assume this is correct function pointer now
    return FunctionPtr;
}

MemberFunctionInfo DigestMemberFunctionPointer(void* FunctionPointerContainer, size_t ContainerSize) {
    auto* ptr = reinterpret_cast<FunctionPointerContainerImpl*>(FunctionPointerContainer);
    uint32_t ThisAdjustment;
    void* CodePointer;
    if (ContainerSize == 8) {
        //Member function of class with single inheritance, no this adjustment required
        CodePointer = ptr->FunctionAddress;
        ThisAdjustment = 0;
    } else if (ContainerSize == 16) {
        //Class has multiple inheritance, so this adjustment is needed
        CodePointer = ptr->FunctionAddress;
        ThisAdjustment = ptr->ThisAdjustment;
    } else {
        //unsupported case - virtual inheritance probably
        std::cerr << "[WARN] Unsupported member function pointer size: " << ContainerSize << std::endl;
        return MemberFunctionInfo{nullptr};
    };
    bool bIsVirtualFunction = false;
    uint32_t VirtualTableOffset = 0;
    void* RealFunctionAddress = DiscoverRealFunctionAddress((uint8_t*) CodePointer, bIsVirtualFunction, VirtualTableOffset);
    return MemberFunctionInfo{RealFunctionAddress, ThisAdjustment, bIsVirtualFunction, VirtualTableOffset};
}

/*HRESULT CoCreateDiaDataSource(HMODULE diaDllHandle, IDiaDataSource** data_source) {
    auto DllGetClassObject = (BOOL (WINAPI*)(REFCLSID, REFIID, LPVOID *)) GetProcAddress(diaDllHandle, "DllGetClassObject");
    if (!DllGetClassObject) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    IClassFactory* pClassFactory;
    HRESULT hr = DllGetClassObject(CLSID_DiaSource, IID_IClassFactory, reinterpret_cast<LPVOID *>(&pClassFactory));
    if (FAILED(hr)) {
        return hr;
    }
    hr = pClassFactory->CreateInstance(nullptr, IID_IDiaDataSource, (void **) data_source);
    if (FAILED(hr)) {
        return hr;
    }
    return S_OK;
}*/
