#define MAX_TYPE_NAME 500
#include "naming_util.h"
#include "logging.h"

size_t levenshteinDistance(const std::string& s, const std::string& t) {
    size_t n = s.length();
    size_t m = t.length();
    ++n; ++m;
    size_t* d = new size_t[n * m];
    memset(d, 0, sizeof(size_t) * n * m);
    for (size_t i = 1, im = 0; i < m; ++i, ++im) {
        for (size_t j = 1, jn = 0; j < n; ++j, ++jn) {
            if (s[jn] == t[im]) {
                d[(i * n) + j] = d[((i - 1) * n) + (j - 1)];
            } else {
                d[(i * n) + j] = min(d[(i - 1) * n + j] + 1, /* A deletion. */
                                     min(d[i * n + (j - 1)] + 1, /* An insertion. */
                                         d[(i - 1) * n + (j - 1)] + 1)); /* A substitution. */
            }
        }
    }
    size_t r = d[n * m - 1];
    delete [] d;
    return r;
}
double computeStringSimilarity(const std::string& string, const std::string& other) {
    size_t sLen = string.length();
    size_t tLen = other.length();
    double longLen = max(sLen, tLen);
    return (longLen - levenshteinDistance(string, other)) / longLen;
}

std::string getFunctionName(const std::string& functionSignature) {
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
        return "@GENERIC_TYPE@"; //TODO proper handling of generic types
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
    }
    bool shouldAddReturnType = true;
    if (functionTypeInfo.MemberFunction) {
        std::string className = getTypeName(infoText, functionTypeInfo.ClassIndex);
        size_t namespaceIdx = className.find_last_of("::");
        //remove namespace from class name if it is there
        if (namespaceIdx != std::string::npos) {
            className = className.substr(namespaceIdx + 1);
        }
        std::string compareFunctionName = functionName;
        namespaceIdx = compareFunctionName.find_last_of("::");
        //remove class name from function name
        if (namespaceIdx != std::string::npos) {
            compareFunctionName = compareFunctionName.substr(namespaceIdx + 1);
        }
        //remove destructor prefix from function name
        if (compareFunctionName[0] == '~') {
            compareFunctionName = compareFunctionName.substr(1);
        }
        if (compareFunctionName == className) {
            //this is constructor or destructor, do not add return type
            shouldAddReturnType = false;
        }
    }
    //we shouldn't add return type to the constructors and destructors
    if (shouldAddReturnType) {
        resultString.append(getTypeName(infoText, functionTypeInfo.RetTypeIndex));
        resultString.append(" ");
    }
    resultString.append(functionName);
    resultString.append("(");
    bool hasTrailingComma = false;
    for (int i = 0; i < functionTypeInfo.NumArgs; i++) {
        resultString.append(getTypeName(infoText, functionTypeInfo.Args[i]));
        resultString.append(",");
        hasTrailingComma = true;
    }
    if (hasTrailingComma) {
        resultString.erase(resultString.length() - 1, 1);
    }
    resultString.append(")");
    //just to match handling in case of formatFunctionName
    replaceGenericShit(resultString);
    replaceAll(resultString, "@GENERIC_TYPE@,@GENERIC_TYPE@", "@GENERIC_TYPE@");
    replaceAll(resultString, "&", "*");
    return resultString;
}

void replaceGenericShit(std::string& functionName) {
    bool isFunctionSignature = functionName.find('(') != std::string::npos;
    if (!isFunctionSignature) {
        return; //code below works only for functions
    }
    uint64_t currentPos;
    while ((currentPos = functionName.find('?')) != std::string::npos ||
            (currentPos = functionName.find('<')) != std::string::npos) {
        auto startIndex = functionName.find_last_of(',', currentPos);
		if (startIndex == std::string::npos) startIndex = functionName.find_last_of('(', currentPos);
		if (startIndex == std::string::npos) startIndex = -1;
		startIndex++;
		auto endIndex = currentPos;
		int depth = 0;
		do {
			if (functionName[endIndex] == '<')
				depth++;
			if (functionName[endIndex] == '>')
				depth--;
			endIndex++;
		} while (depth != 0);
		endIndex--;
        //std::cout << functionName << " " << currentPos << " " << startIndex << " " << endIndex << std::endl;
        functionName.erase(startIndex, endIndex - startIndex + 1);
        functionName.insert(startIndex, "@GENERIC_TYPE@");
        //std::cout << functionName << " " << std::endl;
    }
}

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
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
    replaceAll(functionName, ")const", ")");
    replaceAll(functionName, "virtual ", ""); //virtual is meaningless in compiled code
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
    replaceAll(functionName, "?? :: ??&", "@GENERIC_TYPE@", functionName.find('('));
    replaceGenericShit(functionName);
    replaceAll(functionName, "@GENERIC_TYPE@,@GENERIC_TYPE@", "@GENERIC_TYPE@");
    //looks like PDB code doesn't retain whenever signature was reference or pointer, so it has only pointers
    //so we need to get rid of references in function signature and use pointers instead
    replaceAll(functionName, "&", "*");
    //cleanup spaces once again
    removeExcessiveSpaces(functionName);
    trim(functionName);
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
