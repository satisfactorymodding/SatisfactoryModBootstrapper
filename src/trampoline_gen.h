#ifndef XINPUT1_3_TRAMPOLINE_GEN_H
#define XINPUT1_3_TRAMPOLINE_GEN_H

#include <filesystem>
using path = std::filesystem::path;

class GeneratorEnv {
public:
    const path asmExecutablePath;
    const path templateFilePath;
public:
    GeneratorEnv(path asmPath, path tempPath);
    static GeneratorEnv createDefault();
};

class TrampolineGen {
private:
    const GeneratorEnv generatorEnv;
    std::string textFile;
    uint32_t insertIndex;
public:
    explicit TrampolineGen(GeneratorEnv env);

    void setup();
    void addFunction(const std::string& name, uint64_t offset);
    path compileDLL();
};

#endif //XINPUT1_3_TRAMPOLINE_GEN_H
