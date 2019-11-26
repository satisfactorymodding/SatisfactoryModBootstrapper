#define MAX_TYPE_NAME 500

#include "naming_util.h"

std::string getFunctionName(std::string& functionSignature) {
    uint64_t pos = functionSignature.find('(');
    uint64_t startPos = functionSignature.find_last_of(' ', pos) + 1;
    return functionSignature.substr(startPos, pos - startPos);
}

void replaceAll(std::string& input, const char* text, const char* replacement, uint64_t limit) {
    unsigned long long int pos;
    while ((pos = input.find(text)) != -1 && (limit == -1 || pos < limit)) {
        input.erase(pos, strlen(text));
        input.insert(pos, replacement);
    }
}

void removeExcessiveSpaces(std::string& input) {
    bool prev_is_space = true;
    input.erase(std::remove_if(input.begin(), input.end(), [&prev_is_space](unsigned char curr) {
        bool r = isspace(curr) && prev_is_space;
        prev_is_space = isspace(curr);
        return r;

    }), input.end());
}

std::string getTypeName(CTypeInfoText& infoText, uint64_t typeId) {
    char symbolNameBuffer[MAX_TYPE_NAME];
    infoText.GetTypeName(static_cast<uint32_t>(typeId), nullptr, symbolNameBuffer, MAX_TYPE_NAME);
    std::string resultString(symbolNameBuffer);
    if (resultString.find('<') != std::string::npos) {
        return "<GENERIC_TYPE>"; //TODO proper handling of generic types
    }
    replaceAll(resultString, "NEAR_C", "");
    removeExcessiveSpaces(resultString);
    return resultString;
}

std::string createFunctionName(SymbolInfo& symbolInfo, CTypeInfoText& infoText) {
    FunctionTypeInfo functionTypeInfo{};
    infoText.DumpObj()->DumpFunctionType(static_cast<uint32_t>(symbolInfo.TypeId), functionTypeInfo);
    std::string& functionName = symbolInfo.Name;
    std::string resultString;
    if (functionTypeInfo.StaticFunction) {
        resultString.append("static ");
    } else if (functionTypeInfo.MemberFunction) {
        resultString.append("virtual ");
    }
    resultString.append(getTypeName(infoText, functionTypeInfo.RetTypeIndex));
    resultString.append(" ");
    resultString.append(functionName);
    resultString.append("(");

    for (int i = 0; i < functionTypeInfo.NumArgs; i++) {
        resultString.append(getTypeName(infoText, functionTypeInfo.Args[i]));
        resultString.append(",");
    }
    resultString.erase(resultString.length() - 1, 1);
    resultString.append(")");
    //just to match handling in case of formatFunctionName
    replaceAll(resultString, "<GENERIC_TYPE>,<GENERIC_TYPE>", "<GENERIC_TYPE>");
    return resultString;
}

void replaceGenericShit(std::string& functionName) {
    uint64_t currentPos;
    while ((currentPos = functionName.find('?')) != std::string::npos) {
        auto startIndex = functionName.find_last_of(',', currentPos);
        if (startIndex == std::string::npos) startIndex = functionName.find('(');
        auto endIndex = functionName.find(',', currentPos);
        if (endIndex == std::string::npos) endIndex = functionName.find_last_of(')');
        //std::cout << functionName << " " << currentPos << " " << startIndex << " " << endIndex << std::endl;
        functionName.erase(startIndex + 1, endIndex - startIndex - 1);
        functionName.insert(startIndex + 1, "<GENERIC_TYPE>");
        //std::cout << functionName << " " << std::endl;
    }
}

void formatUndecoratedName(std::string& functionName) {
    removeExcessiveSpaces(functionName);
    replaceAll(functionName, "__int64", "int"); //replace int sizes
    replaceAll(functionName, "__ptr64", ""); //replace pointer markers
    replaceAll(functionName, "(void)", "()"); //replace (void) with empty arguments
    //access modifiers
    replaceAll(functionName, "public: ", "");
    replaceAll(functionName, "protected: ", "");
    replaceAll(functionName, "private: ", "");
    replaceAll(functionName, "const", ""); //const is meaningless in compiled code
    //remove function modifiers
    replaceAll(functionName, ")const", ")");
    //remove calling convention from functions
    replaceAll(functionName, "__cdecl", ""); //(default C/C++ calling convention)
    //remove type identifiers
    replaceAll(functionName, "class ", "");
    replaceAll(functionName, "union  ", "");
    replaceAll(functionName, "struct ", "");
    replaceAll(functionName, "enum ", "");
    //remove excessive spaces between type modifiers
    replaceAll(functionName, " *", "*");
    replaceAll(functionName, " &", "&");
    replaceAll(functionName, " )", ")");
    replaceAll(functionName, " ,", ",");
    replaceAll(functionName, "(*)()", "()*");
    //generic shit TODO find a nicer way of handling it
    replaceAll(functionName, "?? :: ??&", "<GENERIC_TYPE>", functionName.find('('));
    replaceGenericShit(functionName);
    replaceAll(functionName, "<GENERIC_TYPE>,<GENERIC_TYPE>", "<GENERIC_TYPE>");
    removeExcessiveSpaces(functionName);
}

std::string GetLastErrorAsString() {
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}
