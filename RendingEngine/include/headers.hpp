#define GLFW_INCLUDE_VULKAN
#define TINYOBJLOADER_IMPLEMENTATION

#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <array>
#include <vector>
#include <optional>
#include <unordered_map>
#include <fstream>
#include <iterator>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "TransMat.hpp"
#include "Camera.hpp"
#include "Loader.hpp"