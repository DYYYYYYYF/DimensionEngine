#pragma once

#include "Math/MathTypes.hpp"
#include "Math/Transform.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanBuffer.hpp"
#include "Renderer/Interface/IRenderbuffer.hpp"

enum UITextType {
	eUI_Text_Type_Bitmap,
	eUI_Text_Type_system
};

class UIText {
public:
	UIText() : Data(nullptr), Text(nullptr) {}

	bool Create(class IRenderer* renderer, UITextType type, const char* fontName, unsigned short fontSize, const char* textContent);
	void Destroy();

	void SetPosition(Vec3 position);
	void SetText(const char* text);

	void Draw();

private:
	void RegenerateGeometry();

public:
	IRenderer* Renderer;
	UITextType Type;
	struct FontData* Data;
	VulkanBuffer VertexBuffer;
	VulkanBuffer IndexBuffer;
	char* Text;
	Transform Trans;
	uint32_t InstanceID;
	size_t RenderFrameNumber;
};