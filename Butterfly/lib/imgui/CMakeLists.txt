add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_impl_d3d12.cpp
    imgui_impl_glfw.cpp)

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(imgui PUBLIC d3d12 glfw)
target_compile_options(imgui PRIVATE /W0)
