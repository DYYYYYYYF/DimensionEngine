#pragma once
#include "Vector.hpp"

template<typename T>
struct TVertex3 {
	TVector3<T> position;
	TVector3<T> normal;
	TVector2<T> texcoord;
	TVector4<T> color;
	TVector4<T> tangent;

	TVertex3() {
		position = TVector3<T>(0.0f);
		normal = TVector3<T>(0, 0, 1);
		texcoord = TVector2<T>(0, 0);
		color = TVector4<T>(1, 1, 1, 1);
		tangent = TVector4<T>(0, 0, 0, 1);
	}

	TVertex3(const TVertex3& v) {
		position = v.position;
		normal = v.normal;
		texcoord = v.texcoord;
		color = v.color;
		tangent = v.tangent;
	}

	bool operator==(const TVertex3& other) const {
		return Compare(other);
	}

	bool operator!=(const TVertex3& other) const {
		return !(*this == other);
	}

	bool operator<(const TVertex3& other) const {
		if (position.x != other.position.x) return position.x < other.position.x;
		if (position.y != other.position.y) return position.y < other.position.y;
		if (position.z != other.position.z) return position.z < other.position.z;

		if (texcoord.x != other.texcoord.x) return texcoord.x < other.texcoord.x;
		if (texcoord.y != other.texcoord.y) return texcoord.y < other.texcoord.y;

		if (normal.x != other.normal.x) return normal.x < other.normal.x;
		if (normal.y != other.normal.y) return normal.y < other.normal.y;
		if (normal.z != other.normal.z) return normal.z < other.normal.z;

		if (color.x != other.color.x) return color.x < other.color.x;
		if (color.y != other.color.y) return color.y < other.color.y;
		if (color.z != other.color.z) return color.z < other.color.z;
		if (color.w != other.color.w) return color.w < other.color.w;

		return false; 
	}

	friend std::ostream& operator<<(std::ostream& os, const TVertex3& v) {
		return os << "Vertex{pos:" << v.position
			<< ", norm:" << v.normal
			<< ", uv:" << v.texcoord
			<< ", color:" << v.color
			<< ", tangent:" << v.tangent << "}";
	}

	bool Compare(const TVertex3& v0) const {
		return position.Compare(v0.position, D_FLOAT_EPSILON) &&
			normal.Compare(v0.normal, D_FLOAT_EPSILON) &&
			texcoord.Compare(v0.texcoord, D_FLOAT_EPSILON) &&
			color.Compare(v0.color, D_FLOAT_EPSILON) &&
			tangent.Compare(v0.tangent, D_FLOAT_EPSILON);
	}
};

template<typename T>
struct TVertex2 {
	TVertex2() {
		position = TVector2<T>();
		texcoord = TVector2<T>();
	}

	TVertex2(const TVector2<T>& pos) {
		position = pos;
		texcoord = TVector2<T>();
	}

	TVertex2(const TVector2<T>& pos, const TVector2<T>& tex) {
		position = pos;
		texcoord = tex;
	}

	bool operator==(const TVertex2& other) const {
		return Compare(other);
	}

	bool operator!=(const TVertex2& other) const {
		return !(*this == other);
	}

	bool operator<(const TVertex2& other) const {
		if (position.x != other.position.x) return position.x < other.position.x;
		if (position.y != other.position.y) return position.y < other.position.y;
		if (position.z != other.position.z) return position.z < other.position.z;

		if (texcoord.x != other.texcoord.x) return texcoord.x < other.texcoord.x;
		if (texcoord.y != other.texcoord.y) return texcoord.y < other.texcoord.y;

		return false;
	}

	friend std::ostream& operator<<(std::ostream& os, const TVertex2& v) {
		return os << "Vertex{pos:" << v.position
			<< ", uv:" << v.texcoord << "}";
	}

	bool Compare(const TVertex2& v0) const {
		return position.Compare(v0.position, D_FLOAT_EPSILON) &&
			texcoord.Compare(v0.texcoord, D_FLOAT_EPSILON) &&
	}

	TVector2<T> position;
	TVector2<T> texcoord;
};
