// ReSharper disable CppParameterMayBeConstPtrOrRef
// ReSharper disable CppInitializedValueIsAlwaysRewritten
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
import Maths;

namespace constants::profiling_renderer_constants
{
    constexpr int c_separator_size = 2;
    constexpr int c_row_height = 32;
    constexpr int c_font_size = 32;
    constexpr int c_font_letter_spacing = 0;
    constexpr int c_font_line_height = 0;
    constexpr int c_element_gap_small = 0;
    constexpr int c_element_gap_regular = 8;
    constexpr int c_time_jump = 200;

    constexpr Clay_Color c_background_color_frame = {55, 55, 55, 255};
    constexpr Clay_Color c_background_color_separator = {85, 85, 85, 255};
    constexpr Clay_Color c_background_color_error = {185, 0, 0, 255};
    constexpr Color c_timebar_color_deallocated = {0, 185, 0, 255};
    constexpr Color c_timebar_color_timeline = {0, 0, 185, 255};
    constexpr Clay_Color c_text_color_clay = {200, 200, 200, 255};
    constexpr Color c_text_color_raylib = {
        static_cast<unsigned char>(c_text_color_clay.r),
        static_cast<unsigned char>(c_text_color_clay.g),
        static_cast<unsigned char>(c_text_color_clay.b),
        static_cast<unsigned char>(c_text_color_clay.a),
    };
}

export struct loaded_fonts
{
    /**
    * Store the loaded fonts in memory.
    * It has enough capacity for each font weight in both normal and italic variants. 
    */
    Font* fonts;
};

export namespace mem_profile_viewer
{
    /**
     * Declaration of the module to render the profiling results
     */
    class rendering_module
    {
    public:
        /**
         * Constructor to be used with flecs import
         * @param world reference of the flecs world to register the module on
         */
        rendering_module(const flecs::world& world);
    };

    /**
     * Flecs tag to request that the results are rendered.
     * It is added by default by the module but in theory it could be removed.
     */
    struct render_results
    {
    };

    /**
    * Cache of textures representing the bars depending on how long it was allocated for
    */
    struct rendering_cache
    {
        std::unordered_map<double, std::vector<Texture>> cachedImages = {};
        double maxDuration = -99999;
        long long maxIndex = 0;
        std::vector<Texture> timeBar = {};
        bool generateTimebar = false;
    };

    struct ui_element_component
    {
        Clay_ElementDeclaration config;
        mem_profile_viewer::Vector2 reference_scroll_offset;
        mem_profile_viewer::Vector2 current_scroll_offset;

        ui_element_component(const Clay_ElementDeclaration& config) :
            config(config),
            reference_scroll_offset(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()),
            current_scroll_offset(0, 0)
        {
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Clay_ElementDeclaration() const { return config; }
    };

    struct ui_base_frame : public ui_element_component
    {
        ui_base_frame(const Clay_ElementDeclaration& config) : ui_element_component(config)
        {
        }
    };

    struct ui_address_holder : public ui_element_component
    {
        ui_address_holder(const Clay_ElementDeclaration& config) : ui_element_component(config)
        {
        }
    };

    struct ui_bar_holder : public ui_element_component
    {
        ui_bar_holder(const Clay_ElementDeclaration& config) : ui_element_component(config)
        {
        }
    };
}

module :private;

// forward declarations

/**
 * Private callback for the flecs system to update the scroll offset of the UI
 * @param it flecs iterator
 * @param io_state state of the io
 * @param base_frame base frame of the UI
 * @param address_holder address holder UI element
 * @param bar_holder bar holder UI element
 */
void update_ui_elements(
    flecs::iter it,
    size_t,
    mem_profile_viewer::IOState_Component& io_state,
    mem_profile_viewer::ui_base_frame& base_frame,
    mem_profile_viewer::ui_address_holder& address_holder,
    mem_profile_viewer::ui_bar_holder& bar_holder
);

/**
 * Private callback for the flecs system to render the results of the file
 * @param it flecs iterator
 * @param file component holding the file data
 * @param io_state_component component holding the state and the target of the IO devices
 * @param rendering_cache singleton component containing cached elements
 */
void render_file_results(
    flecs::iter& it,
    size_t,
    mem_profile_viewer::File_Holder& file,
    mem_profile_viewer::IOState_Component& io_state_component,
    mem_profile_viewer::rendering_cache& rendering_cache
);

void create_time_bar_texture(Font& font, mem_profile_viewer::rendering_cache& rendering_cache);


mem_profile_viewer::rendering_module::rendering_module(const flecs::world& world)
{
    // If other modules haven't populated the texture size populate it here since we need it.
    if (fw::maxTextureSize < 0)
    {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &fw::maxTextureSize);
    }

