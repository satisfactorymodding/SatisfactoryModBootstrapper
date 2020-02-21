#include "trampoline_gen.h"
#include "logging.h"
#include <fstream>
#include <Windows.h>
#include <utility>
#include <io.h>

std::string readTemplateFile(const path& filename) {
    std::ifstream stream(filename);
    if (!stream.good())
        return "";
    std::string str((std::istreambuf_iterator<char>(stream)),
                    std::istreambuf_iterator<char>());
    return str;
}

bool writeEndFile(const path& fileName, const std::string& textFile) {
    std::ofstream stream(fileName);
    if (!stream.good())
        return false;
    stream << textFile << std:: endl;
    return true;
}

void insertASMFunction(std::string& textFile, uint32_t& lastIdx, const std::string& functionName, uint64_t offset) {
    std::string generatedName;
    generatedName.append("generated_").append(std::to_string(lastIdx++));
    //generate function definition, append it onto the file
    size_t insertIndex = textFile.find("$$FUNCTION_INSERT$$");
    std::string resultText;
    resultText.append("proc ").append(generatedName).append("\n");
    resultText.append("  mov rax, [basePointer]\n");
    resultText.append("  add rax, ").append(std::to_string(offset)).append("\n");
    resultText.append("  ret\n");
    resultText.append("endp\n");
    textFile.insert(insertIndex, resultText);

    //generate export entry, append it too
    size_t exportInsertIndex = textFile.find("$$EXPORT_INSERT$$");
    std::string exportText;
    //continue previous import entry
    exportText.append(",\\\n      ");
    exportText.append(generatedName).append(",");
    exportText.append("\"").append(functionName).append("\"");
    textFile.insert(exportInsertIndex, exportText);
}

void removeAppendMarkers(std::string& textFile) {
    textFile.erase(textFile.find("$$FUNCTION_INSERT$$"), 19);
    textFile.erase(textFile.find("$$EXPORT_INSERT$$"), 17);
}

path createTempFileName() {
    DWORD bufferSize = GetTempPathW(0, nullptr);
    wchar_t stringBuffer[bufferSize + 32];
    GetTempPathW(bufferSize, stringBuffer);
    GetTempFileNameW(stringBuffer, L"asm", 0, stringBuffer);
    return path(stringBuffer);
}

FILE* setupProcessLogging(STARTUPINFOW& startupInfo, const path& outputFile) {
    FILE* fileHandle;
    errno_t error = fopen_s(&fileHandle, outputFile.generic_string().data(), "rw");
    if (error == 0) {
        Logging::logFile << "[DEBUG] Redirecting asm compiler output to " << outputFile.generic_string() << std::endl;
        auto* handle = reinterpret_cast<HANDLE *>(_get_osfhandle(_fileno(fileHandle)));
        startupInfo.hStdOutput = handle;
        startupInfo.hStdError = handle;
        return fileHandle;
    } else {
        Logging::logFile << "[WARNING] Failed to create file to redirect asm compiler info: " << error << std::endl;
        Logging::logFile << "[WARNING] Redirecting to stdout instead" << std::endl;
        startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        return nullptr;
    }
}

std::wstring createCompileCommand(const path& assemblerExePath, const path& srcFile, const path& outputPath) {
    std::wstring commandLine;
    commandLine.append(L"\"").append(assemblerExePath.generic_wstring()).append(L"\" ");
    commandLine.append(L"\"").append(srcFile.generic_wstring()).append(L"\" ");
    commandLine.append(L"\"").append(outputPath.generic_wstring()).append(L"\"");
    return commandLine;
}

path compileDLLFile(const path& assemblerExePath, const path& srcFile) {
    //because flat assembler resolves imports relative to current directory for some reason
    path processCurrentDir = assemblerExePath.parent_path() / "include";
    path outputLogFile = assemblerExePath.parent_path() / "asm-compiler-output.log";
    path outputFile = srcFile.parent_path() / srcFile.filename().generic_string().append(".dll");
    PROCESS_INFORMATION processInfo;
    STARTUPINFOW startupinfo{};
    std::wstring commandLine = createCompileCommand(assemblerExePath, srcFile, outputFile);
    FILE* logFileHandle = setupProcessLogging(startupinfo, outputLogFile);
    bool result = CreateProcessW(nullptr, commandLine.data(),
                                 nullptr,nullptr,
                                 false, 0,
                                 nullptr, processCurrentDir.generic_wstring().c_str(),
                                 &startupinfo, &processInfo);
    if (!result) {
        fclose(logFileHandle);
        Logging::logFile << "[FATAL] Failed to create ASM compiler process" << std::endl;
        exit(1);
    }
    Logging::logFile << "[INFO] Waiting for compiler..." << std::endl;
    DWORD processExitCode;
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    GetExitCodeProcess(processInfo.hProcess, &processExitCode);
    fclose(logFileHandle);
    if (processExitCode != 0) {
        Logging::logFile << "[FATAL] Failed to compile trampoline asm code" << std::endl;
        Logging::logFile << "[FATAL] Compiler exited with code " << processExitCode << std::endl;
        Logging::logFile << "[FATAL] See " << outputLogFile.generic_string() << " for information" << std::endl;
        exit(1);
    }
    Logging::logFile << "[INFO] Successfully compiled trampoline code" << std::endl;
    return outputFile;
}

GeneratorEnv::GeneratorEnv(path asmPath, path tempPath) : asmExecutablePath(std::move(asmPath)), templateFilePath(std::move(tempPath)) {}

path getSelfModuleDirectory() {
    HMODULE selfModule = GetModuleHandleA(nullptr);
    DWORD bufferSize = GetModuleFileNameW(selfModule, nullptr, 0);
    wchar_t nameBuffer[bufferSize + 32];
    GetModuleFileNameW(selfModule, nameBuffer, bufferSize);
    return path(nameBuffer).parent_path();
}

class GeneratorEnv GeneratorEnv::createDefault() {
    path selfModuleDir = getSelfModuleDirectory().parent_path();
    path assemblerPath(selfModuleDir / "fasm-redist" / "fasm.exe");
    path templatePath = selfModuleDir / L"trampoline_template.asm";
    return GeneratorEnv(assemblerPath, templatePath);
}

TrampolineGen::TrampolineGen(GeneratorEnv env) : generatorEnv(std::move(env)), insertIndex(0) {}

void TrampolineGen::setup() {
    textFile = readTemplateFile(generatorEnv.templateFilePath);
    insertIndex = 0;
}

void TrampolineGen::addFunction(const std::string& name, uint64_t offset) {
    assert(!textFile.empty(), "[ERR] Attempt to call addFunction on uninitialized TrampolineGen");
    insertASMFunction(textFile, insertIndex, name, offset);
}

path TrampolineGen::compileDLL() {
    path temporaryFile = createTempFileName();
    writeEndFile(temporaryFile, textFile);
    return compileDLLFile(generatorEnv.asmExecutablePath, temporaryFile);
}