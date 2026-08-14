#include "RenderDataHelper.h"
#include "Rendering/Renderer.hpp"

Vector2f URenderDataHelper::GetRTSize() {
	return Vector2f(
		(float)IRenderer::GetRenderer()->GetWidth(),
		(float)IRenderer::GetRenderer()->GetHeight()
	);
}