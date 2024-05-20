#pragma once

#include "Geometry.hpp"
#include "Math/Transform.hpp"

struct Mesh {
	unsigned short geometry_count;
	Geometry** geometries;
	Transform Transform;
};