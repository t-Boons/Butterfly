add_executable(ButterflyGame
    src/Main.cpp
    src/Core/GameApplication.cpp
    src/Core/GameViewport.cpp
)

target_include_directories(ButterflyGame PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(ButterflyGame PRIVATE Butterfly)

add_custom_command(TARGET ButterflyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Butterfly/lib/dxc/bin/dxcompiler.dll" "$<TARGET_FILE_DIR:ButterflyGame>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Butterfly/lib/dxc/bin/dxil.dll" "$<TARGET_FILE_DIR:ButterflyGame>/dxil.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:optick>" "$<TARGET_FILE_DIR:ButterflyGame>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory    "${CMAKE_SOURCE_DIR}/Butterfly/Assets" "$<TARGET_FILE_DIR:ButterflyGame>/Assets"
)