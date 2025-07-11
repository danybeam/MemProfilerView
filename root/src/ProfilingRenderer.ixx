module;

// Includes that don't play as nicely with the import mechanics
#include <flecs.h>;
#include <raylib.h>;
#include <unordered_map>
#include <format>

#pragma warning( push, 0 )
#include <Clay/clay.h>

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
static std::unordered_map<double, Texture> cachedImages = {};
static double maxDuration = -99999;
static Texture timeBar;

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
    if (file.entries.empty())
    {
        return;
    }

    //ProfileLock::RequestForceLock();

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
    t_addressHolder.layout.sizing = {.width = CLAY_SIZING_GROW(20), .height = CLAY_SIZING_GROW(20)};
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

    /*Clay_ElementDeclaration t_EntryFrame = {};
    t_EntryFrame.id = CLAY_ID("Entry");
    t_EntryFrame.backgroundColor = {.r = 0, .g = 0, .b = 0, .a = 255};
    t_EntryFrame.layout.sizing = {.width = CLAY_SIZING_GROW(), .height = 20};
    // TODO(danybeam) make height constant in program
    t_EntryFrame.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;
    t_EntryFrame.layout.childGap = 8;*/

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

            for (auto& entry : file.entries)
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
            //Clay_SetDebugModeEnabled(true);
            double testIndex = 0;

            // TODO(danybeam) generate time bar for reference
            // Empty element while I make the time bar
            Clay_ElementDeclaration t_timeBarConfig = {};
            t_timeBarConfig.id = CLAY_ID("TimeBarConfig");
            t_timeBarConfig.layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(static_cast<float>(timeBar.width)),
                    .height = CLAY_SIZING_FIXED(static_cast<float>(timeBar.height)),
                },
            };
            t_timeBarConfig.image.imageData = &timeBar;

            CLAY(t_timeBarConfig)
            {
            }

            for (auto& entry : file.entries)
            {
                Clay_ElementDeclaration test = {};

                if (!cachedImages.contains(testIndex))
                {
                    Image testImage = GenImageGradientLinear(200, 32, 90, {0, 255, 0, 255}, {255, 0, 0, 255});
                    cachedImages.insert_or_assign(testIndex++, LoadTextureFromImage(testImage));
                    UnloadImage(testImage);
                }

                test.image.imageData = &cachedImages[testIndex];
                test.layout.sizing = {
                    .width = CLAY_SIZING_FIXED(static_cast<float>(cachedImages[testIndex].width)),
                    .height = CLAY_SIZING_FIXED(static_cast<float>(cachedImages[testIndex].height)),
                };

                CLAY(test)
                {
                }
            }

            for (auto& entry : file.entries)
            {
                double prev_maxDuration = maxDuration;
                // TODO(danybeam) delete after testing
                static Clay_ElementDeclaration t_entry = {};

                if (entry.duration < 0.0)
                {
                    t_entry.backgroundColor = {.r = 255, .g = 0, .b = 0, .a = 255};
                    //t_entry.layout.sizing.width = CLAY_SIZING_FIXED(static_cast<float>(maxDuration));
                    t_entry.layout.sizing.height = CLAY_SIZING_FIXED(32);
                }
                else
                {
                    if (!cachedImages.contains(entry.duration))
                    {
                        maxDuration = std::max(maxDuration, entry.duration);
                        //Image generatedImage = GenImageColor(entry.duration, 20, {230, 41, 50, 255});
                        Image generatedImage = GenImageGradientLinear((int)entry.duration, 32, 90, {0, 255, 0, 255},
                                                                     {255, 0, 255, 255});
                        cachedImages.insert_or_assign(entry.duration,LoadTextureFromImage(generatedImage));
                        UnloadImage(generatedImage);
                    }

                    t_entry.image.imageData = &cachedImages[entry.duration];
                    t_entry.layout.sizing = {
                        .width = CLAY_SIZING_FIXED(static_cast<float>(cachedImages[entry.duration].width)),
                        .height = CLAY_SIZING_FIXED(static_cast<float>(cachedImages[entry.duration].height))
                    };
                }

                CLAY(t_entry)
                {
                }

                if (prev_maxDuration != maxDuration)
                {
                    auto& loadedFonts = it.world().get<LoadedFonts>();
                    if (IsTextureValid(timeBar))
                    {
                        UnloadTexture(timeBar); // unload the previous bar texture
                    }
                    timeBar = createTimeBarTexture(loadedFonts.fonts[FONT_WEIGHT::FONT_REGULAR]);
                }
            }
        }
    }

    // ProfileLock::RequestForceUnlock();
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
