add_executable(ButterflyDemo
    src/Main.cpp
    src/App.cpp
    src/Input/Input.cpp
    src/Tools/Camera.cpp
    src/Tools/SpectatorCamera.cpp
)

target_include_directories(ButterflyDemo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(ButterflyDemo PRIVATE Butterfly)

add_custom_command(TARGET ButterflyDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Butterfly/lib/dxc/bin/dxcompiler.dll" "$<TARGET_FILE_DIR:ButterflyDemo>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Butterfly/lib/dxc/bin/dxil.dll" "$<TARGET_FILE_DIR:ButterflyDemo>/dxil.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:optick>" "$<TARGET_FILE_DIR:ButterflyDemo>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory    "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:ButterflyDemo>/assets")
