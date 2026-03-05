#pragma once

#include "MeshActor.h"
#include "Resources/Font.hpp"
#include <vector>

class IFontDataBase;
class IRenderbuffer;

enum class UITextType {
	eUI_Text_Type_Bitmap,
	eUI_Text_Type_system
};

class ENGINE_API ATextActor : public AMeshActor {
public:
	ATextActor() : AMeshActor(){}
	ATextActor(const FString& Name) : AMeshActor(Name){}
	ATextActor(UITextType type, const FString& fontName, int fontSize, const FString& textContent);
	virtual ~ATextActor() { Unload(); }

public:
	virtual void Draw() override;

	bool Load(UITextType type, const FString& fontName, int fontSize, const FString& textContent);
	void Unload();

public:
	Vector4 GetColor() const { return Color; }
	void SetColor(Vector4 col) { Color = col; }

	size_t GetFrameNumber() const { return RenderFrameNumber; }
	void SetFrameNumber(size_t num) { RenderFrameNumber = num; }

	void SetContent(const FString& content);
	FString GetContent() const { return Content; }
	uint32_t GetContentLength() const { return (uint32_t)Content.Length(); }


private:
	void RegenerateGeometry();

public:
	UITextType Type = UITextType::eUI_Text_Type_Bitmap;
	IFontDataBase* Data = nullptr;
	IRenderbuffer* VertexBuffer = nullptr;
	IRenderbuffer* IndexBuffer = nullptr;
	FString Content = nullptr;

	Vector4 Color = Vector4(1.0f);
	size_t RenderFrameNumber = 0;

	// 需要使用Material
	uint32_t InstanceID = INVALID_ID;

};
