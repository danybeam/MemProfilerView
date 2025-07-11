module;

export module utils;
import <chrono>;

namespace utils
{
    /**
     * Utility function to get the time the program has been running
     * @return How long the program has been running in seconds
     */
    export double getSystemTimeSinceProgramStart()
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
