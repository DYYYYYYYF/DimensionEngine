#include "Material.hpp"

Material::Material() {
	ReferenceCount = 0;
	AutoRelease = false;
	ID = INVALID_ID;
	Generation = INVALID_ID;
	InternalID = INVALID_ID;
	ShaderID = INVALID_ID;
	RenderFrameNumer = 0;
}

Material::~Material() {}