    // Define the module
    // ReSharper disable once CppExpressionWithoutSideEffects
    world.module<rendering_module>("Rendering Module");

    // Add tag to world
    world.add<render_results>();
    world.add<rendering_cache>();

    // Add basic UI elements
    Clay_ElementDeclaration t_base_frame = {};
    t_base_frame.id = CLAY_ID("BaseFrame");
    t_base_frame.backgroundColor = constants::profiling_renderer_constants::c_background_color_frame;
    t_base_frame.layout.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()};
    t_base_frame.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;
    t_base_frame.layout.childGap = constants::profiling_renderer_constants::c_element_gap_regular;
    t_base_frame.layout.padding = {
        .left = constants::profiling_renderer_constants::c_element_gap_regular,
        .right = constants::profiling_renderer_constants::c_element_gap_small,
        .top = constants::profiling_renderer_constants::c_element_gap_small,
        .bottom = constants::profiling_renderer_constants::c_element_gap_small
    };

    world.emplace<ui_base_frame>(t_base_frame);

    Clay_ElementDeclaration t_address_holder = {};
    t_address_holder.id = CLAY_ID("AddressHolder");
    t_address_holder.backgroundColor = constants::profiling_renderer_constants::c_background_color_frame;
    t_address_holder.layout.sizing = {.width = CLAY_SIZING_FIT(), .height = CLAY_SIZING_GROW()};
    t_address_holder.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
    t_address_holder.layout.childGap = 0;
    t_address_holder.clip = {
        .horizontal = false,
        .vertical = true
    };
    world.emplace<ui_address_holder>(t_address_holder);

    Clay_ElementDeclaration t_entry_bars = {};
    t_entry_bars.id = CLAY_ID("EntryBars");
    t_entry_bars.backgroundColor = constants::profiling_renderer_constants::c_background_color_frame;
    t_entry_bars.layout.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()};
    t_entry_bars.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
    t_entry_bars.layout.childGap = constants::profiling_renderer_constants::c_element_gap_regular;
    t_entry_bars.clip = {
        .horizontal = true,
        .vertical = true,
    };
    world.emplace<ui_bar_holder>(t_entry_bars);

    // Declare system
    world.system<IOState_Component, ui_base_frame, ui_address_holder, ui_bar_holder>("Update ioState of elements")
         .with<render_results>()
         .term_at(0).singleton()
         .term_at(1).singleton()
         .term_at(2).singleton()
         .term_at(3).singleton()
         .kind(flecs::PreUpdate)
         .each(update_ui_elements);

    world.system<File_Holder, IOState_Component, rendering_cache>("Render loaded file")
         .with<render_results>()
         .term_at(0).singleton()
         .term_at(1).singleton()
         .term_at(2).singleton()
         .kind(flecs::OnUpdate)
         .each(render_file_results);
}

