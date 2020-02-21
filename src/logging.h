
#ifndef XINPUT1_3_LOGGING_H
#define XINPUT1_3_LOGGING_H

#include <fstream>

#define assert(expression, message)\
    if (!expression) {\
        Logging::logFile << "[FATAL] Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " << message << std::endl;\
        exit(1);\
    }

namespace Logging {
    extern std::ofstream logFile;

    void initializeLogging();
}

#endif //XINPUT1_3_LOGGING_H
