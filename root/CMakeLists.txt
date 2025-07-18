﻿# CMakeList.txt : CMake project , include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.28)



###################################################################
##              Library for the profiler
###################################################################

add_library(${CMAKE_PROJECT_NAME}_lib)

file(GLOB_RECURSE LIB_IXX
	"lib/*.ixx"
)

file(GLOB_RECURSE LIB_HDR
	"lib/*.h"
)

target_sources(${CMAKE_PROJECT_NAME}_lib 
PRIVATE
	${LIB_HDR}
PRIVATE FILE_SET CXX_MODULES FILES # Needed for modules
	${LIB_IXX}
)

target_include_directories(${CMAKE_PROJECT_NAME}_lib PUBLIC
	"lib"
)

# Separate source groups
source_group("lib" FILES
${LIB_IXX}
${LIB_HDR}
)

# set CPP version
set_target_properties(${CMAKE_PROJECT_NAME}_lib PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET ${CMAKE_PROJECT_NAME}_lib PROPERTY CXX_STANDARD 23)

add_library(${CMAKE_PROJECT_NAME}::lib ALIAS ${CMAKE_PROJECT_NAME}_lib)

###################################################################
##              Executable for the viewer
##  - TODO(danybeam) add options to compile lib only
##  - TODO(danybeam) add "installation" for executable
###################################################################


# Define executable
add_executable(${CMAKE_PROJECT_NAME})


# executable options
option(PROFILING "Whether to activate profiling in the project" ON)

# Define sources by group
# source files
file(GLOB ROOT_SRC
	"src/*.cpp"
)
# header files
file(GLOB ROOT_HDR
	"src/*.h"
)
# module files
file(GLOB_RECURSE ROOT_IXX
	"src/*.ixx"
)

# Management scripts
file(GLOB_RECURSE SCRIPTS
	"scripts/*.bat"
)

# Utils
file(GLOB_RECURSE UTILS
	"utils/*.h"
	"utils/*.cpp"
)
file(GLOB_RECURSE UTILS_MODULES
	"utils/*.ixx"
)

# Add sources to target
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
	${ROOT_SRC}
	${ROOT_HDR}
	${SCRIPTS}
	${UTILS}

	PRIVATE FILE_SET CXX_MODULES FILES # Needed for modules
	${UTILS_MODULES}
	${ROOT_IXX}
)

# Separate source groups
source_group("src" FILES
${ROOT_SRC}
)
source_group("hdr" FILES
${ROOT_HDR}
)
source_group("modules" FILES
${ROOT_IXX}
)
source_group("utils" FILES
${UTILS}
${UTILS_MODULES}
)
source_group("scripts" FILES
${SCRIPTS}
)

# Set target include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
"${PROJECT_SOURCE_DIR}/"
"${PROJECT_SOURCE_DIR}/root"
"${PROJECT_SOURCE_DIR}/root/src"
"${PROJECT_SOURCE_DIR}/ext/flecs/distr"
)

# Link libraries to .exe
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
	raylib
	SDL3::SDL3 # the ext CMakeLists specifies raylib to compile using SDL publicly but here we're linking it directly to avoid potential breaking changes 
	flecs_static
	nlohmann_json::nlohmann_json
	${CMAKE_PROJECT_NAME}::lib
)

# set CPP version
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 23)

# If PROFILING is requested add it to the project
if(PROFILING)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-DPROFILE=1> # Adds -DPROFILE=1 only in Debug builds
)
endif()

# Change working directory
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Copy resources to the right folder
add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/resources
        ${PROJECT_BINARY_DIR}/resources)