#include "provided_symbols.h"

void hookRequiredSymbols(SymbolResolver& provider) {
    //hook symbols required to implement provided symbols here and store them in static variables
}

//??0FString@@QEAA@XZ - FString* FString::FString(void)
//just zeroes out string instance and returns this
void* FStringAllocateEmpty(void* thisPtr) {
    *(uint64_t*)thisPtr = 0;
    *((uint64_t*)thisPtr + 1) = 0;
    return thisPtr;
}

//Static config names - macro defined for easier use
#define DEFINE_STATIC_CONFIG_NAME(ClassName) \
const wchar_t* ClassName() { return L#ClassName; }
#define CHECK_RETURN_STATIC_CONFIG_NAME(ClassName) \
if (strcmp(mangledName, "?StaticConfigName@"#ClassName"@@SAPEB_WXZ") == 0) { return reinterpret_cast<void*>(StaticConfigName::ClassName); }

namespace StaticConfigName {
    //add more as necessary. Technically, all UObjects should have one
    DEFINE_STATIC_CONFIG_NAME(AActor)
    DEFINE_STATIC_CONFIG_NAME(UActorComponent)
    DEFINE_STATIC_CONFIG_NAME(UObject)
}

void* provideSymbolImplementation(const char* mangledName) {
    //string constructors
    if (strcmp(mangledName, "??0FString@@QEAA@XZ") == 0) {
        return reinterpret_cast<void*>(&FStringAllocateEmpty);
    }
    //StaticConfigName for UObject
    CHECK_RETURN_STATIC_CONFIG_NAME(AActor);
    CHECK_RETURN_STATIC_CONFIG_NAME(UActorComponent);
    CHECK_RETURN_STATIC_CONFIG_NAME(UObject);
    //other stuff goes here
    return nullptr;
}

