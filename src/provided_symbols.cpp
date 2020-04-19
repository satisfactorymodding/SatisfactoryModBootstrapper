#include "provided_symbols.h"
#include "logging.h"

static DestructorGenerator* destructorGenerator;
static SymbolResolver* symbolResolver;

void hookRequiredSymbols(SymbolResolver& provider) {
    //hook symbols required to implement provided symbols here and store them in static variables
    symbolResolver = &provider;
    destructorGenerator = new DestructorGenerator(symbolResolver->dllBaseAddress, symbolResolver->globalSymbol);
}

//??0FString@@QEAA@XZ - FString* FString::FString(void)
//just zeroes out string instance and returns this
void* FStringAllocateEmpty(void* thisPtr) {
    *(uint64_t*)thisPtr = 0;
    *((uint64_t*)thisPtr + 1) = 0;
    return thisPtr;
}

//??1FChatMessageStruct@@QEAA@XZ
//??1UObject@@UEAA@XZ
void* DummyDestructorCall(void* thisPtr) {
    //TODO: Determine object type, and call destructors on other fields (virtual methods still won't be supported)
    return thisPtr;
}

void* DummyFVTableConstructor() {
    Logging::logFile << "[FATAL] FVTableHelper& constructor called. This should never happen in Shipping!" << std::endl;
    exit(1);
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

bool startsWith(const char *str, const char *pre) {
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

int endsWith(const char *str, const char *suffix) {
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

//??1UObject@@UEAA@XZ
//??1FJsonValue@@MEAA@XZ
std::string GetClassNameFromDestructorMangledName(const char* mangledName) {
    std::string ClassName(mangledName);
    ClassName.erase(0, 3);
    ClassName.erase(ClassName.find('@'));
    return ClassName;
}

//?Z_Construct_UClass_AFGSubsystem_NoRegister@@YAPEAVUClass@@XZ
//?GetPrivateStaticClass@UComponentDelegateBinding@@CAPEAVUClass@@XZ
//?StaticClass@AFGSubsystem_NoRegister@@SAPEAVUClass@@XZ
void* remapZConstructUClassNoRegister(const char* mangledName) {
    std::string newSymbolName(mangledName);
    newSymbolName.erase(0, strlen("?Z_Construct_UClass_"));
    size_t noRegisterIdx = newSymbolName.find("_NoRegister@@");
    newSymbolName.erase(noRegisterIdx);
    newSymbolName.insert(0, "?GetPrivateStaticClass@");
    newSymbolName.append("@@CAPEAVUClass@@XZ");
    return symbolResolver->ResolveSymbol(newSymbolName.c_str());
}

//?GetPrivateStaticClass@UComponentDelegateBinding@@CAPEAVUClass@@XZ
//?StaticClass@UWidgetAnimationDelegateBinding@@SAPEAVUClass@@XZ
void* remapPrivateStaticClass(const char* mangledName) {
    std::string newSymbolName(mangledName);
    newSymbolName.erase(0, strlen("?GetPrivateStaticClass@"));
    newSymbolName.erase(newSymbolName.find('@'));
    newSymbolName.insert(0, "?StaticClass@");
    newSymbolName.append("@@SAPEAVUClass@@XZ");
    return symbolResolver->ResolveSymbol(newSymbolName.c_str());
}

void* provideSymbolImplementation(const char* mangledName) {
    //string constructors
    if (strcmp(mangledName, "??0FString@@QEAA@XZ") == 0) {
        return reinterpret_cast<void*>(&FStringAllocateEmpty);
    }
    //Sometimes StaticClass() exists but GetPrivateStaticClass() doesn't, in that case we fallback to it
    if (startsWith(mangledName, "?GetPrivateStaticClass@")) {
        return remapPrivateStaticClass(mangledName);
    }
    //And sometimes Z_Construct_UClass_XXX_NoRegister is missing. In that case we fallback to GetPrivateStaticClass
    if (startsWith(mangledName, "?Z_Construct_UClass_")) {
        return remapZConstructUClassNoRegister(mangledName);
    }
    //Missing destructor. Generate default implementation and print warning
    if (startsWith(mangledName, "??1")) {
        std::string ClassName = GetClassNameFromDestructorMangledName(mangledName);
        Logging::logFile << "Providing default implementation for destructor of class: " << ClassName << std::endl;
        Logging::logFile << "Warning! It can result in memory leaks if object is not freed up properly." << std::endl;
        DestructorFunctionPtr ResultPtr = destructorGenerator->GenerateDestructor(ClassName);
        if (ResultPtr == nullptr) {
            Logging::logFile << "[ERROR] Failed to generate destructor for class: Class Not Found in PDB: " << ClassName << std::endl;
            return nullptr;
        }
        return reinterpret_cast<void*>(ResultPtr);
    }
    //Dummy FVTableHelper& constructors. Should never get called
    if (endsWith(mangledName, "@@QEAA@AEAVFVTableHelper@@@Z")) {
        return reinterpret_cast<void*>(&DummyFVTableConstructor);
    }
    //StaticConfigName for UObject
    CHECK_RETURN_STATIC_CONFIG_NAME(AActor);
    CHECK_RETURN_STATIC_CONFIG_NAME(UActorComponent);
    CHECK_RETURN_STATIC_CONFIG_NAME(UObject);
    //other stuff goes here
    return nullptr;
}