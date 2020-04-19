#include "DestructorGenerator.h"
#include <iostream>
#include <functional>
#include <comdef.h>
#include "logging.h"
using namespace asmjit::x86;

#define CHECK_FAILED(hr, message) \
    if (FAILED(hr)) { \
        _com_error err(hr); \
        LPCTSTR errMsg = err.ErrorMessage(); \
        std::cerr << message << errMsg << std::endl; \
        exit(1); \
    }

#define CHECK(expr) { HRESULT hr = expr; CHECK_FAILED(hr, #expr); }
#define GET(Type, Name, Call) Type Name; CHECK(Call(&Name));
#define CALL_GET(Type, Name, Call, ...) Type Name; CHECK(Call(__VA_ARGS__, &Name));
#define DUMP_GENERATED_CODE 1

CComPtr<IDiaSymbol> FindFirstSymbol(const CComPtr<IDiaEnumSymbols>& EnumSymbols) {
    CComPtr<IDiaSymbol> FirstSymbol;
    LONG SymbolCount = 0L;
    EnumSymbols->get_Count(&SymbolCount);
    if (SymbolCount > 0) {
        CHECK(EnumSymbols->Item(0, &FirstSymbol));
    }
    return FirstSymbol;
}

void ForEachSymbol(const CComPtr<IDiaEnumSymbols>& EnumSymbols, const std::function<void(const CComPtr<IDiaSymbol>& Symbol)>& Body) {
    LONG SymbolCount = 0;
    EnumSymbols->get_Count(&SymbolCount);
    for (LONG i = 0; i < SymbolCount; i++) {
        CComPtr<IDiaSymbol> SymbolPtr;
        CHECK(EnumSymbols->Item(i, &SymbolPtr));
        Body(SymbolPtr);
    }
}

/**
 * Iterates UDT function symbols and returns first destructor symbol matching
 */
CComPtr<IDiaSymbol> FindFirstDestructorSymbol(const CComPtr<IDiaSymbol>& UDTSymbol) {
    CALL_GET(CComPtr<IDiaEnumSymbols>, Functions, UDTSymbol->findChildren, SymTagFunction, nullptr, nsNone);
    CComPtr<IDiaSymbol> ResultSymbolPtr;
    ForEachSymbol(Functions, [&ResultSymbolPtr](const CComPtr<IDiaSymbol>& FunctionSymbol) {
        GET(BSTR, UndecoratedName, FunctionSymbol->get_undecoratedName);
        if (wcschr(UndecoratedName, '~')) {
            ResultSymbolPtr = FunctionSymbol;
        }
    });
    return ResultSymbolPtr;
}

typedef void (*DestructorFunctionPtr)(void*);
DestructorFunctionPtr FindOrGenerateDestructorFunction(HMODULE BasePtr, const CComPtr<IDiaSymbol>& ClassSymbol);

bool CanGenerateDestructorFor(const CComPtr<IDiaSymbol>& Symbol) {
    GET(DWORD, SymbolTag, Symbol->get_symTag);
    if (SymbolTag == SymTagArrayType) {
        GET(CComPtr<IDiaSymbol>, ElementType, Symbol->get_type);
        return CanGenerateDestructorFor(ElementType);
    }
    return SymbolTag == SymTagUDT;
}

/**
 * Computes stack space required to generate destructor for given variable
 */
uint64_t ComputeLocalVariableStackSpace(const CComPtr<IDiaSymbol>& Symbol) {
    GET(DWORD, SymbolTag, Symbol->get_symTag);
    if (SymbolTag == SymTagArrayType) {
        GET(CComPtr<IDiaSymbol>, ElementType, Symbol->get_type);
        return ComputeLocalVariableStackSpace(ElementType) + 16;
    }
    return 0;
}

