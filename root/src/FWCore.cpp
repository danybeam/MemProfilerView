#include "FWCore.h"

#pragma warning( push, 0 )

#define CLAY_IMPLEMENTATION
#define RAYMATH_IMPLEMENTATION

#include <Clay/clay.h>
#include <Clay/clay_renderer_raylib.cpp>  // NOLINT(bugprone-suspicious-include)

#pragma warning( pop )

#include <flecs.h>
#include <iostream>
#include <SDL.h>
#include <external/glad.h>

#include <Constants.h>

#include "profiler.h"

import IOState;
import ProfilingRenderer;
import utils;

fw::FWCore::FWCore(uint32_t width, uint32_t height) :
    m_errorCodes_(NONE),
    m_mouse_wheel_(0, 0),
    mouse_wheel_acceleration_(fw::FWCore::mouse_wheel_acceleration_base),
    //mouse_wheel_acceleration_tween_(mouse_wheel_acceleration_),
    m_window_client_height_(height),
    m_window_client_width_(width)
{
    m_errorCodes_ = Init();
}

fw::FWCore::~FWCore()
{
    Shutdown();
}

uint32_t fw::FWCore::Run(std::unique_ptr<flecs::world>&& world)
{
    m_world_ = std::move(world);


    // This needs to be added during run to ensure the fonts are loaded properly
    m_world_->emplace<loaded_fonts>(m_clay_font_);

    m_world_->system<mem_profile_viewer::IOState_Component>()
            .term_at(0).singleton()
            .kind(flecs::OnLoad)
            .each(
                [=](flecs::iter& iter, size_t row, mem_profile_viewer::IOState_Component& ioState_component)
                {
                    this->Clay_updateIOState(iter, row, ioState_component);
                });

    m_world_->system("Start drawing")
            .kind(flecs::PostLoad)
            .each(&Clay_startDrawing);

    m_world_->system<const loaded_fonts>("End Drawing")
            .term_at(0).singleton()
            .kind(flecs::OnStore)
            .each(&Clay_endDrawing);

    double lastTime = utils::getSystemTimeSinceProgramStart();
    double currentTime = lastTime;
    float deltaTime = 0;
    bool flecsProgress = true;

    do
    {
        // Calculate delta time
        deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        // Backup the state of the keyboard and mouse.
        for (int i = 0; i < 256; i++)
            m_old_key_states_[i] = m_key_states_[i];

        for (int i = 0; i < 3; i++)
            m_old_mouse_button_states_[i] = m_mouse_button_states_[i];

        // update the world before resetting the mouse wheel.
        // I really didn't want to make a whole new system just for that.
    }
    while (!WindowShouldClose() && m_world_->progress(deltaTime));

    // Request unlock to register all the pending deallocs properly
    // This should not be necessary if I put the flecs world in immediate mode
    // and turned deffer mode off and on again in the rendering system
    // However I decided to not do that as it would need to be done on all systems when ultimately
    // it doesn't matter exactly how long memory was allocated for, as much as whether it got cleaned or not. 
    ProfileLock::RequestForceUnlock();
    return 0;
}

uint8_t fw::FWCore::Init()
{
    if (!InitClay())
    {
        return ERRORCODES::CLAY_INIT_ERROR;
    }

    if (!InitSDL())
    {
        return ERRORCODES::SDL_INIT_ERROR;
    }

    return ERRORCODES::NONE;
}

bool fw::FWCore::InitClay()
{
    Clay_Raylib_Initialize(static_cast<int>(m_window_client_width_), static_cast<int>(m_window_client_height_),
                           "raylib clay base", 0);

    m_clay_requiredMemory_ = static_cast<uint64_t>(8) * Clay_MinMemorySize();

    m_clay_memoryArena_ = {};
    m_clay_memoryArena_.capacity = m_clay_requiredMemory_;
    m_clay_memoryArena_.memory = static_cast<char*>(malloc(m_clay_requiredMemory_));

    m_clay_resolution_ = {
        .width = static_cast<float>(GetScreenWidth()),
        .height = static_cast<float>(GetScreenHeight())
    };

    m_clay_errorHandler_ = {};
    m_clay_errorHandler_.errorHandlerFunction = &Clay_errorHandlerFunction;
    m_clay_errorHandler_.userData = nullptr;

    m_clay_font_[0] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveExtraLight.otf", 32, nullptr, 400);
    m_clay_font_[1] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveExtraLightItalic.otf", 32, nullptr, 400);
    m_clay_font_[2] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveLight.otf", 32, nullptr, 400);
    m_clay_font_[3] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveLightItalic.otf", 32, nullptr, 400);
    m_clay_font_[4] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveSemiLight.otf", 32, nullptr, 400);
    m_clay_font_[5] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveSemiLightItalic.otf", 32, nullptr, 400);
    m_clay_font_[6] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveRegular.otf", 32, nullptr, 400);
    m_clay_font_[7] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveRegularItalic.otf", 32, nullptr, 400);
    m_clay_font_[8] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveSemiBold.otf", 32, nullptr, 400);
    m_clay_font_[9] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveSemiBoldItalic.otf", 32, nullptr, 400);
    m_clay_font_[10] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveBold.otf", 32, nullptr, 400);
    m_clay_font_[11] = LoadFontEx("resources/CaskaydiaCove/CaskaydiaCoveBoldItalic.otf", 32, nullptr, 400);

    Clay_Initialize(m_clay_memoryArena_, m_clay_resolution_, m_clay_errorHandler_);
    Clay_SetMeasureTextFunction(Raylib_MeasureText, m_clay_font_);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    return true;
}

