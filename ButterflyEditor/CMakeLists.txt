add_executable(ButterflyEditor
    src/Main.cpp
    src/EditorApplication.cpp
    src/EditorViewport/SpectatorCamera.cpp
    src/EditorViewport/SceneViewport.cpp
)

target_include_directories(ButterflyEditor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(ButterflyEditor PRIVATE Butterfly)

add_custom_command(TARGET ButterflyEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Butterfly/lib/dxc/bin/dxcompiler.dll" "$<TARGET_FILE_DIR:ButterflyEditor>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Butterfly/lib/dxc/bin/dxil.dll" "$<TARGET_FILE_DIR:ButterflyEditor>/dxil.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:optick>" "$<TARGET_FILE_DIR:ButterflyEditor>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory    "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:ButterflyEditor>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory    "${CMAKE_CURRENT_SOURCE_DIR}/Editor" "$<TARGET_FILE_DIR:ButterflyEditor>/Editor"
)
