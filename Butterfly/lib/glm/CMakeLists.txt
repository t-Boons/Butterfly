add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(glm INTERFACE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_LEFT_HANDED
)