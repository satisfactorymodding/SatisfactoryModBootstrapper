#ifndef XINPUT1_3_DESTRUCTORGENERATOR_H
#define XINPUT1_3_DESTRUCTORGENERATOR_H

#define WIN32_LEAN_AND_MEAN
#define ASMJIT_STATIC 1
#include "asmjit.h"
#include <string>
#include <unordered_map>
#include <atlbase.h>
#include <dia2.h>

typedef void (*DestructorFunctionPtr)(void*);
typedef void (*DummyFunctionPtr)();
typedef void (*DummyFunctionCallHandler)(const char* FunctionName);

class DestructorGenerator {
private:
    CComPtr<IDiaSymbol> globalSymbol;
    LPVOID dllBaseAddress;
    std::unordered_map<std::wstring, DestructorFunctionPtr> GeneratedDestructorsMap;
    std::unordered_map<std::string, DummyFunctionPtr> GeneratedDummyFunctionsMap;
    asmjit::JitRuntime runtime;
    std::vector<char*> ConstantPoolEntries;
public:
    DestructorGenerator(LPVOID gameDllBase, CComPtr<IDiaSymbol> globalSymbol) :
        globalSymbol(std::move(globalSymbol)),
        dllBaseAddress(gameDllBase) {}
    ~DestructorGenerator();

    DestructorFunctionPtr GenerateDestructor(const std::string& ClassName);
    DummyFunctionPtr GenerateDummyFunction(const std::string& FunctionName, DummyFunctionCallHandler CallHandler);
private:
    /**
    * Generates destructor call for the given symbol
    * RCX should be set to the this pointer of the object being destructor
    * @param BasePtr pointer to the location in memory where executable is loaded
    * @param StackOffset offset on the rsp to the beginning of the first variable
    */
    void GenerateDestructorCall(const CComPtr<IDiaSymbol>& Symbol, asmjit::x86::Builder& a, uint64_t StackOffset);
    DestructorFunctionPtr FindOrGenerateDestructorFunction(const CComPtr<IDiaSymbol>& ClassSymbol);
    DestructorFunctionPtr GenerateDestructorFunction(const CComPtr<IDiaSymbol>& ClassSymbol);
    DummyFunctionPtr DoGenerateDummyFunction(const std::string& FunctionName, DummyFunctionCallHandler CallHandler);
};


#endif //XINPUT1_3_DESTRUCTORGENERATOR_H
