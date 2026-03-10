#include "TextActor.h"

#include "Systems/ShaderSystem.h"
#include "Systems/FontSystem.hpp"
#include "Rendering/RendererFrontend.hpp"
#include "Rendering/Vulkan/VulkanBuffer.hpp"
#include "Containers/TString.hpp"

ATextActor::ATextActor(UITextType type, const FString& fontName,
	int fontSize, const FString& textContent) {

	if (!Load(type, fontName, fontSize, textContent)) {
		GLOG(Log::Level::eError, "Load font %s failed. font type: %i", fontName.CStr(), (int)type);
		return;
	}
}

void ATextActor::Draw() {

	//ShaderSystem::BindInstance(TextAsset->InstanceID);

	//if (!ShaderSystem::SetUniformByIndex(0, &TextAsset->Data->atlas)) {
	//	GLOG(Log::eError, "Failed to apply bitmap font diffuse map uniform.");
	//	return;
	//}

	//// TODO: font color
	//Vector4 FontColor = TextAsset->GetColor();
	//if (!ShaderSystem::SetUniformByIndex(1, &FontColor)) {
	//	GLOG(Log::eError, "Failed to apply bitmap font diffuse color uniform.");
	//	return;
	//}

	//bool NeedUpdate = TextAsset->RenderFrameNumber != -1;
	//ShaderSystem::ApplyInstance(NeedUpdate);

	//// Sync frame number.
	//TextAsset->RenderFrameNumber = 0;

	//// Apply the locals.
	//Matrix4 Model = TextAsset->GetLocalTransform();
	//if (!ShaderSystem::SetUniformByIndex(2, &Model)) {
	//	GLOG(Log::eError, "Failde to apply model matrix for text.");
	//}

	// TODO: utf8 length.
	uint32_t TextLength = (uint32_t)Content.Length();
	IRenderer* Renderer = IRenderer::GetRenderer();
	if (TextLength > 0) {
		static const size_t QuadVertCount = 4;

		// 内部有判空
		if (!Renderer->DrawRenderbuffer(VertexBuffer, 0, TextLength * QuadVertCount, true)) {
			GLOG(Log::eError, "Failed to draw ui font vertex buffer.");
		}

		static const unsigned char QuadIndexCount = 6;
		if (!Renderer->DrawRenderbuffer(IndexBuffer, 0, TextLength * QuadIndexCount, false)) {
			GLOG(Log::eError, "Failed to draw ui font index buffer.");
		}
	}
}

bool ATextActor::Load(UITextType type, const FString& fontName, int fontSize, const FString& textContent) {
	if (fontName.Length() == 0 || textContent == nullptr || textContent[0] == '\0') {
		GLOG(Log::eError, " UIText::Create() Requires a valid pointer to fontName and textContent.");
		return false;
	}

	// Assign the type first.
	Type = type;
	IRenderer* Renderer = IRenderer::GetRenderer();

	// Acquire the font of the correct type and assign its internal data.
	// This also gets the atlas texture.
	Data = FontSystem::Acquire(fontName, type, fontSize);
	if (!Data) {
		GLOG(Log::eError, "Unable to acquire font: '%s'. UIText can not be created.", fontName.CStr());
		return false;
	}

	Content = textContent;

	static const size_t QuadSize = (sizeof(Vertex2D) * 4);
	uint32_t TextLength = (uint32_t)Content.Length();
	// In the case of an empty string, can not create an empty buffer so just create enough to hold one for now.
	if (TextLength < 1) {
		TextLength = 1;
	}

	// Acquire resource for font texture map.
	Shader* UIShader = ShaderSystem::Get("Shader.Builtin.UI");	// TODO: Text shader.
	std::vector<TextureMap*> FontMaps = { &Data->atlas };
	InstanceID = Renderer->AcquireInstanceResource(UIShader, FontMaps);
	if (InstanceID == INVALID_ID) {
		GLOG(Log::eFatal, "Unable to acquire shader resource for font texture map.");
		return false;
	}

	// Generate the vertex buffer.
	VertexBuffer = (VulkanBuffer*)Memory::Allocate(sizeof(VulkanBuffer), MemoryType::eMemory_Type_Vulkan);
	VertexBuffer = new (VertexBuffer)VulkanBuffer();
	if (!Renderer->CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Vertex, TextLength * QuadSize, false, VertexBuffer)) {
		GLOG(Log::eError, "UIText::Create() Failed to create vertex renderbuffer.");
		return false;
	}

	if (!Renderer->BindRenderbuffer(VertexBuffer, 0)) {
		GLOG(Log::eError, "UIText::Create() Failed to bind vertex renderbuffer.");
		return false;
	}

	// Generate an index buffer.
	IndexBuffer = (VulkanBuffer*)Memory::Allocate(sizeof(VulkanBuffer), MemoryType::eMemory_Type_Vulkan);
	IndexBuffer = new (IndexBuffer)VulkanBuffer();
	static const unsigned char QuadIndexSize = sizeof(uint32_t) * 6;
	if (!Renderer->CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Index, TextLength * QuadIndexSize, false, IndexBuffer)) {
		GLOG(Log::eError, "UIText::Create() Failed to create index renderbuffer.");
		return false;
	}

	if (!Renderer->BindRenderbuffer(IndexBuffer, 0)) {
		GLOG(Log::eError, "UIText::Create() Failed to bind index renderbuffer.");
		return false;
	}

	// Verify atlas has the glyphs needed.
	if (!FontSystem::VerifyAtlas(Data, textContent)) {
		GLOG(Log::eError, "Font atlas verification failed.");
		return false;
	}

	// Generate geometry.
	RegenerateGeometry();

	return true;
}