void update_ui_elements(
    flecs::iter it,
    size_t,
    mem_profile_viewer::IOState_Component& io_state,
    mem_profile_viewer::ui_base_frame& base_frame,
    mem_profile_viewer::ui_address_holder& address_holder,
    mem_profile_viewer::ui_bar_holder& bar_holder
)
{
    // Set reference points if not set yet

    // ReSharper disable once CommentTypo
    // skipping baseframe as it doesn't have clipping as of now

    if (
        address_holder.reference_scroll_offset.x == std::numeric_limits<float>::lowest() &&
        address_holder.reference_scroll_offset.y == std::numeric_limits<float>::lowest()
    )
    {
        address_holder.reference_scroll_offset = io_state.current_mouse_wheel;
    }

    if (
        bar_holder.reference_scroll_offset.x == std::numeric_limits<float>::lowest() &&
        bar_holder.reference_scroll_offset.y == std::numeric_limits<float>::lowest()
    )
    {
        bar_holder.reference_scroll_offset = io_state.current_mouse_wheel;
    }

    // Update the current scroll wheel
    address_holder.current_scroll_offset = io_state.current_mouse_wheel;
    bar_holder.current_scroll_offset = io_state.current_mouse_wheel;

    // Update the children offset
    // Address bar only scrolls vertically
    address_holder.config.clip.childOffset = {
        0,
        address_holder.reference_scroll_offset.y - address_holder.current_scroll_offset.y
    };
    // simpler as it scrolls in 2 directions
    bar_holder.config.clip.childOffset = bar_holder.reference_scroll_offset - bar_holder.current_scroll_offset;
}