void DestructorGenerator::GenerateDestructorCall(const CComPtr<IDiaSymbol>& Symbol, asmjit::x86::Builder& a, uint64_t StackOffset) {
    GET(DWORD, SymbolTag, Symbol->get_symTag);
    if (SymbolTag == SymTagArrayType) {
        GET(DWORD, ElementCount, Symbol->get_count);
        GET(ULONGLONG, ArraySizeInBytes, Symbol->get_length);
        GET(CComPtr<IDiaSymbol>, ElementType, Symbol->get_type);
        GET(DWORD, ElementTypeSymTag, ElementType->get_symTag);
        ULONGLONG ElementSize = ArraySizeInBytes / ElementCount;

        //Initialize locations to store results
        Mem LoopCounterLocation = qword_ptr(rsp, StackOffset);
        Mem ThisPtrLocation = ptr(rsp, StackOffset + 8);

        //Set loop counter = ElementCounter, this = rcx;
        a.mov(LoopCounterLocation, asmjit::imm(ElementCount));
        a.mov(ThisPtrLocation, rcx);
        asmjit::Label LoopBeginLabel = a.newLabel();
        a.bind(LoopBeginLabel);
        //Remove 1 from loop counter, set rcx = this + ElementSize * LoopCounter;
        a.sub(LoopCounterLocation, 1);
        a.mov(rcx, ThisPtrLocation);
        a.mov(rax, LoopCounterLocation);
        a.imul(rax, asmjit::imm(ElementSize));
        a.add(rcx, rax);
        GenerateDestructorCall(ElementType, a, StackOffset + 16);
        a.cmp(LoopCounterLocation, 0);
        a.jne(LoopBeginLabel);

    } else if (SymbolTag == SymTagUDT) {
        //For UDT, we just need to find correct constructor symbol and call it
        //Or we could need to generate it too, so if we can't find correct symbol, we will try to use fallback
        CComPtr<IDiaSymbol> DestructorSymbol = FindFirstDestructorSymbol(Symbol);
        DWORD64 ResultCallAddress;
        GET(BSTR, ClassName, Symbol->get_name);
        if (DestructorSymbol) {
            //Destructor is already generated, just call it directly
            GET(DWORD, RelativeVirtualAddress, DestructorSymbol->get_relativeVirtualAddress);
            ResultCallAddress = reinterpret_cast<uintptr_t>(dllBaseAddress) + RelativeVirtualAddress;
        } else {
            //Constructor is not generated yet, print info and generate it
            DestructorFunctionPtr GeneratedPtr = FindOrGenerateDestructorFunction(Symbol);
            ResultCallAddress = reinterpret_cast<uint64_t>(GeneratedPtr);
        }
        std::wcout << "Destructing UDT symbol " << ClassName << " with destructor symbol at " << ResultCallAddress << std::endl;
        //Call result function now, with this placed in rcx
        a.call(ResultCallAddress);
    } else {
        std::cout << "[WARN] Unsupported symbol tag for destructor call: " << SymbolTag << std::endl;
    }
}

DestructorFunctionPtr DestructorGenerator::FindOrGenerateDestructorFunction(const CComPtr<IDiaSymbol>& ClassSymbol) {
    GET(BSTR, ClassName, ClassSymbol->get_name);
    std::wstring ClassNameString(ClassName);
    const auto iterator = GeneratedDestructorsMap.find(ClassNameString);
    if (iterator != GeneratedDestructorsMap.end()) {
        return iterator->second;
    }
    DestructorFunctionPtr GeneratedPtr = GenerateDestructorFunction(ClassSymbol);
    GeneratedDestructorsMap.insert({ClassNameString, GeneratedPtr});
    return GeneratedPtr;
}

uint64_t ComputeStackSpaceRequired(IDiaEnumSymbols* ClassVariables) {
    uint64_t StackSpaceRequired = 32;
    ForEachSymbol(ClassVariables, [&StackSpaceRequired](const CComPtr<IDiaSymbol>& MemberVar) {
        GET(DWORD, DataKind, MemberVar->get_dataKind);
        if (DataKind == DataIsMember) {
            GET(CComPtr<IDiaSymbol>, VariableType, MemberVar->get_type);
            if (CanGenerateDestructorFor(VariableType)) {
                StackSpaceRequired += ComputeLocalVariableStackSpace(VariableType);
            }
        }
    });
    return StackSpaceRequired;
}

