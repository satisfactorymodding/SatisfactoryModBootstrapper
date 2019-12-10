#include "logging.h"

namespace Logging {
    std::ofstream logFile;

    void initializeLogging() {
        logFile.open("pre-launch-debug.log", std::ifstream::trunc | std::ifstream::out);
        logFile << "Log System Initialized!" << std::endl;
    }
}
