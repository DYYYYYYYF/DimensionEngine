#pragma once
#include "PrimitiveComponent.h"
#include "Rendering/Resources/Font/Font.hpp"

class UGeometry;

class DAPI UTextComponent : public UPrimitiveComponent
{
	DECLARE_CLASS_TYPE(UTextComponent)

public:
	UTextComponent(const FString& Name);
	~UTextComponent();

	virtual bool Initialize() override;
	virtual void Tick(float deltaTime) override;
	virtual bool CreateRenderProxy() override;
	virtual void UpdateRenderProxy() override;

public:
	bool Create();
	void Destroy();

	const FString& GetText() const;
	bool SetText(const FString& text);

	IFont* GetFont() const;
	void SetFont(IFont* font);

	const Vector4& GetColor() const;
	void SetColor(const Vector4& color);

	bool Regenerate();
	void Draw();

public:
	UGeometry* GetGeometry();
	size_t GetFrameNumber() const;
	void SetFrameNumber(size_t frame_number);

private:
	bool BuildGeometry();
	void BuildCharacterQuad(uint32_t CharacterIndex, float X, float Y,
		const FFontGlyph& Glyph, int AtlasSizeX, int AtlasSizeY, Vertex2D* Vertices, uint32_t* Indices);

private:
	FString Text;
	IFont* TextFont = nullptr;
	Vector4 Color = Vector4(1.0f);
	UGeometry* TextGeometry = nullptr;
	size_t RenderFrameNumber = INVALID_ID_U64;

	float CharacterWidth = 1.0f;
	float CharacterHeight = 1.0f;
	float CharacterSpacing = 0.0f;

	bool IsTextDirty = true;
};