#pragma once
#include "PrimitiveComponent.h"
#include "Rendering/Resources/Font/Font.hpp"

class IGPUBuffer;
class Geometry;
class Material;

enum class UITextType {
	eUI_Text_Type_Bitmap,
	eUI_Text_Type_system
};

class DAPI UTextComponent : public UPrimitiveComponent {
	DECLARE_CLASS_TYPE(UTextComponent)

	UTextComponent();
	virtual ~UTextComponent();

public:
	virtual void Draw();

	bool Load(UITextType type, const FString& fontName, int fontSize, const FString& textContent);
	void Unload();

public:
	void SetContent(const FString& content);
	FString GetContent()     const { return Content; }
	uint32_t GetContentLength() const { return (uint32_t)Content.Length(); }

	const Vector4& GetColor() const { return Color; }
	void SetColor(Vector4 col) { Color = col; }

	size_t GetFrameNumber() const { return RenderFrameNumber; }
	void SetFrameNumber(size_t num) { RenderFrameNumber = num; }

	IFont* GetFont() { return FontData; }

	void SetInstance(uint32_t id) { InstanceID = id; }
	uint32_t GetInstance() const { return InstanceID; }

	Geometry* GetTextGeometry() const { return TextGeometry; }
	Material* GetFontMaterial() const { return FontMaterial; }

private:
	void RegenerateGeometry();

protected:
	FString Content;
	UITextType     Type = UITextType::eUI_Text_Type_Bitmap;
	Geometry* TextGeometry = nullptr;
	IFont* FontData = nullptr;       
	Material* FontMaterial = nullptr;

	IGPUBuffer* VertexBuffer = nullptr;
	IGPUBuffer* IndexBuffer = nullptr;

	Vector4 Color = Vector4(1.0f);
	size_t  RenderFrameNumber = 0;

	uint32_t InstanceID = INVALID_ID;
};