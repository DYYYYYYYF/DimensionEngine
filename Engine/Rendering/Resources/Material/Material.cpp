#include "Material.hpp"

Material::Material() {
	ReferenceCount = 0;
	AutoRelease = false;
	ID = INVALID_ID;
	Generation = INVALID_ID;
	InternalID = INVALID_ID;
	DiffuseColor = Vector4(1.0f);
	Shininess = 32.0f;
	ShaderID = INVALID_ID;
	RenderFrameNumer = 0;
	Metallic = 1.0f;
	Roughness = 32.0f;
	AmbientOcclusion = 1.0f;
}

Material::~Material() {}