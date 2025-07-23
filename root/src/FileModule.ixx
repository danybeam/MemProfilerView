// ReSharper disable CppExpressionWithoutSideEffects
module;

// Flecs doesn't work properly unless included like this
#include <flecs.h>;

export module FilesModule;

// System headers
import <fstream>;
import <string>;
import <vector>;

// Lib and internal headers
import <raylib.h>;
import <nlohmann/json.hpp>;

using json = nlohmann::json;

/**
 * Namespace for the project
 */
export namespace mem_profile_viewer
{
    /**
     * flecs module declaration
     */
    class FilesModule
    {
    public:
        // ReSharper disable once CppNonExplicitConvertingConstructor
        /**
         * Constructor for use with flecs import
         * @param world reference to the world where the module is to be registered
         */
        FilesModule(const flecs::world& world);
    };

    /**
     * Enum for categories of memory allocations
     */
    enum class CATEGORY
    {
        UNKNOWN,
        DEALLOCATED,
        MEM_LEAK
    };

    /**
     * Data structure to hold the data of memory trace entries.
     */
    struct Memory_TraceEntry
    {
        CATEGORY category = CATEGORY::UNKNOWN; /**< The category of the data entry */
        double duration = -1.0;
        /**< How long the memory was allocated for. @remark In milliseconds in memory but microseconds in the file.*/
        std::string memLocation = ""; /**< memory address of what was allocated */
        unsigned long long threadId = 0; /**< ID of the thread that caused the allocation */
        unsigned long long memSize = 0; /**< How much memory was allocated */
        std::vector<std::string> callstack = {}; /**< The callstack at the moment of the allocation. */

        /**
         * default constructor for array creation/vector reservations
         */
        Memory_TraceEntry() = default;

        /**
         * Constructor to construct the entries (mostly) in place
         * @param category The category of the allocation
         * @param duration how long was the memory allocated
         * @param memLocation Where the memory was allocated
         * @param threadId the ID the memory was allocated at
         * @param memSize how much memory was allocated
         * @param callstack The callstack at the moment of the allocation
         */
        Memory_TraceEntry(CATEGORY category, double duration, const std::string& memLocation,
                          unsigned long long threadId,
                          unsigned long long memSize, const std::vector<std::string>& callstack) :
            category(category),
            duration(duration),
            memLocation(memLocation),
            threadId(threadId),
            memSize(memSize),
            callstack(callstack)
        {
        }

        bool operator==(const Memory_TraceEntry& other) const
        {
            bool result = true;

            result &= (&this->memLocation == &other.memLocation);
            result &= (&this->duration == &other.duration);
            result &= (&this->memSize == &other.memSize);
            result &= (&this->threadId == &other.threadId);
            result &= (&this->category == &other.category);
            result &= (&this->callstack == &other.callstack);

            return result;
        }

        bool operator!=(const Memory_TraceEntry& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * Component to hold the file dropped into the window.
     */
    struct File_Holder
    {
        std::string name; /**< Name of the file */
        std::ifstream file; /**< handler of the file stream */
        std::vector<Memory_TraceEntry> entries; /**< Entries in the file */

        /**
         * Destructor to ensure file is closed properly
         */
        ~File_Holder()
        {
            if (file.is_open())
                file.close();
        }
    };
}

module :private;
// private forward declarations
/**
 * private callback for flecs system to check whether a new file got dropped into the window and load the data.
 * @param it flecs iterator
 * @param file component holding the file information
 */
void checkFileDropped(flecs::iter& it, size_t, mem_profile_viewer::File_Holder& file);

// Module implementations
mem_profile_viewer::FilesModule::FilesModule(const flecs::world& world)
{
    // Declare module
    world.module<mem_profile_viewer::FilesModule>("FilesModule");

    // Describe components
    world.component<mem_profile_viewer::File_Holder>("File_Holder")
         .member<std::string>("name")
         .member<std::ofstream>("file");

    // Add component to singleton

    world.add<mem_profile_viewer::File_Holder>();

    // define system
    world.system<mem_profile_viewer::File_Holder>()
         .term_at<mem_profile_viewer::File_Holder>(0).singleton()
         .kind(flecs::OnUpdate)
         .each(checkFileDropped);
}

void checkFileDropped(flecs::iter& it, size_t, mem_profile_viewer::File_Holder& file)
{
    if (!IsFileDropped())
    {
        return;
    }

    auto filePaths = LoadDroppedFiles();
    if (filePaths.count == 0)
    {
        return; // If no files exit early
    }

    std::string filepath = filePaths.paths[0];

    if (filepath == file.name)
    {
        return; // already processed
    }
    
    if (file.file.is_open())
    {
        file.file.close();
    }

    file.name = filepath;
    file.file.open(filepath);
    auto json = json::parse(file.file);
    auto traceEvents = json["traceEvents"];

    if (!file.entries.empty())
    {
        file.entries.clear();
    }

    for (size_t i = 0; i < traceEvents.size(); ++i)
    {
        // (CATEGORY category, double duration, std::string& memLocation,
        // unsigned long long threadId, unsigned long long memSize, int vectorSize)
        mem_profile_viewer::CATEGORY category = traceEvents[i]["cat"].get<std::string>().find("Deallocated") !=
                                                std::string::npos
                                                    ? mem_profile_viewer::CATEGORY::DEALLOCATED
                                                    : mem_profile_viewer::CATEGORY::MEM_LEAK;


        file.entries.emplace_back(
            category,
            traceEvents[i]["dur(us)"].get<long long>() / 1000.0,
            traceEvents[i]["name"].get<std::string>(),
            traceEvents[i]["tid"].get<unsigned long long>(),
            traceEvents[i]["size"].get<unsigned long long>(),
            traceEvents[i]["callStack"].get<std::vector<std::string>>()
        );
    }

    UnloadDroppedFiles(filePaths);
}
