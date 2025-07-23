//
// Basic instrumentation profiler by Cherno expanded by danybeam
// Video link: https://www.youtube.com/watch?v=xlAH4dbMVnU&list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb&index=81

// Usage: include this header file somewhere in your code (e.g. precompiled header), and then use like:
//
// Instrumentor::Get().BeginSession("Session Name");        // Begin session 
// {
//     InstrumentationTimer timer("Profiled Scope Name");   // Place code like this in scopes you'd like to include in profiling
//     // Code
// }
// Instrumentor::Get().EndSession();                        // End Session
//
// You will probably want to macro-fy this, to switch on/off easily and use things like __FUNCSIG__ for the profile name.
//
// ReSharper disable CppParameterMayBeConstPtrOrRef
// ReSharper disable CppClangTidyClangDiagnosticNewDelete
// ReSharper disable CppParameterNamesMismatch
// ReSharper disable CppClangTidyCppcoreguidelinesSpecialMemberFunctions
#pragma once
#pragma warning(push)
#pragma warning( disable : 4595)

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stacktrace>
#include <string>
#include <thread>
#include <unordered_map>


/*
 * Known issues:
 * - The profiling macro needs to be the first thing in the scope to make sure it gets freed last.
 *     - IDK if there's any way around that
 * - This has not been tested in multithreaded apps.
 *     - Even if it could work out of the box IDK what settings should be used. 
 */

/**
 * Mutex-like object to avoid infinite recursion when calling new or delete
 */
struct ProfileLock // NOLINT(cppcoreguidelines-special-member-functions)
{
    /**
     * ProfileLock constructor, this takes ownership of the lock automatically
     */
    ProfileLock()
    {
        selfPointer_ = this;
        // std::cout << "Locking profiler " << selfPointer_ << "\n";
        semaphore_--;
    }

    /**
     * ProfileLock destructor, this releases ownership of the lock automatically
     */
    ~ProfileLock()
    {
        // std::cout << "Unlocking profiler " << selfPointer_ << "\n";
        semaphore_++;
    }

    /**
     * Get the status of forceLock
     * @return forceLock_ Whether the lock is being forced or not
     */
    static bool GetForceLock()
    {
        return forceLock_;
    }

    /**
     * Try to activate the force lock
     * @todo actually 'try' instead of just forcing it.I would need to have some metric of when the lock is absolutely necessary.
     * This might not be necessary until/unless this "library"(header) becomes multithread safe
     */
    static void RequestForceLock()
    {
        forceLock_ = true;
    }

    /**
     * Try to deactivate the force lock
     * @todo actually 'try' instead of just forcing it. I would need to have some metric of when the lock is absolutely necessary.
     * This might not be necessary until/unless this "library"(header) becomes multithread safe
     */
    static void RequestForceUnlock()
    {
        forceLock_ = false;
    }

    /**
     * Get the status of the semaphore value
     * @remark Mostly for debugging purposes
     * @return The current status of saveProfiling
     */
    static uint8_t GetSaveProfiling()
    {
        return semaphore_;
    }

private:
    void* selfPointer_; /**< does nothing. Without this the destructor gets called at weird times. */

    static uint8_t semaphore_;
    /**< Semaphore counter for the lock. Should be defined as one, more than one would work, but it would just get consumed when creating the stack trace. @remark This should change if this becomes multithreaded. IDK how it would behave*/
    static bool forceLock_; /**< Bool to control whether to override the status of the lock and force it.*/
};

/**
 * Struct to store the result of a timer profiling
 */
struct ProfileResult_Time
{
    std::string name; /**< The name of what is being profiled. */
    uint32_t threadId; /**< The thread of the function call being measured. */
    long long start, end; /**< Time stamp of the profiling */
};

/**
 * Struct to store the result of a memory profiling
 */
