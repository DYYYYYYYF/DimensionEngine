#include "TextComponent.h"

#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/FontSystem.hpp"
#include "Systems/GeometrySystem.h"
#include "Rendering/Renderer.hpp"
#include "Rendering/Vulkan/VulkanBuffer.hpp"
#include "Rendering/RenderWorld/RenderProxy.h"

UTextComponent::UTextComponent(const FString& Name) : UPrimitiveComponent(Name){
	TextFont = nullptr;
	TextGeometry = nullptr;
	Text = "";
	Color = Vector4(1.0f);
	RenderFrameNumber = INVALID_ID_U64;
}


UTextComponent::~UTextComponent() {
	Destroy();
}

bool UTextComponent::Initialize() {
	if (!BuildGeometry()) {
		return false;
	}

	return true;
}

void UTextComponent::Tick(float deltaTime) {
	if (IsTextDirty) {
		Regenerate();
	}
}

bool UTextComponent::CreateRenderProxy() {
	RenderProxy = NewObject<FTextRenderProxy>(MemoryType::eMemory_Type_Renderer);
	if (!RenderProxy) {
		GLOG(Log::eError, "Failed to create RenderProxy for StaticMeshComponent.");
		return false;
	}

	return true;
}

void UTextComponent::UpdateRenderProxy() {
	// 还未注册到场景
	if (!IsRegistered) return;

	FTextRenderProxy* Proxy = Cast<FTextRenderProxy*>(RenderProxy);
	if (!Proxy) {
		GLOG(Log::eError, "UTextComponent RenderProxy is null. Cannot update.");
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner) {
		GLOG(Log::eError, "TextComponent has no owner.");
		return;
	}

	// 填充数据
	Proxy->SetMesh(TextGeometry);
	Proxy->SetModelMatrix(GetLocalMatrix());
	Proxy->SetUniqueID(Owner->GetUniqueID());
}

bool UTextComponent::Create() {
	return true;
}


void UTextComponent::Destroy() {
	if (TextGeometry) {
		TextGeometry->DecreaseReferenceCount();
		TextGeometry = nullptr;
	}

	TextFont = nullptr;
	RenderFrameNumber = INVALID_ID_U64;
}


bool UTextComponent::SetText(const FString& text) {
	if (Text.Compare(text) == 0) {
		return true;
	}

	Text = text;
	IsTextDirty = true;

	return true;
}


void UTextComponent::SetFont(IFont* font) {
	if (TextFont == font) {
		return;
	}

	TextFont = font;
	IsTextDirty = true;
}


void UTextComponent::SetColor(const Vector4& color) {
	Color = color;
}


bool UTextComponent::Regenerate() {
	if (!BuildGeometry()) {
		return false;
	}

	UpdateRenderProxy();
	return true;
}


