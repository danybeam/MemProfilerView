module;
#include <flecs.h>;
#include "utils/profiler.h"
#include <raylib.h>;

#pragma warning( push, 0 )
#include <Clay/clay.h>
#pragma warning( pop )

export module ProfilingRenderer;

import FilesModule;
import IOState;

import <Font.h>;

export namespace memProfileViewer
{
    class RenderingModule
    {
    public:
        RenderingModule(const flecs::world& world);
    };

    struct RenderResults
    {
    };
}

module :private;

// forward declarations
void renderFileResults(flecs::iter& it, size_t, memProfileViewer::File_Holder& file,
                       memProfileViewer::IOState_Component& ioState_component);
static std::unordered_map<double, Texture> cachedImages;

memProfileViewer::RenderingModule::RenderingModule(const flecs::world& world)
{
    // Define the module
    // ReSharper disable once CppExpressionWithoutSideEffects
    world.module<RenderingModule>("Rendering Module");

    // Add tag to world
    world.add<RenderResults>();

    // Declare system
    world.system<File_Holder, IOState_Component>("Render loaded file")
         .with<RenderResults>()
         .term_at(0).singleton()
         .term_at(1).singleton()
         .each(renderFileResults);
}

void renderFileResults(flecs::iter& it, size_t, memProfileViewer::File_Holder& file,
                       memProfileViewer::IOState_Component& ioState_component)
{
    //if (file.entries.empty())
    //{
    //    return;
    //}

    ProfileLock::RequestForceLock();

    Clay_ElementDeclaration t_baseFrame = {};
    t_baseFrame.id = CLAY_ID("BaseFrame");
    t_baseFrame.backgroundColor = {.r = 0, .g = 0, .b = 0, .a = 255};
    t_baseFrame.layout.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)};
    t_baseFrame.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
    t_baseFrame.clip = {.horizontal = true, .vertical = true, .childOffset = ioState_component.mouse_wheel};

    Clay_String text = {};
    text.chars = file.name.c_str();
    text.length = file.name.length();

    // TODO(danybeam) scrolling is very slow and choppy. Add tweening first see if it fixes the choppiness
    CLAY(t_baseFrame)
    {
        unsigned char blue = 0;
        for (auto& entry : file.entries)
        {
            if (auto memoryBar = cachedImages.find(entry.duration); memoryBar == cachedImages.end())
            {
                //Image generatedImage = GenImageColor(entry.duration, 20, {230, 41, 50, 255});
                Image generatedImage = GenImageGradientLinear(entry.duration, 20, 90, {230, 41, 50, 255},
                                                              {0, 41, 255, 255});
                Texture texture = LoadTextureFromImage(generatedImage);
                cachedImages[entry.duration] = texture;
                UnloadImage(generatedImage);
            }

            Texture& imageRef = cachedImages[entry.duration];
            Clay_ElementDeclaration t_bar = {};
            t_bar.id = CLAY_IDI("Bar", blue++);
            t_bar.layout.sizing = {
                .width = CLAY_SIZING_FIXED(static_cast<float>(imageRef.width)),
                .height = CLAY_SIZING_FIXED(static_cast<float>(imageRef.height))
            };
            t_bar.image.imageData = &imageRef;

            CLAY(t_bar)
            {
            }
        }
    }

    ProfileLock::RequestForceUnlock();
}