struct ProfileResult_Memory
{
    bool isArray; /**< Whether the memory allocation was for an array or not. */
    void* location; /**< Pointer to the location that memory is being allocated to. */
    size_t size; /**< How much memory was allocated. */
    std::stacktrace stackTrace; /**< The stack trace of where the memory was allocated in code. */
    long long start, end = -1; /**< Time stamp of the profiling */
};

/**
 * Struct related to the instrumentation session. Right now it only stores the name of the session.
 */
struct InstrumentationSession
{
    std::string name;
};

/**
 * Class to manage profiling sessions. It only tracks one concurrent session at a time.
 */
class Instrumentor
{
private:
    class InstrumentationMemory* m_currentMemoryCheck_ = nullptr; /**< Ref pointer to the current memory profiler. */
    InstrumentationSession* m_currentSession_; /**< The current instrumentation session going on. */
    std::ofstream m_outputStream_; /**< handler of the file to write the results into. */
    int m_profileCount_mem_; /**< Counter of how many entries have been in the memory profiling */
    int m_profileCount_time_; /**< Counter of how many entries have been in the time profiling */

    /**
     * Construct the instrumentor with default values.
     * Made private to prevent having multiple instances.
     */
    Instrumentor():
        m_currentSession_(nullptr),
        m_profileCount_mem_(0),
        m_profileCount_time_(0)
    {
    }

public:
    /**
     * Start a profiling session.
     * @param name Name of the session
     * @param filepath Path to save the session info into.
     */
    void BeginSession(const std::string& name, const std::string& filepath = "results.json")
    {
        if (m_currentSession_)
        {
            throw
                "There is already a profiling session running. Make sure you're not calling START_SESSION(name) more than once";
        }

        m_outputStream_.open(filepath);
        WriteHeader();
        m_currentSession_ = new InstrumentationSession{name};
    }

    /**
     * End the current profiling session.
     * @throws error Errors if trying to end a session before it starts/
     */
    void EndSession()
    {
        if (!m_currentSession_)
        {
            throw "Closing a session was requested even though there's no sessions running.";
        }

        WriteFooter();
        m_outputStream_.close();
        delete m_currentSession_;
        m_currentSession_ = nullptr;
        m_profileCount_time_ = 0;
    }

    /**
     * Write the profiling data of a timer profiling into the file.
     * @param profilingData The data of the timer profiling result
     */
    void WriteProfile(const ProfileResult_Time& profilingData)
    {
        if (m_profileCount_time_++ > 0)
            m_outputStream_ << ",";

        std::string name = profilingData.name;
        std::ranges::replace(name, '"', '\'');

        m_outputStream_ << "{";
        m_outputStream_ << "\"cat\":\"function\",";
        m_outputStream_ << "\"dur\":" << (profilingData.end - profilingData.start) << ',';
        m_outputStream_ << "\"name\":\"" << name << "\",";
        m_outputStream_ << "\"ph\":\"X\",";
        m_outputStream_ << "\"pid\":0,";
        m_outputStream_ << "\"tid\":" << profilingData.threadId << ",";
        m_outputStream_ << "\"ts\":" << profilingData.start;
        m_outputStream_ << "}";

        m_outputStream_.flush();
    }

    /**
     * Write the profiling data of a memory profiling session into the file.
     * @param profilingData The data of the memory profiling result
     */
    void WriteProfile(const ProfileResult_Memory& profilingData)
    {
        if (m_currentMemoryCheck_ == nullptr)
        {
            throw "A memory instrumentor tried to write before registering";
        }
        if (m_profileCount_mem_++ > 0)
            m_outputStream_ << ",";

        const uint32_t threadId = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

        m_outputStream_ << "{";
        m_outputStream_ << "\"cat\":\"" << ((profilingData.end >= 0) ? "Deallocated mem" : "Memory leaked") << "\",";
        m_outputStream_ << "\"dur(us)\":" << ((profilingData.end >= 0) ? (profilingData.end - profilingData.start) : -1)
            << ',';
        m_outputStream_ << "\"name\":\"" << profilingData.location << "\",";
        m_outputStream_ << "\"tid\":" << threadId << ",";
        m_outputStream_ << "\"tStart\":" << profilingData.start << ",";
        m_outputStream_ << "\"tEnd\":" << profilingData.end << ",";
        m_outputStream_ << "\"size\":" << profilingData.size << ",";
        m_outputStream_ << "\"callStack\":[";
        for (size_t i = 0; i < profilingData.stackTrace.size(); i++)
        {
            std::string stackTraceString = std::to_string(profilingData.stackTrace[i]);
            std::ranges::replace(stackTraceString, '\\', '/');

            m_outputStream_ << "\"";
            m_outputStream_ << stackTraceString;
            m_outputStream_ << "\"";
            if (i < profilingData.stackTrace.size() - 1)
            {
                m_outputStream_ << ",";
            }
        }
        m_outputStream_ << "]";
        m_outputStream_ << "}";

        m_outputStream_.flush();
    }

