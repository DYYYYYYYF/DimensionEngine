#include "GeometryUtils.hpp"
#include "Core/EngineLogger.hpp"

void GeometryUtils::GenerateNormals(uint32_t vertex_count, Vertex* vertices,
	uint32_t index_count, uint32_t* indices, bool smooth) {
	if (!vertices || !indices || vertex_count == 0 || index_count == 0) {
		return;
	}

	// 如果需要平滑法线，先清零所有法线
	if (smooth) {
		for (uint32_t i = 0; i < vertex_count; ++i) {
			vertices[i].normal = Vector3(0.0f);
		}
	}

	for (uint32_t i = 0; i < index_count; i += 3) {
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		// 边界检查
		if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count) {
			continue;
		}

		Vector3 Edge1 = vertices[i1].position - vertices[i0].position;
		Vector3 Edge2 = vertices[i2].position - vertices[i0].position;
		Vector3 Normal = Edge1.Cross(Edge2);

		// 检查退化三角形
		if (Normal.LengthSquared() < 1e-12f) {
			continue;
		}

		Normal = Normal.Normalized();

		if (smooth) {
			// 平滑法线：累积加权法线
			vertices[i0].normal = vertices[i0].normal + Normal;
			vertices[i1].normal = vertices[i1].normal + Normal;
			vertices[i2].normal = vertices[i2].normal + Normal;
		}
		else {
			// 面法线：直接设置
			vertices[i0].normal = Normal;
			vertices[i1].normal = Normal;
			vertices[i2].normal = Normal;
		}
	}

	// 如果是平滑法线，需要标准化累积的法线
	if (smooth) {
		for (uint32_t i = 0; i < vertex_count; ++i) {
			if (vertices[i].normal.LengthSquared() > 1e-12f) {
				vertices[i].normal = vertices[i].normal.Normalized();
			}
			else {
				// 如果法线为零，设置默认向上法线
				vertices[i].normal = Vector3(0.0f, 1.0f, 0.0f);
			}
		}
	}
}

void GeometryUtils::GenerateTangents(uint32_t vertex_count, Vertex* vertices, 
	uint32_t index_count, uint32_t* indices) {
	if (!vertices || !indices || vertex_count == 0 || index_count == 0) {
		return;
	}

	// 为每个顶点初始化切线和双切线累积器
	std::vector<Vector3> tangents(vertex_count, Vector3(0.0f));
	std::vector<Vector3> bitangents(vertex_count, Vector3(0.0f));

	for (uint32_t i = 0; i < index_count; i += 3) {
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		// 边界检查
		if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count) {
			continue;
		}

		const Vector3& v0 = vertices[i0].position;
		const Vector3& v1 = vertices[i1].position;
		const Vector3& v2 = vertices[i2].position;

		const Vector2& uv0 = vertices[i0].texcoord;
		const Vector2& uv1 = vertices[i1].texcoord;
		const Vector2& uv2 = vertices[i2].texcoord;

		Vector3 Edge1 = v1 - v0;
		Vector3 Edge2 = v2 - v0;

		float DeltaU1 = uv1.x - uv0.x;
		float DeltaV1 = uv1.y - uv0.y;
		float DeltaU2 = uv2.x - uv0.x;
		float DeltaV2 = uv2.y - uv0.y;

		float Determinant = DeltaU1 * DeltaV2 - DeltaU2 * DeltaV1;

		// 检查退化三角形
		if (Dabs(Determinant) < 1e-6f) {
			// 退化三角形，跳过或使用默认切线
			continue;
		}

		float invDet = 1.0f / Determinant;

		Vector3 Tangent = Vector3(
			invDet * (DeltaV2 * Edge1.x - DeltaV1 * Edge2.x),
			invDet * (DeltaV2 * Edge1.y - DeltaV1 * Edge2.y),
			invDet * (DeltaV2 * Edge1.z - DeltaV1 * Edge2.z)
		);

		Vector3 Bitangent = Vector3(
			invDet * (-DeltaU2 * Edge1.x + DeltaU1 * Edge2.x),
			invDet * (-DeltaU2 * Edge1.y + DeltaU1 * Edge2.y),
			invDet * (-DeltaU2 * Edge1.z + DeltaU1 * Edge2.z)
		);

		// 累积到顶点
		tangents[i0] = tangents[i0] + Tangent;
		tangents[i1] = tangents[i1] + Tangent;
		tangents[i2] = tangents[i2] + Tangent;

		bitangents[i0] = bitangents[i0] + Bitangent;
		bitangents[i1] = bitangents[i1] + Bitangent;
		bitangents[i2] = bitangents[i2] + Bitangent;
	}

	// 正交化并存储结果
	for (uint32_t i = 0; i < vertex_count; ++i) {
		const Vector3& n = vertices[i].normal;
		Vector3& t = tangents[i];

		// Gram-Schmidt正交化
		t = (t - n * n.Dot(t)).Normalized();

		// 计算handedness
		float handedness = (n.Cross(t).Dot(bitangents[i]) < 0.0f) ? -1.0f : 1.0f;
		vertices[i].tangent = Vector4(t, handedness);
	}
}

