#include "UIText.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Math/MathTypes.hpp"
#include "Math/Transform.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Renderer/RendererFrontend.hpp"

#include "Systems/FontSystem.hpp"
#include "Systems/ShaderSystem.h"


bool UIText::Create(class IRenderer* renderer, UITextType type, const char* fontName, unsigned short fontSize, const char* textContent) {
	if (fontName == nullptr || textContent == nullptr || textContent[0] == '\0') {
		LOG_ERROR(" UIText::Create() Requires a valid pointer to fontName and textContent.");
		return false;
	}

	// Assign the type first.
	Type = type;
	Renderer = renderer;

	// Acquire the font of the correct type and assign its internal data.
	// This also gets the atlas texture.
	if (!FontSystem::Acquire(fontName, fontSize, this)) {
		LOG_ERROR("Unable to acquire font: '%s'. UIText can not be created.", fontName);
		return false;
	}

	Text = StringCopy(textContent);
	Trans = Transform();
	InstanceID = INVALID_ID;
	RenderFrameNumber = INVALID_ID_U64;

	static const size_t QuadSize = (sizeof(Vertex2D) * 4);

	uint32_t TextLength = (uint32_t)strlen(Text);
	// In the case of an empty string, can not create an empty buffer so just create enough to hold one for now.
	if (TextLength < 1) {
		TextLength = 1;
	}

	// Acquire resource for font texture map.
	Shader* UIShader = ShaderSystem::Get(BUILTIN_SHADER_NAME_UI);	// TODO: Text shader.
	std::vector<TextureMap*> FontMaps = { &Data->atlas };
	InstanceID = renderer->AcquireInstanceResource(UIShader, FontMaps);
	if (InstanceID == INVALID_ID) {
		LOG_FATAL("Unable to acquire shader resource for font texture map.");
		return false;
	}

	// Generate the vertex buffer.
	if (!renderer->CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Vertex, TextLength * QuadSize, false, &VertexBuffer)) {
		LOG_ERROR("UIText::Create() Failed to create vertex renderbuffer.");
		return false;
	}

	if (!renderer->BindRenderbuffer(&VertexBuffer, 0)) {
		LOG_ERROR("UIText::Create() Failed to bind vertex renderbuffer.");
		return false;
	}

	// Generate an index buffer.
	static const unsigned char QuadIndexSize = sizeof(uint32_t) * 6;
	if (!renderer->CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Index, TextLength * QuadIndexSize, false, &IndexBuffer)) {
		LOG_ERROR("UIText::Create() Failed to create index renderbuffer.");
		return false;
	}

	if (!renderer->BindRenderbuffer(&IndexBuffer, 0)) {
		LOG_ERROR("UIText::Create() Failed to bind index renderbuffer.");
		return false;
	}

	// Verify atlas has the glyphs needed.
	if (!FontSystem::VerifyAtlas(Data, textContent)) {
		LOG_ERROR("Font atlas verification failed.");
		return false;
	}

	// Generate geometry.
	RegenerateGeometry();
	return true;
}
void UIText::Destroy() {
	if (Text != nullptr) {
		uint32_t TextLength = (uint32_t)strlen(Text);
		Memory::Free(Text, sizeof(char) * TextLength, MemoryType::eMemory_Type_String);
		Text = nullptr;
	}

	// Destroy buffers.
	Renderer->DestroyRenderbuffer(&VertexBuffer);
	Renderer->DestroyRenderbuffer(&IndexBuffer);

	// Release resources for font texture map.
	Shader* UIShader = ShaderSystem::Get(BUILTIN_SHADER_NAME_UI);	// TODO: Text shader.
	if (!Renderer->ReleaseInstanceResource(UIShader, InstanceID)) {
		LOG_FATAL("Unable to release shader resources for font texture map.");
	}

	Renderer = nullptr;
}
	
void UIText::SetPosition(Vec3 position) {
	Trans.SetPosition(position);
}

void UIText::SetText(const char* text) {
	if (Text) {
		// If strings are already equal, don't do anything.
		if (StringEqual(Text, text)) {
			return;
		}

		uint32_t TextLength = (uint32_t)strlen(Text) + 1;
		Memory::Free(Text, sizeof(char) * TextLength, MemoryType::eMemory_Type_String);
		Text = StringCopy(text);

		// Verify atlas has the glyphs needed
		if (!FontSystem::VerifyAtlas(Data, Text)) {
			LOG_ERROR("Font atlas verification failed.");
		}

		RegenerateGeometry();
	}
}

