#pragma once

#include <vulkan/vulkan.hpp>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "../renderer/vulkan/VkMesh.hpp"

using namespace renderer;

namespace engine{

	struct RenderObject {
	public:
		Mesh* mesh;
		Material* material;

	public:
		void SetScale(float in_scale) {
			scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3{ in_scale, in_scale, in_scale });
		}

		void SetScale(glm::vec3 in_scale) {
			scale = glm::scale(glm::mat4{ 1.0 }, in_scale);
		}

		void SetTranslate(glm::vec3 trans) {
			translate = glm::translate(glm::mat4{ 1.0 }, trans);
		}

		void SetRotate(glm::vec3 axis, float angle) {
			rotate = glm::rotate_slow(glm::mat4{ 1.0 }, glm::radians(angle), axis);
		}

		glm::mat4 GetTransform() const {
			return translate * rotate * scale;
		}

	private:
		glm::mat4 translate = glm::mat4{ 1 };
		glm::mat4 scale = glm::mat4{ 1 };
		glm::mat4 rotate = glm::mat4{ 1 };

		glm::mat4 transformMatrix;

	};
}
