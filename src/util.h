#ifndef XINPUT1_3_UTIL_H
#define XINPUT1_3_UTIL_H

#include <string>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::wstring getModuleFileName(HMODULE moduleHandle);

std::string GetLastErrorAsString();

#endif //XINPUT1_3_UTIL_H