void render_file_results(
    flecs::iter& it,
    size_t,
    mem_profile_viewer::File_Holder& file,
    mem_profile_viewer::IOState_Component& io_state_component,
    mem_profile_viewer::rendering_cache& rendering_cache
)
{
    ProfileLock::RequestForceLock();

    if (file.entries.empty())
    {
        CLAY({
             .layout = {
             .sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
             .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
             }
             })
        {
            CLAY_TEXT(
                CLAY_STRING(
                    "Drag and drop profiling results file to visualize.\nPlease note there's no error checking yet :)"
                ),
                CLAY_TEXT_CONFIG({
                    .userData = nullptr,
                    .textColor = constants::profiling_renderer_constants::c_text_color_clay,
                    .fontId = FONT_WEIGHT::FONT_REGULAR,
                    .fontSize = constants::profiling_renderer_constants::c_font_size,
                    .letterSpacing = constants::profiling_renderer_constants::c_font_letter_spacing,
                    .lineHeight = constants::profiling_renderer_constants::c_font_line_height,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                    })
            );
        }

        return;
    }

    // This will explode if the elements have not been added to the singleton. That is intentional.
    const Clay_ElementDeclaration& t_base_frame = it.world().get<mem_profile_viewer::ui_base_frame>();
    const Clay_ElementDeclaration& t_address_holder = it.world().get<mem_profile_viewer::ui_address_holder>();
    const Clay_ElementDeclaration& t_entry_bars = it.world().get<mem_profile_viewer::ui_bar_holder>();


    Clay_ElementDeclaration t_horizontal_separator = {};
    t_horizontal_separator.backgroundColor = constants::profiling_renderer_constants::c_background_color_separator;
    t_horizontal_separator.layout.sizing = {
        .width = CLAY_SIZING_GROW(20),
        .height = CLAY_SIZING_FIXED(constants::profiling_renderer_constants::c_separator_size)
    };

    Clay_ElementDeclaration t_vertical_separator = {};
    t_vertical_separator.backgroundColor = constants::profiling_renderer_constants::c_background_color_separator;
    t_vertical_separator.layout.sizing = {
        .width = CLAY_SIZING_FIXED(constants::profiling_renderer_constants::c_separator_size),
        .height = CLAY_SIZING_GROW()
    };

    CLAY(t_base_frame)
    {
        CLAY(t_address_holder)
        {
            CLAY_TEXT(
                CLAY_STRING("Address"),
                CLAY_TEXT_CONFIG({
                    .userData = nullptr,
                    .textColor = constants::profiling_renderer_constants::c_text_color_clay,
                    .fontId = FONT_WEIGHT::FONT_REGULAR,
                    .fontSize = constants::profiling_renderer_constants::c_font_size,
                    .letterSpacing = constants::profiling_renderer_constants::c_font_letter_spacing,
                    .lineHeight = constants::profiling_renderer_constants::c_font_line_height,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                    })
            );

            CLAY(t_horizontal_separator)
            {
            }

            for (mem_profile_viewer::Memory_TraceEntry& entry : file.entries)
            {
                Clay_String address_name = {};
                address_name.chars = entry.memLocation.c_str();
                address_name.length = entry.memLocation.length();
                address_name.isStaticallyAllocated = false;

                CLAY_TEXT(
                    address_name,
                    CLAY_TEXT_CONFIG({
                        .userData = nullptr,
                        .textColor = constants::profiling_renderer_constants::c_text_color_clay,
                        .fontId = FONT_WEIGHT::FONT_REGULAR,
                        .fontSize = constants::profiling_renderer_constants::c_font_size,
                        .letterSpacing = constants::profiling_renderer_constants::c_font_letter_spacing,
                        .lineHeight = constants::profiling_renderer_constants::c_font_line_height,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                        })
                );

                if (entry != file.entries.back())
                {
                    CLAY(t_horizontal_separator)
                    {
                    }
                }
            }
        }

        CLAY(t_vertical_separator)
        {
        }

        CLAY(t_entry_bars)
        {
            Clay_ElementDeclaration t_barHolder = {};
            t_barHolder.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;
            t_barHolder.layout.childGap = constants::profiling_renderer_constants::c_element_gap_small;
            // Explicitly set to 0 to not have to deal with Clay defaults
            t_barHolder.layout.padding = {.left = 0, .right = 0, .top = 0, .bottom = 0};
            t_barHolder.layout.sizing = {
                .width = CLAY_SIZING_GROW(),
                .height = CLAY_SIZING_FIXED(constants::profiling_renderer_constants::c_row_height)
            };

            CLAY(t_barHolder)
            {
                Clay_ElementDeclaration t_entry = {};
                // Explicitly set to 0 to not have to deal with Clay defaults
                t_entry.layout.padding = {.left = 0, .right = 0, .top = 0, .bottom = 0};
                t_entry.layout.childGap = 0;
                t_entry.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;

                if (rendering_cache.generateTimebar)
                {
                    const auto fonts = it.world().get<loaded_fonts>();
                    create_time_bar_texture(fonts.fonts[FONT_WEIGHT::FONT_REGULAR], rendering_cache);
                    rendering_cache.generateTimebar = false;
                }

                CLAY(t_entry)
                {
                    for (auto& texture : rendering_cache.timeBar)
                    {
                        Clay_ElementDeclaration t_imageHolder = {};
                        t_imageHolder.image.imageData = &texture;
                        t_imageHolder.layout.sizing = {
                            .width = CLAY_SIZING_FIXED(static_cast<float>(texture.width)),
                            .height = CLAY_SIZING_FIXED(static_cast<float>(texture.height))
                        };

                        CLAY(t_imageHolder)
                        {
                        }
                    }
                }


                for (auto& entry : file.entries)
                {
                    if (entry.duration < 0)
                    {
                        t_entry.layout.sizing = {
                            .width = CLAY_SIZING_GROW(),
                            .height = CLAY_SIZING_FIXED(constants::profiling_renderer_constants::c_row_height)
                        };
                        t_entry.backgroundColor = constants::profiling_renderer_constants::c_background_color_error;
                        CLAY(t_entry)
                        {
                        }

                        if (entry != file.entries.back())
                        {
                            CLAY(t_horizontal_separator)
                            {
                            }
                        }
                        continue;
                    }

                    const auto prev_max_duration = rendering_cache.maxDuration;
                    rendering_cache.maxDuration = std::max(rendering_cache.maxDuration, entry.duration);

                    rendering_cache.generateTimebar |= prev_max_duration != rendering_cache.maxDuration;


                    if (!rendering_cache.cachedImages.contains(entry.duration))
                    {
                        rendering_cache.cachedImages[entry.duration] = std::vector<Texture>{};
                        for (int i = 1; entry.duration > i * fw::maxTextureSize; i++)
                        {
                            const auto image = GenImageColor(
                                fw::maxTextureSize,
                                constants::profiling_renderer_constants::c_row_height,
                                constants::profiling_renderer_constants::c_timebar_color_deallocated
                            );
                            rendering_cache.cachedImages[entry.duration].push_back(LoadTextureFromImage(image));
                            UnloadImage(image);
                        }

                        const auto image = GenImageColor(
                            static_cast<int>(entry.duration) % fw::maxTextureSize,
                            constants::profiling_renderer_constants::c_row_height,
                            constants::profiling_renderer_constants::c_timebar_color_deallocated
                        );
                        rendering_cache.cachedImages[entry.duration].push_back(LoadTextureFromImage(image));
                        UnloadImage(image);
                    }

                    CLAY(t_entry)
                    {
                        for (auto& cached_image : rendering_cache.cachedImages[entry.duration])
                        {
                            Clay_ElementDeclaration t_image_holder = {};
                            t_image_holder.image.imageData = &cached_image;
                            t_image_holder.layout.sizing = {
                                .width = CLAY_SIZING_FIXED(static_cast<float>(cached_image.width)),
                                .height = CLAY_SIZING_FIXED(static_cast<float>(cached_image.height))
                            };

                            CLAY(t_image_holder)
                            {
                            }
                        }
                    }

                    if (entry != file.entries.back())
                    {
                        CLAY(t_horizontal_separator)
                        {
                        }
                    }
                }
            }
        }
    }

    ProfileLock::RequestForceUnlock();
}

