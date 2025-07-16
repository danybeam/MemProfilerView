module;

// Includes that don't play as nicely with the import mechanics
#include <flecs.h>;
#include <raylib.h>;
#include <unordered_map>
#include <format>

#pragma warning( push, 0 )
#include <SDL_render.h>
#include <Clay/clay.h>

#include <profiler.h>
#include <Constants.h>
#include <external/glad.h>

#pragma warning( pop )

export module ProfilingRenderer;

import <Font.h>;

import FilesModule;
import IOState;

export struct LoadedFonts
{
    /**
    * Store the loaded fonts in memory.
    * It has enough capacity for each font weight in both normal and italic variants. 
    */
    Font* fonts;
};

export namespace memProfileViewer
{
    /**
     * Declaration of the module to render the profiling results
     */
    class RenderingModule
    {
    public:
        /**
         * Constructor to be used with flecs import
         * @param world reference of the flecs world to register the module on
         */
        RenderingModule(const flecs::world& world);
    };

    /**
     * Flecs tag to request that the results are rendered.
     * It is added by default by the module but in theory it could be removed.
     */
    struct RenderResults
    {
    };
}

module :private;

// forward declarations
/**
 * Private callback for the flecs system to render the results of the file
 * @param it flecs iterator
 * @param file component holding the file data
 * @param ioState_component component holding the state and the target of the IO devices
 */
void renderFileResults(flecs::iter& it, size_t, memProfileViewer::File_Holder& file,
                       memProfileViewer::IOState_Component& ioState_component);
Texture createTimeBarTexture(Font& font);

/**
 * Temporary(?) cache of textures representing the bars depending on how long it was allocated for
 */
static std::unordered_map<double, std::vector<Texture>> cachedImages = {};
static double maxDuration = -99999;
static long long maxIndex = 0;
static Texture timeBar;

