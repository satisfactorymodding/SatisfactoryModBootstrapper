#include "import_extractor.h"
#include "pelib/PeFile.h"
#include "logging.h"

using namespace PeLib;
#define PRINT_ERROR(msg) Logging::logFile << "[WARNING] Failed to extract import table from executable file " << executablePath.generic_string() << msg << #msg << std::endl;

bool extractImportTable(const path& executablePath, std::unordered_map<std::string, std::vector<std::string>>& resultMap) {
    PeFile* peFile = openPeFile(executablePath.generic_string());
    if (peFile == nullptr) {
        PRINT_ERROR("PE file is corrupted");
        return false;
    }
    if (peFile->getBits() != 64) {
        PRINT_ERROR("Only 64-bit PE files are supported");
        return false;
    }
    auto* peFile64 = dynamic_cast<PeFile64*>(peFile);
    if (peFile64->readImportDirectory() != ERROR_NONE) {
        PRINT_ERROR("Failed reading import directory");
        return false;
    }
    const ImportDirectory64& importDir = peFile64->impDir();
    dword numberOfFiles = importDir.getNumberOfFiles(OLDDIR);
    for (dword i = 0; i < numberOfFiles; i++) {
        std::string fileName = importDir.getFileName(i, OLDDIR);
        std::vector<std::string>& functionList = resultMap[fileName];
        dword numberOfFunctions = importDir.getNumberOfFunctions(i, OLDDIR);
        for (dword j = 0; j < numberOfFunctions; j++) {
            std::string function = importDir.getFunctionName(i, j, OLDDIR);
            functionList.push_back(function);
        }
    }
    return true;
}