    /**
     * Write the results file header.
     */
    void WriteHeader()
    {
        m_outputStream_ << "{\"otherData\": {},\"traceEvents\":[";
        m_outputStream_.flush();
    }

    /**
     * Write the results file footer.
     */
    void WriteFooter()
    {
        m_outputStream_ << "]}";
        m_outputStream_.flush();
    }

    /**
     * Get the singleton instance
     * @return A reference to the Instrumentor singleton instance
     */
    static Instrumentor& Get()
    {
        static Instrumentor instance;
        return instance;
    }

    /**
     * Register a memory profiler to be the active one.
     * @param instrumentation Pointer to the active memory profiler
     */
    static void RegisterInstrumentation(InstrumentationMemory* instrumentation)
    {
        if (Instrumentor::Get().m_currentMemoryCheck_)
        {
            throw "An instrumentation was already registered";
        }

        Instrumentor::Get().m_currentMemoryCheck_ = instrumentation;
    }

    /**
     * Get the current memory instrumentation.
     * @return Pointer to the current memory profiler. nullptr if none have been registered.
     */
    static InstrumentationMemory* GetCurrentMemoryInstrumentation()
    {
        return Instrumentor::Get().m_currentMemoryCheck_;
    }
};

/**
 * A class to manage a timer in a scope automatically.
 */
class InstrumentationTimer final
{
public:
    /**
     * Create and start a timer with a given name.
     * @param name Name of the timer
     */
    explicit InstrumentationTimer(const char* name)
        : m_name_(name), m_stopped_(false)
    {
        m_startTimepoint_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * Destroy and stop a timer.
     */
    ~InstrumentationTimer()
    {
        if (!m_stopped_)
            Stop();
    }

    /**
     * Stop a timer manually. This is not necessary under normal conditions.
     */
    void Stop()
    {
        const auto endTimepoint = std::chrono::high_resolution_clock::now();

        const long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint_).
                                time_since_epoch().
                                count();
        const long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().
            count();

        const uint32_t threadId = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        Instrumentor::Get().WriteProfile({m_name_, threadId, start, end});

        m_stopped_ = true;
    }

private:
    /**
     * Name of the timer
     */
    const char* m_name_;
    /**
     * time point where the timer started.
     */
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimepoint_;
    /**
     * Whether the timer is stoped.
     * It should stay false during the lifetime of the object under normal conditions.
     */
    bool m_stopped_;
};

/**
 * A class to manage memory profiling in a scope automatically.
 * @remark It should be the first thing created in a stack to ensure that it gets deleted last.
 */
class InstrumentationMemory final
{
public:
    /**
     * Create and start a memory profiling object with a given name.
     * @param name Name of the memory profiler
     */
    explicit InstrumentationMemory(const char* name)
        : m_stopped_(false)
    {
        Instrumentor::RegisterInstrumentation(this);
    }

    /**
     * Stop and destroy a memory profiling object.
     */
    ~InstrumentationMemory()
    {
        if (!m_stopped_)
            Stop();
    }