void create_time_bar_texture(Font& font, mem_profile_viewer::rendering_cache& rendering_cache)
{
    for (const auto image : rendering_cache.timeBar)
    {
        if (IsTextureValid(image))
        {
            UnloadTexture(image);
        }
    }
    rendering_cache.timeBar.clear();

    const long long image_width = rendering_cache.maxDuration;
    long long tracked_ms = 0;

    for (long long i = 1; image_width > i * fw::maxTextureSize; i++)
    {
        auto image = GenImageColor(
            fw::maxTextureSize,
            constants::profiling_renderer_constants::c_row_height,
            constants::profiling_renderer_constants::c_timebar_color_timeline
        );

        for (int j = 0; j < image.width; j += constants::profiling_renderer_constants::c_time_jump)
        {
            std::string time_marker = std::format("|{}ms", tracked_ms);
            ImageDrawTextEx(
                &image,
                font,
                time_marker.c_str(),
                {static_cast<float>(j), 0},
                constants::profiling_renderer_constants::c_font_size,
                constants::profiling_renderer_constants::c_font_letter_spacing,
                constants::profiling_renderer_constants::c_text_color_raylib
            );

            tracked_ms += constants::profiling_renderer_constants::c_time_jump;
        }

        rendering_cache.timeBar.push_back(LoadTextureFromImage(image));
        UnloadImage(image);
    }

    auto image = GenImageColor(
        static_cast<int>(image_width) % fw::maxTextureSize,
        constants::profiling_renderer_constants::c_row_height,
        constants::profiling_renderer_constants::c_text_color_raylib
    );

    for (int j = 0; j < image.width; j += constants::profiling_renderer_constants::c_time_jump)
    {
        std::string time_marker = std::format("|{}ms", tracked_ms);
        ImageDrawTextEx(
            &image,
            font,
            time_marker.c_str(),
            {static_cast<float>(j), 0},
            constants::profiling_renderer_constants::c_font_size,
            constants::profiling_renderer_constants::c_font_letter_spacing,
            constants::profiling_renderer_constants::c_text_color_raylib
        );

        tracked_ms += constants::profiling_renderer_constants::c_time_jump;
    }
    rendering_cache.timeBar.push_back(LoadTextureFromImage(image));
    UnloadImage(image);
}
