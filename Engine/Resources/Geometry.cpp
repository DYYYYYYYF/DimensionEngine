#include "Geometry.hpp"

#include "Systems/MaterialSystem.h"

void Geometry::ReloadMaterial(const char* mat_name) {
	if (mat_name == nullptr) {
		if (Material == nullptr) {
			return;
		}

		mat_name = Material->Name;
	}

	// Acquire the material.
	if (strlen(mat_name) > 0) {
		Material = MaterialSystem::Reload(mat_name);
		if (Material == nullptr) {
			Material = MaterialSystem::GetDefaultMaterial();
		}
	}
}