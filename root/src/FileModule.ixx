// ReSharper disable CppExpressionWithoutSideEffects
module;

// Flecs doesn't work properly unless included like this
#include <flecs.h>;

export module FilesModule;

// System headers
import <fstream>;
import <string>;

// Lib and internal headers
import <raylib.h>;

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

    /**
     * Component to hold the file dropped into the window.
     */
    struct File_Holder
    {
        std::string name; /**< Name of the file */
        std::ofstream file; /**< handler of the file stream */

        /**
         * Destructor to ensure file is closed properly
         */
        ~File_Holder()
        {
            if (file.is_open())
                file.close();
        }
    };

    struct Memory_TraceEntry
    {
        enum class CATEGORY
        {
            DEALLOCATED,
            MEM_LEAK
        };

        CATEGORY category;
        double duration; // In millisecondss in memory but microseconds in the file.
        std::string         
    };

    /*{
        "cat": "Deallocated mem",
        "dur(ms)": 3494907,
        "name": "000001CBFC1425D0",
        "tid": 3318535762,
        "tStart": 525613139467,
        "tEnd": 525616634374,
        "size": 8,
        "callStack": []
    }*/
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
}
