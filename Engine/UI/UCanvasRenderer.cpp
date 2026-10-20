#include "UCanvasRenderer.h"

#include "Core/CoreMinimal.hpp"
#include "Core/EngineLogger.hpp"

void UCanvasRenderer::Begin(uint32_t InViewportWidth, uint32_t InViewportHeight) {
	this->ViewportWidth = InViewportWidth;
	this->ViewportHeight = InViewportHeight;
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

void UCanvasRenderer::PushText(const FString& Text, float X, float Y, IFont* Font, const Vector4& Color) {
	if (!Font || Text.Length() == 0) {
		return;
	}

	// [FIX] System 字体为动态 atlas，绘制前必须确保非 ASCII 字形（如中文）已加入。
	// Bitmap 字体的 VerifyAtlas 为 no-op。若不调用，中文字符查不到 Glyph 会回退为方框。
	Font->VerifyAtlas(Text);

	const FFontGlyph* Glyphs = Font->GetGlyphs();
	const uint32_t GlyphCount = Font->GetGlyphCount();
	const FFontKerning* Kernings = Font->GetKernings();
	const uint32_t KerningCount = Font->GetKerningCount();

	const FTextureMap& Atlas = Font->GetAtlas();
	const int AtlasSizeX = Atlas.texture ? (int)Atlas.texture->GetWidth() : 1024;
	const int AtlasSizeY = Atlas.texture ? (int)Atlas.texture->GetHeight() : 1024;

	const bool bBitmap = (Font->GetFontType() == UITextType::eUI_Text_Type_Bitmap);

	const uint32_t TextLength = (uint32_t)Text.Length();
	float PenX = 0.0f;
	float PenY = 0.0f;

	for (uint32_t c = 0; c < TextLength;) {
		int CodePoint = Text[c];

		// 换行
		if (CodePoint == '\n') {
			PenX = 0.0f;
			PenY += (float)Font->GetLineHeight();
			++c;
			continue;
		}

		// Tab
		if (CodePoint == '\t') {
			PenX += Font->GetTabXAdvance();
			++c;
			continue;
		}

		// UTF-8 解码
		FCodepointResult Decoded = FString::BytesToCodepoint(Text.CStr(), Text.Length(), c);
		if (!Decoded.bValid) {
			CodePoint = -1;
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
			if (Decoded.bValid) {
				c += Decoded.Advance;
			}
			continue;
		}

		// Glyph 在屏幕/局部空间中的位置
		const float MinX = X + PenX + (float)Glyph->offsetX;
		const float MinY = Y + PenY + (float)Glyph->offsetY;

		// Glyph 在 Font Atlas 中的位置（Bitmap 字体需翻转 V，与 ATextActor 一致）
		float MinU = (float)Glyph->x / (float)AtlasSizeX;
		float MaxU = (float)(Glyph->x + Glyph->width) / (float)AtlasSizeX;
		float MinV = (float)Glyph->y / (float)AtlasSizeY;
		float MaxV = (float)(Glyph->y + Glyph->height) / (float)AtlasSizeY;
		if (bBitmap) {
			MinV = 1.0f - MinV;
			MaxV = 1.0f - MaxV;
		}

		UIDrawCallElement Elem;
		Elem.PositionX = MinX;
		Elem.PositionY = MinY;
		Elem.Width = (float)Glyph->width;
		Elem.Height = (float)Glyph->height;
		Elem.UV0 = Vector2(MinU, MinV);
		Elem.UV1 = Vector2(MaxU, MaxV);
		Elem.Color = Color;
		Elem.Texture = Atlas.texture;
		PushElement(Elem);

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
			}
		}

		// 更新 pen position
		PenX += (float)(Glyph->advanceX + Kerning);
		if (Decoded.bValid) {
			c += Decoded.Advance;
		}
	}
}
