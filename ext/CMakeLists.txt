message(STATUS "Adding Raylib")
set(PLATFORM "SDL")
set(SDL_RENDER OFF)
add_subdirectory("raylib")
set_property(TARGET raylib APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "raylib")

target_include_directories(raylib PUBLIC
  "${CMAKE_SOURCE_DIR}/ext/sdl/include/SDL3"
)

message(STATUS "Adding Flecs")
option(FLECS_STATIC "Build static flecs lib" ON)
option(FLECS_SHARED "Build shared flecs lib" OFF)
option(FLECS_PIC "Compile static flecs lib with position independent code (PIC)" ON)
option(FLECS_TESTS "Build flecs tests" OFF)
add_subdirectory("flecs")