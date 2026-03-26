#include "TextComponent.h"

#include "Systems/ShaderSystem.h"
#include "Systems/FontSystem.hpp"
#include "Rendering/Renderer.hpp"
#include "Rendering/Vulkan/VulkanBuffer.hpp"

UTextComponent::UTextComponent() : UPrimitiveComponent(){

}

UTextComponent::~UTextComponent() {
	UComponent::~UComponent();
}

void UTextComponent::Draw() {
	uint32_t TextLength = (uint32_t)Content.Length();
	IRenderer* Renderer = IRenderer::GetRenderer();
	if (TextLength > 0) {
		static const size_t QuadVertCount = 4;
		if (!Renderer->DrawRenderbuffer(VertexBuffer, 0, TextLength * QuadVertCount, true)) {
			GLOG(Log::eError, "Failed to draw ui font vertex buffer.");
		}

		static const unsigned char QuadIndexCount = 6;
		if (!Renderer->DrawRenderbuffer(IndexBuffer, 0, TextLength * QuadIndexCount, false)) {
			GLOG(Log::eError, "Failed to draw ui font index buffer.");
		}
	}
}

bool UTextComponent::Load(UITextType type, const FString& fontName, int fontSize, const FString& textContent) {
	if (fontName.IsEmpty()) {
		GLOG(Log::eError, "UIText::Load() requires a valid fontName.");
		return false;
	}

	Type = type;
	IRenderer* Renderer = IRenderer::GetRenderer();

	// Acquire 返回 IFont*，后续所有数据访问均通过 GetXxx() 接口
	FontSystem& FontSystem = FontSystem::Get();
	FontData = FontSystem.Acquire(fontName, type, fontSize);
	if (!FontData) {
		GLOG(Log::eError, "Unable to acquire font: '%s'. UIText can not be created.", fontName.CStr());
		return false;
	}

	Content = textContent;

	static const size_t QuadSize = sizeof(Vertex2D) * 4;
	uint32_t TextLength = (uint32_t)Content.Length();
	if (TextLength < 1) {
		TextLength = 1;
	}

	// AcquireInstanceResource 需要 TextureMap*，通过 GetAtlas() 取得
	Shader* UIShader = ShaderSystem::Get().Get("Shader.Builtin.UI");
	const TextureMap& Atlas = FontData->GetAtlas();
	std::vector<TextureMap*> FontMaps = { const_cast<TextureMap*>(&Atlas) };
	InstanceID = Renderer->AcquireInstanceResource(UIShader, FontMaps);
	if (InstanceID == INVALID_ID) {
		GLOG(Log::eFatal, "Unable to acquire shader resource for font texture map.");
		return false;
	}

	// 顶点缓冲
	VertexBuffer = (VulkanBuffer*)Memory::Allocate(sizeof(VulkanBuffer), MemoryType::eMemory_Type_Vulkan);
	VertexBuffer = new (VertexBuffer)VulkanBuffer();
	VertexBuffer->Type = EGPUBufferType::eRenderbuffer_Type_Vertex;
	VertexBuffer->TotalSize = TextLength * QuadSize;
	VertexBuffer->UseFreelist = false;
	if (!VertexBuffer->Create()) {
		GLOG(Log::eError, "UIText::Load() Failed to create vertex renderbuffer.");
		return false;
	}
	if (!VertexBuffer->Bind(0)) {
		GLOG(Log::eError, "UIText::Load() Failed to bind vertex renderbuffer.");
		return false;
	}

	// 索引缓冲
	IndexBuffer = (VulkanBuffer*)Memory::Allocate(sizeof(VulkanBuffer), MemoryType::eMemory_Type_Vulkan);
	IndexBuffer = new (IndexBuffer)VulkanBuffer();
	static const unsigned char QuadIndexSize = sizeof(uint32_t) * 6;
	IndexBuffer->Type = EGPUBufferType::eRenderbuffer_Type_Index;
	IndexBuffer->TotalSize = TextLength * QuadIndexSize;
	IndexBuffer->UseFreelist = false;
	if (!IndexBuffer->Create()) {
		GLOG(Log::eError, "UIText::Load() Failed to create index renderbuffer.");
		return false;
	}
	if (!IndexBuffer->Bind(0)) {
		GLOG(Log::eError, "UIText::Load() Failed to bind index renderbuffer.");
		return false;
	}

	// 校验 atlas 是否包含所需字符
	if (!FontSystem.VerifyAtlas(FontData, textContent)) {
		GLOG(Log::eError, "Font atlas verification failed.");
		return false;
	}

	RegenerateGeometry();
	return true;
}