    /**
     * Stop a profiler manually. Under normal conditions it should not be necessary.
     */
    void Stop()
    {
        ProfileLock lock;
        for (auto& profileResult : this->m_results_ | std::views::values)
        {
            Instrumentor::Get().WriteProfile(profileResult);
        }

        // std::cout << "Profiling stopped\n";
        m_stopped_ = true;
    }

    /**
     * Register when memory gets allocated.
     * @param address Memory address of the memory being allocated
     * @param size size of the memory being allocated
     * @param isArray boolean indicating whether the memory was allocated for an array
     */
    void Register_push(void* address, const size_t size, const bool isArray)
    {
        if (m_stopped_) return;

        m_results_[address] = {
            .isArray = isArray,
            .location = address,
            .size = size,
            .stackTrace = std::stacktrace::current(),
            .start = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).
                     time_since_epoch().
                     count()
        };
    }


    /**
     * Register memory deallocation.
     * @param address The address of the memory being deallocated.
     */
    void Register_pop(void* address)
    {
        if (m_stopped_) return;

        // This used to explode after closing the window, but it doesn't anymore.
        // I(danybeam) cannot get it to reproduce anymore. If someone can  please fill up an issue in the repo.
        if (const auto findResult = m_results_.find(address); findResult != m_results_.end())
        {
            findResult->second.end = std::chrono::time_point_cast<std::chrono::microseconds>(
                                         std::chrono::high_resolution_clock::now()).
                                     time_since_epoch().
                                     count();
        }
    }

private:
    /**
     * Name of the memory profiler.
     */
    /**
     * map to track the memory being allocated, deallocated and leaked.
     */
    std::unordered_map<void*, ProfileResult_Memory> m_results_;
    /**
     * Whether the profiler is stopped. It should be false during the normal operation of the profiler.
     */
    bool m_stopped_;
};

/*
 * These functions have been left uncommented somewhat on purpose.
 * Here's the official documentation for the new operators. https://en.cppreference.com/w/cpp/memory/new/operator_new
 * Here's the official documentation for the delete operators. https://en.cppreference.com/w/cpp/memory/new/operator_delete.html
 *
 * These functions try to do the same things but injecting the profiling tools into them.
 */
// ReSharper disable once CppInconsistentNaming
inline void* operator new(size_t size)
{
    if (size == 0)
    {
        size++;
    }

    void* ptr = std::malloc(size);

    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }

    if (ProfileLock::GetSaveProfiling() && !ProfileLock::GetForceLock())
    {
        ProfileLock lock;
        if (const auto memoryInstrumentation = Instrumentor::GetCurrentMemoryInstrumentation())
            // Because there's no guarantee there will already be an instrumentor active
            memoryInstrumentation->Register_push(ptr, size, false);
        // std::cout << "Allocated memory for variable at " << size << " at " << ptr << "\n";
    }
    return ptr;
}

// ReSharper disable once CppInconsistentNaming
inline void* operator new[](size_t size)
{
    if (size == 0)
    {
        size++;
    }

    void* ptr = std::malloc(size);

    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }

    if (ProfileLock::GetSaveProfiling() && !ProfileLock::GetForceLock())
    {
        ProfileLock lock;
        if (const auto memoryInstrumentation = Instrumentor::GetCurrentMemoryInstrumentation())
            // Because there's no guarantee there will already be an instrumentor active
            memoryInstrumentation->Register_push(ptr, size, true);
        // std::cout << "Allocated memory for array at " << size << " at " << ptr << "\n";
    }
    return ptr;
}

// ReSharper disable once CppInconsistentNaming
inline void* operator new(const size_t size, const std::nothrow_t& tag) noexcept
{
    void* ptr = std::malloc(size);

    if (ProfileLock::GetSaveProfiling() && !ProfileLock::GetForceLock())
    {
        ProfileLock lock;
        if (const auto memoryInstrumentation = Instrumentor::GetCurrentMemoryInstrumentation())
            // Because there's no guarantee there will already be an instrumentor active
            memoryInstrumentation->Register_push(ptr, ptr ? size : 0, false);
        // std::cout << "Allocated memory for variable noexcept at " << size << " at " << ptr << "\n";
    }
    return ptr;
}

