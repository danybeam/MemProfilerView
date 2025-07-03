#include "FWCore.h"

#pragma warning( push, 0 )
#include <Clay/clay.h>
#include <Clay/clay_renderer_raylib.cpp>  // NOLINT(bugprone-suspicious-include)
#pragma warning( pop )

#include <iostream>
#include <SDL.h>

#include "Font.h"
import utils;

fw::FWCore::FWCore(uint32_t width, uint32_t height) :
    m_errorCodes_(NONE),
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

    double lastTime = utils::getSystemTimeSinceGameStart();
    double currentTime = lastTime;
    float deltaTime = 0;

    // TODO(Implementation) This needs to be yanked out
    // TODO(Implementation) Implement flecs modules in project
    // TODO(Implementation) Do world setup in main before calling this function
    m_world_->system().run(
        [=](flecs::iter& it)
        {
            Clay_ElementDeclaration t_baseFrame = {};
            t_baseFrame.id = CLAY_ID("BaseFrame");
            t_baseFrame.backgroundColor = {.r = 0, .g = 0, .b = 0, .a = 255};
            t_baseFrame.layout.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()};
            t_baseFrame.layout.layoutDirection = CLAY_TOP_TO_BOTTOM;

            Clay_BeginLayout();

            CLAY(t_baseFrame)
            {
                CLAY_TEXT(
                    CLAY_STRING(
                        "This is a sample screen to test compilation and basic functionality. check FWCore.cpp to remove this and check the TODO(Implementation) comments."
                    ),
                    CLAY_TEXT_CONFIG(
                        {
                        .userData = nullptr,
                        .textColor = {255,255,0,255},
                        .fontId = FONT_WEIGHT::FONT_REGULAR ,
                        .fontSize = 32,
                        .letterSpacing = 0,
                        .lineHeight = 0,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                        }

                    )
                );
            }

            auto clay_RenderCommandArray = Clay_EndLayout();

            BeginDrawing();
            ClearBackground({255, 255, 255, 255});
            Clay_Raylib_Render(clay_RenderCommandArray, m_clay_font_);
            EndDrawing();
        }
    );

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

        // Reset mouse wheel scroll
        m_mouse_wheel_ = 0;
    }
    while (!WindowShouldClose() && m_world_->progress(deltaTime));

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
    // TODO(Implementation) Do your event processing here
    switch (event->type)
    {
    default:
        SDL_Log("Unknown Event");
        break;
    }

    return true;
}
