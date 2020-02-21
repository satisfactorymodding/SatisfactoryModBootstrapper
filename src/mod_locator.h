#ifndef XINPUT1_3_MOD_LOCATOR_H
#define XINPUT1_3_MOD_LOCATOR_H

#include <vector>
#include <filesystem>

#define GAME_MODULE_NAME "FactoryGame-Win64-Shipping.exe"

using path = std::filesystem::path;

void initializeModLocator();

const path& getGameRootDirectory();

std::vector<path> findModFiles();

#endif //XINPUT1_3_MOD_LOCATOR_H