void ATextActor::SetContent(const FString& text) {
	if (text.IsEmpty() || Content.Equal(text)) {
		return;
	}

	Content = text;

	// Verify atlas has the glyphs needed
	if (!FontSystem::VerifyAtlas(Data, Content)) {
		GLOG(Log::eError, "Font atlas verification failed.");
	}

	RegenerateGeometry();
}

void ATextActor::RegenerateGeometry() {
	// Get the UTF-8 string length.
	uint32_t TextLengthUTF8 = Content.UTF8Length();
	// Also get the length in characters.
	uint32_t CharLength = (uint32_t)Content.Length();

	// Calculate buffer size.
	static const size_t VertsPerQuad = 4;
	static const unsigned char IndicesPerQuad = 6;
	size_t tVertexBufferSize = sizeof(Vertex2D) * VertsPerQuad * TextLengthUTF8;
	size_t tIndexBufferSize = sizeof(uint32_t) * IndicesPerQuad * TextLengthUTF8;

	IRenderer* Renderer = IRenderer::GetRenderer();

	// Resize the vertex buffer, but only if larger.
	if (tVertexBufferSize > VertexBuffer->TotalSize) {
		if (!Renderer->ResizeRenderbuffer(VertexBuffer, tVertexBufferSize)) {
			GLOG(Log::eError, "UIText::RegenerateGeometry() failed to resize vertex renderbuffer.");
			return;
		}
	}

	// Resize the index buffer, but only if larger.
	if (tIndexBufferSize > IndexBuffer->TotalSize) {
		if (!Renderer->ResizeRenderbuffer(IndexBuffer, tIndexBufferSize)) {
			GLOG(Log::eError, "UIText::RegenerateGeometry() failed to resize index renderbuffer.");
			return;
		}
	}

	// Generate new geometry for each character.
	float x = 0;
	float y = 0;
	// Temp arrays to hold vertex/index data.
	Vertex2D* VertexBufferData = (Vertex2D*)Memory::Allocate(tVertexBufferSize, MemoryType::eMemory_Type_Array);
	uint32_t* IndexBufferData = (uint32_t*)Memory::Allocate(tIndexBufferSize, MemoryType::eMemory_Type_Array);

	// Take the length in chars and get the correct codepoint from it.
	for (uint32_t c = 0, uc = 0; c < CharLength; ++c) {
		int CodePoint = Content[c];

		// Continue to next line for newline.
		if (CodePoint == '\n') {
			x = 0;
			y += Data->lineHeight;
			// Increment utf-8 character count.
			uc++;
			continue;
		}

		if (CodePoint == '\t') {
			x += Data->tabXAdvance;
			uc++;
			continue;
		}

		// NOTE: UTF-8 codepoint handing.
		FCodepointResult Decoded = FString::BytesToCodepoint(Content.CStr(), Content.Length(), c);
		if (!Decoded.bValid) {
			GLOG(Log::eWarn, "Invalid UTF-8 found in string, using unknown codepoint of -1.");
			CodePoint = -1;
		}

		FontGlyph* g = nullptr;
		for (uint32_t i = 0; i < Data->glyphCount; ++i) {
			if (Data->glyphs[i].codePoint == CodePoint) {
				g = &Data->glyphs[i];
				break;
			}
		}

		if (g == nullptr) {
			// If not found, use the codepoint -1.
			CodePoint = -1;
			for (uint32_t i = 0; i < Data->glyphCount; ++i) {
				if (Data->glyphs[i].codePoint == CodePoint) {
					g = &Data->glyphs[i];
					break;
				}
			}
		}

		if (g) {
			// Found the glyph. generate points.
			float MinX = x + g->offsetX;
			float MinY = y + g->offsetY;
			float MaxX = MinX + g->width;
			float MaxY = MinY + g->height;
			float tMinX = (float)g->x / Data->atlasSizeX;
			float tMaxX = (float)(g->x + g->width) / Data->atlasSizeX;
			float tMinY = (float)g->y / Data->atlasSizeY;
			float tMaxY = (float)(g->y + g->height) / Data->atlasSizeY;
			// Flip the y axis for system text
			if (Type == UITextType::eUI_Text_Type_Bitmap) {
				tMinY = 1.0f - tMinY;
				tMaxY = 1.0f - tMaxY;
			}

			Vertex2D p0 = Vertex2D(Vector2f(MinX, MinY), Vector2f(tMinX, tMinY));
			Vertex2D p2 = Vertex2D(Vector2f(MaxX, MinY), Vector2f(tMaxX, tMinY));
			Vertex2D p1 = Vertex2D(Vector2f(MaxX, MaxY), Vector2f(tMaxX, tMaxY));
			Vertex2D p3 = Vertex2D(Vector2f(MinX, MaxY), Vector2f(tMinX, tMaxY));

			VertexBufferData[(uc * 4) + 0] = p0;	// 0		2
			VertexBufferData[(uc * 4) + 1] = p1;	//
			VertexBufferData[(uc * 4) + 2] = p2;	//
			VertexBufferData[(uc * 4) + 3] = p3;	// 3		1

			// Try to find kerning
			int Kerning = 0;

			// Get the offset of the next character. If there is no advance, move forward one,
			// otherwise use advance as-is.
			uint32_t Offset = c + Decoded.Advance;
			if (Offset < TextLengthUTF8 - 1) {
				// Get the next codepoint.
				FCodepointResult Next = FString::BytesToCodepoint(Content.CStr(), Content.Length(), Offset);
				if (!Next.bValid) {
					GLOG(Log::eWarn, "Invalid UTF-8 found in string, using unknown codepoint of -1.");
				}
				else {
					for (uint32_t i = 0; i < Data->kerningCount; ++i) {
						FontKerning* k = &Data->kernings[i];
						if (k->codePoint0 == CodePoint && k->codePoint1 == Next.Codepoint) {
							Kerning = k->amount;
							break;
						}
					}
				}
			}

			x += g->advanceX + Kerning;
		}
		else {
			GLOG(Log::eError, "Unable to find unknown codepoint. Skipping.");
			// Increment utf-8 character count.
			uc++;
			continue;
		}

		// Index data 0, 2, 1, 0, 1, 3
		IndexBufferData[(uc * 6) + 0] = (uc * 4) + 0;
		IndexBufferData[(uc * 6) + 1] = (uc * 4) + 1;
		IndexBufferData[(uc * 6) + 2] = (uc * 4) + 2;
		IndexBufferData[(uc * 6) + 3] = (uc * 4) + 0;
		IndexBufferData[(uc * 6) + 4] = (uc * 4) + 3;
		IndexBufferData[(uc * 6) + 5] = (uc * 4) + 1;

		// Now advance c
		c += Decoded.Advance - 1;
		uc++;
	}

	// Load up the data.
	bool VertexLoadResult = Renderer->LoadRange(VertexBuffer, 0, tVertexBufferSize, VertexBufferData);
	bool IndexLoadResult = Renderer->LoadRange(IndexBuffer, 0, tIndexBufferSize, IndexBufferData);

	// Clean up.
	Memory::Free(VertexBufferData, MemoryType::eMemory_Type_Array);
	Memory::Free(IndexBufferData, MemoryType::eMemory_Type_Array);

	// Verify results.
	if (!VertexLoadResult) {
		GLOG(Log::eError, "UIText::RegenerateGeometry() Failed to load data into vertex buffer range.");
	}
	if (!IndexLoadResult) {
		GLOG(Log::eError, "UIText::RegenerateGeometry() Failed to load data into index buffer range.");
	}
}

void ATextActor::Unload() {
	// Destroy buffers.
	IRenderer* Renderer = IRenderer::GetRenderer();
	Renderer->DestroyRenderbuffer(VertexBuffer);
	Renderer->DestroyRenderbuffer(IndexBuffer);

	// Release resources for font texture map.
	Shader* UIShader = ShaderSystem::Get("Shader.Builtin.UI");	// TODO: Text shader.
	if (!Renderer->ReleaseInstanceResource(UIShader, InstanceID)) {
		GLOG(Log::eFatal, "Unable to release shader resources for font texture map.");
	}
}