// ReSharper disable once CppInconsistentNaming
inline void* operator new[](const size_t size, const std::nothrow_t& tag) noexcept
{
    void* ptr = std::malloc(size);

    if (ProfileLock::GetSaveProfiling() && !ProfileLock::GetForceLock())
    {
        ProfileLock lock;
        if (const auto memoryInstrumentation = Instrumentor::GetCurrentMemoryInstrumentation())
            // Because there's no guarantee there will already be an instrumentor active
            memoryInstrumentation->Register_push(ptr, ptr ? size : 0, true);
        // std::cout << "Allocated memory for array noexcept at " << size << " at " << ptr << "\n";
    }
    return ptr;
}

// ReSharper disable once CppInconsistentNaming
inline void operator delete(void* block) noexcept
{
    if (block == nullptr)
    {
        return;
    }

    if (ProfileLock::GetSaveProfiling() && !ProfileLock::GetForceLock())
    {
        ProfileLock lock;
        if (const auto memoryInstrumentation = Instrumentor::GetCurrentMemoryInstrumentation())
            memoryInstrumentation->Register_pop(block);
        // std::cout << "deleted variable at " << block << "\n";
    }

    std::free(block);
    block = nullptr;
}

// ReSharper disable once CppInconsistentNaming
inline void operator delete[](void* block) noexcept
{
    if (block == nullptr)
    {
        return;
    }

    if (ProfileLock::GetSaveProfiling() && !ProfileLock::GetForceLock())
    {
        ProfileLock lock;
        if (const auto memoryInstrumentation = Instrumentor::GetCurrentMemoryInstrumentation())
            memoryInstrumentation->Register_pop(block);
        // std::cout << "deleted array at " << block << "\n";
    }

    std::free(block);
    block = nullptr;
}

// ReSharper disable once CppInconsistentNaming
inline void operator delete(void* block, const std::nothrow_t& tag) noexcept
{
    if (block == nullptr)
    {
        return;
    }

    if (ProfileLock::GetSaveProfiling() && !ProfileLock::GetForceLock())
    {
        ProfileLock lock;
        if (const auto memoryInstrumentation = Instrumentor::GetCurrentMemoryInstrumentation())
            memoryInstrumentation->Register_pop(block);
        // std::cout << "deleted variable noexcept at " << block << "\n";
    }

    std::free(block);
    block = nullptr;
}

// ReSharper disable once CppInconsistentNaming
inline void operator delete[](void* block, const std::nothrow_t& tag) noexcept
{
    if (block == nullptr)
    {
        return;
    }

    if (ProfileLock::GetSaveProfiling() && !ProfileLock::GetForceLock())
    {
        ProfileLock lock;
        if (const auto memoryInstrumentation = Instrumentor::GetCurrentMemoryInstrumentation())
            memoryInstrumentation->Register_pop(block);
        // std::cout << "deleted array noexcept at " << block << "\n";
    }

    std::free(block);
    block = nullptr;
}

/*
* These preprocessors are used to simplify the creation of the profiler objects.
 */
// The "if" preprocessor command and the macro commands are a mix of Cherno and danybeam (me)
// Regardless of the copyright notice on modified versions of the code in the code the section bellow should be considered under the MIT license.
#pragma once
#define PROFILE_SCOPE_MEMORY(name) ProfileLock::RequestForceLock();InstrumentationMemory memoryProfiler##__LINE__##(name);ProfileLock::RequestForceUnlock();
#define PROFILE_SCOPE_TIME(name) InstrumentationTimer timer##__LINE__##(name)
#define PROFILE_FUNCTION_TIME() PROFILE_SCOPE(__FUNCSIG__)
#define START_SESSION(name)  Instrumentor::Get().BeginSession(name)
#define END_SESSION()  Instrumentor::Get().EndSession()

#pragma warning(pop)