DestructorFunctionPtr DestructorGenerator::GenerateDestructorFunction(const CComPtr<IDiaSymbol>& ClassSymbol) {
    CALL_GET(CComPtr<IDiaEnumSymbols>, ClassVariables, ClassSymbol->findChildren, SymTagData, nullptr, nsNone);
    CALL_GET(CComPtr<IDiaEnumSymbols>, ParentBaseClasses, ClassSymbol->findChildren, SymTagBaseClass, nullptr, nsNone);
    asmjit::CodeHolder code;
    code.init(runtime.codeInfo());
    asmjit::x86::Builder a(&code);
    //function prolog
    a.mov(ptr(rsp, 8), rcx);
    //ComputeStackSpaceRequired will always return multiple of 16, but x64 calling convention requires
    //stack to be 16 bytes-aligned, but `call` will push return address on stack (8 bytes), so we need extra 8 bytes to keep alignment
    uint64_t StackSpaceRequired = ComputeStackSpaceRequired(ClassVariables) + 8;
    a.sub(rsp, StackSpaceRequired);

    //Call field destructors now
    ForEachSymbol(ClassVariables, [&](const CComPtr<IDiaSymbol>& MemberVar) {
        GET(DWORD, DataKind, MemberVar->get_dataKind);
        if (DataKind == DataIsMember) {
            GET(CComPtr<IDiaSymbol>, VariableType, MemberVar->get_type);
            if (CanGenerateDestructorFor(VariableType)) {
                GET(LONG, FieldOffset, MemberVar->get_offset);
                //Move this ptr to rcx, add required offset
                a.mov(rcx, ptr(rsp, StackSpaceRequired + 8));
                a.add(rcx, asmjit::imm(FieldOffset));
                //rsp now points to the end of stack reservation (lowest point, but we reserve 32 bytes for function calls)
                GenerateDestructorCall(VariableType, a, 32);
                GET(BSTR, FieldName, MemberVar->get_name);
            }
        }
    });

    //Call base class destructors now
    ForEachSymbol(ParentBaseClasses, [&](const CComPtr<IDiaSymbol>& BaseClass) {
        GET(CComPtr<IDiaSymbol>, BaseClassUDT, BaseClass->get_type);
        GET(LONG, ClassOffset, BaseClass->get_offset);
        if (BaseClassUDT && CanGenerateDestructorFor(BaseClassUDT)) {
            //Move this ptr to rcx, add required offset
            a.mov(rcx, ptr(rsp, StackSpaceRequired + 8));
            a.add(rcx, asmjit::imm(ClassOffset));
            //rsp now points to the end of stack reservation (lowest point, but we reserve 32 bytes for function calls)
            GenerateDestructorCall(BaseClassUDT, a, 32);
        }
    });

    //function epilogue
    a.add(rsp, StackSpaceRequired);
    a.ret();
#if DUMP_GENERATED_CODE
    Logging::logFile << "-------GENERATED DESTRUCTOR CODE BEGIN-------" << std::endl;
    asmjit::String StrBuilder;
    a.dump(StrBuilder, 0);
    Logging::logFile << StrBuilder.data() << std::endl;
    Logging::logFile << "--------GENERATED DESTRUCTOR CODE END--------" << std::endl;
#endif
    a.finalize();
    DestructorFunctionPtr ResultFunction;
    runtime.add(&ResultFunction, &code);
    return ResultFunction;
}

DestructorFunctionPtr DestructorGenerator::GenerateDestructor(const std::string& ClassName) {
    USES_CONVERSION;
    CALL_GET(CComPtr<IDiaEnumSymbols>, FoundSymbols, globalSymbol->findChildren, SymTagUDT, A2COLE(ClassName.c_str()), nsCaseInsensitive);
    CComPtr<IDiaSymbol> FirstUDTSymbol = FindFirstSymbol(FoundSymbols);
    if (!FirstUDTSymbol) {
        return nullptr;
    }
    return FindOrGenerateDestructorFunction(FirstUDTSymbol);
}
