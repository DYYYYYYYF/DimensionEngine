#include "UCanvasRenderer.h"

void UCanvasRenderer::Begin(uint32_t ViewportWidth, uint32_t ViewportHeight) {
	this->ViewportWidth = ViewportWidth;
	this->ViewportHeight = ViewportHeight;
	Elements.Clear();
	bIsRendering = true;
}

void UCanvasRenderer::End() {
	bIsRendering = false;
}

void UCanvasRenderer::Clear() {
	Elements.Clear();
}

void UCanvasRenderer::PushElement(const UIDrawCallElement& Element) {
	if (Element.Width <= 0.0f || Element.Height <= 0.0f) {
		return;
	}

	Elements.Push(Element);
}
