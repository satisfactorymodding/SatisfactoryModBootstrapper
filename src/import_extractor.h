#ifndef XINPUT1_3_IMPORT_EXTRACTOR_H
#define XINPUT1_3_IMPORT_EXTRACTOR_H

#include <filesystem>
#include <unordered_map>
#include <vector>
using path = std::filesystem::path;

bool extractImportTable(const path& executablePath, std::unordered_map<std::string, std::vector<std::string>>& resultMap);

#endif //XINPUT1_3_IMPORT_EXTRACTOR_H
