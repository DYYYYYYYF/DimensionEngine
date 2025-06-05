#pragma once

#include "MathTypes.hpp"

class GeometryUtils {
public:
	DAPI static void GenerateNormals(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices, bool smooth = false);
	DAPI static void GenerateTangents(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices);
	DAPI static void DeduplicateVertices(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices, uint32_t* out_vertex_count, Vertex** out_vertices);
	DAPI static bool VertexEqual(Vertex v0, const Vertex& v1);
	DAPI static bool VertexEqualWithTolerance(const Vertex& v0, const Vertex& v1, float tolerance = 1e-6f);
	DAPI static void ReassignIndex(uint32_t index_count, uint32_t* indices, uint32_t from, uint32_t to);
};
