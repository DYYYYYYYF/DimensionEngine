#include "GeometryUtils.hpp"

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