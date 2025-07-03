add_subdirectory("${CMAKE_SOURCE_DIR}/ext/sdl" "${CMAKE_BINARY_DIR}/ext/sdl")

if(TARGET SDL3::SDL3)
get_target_property(_aliased SDL3::SDL3 ALIASED_TARGET)
  if(_aliased)
  add_library(SDL2::SDL2 ALIAS ${_aliased})
  endif()
endif()