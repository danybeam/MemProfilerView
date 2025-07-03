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
export namespace memProfileViewer
{
    /**
     * flecs module declaration
     */
    class FilesModule
    {
    public:
        // ReSharper disable once CppNonExplicitConvertingConstructor
        FilesModule(const flecs::world& world);
    };

    enum class CATEGORY
    {
        NONE,
        DEALLOCATED,
        MEM_LEAK
    };

    struct Memory_TraceEntry
    {
        CATEGORY category = CATEGORY::NONE;
        double duration = -1.0; // In millisecondss in memory but microseconds in the file.
        std::string memLocation = "";
        unsigned long long threadId = 0;
        unsigned long long memSize = 0;
        std::vector<std::string> callstack = {};

        Memory_TraceEntry() = default;

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
void checkFileDropped(flecs::iter& it, size_t, memProfileViewer::File_Holder& file);

// Module implementations
memProfileViewer::FilesModule::FilesModule(const flecs::world& world)
{
    // Declare module
    world.module<memProfileViewer::FilesModule>("FilesModule");

    // Describe components
    world.component<memProfileViewer::File_Holder>("File_Holder")
         .member<std::string>("name")
         .member<std::ofstream>("file");

    // Add component to singleton

    world.add<memProfileViewer::File_Holder>();

    // define system
    world.system<memProfileViewer::File_Holder>()
         .term_at<memProfileViewer::File_Holder>(0).singleton()
         .kind(flecs::OnUpdate)
         .each(checkFileDropped);
}

void checkFileDropped(flecs::iter& it, size_t, memProfileViewer::File_Holder& file)
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
        memProfileViewer::CATEGORY category = traceEvents[i]["cat"].get<std::string>().find("Deallocated") !=
                                              std::string::npos
                                                  ? memProfileViewer::CATEGORY::DEALLOCATED
                                                  : memProfileViewer::CATEGORY::MEM_LEAK;


        file.entries.emplace_back(
            category,
            traceEvents[i]["dur(us)"].get<unsigned long long>() / 1000.0,
            traceEvents[i]["name"].get<std::string>(),
            traceEvents[i]["tid"].get<unsigned long long>(),
            traceEvents[i]["size"].get<unsigned long long>(),
            traceEvents[i]["callStack"].get<std::vector<std::string>>()
        );
    }
}
