add_library(tinygltf STATIC
tiny_gltf_loader.cpp
)
target_include_directories(tinygltf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(tinygltf PUBLIC
    stbimage
)
