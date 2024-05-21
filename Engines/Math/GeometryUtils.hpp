#pragma once

#include "MathTypes.hpp"

class GeometryUtils {
public:
	static void GenerateNormals(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices);

	static void GenerateTangents(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices);

	static void DeduplicateVertices(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices, uint32_t* out_vertex_count, Vertex** out_vertices);
	static bool VertexEqual(Vertex v0, const Vertex& v1);
	static void ReassignIndex(uint32_t index_count, uint32_t* indices, uint32_t from, uint32_t to);
};