memProfileViewer::RenderingModule::RenderingModule(const flecs::world& world)
{
    if (fw::maxTextureSize < 0)
    {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &fw::maxTextureSize);
    }

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
    if (file.entries.empty())
    {
        return;
    }

    ProfileLock::RequestForceLock();

    Clay_ElementDeclaration t_baseFrame = {};
    t_baseFrame.id = CLAY_ID("BaseFrame");
    t_baseFrame.backgroundColor = {.r = 0, .g = 0, .b = 0, .a = 255};
    t_baseFrame.layout.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()};
    t_baseFrame.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;
    t_baseFrame.layout.padding = {
        .left = 8,
        .right = 8,
        .top = 8,
        .bottom = 8
    };

    Clay_ElementDeclaration t_addressHolder = {};
    t_addressHolder.id = CLAY_ID("AddressHolder");
    t_addressHolder.backgroundColor = {.r = 50, .g = 0, .b = 0, .a = 255};
    t_addressHolder.layout.sizing = {.width = CLAY_SIZING_FIT(20), .height = CLAY_SIZING_GROW(20)};
    t_addressHolder.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
    t_addressHolder.layout.childGap = 8;
    t_addressHolder.clip = {.horizontal = false, .vertical = true, .childOffset = {0, ioState_component.mouse_wheel.y}};

    Clay_ElementDeclaration t_entryBars = {};
    t_entryBars.id = CLAY_ID("EntryBars");
    t_entryBars.backgroundColor = {.r = 0, .g = 50, .b = 0, .a = 255};
    t_entryBars.layout.sizing = {.width = CLAY_SIZING_GROW(20), .height = CLAY_SIZING_GROW(20)};
    t_entryBars.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
    t_entryBars.layout.childGap = 8;
    t_entryBars.clip = {.horizontal = true, .vertical = true, .childOffset = ioState_component.mouse_wheel};
    
    // TODO(danybeam) make height constant in program 

    // TODO(danybeam) scrolling is very slow and choppy. Add tweening first see if it fixes the choppiness
    // TODO(danybeam) move image cache to component and attach to singleton 
    CLAY(t_baseFrame)
    {
        CLAY(t_addressHolder)
        {
            // Empty element to make space for the time bar
            CLAY({ .layout = { .sizing = { .width = CLAY_SIZING_FIXED(0), .height = CLAY_SIZING_FIXED(32)} } })
            {
            }

            for (memProfileViewer::Memory_TraceEntry& entry : file.entries)
            {
                Clay_String addressName = {};
                addressName.chars = entry.memLocation.c_str();
                addressName.length = entry.memLocation.length();
                addressName.isStaticallyAllocated = false;

                CLAY_TEXT(
                    addressName,
                    CLAY_TEXT_CONFIG({
                        .userData = nullptr,
                        .textColor = {255, 255, 0, 255},
                        .fontId = FONT_WEIGHT::FONT_REGULAR,
                        .fontSize = 32,
                        .letterSpacing = 0,
                        .lineHeight = 0,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                        })
                );
            }
        }

        CLAY(t_entryBars)
        {
            // TODO(danybeam) generate time bar for reference
            Clay_ElementDeclaration t_barHolder = {};
            // t_barHolder.layout.childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER};
            t_barHolder.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
            t_barHolder.layout.childGap = 0;
            t_barHolder.layout.padding = {.left = 0, .right = 0, .top = 0, .bottom = 0};
            t_barHolder.layout.sizing = {.width = CLAY_SIZING_GROW(20), .height = CLAY_SIZING_FIXED(32)};

            CLAY(t_barHolder)
            {
                // TODO(danybeam) keep track of longest duration and use it for timebar and for memory leaks
                for (auto& entry : file.entries)
                {
                    // TODO(danybeam) manage memory leaks/ duration == -1
                    if (entry.duration < 0)
                    {
                        continue;
                    }

                    Clay_ElementDeclaration t_entry = {};
                    t_entry.layout.padding = {.left = 0, .right = 0, .top = 0, .bottom = 0};
                    t_entry.layout.childGap = 0;
                    t_entry.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;

                    if (!cachedImages.contains(entry.duration))
                    {
                        cachedImages[entry.duration] = std::vector<Texture>{};
                        for (int i = 1; entry.duration > i * fw::maxTextureSize; i++)
                        {
                            // TODO(danybeam) change this to gradient to check that it's generating properly
                            // TODO(danybeam) this seems to be generating properly
                            auto image = GenImageColor(fw::maxTextureSize, 32, {
                                                           static_cast<unsigned char>(rand() % 255),
                                                           static_cast<unsigned char>(rand() % 255),
                                                           static_cast<unsigned char>(rand() % 255), 255
                                                       });
                            cachedImages[entry.duration].push_back(LoadTextureFromImage(image));
                            UnloadImage(image);
                        }

                        auto image = GenImageColor(static_cast<int>(entry.duration) % fw::maxTextureSize, 32,
                                                   {0, 255, 0, 255});
                        cachedImages[entry.duration].push_back(LoadTextureFromImage(image));
                        UnloadImage(image);
                    }

                    CLAY(t_entry)
                    {
                        for (auto& cached_image : cachedImages[entry.duration])
                        {
                            Clay_ElementDeclaration t_imageHolder = {};
                            t_imageHolder.image.imageData = &cached_image;
                            t_imageHolder.layout.sizing = {
                                .width = CLAY_SIZING_FIXED(static_cast<float>(cached_image.width)),
                                .height = CLAY_SIZING_FIXED(static_cast<float>(cached_image.height))
                            };

                            CLAY(t_imageHolder)
                            {
                            }
                        }
                    }
                }
            }
        }
    }

    ProfileLock::RequestForceUnlock();
}

Texture createTimeBarTexture(Font& font)
{
    Texture result = {};

    long long imageWidth = maxDuration;

    Image temp_image = GenImageColor(imageWidth, 32, {0, 0, 255, 255});

    for (long long i = 0; i < imageWidth; i += 200)
    {
        std::string timeMarker = std::format("| {}ms", i);
        ImageDrawTextEx(
            &temp_image,
            font,
            timeMarker.c_str(),
            {static_cast<float>(i), 0},
            32,
            0,
            {255, 255, 255, 255}
        );
    }

    result = LoadTextureFromImage(temp_image);
    UnloadImage(temp_image);
    return result;
}
