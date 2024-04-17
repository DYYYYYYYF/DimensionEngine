#pragma once

#include "Defines.hpp"

enum RendererBackendType {
	eRenderer_Backend_Type_Vulkan,
	eRenderer_Backend_Type_OpenGL,
	eRenderer_Backend_Type_DirecX
};

struct SRenderPacket {
	double delta_time;
};