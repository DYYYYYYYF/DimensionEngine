#pragma once

#include "Actor.h"
#include "Framework/Components/TextComponent.h"

class IFont;
class IGPUBuffer;

class ENGINE_API ATextActor : public AActor {
public:
	DECLARE_CLASS_TYPE(ATextActor)

public:
	ATextActor();
	ATextActor(const FString& Name);
	ATextActor(UITextType type, const FString& fontName, int fontSize, const FString& textContent);
	virtual ~ATextActor();

	UTextComponent* GetTextComponent() { return TextComponent; }

public:
	void SetText(const FString& content);
	FString GetText() const;

protected:
	FString Content;
	UTextComponent* TextComponent;
};