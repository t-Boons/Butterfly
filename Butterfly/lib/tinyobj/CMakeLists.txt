add_library(tinyobj STATIC
tiny_obj_loader_loader.cpp
)

target_include_directories(tinyobj INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")