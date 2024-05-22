#include "GeometryUtils.hpp"
#include "Core/EngineLogger.hpp"

void GeometryUtils::GenerateNormals(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices) {
	for (uint32_t i = 0; i < index_count; i+=3) {
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		Vec3 Edge1 = vertices[i1].position - vertices[i0].position;
		Vec3 Edge2 = vertices[i2].position - vertices[i0].position;

		Vec3 Normal = Edge1.Cross(Edge2).Normalize();

		// NOTE: This just generates a face noraml. Smoothing out should be done in a separate pass if desired.
		vertices[i0].normal = Normal;
		vertices[i1].normal = Normal;
		vertices[i2].normal = Normal;
	}
}

void GeometryUtils::GenerateTangents(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices) {
	for (uint32_t i = 0; i < index_count; i+=3) {
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		Vec3 Edge1 = vertices[i1].position - vertices[i0].position;
		Vec3 Edge2 = vertices[i2].position - vertices[i0].position;

		float DeltaU1 = vertices[i1].texcoord.x - vertices[i0].texcoord.x;
		float DeltaV1 = vertices[i1].texcoord.y - vertices[i0].texcoord.y;

		float DeltaU2 = vertices[i2].texcoord.x - vertices[i0].texcoord.x;
		float DeltaV2 = vertices[i2].texcoord.y - vertices[i0].texcoord.y;

		float Dividend = (DeltaU1 * DeltaV2 - DeltaU2 * DeltaV1);
		float FC = 1.0f / Dividend;

		Vec3 Tangent = Vec3{
			(FC * (DeltaV2 * Edge1.x - DeltaV1 * Edge2.x)),
			(FC * (DeltaV2 * Edge1.y - DeltaV1 * Edge2.y)),
			(FC * (DeltaV2 * Edge1.z - DeltaV1 * Edge2.z)),
		};

		Tangent.Normalize();

		float SX = DeltaU1, SY = DeltaU2;
		float TX = DeltaV1, TY = DeltaV2;
		float Handedness = ((TX * SY - TY * SX) < 0.0f) ? -1.0f : 1.0f;
		Vec4 T4 = Vec4(Tangent, Handedness);
		vertices[i0].tangent = T4;
		vertices[i1].tangent = T4;
		vertices[i2].tangent = T4;
	}
}

bool GeometryUtils::VertexEqual(Vertex v0, const Vertex& v1) {
	return v0.Compare(v1);
}

void GeometryUtils::ReassignIndex(uint32_t index_count, uint32_t* indices, uint32_t from, uint32_t to) {
	for (uint32_t i = 0; i < index_count; ++i) {
		if (indices[i] == from) {
			indices[i] = to;
		}
		else if (indices[i] > from) {
			// Pull in all indices higher than 'from' by 1.
			indices[i]--;
		}
	}
}

void GeometryUtils::DeduplicateVertices(uint32_t vertex_count, Vertex* vertices, uint32_t index_count, uint32_t* indices, uint32_t* out_vertex_count, Vertex** out_vertices) {
	if (vertices == nullptr || indices == nullptr || out_vertex_count == nullptr || out_vertices == nullptr) {
		return;
	}
	
	// Create new arrays for the collection to sit in.
	Vertex* UniqueVerts = (Vertex*)Memory::Allocate(sizeof(Vertex) * vertex_count, MemoryType::eMemory_Type_Array);
	*out_vertex_count = 0;

	uint32_t FoundCount = 0;
	for (uint32_t v = 0; v < vertex_count; ++v) {
		bool Found = false;
		for (uint32_t u = 0; u < *out_vertex_count; ++u) {
			if (VertexEqual(UniqueVerts[u], vertices[v])) {
				// Reassign indices, do not copy.
				ReassignIndex(index_count, indices, v - FoundCount, u);
				FoundCount++;
				Found = true;
				break;
			}
		}

		if (!Found) {
			// Copy over to unique.
			UniqueVerts[*out_vertex_count] = vertices[v];
			(*out_vertex_count)++;
		}
	}

	// Allocate new vertices array.
	*out_vertices = (Vertex*)Memory::Allocate(sizeof(Vertex) * (*out_vertex_count), MemoryType::eMemory_Type_Array);
	// Copy over unique.
	Memory::Copy(*out_vertices, UniqueVerts, sizeof(Vertex) * (*out_vertex_count));
	// Destroy temp array.
	Memory::Free(UniqueVerts, sizeof(Vertex) * vertex_count, MemoryType::eMemory_Type_Array);

	uint32_t RemovedCount = vertex_count - *out_vertex_count;
	UL_DEBUG("Geometry system de-duplicate vertices: removed %d vertices, origin/now %d/%d.", RemovedCount, vertex_count, *out_vertex_count);
}
