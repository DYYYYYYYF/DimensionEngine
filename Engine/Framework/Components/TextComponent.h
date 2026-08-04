#pragma once
#include "PrimitiveComponent.h"
#include "Rendering/Resources/Font/Font.hpp"

class UGeometry;

class UTextComponent : public UPrimitiveComponent
{
	DECLARE_CLASS_TYPE(UTextComponent)

public:
	UTextComponent();
	~UTextComponent();

public:
	bool Create();
	void Destroy();

	bool SetText(const FString& text);
	void SetFont(IFont* font);

	void SetColor(const Vector4& color);

	bool Regenerate();

	void Draw();

public:
	const FString& GetText() const;
	IFont* GetFont() const;
	const Vector4& GetColor() const;

	UGeometry* GetGeometry() const;

	size_t GetFrameNumber() const;
	void SetFrameNumber(size_t frame_number);

private:
	bool BuildGeometry();
	void BuildCharacterQuad(uint32_t CharacterIndex, float X, float Y,
		const FontGlyph& Glyph, int AtlasSizeX, int AtlasSizeY, Vertex2D* Vertices, uint32_t* Indices);

private:
	FString Text;
	IFont* TextFont = nullptr;
	Vector4 Color = Vector4(1.0f);
	UGeometry* TextGeometry = nullptr;
	size_t RenderFrameNumber = INVALID_ID_U64;

	float CharacterWidth = 1.0f;
	float CharacterHeight = 1.0f;
	float CharacterSpacing = 0.0f;
};