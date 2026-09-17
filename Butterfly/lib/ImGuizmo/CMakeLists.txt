
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



set(IMGUIZMO_STANDALONE FALSE)

add_library(imguizmo STATIC
   ImGuizmo.cpp
   GraphEditor.cpp
   ImCurveEdit.cpp
   ImGradient.cpp
   ImSequencer.cpp
   ImVectorEditor.cpp
)

add_library(imguizmo::imguizmo ALIAS imguizmo)

target_include_directories(imguizmo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

if(IMGUIZMO_BUILD_EXAMPLE)
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
    target_link_libraries(imguizmo PUBLIC imgui)
endif()

if(IMGUIZMO_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