bool UTextComponent::BuildGeometry() {
	if (!TextFont) {
		GLOG(Log::eWarn, "UTextComponent::BuildGeometry() Text font is nullptr.");
		return false;
	}

	const uint32_t TextLength = static_cast<uint32_t>(Text.Length());
	if (TextLength == 0) {
		return false;
	}

	// Font 数据
	const FFontGlyph* Glyphs = TextFont->GetGlyphs();
	const uint32_t GlyphCount = TextFont->GetGlyphCount();

	const FFontKerning* Kernings = TextFont->GetKernings();
	const uint32_t KerningCount = TextFont->GetKerningCount();

	const int LineHeight = TextFont->GetLineHeight();
	const float TabXAdvance = TextFont->GetTabXAdvance();

	const int AtlasSizeX = TextFont->GetAtlasWidth();
	const int AtlasSizeY = TextFont->GetAtlasHeight();

	// UTF-8 字节长度，用作最大 Quad 数量。
	// 实际 Quad 数量可能更少，因为：
	//   - 一个 UTF-8 字符可能占多个 byte
	//   - '\n' 不生成 Quad
	//   - '\t' 不生成 Quad
	const uint32_t MaxQuadCount = Text.UTF8Length();
	if (MaxQuadCount == 0) {
		return false;
	}

	const uint32_t VertexSize = sizeof(Vertex2D);
	const uint32_t IndexSize = sizeof(uint32_t);
	const uint32_t MaxVertexCount = MaxQuadCount * 4;
	const uint32_t MaxIndexCount = MaxQuadCount * 6;
	Vertex2D* Vertices = new Vertex2D[MaxVertexCount];
	uint32_t* Indices = new uint32_t[MaxIndexCount];
	float x = 0.0f;
	float y = 0.0f;

	uint32_t QuadIndex = 0;
	for (uint32_t c = 0; c < TextLength;) {
		int CodePoint = Text[c];

		// 换行
		if (CodePoint == '\n') {
			x = 0.0f;
			y += static_cast<float>(LineHeight);
			++c;
			continue;
		}

		// Tab
		if (CodePoint == '\t') {
			x += TabXAdvance;
			++c;
			continue;
		}

		// UTF-8 解码
		FCodepointResult Decoded = FString::BytesToCodepoint(Text.CStr(), Text.Length(), c);
		if (!Decoded.bValid) {
			GLOG(Log::eWarn, "Invalid UTF-8 in string, using unknown codepoint -1.");
			CodePoint = -1;

			// 避免 invalid UTF-8 导致死循环
			++c;
		}
		else {
			CodePoint = Decoded.Codepoint;
		}

		// 查找 Glyph
		const FFontGlyph* Glyph = nullptr;
		for (uint32_t i = 0; i < GlyphCount; ++i) {
			if (Glyphs[i].codePoint == CodePoint) {
				Glyph = &Glyphs[i];
				break;
			}
		}

		// 找不到 Glyph，使用 unknown codepoint
		if (!Glyph) {
			for (uint32_t i = 0; i < GlyphCount; ++i) {
				if (Glyphs[i].codePoint == -1) {
					Glyph = &Glyphs[i];
					break;
				}
			}
		}

		if (!Glyph) {
			GLOG(Log::eError, "Unable to find unknown codepoint. Skipping.");

			// 如果 UTF-8 有效，跳过整个 codepoint；
			// 否则前面已经 ++c。
			if (Decoded.bValid) {
				c += Decoded.Advance;
			}

			continue;
		}

		// 生成 Glyph Quad
		BuildCharacterQuad(QuadIndex, x, y, *Glyph, AtlasSizeX, AtlasSizeY, &Vertices[QuadIndex * 4], &Indices[QuadIndex * 6]);

		// Kerning
		int Kerning = 0;
		if (Decoded.bValid) {
			const uint32_t NextOffset = c + Decoded.Advance;

			if (NextOffset < TextLength) {
				FCodepointResult Next = FString::BytesToCodepoint(Text.CStr(), Text.Length(), NextOffset);

				if (Next.bValid) {
					for (uint32_t i = 0; i < KerningCount; ++i) {
						if (Kernings[i].codePoint0 == CodePoint &&
							Kernings[i].codePoint1 == Next.Codepoint) {
							Kerning = Kernings[i].amount;
							break;
						}
					}
				}
				else {
					GLOG(Log::eWarn, "Invalid UTF-8 found in string, using unknown codepoint of -1.");
				}
			}
		}

		// 更新 pen position
		x += static_cast<float>(Glyph->advanceX + Kerning);
		++QuadIndex;

		// 移动到下一个 UTF-8 codepoint
		if (Decoded.bValid) {
			c += Decoded.Advance;
		}
	}

	// 没有任何可绘制 Glyph
	if (QuadIndex == 0) {
		delete[] Vertices;
		delete[] Indices;
		return false;
	}

	// System 字体（stbtt 打包）的 glyph.offsetY 为负值：字形位图顶部会高出
	// 行顶（即组件位置上方）。若不补偿，文本首行字形会视觉上探，例如
	// Log 面板 Console 页首行被顶到页签栏正下方、紧贴甚至压入页签区域。
	// 这里将整个文本几何向下平移 |minOffsetY|，保证任何字形顶部都不超出
	// 组件位置顶部；位图字体 offsetY 语义不同（非负），不参与补偿。
	if (TextFont->GetFontType() == UITextType::eUI_Text_Type_System) {
		float MinGlyphY = 0.0f;
		for (uint32_t i = 0; i < QuadIndex; ++i) {
			const float TopY = Vertices[i * 4 + 0].position.y;
			if (TopY < MinGlyphY) {
				MinGlyphY = TopY;
			}
		}
		float Dy = 0.0f;
		if (MinGlyphY < 0.0f) {
			Dy = -MinGlyphY;
			for (uint32_t i = 0; i < QuadIndex * 4; ++i) {
				Vertices[i].position.y += Dy;
			}
		}
	}

	// 实际 Geometry 数量
	const uint32_t VertexCount = QuadIndex * 4;
	const uint32_t IndexCount = QuadIndex * 6;
	if (!TextGeometry) {
		FGeometryConfig Config;

		Config.name = GetOwner()->GetName();

		Config.vertex_size = VertexSize;
		Config.vertex_count = VertexCount;
		Config.vertices = Vertices;

		Config.index_size = IndexSize;
		Config.index_count = IndexCount;
		Config.indices = Indices;

		Config.material_name = "Material.Builtin.Text";

		TextGeometry = GeometrySystem::Get().AcquireFromConfig(Config, true);

		if (!TextGeometry) {
			delete[] Vertices;
			delete[] Indices;
			return false;
		}

		// --------------------------------------------------------
		// 设置字体 Atlas
		// --------------------------------------------------------
		UMaterialInstance* TextMaterial = TextGeometry->GetMaterialInstance();

		if (TextMaterial && TextMaterial->IsTextureBindingExist("diffuse_texture")) {
			TextMaterial->SetTextureOnBinding("diffuse_texture", TextFont->GetAtlas());

			// 独立 instance 槽：防止多个文本几何共享父材质槽时，同帧采样器守卫
			// 只更新首个纹理绑定，导致不同字体 atlas 的文本互相串扰（复用首文本纹理）。
			if (!TextMaterial->AcquireIndependentInstanceSlot()) {
				GLOG(Log::eError, "TextComponent: failed to acquire independent instance slot for '%s'.", GetOwner() ? GetOwner()->GetName().CStr() : "?");
			}
		}
	}
	else {
		FGeometryConfig Config;

		Config.vertex_size = VertexSize;
		Config.vertex_count = VertexCount;
		Config.vertices = Vertices;

		Config.index_size = IndexSize;
		Config.index_count = IndexCount;
		Config.indices = Indices;

		IRenderer* Renderer = IRenderer::GetRenderer();

		if (!Renderer->CreateGeometry(TextGeometry, Config)) {
			delete[] Vertices;
			delete[] Indices;
			return false;
		}

		// 如果字体可能发生变化，建议这里也重新设置 Atlas。
		UMaterialInstance* TextMaterial = TextGeometry->GetMaterialInstance();
		if (TextMaterial && TextMaterial->IsTextureBindingExist("diffuse_texture")) {
			TextMaterial->SetTextureOnBinding("diffuse_texture", TextFont->GetAtlas());

			// 独立 instance 槽：防止多个文本几何共享父材质槽时，同帧采样器守卫
			// 只更新首个纹理绑定，导致不同字体 atlas 的文本互相串扰（复用首文本纹理）。
			if (!TextMaterial->AcquireIndependentInstanceSlot()) {
				GLOG(Log::eError, "TextComponent: failed to acquire independent instance slot for '%s'.", GetOwner() ? GetOwner()->GetName().CStr() : "?");
			}
		}
	}

	delete[] Vertices;
	delete[] Indices;

	// 重置脏标记
	IsTextDirty = false;
	RenderFrameNumber = INVALID_ID_U64;

	return true;
}


