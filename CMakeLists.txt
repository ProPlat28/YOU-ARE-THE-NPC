cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
project(YouAreTheNpc VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
  raylib
  URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
FetchContent_MakeAvailable(raylib)

add_executable(you_are_the_npc
  src/main.cpp
  src/City.cpp
  src/Simulation.cpp
  src/Game.cpp
)

target_include_directories(you_are_the_npc PRIVATE src)
target_link_libraries(you_are_the_npc PRIVATE raylib)

if(MSVC)
  target_compile_options(you_are_the_npc PRIVATE /W4 /permissive-)
  target_compile_definitions(you_are_the_npc PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
  target_compile_options(you_are_the_npc PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(you_are_the_npc PRIVATE m pthread dl)
endif()

install(TARGETS you_are_the_npc RUNTIME DESTINATION bin)
