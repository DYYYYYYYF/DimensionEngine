#pragma once

#include "Actor.h"
#include "Framework/Components/TextComponent.h"

class IFont;
class IGPUBuffer;

class ENGINE_API ATextActor : public AActor {
public:
	DECLARE_CLASS_TYPE(ATextActor)

public:
	ATextActor(const FString& Name);
	ATextActor(const FString& Name, UITextType type, const FString& fontName, int fontSize, const FString& textContent);
	virtual ~ATextActor();

	UTextComponent* GetTextComponent() { return TextComponent; }

public:
	void SetFontSize(uint32_t FontSize);
	uint32_t GetFontSize() const;

	void SetTextFont(const FString& FontName, UITextType Type, uint32_t FontSize = 25);
	void SetTextFont(IFont* Font);

	void SetText(const FString& content);
	FString GetText() const;

protected:
	FString Content;
	UTextComponent* TextComponent;
};