#ifndef XINPUT1_3_NAMING_UTIL_H
#define XINPUT1_3_NAMING_UTIL_H

#include <limits>
#include <string>
#include <CTypeInfoText.h>
#include <algorithm>

struct SymbolInfo {
    std::string Name;
    uint64_t Address;
    uint64_t TypeId;
};

std::string getFunctionName(std::string& functionSignature);

void replaceAll(std::string& input, const char* text, const char* replacement, uint64_t limit = -1);

void removeExcessiveSpaces(std::string& input);

std::string getTypeName(CTypeInfoText& infoText, uint64_t typeId);

std::string createFunctionName(SymbolInfo& symbolInfo, CTypeInfoText& infoText);

void replaceGenericShit(std::string& functionName);

void formatUndecoratedName(std::string& functionName);

std::string GetLastErrorAsString();

#endif //XINPUT1_3_NAMING_UTIL_H
