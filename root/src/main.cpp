#include <FWCore.h>
#include <flecs.h>

#include <utils/profiler.h>

import FilesModule;
import ProfilingRenderer;

int main()
{
    START_SESSION("TestMemoryCheckSession");
    // Double scoped for profiling potential memory leak
    {
        PROFILE_SCOPE_MEMORY("TestMemoryCheck");

        // Initialize framework
        auto framework = fw::FWCore(1280, 720);

        switch (framework.Get_errorCodes())
        {
        case fw::FWCore::ERRORCODES::CLAY_INIT_ERROR:
            throw std::runtime_error(
                "Something went wrong initializing Clay. I currently don't have detailed errors so please take note of what happened and share it with me.");
        case fw::FWCore::ERRORCODES::SDL_INIT_ERROR:
            throw std::runtime_error(
                "Something went wrong initializing SDL. I currently don't have detailed errors so please take note of what happened and share it with me.");
        default:
        case fw::FWCore::ERRORCODES::NONE:
            break;
        }

        // Set up flecs world
        auto world = std::make_unique<flecs::world>();
        // Optional, gather statistics for explorer
        world->import<flecs::stats>();
        // Creates REST server on default port (27750)
        world->set<flecs::Rest>({});

        world->import<memProfileViewer::FilesModule>();
        world->import<memProfileViewer::RenderingModule>();
        world->import<memProfileViewer::IOStateModule>();

        // Start program
        framework.Run(std::move(world));
    }
    END_SESSION();
    return 0;
}