void UIText::Draw() {
	// TODO: utf8 length.
	uint32_t TextLength = (uint32_t)strlen(Text);
	static const size_t QuadVertCount = 4;
	if (!Renderer->DrawRenderbuffer(&VertexBuffer, 0, TextLength * QuadVertCount, true)) {
		LOG_ERROR("Failed to draw ui font vertex buffer.");
	}

	static const unsigned char QuadIndexCount = 6;
	if (!Renderer->DrawRenderbuffer(&IndexBuffer, 0, TextLength * QuadIndexCount, false)) {
		LOG_ERROR("Failed to draw ui font index buffer.");
	}
}

void UIText::RegenerateGeometry() {
	// Get the UTF-8 string length.
	uint32_t TextLengthUTF8 = StringUTF8Length(Text);
	// Also get the length in characters.
	uint32_t CharLength = (uint32_t)strlen(Text);

	// Calculate buffer size.
	static const size_t VertsPerQuad = 4;
	static const unsigned char IndicesPerQuad = 6;
	size_t tVertexBufferSize = sizeof(Vertex2D) * VertsPerQuad * TextLengthUTF8;
	size_t tIndexBufferSize = sizeof(uint32_t) * IndicesPerQuad * TextLengthUTF8;

	// Resize the vertex buffer, but only if larger.
	if (tVertexBufferSize > VertexBuffer.TotalSize) {
		if (!Renderer->ResizeRenderbuffer(&VertexBuffer, tVertexBufferSize)) {
			LOG_ERROR("UIText::RegenerateGeometry() failed to resize vertex renderbuffer.");
			return;
		}
	}

	// Resize the index buffer, but only if larger.
	if (tIndexBufferSize > IndexBuffer.TotalSize) {
		if (!Renderer->ResizeRenderbuffer(&IndexBuffer, tIndexBufferSize)) {
			LOG_ERROR("UIText::RegenerateGeometry() failed to resize index renderbuffer.");
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
		int CodePoint = Text[c];

		// COntinue to next line for newline.
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
		unsigned char Advance = 0;
		if (!StringBytesToCodepoint(Text, c, &CodePoint, &Advance)) {
			LOG_WARN("Invalid UTF-8 found in string, using unknown codepoint of -1.");
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

			Vertex2D p0 = Vertex2D(Vec2(MinX, MinY), Vec2(tMinX, tMinY));
			Vertex2D p2 = Vertex2D(Vec2(MaxX, MinY), Vec2(tMaxX, tMinY));
			Vertex2D p1 = Vertex2D(Vec2(MaxX, MaxY), Vec2(tMaxX, tMaxY));
			Vertex2D p3 = Vertex2D(Vec2(MinX, MaxY), Vec2(tMinX, tMaxY));

			VertexBufferData[(uc * 4) + 0] = p0;	// 0		2
			VertexBufferData[(uc * 4) + 1] = p1;	//
			VertexBufferData[(uc * 4) + 2] = p2;	//
			VertexBufferData[(uc * 4) + 3] = p3;	// 3		1

			// Try to find kerning
			int Kerning = 0;

			// Get the offset of the next character. If there is no advance, move forward one,
			// otherwise use advance as-is.
			uint32_t Offset = c + Advance;
			if (Offset < TextLengthUTF8 - 1) {
				// Get the next codepoint.
				int NextCodepoint = 0;
				unsigned char NextAdvance = 0;

				if (!StringBytesToCodepoint(Text, Offset, &NextCodepoint, &NextAdvance)) {
					LOG_WARN("Invalid UTF-8 found in string, using unknown codepoint of -1.");
					CodePoint = -1;
				}
				else {
					for (uint32_t i = 0; i < Data->kerningCount; ++i) {
						FontKerning* k = &Data->kernings[i];
						if (k->codePoint0 == CodePoint && k->codePoint1 == NextCodepoint) {
							Kerning = k->amount;
						}
					}
				}
			}

			x += g->advanceX + Kerning;
		}
		else {
			LOG_ERROR("Unable to find unknown codepoint. Skipping.");
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
		c += Advance - 1;
		uc++;
	}

	// Load up the data.
	bool VertexLoadResult = Renderer->LoadRange(&VertexBuffer, 0, tVertexBufferSize, VertexBufferData);
	bool IndexLoadResult = Renderer->LoadRange(&IndexBuffer, 0, tIndexBufferSize, IndexBufferData);

	// Clean up.
	Memory::Free(VertexBufferData, tVertexBufferSize, MemoryType::eMemory_Type_Array);
	Memory::Free(IndexBufferData, tIndexBufferSize, MemoryType::eMemory_Type_Array);

	// Verify results.
	if (!VertexLoadResult) {
		LOG_ERROR("UIText::RegenerateGeometry() Failed to load data into vertex buffer range.");
	}
	if (!IndexLoadResult) {
		LOG_ERROR("UIText::RegenerateGeometry() Failed to load data into index buffer range.");
	}
}