bool GeometryUtils::VertexEqual(Vertex v0, const Vertex& v1) {
	return v0.Compare(v1);
}

bool GeometryUtils::VertexEqualWithTolerance(const Vertex& v0, const Vertex& v1, float tolerance) {
	return v0.position.Compare(v1.position, tolerance) &&
		v0.normal.Compare(v1.normal, tolerance) &&
		v0.texcoord.Compare(v1.texcoord, tolerance) &&
		v0.color.Compare(v1.color, tolerance) &&
		v0.tangent.Compare(v1.tangent, tolerance);
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
	if (!vertices || !indices || !out_vertex_count || !out_vertices || vertex_count == 0) {
		return;
	}

	// 使用哈希表进行快速查找
	struct VertexHash {
		std::size_t operator()(const Vertex& v) const {
			// 简单的哈希函数组合
			auto h1 = std::hash<float>{}(v.position.x);
			auto h2 = std::hash<float>{}(v.position.y);
			auto h3 = std::hash<float>{}(v.position.z);
			auto h4 = std::hash<float>{}(v.texcoord.x);
			auto h5 = std::hash<float>{}(v.texcoord.y);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
		}
	};

	std::unordered_map<Vertex, uint32_t, VertexHash> vertex_map;
	std::vector<Vertex> unique_vertices;
	std::vector<uint32_t> remap_table(vertex_count);

	// 第一遍：建立唯一顶点列表和重映射表
	for (uint32_t i = 0; i < vertex_count; ++i) {
		auto it = vertex_map.find(vertices[i]);
		if (it != vertex_map.end()) {
			// 找到重复顶点
			remap_table[i] = it->second;
		}
		else {
			// 新的唯一顶点
			uint32_t new_index = static_cast<uint32_t>(unique_vertices.size());
			unique_vertices.push_back(vertices[i]);
			vertex_map[vertices[i]] = new_index;
			remap_table[i] = new_index;
		}
	}

	// 第二遍：重新映射索引
	for (uint32_t i = 0; i < index_count; ++i) {
		if (indices[i] < vertex_count) {
			indices[i] = remap_table[indices[i]];
		}
	}

	// 分配输出内存
	*out_vertex_count = static_cast<uint32_t>(unique_vertices.size());
	*out_vertices = static_cast<Vertex*>(Memory::Allocate(
		sizeof(Vertex) * (*out_vertex_count), MemoryType::eMemory_Type_Array));

	// 复制唯一顶点
	Memory::Copy(*out_vertices, unique_vertices.data(),
		sizeof(Vertex) * (*out_vertex_count));

	uint32_t removed_count = vertex_count - *out_vertex_count;
	GLOG(Log::eDebug, "Geometry system de-duplicate vertices: removed %d vertices, original/now %d/%d.",
		removed_count, vertex_count, *out_vertex_count);
}
