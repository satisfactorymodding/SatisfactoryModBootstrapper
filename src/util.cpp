#include <vector>
#include "util.h"
#include "logging.h"

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

std::string GetLastErrorAsString() {
    //Get the error message, if any.
    DWORD errorMessageID = GetLastError();
    if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, nullptr);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}