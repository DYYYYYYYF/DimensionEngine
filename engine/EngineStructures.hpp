#pragma once

#include <vulkan/vulkan.hpp>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "../renderer/vulkan/VkMesh.hpp"

using namespace renderer;

struct RenderObject {
public:
	void SetMesh(Mesh* in_mesh) { mesh = in_mesh; }
	Mesh* GetMesh() const { return mesh; }
	const Mesh* GetMeshConst() const { return mesh; }

	void SetMaterial(Material* in_material) { material = in_material; }
	Material* GetMaterial() const { return material; }
	const Material* GetMaterialConst() const { return material; }
		
	void SetScale(float in_scale) { scale = Vector3{ in_scale, in_scale, in_scale }; }
	void SetScale(Vector3 in_scale) { scale = in_scale; }
	Vector3& GetScale() { return scale; }
	const Vector3& GetScale() const { return scale; }

	void SetTranslate(Vector3 trans) { translate = trans; }
	Vector3& GetTranslate() { return translate; }
	const Vector3& GetTranslate() const { return translate; }

	void SetRotate(Vector3 axis, float angle) { rotateAxis = axis; rotateAngle = angle; }
		
	Matrix4 GetTransform() const {
		Matrix4 mtxScale = glm::scale(Matrix4{ 1.0 }, scale);
		Matrix4 mtxTranslate = glm::translate(glm::mat4{ 1.0 }, translate);
		Matrix4 mtxRotate = glm::rotate_slow(glm::mat4{ 1.0 }, glm::radians(rotateAngle), rotateAxis);

		return mtxTranslate * mtxRotate * mtxScale;
	}

	void SetModelFile(const char* filename) { modelFile = filename; }
	const char* GetModelFile() const { return modelFile; }

	void SetVertShader(const char* filename) { vertShader = filename; }
	const char* GetVertShader() const { return vertShader; }

	void SetFragShader(const char* filename) { fragShader = filename; }
	const char* GetFragShader() const { return fragShader; }


private:
	Mesh* mesh;
	Material* material;

	const char* modelFile;
	const char* vertShader;
	const char* fragShader;

	Vector3 translate = Vector3{ 1 };
	Vector3 scale = Vector3{ 1 };
	float rotateAngle = 0.0f;
	Vector3 rotateAxis = Vector3{ 1 };

	glm::mat4 transformMatrix;

};
