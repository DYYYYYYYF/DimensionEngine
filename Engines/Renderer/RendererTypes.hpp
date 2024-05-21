#pragma once

#include "Defines.hpp"
#include "Math/MathTypes.hpp"
#include "vector"

enum RendererBackendType {
	eRenderer_Backend_Type_Vulkan,
	eRenderer_Backend_Type_OpenGL,
	eRenderer_Backend_Type_DirecX
};

struct SRenderPacket {
	double delta_time;

	uint32_t geometry_count;
	std::vector<struct GeometryRenderData> geometries;

	uint32_t ui_geometry_count;
	struct GeometryRenderData* ui_geometries = nullptr;
};

struct GeometryRenderData {
	Matrix4 model;
	class Geometry* geometry = nullptr;
};