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

void UCanvasRenderer::PushClipRect(float X, float Y, float W, float H) {
	if (W <= 0.0f || H <= 0.0f) {
		// 无效裁剪矩形：推入空矩形，使后续元素全部被丢弃
		FClipRect Dummy;
		Dummy.X = 0.0f; Dummy.Y = 0.0f; Dummy.W = 0.0f; Dummy.H = 0.0f;
		ClipStack.Push(Dummy);
		return;
	}
	// 与当前栈顶求交，保持嵌套裁剪单调收窄（空栈视为未裁剪）
	FClipRect C;
	C.X = X; C.Y = Y; C.W = W; C.H = H;
	if (!ClipStack.IsEmpty()) {
		const FClipRect& P = ClipStack[ClipStack.Size() - 1];
		const float L = (X > P.X) ? X : P.X;
		const float T = (Y > P.Y) ? Y : P.Y;
		const float R = (X + W < P.X + P.W) ? X + W : P.X + P.W;
		const float B = (Y + H < P.Y + P.H) ? Y + H : P.Y + P.H;
		C.X = L; C.Y = T; C.W = (R > L) ? R - L : 0.0f; C.H = (B > T) ? B - T : 0.0f;
	}
	ClipStack.Push(C);
}

void UCanvasRenderer::PopClipRect() {
	if (!ClipStack.IsEmpty()) {
		ClipStack.pop_back();
	}
}

void UCanvasRenderer::PushElement(const UIDrawCallElement& Element) {
	if (Element.Width <= 0.0f || Element.Height <= 0.0f) {
		return;
	}

	if (!ClipStack.IsEmpty()) {
		const FClipRect& C = ClipStack[ClipStack.Size() - 1];
		if (C.W <= 0.0f || C.H <= 0.0f) {
			return; // 空裁剪矩形：全部丢弃
		}
		const float L0 = Element.PositionX;
		const float T0 = Element.PositionY;
		const float R0 = Element.PositionX + Element.Width;
		const float B0 = Element.PositionY + Element.Height;
		const float CL = C.X, CT = C.Y, CR = C.X + C.W, CB = C.Y + C.H;
		const float L1 = (L0 > CL) ? L0 : CL;
		const float T1 = (T0 > CT) ? T0 : CT;
		const float R1 = (R0 < CR) ? R0 : CR;
		const float B1 = (B0 < CB) ? B0 : CB;
		if (L1 >= R1 || T1 >= B1) {
			return; // 完全在裁剪区外
		}
		if (L1 != L0 || T1 != T0 || R1 != R0 || B1 != B0) {
			// 部分相交：裁剪矩形并按比例插值 UV，保证字形/纹理不拉伸错位
			UIDrawCallElement E = Element;
			const float U0 = E.UV0.x, V0 = E.UV0.y;
			const float U1 = E.UV1.x, V1 = E.UV1.y;
			const float InvW = (R0 - L0) > 0.0f ? 1.0f / (R0 - L0) : 0.0f;
			const float InvH = (B0 - T0) > 0.0f ? 1.0f / (B0 - T0) : 0.0f;
			const float uL = U0 + (U1 - U0) * (L1 - L0) * InvW;
			const float uR = U0 + (U1 - U0) * (R1 - L0) * InvW;
			const float vT = V0 + (V1 - V0) * (T1 - T0) * InvH;
			const float vB = V0 + (V1 - V0) * (B1 - T0) * InvH;
			E.PositionX = L1;
			E.PositionY = T1;
			E.Width = R1 - L1;
			E.Height = B1 - T1;
			E.UV0 = Vector2(uL, vT);
			E.UV1 = Vector2(uR, vB);
			Elements.Push(E);
			return;
		}
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
	const int AtlasSizeX = Font->GetAtlasWidth();
	const int AtlasSizeY = Font->GetAtlasHeight();
	// [UI-DIAG] 每帧刷屏，验证后已移除；atlas 尺寸统一走 Font->GetAtlasWidth()

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