bool fw::FWCore::InitSDL()
{
    return SDL_AddEventWatch(&FWCore::ProcessSDLEvent_wrapper, this);
}

void fw::FWCore::Shutdown()
{
    ShutdownClay();
    ShutdownSDL();
    CloseWindow();
}

void fw::FWCore::ShutdownClay()
{
    delete m_clay_memoryArena_.memory;
    m_clay_memoryArena_.memory = nullptr;
    m_clay_errorHandler_.errorHandlerFunction = nullptr;
}

void fw::FWCore::ShutdownSDL()
{
    SDL_RemoveEventWatch(&FWCore::ProcessSDLEvent_wrapper, this);
}

// ReSharper disable once CppPassValueParameterByConstReference
void fw::FWCore::Clay_errorHandlerFunction(Clay_ErrorData err)
{
    std::cout << "CLAY ERROR: " << err.errorText.chars << "\n";
}

bool fw::FWCore::ProcessSDLEvent_wrapper(void* userData, SDL_Event* event)
{
    if (FWCore* obj = static_cast<FWCore*>(userData))
    {
        obj->ProcessSDLEvent(event);
        return true;
    }

    return false;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
bool fw::FWCore::ProcessSDLEvent(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_MOUSE_WHEEL:
        {
            SDL_Log("SDL_EVENT_MOUSE_WHEEL");
            SDL_Log("x %f y %f flipped", event->wheel.x, event->wheel.y, event->wheel.direction);

            this->mouse_wheel_acceleration_++;

            float acceleration = this->mouse_wheel_acceleration_;

            m_mouse_wheel_.x += event->wheel.x * acceleration;
            m_mouse_wheel_.y += event->wheel.y * acceleration;
        }
        break;
    default:
        SDL_Log("Unknown Event");
        break;
    }

    return true;
}

void fw::FWCore::Clay_updateIOState(flecs::iter& iter, size_t, mem_profile_viewer::IOState_Component& ioState_component)
{
    ioState_component.prev_mouse_wheel = ioState_component.current_mouse_wheel;
    ioState_component.target_mouse_wheel += this->m_mouse_wheel_;

    ioState_component.target_mouse_wheel.Clamp(0, std::numeric_limits<float>::infinity());

    if (ioState_component.target_mouse_wheel.x != ioState_component.current_mouse_wheel_tween_x.m_target())
    {
        ioState_component.current_mouse_wheel_tween_x.start(
            ioState_component.target_mouse_wheel.x,
            1.0f, // arbitrary duration
            mem_profile_viewer::Elastic::Out
        );
    }

    if (ioState_component.target_mouse_wheel.y != ioState_component.current_mouse_wheel_tween_y.m_target())
    {
        ioState_component.current_mouse_wheel_tween_y.start(
            ioState_component.target_mouse_wheel.y,
            1.0f, // arbitrary duration
            mem_profile_viewer::Elastic::Out
        );
    }

    if (this->mouse_wheel_acceleration_ - ioState_component.mouse_wheel_acceleration > 0.5f)
    {
        ioState_component.mouse_wheel_acceleration = this->mouse_wheel_acceleration_;
        ioState_component.mouse_wheel_acceleration_tween.start(
            ioState_component.mouse_wheel_acceleration_base,
            1.0f, // arbitrary duration
            mem_profile_viewer::Elastic::Out
        );
    }

    ioState_component.current_mouse_wheel_tween_x.on_update(iter.delta_time());
    ioState_component.current_mouse_wheel_tween_y.on_update(iter.delta_time());
    ioState_component.mouse_wheel_acceleration_tween.on_update(iter.delta_time());

    this->m_mouse_wheel_ = {0, 0};
    this->mouse_wheel_acceleration_ = ioState_component.mouse_wheel_acceleration;

    Clay_SetLayoutDimensions({
        static_cast<float>(GetRenderWidth()),
        static_cast<float>(GetRenderHeight())
    });

    Clay_SetPointerState(
        {static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY())},
        IsMouseButtonDown(0) || IsMouseButtonDown(1)
    );

    Clay_UpdateScrollContainers(
        false,
        ioState_component.current_mouse_wheel - ioState_component.prev_mouse_wheel,
        iter.delta_time()
    );
}

void fw::FWCore::Clay_startDrawing(flecs::iter& iter, size_t)
{
    Clay_BeginLayout();
    BeginDrawing();
    ClearBackground({0, 0, 0, 255});
}

void fw::FWCore::Clay_endDrawing(flecs::iter& iter, size_t, const loaded_fonts& fonts)
{
    auto clay_RenderCommandArray = Clay_EndLayout();

    Clay_Raylib_Render(clay_RenderCommandArray, fonts.fonts);
    EndDrawing();
}
