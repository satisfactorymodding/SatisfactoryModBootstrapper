#include "util.h"

std::wstring getModuleFileName(HMODULE moduleHandle) {
    std::wstring resultBuffer(MAX_PATH, '\0');
    size_t resultCopied;
    while (true) {
        resultCopied = GetModuleFileNameW(moduleHandle, resultBuffer.data(), resultBuffer.length() + 1);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            resultBuffer.append(resultBuffer.length(), '\0');
        } else {
            resultBuffer.erase(resultCopied);
            break;
        }
    }
    return resultBuffer;
}