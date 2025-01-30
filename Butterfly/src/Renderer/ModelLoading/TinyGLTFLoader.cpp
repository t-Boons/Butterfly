#pragma once
#include "Core/Common.hpp"

BF_WARNING_PUSH(0)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include "tinygltf/tiny_gltf.h"
BF_WARNING_POP