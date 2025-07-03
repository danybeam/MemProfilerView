module;

#include <utils/profiler.h>
uint8_t ProfileLock::semaphore_ = 1; // Necessary to define saveProfiling
bool ProfileLock::forceLock_ = false;

export module utils;
import <chrono>;

namespace utils
{
    export double getSystemTimeSinceGameStart()
    {
        static bool hasInitialized = false;
        static std::chrono::steady_clock::time_point startTime = {};

        if (!hasInitialized)
        {
            startTime = std::chrono::steady_clock::now();
            hasInitialized = true;
        }
        const std::chrono::duration<double> duration_in_seconds = std::chrono::steady_clock::now() - startTime;
        return duration_in_seconds.count();
    }
}