void UTextComponent::SetContent(const FString& text) {
	if (text.IsEmpty() || Content.Equal(text)) {
		return;
	}

	Content = text;

	if (!FontSystem::Get().VerifyAtlas(FontData, Content)) {
		GLOG(Log::eError, "Font atlas verification failed.");
	}

	RegenerateGeometry();
}

void UTextComponent::RegenerateGeometry() {
	uint32_t TextLengthUTF8 = Content.UTF8Length();
	uint32_t CharLength = (uint32_t)Content.Length();

	static const size_t        VertsPerQuad = 4;
	static const unsigned char IndicesPerQuad = 6;
	size_t tVertexBufferSize = sizeof(Vertex2D) * VertsPerQuad * TextLengthUTF8;
	size_t tIndexBufferSize = sizeof(uint32_t) * IndicesPerQuad * TextLengthUTF8;

	if (tVertexBufferSize > VertexBuffer->TotalSize) {
		if (!VertexBuffer->Resize(tVertexBufferSize)) {
			GLOG(Log::eError, "UIText::RegenerateGeometry() failed to resize vertex renderbuffer.");
			return;
		}
	}
	if (tIndexBufferSize > IndexBuffer->TotalSize) {
		if (!IndexBuffer->Resize(tIndexBufferSize)) {
			GLOG(Log::eError, "UIText::RegenerateGeometry() failed to resize index renderbuffer.");
			return;
		}
	}

	// 通过 IFont 接口取得本帧所需的所有字体数据
	const FontGlyph* Glyphs = FontData->GetGlyphs();
	uint32_t           GlyphCount = FontData->GetGlyphCount();
	const FontKerning* Kernings = FontData->GetKernings();
	uint32_t           KerningCount = FontData->GetKerningCount();
	int                LineHeight = FontData->GetLineHeight();
	float              TabXAdvance = FontData->GetTabXAdvance();
	const TextureMap& Atlas = FontData->GetAtlas();

	// atlas 尺寸用于计算 UV，从纹理对象上取
	int AtlasSizeX = Atlas.texture ? Atlas.texture->GetWidth() : 1024;
	int AtlasSizeY = Atlas.texture ? Atlas.texture->GetHeight() : 1024;

	Vertex2D* VertexBufferData = (Vertex2D*)Memory::Allocate(tVertexBufferSize, MemoryType::eMemory_Type_Array);
	uint32_t* IndexBufferData = (uint32_t*)Memory::Allocate(tIndexBufferSize, MemoryType::eMemory_Type_Array);

	float x = 0.f;
	float y = 0.f;

	for (uint32_t c = 0, uc = 0; c < CharLength; ++c) {
		int CodePoint = Content[c];

		if (CodePoint == '\n') {
			x = 0;
			y += LineHeight;
			uc++;
			continue;
		}

		if (CodePoint == '\t') {
			x += TabXAdvance;
			uc++;
			continue;
		}

		// UTF-8 解码
		FCodepointResult Decoded = FString::BytesToCodepoint(Content.CStr(), Content.Length(), c);
		if (!Decoded.bValid) {
			GLOG(Log::eWarn, "Invalid UTF-8 in string, using unknown codepoint -1.");
			CodePoint = -1;
		}

		// 查找对应 glyph
		const FontGlyph* g = nullptr;
		for (uint32_t i = 0; i < GlyphCount; ++i) {
			if (Glyphs[i].codePoint == CodePoint) {
				g = &Glyphs[i];
				break;
			}
		}

		// 找不到则退回到未知字符占位（codepoint -1）
		if (!g) {
			CodePoint = -1;
			for (uint32_t i = 0; i < GlyphCount; ++i) {
				if (Glyphs[i].codePoint == CodePoint) {
					g = &Glyphs[i];
					break;
				}
			}
		}

		if (g) {
			float MinX = x + g->offsetX;
			float MinY = y + g->offsetY;
			float MaxX = MinX + g->width;
			float MaxY = MinY + g->height;
			float tMinX = (float)g->x / AtlasSizeX;
			float tMaxX = (float)(g->x + g->width) / AtlasSizeX;
			float tMinY = (float)g->y / AtlasSizeY;
			float tMaxY = (float)(g->y + g->height) / AtlasSizeY;

			// Bitmap 字体需要翻转 Y 轴
			if (Type == UITextType::eUI_Text_Type_Bitmap) {
				tMinY = 1.0f - tMinY;
				tMaxY = 1.0f - tMaxY;
			}

			Vertex2D p0 = Vertex2D(Vector2f(MinX, MinY), Vector2f(tMinX, tMinY));
			Vertex2D p1 = Vertex2D(Vector2f(MaxX, MaxY), Vector2f(tMaxX, tMaxY));
			Vertex2D p2 = Vertex2D(Vector2f(MaxX, MinY), Vector2f(tMaxX, tMinY));
			Vertex2D p3 = Vertex2D(Vector2f(MinX, MaxY), Vector2f(tMinX, tMaxY));

			VertexBufferData[(uc * 4) + 0] = p0;
			VertexBufferData[(uc * 4) + 1] = p1;
			VertexBufferData[(uc * 4) + 2] = p2;
			VertexBufferData[(uc * 4) + 3] = p3;

			// 查找 kerning
			int Kerning = 0;
			uint32_t Offset = c + Decoded.Advance;
			if (Offset < CharLength - 1) {
				FCodepointResult Next = FString::BytesToCodepoint(Content.CStr(), Content.Length(), Offset);
				if (!Next.bValid) {
					GLOG(Log::eWarn, "Invalid UTF-8 found in string, using unknown codepoint of -1.");
				}
				else {
					for (uint32_t i = 0; i < KerningCount; ++i) {
						if (Kernings[i].codePoint0 == CodePoint &&
							Kernings[i].codePoint1 == Next.Codepoint) {
							Kerning = Kernings[i].amount;
							break;
						}
					}
				}
			}

			x += g->advanceX + Kerning;
		}
		else {
			GLOG(Log::eError, "Unable to find unknown codepoint. Skipping.");
			uc++;
			continue;
		}

		// 索引数据：0, 1, 2, 0, 3, 1
		IndexBufferData[(uc * 6) + 0] = (uc * 4) + 0;
		IndexBufferData[(uc * 6) + 1] = (uc * 4) + 1;
		IndexBufferData[(uc * 6) + 2] = (uc * 4) + 2;
		IndexBufferData[(uc * 6) + 3] = (uc * 4) + 0;
		IndexBufferData[(uc * 6) + 4] = (uc * 4) + 3;
		IndexBufferData[(uc * 6) + 5] = (uc * 4) + 1;

		c += Decoded.Advance - 1;
		uc++;
	}

	bool VertexLoadResult = VertexBuffer->Load(0, tVertexBufferSize, VertexBufferData);
	bool IndexLoadResult = IndexBuffer->Load(0, tIndexBufferSize, IndexBufferData);

	Memory::Free(VertexBufferData, MemoryType::eMemory_Type_Array);
	Memory::Free(IndexBufferData, MemoryType::eMemory_Type_Array);

	if (!VertexLoadResult) {
		GLOG(Log::eError, "UIText::RegenerateGeometry() Failed to load data into vertex buffer.");
	}
	if (!IndexLoadResult) {
		GLOG(Log::eError, "UIText::RegenerateGeometry() Failed to load data into index buffer.");
	}
}

void UTextComponent::Unload() {
	IRenderer* Renderer = IRenderer::GetRenderer();
	VertexBuffer->Destroy();
	DeleteObject(VertexBuffer);
	VertexBuffer = nullptr;

	IndexBuffer->Destroy();
	DeleteObject(IndexBuffer);
	IndexBuffer = nullptr;

	Shader* UIShader = ShaderSystem::Get().Get("Shader.Builtin.UI");
	if (!Renderer->ReleaseInstanceResource(UIShader, InstanceID)) {
		GLOG(Log::eFatal, "Unable to release shader resources for font texture map.");
	}
}