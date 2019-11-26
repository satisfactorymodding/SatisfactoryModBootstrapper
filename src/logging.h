
#ifndef XINPUT1_3_LOGGING_H
#define XINPUT1_3_LOGGING_H

#include <fstream>

namespace Logging {
    extern std::ofstream logFile;

    void initializeLogging();
}

#endif //XINPUT1_3_LOGGING_H