void UTextComponent::BuildCharacterQuad(uint32_t CharacterIndex, float X, float Y, 
	const FFontGlyph& Glyph, int AtlasSizeX, int AtlasSizeY, Vertex2D* Vertices, uint32_t* Indices) {

	// Glyph 在屏幕/局部空间中的位置
	// offsetX / offsetY 是 Glyph 相对于当前 pen position 的偏移。
	const float MinX = X + static_cast<float>(Glyph.offsetX);
	const float MinY = Y + static_cast<float>(Glyph.offsetY);
	const float MaxX = MinX + static_cast<float>(Glyph.width);
	const float MaxY = MinY + static_cast<float>(Glyph.height);

	// Glyph 在 Font Atlas 中的位置
	float MinU = static_cast<float>(Glyph.x) / static_cast<float>(AtlasSizeX);
	float MaxU = static_cast<float>(Glyph.x + Glyph.width) / static_cast<float>(AtlasSizeX);
	float MinV = static_cast<float>(Glyph.y) / static_cast<float>(AtlasSizeY);
	float MaxV = static_cast<float>(Glyph.y + Glyph.height) / static_cast<float>(AtlasSizeY);

	// Bitmap 字体需要翻转 Y
	if (TextFont->GetFontType() == UITextType::eUI_Text_Type_Bitmap) {
		MinV = 1.0f - MinV;
		MaxV = 1.0f - MaxV;
	}

	// p0 = 左上/MinX MinY  p1 = 右下/MaxX MaxY
	// p2 = 右上/MaxX MinY  p3 = 左下/MinX MaxY
	Vertices[0] = Vertex2D(Vector2(MinX, MinY), Vector2(MinU, MinV));
	Vertices[1] = Vertex2D(Vector2(MaxX, MaxY), Vector2(MaxU, MaxV));
	Vertices[2] = Vertex2D(Vector2(MaxX, MinY), Vector2(MaxU, MinV));
	Vertices[3] = Vertex2D(Vector2(MinX, MaxY), Vector2(MinU, MaxV));

	// Index:
	//   0, 1, 2
	//   0, 3, 1
	const uint32_t BaseVertex = CharacterIndex * 4;
	Indices[0] = BaseVertex + 0;
	Indices[1] = BaseVertex + 1;
	Indices[2] = BaseVertex + 2;
	Indices[3] = BaseVertex + 0;
	Indices[4] = BaseVertex + 3;
	Indices[5] = BaseVertex + 1;
}

void UTextComponent::Draw()
{
	if (!TextGeometry) {
		return;
	}

	GeometryRenderData RenderData;
	RenderData.geometry = TextGeometry;

	IRenderer* Renderer = IRenderer::GetRenderer();
	Renderer->DrawGeometry(&RenderData);
}


const FString& UTextComponent::GetText() const {
	return Text;
}


IFont* UTextComponent::GetFont() const {
	return TextFont;
}


const Vector4& UTextComponent::GetColor() const {
	return Color;
}


UGeometry* UTextComponent::GetGeometry() {
	if (IsTextDirty)
	{
		Regenerate();
	}

	return TextGeometry;
}


size_t UTextComponent::GetFrameNumber() const {
	return RenderFrameNumber;
}


void UTextComponent::SetFrameNumber(size_t frame_number) {
	RenderFrameNumber = frame_number;
}