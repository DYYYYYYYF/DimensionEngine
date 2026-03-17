#pragma once

#include "Actor.h"
#include "Rendering/Resources/Font/Font.hpp"
#include <vector>

class IFont;
class IGPUBuffer;

enum class UITextType {
	eUI_Text_Type_Bitmap,
	eUI_Text_Type_system
};

class ENGINE_API ATextActor : public AActor {
public:
	DECLARE_CLASS_TYPE(ATextActor)

public:
	ATextActor() : AActor() {}
	ATextActor(const FString& Name) : AActor(Name) {}
	ATextActor(UITextType type, const FString& fontName, int fontSize, const FString& textContent);
	virtual ~ATextActor() { Unload(); }

public:
	virtual void Draw();

	bool Load(UITextType type, const FString& fontName, int fontSize, const FString& textContent);
	void Unload();

public:
	Vector4  GetColor()       const { return Color; }
	void     SetColor(Vector4 col) { Color = col; }

	size_t   GetFrameNumber() const { return RenderFrameNumber; }
	void     SetFrameNumber(size_t num) { RenderFrameNumber = num; }

	void     SetContent(const FString& content);
	FString  GetContent()     const { return Content; }
	uint32_t GetContentLength() const { return (uint32_t)Content.Length(); }

private:
	void RegenerateGeometry();

public:
	UITextType     Type = UITextType::eUI_Text_Type_Bitmap;
	IFont* Data = nullptr;        // 原 IFontDataBase*，改为 IFont*
	IGPUBuffer* VertexBuffer = nullptr;
	IGPUBuffer* IndexBuffer = nullptr;
	FString        Content = nullptr;

	Vector4 Color = Vector4(1.0f);
	size_t  RenderFrameNumber = 0;

	uint32_t InstanceID = INVALID_ID;
};