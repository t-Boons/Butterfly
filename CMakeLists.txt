cmake_minimum_required(VERSION 3.21)

set(CMAKE_CONFIGURATION_TYPES
    Debug
    Release
    RelWithDebInfo
    RelWithProfiling
)

project(ButterflyWorkspace LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Butterfly)
add_subdirectory(ButterflyDemo)

target_compile_definitions(Butterfly PRIVATE
    $<$<CONFIG:RelWithProfiling>:REL_WITH_PROFILING>
)

target_compile_definitions(ButterflyDemo PRIVATE
    $<$<CONFIG:RelWithProfiling>:REL_WITH_PROFILING>
)
