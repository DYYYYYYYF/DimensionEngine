#pragma once

#include "MathTypes.hpp"

class GeometryUtils {
public:
	static void GenerateNormals(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices);

	static void GenerateTangents(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices);
};
