#pragma once

#include <vulkan/vulkan.hpp>
#include "../renderer/vulkan/VkMesh.hpp"

using namespace renderer;

namespace engine{

	struct RenderObject {
		Mesh* mesh;

		Material* material;

		glm::mat4 transformMatrix;
	};
}
