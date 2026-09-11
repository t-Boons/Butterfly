add_library(stbimage STATIC
stb_loader.cpp
)	
target_include_directories(stbimage PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
