#include "TextActor.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Systems/GeometrySystem.h"

void TextActor::Draw() {
	for (uint32_t j = 0; j < geometry_count; j++) {
		GeometryRenderData RenderData;
		RenderData.geometry = geometries[j];
		RenderData.model = GetWorldTransform();
		RenderData.uniqueID = GetUniqueID();
	}
}


void TextActor::Unload() {
	for (uint32_t i = 0; i < geometry_count; ++i) {
		GeometrySystem::Release(geometries[i]);
	}

	Memory::Free(geometries, MemoryType::eMemory_Type_Array);
	geometries = nullptr;

	// For good measure. Invalidate the geometry so it doesn't attemp to be renderer.
	geometry_count = 0;
	Generation = INVALID_ID_U8;
}
