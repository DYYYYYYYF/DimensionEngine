#include "Material.hpp"
#include "Systems/MaterialSystem.h"

Material::Material() {
	ReferenceCount = 0;
	AutoRelease = false;
	Generation = INVALID_ID;
	InternalID = INVALID_ID;
	ShaderID = INVALID_ID;
	RenderFrameNumer = 0;
}

Material::~Material() {
	if (ReferenceCount > 0 && AutoRelease) {
		GLOG(Log::eWarn, "Material '%s' is being destroyed while still in use. Reference count: %zu", Name.CStr(), ReferenceCount);
	}
}

void Material::DecreaseReferenceCount(uint32_t count/* = 1*/){
	if (ReferenceCount <= 0) return;

	ReferenceCount -= count; 
	if (ReferenceCount == 0) {
		// Material is no longer in use, trigger cleanup.
		DestroyInstance();
	}
}

void Material::DestroyInstance() {
	MaterialSystem::Get().DestroyMaterial(this);
}
