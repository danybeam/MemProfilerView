export module utils;

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
