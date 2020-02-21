#include "mod_locator.h"
#include <libloaderapi.h>
#include "logging.h"

#define MOD_DIRECTORIES std::vector<std::string> {"mods", "loaders"};
static path* gameRootDir = nullptr;


path resolveGameRootDir() {
    wchar_t pathBuffer[2048]; //just to be sure it will always fit
    GetModuleFileNameW(GetModuleHandleA(GAME_MODULE_NAME), pathBuffer, 2048);
    std::filesystem::path rootDirPath{pathBuffer};
    std::string gameFolderName(GAME_MODULE_NAME);
    gameFolderName.erase(gameFolderName.find('-'));
    //we go up the directory tree until we find the folder called
    //FactoryGame, which denotes the root of the game directory
    while (!std::filesystem::exists(rootDirPath / gameFolderName)) {
        rootDirPath = rootDirPath.parent_path();
    }
    Logging::logFile << "[INFO] Game Root Directory: " << rootDirPath.generic_string() << std::endl;
    return rootDirPath;
}

void initializeModLocator() {
    gameRootDir = new path(resolveGameRootDir());
}

const path& getGameRootDirectory() {
    assert(gameRootDir, "Attempt to call getGameRootDirectory before mod locator is initialized!");
    return *gameRootDir;
}

std::vector<path> findModFiles() {
    const path& gameDirectory = getGameRootDirectory();
    std::vector<std::string> modDirectories = MOD_DIRECTORIES;


}